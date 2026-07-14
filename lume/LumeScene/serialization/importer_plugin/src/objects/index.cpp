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

#include "index.h"

#include <scene/ext/scene_utils.h>
#include <scene/interface/resource/types.h>
#include <scene/interface/template/intf_template_options.h>

#include <meta/api/metadata_util.h>
#include <meta/interface/animation/builtin_animations.h>
#include <meta/interface/resource/intf_resource_manager_extension.h>

#include "../config.h"
#include "../import_helpers.h"
#include "../loaders/resource_types.h"
#include "../perf/cpu_perf_scope.h"
#include "../util.h"
#include "property.h"

SCENE_IMP_BEGIN_NAMESPACE()

struct RNamedType {
    BASE_NS::string_view name;
    CORE_NS::ResourceType type;
};

static BASE_NS::vector<RNamedType> TYPES = {{"mesh", SCENE_NS::ClassId::MeshResource.Id().ToUid()},
    {"metallicRoughnessMaterial", SCENE_NS::ClassId::MaterialResource.Id().ToUid()},
    {"unlitMaterial", SCENE_NS::ClassId::MaterialResource.Id().ToUid()},
    {"unlitShadowAlphaMaterial", SCENE_NS::ClassId::MaterialResource.Id().ToUid()},
    {"customMaterial", SCENE_NS::ClassId::MaterialResource.Id().ToUid()},
    {"occlusionMaterial", SCENE_NS::ClassId::OcclusionMaterialResource.Id().ToUid()},
    {"image", SCENE_NS::ClassId::ImageResource.Id().ToUid()},
    {"shader", SCENE_NS::ClassId::ShaderResource.Id().ToUid()},
    {"gltfAnimation", SCENE_NS::ClassId::AnimationResource.Id().ToUid()},
    {"trackAnimation", META_NS::ClassId::AnimationResource.Id().ToUid()},
    {"keyframeAnimation", META_NS::ClassId::AnimationResource.Id().ToUid()},
    {"sequentialAnimation", META_NS::ClassId::AnimationResource.Id().ToUid()},
    {"parallelAnimation", META_NS::ClassId::AnimationResource.Id().ToUid()},
    {"scene", SCENE_NS::ClassId::SceneResource.Id().ToUid()},
    {"gltf", SCENE_NS::ClassId::GltfSceneResource.Id().ToUid()},
    {"environment", SCENE_NS::ClassId::EnvironmentResource.Id().ToUid()},
    {"postProcess", SCENE_NS::ClassId::PostProcessResource.Id().ToUid()},
    {"nodeTemplate", SCENE_NS::ClassId::NodeTemplateResource.Id().ToUid()},
    // resource templates
    {"metallicRoughnessMaterialTemplate", SCENE_NS::ClassId::MaterialResourceTemplate.Id().ToUid()},
    {"unlitMaterialTemplate", SCENE_NS::ClassId::MaterialResourceTemplate.Id().ToUid()},
    {"unlitShadowAlphaMaterialTemplate", SCENE_NS::ClassId::MaterialResourceTemplate.Id().ToUid()},
    {"customMaterialTemplate", SCENE_NS::ClassId::MaterialResourceTemplate.Id().ToUid()},
    {"meshTemplate", SCENE_NS::ClassId::MeshResourceTemplate.Id().ToUid()},
    {"occlusionMaterialTemplate", SCENE_NS::ClassId::OcclusionMaterialResourceTemplate.Id().ToUid()},
    {"postProcessTemplate", SCENE_NS::ClassId::PostProcessResourceTemplate.Id().ToUid()},
    {"animationTemplate", SCENE_NS::ClassId::AnimationResourceTemplate.Id().ToUid()},
    {"environmentTemplate", SCENE_NS::ClassId::EnvironmentResourceTemplate.Id().ToUid()}};

static CORE_NS::ResourceType GetResourceType(BASE_NS::string_view str)
{
    for (auto&& t : TYPES) {
        if (t.name == str) {
            return t.type;
        }
    }
    return {};
}

static IDiagnostics::Ptr ConfigureOptions(ImportContext& context, const CORE_NS::IResourceOptions::Ptr& options)
{
    ErrorHandler h(context);
    if (auto tc = interface_pointer_cast<SCENE_NS::ITemplateOptions>(options)) {
        tc->SetTemplateContext(context.IsTemplateContext());
    }
    // derivedFrom registration is handled centrally in ImportContext::ImportSubType.
    return h;
}

OptValue<CORE_NS::IResourceOptions::Ptr> GetOptionsData(ImportContext& context, BASE_NS::string_view type)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", "OptionsData");
    ErrorHandler h(context);
    // The outer index entry already declares the resource type, so handlers should see
    // the same type inside their options JSON without requiring authors to repeat it.
    // If an inner type is present it must match — mismatches are author errors.
    auto json = context.GetJsonValue();
    if (json.is_object()) {
        if (auto existing = json.find("type")) {
            BASE_NS::string innerStr;
            if (existing->is_string()) {
                innerStr = CORE_NS::json::unescape(existing->string_);
            }
            if (!existing->is_string() || innerStr != type) {
                CORE_LOG_E("Index entry options.type '%s' does not match entry type '%s'",
                    innerStr.c_str(),
                    BASE_NS::string(type).c_str());
                return OptValue<CORE_NS::IResourceOptions::Ptr>{context.CreateDiagnostics(
                    "Index entry options.type '" + innerStr + "' does not match entry type '" + type + "'")};
            }
        } else {
            json["type"] = type;
        }
    }
    auto result = context.ImportSubType(type, BASE_NS::move(json));
    if (result.error && h.Handle(result.error)) {
        return OptValue<CORE_NS::IResourceOptions::Ptr>{result.error};
    }
    // Every import handler now produces a typed template object that implements
    // IResourceOptions directly (MaterialTemplate, ImageTemplate, etc.).
    auto direct = interface_pointer_cast<CORE_NS::IResourceOptions>(result.object);
    if (!direct) {
        CORE_LOG_E("Import handler for '%s' did not produce an IResourceOptions", BASE_NS::string(type).c_str());
        return OptValue<CORE_NS::IResourceOptions::Ptr>{
            context.CreateDiagnostics("Import handler did not produce IResourceOptions for type: " + type)};
    }
    if (auto err = ConfigureOptions(context, direct); h.Handle(err)) {
        return OptValue<CORE_NS::IResourceOptions::Ptr>{err};
    }
    OptValue<CORE_NS::IResourceOptions::Ptr> ret{BASE_NS::move(direct)};
    ret.error = h;
    return ret;
}

// Resolve the resource options for an index entry. Runs the type handler
// whenever the entry declares an `options` block — even an empty `{}` — so the
// produced template carries the resource-type identity (e.g. MaterialType for
// material variants). Returns an empty value (no error) when the entry has no
// `options` block.
static OptValue<CORE_NS::IResourceOptions::Ptr> ResolveEntryOptions(ImportContext& context, BASE_NS::string_view type)
{
    if (!context.GetOptValue("options").is_object()) {
        return OptValue<CORE_NS::IResourceOptions::Ptr>{};
    }
    auto ncont = context.CreateContext(context.GetOptObject("options"));
    return GetOptionsData(ncont, type);
}

IDiagnostics::Ptr ImportIndexEntry(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, META_NS::IResourceManagerExtension& res)
{
    SCENE_IMP_CPU_PERF_SCOPE("ImportIndexEntry", "Entry");
    ErrorHandler h(context);
    auto rid = GetOptResourceId(context, "resourceId");
    if (h.HandleOptValue(rid) && rid.error) {
        return rid.error;
    }
    if (!rid.value) {
        CORE_LOG_E("Index entry missing resourceId");
        return context.CreateDiagnostics("Index entry missing resourceId");
    }
    BASE_NS::string type;
    if (auto err = context.GetRequiredString("type", type)) {
        return err;
    }
    auto name = context.GetOptString("name");
    auto path = context.GetOptString("path");

    CORE_NS::ResourceType rtype = GetResourceType(type);
    if (rtype == CORE_NS::ResourceType{}) {
        CORE_LOG_E("Invalid resource type: %s", type.c_str());
        return context.CreateDiagnostics("Invalid resource type: " + type);
    }

    CORE_NS::IResourceOptions::Ptr opts;
    auto optsCont = ResolveEntryOptions(context, type);
    if (h.HandleOptValue(optsCont) && optsCont.error) {
        return optsCont.error;
    }
    if (optsCont.value) {
        opts = *optsCont.value;
    }

    META_NS::ResourceData d;
    d.version = "JsonSchemaImport";
    d.id = *rid.value;
    d.type = rtype;
    d.path = path;
    d.options = opts;
    d.name = name;

    if (!res.AddResource(BASE_NS::move(d), scene)) {
        CORE_LOG_E("Failed to add resource: %s", rid.value->ToString().c_str());
        return context.CreateDiagnostics("Failed to add resource: " + rid.value->ToString());
    }
    return h;
}

ImportResult ImportResourceIndex(ImportContext& context, CORE_NS::IResourceManager::Ptr& resources)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", "ResourceIndex");
    if (auto err = context.RequireString("type", "resourceIndex")) {
        return ImportResult{err};
    }
    auto extRes = interface_cast<META_NS::IResourceManagerExtension>(resources);
    if (!extRes) {
        CORE_LOG_E("Invalid resource manager");
        return ImportResult{context.CreateDiagnostics("Invalid resource manager")};
    }

    if (auto imp = interface_pointer_cast<IImporter>(context.GetConfig().staticConfig.importer.lock())) {
        RegisterResourceTypes(imp, resources);
    }

    ErrorHandler h(context);
    DeferredResourceScope deferred(context);
    auto scene = context.GetImportParameters().scene;
    auto arr = context.GetOptArray("index");
    for (size_t i = 0; i < arr.size(); ++i) {
        auto ncont = context.CreateContext(arr[i]);
        auto entryTrace = ncont.TraceIndex("index", i);
        if (auto err = ImportIndexEntry(ncont, context.GetImportParameters().scene, *extRes); h.Handle(err)) {
            return ImportResult{err};
        }
    }
    {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "IndexDeferredActions");
        if (auto err = deferred.Execute(h)) {
            return ImportResult{err};
        }
    }
    ImportResult result{interface_pointer_cast<META_NS::IObject>(resources)};
    result.error = h;
    return result;
}

ImportResult ImportIndex::Import(ImportContext& context)
{
    auto resources = interface_pointer_cast<CORE_NS::IResourceManager>(context.GetImportParameters().object);
    if (!resources) {
        resources = SCENE_NS::GetResourceManager(context.GetImportParameters().scene);
    }
    if (!resources) {
        resources = context.GetRenderContext()->GetResources();
    }
    if (!resources) {
        CORE_LOG_E("Invalid state: resource manager not available when loading index");
        return ImportResult{
            context.CreateDiagnostics("Invalid state: resource manager not available when loading index")};
    }
    return SCENE_IMP_NS::ImportResourceIndex(context, resources);
}

SCENE_IMP_END_NAMESPACE()
