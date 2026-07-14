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
#include <scene/interface/serialization/intf_metadata_importer.h>
#include <scene/interface/serialization/intf_scene_file_loader.h>

#include <core/intf_engine.h>
#include <core/json/json.h>
#include <render/intf_render_context.h>

#include <meta/api/task_queue.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/intf_object_hierarchy_observer.h>
#include <meta/interface/resource/intf_object_resource.h>

#include "asset/asset_object.h"
#include "perf/cpu_perf_scope.h"
#include "resource/util.h"

SCENE_BEGIN_NAMESPACE()

static constexpr auto DEFAULT_PROJECT_RESOURCE_GROUP_URI = "project://default_resources.res";
static constexpr auto PROJECT_JSON_URI = "project://project.json";
// Version string in project.json that selects the new JSON scene importer.
// Anything else (including a missing project.json) routes to the legacy path.
static constexpr auto JSON_IMPORTER_V1_VERSION = "JsonImporter v1";

struct ProjectInfo {
    BASE_NS::string importVersion;
    BASE_NS::vector<BASE_NS::string> resourceUris;
};

static bool HasDefaultResourceGroup(const CORE_NS::IResourceManager::Ptr& resources);
static ProjectInfo ReadProjectInfo(const CORE_NS::IResourceManager::Ptr& resources);
static BASE_NS::vector<BASE_NS::string> BuildSceneLoaderIndices(
    const CORE_NS::IResourceManager::Ptr& resources, BASE_NS::vector<BASE_NS::string> projectUris);
static IScene::Ptr LoadJsonImporterScene(
    const IRenderContext::Ptr& renderContext, BASE_NS::string_view path, BASE_NS::vector<BASE_NS::string> resourceUris);

bool SceneManager::UseDefaultContext()
{
    auto app = GetDefaultApplicationContext();
    if (!app) {
        return true;
    }
    CORE_LOG_D("Using default context for scene manager");
    context_ = app->GetRenderContext();
    if (!context_) {
        CORE_LOG_W("No default context set");
        return false;
    }
    // use opts from app context if we are using it
    opts_ = app->GetDefaultSceneOptions();
    return true;
}

bool SceneManager::Build(const META_NS::IMetadata::Ptr& d)
{
    if (!Super::Build(d)) {
        return false;
    }
    if (d) {
        context_ = GetInterfaceBuildArg<IRenderContext>(d, "RenderContext");
        opts_ = GetBuildArg<SceneOptions>(d, "Options");
    }
    if (!context_ && !UseDefaultContext()) {
        return false;
    }
    if (auto importer = META_NS::GetObjectRegistry().Create<IMetadataImporter>(ClassId::MetadataImporter)) {
        importer->RegisterResourceTypes(context_, opts_);
    }
    return true;
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
    return context_->AddTaskOrRunDirectly([context = CreateContext(BASE_NS::move(opts))] {
        if (auto scene = META_NS::GetObjectRegistry().Create<IScene>(SCENE_NS::ClassId::Scene, context)) {
            auto iScene = scene->GetInternalScene();
            if (auto c = iScene->GetContext()) {
                if (auto res = c->GetResources()) {
                    auto group = UniqueGroupName(res, "Scene", scene);
                    SetPrimaryGroupOnly(scene, group);
                }
            }
            auto& ecs = iScene->GetEcsContext();
            if (ecs.CreateUnnamedRootNode()) {
                return scene;
            }
            CORE_LOG_E("Failed to create root node");
        }
        return SCENE_NS::IScene::Ptr{};
    });
}

static IScene::Ptr Load(const IScene::Ptr& scene, BASE_NS::string_view uri, bool createResources,
    const CORE_NS::ResourceId& rid, int64_t offset)
{
    CORE_LOG_I("Loading scene: '%s'", BASE_NS::string(uri).c_str());
    if (auto assets = META_NS::GetObjectRegistry().Create<IAssetObject>(ClassId::AssetObject)) {
        if (assets->Load(scene, uri, createResources, rid, offset)) {
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

Future<IScene::Ptr> SceneManager::CreateScene(BASE_NS::string_view uri, int64_t offset)
{
    opts_.dataOffset = offset;
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
    bool createRes = opts.createResources;
    CORE_NS::ResourceId rid = opts.resourceId;
    int64_t offset = opts.dataOffset;
    return context_->AddTaskOrRunDirectly([path = BASE_NS::string(uri),
                                              renderContext = context_,
                                              args = CreateContext(BASE_NS::move(opts)),
                                              createRes,
                                              rid,
                                              offset] {
        // Bracket the entire user-facing load (gltf branch + editor-scene
        // branch) so that any throwaway sub-scenes created during this call
        // — including nested CreateScene invocations from gltf-scene
        // resource loaders — observe the SceneLoad scope and skip the
        // empty-render flush in their Uninitialize. We do NOT push
        // ImporterDeferred here: that is the importer parse phase's
        // responsibility (per-thread, narrow), and routing deferred loads
        // outside an importer drain would leak shell images.
        IRenderContext::RenderContextScope sceneLoadScope(renderContext, IRenderContext::ScopeType::SceneLoad);
        IScene::Ptr result;
        const auto pathLowerCase = path.toLower();
        if (pathLowerCase.ends_with(".gltf") || pathLowerCase.ends_with(".glb") || pathLowerCase.ends_with(".mp4")) {
            result = META_NS::GetObjectRegistry().Create<IScene>(SCENE_NS::ClassId::Scene, args);
            if (result) {
                result = Load(result, path, createRes, rid, offset);
            }
        } else if (SetProjectPath(renderContext, path, ProjectPathAction::REGISTER)) {
            // project.json's "importVersion" string selects the importer:
            //   "JsonImporter v1" -> new JSON scene importer
            //   anything else (or no project.json at all) -> legacy path
            auto info = ReadProjectInfo(renderContext->GetResources());
            if (info.importVersion == JSON_IMPORTER_V1_VERSION) {
                result = LoadJsonImporterScene(renderContext, path, BASE_NS::move(info.resourceUris));
            } else {
                result = LoadSceneWithIndex(renderContext, path, BASE_NS::move(info.resourceUris));
            }
            SetProjectPath(renderContext, path, ProjectPathAction::UNREGISTER);
        }
        return result;
    });
}

static bool HasDefaultResourceGroup(const CORE_NS::IResourceManager::Ptr& resources)
{
    if (!resources) {
        return false;
    }
    for (auto&& group : resources->GetResourceGroups(nullptr)) {
        if (group.empty()) {
            return true;
        }
    }
    return false;
}

void SceneManager::LoadDefaultResourcesIfNeeded(const CORE_NS::IResourceManager::Ptr& resources)
{
    SCENE_CPU_PERF_SCOPE("LoadScene", "LoadDefaultResources");
    if (!HasDefaultResourceGroup(resources)) {
        // Did not find unnamed (default) resource group, load default resource file
        resources->Import(DEFAULT_PROJECT_RESOURCE_GROUP_URI);
    }
}

static ProjectInfo ReadProjectInfo(const CORE_NS::IResourceManager::Ptr& resources)
{
    ProjectInfo info;
    if (!resources) {
        return info;
    }
    auto fileManager = resources->GetFileManager();
    if (!fileManager) {
        return info;
    }
    auto file = fileManager->OpenFile(PROJECT_JSON_URI);
    if (!file) {
        return info;
    }
    const uint64_t byteLength = file->GetLength();
    BASE_NS::string raw(static_cast<size_t>(byteLength), BASE_NS::string::value_type());
    if (file->Read(raw.data(), byteLength) != byteLength) {
        CORE_LOG_W("Failed to read project.json");
        return info;
    }
    const auto json = CORE_NS::json::parse(raw.c_str());
    if (!json) {
        CORE_LOG_W("Failed to parse project.json");
        return info;
    }
    if (const auto* importVersion = json.find("importVersion"); importVersion && importVersion->is_string()) {
        info.importVersion = CORE_NS::json::unescape(importVersion->string_);
        CORE_LOG_D("project.json importVersion: '%s'", info.importVersion.c_str());
    }
    const auto* resourcesArray = json.find("resources");
    if (!resourcesArray || !resourcesArray->is_array()) {
        return info;
    }
    info.resourceUris.reserve(resourcesArray->array_.size());
    for (const auto& entry : resourcesArray->array_) {
        if (entry.is_string()) {
            info.resourceUris.push_back(BASE_NS::string{entry.string_});
        }
    }
    return info;
}

static BASE_NS::vector<BASE_NS::string> BuildSceneLoaderIndices(
    const CORE_NS::IResourceManager::Ptr& resources, BASE_NS::vector<BASE_NS::string> projectUris)
{
    BASE_NS::vector<BASE_NS::string> indices;
    if (HasDefaultResourceGroup(resources)) {
        return indices;
    }
    indices = BASE_NS::move(projectUris);
    if (indices.empty()) {
        indices.push_back(DEFAULT_PROJECT_RESOURCE_GROUP_URI);
    }
    return indices;
}

static IScene::Ptr LoadJsonImporterScene(
    const IRenderContext::Ptr& renderContext, BASE_NS::string_view path, BASE_NS::vector<BASE_NS::string> resourceUris)
{
    auto loader = META_NS::GetObjectRegistry().Create<ISceneFileLoader>(ClassId::SceneFileLoader);
    if (!loader) {
        return {};
    }
    auto indices = BuildSceneLoaderIndices(renderContext->GetResources(), BASE_NS::move(resourceUris));
    return loader->LoadScene(renderContext, path, indices);
}

bool SceneManager::LoadProjectResources(
    const CORE_NS::IResourceManager::Ptr& resources, const BASE_NS::vector<BASE_NS::string>& resourceUris)
{
    SCENE_CPU_PERF_SCOPE("LoadScene", "LoadProjectResources");
    if (resourceUris.empty()) {
        return false;
    }
    for (const auto& uri : resourceUris) {
        CORE_LOG_D("Importing project resource: '%s'", uri.c_str());
        resources->Import(uri);
    }
    return true;
}

IScene::Ptr SceneManager::LoadSceneWithIndex(
    const IRenderContext::Ptr& context, BASE_NS::string_view uri, BASE_NS::vector<BASE_NS::string> resourceUris)
{
    SCENE_CPU_PERF_SCOPE("LoadScene", uri);
    if (!context) {
        return {};
    }
    auto resources = context->GetResources();
    if (!resources) {
        return {};
    }

    // Make sure that project default resource group has been loaded
    if (!HasDefaultResourceGroup(resources)) {
        if (!LoadProjectResources(resources, resourceUris)) {
            LoadDefaultResourcesIfNeeded(resources);
        }
    }

    CORE_NS::ResourceIdContext rid{BASE_NS::string(uri)};
    // see if the scene is resource in the index, if not, add it
    bool missingSceneRid = !resources->GetResourceInfo(rid).id.IsValid();
    if (missingSceneRid) {
        resources->AddResource(rid, ClassId::SceneResource.Id().ToUid(), uri);
    }
    auto result = interface_pointer_cast<IScene>(resources->GetResource(rid));
    if (result) {
        SCENE_CPU_PERF_SCOPE("LoadScene", "PostLoadTrim");
        auto root = result->GetRootNode().GetResult();
        if (auto i = result->GetInternalScene()) {
            // make sure all changes to ecs are applied before trimming the objects
            i->SyncProperties();
            i->ReleaseNode(BASE_NS::move(root), true);

            auto groups = i->GetResourceGroups();
            for (auto&& g : groups.GetAllHandles()) {
                auto name = g->GetGroup();
                CORE_LOG_D("Purging resource group '%s'", name.c_str());
                resources->PurgeGroup(name, result);
            }
        }
    }
    if (missingSceneRid) {
        resources->RemoveResource(rid);
    }
    return result;
}

bool SceneManager::SetProjectPath(
    const IRenderContext::Ptr& renderContext, BASE_NS::string_view uri, ProjectPathAction action)
{
    if (uri.starts_with("project:")) {
        return true;
    }
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
    return BASE_NS::string{uri.substr(0, secondToLastSlashPos)};
}

SCENE_END_NAMESPACE()
