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

#include "scene_holder.h"

#include <algorithm>
#include <chrono>
#include <inttypes.h>
#include <scene_plugin/api/material.h>
#include <scene_plugin/interface/intf_scene.h>

#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_batch_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/implementation_uids.h>
#include <3d/render/default_material_constants.h>
#include <3d/util/intf_mesh_builder.h>
#include <3d/util/intf_scene_util.h>
#include <base/util/uid_util.h>
#include <core/ecs/intf_system_graph_loader.h>
#include <core/intf_engine.h>
#include <core/plugin/intf_class_factory.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/util/intf_render_util.h>

#include <meta/api/make_callback.h>
#include <meta/base/shared_ptr.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "asset_loader.h"
#include "asset_manager.h"
#include "ecs_util.h"
#include "entity_collection.h"
#include "intf_node_private.h"
#include "task_utils.h"

// Initialize ecs on scene holder initialization or do it lazily when there's a scene to load

using namespace BASE_NS;
using namespace CORE_NS;
using namespace CORE3D_NS;
using namespace RENDER_NS;

using SCENE_NS::MakeTask;

namespace {

bool TickFrame(IEcs& ecs, uint64_t totalTime, uint64_t deltaTime)
{
    // run garbage collection before updating the systems to ensure only valid entities/ components are available.
    ecs.ProcessEvents();

    const bool needRender = ecs.Update(totalTime, deltaTime);

    // do gc also after the systems have been updated to ensure any deletes done by systems are effective
    // and client doesn't see stale entities.
    ecs.ProcessEvents();

    return needRender;
}
constexpr GpuImageDesc GetColorImageDesc(const uint32_t width, const uint32_t height)
{
    GpuImageDesc resolveDesc;
    resolveDesc.width = width;
    resolveDesc.height = height;
    resolveDesc.depth = 1;
    resolveDesc.format = BASE_NS::Format::BASE_FORMAT_R8G8B8A8_SRGB;
    resolveDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    resolveDesc.usageFlags =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    resolveDesc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    resolveDesc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    resolveDesc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    resolveDesc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    return resolveDesc;
}

constexpr GpuImageDesc GetDepthImageDesc(const uint32_t width, const uint32_t height)
{
    GpuImageDesc resolveDesc;
    resolveDesc.width = width;
    resolveDesc.height = height;
    resolveDesc.depth = 1;
    resolveDesc.format = BASE_NS::Format::BASE_FORMAT_D16_UNORM;
    resolveDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    resolveDesc.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                             ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    resolveDesc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    resolveDesc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    resolveDesc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    resolveDesc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    return resolveDesc;
}

static BASE_NS::string ROOTNODE_NAME = "rootNode_";
static BASE_NS::string ROOTNODE_PATH = "/" + ROOTNODE_NAME + "/";

BASE_NS::string_view RemoveRootNodeFromPath(const BASE_NS::string_view& path)
{
    if (path.starts_with(ROOTNODE_PATH)) {
        return path.substr(ROOTNODE_PATH.length());
    }

    if (path.starts_with("/")) {
        return path.substr(1);
    }

    return path;
}
} // namespace

SceneHolder::SceneHolder(META_NS::InstanceId uid, META_NS::IObjectRegistry& registry,
    const BASE_NS::shared_ptr<RENDER_NS::IRenderContext>& gc, META_NS::ITaskQueue::Ptr appQueue,
    META_NS::ITaskQueue::Ptr engineQueue)
    : instanceId_(uid), renderContext_(gc), appTaskQueue_(appQueue), engineTaskQueue_(engineQueue),
      objectRegistry_(registry)
{}

SceneHolder::~SceneHolder()
{
    ResetScene(false);
}

CORE_NS::IEcs::Ptr SceneHolder::GetEcs()
{
    // once we offer Ecs to anyone, we cannot know it is intact
    scenePrepared_ = false;
    return ecs_;
}

void SceneHolder::Initialize(WeakPtr me)
{
    me_ = me;
    InitializeScene();
}

void SceneHolder::Uninitialize()
{
    // Remove callbacks, these are no longer needed.
    SetInitializeCallback({}, me_);
    SetSceneLoadedCallback({}, me_);

    // Move processing to engine thread.
    UninitializeScene();
}
void SceneHolder::SetRenderSize(uint32_t width, uint32_t height, uint64_t cameraHandle)
{
    // Render size changed, schedule update engine thread.
    UpdateViewportSize(width, height, cameraHandle);
}

void SceneHolder::SetCameraTarget(
    const SCENE_NS::ICamera::Ptr& camera, BASE_NS::Math::UVec2 size, RENDER_NS::RenderHandleReference ref)
{
    auto cameraEnt = interface_cast<SCENE_NS::IEcsObject>(camera)->GetEntity();

    CameraData::Ptr cd;
    for (auto c : cameras_) {
        if (c->entity == cameraEnt) {
            cd = c;
            break;
        }
    }
    if (!cd) {
        // invalid.
        return;
    }
    cd->width = size.x;
    cd->height = size.y;
    if (!ref) {
        cd->colorImage = {};
        cd->ownsColorImage = true;
        RecreateOutputTexture(cd);
    } else {
        cd->colorImage = ref;
        cd->ownsColorImage = false;
    }
    cd->updateTargets = true;
}

void SceneHolder::RequestReload()
{
    LoadScene();
}

void SceneHolder::Load(const BASE_NS::string& uri)
{
    // Scene systems graph uri changed, schedule update to engine thread.
    if (sceneUri_ != uri) {
        sceneUri_ = uri;

        RequestReload();
    }
}

void SceneHolder::SetSystemGraphUri(const string& uri)
{
    // Scene systems graph uri changed, schedule update to engine thread.
    if (uri != sceneSystemGraphUri_) {
        sceneSystemGraphUri_ = uri;

        RequestReload();
    }
}

void SceneHolder::SetInitializeCallback(ISceneInitialized::Ptr callback, SceneHolder::WeakPtr weakMe)
{
    // Schedule update of callback to engine thread.
    sceneInitializedCallback_ = callback;
}

void SceneHolder::SetSceneLoadedCallback(ISceneLoaded::Ptr callback, SceneHolder::WeakPtr weakMe)
{
    // Schedule update of callback to engine thread.
    sceneLoadedCallback_ = callback;
}

void SceneHolder::SetUninitializeCallback(ISceneUninitialized::Ptr callback, SceneHolder::WeakPtr weakMe)
{
    // Schedule update of callback to engine thread.
    sceneUninitializedCallback_ = callback;
}

void SceneHolder::ActivateCamera(const Entity& cameraEntity) const
{
    if (!(cameraComponentManager_ && EntityUtil::IsValid(cameraEntity))) {
        CORE_LOG_W("SceneHolder::ActivateCamera: Can not be activated. cameraEntity: %" PRIx64
                   ", cameraComponentManager_: %d, IsValid(cameraEntity): %d",
            cameraEntity.id, static_cast<bool>(cameraComponentManager_), EntityUtil::IsValid(cameraEntity));
        return;
    }

    CameraComponent cameraComponent = cameraComponentManager_->Get(cameraEntity);
    cameraComponent.sceneFlags |= CameraComponent::ACTIVE_RENDER_BIT;
    cameraComponentManager_->Set(cameraEntity, cameraComponent);
}

void SceneHolder::DeactivateCamera(const Entity& cameraEntity) const
{
    if (!(cameraComponentManager_ && EntityUtil::IsValid(cameraEntity))) {
        CORE_LOG_W("SceneHolder::DeactivateCamera: Can not be deactivated. cameraEntity: %" PRIx64
                   ", cameraComponentManager_: %d, IsValid(cameraEntity): %d",
            cameraEntity.id, static_cast<bool>(cameraComponentManager_), EntityUtil::IsValid(cameraEntity));
        return;
    }

    CameraComponent cameraComponent = cameraComponentManager_->Get(cameraEntity);
    cameraComponent.sceneFlags &= ~(CameraComponent::ACTIVE_RENDER_BIT /* | CameraComponent::MAIN_CAMERA_BIT*/);
    cameraComponentManager_->Set(cameraEntity, cameraComponent);
}

bool SceneHolder::IsCameraActive(const Entity& cameraEntity) const
{
    if (!(cameraComponentManager_ && EntityUtil::IsValid(cameraEntity))) {
        CORE_LOG_W("SceneHolder::DeactivateCamera: Can not be deactivated. cameraEntity: %" PRIx64
                   ", cameraComponentManager_: %d, IsValid(cameraEntity): %d",
            cameraEntity.id, static_cast<bool>(cameraComponentManager_), EntityUtil::IsValid(cameraEntity));
        return false;
    }

    CameraComponent cameraComponent = cameraComponentManager_->Get(cameraEntity);
    return cameraComponent.sceneFlags & (CameraComponent::ACTIVE_RENDER_BIT);
}

bool SceneHolder::InitializeScene()
{
    graphicsContext3D_ = CreateInstance<CORE3D_NS::IGraphicsContext>(
        *renderContext_->GetInterface<IClassFactory>(), UID_GRAPHICS_CONTEXT);
    graphicsContext3D_->Init();

    picking_ = GetInstance<CORE3D_NS::IPicking>(*renderContext_->GetInterface<IClassRegister>(), UID_PICKING);

    // scene loading was attempted before initialization, retry now
    if (loadSceneFailed_) {
        loadSceneFailed_ = false;
        RequestReload();
    } else {
        // business as usual, set up things according build time settings
#ifdef INITIALIZE_ECS_IMMEDIATELY
        ResetScene(true);
        scenePrepared_ = true;
#else
        ResetScene(false);
#endif
    }

    // This is one-time init.
    return false;
}

BASE_NS::vector<CORE_NS::Entity> SceneHolder::RenderCameras()
{
    // nobody has asked for updates, yet
    if (!ecs_ /*|| isReadyForNewFrame_.empty()*/) {
        return {};
    }

    bool busy = false;
    BASE_NS::vector<CORE_NS::Entity> activeCameras;
    for (auto& camera : cameras_) {
        if (!EntityUtil::IsValid(camera->entity))
            continue;

        if (auto cc = cameraComponentManager_->Read(camera->entity)) {
            if (cc->sceneFlags & CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT) {
                if (camera->updateTargets) {
                    UpdateCameraRenderTarget(camera);
                    camera->updateTargets = false;
                }
                activeCameras.push_back(camera->entity);
            }
        }
    }
    isRunningFrame_ = true;
    // Tick frame. One camera could have static view while other is moving
    IEcs* ecs = ecs_.get();
    ecs->ProcessEvents();
    using namespace std::chrono;
    const auto currentTime =
        static_cast<uint64_t>(duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());
    if (firstTime_ == ~0u) {
        previousFrameTime_ = firstTime_ = currentTime;
    }
    auto deltaTime = currentTime - previousFrameTime_;
    constexpr auto limitHz = duration_cast<microseconds>(duration<float, std::ratio<1, 15u>>(1)).count();
    if (deltaTime > limitHz) {
        deltaTime = limitHz; // clampthe time step to no longer than 15hz.
    }
    previousFrameTime_ = currentTime;
    const uint64_t totalTime = currentTime - firstTime_;
    const bool needsRender = ecs->Update(totalTime, deltaTime);
    ecs->ProcessEvents();
    auto renderHandles = graphicsContext3D_->GetRenderNodeGraphs(*ecs_);

    if ((needsRender) && (!renderHandles.empty())) {
        // The scene needs to be rendered.
        RENDER_NS::IRenderer& renderer = renderContext_->GetRenderer();
        renderer.RenderDeferred(renderHandles);
    } else {
        // did not render anything.
        bool nr = needsRender;
        activeCameras.clear();
    }
    isRunningFrame_ = false;

    if (requiresEventProcessing_) {
        ProcessEvents();
    }

    // Schedule new update to scene.
    return activeCameras;
}

bool SceneHolder::UninitializeScene()
{
    ResetScene();

    if (ecs_) {
        ecs_->Uninitialize();
        ecs_.reset();
    }

    if (sceneUninitializedCallback_) {
        QueueApplicationTask(MakeTask(
                                 [](auto callback) {
                                     callback->Invoke();
                                     return false;
                                 },
                                 sceneUninitializedCallback_),
            false);
    }

    sceneInitializedCallback_.reset();
    sceneUpdatedCallback_.reset();
    sceneUninitializedCallback_.reset();

    return false;
}

void SceneHolder::ProcessEvents()
{
    if (isRunningFrame_) {
        requiresEventProcessing_ = true;
        return;
    }

    if (ecsListener_ && ecsListener_->IsCallbackActive()) {
        requiresEventProcessing_ = true;
        return;
    }

    if (ecs_) {
        ecs_->ProcessEvents();
        requiresEventProcessing_ = false;
    }
}

bool SceneHolder::CreateDefaultEcs()
{
    if (!renderContext_) {
        CORE_LOG_W("%s: Graphics contexts is not available, can not create ecs", __func__);
        return false;
    }

    auto& engine = renderContext_->GetEngine();

    ecs_ = engine.CreateEcs();
    ecs_->SetRenderMode(renderMode_);

    if (!ecsListener_) {
        ecsListener_ = BASE_NS::shared_ptr<SCENE_NS::EcsListener>(new SCENE_NS::EcsListener());
    }
    ecsListener_->SetEcs(ecs_);

    // Get systemGraphLoader factory from global plugin registry.
    auto* factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(engine.GetFileManager());

    // First try to load project-specific system graph.
    auto result = sceneSystemGraphUri_.empty() ? false : systemGraphLoader->Load(sceneSystemGraphUri_, *ecs_).success;

    if (!result) {
        // Fall back to default graph.
        systemGraphLoader->Load("rofs3D://systemGraph.json", *ecs_);
    }

    auto renderPreprocessorSystem = GetSystem<IRenderPreprocessorSystem>(*ecs_);

    string dataStorePrefix = "renderDataStore:" + GetUniqueName();

    auto& renderDataStoreManager = renderContext_->GetRenderDataStoreManager();

    auto sceneDataStore = dataStorePrefix + "RenderDataStoreDefaultScene";
    auto cameraDataStore = dataStorePrefix + "RenderDataStoreDefaultCamera";
    auto lightDataStore = dataStorePrefix + "RenderDataStoreDefaultLight";
    auto materialDataStore = dataStorePrefix + "RenderDataStoreDefaultMaterial";
    auto morphDataStore = dataStorePrefix + "RenderDataStoreMorph";

    if (renderPreprocessorSystem) {
        IRenderPreprocessorSystem::Properties props;
        props.dataStorePrefix = dataStorePrefix;
        props.dataStoreScene = sceneDataStore;
        props.dataStoreCamera = cameraDataStore;
        props.dataStoreLight = lightDataStore;
        props.dataStoreMaterial = materialDataStore;
        props.dataStoreMorph = morphDataStore;

        *ScopedHandle<IRenderPreprocessorSystem::Properties>(renderPreprocessorSystem->GetProperties()) = props;
    }

    if (auto callback = ecsInitializationCallback_.lock()) {
        auto result = callback->Invoke(ecs_);
        if (!result) {
            CORE_LOG_W("Ecs initialization failed in the callback, expect trouble");
        }
    }

    ecs_->Initialize();

    animationComponentManager_ = CORE_NS::GetManager<CORE3D_NS::IAnimationComponentManager>(*ecs_);
    cameraComponentManager_ = GetManager<CORE3D_NS::ICameraComponentManager>(*ecs_);
    envComponentManager_ = GetManager<CORE3D_NS::IEnvironmentComponentManager>(*ecs_);
    layerComponentManager_ = GetManager<CORE3D_NS::ILayerComponentManager>(*ecs_);
    lightComponentManager_ = GetManager<CORE3D_NS::ILightComponentManager>(*ecs_);
    materialComponentManager_ = GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs_);
    meshComponentManager_ = GetManager<CORE3D_NS::IMeshComponentManager>(*ecs_);
    nameComponentManager_ = GetManager<CORE3D_NS::INameComponentManager>(*ecs_);
    nodeComponentManager_ = GetManager<CORE3D_NS::INodeComponentManager>(*ecs_);
    renderMeshComponentManager_ = GetManager<CORE3D_NS::IRenderMeshComponentManager>(*ecs_);
    rhComponentManager_ = GetManager<CORE3D_NS::IRenderHandleComponentManager>(*ecs_);
    transformComponentManager_ = GetManager<CORE3D_NS::ITransformComponentManager>(*ecs_);
    uriComponentManager_ = GetManager<CORE3D_NS::IUriComponentManager>(*ecs_);

    if (animationComponentManager_) {
        animationQuery_.reset(new CORE_NS::ComponentQuery());
        animationQuery_->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = { { *nodeComponentManager_,
                                                             ComponentQuery::Operation::OPTIONAL },
            { *nameComponentManager_, ComponentQuery::Operation::OPTIONAL } };
        animationQuery_->SetupQuery(*animationComponentManager_, operations);
    }

    if (meshComponentManager_) {
        meshQuery_.reset(new CORE_NS::ComponentQuery());
        meshQuery_->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = { { *nodeComponentManager_,
                                                             ComponentQuery::Operation::OPTIONAL },
            { *nameComponentManager_, ComponentQuery::Operation::OPTIONAL } };
        meshQuery_->SetupQuery(*meshComponentManager_, operations);
    }

    if (materialComponentManager_) {
        materialQuery_.reset(new CORE_NS::ComponentQuery());
        materialQuery_->SetEcsListenersEnabled(true);
        const ComponentQuery::Operation operations[] = { { *nodeComponentManager_,
                                                             ComponentQuery::Operation::OPTIONAL },
            { *nameComponentManager_, ComponentQuery::Operation::OPTIONAL },
            { *uriComponentManager_, ComponentQuery::Operation::OPTIONAL } };
        materialQuery_->SetupQuery(*materialComponentManager_, operations);
    }
    nodeSystem_ = static_cast<CORE3D_NS::INodeSystem*>(ecs_->GetSystem(CORE3D_NS::INodeSystem::UID));
    return true;
}

CORE_NS::Entity SceneHolder::CreateCamera(const BASE_NS::string& name, uint32_t flagBits)
{
    return CreateCamera({}, name, flagBits);
}

CORE_NS::Entity SceneHolder::CreateCamera(const BASE_NS::string& path, const BASE_NS::string& name, uint32_t flagBits)
{
    if (ecs_) {
        auto pathWithoutRootNode = RemoveRootNodeFromPath(path);

        // Create camera. Scene util produces an invalid node so set up our camera from node system ourself
        auto nodeSystem = GetSystem<INodeSystem>(*ecs_);
        auto node = nodeSystem->CreateNode();
        auto entity = node->GetEntity();

        CORE3D_NS::ISceneNode* parent = nullptr;
        if (pathWithoutRootNode.empty()) {
            parent = rootNode_;
        } else {
            parent = rootNode_->LookupNodeByPath(pathWithoutRootNode);
        }

        CORE_ASSERT(parent);

        node->SetParent(*parent);
        node->SetName(name);

        auto tcm = GetManager<ITransformComponentManager>(*ecs_);
        TransformComponent tc;
        tc.position = Math::Vec3(0.0f, 0.0f, 2.5f);
        tc.rotation = {};
        tcm->Set(entity, tc);

        CameraComponent cc;
        cc.sceneFlags |= CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT;
        cc.projection = CameraComponent::Projection::PERSPECTIVE;
        cc.yFov = Math::DEG2RAD * 60.f;
        cc.zNear = 0.1f;
        cc.zFar = 100.f;
        cc.sceneFlags |= flagBits;
        cameraComponentManager_->Set(entity, cc);

        cameras_.emplace_back(CameraData::Create(entity));
        return entity;
    } else {
        CORE_LOG_E("%s: Ecs not available, can not create camera %s", __func__, name.c_str());
    }
    return CORE_NS::Entity();
}

void SceneHolder::SetDefaultCamera()
{
    if (cameraComponentManager_) {
        auto cameraCount = cameraComponentManager_->GetComponentCount();
        while (cameraCount) {
            --cameraCount;
            // if the scene already contains main camera, respect it
            if (cameraComponentManager_->Get(cameraComponentManager_->GetEntity(cameraCount)).sceneFlags &
                CameraComponent::MAIN_CAMERA_BIT) {
                auto entity = cameraComponentManager_->GetEntity(cameraCount);
                cameras_.emplace_back(CameraData::Create(entity));
                defaultCameraEntity_ = entity;
                SetMainCamera(entity);
                return;
            }
        }
    }
}

void SceneHolder::SetMainCamera(const Entity& entity)
{
    if (cameras_.empty() || (mainCamera_ && entity == mainCamera_->entity)) {
        return;
    }

    if (mainCamera_ && cameraComponentManager_ && EntityUtil::IsValid(mainCamera_->entity)) {
        bool isAlive = ecs_->GetEntityManager().IsAlive(mainCamera_->entity);
        if (isAlive) {
            // Mark previous main camera as inactive.
            CameraComponent cameraComponent = cameraComponentManager_->Get(mainCamera_->entity);
            cameraComponent.sceneFlags &= ~CameraComponent::MAIN_CAMERA_BIT;
            cameraComponentManager_->Set(mainCamera_->entity, cameraComponent);
        }
    }

    mainCamera_ = {};

    for (auto& camera : cameras_) {
        if (camera->entity == entity) {
            mainCamera_ = camera;
            break;
        }
    }

    if (!mainCamera_) {
        if (EntityUtil::IsValid(entity)) {
            CORE_LOG_W("%s: was not able to set camera for: %" PRIx64 " ", __func__, entity.id);
        }
        return;
    }

    if (cameraComponentManager_ && EntityUtil::IsValid(mainCamera_->entity)) {
        // Mark new camera as active.
        CameraComponent cameraComponent = cameraComponentManager_->Get(mainCamera_->entity);
        cameraComponent.sceneFlags |= CameraComponent::MAIN_CAMERA_BIT;
        cameraComponentManager_->Set(mainCamera_->entity, cameraComponent);
        UpdateViewportSize(mainCamera_->width, mainCamera_->height, mainCamera_->entity.id);
    }
}

void SceneHolder::AddCamera(const CORE_NS::Entity& entity)
{
    for (auto c : cameras_) {
        if (c->entity == entity) {
            return;
        }
    }
    cameras_.emplace_back(CameraData::Create(entity));
}

void SceneHolder::RemoveCamera(const CORE_NS::Entity& entity)
{
    auto it = std::find_if(cameras_.begin(), cameras_.end(), [entity](const auto& item) {
        // Check if camera entities are the same.
        return item->entity == entity;
    });

    if (it != cameras_.end()) {
        cameras_.erase(it);
    }
}

void SceneHolder::UpdateViewportSize(uint32_t width, uint32_t height, uint64_t cameraHandle)
{
    CORE_LOG_D("%s: w: %u h: %u", __func__, width, height);
    // these would be never valid as render target so use some default values to make sure that buffers are prepared
    if (width == 0) {
        width = 128; // 128 default width
    }
    if (height == 0) {
        height = 128; // // 128 default height
    }

    if (ecs_) {
        if (cameraHandle == 0 && !cameras_.empty()) {
            // short circuit defaut camera
            cameraHandle = cameras_.front()->entity.id;
        }

        // Make sure that render target will be updated
        // ToDo: Clean this up to select the correct camera instance
        CameraData::Ptr camera;

        for (auto& c : cameras_) {
            if (c->entity.id == cameraHandle) {
                camera = c;
                break;
            }
        }
        if (!camera) {
            return;
        }
        if (!camera->ownsColorImage) {
            // we don't own the bitmap so.
            return;
        }

        camera->width = width;
        camera->height = height;
        RecreateOutputTexture(camera);

        if (!EntityUtil::IsValid(camera->entity)) {
            return;
        }
        camera->updateTargets = true;
        ecs_->RequestRender();
    }
}

void SceneHolder::RecreateOutputTexture(CameraData::Ptr camera)
{
    if ((renderContext_) && (camera)) {
        if (camera->ownsColorImage) {
            camera->colorImage = renderContext_->GetDevice().GetGpuResourceManager().Create(
                GetColorImageDesc(camera->width, camera->height));
#ifdef SCENE_PLUGIN_CREATE_IMPLICIT_DEPTH_TARGET
            camera->depthImage = renderContext_->GetDevice().GetGpuResourceManager().Create(
                GetDepthImageDesc(camera->width, camera->height));
#endif
        }
    }
}

bool SceneHolder::UpdateCameraRenderTarget(CameraData::Ptr camera)
{
    if (!EntityUtil::IsValid(camera->entity))
        return false;

    if (auto cc = cameraComponentManager_->Write(camera->entity)) {
        auto& em = ecs_->GetEntityManager();
        cc->renderResolution[0] = static_cast<float>(camera->width);
        cc->renderResolution[1] = static_cast<float>(camera->height);
        cc->customColorTargets = { GetOrCreateEntityReference(em, *rhComponentManager_, camera->colorImage) };
        if (camera->depthImage) {
            cc->customDepthTarget = GetOrCreateEntityReference(em, *rhComponentManager_, camera->depthImage);
        }
        if (cc->sceneFlags & CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT) {
            // please render also.
            return true;
        }
    }
    // no render.
    return false;
}

void SceneHolder::ResetScene(bool initialize)
{
    bool wasScenePrepared = scenePrepared_;
    scenePrepared_ = false;
    if (wasScenePrepared && initialize) {
        return;
    }

    SetMainCamera({});
    cameras_.clear();

    animations_.clear();

    // Release resource data.
    gltfResourceData_ = {};
    sceneEntity_ = {};

    // Release default camera.
    defaultCameraEntity_ = {};

    // Release scene
    scene_ = {};
    assetManager_ = {};

    // Release scene root.
    rootNode_ = {};

    // fully recreate ECS if requested
    if (ecs_) {
        cameraComponentManager_ = {};
        meshComponentManager_ = {};
        nameComponentManager_ = {};
        nodeComponentManager_ = {};
        meshQuery_ = {};
        materialQuery_ = {};
        animationQuery_ = {};
        ecsListener_->Reset();
        ecs_->Uninitialize();
        ecs_.reset();
    }

    if (sceneUninitializedCallback_) {
        QueueApplicationTask(MakeTask(
                                 [](auto callback) {
                                     callback->Invoke();
                                     return false;
                                 },
                                 sceneUninitializedCallback_),
            false);
    }

    if (initialize && CreateDefaultEcs()) {
        // Create new root node.
        CORE3D_NS::INodeSystem& nodeSystem = *GetSystem<CORE3D_NS::INodeSystem>(*ecs_);
        rootNode_ = nodeSystem.CreateNode();
        rootNode_->SetName(ROOTNODE_NAME);
        if (nodeComponentManager_) {
            nodeComponentManager_->Create(rootNode_->GetEntity());
        }
        scene_ = SCENE_NS::IEntityCollection::Ptr { new SCENE_NS::EntityCollection(*ecs_, "scene", {}) };

        // it seems that having our root entity as a part of loaded entify has no significant effect on
        // anything Presumably it is ok to keep it out of the serialization auto entityRef =

        rootNode_->SetEnabled(true);

        assetManager_ =
            SCENE_NS::IAssetManager::Ptr { new SCENE_NS::AssetManager(*renderContext_, *graphicsContext3D_) };
    }
}

void SceneHolder::SetSceneSystemGraph(const BASE_NS::string& uri)
{
    if (uri != sceneSystemGraphUri_) {
        sceneSystemGraphUri_ = uri;
        scenePrepared_ = false;
        if (scene_) {
            // reload with new graph
            LoadScene();

        } else if (sceneLoadedCallback_) {
            QueueApplicationTask(MakeTask(
                                     [](auto sceneLoadedCallback) {
                                         if (sceneLoadedCallback) {
                                             sceneLoadedCallback->Invoke(SCENE_NS::IScene::SCENE_STATUS_UNINITIALIZED);
                                         }
                                         return false;
                                     },
                                     sceneLoadedCallback_),
                false);
        }
    }
}

void SceneHolder::LoadScene(const BASE_NS::string& uri)
{
    // ToDo: LoadScene shoud deal with this, but currently triggers a problem when we swap between instances
    if (sceneUri_ == uri) {
        if (sceneLoadedCallback_) {
            bool haveEcs = (ecs_ == nullptr);
            QueueApplicationTask(MakeTask(
                                     [haveEcs](auto sceneLoadedCallback) {
                                         if (sceneLoadedCallback) {
                                             sceneLoadedCallback->Invoke(
                                                 haveEcs ? SCENE_NS::IScene::SCENE_STATUS_READY
                                                         : SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED);
                                         }
                                         return false;
                                     },
                                     sceneLoadedCallback_),
                false);
        }
        return;
    }
    sceneUri_ = uri;
    LoadScene();
}

static constexpr BASE_NS::string_view KDUMMY_STRING { "!#&/dummy" };

void SceneHolder::IntrospectNodeless()
{
#ifndef NDEBUG
    Entity dummy {};
    FindMesh(KDUMMY_STRING, KDUMMY_STRING, dummy);
    FindMaterial(KDUMMY_STRING, KDUMMY_STRING, dummy);
#endif // !NDEBUG

    RemoveUriComponentsFromMeshes();
    ResolveAnimations();
}

bool GetEntityId(const CORE_NS::Entity& entity, BASE_NS::string_view& entityName,
    SCENE_NS::IEntityCollection& collection, BASE_NS::string_view backupCollection,
    CORE3D_NS::INameComponentManager& nameComponentManager)
{
    if (EntityUtil::IsValid(entity)) {
        entityName = collection.GetUniqueIdentifierRecursive(entity);
        if (!entityName.empty() && entityName != "/") {
            return true;
        } else {
            if (auto readHandle = nameComponentManager.Read(entity)) {
                entityName = readHandle->name;
                collection.AddEntityToSubcollection(backupCollection, entityName, entity, true);
                auto gltfCollectionIx = collection.GetSubCollectionIndex(backupCollection);
                if (auto gltfCollection = collection.GetSubCollection(gltfCollectionIx)) {
                    entityName = gltfCollection->GetUniqueIdentifier(entity);
                    return true;
                }
            } else {
                // ToDo: we could store entity id even it did not have name component
                CORE_LOG_W("%s: entity missing from collections", __func__);
            }
        }
    }
    return false;
}

void ExtractQueryResults(CORE_NS::ComponentQuery& query, SCENE_NS::IEntityCollection& collection,
    BASE_NS::string_view backupCollection, BASE_NS::shared_ptr<BASE_NS::vector<BASE_NS::string_view>>& result,
    CORE3D_NS::INameComponentManager& nameComponentManager)
{
    query.Execute();
    auto results = query.GetResults();
    for (auto& row : results) {
        BASE_NS::string_view id;
        if (GetEntityId(row.entity, id, collection, backupCollection, nameComponentManager)) {
            result->push_back(id);
        }
    }
}

BASE_NS::shared_ptr<BASE_NS::vector<BASE_NS::string_view>> SceneHolder::ListMaterialNames()
{
    BASE_NS::shared_ptr<BASE_NS::vector<BASE_NS::string_view>> ret { new BASE_NS::vector<BASE_NS::string_view> };
    if (materialQuery_ && scene_) {
        ExtractQueryResults(*materialQuery_, *scene_.get(), "GLTF_Materials", ret, *nameComponentManager_);
    }
    return ret;
}

BASE_NS::shared_ptr<BASE_NS::vector<BASE_NS::string_view>> SceneHolder::ListMeshNames()
{
    BASE_NS::shared_ptr<BASE_NS::vector<BASE_NS::string_view>> ret { new BASE_NS::vector<BASE_NS::string_view> };
    if (meshQuery_ && scene_) {
        ExtractQueryResults(*meshQuery_, *scene_.get(), "GLTF_Meshes", ret, *nameComponentManager_);
    }
    return ret;
}

void SceneHolder::RemoveUriComponentsFromMeshes()
{
    meshQuery_->Execute();
    auto meshes = meshQuery_->GetResults();

    SCENE_PLUGIN_VERBOSE_LOG("found %zu meshes", meshes.size());

    // Remove uri component from meshes, since it causes issues with prefab loading.
    // The gltf importer shares the meshes between collections, if they have
    // similar uri.
    // However, this is not desired behavior, because scene-overridden property
    // values will propagate to future prefab instances (e.g. material).
    for (auto& mesh : meshes) {
        if (uriComponentManager_->HasComponent(mesh.entity)) {
            uriComponentManager_->Destroy(mesh.entity);
        }
    }
}

bool SceneHolder::FindMesh(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, Entity& entity)
{
    if (ecs_) {
        entity = scene_->GetEntityRecursive(fullPath);
        if (CORE_NS::EntityUtil::IsValid(entity)) {
            return true;
        }

        meshQuery_->Execute();
        auto meshes = meshQuery_->GetResults();

        SCENE_PLUGIN_VERBOSE_LOG("found %zu meshes", meshes.size());

#ifdef NDEBUG
        if (name == KDUMMY_STRING) {
            return false;
        }
#endif

        for (auto&& mesh : meshes) {
            bool effectivelyEnabled = false;
            if (auto readHandle = nodeComponentManager_->Read(mesh.components[1])) {
                effectivelyEnabled = readHandle->effectivelyEnabled;
            }
            BASE_NS::string entityName;
            if (auto readHandle = nameComponentManager_->Read(mesh.components[2])) {
                entityName = readHandle->name;
            }
            SCENE_PLUGIN_VERBOSE_LOG("name: %s enabled: %i", entityName.c_str(), effectivelyEnabled);
            if (name == entityName || fullPath == entityName) {
                entity = mesh.entity;
                return true;
            }
        }
        // Could not find the mesh with given the name, try if it has a node
        const auto& root = nodeSystem_->GetRootNode();
        if (auto ecsNode = root.LookupNodeByPath(fullPath)) {
            entity = ecsNode->GetEntity();
            return true;
        }
    }
    return false;
}

BASE_NS::string SceneHolder::GetResourceId(CORE_NS::Entity entity)
{
    BASE_NS::string result;

    if (scene_) {
        result = scene_->GetUniqueIdentifierRecursive(entity);
    }
    return result;
}

bool SceneHolder::FindMaterial(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, Entity& entity)
{
    if (ecs_) {
        entity = scene_->GetEntityRecursive(fullPath);
        if (CORE_NS::EntityUtil::IsValid(entity)) {
            return true;
        }

        materialQuery_->Execute();
        auto materials = materialQuery_->GetResults();

        SCENE_PLUGIN_VERBOSE_LOG("found %zu materials", materials.size());

#ifdef NDEBUG
        if (name == KDUMMY_STRING) {
            return false;
        }
#endif

        for (auto&& material : materials) {
            bool effectivelyEnabled = false;
            if (auto readHandle = nodeComponentManager_->Read(material.components[1])) {
                effectivelyEnabled = readHandle->effectivelyEnabled;
            }
            BASE_NS::string entityName;
            if (auto readHandle = nameComponentManager_->Read(material.components[2])) {
                entityName = readHandle->name;
            }
            SCENE_PLUGIN_VERBOSE_LOG("name: %s enabled: %i", entityName.c_str(), effectivelyEnabled);
            if (name == entityName) {
                entity = material.entity;
                return true;
            }
            if (auto readHandle = uriComponentManager_->Read(material.components[3])) {
                entityName = readHandle->uri;
            }

            if (!entityName.empty()) {
                if (name == entityName || fullPath == entityName) {
                    entity = material.entity;
                    return true;
                }
            }
        }
        // Could not find the material with the given name, try if it has a node
        const auto& root = nodeSystem_->GetRootNode();
        if (auto ecsNode = root.LookupNodeByPath(fullPath)) {
            entity = ecsNode->GetEntity();
            return true;
        }
        // Desperate measures, to be rectified
        if (auto created = rootNode_->LookupNodeByPath(fullPath)) {
            entity = created->GetEntity();
            return true;
        }
    }
    return false;
}

bool SceneHolder::FindAnimation(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, Entity& entity)
{
    if (ecs_) {
        if (!animations_.empty()) {
            size_t ix = name.rfind(":");
            if (ix != BASE_NS::string_view::npos && ix != name.size() - 1) {
                char* dummy = nullptr;
                auto entityId = strtoll(name.substr(ix + 1).data(), &dummy, 16);
                if (animations_.find(entityId) != animations_.cend()) {
                    entity.id = entityId;
                    return true;
                }
            }

            for (auto&& animation : animations_) {
                if (META_NS::GetValue(interface_pointer_cast<META_NS::INamed>(animation.second)->Name()) == name) {
                    entity.id = animation.first;
                    return true;
                }
            }
        } else {
            animationQuery_->Execute();
            auto animations = animationQuery_->GetResults();

            SCENE_PLUGIN_VERBOSE_LOG("found %zu animations", animations.size());

#ifdef NDEBUG
            if (name == KDUMMY_STRING) {
                return false;
            }
#endif

            for (auto&& animation : animations) {
                bool effectivelyEnabled = false;
                if (auto readHandle = nodeComponentManager_->Read(animation.components[1])) {
                    effectivelyEnabled = readHandle->effectivelyEnabled;
                }
                BASE_NS::string entityName;
                if (auto readHandle = nameComponentManager_->Read(animation.components[2])) {
                    entityName = readHandle->name;
                }
                SCENE_PLUGIN_VERBOSE_LOG("name: %s enabled: %i", entityName.c_str(), effectivelyEnabled);
                if (name == entityName || fullPath == entityName) {
                    entity = animation.entity;
                    return true;
                }
            }
            // Could not find the animation with given the name, try if it has a node
            const auto& root = nodeSystem_->GetRootNode();
            if (auto ecsNode = root.LookupNodeByPath(fullPath)) {
                entity = ecsNode->GetEntity();
                return true;
            }
        }
    }
    return false;
}

void SceneHolder::ResolveAnimations()
{
    if (ecs_) {
        animationQuery_->Execute();
        auto animations = animationQuery_->GetResults();

        SCENE_PLUGIN_VERBOSE_LOG("%s, found %zu animations", __func__, animations.size());

        for (auto&& animation : animations) {
            if (animations_.find(animation.entity.id) == animations_.cend()) {
                animations_[animation.entity.id] =
                    GetObjectRegistry().Create<SCENE_NS::IEcsAnimation>(SCENE_NS::ClassId::EcsAnimation);
                if (auto ecsProxyIf =
                        interface_pointer_cast<SCENE_NS::IEcsProxyObject>(animations_[animation.entity.id])) {
                    ecsProxyIf->SetCommonListener(GetCommonEcsListener());
                }
                animations_[animation.entity.id]->SetEntity(*ecs_, animation.entity);
            }
        }
    }
}

void SceneHolder::UpdateAttachments(SCENE_NS::IEcsObject::Ptr& ecsObject)
{
    if (ecs_) {
        auto entity = ecsObject->GetEntity();
        if (EntityUtil::IsValid(entity)) {
            for (auto&& animation : animations_) {
                if (animation.second->GetRootEntity() == entity) {
                    ecsObject->AddAttachment(animation.second->GetEntity());
                }
            }
        }
    }
}

// This could be highly inefficient, in most cases we should have a clue of resource type
bool SceneHolder::FindResource(const BASE_NS::string_view name, const BASE_NS::string_view fullPath, Entity& entity)
{
    if (ecs_ && scene_) {
        if (FindAnimation(name, fullPath, entity) || FindMaterial(name, fullPath, entity) ||
            FindMesh(name, fullPath, entity)) {
            return true;
        }

        // traverse through scene collection
        for (size_t ix = 0; ix < scene_->GetSubCollectionCount(); ix++) {
            if (const auto& subcollection = scene_->GetSubCollection(ix); subcollection->GetUri() == fullPath) {
                // the assumption is that we do not have to dig deeper
                if (subcollection->GetEntityCount() > 0) {
                    entity = subcollection->GetEntity(0);
                    return true;
                }
            }
        }

        // Try finding an existing resource with an uri. This is needed e.g. for the images loaded from an glTF
        // file where the uri does not point to an actual image file but the resource is just a entity with uri in
        // ECS.
        if (renderMeshComponentManager_ && uriComponentManager_) {
            for (size_t i = 0; i < uriComponentManager_->GetComponentCount(); ++i) {
                const Entity currentEntity =
                    uriComponentManager_->GetEntity(static_cast<IComponentManager::ComponentId>(i));
                if (ecs_->GetEntityManager().IsAlive(currentEntity)) {
                    if (auto uriComponent = uriComponentManager_->Read(currentEntity);
                        uriComponent && uriComponent->uri == fullPath) {
                        if (renderMeshComponentManager_->HasComponent(currentEntity)) {
                            entity = currentEntity;
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

bool SceneHolder::GetImageEntity(CORE_NS::Entity material, size_t index, CORE_NS::Entity& entity)
{
    bool ret { false };
    if (ecs_ && scene_ && CORE_NS::EntityUtil::IsValid(material)) {
        if (auto handle = materialComponentManager_->Read(material)) {
            entity = handle->textures[index].image;
            ret = true;
        }
    }

    return ret;
}

bool SceneHolder::GetRenderHandleUri(const RENDER_NS::RenderHandle& handle, BASE_NS::string& uriString)
{
    if (ecs_) {
        auto& renderUtil = graphicsContext3D_->GetRenderContext().GetRenderUtil();

        size_t index { 0 };
        while (auto handleReference = rhComponentManager_->GetRenderHandleReference(index)) {
            if (handleReference.GetHandle() == handle) {
                auto& renderUtil = graphicsContext3D_->GetRenderContext().GetRenderUtil();
                const auto desc = renderUtil.GetRenderHandleDesc(handleReference);
                if (desc.type == RenderHandleType::GPU_IMAGE && !desc.name.empty()) {
                    // Note: assuming that the name is the image uri. Ignore the name if not in uri format.
                    if (desc.name.find(":/") != BASE_NS::string::npos) {
                        uriString = desc.name;
                        return true;
                    }
                }
            }
            index++;
        }
    }
    return false;
}

CORE_NS::Entity SceneHolder::CreatePostProcess()
{
    CORE_NS::Entity entity;
    if (ecs_) {
        auto pm = CORE_NS::GetManager<CORE3D_NS::IPostProcessComponentManager>(*ecs_);
        entity = ecs_->GetEntityManager().Create();
        // createComponent
        pm->Create(entity);
    }
    return entity;
}

CORE_NS::Entity SceneHolder::CreateRenderConfiguration()
{
    CORE_NS::Entity entity;
    if (ecs_) {
        entity = ecs_->GetEntityManager().Create();

        auto rcm = CORE_NS::GetManager<CORE3D_NS::IRenderConfigurationComponentManager>(*ecs_);
        rcm->Create(entity);
    }
    return entity;
}

void SceneHolder::SetRenderHandle(const CORE_NS::Entity& target, const CORE_NS::Entity& source)
{
    if (ecs_) {
        if (auto sourceHandle = rhComponentManager_->GetRenderHandleReference(source)) {
            if (!rhComponentManager_->HasComponent(target)) {
                rhComponentManager_->Create(target);
            }
            if (auto handle = rhComponentManager_->Write(target)) {
                handle->reference = sourceHandle;
            }
        }
    }
}

bool SceneHolder::GetEntityUri(const CORE_NS::Entity& entity, BASE_NS::string& uriString)
{
    bool ret { false };
    if (ecs_ && scene_ && CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto handle = uriComponentManager_->Read(entity)) {
            uriString = BASE_NS::string(handle->uri.data(), handle->uri.size());
            ret = true;
        }
    }

    if (!ret) {
        // Special handling for render handles (automatically loaded images in the ecs are now just render handles
        // with a name containing the uri).
        auto handle = rhComponentManager_->GetRenderHandleReference(entity);
        if (handle) {
            auto& renderUtil = renderContext_->GetRenderUtil();
            const auto desc = renderUtil.GetRenderHandleDesc(handle);
            if (desc.type == RenderHandleType::GPU_IMAGE && !desc.name.empty()) {
                // Note: assuming that the name is the image uri. Ignore the name if not in uri format.
                if (desc.name.find(":/") != BASE_NS::string::npos) {
                    uriString = desc.name;
                    ret = true;
                }
            }
        }
    }

    return ret;
}
bool SceneHolder::GetImageHandle(
    const CORE_NS::Entity& entity, RENDER_NS::RenderHandleReference& handle, RENDER_NS::GpuImageDesc& desc)
{
    if (rhComponentManager_->HasComponent(entity)) {
        handle = rhComponentManager_->GetRenderHandleReference(entity);
        desc = renderContext_->GetDevice().GetGpuResourceManager().GetImageDescriptor(handle);
        return true;
    }
    return false;
}

bool SceneHolder::SetEntityUri(const CORE_NS::Entity& entity, const BASE_NS::string& uriString)
{
    bool ret { false };
    if (ecs_ && scene_ && CORE_NS::EntityUtil::IsValid(entity)) {
        if (!uriComponentManager_->HasComponent(entity)) {
            uriComponentManager_->Create(entity);
        }

        if (auto handle = uriComponentManager_->Write(entity)) {
            handle->uri = uriString;
            ret = true;
        }
    }

    return ret;
}

bool SceneHolder::GetEntityName(const CORE_NS::Entity& entity, BASE_NS::string& nameString)
{
    bool ret { false };
    if (ecs_ && scene_ && CORE_NS::EntityUtil::IsValid(entity)) {
        if (auto handle = nameComponentManager_->Read(entity)) {
            nameString = BASE_NS::string(handle->name.data(), handle->name.size());
            ret = true;
        }
    }

    return ret;
}

CORE_NS::Entity SceneHolder::GetEntityByUri(BASE_NS::string_view uriString)
{
    for (size_t i = 0; i < uriComponentManager_->GetComponentCount(); ++i) {
        const Entity entity = uriComponentManager_->GetEntity(static_cast<IComponentManager::ComponentId>(i));
        if (ecs_->GetEntityManager().IsAlive(entity)) {
            if (auto uriComponent = uriComponentManager_->Read(entity);
                uriComponent && uriComponent->uri == uriString) {
                return entity;
            }
        }
    }

    return {};
}

bool ResolveNodeFullPath(CORE3D_NS::ISceneNode* node, BASE_NS::string& path)
{
    if (node) {
        path = node->GetName();
        while ((node = node->GetParent())) {
            path.insert(0, "/");
            auto name = node->GetName();
            path.insert(0, name.data(), name.size());
        }
    }
    return (!path.empty());
}

BASE_NS::string ResolveNodeFullPath(CORE_NS::IEcs& ecs, const CORE_NS::Entity& entity)
{
    CORE3D_NS::INodeSystem& nodeSystem = *CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
    BASE_NS::string ret;
    ResolveNodeFullPath(nodeSystem.GetNode(entity), ret);
    return ret;
}

void SceneHolder::LoadScene()
{
    // Reset scene to default.
    CORE_LOG_I("Loading scene: '%s'", sceneUri_.c_str());
    uint32_t loadingStatus = SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED;
    bool replacedRoot { false };

    while (true) {
        if (sceneUri_.empty()) {
            ResetScene();
            loadingStatus = SCENE_NS::IScene::SCENE_STATUS_UNINITIALIZED;
            break;
        }

        auto params = SCENE_NS::PathUtil::GetUriParameters(sceneUri_);
        auto ite = params.find("target");
        if (!scene_ || ite == params.end() || ite->second.empty()) {
            ResetScene(true);
            replacedRoot = true;
        } else {
            CORE_LOG_I("Load into scene: %s", ite->second.c_str());
        }

        // If ecs initializing failed, report it back and return
        if (!ecs_) {
            break;
        }

        if (assetManager_) {
            if ((sceneUri_ != "scene://empty") && !assetManager_->LoadAsset(*scene_, sceneUri_, "project://")) {
                CORE_LOG_E("Loading scene %s failed", sceneUri_.c_str());
                ResetScene();
                break;
            } else { // ToDo: This is currently done also when a node is added to container, so no need to do it
                     // here
                // Rearrange scene to have a concrete common root node, this will be more important for prefabs
                CORE3D_NS::INodeSystem& nodeSystem = *CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*ecs_);
                auto& root = nodeSystem.GetRootNode();

                // ToDo: should this fetch the named root node instead
                auto ourRoot = root.GetChildren().at(0);
                ourRoot->SetEnabled(true);

                auto ite = root.GetChildren().begin() + 1;
                while (ite != root.GetChildren().end()) {
                    (*ite)->SetParent(*ourRoot);
                    ite = root.GetChildren().begin() + 1;
                }
            }

            // Loading might have cached some assets (in case there are multiple
            // instances). Scene widget will not need them after loading so clear the
            // cache.
            if (replacedRoot) {
                assetManager_->ClearCache();
            }
        }
        if (sceneUri_ != "scene://empty") {
            // Set default camera.
            SetDefaultCamera();
        }
        // ask synchronous update
        RenderCameras();
        loadingStatus = SCENE_NS::IScene::SCENE_STATUS_READY;
        break;
    }

    // If ecs initialization fails, all we can do is to retry loading later
    if (!ecs_) {
        loadSceneFailed_ = true;

        if (sceneLoadedCallback_) {
            QueueApplicationTask(MakeTask(
                                     [](auto sceneLoadedCallback, auto loadingStatus) {
                                         if (sceneLoadedCallback)
                                             sceneLoadedCallback->Invoke(loadingStatus);
                                         return false;
                                     },
                                     sceneLoadedCallback_, loadingStatus),
                false);
        }
        return;
    }

    // make sure that components get updated
    ProcessEvents();

    IntrospectNodeless();
    // Call back the app thread, notify that the graphics and ecs are ready. This
    // is not quite true, though
    if (ecs_ && replacedRoot && sceneInitializedCallback_) {
        BASE_NS::string id { rootNode_->GetName() };
        BASE_NS::string cameraId = ResolveNodeFullPath(*ecs_, defaultCameraEntity_);
        QueueApplicationTask(MakeTask(
                                 [](auto sceneInitializedCallback, auto id, auto cameraId) {
                                     if (sceneInitializedCallback) {
                                         sceneInitializedCallback->Invoke(id, cameraId);
                                     }
                                     return false;
                                 },
                                 sceneInitializedCallback_, id, cameraId),
            false);
    } else if (sceneLoadedCallback_) { // Call back the app thread, notify that new scene is loaded
        QueueApplicationTask(MakeTask(
                                 [](auto sceneLoadedCallback, auto loadingStatus) {
                                     if (sceneLoadedCallback) {
                                         sceneLoadedCallback->Invoke(loadingStatus);
                                     }
                                     return false;
                                 },
                                 sceneLoadedCallback_, loadingStatus),
            false);
    }
}

void SceneHolder::ChangeCamera(SCENE_NS::ICamera::Ptr camera)
{
    if (auto cameraObject = interface_pointer_cast<SCENE_NS::IEcsObject>(camera)) {
        auto entity = cameraObject->GetEntity();
        if (EntityUtil::IsValid(entity)) {
            SetMainCamera(entity);
        }
    }
}

void logNodes(const CORE3D_NS::ISceneNode& node, BASE_NS::string path)
{
    if (!path.empty() && path.back() != '/') {
        path.append("/");
    }
    if (!node.GetName().empty()) {
        path.append(node.GetName());
    } else {
        path.append("[");
        path.append(BASE_NS::to_string(node.GetEntity().id));
        path.append("]");
    }
    CORE_LOG_I("%s", path.c_str());

    for (const auto child : node.GetChildren())
        logNodes(*child, path);
}

void SceneHolder::SaveScene(const BASE_NS::string& fileName)
{
    assetManager_->SaveJsonEntityCollection(*scene_.get(), fileName.empty() ? sceneUri_ : fileName, "file://");

    // Verbose logs, to be suppressed at some point
    CORE3D_NS::INodeSystem& nodeSystem = *GetSystem<CORE3D_NS::INodeSystem>(*ecs_);
    logNodes(nodeSystem.GetRootNode(), "/");
    IntrospectNodeless();
}

CORE3D_NS::ISceneNode* SceneHolder::CreateNode(const BASE_NS::string& name)
{
    return CreateNode({}, name);
}

CORE3D_NS::ISceneNode* SceneHolder::CreateNode(const BASE_NS::string& path, const BASE_NS::string& name)
{
    if (ecs_) {
        auto pathWithoutRootNode = RemoveRootNodeFromPath(path);

        CORE3D_NS::INodeSystem& nodeSystem = *GetSystem<CORE3D_NS::INodeSystem>(*ecs_);

        CORE3D_NS::ISceneNode* parent = nullptr;
        if (pathWithoutRootNode.empty()) {
            parent = rootNode_;
        } else {
            parent = rootNode_->LookupNodeByPath(pathWithoutRootNode);
        }

        CORE_ASSERT(parent);

        auto instanceRoot = nodeSystem.CreateNode();
        instanceRoot->SetName(name);
        instanceRoot->SetParent(*parent);
        return instanceRoot;
    } else {
        CORE_LOG_E("%s: Ecs not available, can not create node %s", __func__, name.c_str());
    }

    return nullptr;
}

CORE_NS::Entity SceneHolder::CreateMaterial(const BASE_NS::string& name)
{
    if (ecs_) {
        auto entity = ecs_->GetEntityManager().Create();

        RenameEntity(entity, name);

        materialComponentManager_->Create(entity);
        scene_->AddEntityToSubcollection("created_materials", name, entity);
        ProcessEvents();
        return entity;

    } else {
        CORE_LOG_E("%s: Ecs not available, can not create material %s", __func__, name.c_str());
    }

    return {};
}

void SceneHolder::RenameEntity(CORE_NS::Entity entity, const BASE_NS::string& name)
{
    if (ecs_) {
        // Name Component
        if (!nameComponentManager_->HasComponent(entity)) {
            nameComponentManager_->Create(entity);
        }
        if (auto nameHandle = nameComponentManager_->Write(entity)) {
            nameHandle->name = name;
        }

        // Root level entity collection item
        if (BASE_NS::string_view prevName = scene_->GetUniqueIdentifier(entity); !prevName.empty()) {
            scene_->SetUniqueIdentifier(name, scene_->GetEntity(prevName));
        } else if (nodeComponentManager_->HasComponent(entity)) {
            if (auto ecsNode = nodeSystem_->GetNode(entity)) {
                if (const auto& children = ecsNode->GetChildren(); children.size()) {
                    if (auto ix = scene_->GetSubCollectionIndexByRoot(children[0]->GetEntity()); ix != -1) {
                        scene_->GetSubCollection(ix)->SetUri(name);
                    }
                }
            }
        }
        // Experimentals: Subcollection root (will not traverse further, the rest should go through normal property
        // serialization)
    }
}

// Clone the entity, reference will be stored
CORE_NS::Entity SceneHolder::CloneEntity(CORE_NS::Entity entity, const BASE_NS::string& name, bool storeWithUniqueId)
{
    CORE_NS::Entity ret {};

    if (ecs_ && scene_) {
        ret = ecs_->GetEntityManager().Create();
        CORE_NS::CloneComponents(*ecs_, entity, *ecs_, ret);
        RenameEntity(ret, name);
        // when cloning, we trust that implementation uses unique name so no padding required
        scene_->AddEntityToSubcollection("cloned_entities", name, ret, storeWithUniqueId);
    }
    return ret;
}

void SceneHolder::DestroyEntity(CORE_NS::Entity entity)
{
    // Destroy created materials & meshes here.
    auto meshCollectionIndex = scene_->GetSubCollectionIndex("GLTF_Meshes");
    if (meshCollectionIndex != -1) {
        auto meshCollection = scene_->GetSubCollection(meshCollectionIndex);
        if (meshCollection->Contains(entity)) {
            auto ref = meshCollection->GetReference(entity);
            meshCollection->RemoveEntity(ref);
        }
    }

    auto materialCollectionIndex = scene_->GetSubCollectionIndex("GLTF_Materials");
    if (materialCollectionIndex != -1) {
        auto materialCollection = scene_->GetSubCollection(materialCollectionIndex);
        if (materialCollection->Contains(entity)) {
            auto ref = materialCollection->GetReference(entity);
            materialCollection->RemoveEntity(ref);
        }
    }
    scene_->RemoveEntityRecursive(entity);
}

void SceneHolder::SetEntityActive(CORE_NS::Entity entity, bool active)
{
    ecs_->GetEntityManager().SetActive(entity, active);
}

bool SceneHolder::ReparentEntity(CORE_NS::Entity entity, const BASE_NS::string& parentPath, size_t index)
{
    auto& root = nodeSystem_->GetRootNode();

    auto node = nodeSystem_->GetNode(entity);
    if (!node) {
        CORE_LOG_W("SceneHolder::ReparentEntity, Failed to find node from entity.");
        return false;
    }

    auto parentNode = root.LookupNodeByPath(parentPath);
    if (!parentNode) {
        if (parentPath == "/" || parentPath.empty()) {
            parentNode = &root;
        } else {
            CORE_LOG_W("SceneHolder::ReparentEntity, Failed to find parent node '%s'", parentPath.c_str());
            return false;
        }
    }

    if (parentNode != node->GetParent()) {
        if (index != SIZE_MAX) {
            parentNode->InsertChild(index, *node);
        } else {
            parentNode->AddChild(*node);
        }
    }

    return true;
}

const CORE3D_NS::ISceneNode* SceneHolder::ReparentEntity(const BASE_NS::string& parentPath, const BASE_NS::string& name)
{
    if (ecs_) {
        const auto& root = nodeSystem_->GetRootNode();
        auto ecsNode = root.LookupNodeByPath(name);
        auto parentNode = root.LookupNodeByPath(parentPath);

        if (!ecsNode && parentNode) {
            if (ecsNode = parentNode->LookupNodeByPath(name); !ecsNode) {
                CORE_LOG_W("Could not find: '%s', tried root and parent '%s'", name.c_str(), parentPath.c_str());
            }
        } else if (parentNode && parentNode != ecsNode) { // prefabs may get have their path pointing them self
                                                          // before they are positioned into scene
            const_cast<CORE3D_NS::ISceneNode*>(ecsNode)->SetParent(*parentNode);
            SCENE_PLUGIN_VERBOSE_LOG("reparenting '%s' to '%s'", name.c_str(), parentPath.c_str());
        } else if (!parentPath.empty() && parentPath != "/") {
            CORE_LOG_W("Could not find parent '%s'", parentPath.c_str());
            // Even parent is not found, we can still activate the bindings, still we could
            // reschedule retry here
        }
        return ecsNode;
    }
    return {};
}

void SceneHolder::SetMesh(CORE_NS::Entity targetEntity, CORE_NS::Entity mesh)
{
    if (ecs_) {
        if (EntityUtil::IsValid(targetEntity) && EntityUtil::IsValid(mesh)) {
            // ToDo: should perhaps generate render mesh component for entity if it does not exist yet?

            if (!renderMeshComponentManager_->HasComponent(targetEntity)) {
                renderMeshComponentManager_->Create(targetEntity);
            }

            if (auto handle = renderMeshComponentManager_->Write(targetEntity)) {
                handle->mesh = mesh;
            }
        }
    }
}

CORE_NS::Entity SceneHolder::GetMaterial(CORE_NS::Entity meshEntity, int64_t submeshIndex)
{
    CORE_NS::Entity material;
    if (ecs_) {
        if (EntityUtil::IsValid(meshEntity)) {
            if (auto handle = meshComponentManager_->Read(meshEntity)) {
                if (submeshIndex >= 0 && submeshIndex < handle->submeshes.size()) {
                    material = handle->submeshes[submeshIndex].material;
                }
            }
        }
    }
    return material;
}

BASE_NS::string_view SceneHolder::GetMaterialName(CORE_NS::Entity meshEntity, int64_t submeshIndex)
{
    BASE_NS::string_view entityName;
    auto entity = GetMaterial(meshEntity, submeshIndex);

    if (!GetEntityId(entity, entityName, *scene_.get(), "GLTF_Materials", *nameComponentManager_)) {
        CORE_LOG_W("%s: could not find material", __func__);
    }

    return entityName;
}

BASE_NS::string_view SceneHolder::GetMeshName(CORE_NS::Entity referringEntity)
{
    BASE_NS::string_view entityName;

    if (EntityUtil::IsValid(referringEntity)) {
        CORE_NS::Entity meshEntity;

        if (renderMeshComponentManager_->HasComponent(referringEntity)) {
            if (auto handle = renderMeshComponentManager_->Read(referringEntity)) {
                meshEntity = handle->mesh;
            }
        } else {
            CORE_LOG_W("%s: could not find mesh from entity", __func__);
        }
        if (!GetEntityId(meshEntity, entityName, *scene_.get(), "GLTF_Meshes", *nameComponentManager_)) {
            CORE_LOG_W("%s: could not find valid mesh", __func__);
        }
    }
    return entityName;
}

void SceneHolder::SetMaterial(CORE_NS::Entity targetEntity, CORE_NS::Entity material, int64_t submeshIndex)
{
    if (ecs_) {
        if (EntityUtil::IsValid(targetEntity)) {
            // ToDo: should perhaps generate mesh component for entity if it does not exist yet?

            if (auto handle = meshComponentManager_->Write(targetEntity)) {
                if (submeshIndex == -1) {
                    for (auto&& submesh : handle->submeshes) {
                        submesh.material = material;
                    }
                } else if (submeshIndex >= 0 && submeshIndex < handle->submeshes.size()) {
                    handle->submeshes[submeshIndex].material = material;
                }
            }
        }
    }
}

CORE_NS::EntityReference SceneHolder::BindUIBitmap(SCENE_NS::IBitmap::Ptr bitmap, bool createNew)
{
    if (ecs_) {
        BASE_NS::string uri;
        RENDER_NS::RenderHandleReference imageHandle {};

        // Need two things, uri
        if (auto uriBmp = interface_pointer_cast<SCENE_NS::IBitmap>(bitmap)) {
            uri = uriBmp->Uri()->GetValue();
        } else {
            uri = interface_pointer_cast<META_NS::IObjectInstance>(bitmap)->GetInstanceId().ToString();
        }

        // And concrete handle to data
        if (auto lumeBmp = interface_pointer_cast<SCENE_NS::IBitmap>(bitmap)) {
            imageHandle = lumeBmp->GetRenderHandle();
        }

        if (imageHandle) {
            // check if we have existing resource
            CORE_NS::Entity existing;
            if (FindResource(uri, uri, existing)) {
                auto hande = rhComponentManager_->Get(existing);
                hande.reference = imageHandle;
                rhComponentManager_->Set(existing, hande);

                return ecs_->GetEntityManager().GetReferenceCounted(existing);
            }

            if (!createNew) {
                return {};
            }

            // if not, create new one
            EntityReference entity = ecs_->GetEntityManager().CreateReferenceCounted();

            nameComponentManager_->Create(entity);
            auto component = nameComponentManager_->Get(entity);
            component.name = uri;
            nameComponentManager_->Set(entity, component);

            // Not sure if we should do this, somehow we need to keep the entity alive
            // with proper uri it could be found afterwards
            auto& subcollection = scene_->AddSubCollection(uri, "bitmap://");
            subcollection.AddEntity(entity);

            // Or this. We should perhaps use URI component more
            rhComponentManager_->Create(entity);
            auto hande = rhComponentManager_->Get(entity);
            hande.reference = imageHandle;
            rhComponentManager_->Set(entity, hande);
            return entity;
        }
    }

    return {};
}

void SceneHolder::SetTexture(size_t index, CORE_NS::Entity targetEntity, CORE_NS::EntityReference imageEntity)
{
    if (ecs_) {
        if (auto handle = materialComponentManager_->Write(targetEntity)) {
            handle->textures[index].image = imageEntity;
        }
    }
}

BASE_NS::string_view ResolveSamplerUri(SCENE_NS::ITextureInfo::SamplerId samplerId)
{
    switch (samplerId) {
        case SCENE_NS::ITextureInfo::CORE_DEFAULT_SAMPLER_NEAREST_REPEAT:
            return "engine://CORE_DEFAULT_SAMPLER_NEAREST_REPEAT";
        case SCENE_NS::ITextureInfo::CORE_DEFAULT_SAMPLER_NEAREST_CLAMP:
            return "engine://CORE_DEFAULT_SAMPLER_NEAREST_CLAMP";
        case SCENE_NS::ITextureInfo::CORE_DEFAULT_SAMPLER_LINEAR_REPEAT:
            return "engine://CORE_DEFAULT_SAMPLER_LINEAR_REPEAT";
        case SCENE_NS::ITextureInfo::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP:
            return "engine://CORE_DEFAULT_SAMPLER_LINEAR_CLAMP";
        case SCENE_NS::ITextureInfo::CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT:
            return "engine://CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT";
        case SCENE_NS::ITextureInfo::CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP:
            return "engine://CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP";
        default:
            CORE_LOG_W("%s: unable to find sampler uri for: %d", __func__, samplerId);
    }

    return "";
}

static constexpr size_t ENGINE_URI_PREFIX_LEN = BASE_NS::string_view("engine://").size();

SCENE_NS::IShader::Ptr SceneHolder::GetShader(CORE_NS::Entity materialEntity, ShaderType type)
{
    // Resolve render handle from material.
    if (!ecs_) {
        return {};
    }

    if (!EntityUtil::IsValid(materialEntity)) {
        return {};
    }

    RENDER_NS::RenderHandleReference shaderHandleRef;

    EntityReference shaderEntityRef {};
    if (auto readHandle = materialComponentManager_->Read(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            shaderEntityRef = readHandle->materialShader.shader;
        } else if (type == ShaderType::DEPTH_SHADER) {
            shaderEntityRef = readHandle->depthShader.shader;
        }
    }

    if (shaderEntityRef) {
        auto rh = rhComponentManager_->GetRenderHandleReference(shaderEntityRef);
        if (RENDER_NS::RenderHandleUtil::IsValid(rh.GetHandle())) {
            auto uri = GetHandleUri(rh, HANDLE_TYPE_SHADER);
            if (!uri.empty()) {
                auto shader = SCENE_NS::Shader();

                auto shaderInterface = interface_pointer_cast<SCENE_NS::IShader>(shader);
                SetValue(shaderInterface->Uri(), uri);

                return shader;
            }
        }
    }

    return {};
}

SCENE_NS::IGraphicsState::Ptr SceneHolder::GetGraphicsState(CORE_NS::Entity materialEntity, ShaderType type)
{
    // Resolve render handle from material.
    if (!ecs_) {
        return {};
    }

    if (!EntityUtil::IsValid(materialEntity)) {
        return {};
    }

    RENDER_NS::RenderHandleReference shaderHandleRef;

    EntityReference stateEntityRef {};
    if (auto readHandle = materialComponentManager_->Read(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            stateEntityRef = readHandle->materialShader.graphicsState;
        } else if (type == ShaderType::DEPTH_SHADER) {
            stateEntityRef = readHandle->depthShader.graphicsState;
        }
    }

    if (stateEntityRef) {
        auto rh = rhComponentManager_->GetRenderHandleReference(stateEntityRef);
        if (RENDER_NS::RenderHandleUtil::IsValid(rh.GetHandle())) {
            auto uri = GetHandleUri(rh);
            if (!uri.empty()) {
                auto state = SCENE_NS::GraphicsState();

                auto interface = interface_pointer_cast<SCENE_NS::IGraphicsState>(state);

                // The uri contains either .shader or .shadergs.
                BASE_NS::string extension = ".shadergs";
                if (uri.find(extension) == BASE_NS::string::npos) {
                    extension = ".shader";
                }

                // Split the uri to uri and variant.
                auto ix = uri.find_last_of(extension);
                if (ix != BASE_NS::string_view::npos && ix < uri.size() - 1) {
                    SetValue(interface->Uri(), BASE_NS::string(uri.substr(0, ix + 1)));
                    SetValue(interface->Variant(), BASE_NS::string(uri.substr(ix + 1)));
                    SCENE_PLUGIN_VERBOSE_LOG("Got graphics state: %s variant: %s",
                        META_NS::GetValue(interface->Uri()).c_str(), META_NS::GetValue(interface->Variant()).c_str());
                } else {
                    SetValue(interface->Uri(), uri);
                    SetValue(interface->Variant(), "");
                }

                if (auto shRef = interface_cast<ISceneHolderRef>(state)) {
                    shRef->SetSceneHolder(me_);
                    shRef->SetIndex(type);
                }

                return state;
            }
        }
    }

    return {};
}

void SceneHolder::SetGraphicsState(
    CORE_NS::Entity materialEntity, ShaderType type, const RENDER_NS::GraphicsState& state)
{
    if (!ecs_) {
        return;
    }

    if (!EntityUtil::IsValid(materialEntity)) {
        return;
    }

    CORE_NS::EntityReference entityRef;
    if (auto readHandle = materialComponentManager_->Read(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            entityRef = readHandle->materialShader.graphicsState;
        } else if (type == ShaderType::DEPTH_SHADER) {
            entityRef = readHandle->depthShader.graphicsState;
        }
    }

    auto rh = rhComponentManager_->GetRenderHandleReference(entityRef);
    if (RENDER_NS::RenderHandleUtil::IsValid(rh.GetHandle())) {
        auto& engineShaderManager = renderContext_->GetDevice().GetShaderManager();
        auto hash = engineShaderManager.HashGraphicsState(state);
        auto currentState = engineShaderManager.GetGraphicsState(rh);
        if (hash != engineShaderManager.HashGraphicsState(currentState)) {
            auto desc = engineShaderManager.GetIdDesc(rh);
            engineShaderManager.CreateGraphicsState({ desc.path, state });
        }
    }
}

bool SceneHolder::GetGraphicsState(
    CORE_NS::Entity materialEntity, ShaderType type, const SCENE_NS::IShaderGraphicsState::Ptr& ret)
{
    if (!ecs_) {
        return false;
    }

    if (!EntityUtil::IsValid(materialEntity)) {
        return false;
    }

    CORE_NS::EntityReference entityRef;
    if (auto readHandle = materialComponentManager_->Read(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            entityRef = readHandle->materialShader.graphicsState;
        } else if (type == ShaderType::DEPTH_SHADER) {
            entityRef = readHandle->depthShader.graphicsState;
        }
    }
    auto rh = rhComponentManager_->GetRenderHandleReference(entityRef);
    if (RENDER_NS::RenderHandleUtil::IsValid(rh.GetHandle())) {
        auto& engineShaderManager = renderContext_->GetDevice().GetShaderManager();
        auto state = engineShaderManager.GetGraphicsState(rh);
        if (auto typed = interface_cast<SCENE_NS::IPendingRequestData<RENDER_NS::GraphicsState>>(ret)) {
            typed->Add(state);
            return true;
        }
    }
    return false;
}

void SceneHolder::SetShader(CORE_NS::Entity materialEntity, ShaderType type, SCENE_NS::IShader::Ptr shader)
{
    if (!ecs_) {
        return;
    }

    if (!EntityUtil::IsValid(materialEntity)) {
        return;
    }

    EntityReference shaderEntityRef {};
    RenderHandleReference rh {};

    auto& engineShaderManager = renderContext_->GetDevice().GetShaderManager();
    if (shader) {
        rh = shader->GetRenderHandleReference(engineShaderManager);
        auto uri = META_NS::GetValue(shader->Uri());

        if (RenderHandleUtil::IsValid(rh.GetHandle())) {
            shaderEntityRef = GetOrCreateEntityReference(ecs_->GetEntityManager(), *rhComponentManager_, rh);
            // Studio used to update also uri component, maybe someone else should do this
            // if needed to begin with
            if (!uriComponentManager_->HasComponent(shaderEntityRef)) {
                uriComponentManager_->Create(shaderEntityRef);
            }
            if (auto handle = uriComponentManager_->Write(shaderEntityRef)) {
                handle->uri = uri;
            }
        } else {
            CORE_LOG_W("Failed to set shader, invalid render handle: %s", uri.c_str());
            return;
        }
        if (auto shref = interface_cast<ISceneHolderRef>(shader)) {
            shref->SetSceneHolder(me_);
            shref->SetIndex(type);
        }
    }

    // Only writing the value if it has changed.
    bool valueChanged = false;

    if (auto readHandle = materialComponentManager_->Read(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            valueChanged = readHandle->materialShader.shader != shaderEntityRef;
        } else if (type == ShaderType::DEPTH_SHADER) {
            valueChanged = readHandle->depthShader.shader != shaderEntityRef;
        }
    }

    if (!valueChanged) {
        return;
    }

    if (auto writeHandle = materialComponentManager_->Write(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            writeHandle->materialShader.shader = shaderEntityRef;
            if (!writeHandle->materialShader.graphicsState) {
                // initialize the graphics state handle, so we can resolve it later
                if (auto graphicsStateHandle = engineShaderManager.GetGraphicsStateHandleByShaderHandle(rh)) {
                    writeHandle->materialShader.graphicsState =
                        GetOrCreateEntityReference(ecs_->GetEntityManager(), *rhComponentManager_, graphicsStateHandle);
                }
            }
        } else if (type == ShaderType::DEPTH_SHADER) {
            writeHandle->depthShader.shader = shaderEntityRef;
            if (!writeHandle->depthShader.graphicsState) {
                if (auto graphicsStateHandle = engineShaderManager.GetGraphicsStateHandleByShaderHandle(rh)) {
                    writeHandle->depthShader.graphicsState =
                        GetOrCreateEntityReference(ecs_->GetEntityManager(), *rhComponentManager_, graphicsStateHandle);
                }
            }
        }
    }

    ProcessEvents();
}

void SceneHolder::SetGraphicsState(CORE_NS::Entity materialEntity, ShaderType type, SCENE_NS::IGraphicsState::Ptr state)
{
    if (!ecs_) {
        return;
    }

    if (!EntityUtil::IsValid(materialEntity)) {
        return;
    }

    EntityReference stateEntityRef {};
    if (state) {
        auto& engineShaderManager = renderContext_->GetDevice().GetShaderManager();
        auto rh = state->GetRenderHandleReference(engineShaderManager);

        if (RenderHandleUtil::IsValid(rh.GetHandle())) {
            auto uri = META_NS::GetValue(state->Uri());
            auto variant = META_NS::GetValue(state->Variant());

            stateEntityRef = GetOrCreateEntityReference(ecs_->GetEntityManager(), *rhComponentManager_, rh);
            // Studio used to update also uri component, maybe someone else should do this
            // if needed to begin with
            if (!uriComponentManager_->HasComponent(stateEntityRef)) {
                uriComponentManager_->Create(stateEntityRef);
            }
            if (auto handle = uriComponentManager_->Write(stateEntityRef)) {
                handle->uri = uri.append(variant);
            }
        } else {
            CORE_LOG_W("Failed to set shader, invalid render handle: %s", META_NS::GetValue(state->Uri()).c_str());
            return;
        }
        if (auto shref = interface_cast<ISceneHolderRef>(state)) {
            shref->SetSceneHolder(me_);
            shref->SetIndex(type);
        }
    }

    // Only writing the value if it has changed.
    bool valueChanged = false;

    if (auto readHandle = materialComponentManager_->Read(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            valueChanged = readHandle->materialShader.graphicsState != stateEntityRef;
        } else if (type == ShaderType::DEPTH_SHADER) {
            valueChanged = readHandle->depthShader.graphicsState != stateEntityRef;
        }
    }

    if (!valueChanged) {
        return;
    }

    if (auto writeHandle = materialComponentManager_->Write(materialEntity)) {
        if (type == ShaderType::MATERIAL_SHADER) {
            writeHandle->materialShader.graphicsState = stateEntityRef;
        } else if (type == ShaderType::DEPTH_SHADER) {
            writeHandle->depthShader.graphicsState = stateEntityRef;
        }
    }

    ProcessEvents();
}

BASE_NS::string SceneHolder::GetHandleUri(RENDER_NS::RenderHandleReference renderHandleReference, UriHandleType type)
{
    if (ecs_ && renderContext_) {
        auto& device = renderContext_->GetDevice();

        auto& shaderManager = device.GetShaderManager();

        if ((type == HANDLE_TYPE_SHADER && shaderManager.IsShader(renderHandleReference)) ||
            (type == HANDLE_TYPE_DO_NOT_CARE)) {
            auto desc = shaderManager.GetIdDesc(renderHandleReference);
            return desc.path;
        }
    }
    return BASE_NS::string();
}

CORE_NS::Entity SceneHolder::LoadSampler(BASE_NS::string_view uri)
{
    CORE_NS::Entity ret;
    if (ecs_ && renderContext_) {
        const auto name = uri.substr(ENGINE_URI_PREFIX_LEN);
        auto linearHandle = renderContext_->GetDevice().GetGpuResourceManager().GetSamplerHandle(name);

        auto entity = rhComponentManager_->GetEntityWithReference(linearHandle);
        if (!EntityUtil::IsValid(entity)) {
            // No existing sampler found, prepare one
            auto uriManager = GetManager<CORE3D_NS::IUriComponentManager>(*ecs_);
            entity = ecs_->GetEntityManager().Create();

            uriManager->Create(entity);
            uriManager->Write(entity)->uri = uri;

            rhComponentManager_->Create(entity);
            rhComponentManager_->Write(entity)->reference = linearHandle;

            RenameEntity(entity, BASE_NS::string(name.data(), name.size()));
            scene_->AddEntityToSubcollection("samplers", name, entity, false);
        }
        ret = entity;
    }

    return ret;
}

CORE_NS::EntityReference SceneHolder::LoadImage(BASE_NS::string_view uri, RENDER_NS::RenderHandleReference rh)
{
    CORE_NS::Entity ret;
    if (FindResource(uri, uri, ret)) {
        return ecs_->GetEntityManager().GetReferenceCounted(ret);
    }

    if (scene_ && assetManager_) {
        RENDER_NS::RenderHandleReference imageHandle;
        if (rh) {
            imageHandle = rh;
        } else {
            imageHandle = assetManager_->GetEcsSerializer().LoadImageResource(uri);
        }
        if (imageHandle) {
            return GetOrCreateEntityReference(ecs_->GetEntityManager(), *rhComponentManager_, imageHandle);
        }
    }

    return {};
}

void SceneHolder::SetSampler(size_t index, CORE_NS::Entity targetEntity, SCENE_NS::ITextureInfo::SamplerId samplerId)
{
    if (ecs_) {
        if (EntityUtil::IsValid(targetEntity)) {
            bool wrote = false;
            if (auto handle = materialComponentManager_->Write(targetEntity)) {
                auto entity = LoadSampler(ResolveSamplerUri(samplerId));
                if (EntityUtil::IsValid(entity)) {
                    // both scene and texture array will hold a reference to entity
                    handle->textures[index].sampler = ecs_->GetEntityManager().GetReferenceCounted(entity);
                    wrote = true;
                }
            }
            if (wrote) {
                ProcessEvents();
            }
        }
    }
}

void SceneHolder::SetSubmeshRenderSortOrder(CORE_NS::Entity targetEntity, int64_t submeshIndex, uint8_t value)
{
    if (ecs_) {
        if (EntityUtil::IsValid(targetEntity)) {
            bool wrote = false;
            if (auto handle = meshComponentManager_->Write(targetEntity)) {
                if (submeshIndex >= 0 && submeshIndex < handle->submeshes.size()) {
                    handle->submeshes[submeshIndex].renderSortLayerOrder = value;
                    wrote = true;
                }
            }
            if (wrote) {
                ProcessEvents();
            }
        }
    }
}

void SceneHolder::SetSubmeshAABBMin(CORE_NS::Entity targetEntity, int64_t submeshIndex, const BASE_NS::Math::Vec3& vec)
{
    if (ecs_) {
        if (EntityUtil::IsValid(targetEntity)) {
            bool wrote = false;
            if (auto handle = meshComponentManager_->Write(targetEntity)) {
                if (submeshIndex >= 0 && submeshIndex < handle->submeshes.size()) {
                    handle->submeshes[submeshIndex].aabbMin = vec;
                    handle->aabbMin = min(handle->aabbMin, vec);
                    wrote = true;
                }
            }
            if (wrote) {
                ProcessEvents();
            }
        }
    }
}

void SceneHolder::RemoveSubmesh(CORE_NS::Entity targetEntity, int64_t submeshIndex)
{
    if (ecs_) {
        if (EntityUtil::IsValid(targetEntity)) {
            bool wrote = false;
            if (auto handle = meshComponentManager_->Write(targetEntity)) {
                if (submeshIndex < 0) {
                    handle->submeshes.clear();
                    handle->aabbMin = { 0.f, 0.f, 0.f };
                    handle->aabbMax = { 0.f, 0.f, 0.f };
                    wrote = true;
                } else if (submeshIndex < handle->submeshes.size()) {
                    handle->submeshes.erase(handle->submeshes.begin() + submeshIndex);
                    for (const auto& submesh : handle->submeshes) {
                        handle->aabbMin = BASE_NS::Math::min(handle->aabbMin, submesh.aabbMin);
                        handle->aabbMax = BASE_NS::Math::max(handle->aabbMax, submesh.aabbMax);
                    }
                    wrote = true;
                }
            }
            if (wrote) {
                ProcessEvents();
            }
        }
    }
}

void SceneHolder::SetSubmeshAABBMax(CORE_NS::Entity targetEntity, int64_t submeshIndex, const BASE_NS::Math::Vec3& vec)
{
    if (ecs_) {
        if (EntityUtil::IsValid(targetEntity)) {
            bool wrote = false;
            if (auto handle = meshComponentManager_->Write(targetEntity)) {
                if (submeshIndex >= 0 && submeshIndex < handle->submeshes.size()) {
                    handle->submeshes[submeshIndex].aabbMax = vec;
                    handle->aabbMax = max(handle->aabbMax, vec);
                    wrote = true;
                }
            }
            if (wrote) {
                ProcessEvents();
            }
        }
    }
}

void SceneHolder::EnableEnvironmentComponent(CORE_NS::Entity entity)
{
    if (ecs_) {
        if (EntityUtil::IsValid(entity)) {
            if (!envComponentManager_->HasComponent(entity)) {
                envComponentManager_->Create(entity);
            }
        }
    }
}

void SceneHolder::EnableLayerComponent(CORE_NS::Entity entity)
{
    if (ecs_) {
        if (EntityUtil::IsValid(entity)) {
            if (!layerComponentManager_->HasComponent(entity)) {
                layerComponentManager_->Create(entity);
            }
        }
    }
}

void SceneHolder::EnableLightComponent(CORE_NS::Entity entity)
{
    if (ecs_) {
        if (EntityUtil::IsValid(entity)) {
            if (!lightComponentManager_->HasComponent(entity)) {
                lightComponentManager_->Create(entity);
            }
        }
    }
}

template<typename T>
constexpr inline CORE3D_NS::IMeshBuilder::DataBuffer FillData(array_view<const T> c) noexcept
{
    Format format = BASE_FORMAT_UNDEFINED;
    if constexpr (is_same_v<T, Math::Vec2>) {
        format = BASE_FORMAT_R32G32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec3>) {
        format = BASE_FORMAT_R32G32B32_SFLOAT;
    } else if constexpr (is_same_v<T, Math::Vec4>) {
        format = BASE_FORMAT_R32G32B32A32_SFLOAT;
    } else if constexpr (is_same_v<T, uint16_t>) {
        format = BASE_FORMAT_R16_UINT;
    } else if constexpr (is_same_v<T, uint32_t>) {
        format = BASE_FORMAT_R32_UINT;
    }
    return CORE3D_NS::IMeshBuilder::DataBuffer { format, sizeof(T),
        { reinterpret_cast<const uint8_t*>(c.data()), c.size() * sizeof(T) } };
}

template<typename IndicesType>
CORE_NS::Entity SceneHolder::CreateMeshFromArrays(const BASE_NS::string& name,
    SCENE_NS::MeshGeometryArrayPtr<IndicesType> arrays, RENDER_NS::IndexType indexType, Entity existingEntity,
    bool append)
{
    if (!ecs_) {
        CORE_LOG_W("%s: no ecs, cannot create mesh", __func__);
        return {};
    }

    if (!arrays || arrays->size() == 0) {
        if (EntityUtil::IsValid(existingEntity)) {
            RemoveSubmesh(existingEntity, -1);
            return existingEntity;
        } else {
            // create empty, submeshes may follow
            auto entity = ecs_->GetEntityManager().Create();

            RenameEntity(entity, name);

            meshComponentManager_->Create(entity);
            scene_->AddEntityToSubcollection("created_meshes", name, entity);
            return entity;
        }
    }

    if (EntityUtil::IsValid(existingEntity) && !append) {
        RemoveSubmesh(existingEntity, -1);
    }

    size_t subMeshIndex = 0;
    auto meshBuilder = CORE_NS::CreateInstance<CORE3D_NS::IMeshBuilder>(*renderContext_, CORE3D_NS::UID_MESH_BUILDER);

    RENDER_NS::IShaderManager& shaderManager = renderContext_->GetDevice().GetShaderManager();
    const RENDER_NS::VertexInputDeclarationView vertexInputDeclaration =
        shaderManager.GetVertexInputDeclarationView(shaderManager.GetVertexInputDeclarationHandle(
            CORE3D_NS::DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD));

    meshBuilder->Initialize(vertexInputDeclaration, 1);

    for (auto&& geometry : *arrays.get()) {
        CORE3D_NS::IMeshBuilder::Submesh submesh;

        submesh.indexType = indexType;
        submesh.vertexCount = static_cast<uint32_t>(geometry->vertices.size());
        submesh.indexCount = static_cast<uint32_t>(geometry->indices.size());
        submesh.tangents = geometry->generateTangents;

        meshBuilder->AddSubmesh(submesh);
    }
    meshBuilder->Allocate();

    for (auto&& geometry : *arrays.get()) {
        meshBuilder->SetVertexData(subMeshIndex, FillData<BASE_NS::Math::Vec3>(geometry->vertices),
            FillData<BASE_NS::Math::Vec3>(geometry->normals), FillData<BASE_NS::Math::Vec2>(geometry->uvs),
            FillData<BASE_NS::Math::Vec2>(geometry->uv2s), FillData<BASE_NS::Math::Vec4>(geometry->tangents),
            FillData<BASE_NS::Math::Vec4>(geometry->colors));
        meshBuilder->CalculateAABB(subMeshIndex, FillData<BASE_NS::Math::Vec3>(geometry->vertices));
        meshBuilder->SetIndexData(subMeshIndex, FillData<IndicesType>(geometry->indices));
    }

    auto entity = meshBuilder->CreateMesh(*ecs_);

    if (EntityUtil::IsValid(existingEntity)) {
        for (size_t ii = 0; ii < arrays->size(); ii++) {
            CopySubMesh(existingEntity, entity, ii);
        }
        ecs_->GetEntityManager().Destroy(entity);
        return existingEntity;
    }

    RenameEntity(entity, name);
    scene_->AddEntityToSubcollection("created_meshes", name, entity);

    ProcessEvents();
    return entity;
}

// we rely that someone else will ret up correct flags to components
void SceneHolder::SetMultiviewCamera(CORE_NS::Entity target, CORE_NS::Entity source)
{
    if (!ecs_ || !EntityUtil::IsValid(target) || !EntityUtil::IsValid(source) ||
        !cameraComponentManager_->HasComponent(source) || !cameraComponentManager_->HasComponent(target)) {
        CORE_LOG_W("%s: camera component not valid", __func__);
        return;
    }
    if (auto handle = cameraComponentManager_->Write(target)) {
        for (auto& ref : handle->multiViewCameras) {
            if (ref.id == target.id) {
                return;
            }
        }
        handle->multiViewCameras.push_back(source);
    }
}

// we rely that someone else will ret up correct flags to components
void SceneHolder::RemoveMultiviewCamera(CORE_NS::Entity target, CORE_NS::Entity source)
{
    if (!ecs_ || !EntityUtil::IsValid(target) || !EntityUtil::IsValid(source) ||
        !cameraComponentManager_->HasComponent(source) || !cameraComponentManager_->HasComponent(target)) {
        CORE_LOG_W("%s: camera component not valid", __func__);
        return;
    }
    if (auto handle = cameraComponentManager_->Write(target)) {
        for (size_t ii = 0; ii < handle->multiViewCameras.size(); ii++) {
            if (handle->multiViewCameras[ii] == target) {
                handle->multiViewCameras.erase(handle->multiViewCameras.cbegin() + ii);
                return;
            }
        }
    }
}

void SceneHolder::CopySubMesh(CORE_NS::Entity target, CORE_NS::Entity source, size_t index)
{
    if (!ecs_ || !EntityUtil::IsValid(target) || !EntityUtil::IsValid(source) ||
        !meshComponentManager_->HasComponent(source)) {
        CORE_LOG_W("%s: cannot copy submesh", __func__);
        return;
    }

    bool wrote = false;
    if (auto sourceComponent = meshComponentManager_->Read(source)) {
        if (sourceComponent->submeshes.size() > index) {
            if (!meshComponentManager_->HasComponent(target)) {
                meshComponentManager_->Create(target);
            }
            if (auto targetComponent = meshComponentManager_->Write(target)) {
                targetComponent->submeshes.push_back(CORE3D_NS::MeshComponent::Submesh());
                // Todo: verify if copy by assignment is enough, or do we need deep copy
                // and explicit reference modifications
                targetComponent->submeshes.back() = sourceComponent->submeshes.at(index);
                for (const auto& submesh : targetComponent->submeshes) {
                    targetComponent->aabbMin = BASE_NS::Math::min(targetComponent->aabbMin, submesh.aabbMin);
                    targetComponent->aabbMax = BASE_NS::Math::max(targetComponent->aabbMax, submesh.aabbMax);
                }
                wrote = true;
            }
        }
    }
    if (wrote) {
        ProcessEvents();
    }
}

void SceneHolder::ReleaseOwnership(CORE_NS::Entity entity)
{
    if (ecs_ && scene_) {
        if (EntityUtil::IsValid(entity)) {
            if (const auto cachedEntity = FindCachedRelatedEntity(entity);
                EntityUtil::IsValid(cachedEntity) && entity != cachedEntity) {
                SCENE_PLUGIN_VERBOSE_LOG("%s: Cached entity does not match: entity: %I64u, cachedEntity: %I64u",
                    __func__, entity.id, cachedEntity.id);
                if (auto cachedIdx = scene_->GetSubCollectionIndexByRoot(cachedEntity); cachedIdx != -1) {
                    scene_->GetSubCollection(cachedIdx)->SetActive(false);
                    scene_->RemoveEntityRecursive(cachedEntity);
                }
            }
            scene_->RemoveEntityRecursive(entity);
        }
    }
}

// Reposition a node within its parent
void SceneHolder::ReindexEntity(CORE_NS::Entity target, size_t index)
{
    if (ecs_ && scene_ && EntityUtil::IsValid(target)) {
        // Get the node
        if (auto node = nodeSystem_->GetNode(target)) {
            // Get the parent
            if (auto parent = node->GetParent()) {
                parent->RemoveChild(*node);
                parent->InsertChild(index, *node);
            }
        }

        // do we need to adjust also collection manually (it should be reference only)
        scene_->MarkModified(true, true);
    }
}

template CORE_NS::Entity SceneHolder::CreateMeshFromArrays<uint16_t>(const BASE_NS::string& name,
    SCENE_NS::MeshGeometryArrayPtr<uint16_t> arrays, RENDER_NS::IndexType indexType, Entity existingEntity,
    bool append);

template CORE_NS::Entity SceneHolder::CreateMeshFromArrays<uint32_t>(const BASE_NS::string& name,
    SCENE_NS::MeshGeometryArrayPtr<uint32_t> arrays, RENDER_NS::IndexType indexType, Entity existingEntity,
    bool append);

string SceneHolder::GetUniqueName()
{
    return instanceId_.ToString().append(to_string(++instanceNumber_));
}

// Enable new multi-mesh batch
CORE_NS::Entity SceneHolder::CreateMultiMeshInstance(CORE_NS::Entity baseComponent)
{
    if (ecs_) {
        if (!nodeSystem_->GetNode(baseComponent)) {
            CORE_LOG_I("%s: selected base entity does not have node on ecs", __func__);

            // Now, the assumption is that entity should have either RenderMeshComponent or MeshComponent
            // It is bit open if it has render mesh component, shoud we copy its mesh data implicitly
            CORE3D_NS::RenderMeshComponent renderMeshData;

            if (auto data = renderMeshComponentManager_->Read(baseComponent)) {
                renderMeshData = *data;
            }

            auto node = nodeSystem_->CreateNode();
            auto nodeEntity = node->GetEntity();

            renderMeshComponentManager_->Create(nodeEntity);
            if (auto data = renderMeshComponentManager_->Write(nodeEntity)) {
                *data = renderMeshData;

                if (!EntityUtil::IsValid(data->mesh)) {
                    // base component may have mesh component, or then it does not
                    data->mesh = baseComponent;
                }
            }
            baseComponent = nodeEntity;
        }

        if (!renderMeshComponentManager_->HasComponent(baseComponent)) {
            renderMeshComponentManager_->Create(baseComponent);
        }
    }

    return baseComponent;
}

static constexpr size_t MULTI_MESH_CHILD_PREFIX_LEN = MULTI_MESH_CHILD_PREFIX.size();

bool SceneHolder::IsMultiMeshChild(const CORE3D_NS::ISceneNode* child)
{
    auto entity = child->GetEntity();
    if (auto nameHandle = nameComponentManager_->Read(entity)) {
        auto name = BASE_NS::string_view(nameHandle->name.data(), nameHandle->name.size());
        if (name.compare(0, MULTI_MESH_CHILD_PREFIX_LEN, MULTI_MESH_CHILD_PREFIX) == 0) {
            return true;
        }
    }
    return false;
}

CORE_NS::Entity SceneHolder::FindCachedRelatedEntity(const CORE_NS::Entity& entity)
{
    if (!(scene_ && nodeSystem_ && CORE_NS::EntityUtil::IsValid(entity))) {
        return {};
    }

    CORE_NS::Entity cachedEntity {};
    if (const auto node = nodeSystem_->GetNode(entity)) {
        if (const auto idx = scene_->GetSubCollectionIndex(node->GetName()); idx != -1) {
            cachedEntity = scene_->GetSubCollection(idx)->GetEntity("/");
        }
    }

    return cachedEntity;
}

// Set mesh to multi-mesh
void SceneHolder::SetMeshMultimeshArray(CORE_NS::Entity target, CORE_NS::Entity mesh)
{
    if (ecs_) {
        if (auto data = renderMeshComponentManager_->Write(target)) {
            data->mesh = mesh;
        }

        if (const CORE3D_NS::ISceneNode* parent = nodeSystem_->GetNode(target)) {
            MeshComponent givenData;
            if (auto givenMeshData = meshComponentManager_->Read(mesh)) {
                givenData = *givenMeshData;
            }

            for (auto& child : parent->GetChildren()) {
                auto entity = child->GetEntity();
                if (!IsMultiMeshChild(child)) {
                    continue;
                }

                if (auto data = renderMeshComponentManager_->Read(entity)) {
                    auto mesh = data->mesh;
                    if (auto meshData = meshComponentManager_->Write(mesh)) {
                        *meshData = givenData;
                    }
                }

                if (auto data = renderMeshComponentManager_->Write(entity)) {
                    data->renderMeshBatch = target;
                }
            }
        }
    }
}

void SetAll(CORE_NS::Entity target, CORE_NS::Entity material, BASE_NS::vector<CORE_NS::Entity>& ret,
    CORE3D_NS::IMeshComponentManager* meshComponentManager)
{
    if (EntityUtil::IsValid(target)) {
        if (auto handle = meshComponentManager->Write(target)) {
            for (auto&& submesh : handle->submeshes) {
                ret.push_back(submesh.material);
                submesh.material = material;
            }
        }
    }
}

void ResetAll(CORE_NS::Entity target, BASE_NS::vector<CORE_NS::Entity>& in,
    CORE3D_NS::IMeshComponentManager* meshComponentManager)
{
    if (EntityUtil::IsValid(target)) {
        if (auto handle = meshComponentManager->Write(target)) {
            for (auto&& submesh : handle->submeshes) {
                if (in.size() > 0) {
                    submesh.material = in.front();
                    in.erase(in.begin());
                }
            }
        }
    }
}

// Set override material to multi-mesh
BASE_NS::vector<CORE_NS::Entity> SceneHolder::SetOverrideMaterialMultimeshArray(
    CORE_NS::Entity target, CORE_NS::Entity material)
{
    BASE_NS::vector<CORE_NS::Entity> ret;
    if (ecs_) {
        // if we contain render mesh handle, use it to set material
        if (auto data = renderMeshComponentManager_->Read(target)) {
            auto mesh = data->mesh;
            SetAll(mesh, material, ret, meshComponentManager_);
        }

        // if our child contain meshes, update material on those
        if (const CORE3D_NS::ISceneNode* parent = nodeSystem_->GetNode(target)) {
            for (auto& child : parent->GetChildren()) {
                if (IsMultiMeshChild(child)) {
                    auto entity = child->GetEntity();
                    if (auto data = renderMeshComponentManager_->Read(entity)) {
                        auto mesh = data->mesh;
                        SetAll(mesh, material, ret, meshComponentManager_);
                    }
                }
            }
        }
    }
    return ret;
}

// reset override material from multi-mesh
void SceneHolder::ResetOverrideMaterialMultimeshArray(CORE_NS::Entity target, BASE_NS::vector<CORE_NS::Entity>& in)
{
    if (ecs_) {
        BASE_NS::vector<CORE_NS::Entity> ret;

        // if we contain render mesh handle, use it to set material
        if (auto data = renderMeshComponentManager_->Read(target)) {
            auto mesh = data->mesh;
            ResetAll(mesh, in, meshComponentManager_);
        }

        // if our child contain meshes, update material on those
        if (const CORE3D_NS::ISceneNode* parent = nodeSystem_->GetNode(target)) {
            for (auto& child : parent->GetChildren()) {
                if (IsMultiMeshChild(child)) {
                    auto entity = child->GetEntity();
                    if (auto data = renderMeshComponentManager_->Read(entity)) {
                        auto mesh = data->mesh;
                        ResetAll(mesh, in, meshComponentManager_);
                    }
                }
            }
        }
    }
    in.clear();
}

// Set instance count to multi-mesh
void SceneHolder::SetInstanceCountMultimeshArray(CORE_NS::Entity target, size_t count)
{
    if (ecs_) {
        if (auto parent = nodeSystem_->GetNode(target)) {
            // this is kind of sub-optimization. If we have instances available, the mesh component is stored only
            // in instances
            CORE_NS::Entity firstBorn;
            auto children = parent->GetChildren();

            for (auto& child : children) {
                if (IsMultiMeshChild(child)) {
                    firstBorn = child->GetEntity();
                    break;
                }
            }

            CORE3D_NS::MeshComponent meshData;

            // when the instance count goes back to zero, we need to restore the data on parent
            if (count == 0) {
                if (CORE_NS::EntityUtil::IsValid(firstBorn)) {
                    renderMeshComponentManager_->Create(target);
                    if (auto renderMeshData = renderMeshComponentManager_->Write(target)) {
                        if (auto instancingData = renderMeshComponentManager_->Read(firstBorn)) {
                            *renderMeshData = *instancingData;
                        }
                    }
                }
                // now, when we move the templated item back to root, it will become visible by default
            } else {
                // if we have mesh, set things up using mesh
                if (auto data = meshComponentManager_->Read(target)) {
                    meshData = *data;
                } else if (auto instanceData = renderMeshComponentManager_->Read(firstBorn)) {
                    // otherwise we prefer the instanced data
                    if (auto data = meshComponentManager_->Read(instanceData->mesh)) {
                        // use instance data
                        meshData = *data;
                        // and switch target
                        target = firstBorn;
                    }
                } else if (auto renderMeshData = renderMeshComponentManager_->Read(target)) {
                    // and fallback to existing render mesh data if it exists
                    if (auto data = meshComponentManager_->Read(renderMeshData->mesh)) {
                        meshData = *data;
                    }
                }
            }

            size_t mmcount = 0;
            for (auto& child : children) {
                if (IsMultiMeshChild(child)) {
                    if (mmcount == count) {
                        parent->RemoveChild(*child);
                    } else {
                        mmcount++;
                    }
                }
            }

            BASE_NS::vector<CORE_NS::Entity> clones;
            while (mmcount < count) {
                clones.push_back(ecs_->CloneEntity(target));
                mmcount++;
            }

            for (auto clone : clones) {
                // set up node system
                auto node = nodeSystem_->GetNode(clone);
                node->SetParent(*parent);
                node->SetEnabled(true);
                nameComponentManager_->Create(clone);

                // set up name so we can identify cloned instances afterwards
                if (auto nameHandle = nameComponentManager_->Write(clone)) {
                    BASE_NS::string postFixed(MULTI_MESH_CHILD_PREFIX.data(), MULTI_MESH_CHILD_PREFIX.size());
                    postFixed.append(BASE_NS::to_string(mmcount));
                    nameHandle->name = postFixed;
                }

                // and finally mesh specifics
                if (auto rmcHandle = renderMeshComponentManager_->Write(clone)) {
                    // during the first clone, switch from template to first instance
                    if (mmcount == 0) {
                        renderMeshComponentManager_->Destroy(target);
                        target = clone;
                    }
                    // Set batch
                    rmcHandle->renderMeshBatch = target;
                    // create new entity for mesh
                    const CORE_NS::Entity entity = ecs_->GetEntityManager().Create();
                    meshComponentManager_->Create(entity);
                    // copy the data of the original
                    *meshComponentManager_->Write(entity) = meshData;
                    // and set the mesh to a new instance
                    rmcHandle->mesh = entity;
                }
                mmcount++;
            }
            // Set up a batch
            auto renderMeshBatchComponentManager =
                CORE_NS::GetManager<CORE3D_NS::IRenderMeshBatchComponentManager>(*ecs_);
            if (!renderMeshBatchComponentManager->HasComponent(firstBorn)) {
                renderMeshBatchComponentManager->Create(firstBorn);
            }
            if (auto batchHandle = renderMeshBatchComponentManager->Write(firstBorn)) {
                batchHandle->batchType = CORE3D_NS::RenderMeshBatchComponent::BatchType::GPU_INSTANCING;
            }
        }
    }
}

// Set visible count to multi-mesh
void SceneHolder::SetVisibleCountMultimeshArray(CORE_NS::Entity target, size_t count)
{
    if (ecs_) {
        if (const CORE3D_NS::ISceneNode* parent = nodeSystem_->GetNode(target)) {
            size_t ix = 0;
            for (auto& child : parent->GetChildren()) {
                if (IsMultiMeshChild(child)) {
                    child->SetEnabled(ix < count);
                    ix++;
                }
            }
        }
    }
}

// Set custom data to multi-mesh index
void SceneHolder::SetCustomData(CORE_NS::Entity target, size_t index, const BASE_NS::Math::Vec4& data)
{
    if (ecs_) {
        if (const CORE3D_NS::ISceneNode* parent = nodeSystem_->GetNode(target)) {
            size_t ix = 0;
            for (auto& child : parent->GetChildren()) {
                if (IsMultiMeshChild(child)) {
                    if (ix == index) {
                        auto entity = child->GetEntity();
                        if (auto handle = renderMeshComponentManager_->Write(entity)) {
                            *static_cast<float*>(static_cast<void*>(&handle->customData[0].x)) = data.x;
                            *static_cast<float*>(static_cast<void*>(&handle->customData[0].y)) = data.y;
                            *static_cast<float*>(static_cast<void*>(&handle->customData[0].z)) = data.z;
                            *static_cast<float*>(static_cast<void*>(&handle->customData[0].w)) = data.w;
                        }
                        return;
                    }
                    ix++;
                }
            }
        }
    }
}

// Set transformation to multi-mesh index
void SceneHolder::SetTransformation(CORE_NS::Entity target, size_t index, const BASE_NS::Math::Mat4X4& transform)
{
    if (ecs_) {
        if (const CORE3D_NS::ISceneNode* parent = nodeSystem_->GetNode(target)) {
            size_t ix = 0;
            for (auto& child : parent->GetChildren()) {
                if (IsMultiMeshChild(child)) {
                    if (ix == index) {
                        auto entity = child->GetEntity();
                        if (auto handle = transformComponentManager_->Write(entity)) {
                            BASE_NS::Math::Quat rotation;
                            BASE_NS::Math::Vec3 scale;
                            BASE_NS::Math::Vec3 position;
                            BASE_NS::Math::Vec3 skew;
                            BASE_NS::Math::Vec4 persp;
                            if (BASE_NS::Math::Decompose(transform, scale, rotation, position, skew, persp)) {
                                handle->scale = scale;
                                handle->rotation = rotation;
                                handle->position = position;
                            }
                            return;
                        }
                    }
                    ix++;
                }
            }
        }
    }
}

bool SceneHolder::GetWorldMatrixComponentAABB(
    SCENE_NS::IPickingResult::Ptr ret, CORE_NS::Entity entity, bool isRecursive)
{
    if (picking_ && ecs_) {
        auto result = picking_->GetWorldMatrixComponentAABB(entity, isRecursive, *ecs_);
        if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(ret)) {
            writable->Add(result.minAABB);
            writable->Add(result.maxAABB);
            return true;
        }
    }
    return false;
}

bool SceneHolder::GetTransformComponentAABB(SCENE_NS::IPickingResult::Ptr ret, CORE_NS::Entity entity, bool isRecursive)
{
    if (picking_ && ecs_) {
        auto result = picking_->GetTransformComponentAABB(entity, isRecursive, *ecs_);
        if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(ret)) {
            writable->Add(result.minAABB);
            writable->Add(result.maxAABB);
            return true;
        }
    }
    return false;
}

bool SceneHolder::GetWorldAABB(SCENE_NS::IPickingResult::Ptr ret, const BASE_NS::Math::Mat4X4& world,
    const BASE_NS::Math::Vec3& aabbMin, const BASE_NS::Math::Vec3& aabbMax)
{
    if (picking_ && ecs_) {
        auto result = picking_->GetWorldAABB(world, aabbMin, aabbMax);
        if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(ret)) {
            writable->Add(result.minAABB);
            writable->Add(result.maxAABB);
            return true;
        }
    }
    return false;
}

bool ConvertToToolkit(const BASE_NS::vector<CORE3D_NS::RayCastResult>& result, SCENE_NS::IRayCastResult::Ptr ret)
{
    if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<SCENE_NS::NodeDistance>>(ret)) {
        for (auto& value : result) {
            BASE_NS::string path;
            if (ResolveNodeFullPath(value.node, path)) {
                writable->Add({ {}, value.distance });
                writable->MetaData().push_back(path);
            }
        }
        return true;
    }
    return false;
}

bool SceneHolder::RayCast(
    SCENE_NS::IRayCastResult::Ptr ret, const BASE_NS::Math::Vec3& start, const BASE_NS::Math::Vec3& direction)
{
    if (picking_ && ecs_) {
        auto result = picking_->RayCast(*ecs_, start, direction);
        return ConvertToToolkit(result, ret);
    }
    return false;
}

bool SceneHolder::RayCast(SCENE_NS::IRayCastResult::Ptr ret, const BASE_NS::Math::Vec3& start,
    const BASE_NS::Math::Vec3& direction, uint64_t layerMask)
{
    if (picking_ && ecs_) {
        auto result = picking_->RayCast(*ecs_, start, direction, layerMask);
        return ConvertToToolkit(result, ret);
    }
    return false;
}

bool SceneHolder::ScreenToWorld(
    SCENE_NS::IPickingResult::Ptr ret, CORE_NS::Entity camera, BASE_NS::Math::Vec3 screenCoordinate)
{
    if (picking_ && ecs_) {
        auto result = picking_->ScreenToWorld(*ecs_, camera, screenCoordinate);
        if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(ret)) {
            writable->Add(result);
            return true;
        }
    }
    return false;
}

bool SceneHolder::WorldToScreen(
    SCENE_NS::IPickingResult::Ptr ret, CORE_NS::Entity camera, BASE_NS::Math::Vec3 worldCoordinate)

{
    if (picking_ && ecs_) {
        auto result = picking_->WorldToScreen(*ecs_, camera, worldCoordinate);
        if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(ret)) {
            writable->Add(result);
            return true;
        }
    }
    return false;
}

bool SceneHolder::RayCastFromCamera(
    SCENE_NS::IRayCastResult::Ptr ret, CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos)
{
    if (picking_ && ecs_) {
        auto result = picking_->RayCastFromCamera(*ecs_, camera, screenPos);
        return ConvertToToolkit(result, ret);
    }
    return false;
}

bool SceneHolder::RayCastFromCamera(
    SCENE_NS::IRayCastResult::Ptr ret, CORE_NS::Entity camera, const BASE_NS::Math::Vec2& screenPos, uint64_t layerMask)
{
    if (picking_ && ecs_) {
        auto result = picking_->RayCastFromCamera(*ecs_, camera, screenPos, layerMask);
        return ConvertToToolkit(result, ret);
    }
    return false;
}

bool SceneHolder::RayFromCamera(
    SCENE_NS::IPickingResult::Ptr ret, CORE_NS::Entity camera, BASE_NS::Math::Vec2 screenCoordinate)
{
    if (auto cameraComponent = cameraComponentManager_->Read(camera)) {
        if (auto worldMatrixManager = GetManager<CORE3D_NS::IWorldMatrixComponentManager>(*ecs_)) {
            if (auto cameraWorldMatrixComponent = worldMatrixManager->Read(camera)) {
                if (cameraComponent->projection == CORE3D_NS::CameraComponent::Projection::ORTHOGRAPHIC) {
                    const auto worldPos = picking_->ScreenToWorld(
                        *ecs_, camera, BASE_NS::Math::Vec3(screenCoordinate.x, screenCoordinate.y, 0.0f));
                    const auto direction =
                        cameraWorldMatrixComponent->matrix * BASE_NS::Math::Vec4(0.0f, 0.0f, -1.0f, 0.0f);
                    if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(ret)) {
                        writable->Add(worldPos);
                        writable->Add(direction);
                        return true;
                    }
                } else {
                    // Ray origin is the camera world position.
                    const auto rayOrigin = BASE_NS::Math::Vec3(cameraWorldMatrixComponent->matrix.w);
                    const auto targetPos = picking_->ScreenToWorld(
                        *ecs_, camera, BASE_NS::Math::Vec3(screenCoordinate.x, screenCoordinate.y, 1.0f));
                    const auto direction = BASE_NS::Math::Normalize(targetPos - rayOrigin);
                    if (auto writable = interface_cast<SCENE_NS::IPendingRequestData<BASE_NS::Math::Vec3>>(ret)) {
                        writable->Add(rayOrigin);
                        writable->Add(direction);
                        return true;
                    }
                }
            }
        }
    }
    return false;
}
