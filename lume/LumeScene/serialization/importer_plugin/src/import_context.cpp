/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define JSON_IMPL
#include "import_context.h"

#include <core/json/json.h>

#include <meta/interface/resource/intf_resource.h>

#include "diagnostics.h"
#include "import_helpers.h"
#include "perf/cpu_perf_scope.h"

SCENE_IMP_BEGIN_NAMESPACE()

ImportContext::ImportContext(const ImportContext& parent, ImportContextParameters params, CORE_NS::json::value json)
    : config_(parent.config_),
      params_(params),
      jvalue_(BASE_NS::move(json)),
      trace_(parent.trace_),
      isTemplateContext_(parent.isTemplateContext_),
      resGroupOverride_(parent.resGroupOverride_),
      deferredStack_(parent.deferredStack_)
{}

void ImportContext::AddDeferredAction(DeferTier tier, DeferredAction action)
{
    if (auto* scope = FindDeferredScope(tier)) {
        scope->actions_.push_back(BASE_NS::move(action));
    } else {
        action();
    }
}

DeferredScope* ImportContext::FindDeferredScope(DeferTier tier) const
{
    for (auto it = deferredStack_.rbegin(); it != deferredStack_.rend(); ++it) {
        if ((*it)->tier_ == tier) {
            return *it;
        }
    }
    return nullptr;
}

void ImportContext::PushDeferredScope(DeferredScope* scope)
{
    if (scope) {
        deferredStack_.push_back(scope);
    }
}

void ImportContext::ClearDeferredScopes()
{
    deferredStack_.clear();
}

DeferredScope::DeferredScope(ImportContext& context, ImportContext::DeferTier tier) : context_(context), tier_(tier)
{
    context_.deferredStack_.push_back(this);
}

DeferredScope::~DeferredScope()
{
    if (!context_.deferredStack_.empty() && context_.deferredStack_.back() == this) {
        context_.deferredStack_.pop_back();
    }
}

IDiagnostics::Ptr DeferredScope::Execute(ErrorHandler& h)
{
    for (auto&& action : actions_) {
        if (auto err = action(); h.Handle(err)) {
            return err;
        }
    }
    return nullptr;
}

ImportContext::ImportContext(const ImporterConfig& config, ImportContextParameters params, CORE_NS::json::value json,
    BASE_NS::vector<ErrorInfo> trace)
    : config_(config), params_(params), jvalue_(BASE_NS::move(json)), trace_(trace)
{}

IDiagnostics::Ptr ImportContext::CreateDiagnostics(BASE_NS::string_view message)
{
    return MakeSimpleError(BASE_NS::string(message), params_.filename, trace_);
}

IDiagnostics::Ptr ImportContext::RequireString(BASE_NS::string_view name, BASE_NS::string_view value)
{
    BASE_NS::string str;
    if (auto err = GetRequiredString(name, str)) {
        return err;
    }
    if (str != value) {
        BASE_NS::string n(name);
        BASE_NS::string v(value);
        CORE_LOG_E("Invalid value for required string (name: %s) [%s != %s]", n.c_str(), v.c_str(), str.c_str());
        return CreateDiagnostics("Invalid value for required string '" + name + "'");
    }
    return nullptr;
}

IDiagnostics::Ptr ImportContext::GetRequiredString(BASE_NS::string_view name, BASE_NS::string& out)
{
    const auto& v = jvalue_.find(name);
    if (!v || !v->is_string()) {
        BASE_NS::string n(name);
        CORE_LOG_E("Missing required string (name: %s)", n.c_str());
        return CreateDiagnostics("Missing required string '" + n + "'");
    }
    out = CORE_NS::json::unescape(v->string_);
    return nullptr;
}

CORE_NS::json::value::array ImportContext::GetOptArray(BASE_NS::string_view name)
{
    if (const auto& v = jvalue_.find(name)) {
        if (v->is_array()) {
            return v->array_;
        }
    }
    return {};
}

CORE_NS::json::value::object ImportContext::GetOptObject(BASE_NS::string_view name)
{
    if (const auto& v = jvalue_.find(name)) {
        if (v->is_object()) {
            return v->object_;
        }
    }
    return {};
}

BASE_NS::string ImportContext::GetOptString(BASE_NS::string_view name)
{
    BASE_NS::string str;
    if (const auto& v = jvalue_.find(name)) {
        if (v->is_string()) {
            str = CORE_NS::json::unescape(v->string_);
        }
    }
    return str;
}

CORE_NS::json::value ImportContext::GetOptValue(BASE_NS::string_view name)
{
    if (const auto& v = jvalue_.find(name)) {
        return *v;
    }
    return {};
}

ImportResult ImportContext::ImportSubType(BASE_NS::string_view type, CORE_NS::json::value json)
{
    return ImportSubType(type, json, params_);
}
ImportResult ImportContext::ImportSubType(
    BASE_NS::string_view type, CORE_NS::json::value json, ImportContextParameters params)
{
    ImportContext newContext(*this, params, json);
    return newContext.ImportSubType(type);
}
ImportResult ImportContext::ImportSubType(BASE_NS::string_view type)
{
    SCENE_IMP_CPU_PERF_SCOPE("ImportSubType", type);
    auto it = config_.staticConfig.subTypes.find(type);
    if (it == config_.staticConfig.subTypes.end()) {
        BASE_NS::string t(type);
        CORE_LOG_E("Requested unknown object type: %s", t.c_str());
        return ImportResult{CreateDiagnostics("Requested unknown object type: " + t)};
    }
    auto result = it->second->Import(*this);
    // Centralize parse-time derivedFrom registration: any IDerivedResourceOptions result
    // gets its baseResource set here, regardless of whether it came from a top-level
    // index entry or an inline child. Actual base loading is still lazy at
    // ApplyOptions/LoadBaseResource time (top-level), or eager via per-handler logic
    // (inline children) — unifying that is a separate change.
    if (result.object) {
        if (auto derived = interface_pointer_cast<META_NS::IDerivedResourceOptions>(result.object)) {
            if (auto rid = GetOptResourceId(*this, "derivedFrom"); rid.value && rid.value->IsValid()) {
                derived->SetBaseResource(*rid.value);
            }
        }
    }
    return result;
}

ImportContext ImportContext::CreateContext(CORE_NS::json::value json)
{
    return ImportContext(*this, params_, json);
}

ImportContext ImportContext::CreateContext(CORE_NS::json::value json, ImportContextParameters params)
{
    return ImportContext(*this, params, json);
}

TracePoint ImportContext::Trace(BASE_NS::string_view message)
{
    trace_.push_back(Error{BASE_NS::string(message), params_.filename});
    return TracePoint(*this);
}
TracePoint ImportContext::TraceIndex(BASE_NS::string_view kind, size_t index)
{
    return Trace(BASE_NS::string(kind) + "[" + BASE_NS::to_string(index) + "]");
}
void ImportContext::PopTrace()
{
    if (!trace_.empty()) {
        trace_.pop_back();
    }
}

TopLevelImportContext::TopLevelImportContext(const ImporterConfig& config, CORE_NS::IFile& input,
    const ImportContextParameters& params, BASE_NS::vector<ErrorInfo> trace)
    : ImportContext(config, params, {}, BASE_NS::move(trace))
{
    {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "FileRead");
        data_.resize(input.GetLength());
        if (input.Read(data_.data(), data_.size()) != data_.size()) {
            return;
        }
    }
    {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "JsonParse");
        jvalue_ = CORE_NS::json::parse(data_.data());
    }
}

ImportResult TopLevelImportContext::Import(BASE_NS::string_view reqType)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", "TopLevel");
    if (!jvalue_.is_object()) {
        CORE_LOG_E("Invalid file, top-level JSON is not an object (%s)", params_.filename.c_str());
        return ImportResult{
            CreateDiagnostics("Invalid file, top-level JSON is not an object (" + params_.filename + ")")};
    }
    if (auto err = RequireString("version", "1.0")) {
        return ImportResult{err};
    }
    auto data = jvalue_.find("data");
    if (!data || !data->is_object()) {
        CORE_LOG_E("'data' object missing or not an object (%s)", params_.filename.c_str());
        return ImportResult{CreateDiagnostics("'data' object missing or not an object (" + params_.filename + ")")};
    }
    // Use the parent constructor so deferred-action slots configured on the
    // top-level (e.g. by an outer LoadFile call propagating Hierarchy) are
    // inherited by the inner context handling the import.
    ImportContext newContext(*this, params_, *data);

    BASE_NS::string type;
    if (auto err = newContext.GetRequiredString("type", type)) {
        return ImportResult{err};
    }
    if (!reqType.empty() && reqType != type) {
        BASE_NS::string expected(reqType);
        CORE_LOG_E("Wrong type '%s', expected '%s' (%s)", type.c_str(), expected.c_str(), params_.filename.c_str());
        return ImportResult{CreateDiagnostics("Wrong type '" + type + "', expected '" + expected + "'")};
    }

    auto it = config_.staticConfig.topTypes.find(type);
    if (it == config_.staticConfig.topTypes.end()) {
        BASE_NS::string t(type);
        CORE_LOG_E("Requested unknown object type: %s", t.c_str());
        return ImportResult{CreateDiagnostics("Requested unknown object type: " + t)};
    }
    auto trace = newContext.Trace("Import with type '" + type + "'");
    return it->second->Import(newContext);
}

ErrorHandler::ErrorHandler(const ImportContext& context)
    : continueOnError_(context.GetConfig().staticConfig.options.continueOnError)
{}

bool ErrorHandler::Handle(const IDiagnostics::Ptr& error)
{
    if (!error) {
        return false;
    }
    if (!continueOnError_) {
        return true;
    }
    Accumulate(error);
    return false;
}

ErrorHandler::operator IDiagnostics::Ptr() const
{
    return accumulated_;
}

void ErrorHandler::Accumulate(const IDiagnostics::Ptr& error)
{
    MergeDiagnostics(accumulated_, error);
}

SCENE_IMP_END_NAMESPACE()