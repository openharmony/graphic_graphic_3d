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

#include "scene.h"

#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/intf_render_resource_manager.h>

#include <meta/interface/resource/intf_owned_resource_groups.h>

#include "../import_helpers.h"
#include "../perf/cpu_perf_scope.h"
#include "attachment.h"
#include "render_configuration.h"

SCENE_IMP_BEGIN_NAMESPACE()

static IDiagnostics::Ptr LoadResourceIndex(ImportContext& context, BASE_NS::string_view resourceIndex)
{
    return LoadFile(context, resourceIndex, "resourceIndex").error;
}

static SCENE_NS::IScene::Ptr ConstructScene(ImportContext& context, const BASE_NS::string& name)
{
    auto m = SCENE_NS::GetSceneManager(context.GetRenderContext());
    if (!m) {
        // Could not get scene manager from context, create a new one
        m = META_NS::GetObjectRegistry().Create<SCENE_NS::ISceneManager>(
            SCENE_NS::ClassId::SceneManager, CreateRenderContextArg(context.GetRenderContext()));
    }
    SCENE_NS::IScene::Ptr scene;
    if (m) {
        scene = m->CreateScene().GetResult();
        if (auto named = interface_cast<META_NS::INamed>(scene)) {
            named->Name()->SetValue(name);
        }
    }
    return scene;
}

static void SetPrimaryGroupOnly(const SCENE_NS::IScene::Ptr& scene, BASE_NS::string_view primary)
{
    if (auto iScene = scene->GetInternalScene()) {
        if (auto c = iScene->GetContext()) {
            if (auto res = interface_pointer_cast<META_NS::IOwnedResourceGroups>(c->GetResources())) {
                auto handle = res->GetGroupHandle(primary, scene);
                auto bundle = iScene->GetResourceGroups();
                bundle = SCENE_NS::ResourceGroupBundle({handle});
                iScene->SetResourceGroups(BASE_NS::move(bundle));
            }
        }
    }
}

// Returns an ImportResult on fatal failure (object null, error set), or nullopt to continue.
static std::optional<ImportResult> ImportSceneRootNode(ImportContext& context, ErrorHandler& h)
{
    auto root = context.GetOptObject("rootNode");
    if (root.empty()) {
        return {};
    }
    auto res = context.ImportSubType("node", CORE_NS::json::value(BASE_NS::move(root)));
    if (!res && h.Handle(res.error)) {
        return res;
    }
    return {};
}

static IDiagnostics::Ptr ImportSceneRenderConfiguration(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, ErrorHandler& h)
{
    auto rcJson = context.GetOptObject("renderConfiguration");
    if (rcJson.empty()) {
        return nullptr;
    }
    auto rcCtx = context.CreateContext(CORE_NS::json::value(BASE_NS::move(rcJson)));
    if (auto err = ImportRenderConfigurationInto(rcCtx, scene); h.Handle(err)) {
        return err;
    }
    return nullptr;
}

// Imports rootNode, renderConfiguration and attachments, then drains the hierarchy-tier
// deferred scope. Returns an ImportResult on fatal failure (object null, error set), or
// nullopt to continue with the success path.
static std::optional<ImportResult> ImportSceneContent(
    ImportContext& context, const SCENE_NS::IScene::Ptr& scene, DeferredHierarchyScope& deferred, ErrorHandler& h)
{
    {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "RootNode");
        if (auto fail = ImportSceneRootNode(context, h)) {
            return *fail;
        }
    }
    {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "RenderConfiguration");
        if (auto err = ImportSceneRenderConfiguration(context, scene, h)) {
            return ImportResult{err};
        }
    }
    {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "Attachments");
        if (auto err = ImportAttachments(context, interface_pointer_cast<META_NS::IObject>(scene)); h.Handle(err)) {
            return ImportResult{err};
        }
    }
    {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "DeferredActions");
        if (auto err = deferred.Execute(h)) {
            return ImportResult{err};
        }
    }
    return {};
}

ImportResult ImportScene::Import(ImportContext& context)
{
    SCENE_IMP_CPU_PERF_SCOPE("Import", "Scene");
    if (auto err = context.RequireString("type", "scene")) {
        return ImportResult{err};
    }
    auto name = context.GetOptString("name");
    auto index = context.GetOptString("index");
    auto group = context.GetOptString("primaryGroup");

    auto scene = ConstructScene(context, name);
    if (!scene) {
        CORE_LOG_E("Failed to create scene");
        return ImportResult{context.CreateDiagnostics("Failed to construct scene")};
    }
    if (!group.empty()) {
        SetPrimaryGroupOnly(scene, group);
    }

    auto params = context.GetImportParameters();
    params.scene = scene;
    auto newContext = context.CreateContext(context.GetJsonValue(), params);

    ErrorHandler h(context);
    // Hierarchy-tier scope wraps LoadResourceIndex so any deferred resolutions registered
    // there (e.g. animation property paths) hold until rootNode is built.
    DeferredHierarchyScope deferred(newContext);
    if (!index.empty()) {
        if (auto err = LoadResourceIndex(newContext, index); h.Handle(err)) {
            return ImportResult{err};
        }
    }

    auto rman = META_NS::GetObjectRegistry().Create<SCENE_NS::IRenderResourceManager>(
        SCENE_NS::ClassId::RenderResourceManager, SCENE_NS::CreateRenderContextArg(context.GetRenderContext()));
    SCENE_NS::IRenderContext::RenderContextScope importScope(context.GetRenderContext(),
        SCENE_NS::IRenderContext::ScopeType::SceneLoad | SCENE_NS::IRenderContext::ScopeType::ImporterDeferred);

    if (auto fail = ImportSceneContent(newContext, scene, deferred, h)) {
        return *fail;
    }

    if (rman) {
        SCENE_IMP_CPU_PERF_SCOPE("Import", "WaitPendingLoads");
        rman->WaitAllPendingLoads();
    }

    ImportResult result{interface_pointer_cast<META_NS::IObject>(scene)};
    result.error = h;
    return result;
}
SCENE_IMP_END_NAMESPACE()
