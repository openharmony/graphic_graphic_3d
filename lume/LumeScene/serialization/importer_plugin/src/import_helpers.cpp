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

#include "import_helpers.h"

#include <scene/ext/scene_utils.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_shader.h>

#include <core/intf_engine.h>
#include <core/json/json.h>

#include <meta/base/type_traits.h>
#include <meta/interface/property/construct_property.h>
#include <meta/interface/resource/intf_resource.h>

#include "diagnostics.h"
#include "import_context.h"
#include "objects/builtin.h"
#include "util.h"

SCENE_IMP_BEGIN_NAMESPACE()

OptValue<BASE_NS::string> GetOptString(ImportContext& context, BASE_NS::string_view name)
{
    auto v = context.GetOptValue(name);
    if (!v) {
        return {};
    }
    if (!v.is_string()) {
        CORE_LOG_W("property '%s' type mismatch, expected string", BASE_NS::string(name).c_str());
        return OptValue<BASE_NS::string>{
            context.CreateDiagnostics(BASE_NS::string("property '" + name + "' type mismatch, expected string"))};
    }
    return OptValue<BASE_NS::string>{CORE_NS::json::unescape(v.string_)};
}

OptValue<float> GetOptFloat(ImportContext& context, BASE_NS::string_view name)
{
    auto v = context.GetOptValue(name);
    if (!v) {
        return {};
    }
    if (!v.is_number()) {
        CORE_LOG_W("property '%s' type mismatch, expected number", BASE_NS::string(name).c_str());
        return OptValue<float>{
            context.CreateDiagnostics(BASE_NS::string("property '" + name + "' type mismatch, expected number"))};
    }
    return OptValue<float>{v.as_number<float>()};
}

OptValue<uint64_t> GetOptUInt(ImportContext& context, BASE_NS::string_view name)
{
    auto v = context.GetOptValue(name);
    if (!v) {
        return {};
    }
    if ((!v.is_signed_int() && !v.is_unsigned_int()) || (v.is_signed_int() && v.signed_ < 0)) {
        CORE_LOG_W("property '%s' type mismatch, expected unsigned integer", BASE_NS::string(name).c_str());
        return OptValue<uint64_t>{context.CreateDiagnostics(
            BASE_NS::string("property '" + name + "' type mismatch, expected unsigned integer"))};
    }
    return OptValue<uint64_t>{v.is_signed_int() ? static_cast<uint64_t>(v.signed_) : v.unsigned_};
}

OptValue<int64_t> GetOptInt(ImportContext& context, BASE_NS::string_view name)
{
    auto v = context.GetOptValue(name);
    if (!v) {
        return {};
    }
    if ((!v.is_signed_int() && !v.is_unsigned_int()) ||
        (v.is_unsigned_int() && v.unsigned_ > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()))) {
        CORE_LOG_W("property '%s' type mismatch, expected signed integer", BASE_NS::string(name).c_str());
        return OptValue<int64_t>{context.CreateDiagnostics(
            BASE_NS::string("property '" + name + "' type mismatch, expected signed integer"))};
    }
    return OptValue<int64_t>{v.is_signed_int() ? v.signed_ : static_cast<int64_t>(v.unsigned_)};
}

OptValue<bool> GetOptBool(ImportContext& context, BASE_NS::string_view name)
{
    auto v = context.GetOptValue(name);
    if (!v) {
        return {};
    }
    if (!v.is_boolean()) {
        CORE_LOG_W("property '%s' type mismatch, expected boolean", BASE_NS::string(name).c_str());
        return OptValue<bool>{
            context.CreateDiagnostics(BASE_NS::string("property '" + name + "' type mismatch, expected boolean"))};
    }
    return OptValue<bool>{v.boolean_};
}

template <typename Type, size_t... Index>
static OptValue<Type> GetOptVecImpl(ImportContext& context, BASE_NS::string_view name, META_NS::IndexSequence<Index...>)
{
    auto arr = context.GetOptArray(name);
    if (arr.empty()) {
        return {};
    }
    if (arr.size() != sizeof...(Index) || (false || ... || !arr[Index].is_number())) {
        return OptValue<Type>{
            context.CreateDiagnostics(BASE_NS::string("invalid property value (property: ") + name + ")")};
    }
    return OptValue<Type>{Type{arr[Index].as_number<float>()...}};
}

template <typename Type, size_t Elements>
static OptValue<Type> GetOptVec(ImportContext& context, BASE_NS::string_view name)
{
    return GetOptVecImpl<Type>(context, name, META_NS::MakeIndexSequence<Elements>());
}

OptValue<BASE_NS::Math::Vec2> GetOptVec2(ImportContext& context, BASE_NS::string_view name)
{
    return GetOptVec<BASE_NS::Math::Vec2, 2>(context, name);
}
OptValue<BASE_NS::Math::Vec3> GetOptVec3(ImportContext& context, BASE_NS::string_view name)
{
    return GetOptVec<BASE_NS::Math::Vec3, 3>(context, name);
}
OptValue<BASE_NS::Math::Quat> GetOptQuat(ImportContext& context, BASE_NS::string_view name)
{
    return GetOptVec<BASE_NS::Math::Quat, 4>(context, name);
}
OptValue<BASE_NS::Math::Vec4> GetOptVec4(ImportContext& context, BASE_NS::string_view name)
{
    return GetOptVec<BASE_NS::Math::Vec4, 4>(context, name);
}
OptValue<BASE_NS::Color> GetOptColor(ImportContext& context, BASE_NS::string_view name)
{
    return GetOptVec<BASE_NS::Color, 4>(context, name);
}

OptValue<BASE_NS::Math::UVec2> GetOptUVec2(ImportContext& context, BASE_NS::string_view name)
{
    auto arr = context.GetOptArray(name);
    if (arr.empty()) {
        return {};
    }
    if (arr.size() != 2 || !arr[0].is_number() || !arr[1].is_number()) {
        CORE_LOG_W("Size or type mismatch with UVec2: %s", BASE_NS::string(name).c_str());
        return OptValue<BASE_NS::Math::UVec2>{
            context.CreateDiagnostics(BASE_NS::string("invalid property value (property: ") + name + ")")};
    }
    return OptValue<BASE_NS::Math::UVec2>{
        BASE_NS::Math::UVec2{arr[0].as_number<uint32_t>(), arr[1].as_number<uint32_t>()}};
}

static OptValue<BASE_NS::Math::Vec4> ParseMat4Column(
    ImportContext& context, const CORE_NS::json::value& col, BASE_NS::string_view name)
{
    if (!col.is_array() || col.array_.size() != 4) {
        CORE_LOG_W("Invalid size with Mat4X4: %s", BASE_NS::string(name).c_str());
        return OptValue<BASE_NS::Math::Vec4>{
            context.CreateDiagnostics(BASE_NS::string("invalid matrix size (property: ") + name + ")")};
    }
    auto& a = col.array_;
    if (!a[0].is_number() || !a[1].is_number() || !a[2].is_number() || !a[3].is_number()) {
        CORE_LOG_W("Type mismatch with Mat4X4: %s", BASE_NS::string(name).c_str());
        return OptValue<BASE_NS::Math::Vec4>{
            context.CreateDiagnostics(BASE_NS::string("invalid property value (property: ") + name + ")")};
    }
    return OptValue<BASE_NS::Math::Vec4>{BASE_NS::Math::Vec4{
        a[0].as_number<float>(), a[1].as_number<float>(), a[2].as_number<float>(), a[3].as_number<float>()}};
}

OptValue<BASE_NS::Math::Mat4X4> GetOptMat4x4(ImportContext& context, BASE_NS::string_view name)
{
    auto arr = context.GetOptArray(name);
    if (arr.empty()) {
        return {};
    }
    if (arr.size() != 4) {
        CORE_LOG_W("Invalid size with Mat4X4: %s", BASE_NS::string(name).c_str());
        return OptValue<BASE_NS::Math::Mat4X4>{
            context.CreateDiagnostics(BASE_NS::string("invalid matrix size (property: ") + name + ")")};
    }
    BASE_NS::Math::Vec4 cols[4];
    for (size_t i = 0; i < 4; ++i) {
        auto col = ParseMat4Column(context, arr[i], name);
        if (col.error) {
            return OptValue<BASE_NS::Math::Mat4X4>{col.error};
        }
        if (col.value) {
            cols[i] = *col.value;
        }
    }
    return OptValue<BASE_NS::Math::Mat4X4>{BASE_NS::Math::Mat4X4{cols[0], cols[1], cols[2], cols[3]}};
}

OptValue<META_NS::ObjectId> GetOptObjectId(ImportContext& context, BASE_NS::string_view name)
{
    auto str = context.GetOptString(name);
    if (str.empty()) {
        return {};
    }
    if (!BASE_NS::IsUidString(str)) {
        return OptValue<META_NS::ObjectId>{
            context.CreateDiagnostics(BASE_NS::string("invalid uid value '") + str + "' (property: " + name + ")")};
    }
    return OptValue<META_NS::ObjectId>{META_NS::ObjectId(BASE_NS::StringToUid(str))};
}

OptValue<CORE_NS::ResourceId> GetOptResourceId(ImportContext& context)
{
    auto name = context.GetOptString("name");
    auto group = context.GetOptString("group");

    CORE_NS::ResourceId rid(name, group);
    if (!rid.IsValid()) {
        CORE_LOG_E("Invalid resourceId (name: '%s', group: '%s')", name.c_str(), group.c_str());
        return OptValue<CORE_NS::ResourceId>{
            context.CreateDiagnostics("Invalid resourceId (name: '" + name + "', group: '" + group + "')")};
    }
    if (auto overr = context.FindResourceGroupOverride(group)) {
        rid.group = *overr;
    }
    return OptValue<CORE_NS::ResourceId>{rid};
}

OptValue<CORE_NS::ResourceId> GetOptResourceId(ImportContext& context, BASE_NS::string_view name)
{
    CORE_NS::ResourceId rid;
    auto ridObj = context.GetOptObject(name);
    if (ridObj.empty()) {
        return {};
    }
    auto ncont = context.CreateContext(BASE_NS::move(ridObj));
    return GetOptResourceId(ncont);
}

CORE_NS::IResource::Ptr ResolveDeferredResource(const ImportContextParameters& params,
    const SCENE_NS::IRenderContext::Ptr& renderContext, const CORE_NS::ResourceId& resourceId)
{
    auto resources = interface_pointer_cast<CORE_NS::IResourceManager>(params.object);
    if (!resources) {
        resources = SCENE_NS::GetResourceManager(params.scene);
    }
    if (!resources) {
        resources = renderContext->GetResources();
    }
    if (!resources) {
        return nullptr;
    }
    CORE_NS::ResourceIdContext pres{resourceId, params.scene};
    auto resource = resources->GetResource(pres);
    if (!resource) {
        pres.context = nullptr;
        resource = resources->GetResource(pres);
    }
    return resource;
}

struct DeferredContext {
    ImportContextParameters params;
    SCENE_NS::IRenderContext::Ptr renderContext;
    BASE_NS::string filename;
    BASE_NS::vector<ErrorInfo> trace;

    IDiagnostics::Ptr CreateError(BASE_NS::string_view message) const
    {
        return MakeSimpleError(BASE_NS::string(message), filename, trace);
    }
};

static DeferredContext MakeDeferredContext(ImportContext& context)
{
    return {context.GetImportParameters(),
        context.GetRenderContext(),
        context.GetImportParameters().filename,
        context.GetTrace()};
}

IDiagnostics::Ptr ImportImageProperty(ImportContext& context, const META_NS::IObject::Ptr& metaOwner,
    BASE_NS::string_view jsonName, BASE_NS::string_view propName)
{
    ErrorHandler h(context);
    if (auto rid = GetOptResourceId(context, jsonName); h.HandleOptValue(rid)) {
        if (rid.error) {
            return rid.error;
        }
        auto resourceId = rid.GetValue();
        auto pName = BASE_NS::string(propName);
        auto dc = MakeDeferredContext(context);
        context.AddDeferredAction(
            ImportContext::DeferTier::Resource, [metaOwner, resourceId, pName, dc]() -> IDiagnostics::Ptr {
                auto resource = ResolveDeferredResource(dc.params, dc.renderContext, resourceId);
                if (!resource) {
                    CORE_LOG_E("Failed to resolve image resource: %s (property: %s)",
                        resourceId.ToString().c_str(),
                        pName.c_str());
                    return dc.CreateError("Failed to resolve image resource '" + resourceId.ToString() +
                                          "' for property '" + pName + "'");
                }
                auto image = interface_pointer_cast<SCENE_NS::IImage>(resource);
                if (!image) {
                    CORE_LOG_E(
                        "Resource is not an image: %s (property: %s)", resourceId.ToString().c_str(), pName.c_str());
                    return dc.CreateError(
                        "Resource is not an image: '" + resourceId.ToString() + "' for property '" + pName + "'");
                }
                if (auto meta = interface_cast<META_NS::IMetadata>(metaOwner)) {
                    auto prop = META_NS::ConstructProperty<SCENE_NS::IImage::Ptr>(pName);
                    meta->AddProperty(prop.GetProperty());
                    META_NS::SetValue<SCENE_NS::IImage::Ptr>(prop.GetProperty(), image);
                }
                return nullptr;
            });
    }
    return h;
}

IDiagnostics::Ptr ImportEnvironmentProperty(
    ImportContext& context, const META_NS::IProperty::Ptr& target, BASE_NS::string_view jsonName)
{
    if (!target) {
        return context.CreateDiagnostics("Environment target property is null");
    }
    ErrorHandler h(context);
    auto rid = GetOptResourceId(context, jsonName);
    if (!h.HandleOptValue(rid)) {
        return h;
    }
    if (rid.error) {
        return rid.error;
    }
    auto resourceId = rid.GetValue();
    auto pName = BASE_NS::string(jsonName);
    auto dc = MakeDeferredContext(context);
    context.AddDeferredAction(
        ImportContext::DeferTier::Resource, [target, resourceId, pName, dc]() -> IDiagnostics::Ptr {
            auto resource = ResolveDeferredResource(dc.params, dc.renderContext, resourceId);
            if (!resource) {
                CORE_LOG_E("Failed to resolve environment resource: %s (property: %s)",
                    resourceId.ToString().c_str(),
                    pName.c_str());
                return dc.CreateError("Failed to resolve environment resource '" + resourceId.ToString() +
                                      "' for property '" + pName + "'");
            }
            auto env = interface_pointer_cast<SCENE_NS::IEnvironment>(resource);
            if (!env) {
                CORE_LOG_E(
                    "Resource is not an environment: %s (property: %s)", resourceId.ToString().c_str(), pName.c_str());
                return dc.CreateError(
                    "Resource is not an environment: '" + resourceId.ToString() + "' for property '" + pName + "'");
            }
            META_NS::SetValue<SCENE_NS::IEnvironment::Ptr>(target, env);
            return nullptr;
        });
    return h;
}

BASE_NS::string ResolveFilename(ImportContext& context, BASE_NS::string_view resourceIndex)
{
    if (resourceIndex.find("://") != BASE_NS::string_view::npos) {
        return BASE_NS::string(resourceIndex);
    }
    auto params = context.GetImportParameters();
    if (params.filename.find("://") != BASE_NS::string_view::npos) {
        return SCENE_NS::ParentPath(params.filename) + "/" + resourceIndex;
    }
    return BASE_NS::string(resourceIndex);
}

ImportResult LoadFile(ImportContext& context, BASE_NS::string_view file, BASE_NS::string_view type)
{
    auto filename = ResolveFilename(context, file);
    BASE_NS::string typeStr(type);
    auto trace = context.Trace("Loading file '" + filename + "' as type '" + typeStr + "'");
    auto& rm = context.GetRenderContext()->GetRenderer()->GetEngine().GetFileManager();
    auto f = rm.OpenFile(filename);
    if (!f) {
        CORE_LOG_E("Failed to open file: %s", filename.c_str());
        return ImportResult{context.CreateDiagnostics("Failed to open file: " + filename)};
    }
    constexpr size_t MAX_FILE_DEPTH = 16;
    ImportContextParameters p = context.GetImportParameters();
    if (p.fileDepth >= MAX_FILE_DEPTH) {
        CORE_LOG_E("Maximum import depth exceeded when loading: %s", filename.c_str());
        return ImportResult{context.CreateDiagnostics("Maximum import depth exceeded when loading: " + filename)};
    }
    p.filename = filename;
    p.fileDepth++;

    TopLevelImportContext sc(context.GetConfig(), *f, p, context.GetTrace());
    // Hierarchy-tier deferred actions registered while loading a nested file
    // (e.g. animation property paths in a separate .index file) must reach the
    // outer scene's Hierarchy scope so they fire after the rootNode is built.
    // Resource-tier actions stay local to each file's own index loading.
    sc.PushDeferredScope(context.FindDeferredScope(ImportContext::DeferTier::Hierarchy));
    return sc.Import(type);
}

SCENE_IMP_END_NAMESPACE()