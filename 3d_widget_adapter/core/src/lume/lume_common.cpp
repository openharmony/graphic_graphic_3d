/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "lume_common.h"

#include <dlfcn.h>
#include <string_view>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES3/gl31.h>

#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_batch_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_scene_util.h>

#include <base/containers/array_view.h>

#include <core/ecs/intf_system_graph_loader.h>
#include <core/implementation_uids.h>
#include <core/io/intf_file_manager.h>
#include <core/namespace.h>
#include <core/os/intf_platform.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property/intf_property_handle.h>

#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#if CORE_HAS_GLES_BACKEND || CORE_HAS_GL_BACKEND
#include <render/gles/intf_device_gles.h>
#endif
#include <render/implementation_uids.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#if CORE_HAS_VULKAN_BACKEND
#include <render/vulkan/intf_device_vk.h>
#endif

#include "3d_widget_adapter_log.h"
#include "graphics_manager.h"
#include "widget_trace.h"

#include "include/gpu/vk/GrVkExtensions.h"
#include "vulkan/vulkan_core.h"
#include "vulkan/vulkan_ohos.h"

#include <parameter.h>
#include <parameters.h>
#include "param/sys_param.h"

#if defined(CORE_DYNAMIC) && (CORE_DYNAMIC == 1)

CORE_BEGIN_NAMESPACE()
/** Get plugin register */
IPluginRegister& (*GetPluginRegister)() = nullptr;

/** Setup the plugin registry */
void (*CreatePluginRegistry)(
    const struct PlatformCreateInfo& platformCreateInfo) = nullptr;

/** Get whether engine is build in debug mode */
bool (*IsDebugBuild)() = nullptr;

/** Get version */
BASE_NS::string_view (*GetVersion)() = nullptr;
CORE_END_NAMESPACE()
#endif // CORE_DYNAMIC

namespace OHOS::Render3D {
LumeCommon::~LumeCommon()
{
    // explicit release resource before destructor
}

void LumeCommon::UnloadEngineLib()
{
    DeInitEngine();
    if (libHandle_ == nullptr) {
        return;
    }
    dlclose(libHandle_);
    libHandle_ = nullptr;

    CORE_NS::GetPluginRegister = nullptr;
    CORE_NS::CreatePluginRegistry = nullptr;
    CORE_NS::IsDebugBuild = nullptr;
    CORE_NS::GetVersion = nullptr;
}

template<typename T>
bool LoadFunc(T &fn, const char *fName, void* handle)
{
    fn = reinterpret_cast<T>(dlsym(handle, fName));
    if (fn == nullptr) {
        WIDGET_LOGE("%s open %s", __func__, dlerror());
        return false;
    }
    return true;
}

bool LumeCommon::LoadEngineLib()
{
    if (libHandle_ != nullptr) {
        WIDGET_LOGE("%s, already loaded", __func__);
        return false;
    }

    #define TO_STRING(name) #name
    #define LIB_NAME(name) TO_STRING(name)
    constexpr std::string_view lib { LIB_NAME(LIB_ENGINE_CORE)".so" };
    libHandle_ = dlopen(lib.data(), RTLD_LAZY);

    if (libHandle_ == nullptr) {
        WIDGET_LOGE("%s, open lib fail %s", __func__, dlerror());
    }
    #undef TO_STRING
    #undef LIB_NAME

    #define LOAD_FUNC(fn, name) LoadFunc<decltype(fn)>(fn, name, libHandle_)
    if (!(LOAD_FUNC(CORE_NS::CreatePluginRegistry,
        "_ZN4Core20CreatePluginRegistryERKNS_18PlatformCreateInfoE")
        && LOAD_FUNC(CORE_NS::GetPluginRegister, "_ZN4Core17GetPluginRegisterEv")
        && LOAD_FUNC(CORE_NS::IsDebugBuild, "_ZN4Core12IsDebugBuildEv")
        && LOAD_FUNC(CORE_NS::GetVersion, "_ZN4Core13GetVersionRevEv"))) {
        return false;
    }
    #undef LOAD_FUNC

    return true;
}

void LumeCommon::Clone(IEngine* proto)
{
    auto p = static_cast<LumeCommon *>(proto);
    engine_ = p->GetCoreEngine();
    renderContext_ = p->GetRenderContext();
    graphicsContext_ = p->GetGraphicsContext();
    device_ = p->GetDevice();
    activateWeatherPhys_ = p->activateWeatherPhys_;
}

bool LumeCommon::InitEngine(EGLContext gfxContext, const PlatformData& data)
{
    return CreateCoreEngine(ToEnginePlatformData(data)) && CreateRenderContext(gfxContext) && CreateGfx3DContext()
        && GetDevice();
}

CORE_NS::IEngine::Ptr LumeCommon::CreateCoreEngine(const Core::PlatformCreateInfo &info)
{
    CORE_NS::CreatePluginRegistry(info);
    auto factory = CORE_NS::GetInstance<Core::IEngineFactory>(Core::UID_ENGINE_FACTORY);

    const Core::EngineCreateInfo engineCreateInfo { info,
        {
            "gltf_viewer",  // name
            0,  // versionMajor
            0,  // versionMinor
            0,  // versionPatch
        },
    {}};
    engine_ = factory->Create(engineCreateInfo);

    if (engine_ == nullptr) {
        WIDGET_LOGE("3D engine create fail");
        return nullptr;
    }

    RegisterAssertPath();

    engine_->Init();
    return engine_;
}

void LumeCommon::OnWindowChange(const TextureInfo& textureInfo)
{
    textureInfo_ = textureInfo;
    SetupCustomRenderTarget(textureInfo);
    float widthScale = textureInfo.widthScale_;
    float heightScale = textureInfo.heightScale_;
    SetupCameraViewPort(textureInfo.width_ * widthScale, textureInfo.height_ * heightScale);

    if (customRender_) { // this moment customRender may not ready
        customRender_->OnSizeChange(textureInfo.width_ * widthScale, textureInfo.height_ * heightScale);
        customRender_->SetScaleInfo(widthScale, heightScale);
    }
}

RENDER_NS::IRenderContext::Ptr LumeCommon::CreateRenderContext(EGLContext gfxContext)
{
    // Create render context
    constexpr BASE_NS::Uid uidRender[] = { RENDER_NS::UID_RENDER_PLUGIN };
    CORE_NS::GetPluginRegister().LoadPlugins(uidRender);

    renderContext_ = CORE_NS::CreateInstance<RENDER_NS::IRenderContext>(
        *engine_->GetInterface<Core::IClassFactory>(),
        RENDER_NS::UID_RENDER_CONTEXT);

    if (renderContext_ == nullptr) {
        WIDGET_LOGE("lume Create render context fail");
        return nullptr;
    }

    Render::DeviceCreateInfo deviceCreateInfo;
    std::string backendProp = system::GetParameter("AGP_BACKEMD_CONFIG", "gles");
    RENDER_NS::BackendExtraVk vkExtra;
    RENDER_NS::BackendExtraGLES glesExtra;
    if (backendProp == "vulkan") {
        vkExtra.extensions.extensionNames.push_back(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
        vkExtra.extensions.extensionNames.push_back(VK_KHR_MAINTENANCE1_EXTENSION_NAME);
        vkExtra.extensions.extensionNames.push_back(VK_KHR_MAINTENANCE2_EXTENSION_NAME);
        vkExtra.extensions.extensionNames.push_back(VK_KHR_MAINTENANCE3_EXTENSION_NAME);
        vkExtra.extensions.extensionNames.push_back(VK_KHR_BIND_MEMORY_2_EXTENSION_NAME);
        deviceCreateInfo.backendType = RENDER_NS::DeviceBackendType::VULKAN;
        deviceCreateInfo.backendConfiguration = &vkExtra;
    } else {
        glesExtra.applicationContext = EGL_NO_CONTEXT;
        glesExtra.sharedContext = gfxContext;
        glesExtra.MSAASamples = 0;
        glesExtra.depthBits = 0; // 24 bits of depth buffer.
        deviceCreateInfo.backendType = Render::DeviceBackendType::OPENGLES;
        deviceCreateInfo.backendConfiguration = &glesExtra;
    }
    WIDGET_LOGD("config backend %s", backendProp.c_str());
    const RENDER_NS::RenderCreateInfo renderCreateInfo {
        {
            "core_gltf_viewer", // name
            1,
            0,
            0,
        },
        deviceCreateInfo,
    };

    auto ret =  renderContext_->Init(renderCreateInfo);
    if (ret != RENDER_NS::RenderResultCode::RENDER_SUCCESS) {
        WIDGET_LOGE("lume Init render context fail");
        return nullptr;
    }
    return renderContext_;
}

CORE3D_NS::IGraphicsContext::Ptr LumeCommon::CreateGfx3DContext()
{
    // Create an engine bound graphics context instance.
    constexpr BASE_NS::Uid uid3D[] = { CORE3D_NS::UID_3D_PLUGIN };
    CORE_NS::GetPluginRegister().LoadPlugins(uid3D);

    graphicsContext_ = CORE_NS::CreateInstance<CORE3D_NS::IGraphicsContext>(
        *renderContext_->GetInterface<Core::IClassFactory>(),
        CORE3D_NS::UID_GRAPHICS_CONTEXT);


    if (graphicsContext_ == nullptr) {
        WIDGET_LOGE("lume Create Graphic context fail");
        return nullptr;
    }

    graphicsContext_->Init();
    return graphicsContext_;
}

CORE_NS::IEngine::Ptr LumeCommon::GetCoreEngine()
{
    return engine_;
}

RENDER_NS::IRenderContext::Ptr LumeCommon::GetRenderContext()
{
    return renderContext_;
}

CORE3D_NS::IGraphicsContext::Ptr LumeCommon::GetGraphicsContext()
{
    return graphicsContext_;
}

RENDER_NS::IDevice* LumeCommon::GetDevice()
{
    if (renderContext_ == nullptr) {
        WIDGET_LOGE("Get device but render context is empty");
        return nullptr;
    }

    device_ = &(renderContext_->GetDevice());

    if (device_ == nullptr) {
        WIDGET_LOGE("Get device fail");
    }

    return device_;
}

void LumeCommon::DeInitEngine()
{
    DestroySwapchain();
    DestroyResource();
    graphicsContext_ = nullptr;
    renderContext_ = nullptr;
    engine_ = nullptr;
    ecs_ = nullptr;
    device_ = nullptr;
}

void LumeCommon::UpdateGeometries(const std::vector<std::shared_ptr<Geometry>>& shapes)
{
    for (auto shape = shapes_.begin(); shape != shapes_.end();) {
        auto find = std::find_if(shapes.begin(), shapes.end(), [&shape](const std::shared_ptr<Geometry>& sNew) {
            return (*shape)->GetName() == sNew->GetName();
        });
        if (find == shapes.end()) {
            shape = shapes_.erase(shape);
            continue;
        }
        shape++;
    }
    LoadCustGeometry(shapes);
}

void LumeCommon::UpdateCustomRender(const std::shared_ptr<CustomRenderDescriptor>& customRenderDescriptor)
{
    if (customRenderDescriptor) {
        auto needsFrameCallback = customRenderDescriptor->NeedsFrameCallback();
        if (!customRender_) {
            customRender_ = std::make_shared<LumeCustomRender>(needsFrameCallback);
        }
        customRender_->Initialize({ GetCoreEngine(), GetGraphicsContext(), GetRenderContext(), ecs_,
            textureInfo_.width_ * textureInfo_.widthScale_, textureInfo_.height_ * textureInfo_.heightScale_});
        customRender_->LoadRenderNodeGraph(customRenderDescriptor->GetUri(), gpuResourceImgHandle_);
        if (needsFrameCallback) {
            needsFrameCallback_ = needsFrameCallback;
        }
    }
}

void LumeCommon::UpdateShaderPath(const std::string& shaderPath)
{
    if (customRender_) {
        customRender_->RegistorShaderPath(shaderPath);
    }
}

void LumeCommon::UpdateImageTexturePaths(const std::vector<std::string>& imageTextures)
{
    if (customRender_) {
        customRender_->LoadImages(imageTextures);
    }
}

void LumeCommon::UpdateShaderInputBuffer(const std::shared_ptr<OHOS::Render3D::ShaderInputBuffer>& shaderInputBuffer)
{
    if (customRender_ && shaderInputBuffer) {
        customRender_->UpdateShaderInputBuffer(shaderInputBuffer);
    }
}

void LumeCommon::DestroySceneNodeAndRes(CORE_NS::Entity& importedEntity,
    BASE_NS::vector<CORE3D_NS::GLTFResourceData>& res)
{
    if (!CORE_NS::EntityUtil::IsValid(importedEntity)) {
        return;
    }

    CORE3D_NS::INodeSystem& nodeSystem = *CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*ecs_);
    CORE3D_NS::ISceneNode* sceneRoot = nodeSystem.GetNode(importedEntity);
    if (sceneRoot) {
        nodeSystem.DestroyNode(*sceneRoot);
    }

    importedEntity = {};
    res.clear();
}

void LumeCommon::UnloadSceneModel()
{
    // clean scene nodes
    auto animationSystem = CORE_NS::GetSystem<CORE3D_NS::IAnimationSystem>(*ecs_);
    for (auto animation : animations_) {
        animationSystem->DestroyPlayback(animation);
    }
    animations_.clear();
    DestroySceneNodeAndRes(importedSceneEntity_, importedSceneResources_);
}

void LumeCommon::UnloadEnvModel()
{
    WIDGET_LOGD("Unload enviroment model");
    CORE_NS::ScopedHandle<CORE3D_NS::RenderConfigurationComponent> sceneComponent =
        sceneManager_->Write(sceneEntity_);
    if (sceneComponent) {
        auto envManager = CORE_NS::GetManager<CORE3D_NS::IEnvironmentComponentManager>(*ecs_);
        envManager->Destroy(sceneComponent->environment);
    }

    // clean scene nodes
    DestroySceneNodeAndRes(importedEnvEntity_, importedEnvResources_);
}

void LumeCommon::DestroyResource()
{
    WIDGET_SCOPED_TRACE("LumeCommon::UnloadModel");
    if (!ecs_) {
        return;
    }
    auto& ecs = *ecs_;
    UnloadSceneModel();
    UnloadEnvModel();

    // run garbage collection
    ecs.ProcessEvents();
#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    if (ecs_) {
        OHOS::Render3D::GraphicsManager::GetInstance().UnloadEcs(reinterpret_cast<void *>(ecs_.get()));
        // gltf src update case crashes when MULTI_ECS_UPDATE_AT_ONCE is ON.
    }
#endif
    renderHandles_.clear();
}

bool LumeCommon::LoadAndImport(const GltfImportInfo& info, CORE_NS::Entity& importedEntity,
    BASE_NS::vector<CORE3D_NS::GLTFResourceData>& res)
{
    WIDGET_SCOPED_TRACE_ARGS("LoadAndImport %s", info.fileName_);
    auto& ecs = *ecs_;
    auto gltf = graphicsContext_->GetGltf().LoadGLTF(info.fileName_);
    if (!gltf.success) {
        WIDGET_LOGE("LoadAndImport() Loaded '%s' with errors:\n%s",
            info.fileName_, gltf.error.c_str());
        return false;
    }
    if (!gltf.data) {
        WIDGET_LOGE("LoadAndImport gltf data is null. Error: %s ", gltf.error.c_str());
        return false;
    }

    auto importer = graphicsContext_->GetGltf().CreateGLTF2Importer(ecs);
    importer->ImportGLTF(*gltf.data, info.resourceImportFlags_);

    auto const gltfImportResult = importer->GetResult();

    if (!gltfImportResult.success) {
        WIDGET_LOGE("LoadAndImport() Importing of '%s' failed: %s",
            info.fileName_, gltfImportResult.error.c_str());
        return false;
    }

    res.push_back(gltfImportResult.data);

    // Import the default scene, or first scene if there is no default scene
    // set.
    size_t sceneIndex = gltf.data->GetDefaultSceneIndex();
    if (sceneIndex == CORE3D_NS::CORE_GLTF_INVALID_INDEX && gltf.data->GetSceneCount() > 0) {
        // Use first scene.
        sceneIndex = 0;
    }

    CORE_NS::Entity importedSceneEntity;
    if (sceneIndex != CORE3D_NS::CORE_GLTF_INVALID_INDEX) {
        importedSceneEntity = graphicsContext_->GetGltf().ImportGltfScene(
            sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity_, info.sceneImportFlags_);
        if (CORE_NS::EntityUtil::IsValid(importedSceneEntity)) {
            importedEntity = importedSceneEntity;
            WIDGET_LOGE("assign to the importedSceneEntity_");
        }
    }

    // Assume that we always animate the 1st imported scene.
    CORE_NS::Entity animationRoot = sceneEntity_;
    if (info.target_ == GltfImportInfo::AnimateImportedScene &&
        CORE_NS::EntityUtil::IsValid(importedSceneEntity)) {
        // Scenes are self contained, so animate this particular imported
        // scene.
        animationRoot = importedSceneEntity;
    }

    // Create animation playbacks.
    if (!gltfImportResult.data.animations.empty()) {
        CORE3D_NS::INodeSystem* nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
        assert(nodeSystem);

        CORE3D_NS::IAnimationSystem* animationSystem = CORE_NS::GetSystem<CORE3D_NS::IAnimationSystem>(ecs);
        assert(animationSystem);
        if (auto animationRootNode = nodeSystem->GetNode(animationRoot); animationRootNode) {
            auto state = CORE3D_NS::AnimationComponent::PlaybackState::STOP;
            for (const auto& animation : gltfImportResult.data.animations) {
                CORE3D_NS::IAnimationPlayback* playback =
                  animationSystem->CreatePlayback(animation, *animationRootNode);
                if (playback) {
                    playback->SetPlaybackState(state);
                    playback->SetRepeatCount(-1);
                    animations_.push_back(playback);
                }
            }
        }
    }
    return true;
}

void LumeCommon::DrawFrame()
{
    WIDGET_SCOPED_TRACE("LumeCommon::DrawFrame");

    auto* ecs = ecs_.get();
    if (const bool needsRender = engine_->TickFrame(BASE_NS::array_view(&ecs, 1)); needsRender) {
        // Collect render handles here
        CollectRenderHandles();

        //Update custom renders
        const Core::EngineTime et = engine_->GetEngineTime();
        Tick(et.deltaTimeUs);
        if (customRender_) {
            customRender_->OnDrawFrame();
        }

        GetRenderContext()->GetRenderer().RenderFrame(BASE_NS::array_view(renderHandles_.data(),
            renderHandles_.size()));
        if (textureInfo_.textureId_ == 0U && textureInfo_.nativeWindow_) {
            return;
        }
        AddTextureMemoryBarrrier();
    }
}

void LumeCommon::Tick(const uint64_t deltaTime)
{
    if (transformManager_ && sceneManager_ &&
        CORE_NS::EntityUtil::IsValid(cameraEntity_)) {
        orbitCamera_.Update(deltaTime);

        auto const position = orbitCamera_.GetCameraPosition();
        auto const rotation = orbitCamera_.GetCameraRotation();
        if (cameraUpdated_ || position != cameraPosition_ || (rotation.x != cameraRotation_.x) ||
            (rotation.y != cameraRotation_.y) ||
            (rotation.z != cameraRotation_.z) ||
            (rotation.w != cameraRotation_.w)) {
            cameraPosition_ = position;
            cameraRotation_ = rotation;

            auto cameraTransform = transformManager_->Write(cameraEntity_);
            cameraTransform->position = position;
            cameraTransform->rotation = rotation;
            cameraUpdated_ = false;
        }
    }
}

void LumeCommon::OnTouchEvent(const PointerEvent& event)
{
    bool perspectiveCamera = true;
    if (cameraManager_) {
        if (auto cameraHandle = cameraManager_->Read(cameraEntity_); cameraHandle) {
            perspectiveCamera = (cameraHandle->projection == CORE3D_NS::CameraComponent::Projection::PERSPECTIVE);
        }
    }

    if (perspectiveCamera) {
        orbitCamera_.HandlePointerEvent(event);
    }
}

OrbitCameraHelper::OrbitCameraHelper() : pressedButtonsBits_(0x0), touchPointerCount_(0), touchPointers_(),
    orbitDistance_(3.0f), cameraTargetPosition_(0.0f, 0.0f, 0.0f), cameraRotation_(0.0f, 0.0f, 0.0f, 1.0f)
{
}

void OrbitCameraHelper::SetOrbitFromEye(const BASE_NS::Math::Vec3& eyePosition,
                                        const BASE_NS::Math::Quat& rotation,
                                        float orbitDistance)
{
    orbitDistance_ = orbitDistance;
    cameraRotation_ = rotation;
    const BASE_NS::Math::Vec3 toTargetVec =
        cameraRotation_ * BASE_NS::Math::Vec3(0.0f, 0.0f, -orbitDistance_);
    cameraTargetPosition_ = eyePosition + toTargetVec;
}

void OrbitCameraHelper::SetOrbitFromTarget(const BASE_NS::Math::Vec3& targetPosition,
    const BASE_NS::Math::Quat& rotation, float orbitDistance)
{
    orbitDistance_ = orbitDistance;
    cameraRotation_ = rotation;
    cameraTargetPosition_ = targetPosition;
}

BASE_NS::Math::Vec3 OrbitCameraHelper::GetCameraPosition()
{
    return cameraRotation_ * BASE_NS::Math::Vec3(0.0f, 0.0f, orbitDistance_) +
        cameraTargetPosition_;
}

BASE_NS::Math::Quat OrbitCameraHelper::GetCameraRotation()
{
    return cameraRotation_;
}

void OrbitCameraHelper::Update(uint64_t /* delta */)
{
    // Simple stupid pinch zoom (dolly) gesture.
    if (touchPointerCount_ == 2) {
        const BASE_NS::Math::Vec2 pos0(touchPointers_[0].x_, touchPointers_[0].y_);
        const BASE_NS::Math::Vec2 pos1(touchPointers_[1].x_, touchPointers_[1].y_);

        const BASE_NS::Math::Vec2 prevPos0(
            touchPointers_[0].x_ - touchPointers_[0].deltaX_,
            touchPointers_[0].y_ - touchPointers_[0].deltaY_);
        const BASE_NS::Math::Vec2 prevPos1(
            touchPointers_[1].x_ - touchPointers_[1].deltaX_,
            touchPointers_[1].y_ - touchPointers_[1].deltaY_);
        const float deltaDistance = BASE_NS::Math::distance(prevPos0, prevPos1) -
            BASE_NS::Math::distance(pos0, pos1);

        const float sensitivity = 10.0f;
        orbitDistance_ *= 1.0f + (deltaDistance * sensitivity);
        if (orbitDistance_ < 0.01f) {
            orbitDistance_ = 0.01f;
        }

        touchPointers_[0].deltaX_ = 0.0f;
        touchPointers_[0].deltaY_ = 0.0f;
        touchPointers_[1].deltaX_ = 0.0f;
        touchPointers_[1].deltaY_ = 0.0f;
    }
}

void OrbitCameraHelper::ResetPointerEvents()
{
    touchPointerCount_ = 0;
}

void OrbitCameraHelper::OnPress(const PointerEvent& event)
{
    if (event.buttonIndex_ >= 0) {
        pressedButtonsBits_ |= 1 << event.buttonIndex_;
    }

    const bool isMouse = (event.pointerId_ == -1);
    if (isMouse) {
        return;
    }

    touchPointerCount_++;
    if (touchPointerCount_ <= 2 && touchPointerCount_ > 0) {
        touchPointers_[touchPointerCount_ - 1] = event;
    }

    // Initialize midpoint on second press with default values
    if (touchPointerCount_ == 2) {
        midPoint_.x_ = 0;
        midPoint_.y_ = 0;
        midPoint_.deltaX_ = 0;
        midPoint_.deltaY_ = 0;
    }
}

void OrbitCameraHelper::OnRelease(const PointerEvent& event)
{
    if (event.buttonIndex_ >= 0) {
        pressedButtonsBits_ &= ~(1 << event.buttonIndex_);
    }

    const bool isMouse = (event.pointerId_ == -1);
    if (isMouse) {
        return;
    }

    for (int i = 0; i < 2; ++i) {
        if (touchPointers_[i].pointerId_ == event.pointerId_) {
            touchPointers_[i].pointerId_ = -1;
            break;
        }
    }
    touchPointerCount_--;
    if (touchPointerCount_ < 0) {
        touchPointerCount_ = 0;
    }
    // Touch released. Reset midPoint_ to default values
    if (touchPointerCount_ < 2) {
        midPoint_.x_ = 0;
        midPoint_.y_ = 0;
        midPoint_.deltaX_ = 0;
        midPoint_.deltaY_ = 0;
    }
}

void OrbitCameraHelper::UpdateCameraRotation(float dx, float dy)
{
    BASE_NS::Math::Quat rotationX = BASE_NS::Math::AngleAxis(dx, BASE_NS::Math::Vec3(0.0f, -1.0f, 0.0f));
    BASE_NS::Math::Quat rotationY = BASE_NS::Math::AngleAxis(dy, cameraRotation_ *
        BASE_NS::Math::Vec3(-1.0f, 0.0f, 0.0f));
    cameraRotation_ = BASE_NS::Math::Normalize(rotationX * rotationY * cameraRotation_);
}

void OrbitCameraHelper::OnMove(const PointerEvent& event)
{
    if (touchPointerCount_ == 1) {
        touchPointers_[0] = event;

        // Orbit camera with single touch.
        const float sensitivity = 25.0f;
        UpdateCameraRotation(sensitivity * event.deltaX_, sensitivity * event.deltaY_);
    }

    if (touchPointerCount_ == 2) {
        // Pan camera with double touch (will apply this for both pointers).
        PointerEvent newMidPoint;

        auto t1 = touchPointers_[0];
        auto t2 = touchPointers_[1];

        if (t1.pointerId_ == event.pointerId_) {
            t1 = event;
        } else {
            t2 = event;
        }

        auto offset = t1.x_ == std::min(t2.x_, t1.x_) ? t1 : t2;

        newMidPoint.x_ = (abs(t2.x_ - t1.x_) / 2) + offset.x_;
        newMidPoint.y_ = (abs(t2.y_ - t1.y_) / 2) + offset.y_;

        // Mid point at default value (0, 0), assume value of current mid point
        if (midPoint_.x_ == 0 && midPoint_.y_ == 0) {
            midPoint_.x_ = newMidPoint.x_;
            midPoint_.y_ = newMidPoint.y_;
        }

        float dX = newMidPoint.x_ - midPoint_.x_;
        float dY = newMidPoint.y_ - midPoint_.y_;

        if (dX != 0 || dY != 0) {
            const float sensitivity = 3.0f;
            UpdateCameraRotation(sensitivity * dX, sensitivity * dY);
        }

        midPoint_.x_ = newMidPoint.x_;
        midPoint_.y_ = newMidPoint.y_;
        midPoint_.deltaX_ = dX;
        midPoint_.deltaY_ = dY;

        for (int i = 0; i < 2; ++i) {
            if (touchPointers_[i].pointerId_ == event.pointerId_) {
                touchPointers_[i] = event;
                break;
            }
        }
    }
}

void OrbitCameraHelper::HandlePointerEvent(const PointerEvent& event)
{
    switch (event.eventType_) {
        case PointerEventType::PRESSED:
            OnPress(event);
            break;

        case PointerEventType::RELEASED:
        case PointerEventType::CANCELLED:
            OnRelease(event);
            break;

        case PointerEventType::MOVED:
            OnMove(event);
            break;
        default:
            break;
    }
}

#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
void LumeCommon::DeferDraw()
{
    WIDGET_SCOPED_TRACE("LumeCommon::DeferDraw");

    if (renderHandles_.empty()) {
        needsRedraw_ = true;
        CollectRenderHandles();
    } else {
        needsRedraw_ = false;
    }

    const Core::EngineTime et = engine_->GetEngineTime();
    // Update app scene.
    Tick(et.deltaTimeUs);

    //Update custom renders
    if (customeRender_) {
        customeRender_->OnDrawFrame();
    }

    OHOS::Render3D::GraphicsManager::GetInstance().DrawFrame(reinterpret_cast<void *>(ecs_.get()),
        reinterpret_cast<void *>(&renderHandles_));
}

void LumeCommon::DrawMultiEcs(const std::unordered_map<void*, void*>& ecss)
{
    if (ecss.size() == 0) {
        WIDGET_LOGW("ACE-3D LumeCommon::DrawMultiEcs() - No dirty views");
        return;
    }

    BASE_NS::vector<RENDER_NS::RenderHandleReference> handles;
    BASE_NS::vector<CORE_NS::IEcs*> ecsInputs;

    for (auto& key : ecss) {
        CORE_NS::IEcs* ecs = reinterpret_cast<CORE_NS::IEcs *>(key.first);
        ecsInputs.push_back(ecs);

        BASE_NS::vector<RENDER_NS::RenderHandleReference> * dirty =
            reinterpret_cast<BASE_NS::vector<RENDER_NS::RenderHandleReference> *>(key.second);
        handles.insert(handles.end(), dirty->begin(), dirty->end());
    }

    if (engine_->TickFrame(BASE_NS::array_view(ecsInputs.data(), ecsInputs.size()))) {
        GetRenderContext()->GetRenderer().RenderFrame(BASE_NS::array_view(handles.data(), handles.size()));
    }
}
#endif

void LumeCommon::CollectRenderHandles()
{
    renderHandles_.clear();

    if (customRender_) {
        auto rngs = customRender_->GetRenderHandles();
        for (auto r : rngs) {
            renderHandles_.push_back(r);
        }
    }

    if (!renderHandles_.empty()) {
        return;
    }

    auto* ecs = ecs_.get();
    BASE_NS::array_view<const RENDER_NS::RenderHandleReference> main = GetGraphicsContext()->GetRenderNodeGraphs(*ecs);
    if (main.size() == 0) {
        // GLTF resource handles not ready yet in lume. Do not schedule for render.
        return;
    }

    //Order of handles matters to Lume engine. Add custom render last to be drawn on top of GLTF resources
    for (auto handle : main) {
        renderHandles_.push_back(handle);
    }
}

void LumeCommon::CreateEcs(uint32_t key)
{
    if (ecs_ != nullptr) {
        return;
    }
    key_ = key;
    ecs_ = engine_->CreateEcs();
}

void LumeCommon::LoadSystemGraph(BASE_NS::string sysGraph)
{
    auto& ecs = *ecs_;
    static constexpr const RENDER_NS::IShaderManager::ShaderFilePathDesc desc{ "shaders://" };
    GetRenderContext()->GetDevice().GetShaderManager().LoadShaderFiles(desc);

    auto graphFactory = CORE_NS::GetInstance<CORE_NS::ISystemGraphLoaderFactory>(
        CORE_NS::UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = graphFactory->Create(engine_->GetFileManager());
    auto result = systemGraphLoader->Load(sysGraph, ecs);
    if (!result.success) {
        WIDGET_LOGE("load system graph %s error %s", sysGraph.c_str(), result.error.c_str());
    }

    ecs.Initialize();
    transformManager_ = CORE_NS::GetManager<CORE3D_NS::ITransformComponentManager>(ecs);
    cameraManager_ = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(ecs);
    sceneManager_ = CORE_NS::GetManager<CORE3D_NS::IRenderConfigurationComponentManager>(ecs);
    materialManager_ = CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(ecs);
    meshManager_ = CORE_NS::GetManager<CORE3D_NS::IMeshComponentManager>(ecs);
    nameManager_ = CORE_NS::GetManager<CORE3D_NS::INameComponentManager>(ecs);
    uriManager_ = CORE_NS::GetManager<CORE3D_NS::IUriComponentManager>(ecs);
    gpuHandleManager_ = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(ecs);
    nodeSystem_ = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
    renderMeshManager_ = CORE_NS::GetManager<CORE3D_NS::IRenderMeshComponentManager>(ecs);
    lightManager_ = CORE_NS::GetManager<CORE3D_NS::ILightComponentManager>(ecs);
    postprocessManager_ = CORE_NS::GetManager<CORE3D_NS::IPostProcessComponentManager>(ecs);
}

void LumeCommon::CreateScene()
{
    auto nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*ecs_);
    assert(nodeSystem);

    if (auto sceneHandle = sceneManager_->Read(sceneEntity_); sceneHandle) {
        return;
    }

    CORE3D_NS::ISceneNode* rootNode = nodeSystem->CreateNode();
    sceneEntity_ = rootNode->GetEntity();
    sceneManager_->Create(sceneEntity_);

    CORE_NS::ScopedHandle<CORE3D_NS::RenderConfigurationComponent> sceneComponent =
        sceneManager_->Write(sceneEntity_);
    sceneComponent->environment = sceneEntity_;
    sceneComponent->renderingFlags =
        CORE3D_NS::RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
}

void LumeCommon::CreateEnvScene(CORE3D_NS::EnvironmentComponent::Background type)
{
    auto sceneComponent = sceneManager_->Write(sceneEntity_);

    auto envManager = CORE_NS::GetManager<CORE3D_NS::IEnvironmentComponentManager>(*ecs_);
    if (auto envDataHandle = envManager->Read(sceneComponent->environment); envDataHandle) {
        return;
    }

    envManager->Create(sceneComponent->environment);
    auto envDataHandle = envManager->Write(sceneComponent->environment);
    if (!envDataHandle) {
        WIDGET_LOGE("ACE-3D LumeCommon::LoadBackgroundMode get env manager fail");
        return;
    }
    CORE3D_NS::EnvironmentComponent& envComponent = *envDataHandle;
    envComponent.background = type;

    const BASE_NS::Math::Vec3 defaultIrradianceCoefficients[] { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };
    const size_t countOfSh = BASE_NS::countof(envComponent.irradianceCoefficients);
    CORE_ASSERT(countOfSh == BASE_NS::countof(defaultIrradianceCoefficients));
    std::copy(defaultIrradianceCoefficients, defaultIrradianceCoefficients + countOfSh,
        envComponent.irradianceCoefficients);

    if (auto cameraHandle = cameraManager_->Write(cameraEntity_); cameraHandle) {
        if (type == CORE3D_NS::EnvironmentComponent::Background::NONE) {
            cameraHandle->pipelineFlags |= CORE3D_NS::CameraComponent::CLEAR_COLOR_BIT;
        }

        const auto environments = static_cast<CORE_NS::IComponentManager::ComponentId>(
            envManager->GetComponentCount());

        for (CORE_NS::IComponentManager::ComponentId id = 0; id < environments; ++id) {
            const auto entity = envManager->GetEntity(id);
            if (entity == sceneComponent->environment) {
                continue;
            }
            cameraHandle->environment = entity;
            break;
        }
    }
}

void LumeCommon::CreateCamera()
{
    if (auto cameraReadHandle = cameraManager_->Read(cameraEntity_); cameraReadHandle) {
        return;
    }

    const auto& sceneUtil = graphicsContext_->GetSceneUtil();
    cameraEntity_ = sceneUtil.CreateCamera(
        *ecs_, cameraPosition_, cameraRotation_, zNear_, zFar_, fovDegrees_);
    const float distance = BASE_NS::Math::Magnitude(cameraPosition_);
    orbitCamera_.SetOrbitFromEye(cameraPosition_, cameraRotation_, distance);

    if (auto cameraWriteHandle = cameraManager_->Write(cameraEntity_); cameraWriteHandle) {
        cameraWriteHandle->sceneFlags |= CORE3D_NS::CameraComponent::MAIN_CAMERA_BIT |
            CORE3D_NS::CameraComponent::ACTIVE_RENDER_BIT;
        cameraWriteHandle->pipelineFlags |= CORE3D_NS::CameraComponent::MSAA_BIT;
        cameraWriteHandle->renderingPipeline = CORE3D_NS::CameraComponent::RenderingPipeline::FORWARD;
    }
}

void LumeCommon::LoadEnvModel(const std::string& modelPath, BackgroundType type)
{
    UnloadEnvModel();

    GltfImportInfo file { modelPath.c_str(), GltfImportInfo::AnimateImportedScene,
        CORE3D_NS::CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL,
        CORE3D_NS::CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL };

    auto loadResult = LoadAndImport(file, importedEnvEntity_, importedEnvResources_);
    if (!loadResult) {
        WIDGET_LOGE("3D model load fail");
    }

    CORE3D_NS::EnvironmentComponent::Background engineBackgourndType;
    switch (type) {
        case BackgroundType::TRANSPARENT:
            engineBackgourndType = CORE3D_NS::EnvironmentComponent::Background::NONE;
            break;
        case BackgroundType::CUBE_MAP:
        default:
           engineBackgourndType = CORE3D_NS::EnvironmentComponent::Background::CUBEMAP;
           break;
    }

    if (auto sceneHandle = sceneManager_->Read(sceneEntity_); !sceneHandle) {
        CreateScene();
    }
    CreateEnvScene(engineBackgourndType);
}

void LumeCommon::LoadSceneModel(const std::string& modelPath)
{
    uint32_t resourceImportFlags = CORE3D_NS::CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL;
    uint32_t componentImportFlags = CORE3D_NS::CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL;
    componentImportFlags &= ~CORE3D_NS::CORE_GLTF_IMPORT_COMPONENT_ENVIRONMENT;
    UnloadSceneModel();
    CreateScene();
    GltfImportInfo file { modelPath.c_str(), GltfImportInfo::AnimateImportedScene, resourceImportFlags,
        componentImportFlags};
    auto loadResult = LoadAndImport(file, importedSceneEntity_, importedSceneResources_);
    if (!loadResult) {
        WIDGET_LOGE("3D environment model load fail");
    }
}

void LumeCommon::SetupPostprocess()
{
    if (enablePostprocess_ == false) {
        return;
    }

    CORE_NS::IEntityManager& em = ecs_->GetEntityManager();
    postprocessEntity_ = em.Create();
    postprocessManager_->Create(postprocessEntity_);
    auto postProcessHandle = postprocessManager_->Write(postprocessEntity_);

    if (!postProcessHandle) {
        return;
    }

    CORE3D_NS::PostProcessComponent& postProcess = *postProcessHandle;
    postProcess.enableFlags = RENDER_NS::PostProcessConfiguration::ENABLE_BLOOM_BIT
        | RENDER_NS::PostProcessConfiguration::ENABLE_TONEMAP_BIT
        | RENDER_NS::PostProcessConfiguration::ENABLE_COLOR_FRINGE_BIT;
    postProcess.bloomConfiguration.thresholdHard = 0.9f;
    postProcess.bloomConfiguration.thresholdSoft = 2.0f;
    postProcess.bloomConfiguration.amountCoefficient = 2.0f;
    postProcess.colorFringeConfiguration.coefficient = 1.5f;
    postProcess.colorFringeConfiguration.distanceCoefficient = 2.5f;
}

void LumeCommon::UpdateGLTFAnimations(const std::vector<std::shared_ptr<GLTFAnimation>>& animations)
{
    gltfAnimations_.clear();
    gltfAnimations_ = animations;
    animProgress_ = false;
    ProcessGLTFAnimations();
}

void LumeCommon::ProcessGLTFAnimations()
{
    if (animations_.empty()) {
        WIDGET_LOGE("ProcessGLTFAnimations animations empty");
        return;
    }

    for (auto& gltfAnimation : gltfAnimations_) {
        std::string name = gltfAnimation->GetName();
        if (name == "") {
            UpdateSingleGLTFAnimation(0, gltfAnimation);
            WIDGET_LOGE("3D GLTFAnimations name empty");
            continue;
        }

        int index = FindGLTFAnimationIndex(name);
        if (index == -1) {
            continue;
        }
        UpdateSingleGLTFAnimation(index, gltfAnimation);
    }
}

int LumeCommon::FindGLTFAnimationIndex(const std::string& name)
{
    const char* animNameChr = name.c_str();
    uint32_t index = 0;
    for (auto animation : animations_) {
        BASE_NS::string_view getName = animation->GetName();
        int r = getName.compare(animNameChr);
        if (r == 0) {
            // Animation is found.
            break;
        }
        index++;
    }

    if (index == animations_.size()) {
        return -1;
    } else {
        return index;
    }
}

void LumeCommon::UpdateSingleGLTFAnimation(int index, const std::shared_ptr<GLTFAnimation>& gltfAnimation)
{
    CORE3D_NS::AnimationComponent::PlaybackState state;
    switch (gltfAnimation->GetState()) {
        case AnimationState::STOP:
            state = CORE3D_NS::AnimationComponent::PlaybackState::STOP;
            break;
        case AnimationState::PAUSE:
            state = CORE3D_NS::AnimationComponent::PlaybackState::PAUSE;
            break;
        case AnimationState::PLAY:
        default:
            state = CORE3D_NS::AnimationComponent::PlaybackState::PLAY;
            animProgress_ = true;
            break;
    }
    animations_[index]->SetPlaybackState(state);

    if (gltfAnimation->GetRepeatCount() == -1) {
        animations_[index]->SetRepeatCount(-1); // infinite
    } else {
        animations_[index]->SetRepeatCount(gltfAnimation->GetRepeatCount());
    }

    if (gltfAnimation->GetReverse()) {
        animations_[index]->SetSpeed(-gltfAnimation->GetSpeed());
    } else {
        animations_[index]->SetSpeed(gltfAnimation->GetSpeed());
    }

    if (gltfAnimation->GetDuration() != -1.0) {
        animations_[index]->SetDuration(gltfAnimation->GetDuration());
    }
}

void CreateGeometry(CORE_NS::Entity& sceneEntity_, CORE_NS::Entity& entityMesh,
    const std::shared_ptr<Geometry>& geometryEntity, CORE3D_NS::INodeSystem& nodeSystem,
    CORE3D_NS::IRenderMeshComponentManager& rmm)
{
    auto pos = geometryEntity->GetPosition();

    auto scene = nodeSystem.GetNode(sceneEntity_);
    auto node = nodeSystem.CreateNode();
    node->SetName(geometryEntity->GetName().c_str());
    node->SetPosition({pos.GetX(), pos.GetY(), pos.GetZ()});
    node->SetParent(*scene);

    auto entity = node->GetEntity();
    rmm.Create(entity);

    auto mesh = rmm.Get(entity);
    mesh.mesh = entityMesh;
    rmm.Set(entity, mesh);
}

void DestroyNode(CORE_NS::Entity& sceneEntity,
    std::shared_ptr<Geometry>& shape, CORE3D_NS::INodeSystem& nodeSystem)
{
    auto sceneNode = nodeSystem.GetNode(sceneEntity);
    auto shapeNode = sceneNode->LookupNodeByName(shape->GetName().c_str());
    if (shapeNode) {
        nodeSystem.DestroyNode(*shapeNode);
    } else {
        WIDGET_LOGW("Failed to remove: %s", shape->GetName().c_str());
    }
}

void CreateNode(CORE_NS::Entity& sceneEntity, CORE_NS::IEcs::Ptr& ecs,
    CORE3D_NS::IMeshUtil& meshUtil, const std::shared_ptr<Geometry>& entity,
    const CORE_NS::Entity& materialEntity, CORE3D_NS::INodeSystem& nodeSystem,
    CORE3D_NS::IRenderMeshComponentManager& rmm)
{
    switch (entity->GetType()) {
        case GeometryType::CUBE:
        {
            auto& cube = static_cast<Cube &>(*entity);
            auto mesh = meshUtil.GenerateCubeMesh(*ecs, entity->GetName().c_str(), materialEntity,
            cube.GetWidth(), cube.GetHeight(), cube.GetDepth());
            CreateGeometry(sceneEntity, mesh, entity, nodeSystem, rmm);
            break;
        }
        case GeometryType::SPHARE:
        {
            auto& sphere = static_cast<Sphere &>(*entity);
            auto mesh = meshUtil.GenerateSphereMesh(*ecs, entity->GetName().c_str(),
                materialEntity, sphere.GetRadius(), sphere.GetRings(), sphere.GetSectors());
            CreateGeometry(sceneEntity, mesh, entity, nodeSystem, rmm);
            break;
        }
        case GeometryType::CONE:
        {
            auto& cone = static_cast<Cone &>(*entity);
            auto mesh = meshUtil.GenerateConeMesh(*ecs, entity->GetName().c_str(),
                materialEntity, cone.GetRadius(), cone.GetLength(), cone.GetSectors());
            CreateGeometry(sceneEntity, mesh, entity, nodeSystem, rmm);
            break;
        }
        default:
            break;
    }
}

void UpdateNodePosition(CORE_NS::Entity& sceneEntity,
    std::shared_ptr<Geometry>& shape, CORE3D_NS::INodeSystem& nodeSystem)
{
    auto sceneNode = nodeSystem.GetNode(sceneEntity);
    auto shapeNode = sceneNode->LookupNodeByName(shape->GetName().c_str());
    if (shapeNode) {
        auto pos = shape->GetPosition();
        shapeNode->SetPosition({pos.GetX(), pos.GetY(), pos.GetZ()});
        return;
    }

    WIDGET_LOGW("Failed to Update: %s", shape->GetName().c_str());
}

void LumeCommon::LoadCustGeometry(const std::vector<std::shared_ptr<Geometry>>& shapes)
{
    if (!(sceneManager_ && sceneManager_->Read(sceneEntity_))) {
        WIDGET_LOGE("ACE-3D LumeCommon::LoadCustGeometry() Scene is not set up yet");
        return;
    }

    auto rmm = CORE_NS::GetManager<CORE3D_NS::IRenderMeshComponentManager>(*ecs_);
    auto nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*ecs_);
    if (!rmm || !nodeSystem) {
        WIDGET_LOGE("render mesh component %d nodeSystem %d", !!rmm, !!nodeSystem);
        return;
    }

    CORE3D_NS::MaterialComponent desc;
    desc.textures[CORE3D_NS::MaterialComponent::TextureIndex::MATERIAL]
        .factor.z = 0.0f;
    desc.textures[CORE3D_NS::MaterialComponent::TextureIndex::MATERIAL]
        .factor.y = 1.0f;

    // default material
    const CORE_NS::Entity materialEntity = ecs_->GetEntityManager().Create();
    auto materialManager =
        CORE_NS::GetManager<CORE3D_NS::IMaterialComponentManager>(*ecs_);
    materialManager->Set(materialEntity, desc);

    CORE3D_NS::IMeshUtil& meshUtil = graphicsContext_->GetMeshUtil();

    for (auto& shape : shapes) {
        auto find = std::find_if(shapes_.begin(), shapes_.end(), [&shape](const std::shared_ptr<Geometry>& sOld) {
            return shape->GetName() == sOld->GetName();
        });
        if (find != shapes_.end()) {
            // shape already exists on scene, update
            const std::shared_ptr<OHOS::Render3D::Geometry>& oldShape = *find;

            bool updateEntity = !(shape->Equals(*oldShape));
            bool updatePosition = !(shape->PositionEquals(*oldShape));
            if (updateEntity) {
                // destroy node, and recreate it
                DestroyNode(sceneEntity_, *find, *nodeSystem);
                CreateNode(sceneEntity_, ecs_, meshUtil, shape, materialEntity, *nodeSystem, *rmm);
            } else if (updatePosition) {
                // just reposition the node
                UpdateNodePosition(sceneEntity_, *find, *nodeSystem);
            } else {
                // just update the map
            }
            shapesMap_[shape->GetName()] = shape;
        } else {
            // Shape does not exist on scene, create
            // update map
            CreateNode(sceneEntity_, ecs_, meshUtil, shape, materialEntity, *nodeSystem, *rmm);
            shapes_.push_back(shape);
        }
    }
}

bool LumeCommon::DestroySwapchain()
{
    WIDGET_LOGD("LumeCommon::DestroySwapchin");
#ifdef CREATE_SURFACE
    EGLBoolean ret = EGL_TRUE;
    if (!device_) {
        WIDGET_LOGE("no device but has eglSurface");
        return false;
    }

    if (eglSurface_ == EGL_NO_SURFACE) {
        return true;
    }

    if (swapchainHandle_ && device_) {
        device_->DestroySwapchain(swapchainHandle_);
    }

    const auto& data = static_cast<const RENDER_NS::DevicePlatformDataGLES&>(device_->GetPlatformData());
    ret = eglDestroySurface(data.display, eglSurface_);
    eglSurface_ = EGL_NO_SURFACE;
#endif
    // need check destroy swapchain
    swapchainHandle_ = {};
    return true;
}

void LumeCommon::InitializeScene(uint32_t key)
{
    CreateEcs(key);
    BASE_NS::string sysGraph = "rofs3D://systemGraph.json";
    LoadSystemGraph(sysGraph);
    CreateCamera();
}

bool LumeCommon::CreateSwapchain(void* nativeWindow)
{
    WIDGET_SCOPED_TRACE("LumeCommon::CreateSwapchain");
    WIDGET_LOGD("LumeCommon::CreateSwapchain");
#ifdef CREATE_SURFACE
    if (nativeWindow == nullptr) {
        return false;
    }
    DestroySwapchain();

    auto window = reinterpret_cast<EGLNativeWindowType>(nativeWindow);
    EGLint COLOR_SPACE = 0;
    EGLint COLOR_SPACE_SRGB = 0;
    const auto& data = static_cast<const RENDER_NS::DevicePlatformDataGLES&>(device_->GetPlatformData());
    EGLConfig config = data.config;

    bool hasSRGB = true;
    if ((data.majorVersion > 1) || ((data.majorVersion == 1) && (data.minorVersion > 4))) {
        COLOR_SPACE = EGL_GL_COLORSPACE;
        COLOR_SPACE_SRGB = EGL_GL_COLORSPACE_SRGB;
    } else if (data.hasColorSpaceExt) {
        COLOR_SPACE = EGL_GL_COLORSPACE_KHR;
        COLOR_SPACE_SRGB = EGL_GL_COLORSPACE_SRGB_KHR;
    } else {
        hasSRGB = false;
    }

    if (hasSRGB) {
        const EGLint attribs[] = { COLOR_SPACE, COLOR_SPACE_SRGB, EGL_NONE };
        eglSurface_ = eglCreateWindowSurface(data.display, config, window, attribs);
        if (eglSurface_ == EGL_NO_SURFACE) {
            // okay fallback to whatever colorformat
            EGLint error = eglGetError();
            // EGL_BAD_ATTRIBUTE is generated if attrib_list contains an invalid window attribute or if an
            // attribute value is not recognized or is out of range
            WIDGET_LOGD("fallback to linear egl surface for reason %d", error); // EGL_BAD_ATTRIBUTE 0x3004
            hasSRGB = false;
        }
    }

    if (!hasSRGB) {
        eglSurface_ = eglCreateWindowSurface(data.display, config, window, nullptr);
        if (eglSurface_ != EGL_NO_SURFACE) {
            WIDGET_LOGI("create linear egl surface success");
        } else {
            EGLint error = eglGetError();
            WIDGET_LOGE("fail to create linear egl surface for reason %d", error);
        }
    }
    RENDER_NS::SwapchainCreateInfo swapchainCreateInfo {
        reinterpret_cast<uint64_t>(eglSurface_),
        RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT |
            RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT,
    };
#endif
    RENDER_NS::SwapchainCreateInfo swapchainCreateInfo {
        // reinterpret_cast<uint64_t>(eglSurface_), true, true,
        0U,
        RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT |
        RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT |
        RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT,
        RENDER_NS::ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        {
            reinterpret_cast<uintptr_t>(nativeWindow),
            reinterpret_cast<uintptr_t>(static_cast<const RENDER_NS::DevicePlatformDataGLES&>(
                device_->GetPlatformData()).display)
        }

    };
    swapchainHandle_ = device_->CreateSwapchainHandle(swapchainCreateInfo, swapchainHandle_, {});
    return eglSurface_ != EGL_NO_SURFACE;
}

void LumeCommon::SetupCustomRenderTarget(const TextureInfo &info)
{
    if (ecs_ == nullptr) {
        WIDGET_LOGE("ecs has not created");
        return;
    }

    auto& ecs = *ecs_;
    auto cameraComponent = cameraManager_->Write(cameraEntity_);
    auto* rhcManager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(ecs);
    cameraComponent->customColorTargets.clear();
    CORE_NS::EntityReference imageEntity;
    
    if (info.textureId_ == 0U && info.nativeWindow_) {
        // need check recreate window
        CreateSwapchain(info.nativeWindow_);
        imageEntity = GetOrCreateEntityReference(ecs.GetEntityManager(), *rhcManager, swapchainHandle_);
    } else {
        auto imageEntity = CORE3D_NS::GetOrCreateEntityReference(ecs.GetEntityManager(),
            *rhcManager, SetupGpuImageTarget());
        auto depthEntity = CORE3D_NS::GetOrCreateEntityReference(ecs.GetEntityManager(),
            *rhcManager, SetupGpuDepthTarget());
    }
    cameraComponent->postProcess = postprocessEntity_;
    cameraComponent->customColorTargets.emplace_back(std::move(imageEntity));
}

void LumeCommon::UpdateLights(const std::vector<std::shared_ptr<OHOS::Render3D::Light>>& lights)
{
    const auto& sceneUtil = graphicsContext_->GetSceneUtil();
    auto& ecs = *ecs_;

    for (auto i = 0U; i < lights.size(); i++) {
        BASE_NS::Math::Vec3 position;
        BASE_NS::Math::Quat rotation;
        auto& light = lights[i];
        GetLightPositionAndRotation(light, position, rotation);
        // Check and update exisiting light entity or create a new light entity.
        if (lightEntities_.size() > i) {
            if (auto oldLC = lightManager_->Write(lightEntities_.at(i)); oldLC) {
                oldLC->type = static_cast<CORE3D_NS::LightComponent::Type>(light->GetLightType());
                oldLC->color = BASE_NS::Math::Vec3(light->GetLightColor().GetX(), light->GetLightColor().GetY(),
                    light->GetLightColor().GetZ());
                oldLC->intensity = light->GetLightIntensity();
                oldLC->shadowEnabled = light->GetLightShadow();

                CORE3D_NS::TransformComponent tc;
                tc.position = position;
                tc.rotation = rotation;
                transformManager_->Set(lightEntities_.at(i), tc);
            } else {
                WIDGET_LOGE("update exsiting light error");
            }
        } else {
            CORE3D_NS::LightComponent lc;
            lc.type = static_cast<CORE3D_NS::LightComponent::Type>(light->GetLightType());
            lc.intensity = light->GetLightIntensity();
            lc.shadowEnabled = light->GetLightShadow();
            lc.color = BASE_NS::Math::Vec3(light->GetLightColor().GetX(), light->GetLightColor().GetY(),
                light->GetLightColor().GetZ());
            lightEntities_.push_back(sceneUtil.CreateLight(ecs, lc, position, rotation));
        }
    }
}

RENDER_NS::RenderHandleReference LumeCommon::SetupGpuImageTarget()
{
    std::string name = "tex_img" + std::to_string(key_);
    if (gpuResourceImgHandle_) {
        return gpuResourceImgHandle_;
    }

    auto imageDesc = GetImageDesc();
    RENDER_NS::ImageDescGLES glesImageDesc;

    glesImageDesc.type = GL_TEXTURE_2D;
    glesImageDesc.image = textureInfo_.textureId_;
    glesImageDesc.internalFormat = GL_RGBA8_OES;
    glesImageDesc.format = imageDesc.format;
    glesImageDesc.dataType = GL_UNSIGNED_BYTE;
    glesImageDesc.bytesperpixel = 4;

    gpuResourceImgHandle_ = GetRenderContext()->GetDevice().GetGpuResourceManager().CreateView(
        BASE_NS::string_view(name.c_str()), imageDesc, glesImageDesc);

    WIDGET_LOGD("ACE-3D LumeCommon::SetupGpuImageTarget texture %d", textureInfo_.textureId_);
    return gpuResourceImgHandle_;
}

RENDER_NS::RenderHandleReference LumeCommon::SetupGpuDepthTarget()
{
    std::string name = "depth_target" + std::to_string(key_);
    if (gpuDepthTargetHandle_) {
        return gpuDepthTargetHandle_;
    }

    auto imageDesc = GetImageDesc();
    imageDesc.format = Base::Format::BASE_FORMAT_D24_UNORM_S8_UINT;
    imageDesc.usageFlags =
        Render::ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    gpuDepthTargetHandle_ =
        GetRenderContext()->GetDevice().GetGpuResourceManager().Create(name.c_str(), imageDesc);

    return gpuDepthTargetHandle_;
}

RENDER_NS::GpuImageDesc LumeCommon::GetImageDesc()
{
    RENDER_NS::GpuImageDesc imageDesc;
    imageDesc.imageType = Render::ImageType::CORE_IMAGE_TYPE_2D;
    imageDesc.imageViewType = Render::ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    imageDesc.format = Base::Format::BASE_FORMAT_R8G8B8A8_SRGB;
    imageDesc.imageTiling = Render::ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    imageDesc.usageFlags =
        Render::ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageDesc.memoryPropertyFlags =
        Render::MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imageDesc.createFlags = 0;
    imageDesc.engineCreationFlags =
        RENDER_NS::EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS |
        RENDER_NS::EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    imageDesc.width = textureInfo_.width_;
    imageDesc.height = textureInfo_.height_;
    imageDesc.depth = 1;

    return imageDesc;
}

void LumeCommon::SetupCameraViewProjection(float zNear, float zFar, float fovDegrees)
{
    zNear_ = zNear;
    zFar_ = zFar;
    fovDegrees_ = fovDegrees;
}

void LumeCommon::SetupCameraTransform(const OHOS::Render3D::Position& position, const OHOS::Render3D::Vec3& lookAt,
    const OHOS::Render3D::Vec3& up, const OHOS::Render3D::Quaternion& rotation)
{
    if (position.GetIsAngular()) {
        float radius = position.GetDistance();
        float x = (radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * position.GetX())));
        float z = (radius * BASE_NS::Math::cos((BASE_NS::Math::DEG2RAD * position.GetZ())));
        float y = (radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * position.GetY())));
        cameraPosition_ = BASE_NS::Math::Vec3(x, y, z);
    } else {
        cameraPosition_ = BASE_NS::Math::Vec3(position.GetX(), position.GetY(), position.GetZ());
    }

    if (IsValidQuaternion(rotation)) {
        // App provided rotation. So use it directly.
        cameraRotation_ = BASE_NS::Math::Quat(
            rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW());
    } else {
        // Use LookAtRh API.
        BASE_NS::Math::Vec3 look(lookAt.GetX(), lookAt.GetY(), lookAt.GetZ());
        BASE_NS::Math::Vec3 upTmp(up.GetX(), up.GetY(), up.GetZ());
        BASE_NS::Math::Mat4X4 mat = BASE_NS::Math::LookAtRh(cameraPosition_, look, upTmp);
        float determinantOut;
        BASE_NS::Math::Mat4X4 invMat = BASE_NS::Math::Inverse(mat, determinantOut);
        // Workaround for Lume LookAtRh API invalid matrix for specific data. Eg:
        // This workaround can handle only limited cases. Proper fix should come form Lume engine.
        // Eg., test data:
        // pos(0.0, 3.0, 0.0), lookAt(0.0, 0.0, 0.0), up(0.0, 1.0, 0.0)
        // pos(3.0, 0.0, 0.0), lookAt(0.0, 0.0, 0.0), up(1.0, 0.0, 0.0)
        // pos(0.0, 0.0, 3.0), lookAt(0.0, 0.0, 0.0), up(0.0, 0.0, 1.0)
        if (std::abs(determinantOut) < 0.001f) {
            WIDGET_LOGW("ACE-3D Inverse LookAt matrix is invalid.");
            BASE_NS::Math::Vec3 modifiedPos(cameraPosition_.x, cameraPosition_.y, cameraPosition_.z + 0.0001f);
            mat = BASE_NS::Math::LookAtRh(modifiedPos, look, upTmp);
            invMat = BASE_NS::Math::Inverse(mat, determinantOut);
        }
        BASE_NS::Math::Vec3 scale;
        BASE_NS::Math::Quat orientation;
        BASE_NS::Math::Vec3 translation;
        BASE_NS::Math::Vec3 skew;
        BASE_NS::Math::Vec4 perspective;
        BASE_NS::Math::Decompose(invMat, scale, orientation, translation, skew, perspective);
        cameraPosition_ = translation;
        cameraRotation_ = orientation;
    }
    // Update the Orbit camera.
    const float distance = BASE_NS::Math::Magnitude(cameraPosition_);
    orbitCamera_.SetOrbitFromEye(cameraPosition_, cameraRotation_, distance);
    cameraUpdated_ = true;
    // Needed to update Transform manager's cameraTransform.
}

bool LumeCommon::NeedsRepaint()
{
    auto isAnimating = animProgress_ || needsFrameCallback_;
    auto handlesNotReady = renderHandles_.size() == 0 || needsRedraw_;
    return isAnimating || handlesNotReady;
}

void LumeCommon::SetupCameraViewPort(uint32_t width, uint32_t height)
{
    if (cameraManager_ == nullptr || graphicsContext_ == nullptr) {
        return;
    }

    auto cameraComponent = cameraManager_->Read(cameraEntity_);
    if (!cameraComponent) {
        WIDGET_LOGE("ACE-3D LumeCommon::SetUpCameraViewPort get camera component error");
        return;
    }

    autoAspect_ = (cameraComponent->aspect <= 0.0f);
    originalYfov_ = cameraComponent->yFov;
    graphicsContext_->GetSceneUtil().UpdateCameraViewport(*ecs_, cameraEntity_,
        { width, height }, autoAspect_, originalYfov_, orthoScale_);
}

bool LumeCommon::IsValidQuaternion(const OHOS::Render3D::Quaternion& quat)
{
    auto max = std::numeric_limits<float>::max();
    if (quat == Quaternion(max, max, max, max)) {
        return false;
    }
    return true;
}

void LumeCommon::AddTextureMemoryBarrrier()
{
    WIDGET_SCOPED_TRACE("LumeCommon::AddTextureMemoryBarrrier");
    AutoRestore scope;
    auto lumeContext = static_cast<const RENDER_NS::DevicePlatformDataGLES&>(
        GetRenderContext()->GetDevice().GetPlatformData()).context;

    auto disp = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    auto res = eglMakeCurrent(disp, EGL_NO_SURFACE, EGL_NO_SURFACE, lumeContext);

    if (!res) {
        WIDGET_LOGE("Make lume context error %d", eglGetError());
        return;
    }

    glMemoryBarrierByRegion(GL_TEXTURE_FETCH_BARRIER_BIT | GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    auto sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    glWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
    glDeleteSync(sync);
}

void LumeCommon::GetLightPositionAndRotation(const std::shared_ptr<Light>& light,
    BASE_NS::Math::Vec3& position, BASE_NS::Math::Quat& rotation)
{
    if (!light) {
        WIDGET_LOGE("GetLightPositionAndRotation() light is null");
        return;
    }

    if (light->GetPosition().GetIsAngular()) {
        float radius = light->GetPosition().GetDistance();
        float x = (radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * light->GetPosition().GetX())));
        float z = (radius * BASE_NS::Math::cos((BASE_NS::Math::DEG2RAD * light->GetPosition().GetZ())));
        float y = (radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * light->GetPosition().GetY())));
        position = BASE_NS::Math::Vec3(x, y, z);
    } else {
        position = BASE_NS::Math::Vec3(light->GetPosition().GetX(), light->GetPosition().GetY(),
            light->GetPosition().GetZ());
    }

    if (IsValidQuaternion(light->GetRotation())) {
        // App provided rotation. So use it directly.
        rotation = BASE_NS::Math::Quat(light->GetRotation().GetX(), light->GetRotation().GetY(),
            light->GetRotation().GetZ(), light->GetRotation().GetW());
    } else {
        // Use defaults. lookAt and up vectors are not provided in Applicatino API currently.
        BASE_NS::Math::Vec3 look(0.0, 0.0, 0.0);
        BASE_NS::Math::Vec3 up(0.0, 1.0, 0.0);
        BASE_NS::Math::Mat4X4 mat = BASE_NS::Math::LookAtRh(position, look, up);
        float determinantOut;
        BASE_NS::Math::Mat4X4 invMat = BASE_NS::Math::Inverse(mat, determinantOut);
        // Workaround for Lume LookAtRh API invalid matrix for specific data.
        if (std::abs(determinantOut) < 0.001f) {
            WIDGET_LOGE("ACE-3D Inverse LookAt matrix is invalid.");
            BASE_NS::Math::Vec3 modified_pos(position.x, position.y, position.z + 0.0001f);
            mat = BASE_NS::Math::LookAtRh(modified_pos, look, up);
            invMat = BASE_NS::Math::Inverse(mat, determinantOut);
            WIDGET_LOGD("ACE-3D 2nd Inverse LookAtRh: determinantOut: %f", determinantOut);
        }
        BASE_NS::Math::Vec3 scale;
        BASE_NS::Math::Quat orientation;
        BASE_NS::Math::Vec3 translation;
        BASE_NS::Math::Vec3 skew;
        BASE_NS::Math::Vec4 perspective;
        BASE_NS::Math::Decompose(invMat, scale, orientation, translation, skew, perspective);
        position = translation;
        rotation = orientation;
    }
}

} // namespace OHOS::Render3D
