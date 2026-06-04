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

#ifndef SCENE_IMP_SRC_IMPORT_CONTEXT_H
#define SCENE_IMP_SRC_IMPORT_CONTEXT_H

#include <functional>
#include <optional>
#include <scene_importer/interface/intf_importer.h>

#include <base/containers/flat_map.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <core/json/json.h>

#include <meta/interface/property/construct_property.h>

#include "config.h"
#include "import_helpers.h"
#include "util.h"

SCENE_IMP_BEGIN_NAMESPACE()

struct ImportContextParameters : ImportParameters {
    META_NS::IObject::Ptr object;
    // Local root for `/`-prefixed paths in `ResolveObject`. Only set at
    // template-instantiation boundaries (TInstantiator, NodeTemplateUpdater) so
    // paths inside an instantiated subtree anchor at that subtree's root rather
    // than at the global scene root. Stays null for normal scene-tree imports
    // so `/` keeps its scene-root semantics there.
    META_NS::IObject::Ptr importRoot;
    size_t fileDepth{0};
    size_t nodeDepth{0};
};

class TracePoint;
class DeferredScope;

class ImportContext {
public:
    ImportContext(const ImportContext& parent, ImportContextParameters params, CORE_NS::json::value json);
    ImportContext(const ImporterConfig& config, ImportContextParameters params, CORE_NS::json::value json,
        BASE_NS::vector<ErrorInfo> trace = {});
    ~ImportContext() = default;

    IDiagnostics::Ptr RequireString(BASE_NS::string_view name, BASE_NS::string_view value);
    IDiagnostics::Ptr GetRequiredString(BASE_NS::string_view name, BASE_NS::string& out);
    BASE_NS::string GetOptString(BASE_NS::string_view name);
    CORE_NS::json::value::array GetOptArray(BASE_NS::string_view name);
    CORE_NS::json::value::object GetOptObject(BASE_NS::string_view name);
    CORE_NS::json::value GetOptValue(BASE_NS::string_view name);

    ImportResult ImportSubType(BASE_NS::string_view type);
    ImportResult ImportSubType(BASE_NS::string_view type, CORE_NS::json::value json);
    ImportResult ImportSubType(BASE_NS::string_view type, CORE_NS::json::value json, ImportContextParameters params);

    ImportContext CreateContext(CORE_NS::json::value json);
    ImportContext CreateContext(CORE_NS::json::value json, ImportContextParameters params);

    IDiagnostics::Ptr CreateDiagnostics(BASE_NS::string_view message);

    SCENE_NS::IRenderContext::Ptr GetRenderContext() const
    {
        return config_.context;
    }
    ImportContextParameters GetImportParameters() const
    {
        return params_;
    }
    const CORE_NS::json::value& GetJsonValue() const
    {
        return jvalue_;
    }
    const ImporterConfig& GetConfig() const
    {
        return config_;
    }
    const BASE_NS::vector<ErrorInfo>& GetTrace() const
    {
        return trace_;
    }

    TracePoint Trace(BASE_NS::string_view message);
    TracePoint TraceIndex(BASE_NS::string_view kind, size_t index);
    void PopTrace();

    bool IsTemplateContext() const
    {
        return isTemplateContext_;
    }
    void SetTemplateContext(bool templateContext)
    {
        isTemplateContext_ = templateContext;
    }
    void AddResourceGroupOverride(const BASE_NS::string& ogroup, const BASE_NS::string& ngroup)
    {
        resGroupOverride_[ogroup] = ngroup;
    }
    std::optional<BASE_NS::string> FindResourceGroupOverride(const BASE_NS::string& ogroup) const
    {
        auto it = resGroupOverride_.find(ogroup);
        return it != resGroupOverride_.end() ? std::optional<BASE_NS::string>(it->second) : std::nullopt;
    }

    using DeferredAction = std::function<IDiagnostics::Ptr()>;
    using DeferredActionList = BASE_NS::vector<DeferredAction>;

    // Tag identifying which scope a deferred action targets. Each DeferredScope
    // is constructed with a tag; AddDeferredAction walks the active scope stack
    // (top-down) and appends to the nearest scope with a matching tag. If no
    // matching scope is on the stack, the action runs immediately.
    //   Resource  - resource manager lookups by id (shaders/images/child anims).
    //               Flushed at end of `LoadResourceIndex`, before scene rootNode.
    //   Hierarchy - scene-hierarchy lookups (objectRef paths, animation property paths).
    //               Flushed at end of `ImportScene`, after rootNode + attachments.
    enum class DeferTier { Resource, Hierarchy };

    void AddDeferredAction(DeferTier tier, DeferredAction action);

    // Returns the nearest scope on the stack whose tag matches `tier`, or
    // nullptr. Used to test whether a tier is currently active and to inherit
    // a parent's scope into a sub-importer (see PushDeferredScope).
    DeferredScope* FindDeferredScope(DeferTier tier) const;

    // Pushes an externally-owned scope onto this context's stack. The pushed
    // scope is not popped by this context; only its owning context manages
    // its lifetime. Use to forward a parent's scope into a fresh sub-context.
    void PushDeferredScope(DeferredScope* scope);

    // Empties the scope stack. Use on a context captured by a deferred action
    // so that further AddDeferredAction calls inside the action's lambda run
    // immediately rather than re-deferring.
    void ClearDeferredScopes();

protected:
    friend class DeferredScope;

    const ImporterConfig& config_;
    ImportContextParameters params_;
    CORE_NS::json::value jvalue_;
    BASE_NS::vector<ErrorInfo> trace_;
    bool isTemplateContext_{};
    BASE_NS::flat_map<BASE_NS::string, BASE_NS::string> resGroupOverride_;
    BASE_NS::vector<DeferredScope*> deferredStack_;
};

class TracePoint {
public:
    explicit TracePoint(ImportContext& c) : context_(c)
    {}
    ~TracePoint()
    {
        context_.PopTrace();
    }

private:
    ImportContext& context_;
};

class ErrorHandler {
public:
    explicit ErrorHandler(const ImportContext& context);

    template <typename T>
    bool HandleOptValue(OptValue<T>& value)
    {
        if (!value.error) {
            return bool(value.value);
        }
        if (!continueOnError_) {
            return true;
        }
        Accumulate(value.error);
        return false;
    }

    bool Handle(const IDiagnostics::Ptr& error);

    operator IDiagnostics::Ptr() const;

private:
    void Accumulate(const IDiagnostics::Ptr& error);

    bool continueOnError_{};
    IDiagnostics::Ptr accumulated_;
};

// Owns a tagged deferred-action list for the lifetime of an import phase.
// Constructor pushes itself onto the context's deferred-scope stack; the
// destructor pops it. Execute() drains the captured actions in registration
// order. Re-entrant scopes nest naturally: AddDeferredAction(tier) lands in
// the *nearest* scope with the matching tag.
class DeferredScope {
public:
    DeferredScope(ImportContext& context, ImportContext::DeferTier tier);
    ~DeferredScope();

    DeferredScope(const DeferredScope&) = delete;
    DeferredScope& operator=(const DeferredScope&) = delete;

    IDiagnostics::Ptr Execute(ErrorHandler& h);

    ImportContext::DeferTier Tier() const
    {
        return tier_;
    }

private:
    friend class ImportContext;

    ImportContext& context_;
    ImportContext::DeferTier tier_{};
    ImportContext::DeferredActionList actions_;
};

// Aliases preserve the call-site spellings used throughout the importer.
struct DeferredResourceScope : DeferredScope {
    explicit DeferredResourceScope(ImportContext& c) : DeferredScope(c, ImportContext::DeferTier::Resource)
    {}
};
struct DeferredHierarchyScope : DeferredScope {
    explicit DeferredHierarchyScope(ImportContext& c) : DeferredScope(c, ImportContext::DeferTier::Hierarchy)
    {}
};

template <typename T>
void SetValue(const ImportContext& context, const META_NS::Property<T>& prop, const T& value)
{
    if (context.IsTemplateContext()) {
        if (prop) {
            prop->SetDefaultValue(value, prop->GetValue() == value);
        }
    } else {
        META_NS::SetValue(prop, value);
    }
}

template <typename T>
void SetValue(const ImportContext& context, const META_NS::ArrayProperty<T>& prop, const BASE_NS::vector<T>& value)
{
    if (context.IsTemplateContext()) {
        if (prop) {
            prop->SetDefaultValue(value, VectorEquals(prop->GetValue(), value));
        }
    } else {
        prop->SetValue(value);
    }
}

template <typename FlagType, size_t N>
IDiagnostics::Ptr ImportFlagsFromArray(ImportContext& context, const CORE_NS::json::value::array& arr,
    const NamedValue<FlagType> (&table)[N], BASE_NS::string_view flagsName, FlagType& out)
{
    ErrorHandler h(context);
    for (auto&& v : arr) {
        if (!v.is_string()) {
            auto err = context.CreateDiagnostics(BASE_NS::string("Invalid ") + flagsName + " entry: expected string");
            if (h.Handle(err)) {
                return err;
            }
        } else if (auto flag = LookupValue(table, v.string_)) {
            out = out | *flag;
        } else {
            CORE_LOG_E("Invalid %s value: %s", BASE_NS::string(flagsName).c_str(), BASE_NS::string(v.string_).c_str());
            auto err = context.CreateDiagnostics(BASE_NS::string("Invalid ") + flagsName + " value: " + v.string_);
            if (h.Handle(err)) {
                return err;
            }
        }
    }
    return h;
}

template <typename EnumType, size_t N>
IDiagnostics::Ptr ImportEnumProperty(ImportContext& context, META_NS::IMetadata& meta, BASE_NS::string_view jsonName,
    BASE_NS::string_view propName, const NamedValue<EnumType> (&table)[N])
{
    ErrorHandler h(context);
    if (auto value = GetOptString(context, jsonName); h.HandleOptValue(value)) {
        if (value.error) {
            return value.error;
        }
        auto result = LookupValue(table, value.GetValue());
        if (!result) {
            CORE_LOG_E("Invalid %s: %s", BASE_NS::string(jsonName).c_str(), value.GetValue().c_str());
            return context.CreateDiagnostics(BASE_NS::string("Invalid ") + jsonName + ": " + value.GetValue());
        }
        auto prop = META_NS::ConstructProperty<EnumType>(propName);
        meta.AddProperty(prop.GetProperty());
        META_NS::SetValue<EnumType>(prop.GetProperty(), *result);
    }
    return h;
}

class TopLevelImportContext : public ImportContext {
public:
    TopLevelImportContext(const ImporterConfig& config, CORE_NS::IFile& input, const ImportContextParameters& params,
        BASE_NS::vector<ErrorInfo> trace = {});

    ImportResult Import(BASE_NS::string_view type = {});

private:
    BASE_NS::string data_;
};

SCENE_IMP_END_NAMESPACE()

#endif