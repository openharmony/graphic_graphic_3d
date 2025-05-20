/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "scene_manager.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/util.h>
#include <scene/interface/intf_application_context.h>
#include <scene/interface/intf_environment.h>
#include <scene/interface/intf_image.h>
#include <scene/interface/intf_material.h>
#include <scene/interface/intf_postprocess.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/intf_scene_manager.h>
#include <scene/interface/intf_shader.h>
#include <scene/interface/resource/types.h>

#include <core/intf_engine.h>
#include <render/intf_render_context.h>

#include <meta/api/task_queue.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_hierarchy_observer.h>
#include <meta/interface/resource/intf_object_resource.h>

#include "asset/asset_object.h"
#include "resource/resource_types.h"

SCENE_BEGIN_NAMESPACE()

bool SceneManager::Build(const META_NS::IMetadata::Ptr& d)
{
    bool res = Super::Build(d);
    if (res) {
        if (d) {
            context_ = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
            opts_ = GetBuildArg<SceneOptions>(d, "Options");
        }
        if (!context_) {
            if (auto app = GetDefaultApplicationContext()) {
                CORE_LOG_D("Using default context for scene manager");
                context_ = GetDefaultApplicationContext()->GetRenderContext();
                if (!context_) {
                    CORE_LOG_W("No default context set");
                    return false;
                }
                // use opts from app context if we are using it
                opts_ = app->GetDefaultSceneOptions();
            }
        }
        RegisterResourceTypes(context_, opts_);
    }
    return res;
}

META_NS::IMetadata::Ptr SceneManager::CreateContext(SceneOptions opts) const
{
    auto context = CreateRenderContextArg(context_);
    if (context) {
        if (auto op = META_NS::ConstructProperty<SceneOptions>("Options", opts)) {
            context->AddProperty(op);
        }
    }
    return context;
}

Future<IScene::Ptr> SceneManager::CreateScene()
{
    return CreateScene(opts_);
}

Future<IScene::Ptr> SceneManager::CreateScene(SceneOptions opts)
{
    return context_->AddTask([context = CreateContext(BASE_NS::move(opts))] {
        if (auto scene = META_NS::GetObjectRegistry().Create<IScene>(SCENE_NS::ClassId::Scene, context)) {
            auto& ecs = scene->GetInternalScene()->GetEcsContext();
            if (ecs.CreateUnnamedRootNode()) {
                return scene;
            }
            CORE_LOG_E("Failed to create root node");
        }
        return SCENE_NS::IScene::Ptr {};
    });
}

static IScene::Ptr Load(const IScene::Ptr& scene, BASE_NS::string_view uri)
{
    CORE_LOG_I("Loading scene: '%s'", BASE_NS::string(uri).c_str());
    if (auto assets = META_NS::GetObjectRegistry().Create<IAssetObject>(ClassId::AssetObject)) {
        if (assets->Load(scene, uri)) {
            if (auto att = interface_cast<META_NS::IAttach>(scene)) {
                att->Attach(assets);
            }
            return scene;
        }
    }
    return {};
}

Future<IScene::Ptr> SceneManager::CreateScene(BASE_NS::string_view uri)
{
    return CreateScene(uri, opts_);
}

Future<IScene::Ptr> SceneManager::CreateScene(BASE_NS::string_view uri, SceneOptions opts)
{
    if (uri == "" || uri == "scene://empty") {
        return CreateScene(BASE_NS::move(opts));
    }
    if (!context_) {
        return {};
    }
    return context_->AddTask(
        [path = BASE_NS::string(uri), renderContext = context_, args = CreateContext(BASE_NS::move(opts))] {
            IScene::Ptr result;
            if (path.ends_with(".scene") || path.ends_with(".scene2")) {
                // Try loading as an editor project
                if (SetProjectPath(renderContext, path, ProjectPathAction::REGISTER)) {
                    // Rely on an implementation detail: If the path is already registered, it can be re-registered.
                    // Unregistering will remove only the re-registered path and leave the original.
                    result = LoadSceneWithIndex(renderContext, path);
                    SetProjectPath(renderContext, path, ProjectPathAction::UNREGISTER);
                }
            }
            if (!result) {
                // Was not an editor project (so could be either a .gltf or old format .scene)
                result = META_NS::GetObjectRegistry().Create<IScene>(SCENE_NS::ClassId::Scene, args);
                Load(result, path);
            }
            return result;
        });
}

void SceneManager::LoadDefaultResourcesIfNeeded(const CORE_NS::IResourceManager::Ptr& resources)
{
    static constexpr auto DEFAULT_PROJECT_RESOURCE_GROUP_URI = "project://default_resources.res";
    bool foundDefault = false;
    for (auto&& group : resources->GetResourceGroups()) {
        // The project default resource group has no name
        if (group.empty()) {
            foundDefault = true;
            break;
        }
    }
    if (!foundDefault) {
        // Did not find unnamed (default) resource group, load default resource file
        resources->Import(DEFAULT_PROJECT_RESOURCE_GROUP_URI);
    }
}

IScene::Ptr SceneManager::LoadSceneWithIndex(const IRenderContext::Ptr& context, BASE_NS::string_view uri)
{
    if (!context) {
        return {};
    }
    auto resources = context->GetResources();
    if (!resources) {
        return {};
    }

    // Make sure that project default resource group has been loaded
    LoadDefaultResourcesIfNeeded(resources);

    const auto index = GuessIndexFilePath(uri);
    auto ires = resources->Import(index);
    if (ires != CORE_NS::IResourceManager::Result::OK) {
        CORE_LOG_E("Failed to load resource index: %s [%d]", index.c_str(), int(ires));
        return {};
    }
    CORE_NS::ResourceId rid { BASE_NS::string(uri) };
    // see if the scene is resource in the index, if not, add it
    if (!resources->GetResourceInfo(rid).id.IsValid()) {
        resources->AddResource(rid, ClassId::SceneResource.Id().ToUid(), uri);
    }
    return interface_pointer_cast<IScene>(resources->GetResource(rid));
}

bool SceneManager::SetProjectPath(
    const IRenderContext::Ptr& renderContext, BASE_NS::string_view uri, ProjectPathAction action)
{
    if (renderContext) {
        if (const auto renderer = renderContext->GetRenderer()) {
            auto& fileManager = renderer->GetEngine().GetFileManager();
            const auto pathToProject = GuessProjectPath(uri);
            if (action == ProjectPathAction::REGISTER) {
                return fileManager.RegisterPath("project", pathToProject, true);
            } else {
                fileManager.UnregisterPath("project", pathToProject);
            }
            return true;
        }
    }
    CORE_LOG_E("Unable to access file manager: render context missing");
    return false;
}

BASE_NS::string SceneManager::GuessIndexFilePath(BASE_NS::string_view uri)
{
    auto pos = uri.find_last_of('.');
    return uri.substr(0, pos) + ".res";
}

BASE_NS::string SceneManager::GuessProjectPath(BASE_NS::string_view uri)
{
    const auto secondToLastSlashPos = uri.find_last_of('/', uri.find_last_of('/') - 1);
    return BASE_NS::string { uri.substr(0, secondToLastSlashPos) };
}

SCENE_END_NAMESPACE()
