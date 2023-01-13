/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "lume_common.h"

#include <dlfcn.h>
#include <string_view>

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES3/gl31.h>
#include <GLES2/gl2ext.h>

#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/local_matrix_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/ecs/systems/intf_render_system.h>
#include <3d/implementation_uids.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_picking.h>
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
#include "base/log/ace_trace.h"
#include "graphics_manager.h"

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
    DeInitEngine();
}

void LumeCommon::UnLoadEngineLib()
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
        WIDGET_LOGE("lume Create Engine fail");
        return nullptr;
    }

    RegisterAssertPath();

    engine_->Init();
    return engine_;
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

#if CORE_HAS_VULKAN_BACKEND
    Core::BackendExtraVk vkExtra;
    Core::DeviceCreateInfo vulkanDeviceCreateInfo;
    vulkanDeviceCreateInfo.backendType = Core::DeviceBackendType::VULKAN;
    vulkanDeviceCreateInfo.backendConfiguration = &vkExtra;
#endif

#if CORE_HAS_GLES_BACKEND
    RENDER_NS::BackendExtraGLES glesExtra;
    glesExtra.applicationContext = EGL_NO_CONTEXT;
    glesExtra.sharedContext = gfxContext;
    glesExtra.MSAASamples = 0;
    glesExtra.depthBits = 0; // 24 bits of depth buffer.
    Render::DeviceCreateInfo glesDeviceCreateInfo;
    glesDeviceCreateInfo.backendType = Render::DeviceBackendType::OPENGLES;
    glesDeviceCreateInfo.backendConfiguration = &glesExtra;

#endif

#if CORE_HAS_GL_BACKEND
    Core::BackendExtraGL glExtra;
    glExtra.MSAASamples = 0;
    glExtra.depthBits = 24;
    glExtra.alphaBits = 8;
    glExtra.stencilBits = 0;
    DeviceCreateInfo glDeviceCreateInfo;
    glDeviceCreateInfo.backendType = DeviceBackendType::OPENGL;
    glDeviceCreateInfo.backendConfiguration = &glExtra;
#endif

    const RENDER_NS::RenderCreateInfo renderCreateInfo {
        {
            "core_gltf_viewer", // name
            1,
            0,
            0,
        },
        glesDeviceCreateInfo,
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
    UnLoadModel();
    graphicsContext_ = nullptr;
    renderContext_ = nullptr;
    engine_ = nullptr;
    ecs_ = nullptr;
}

void LumeCommon::AddGeometries(const std::vector<OHOS::Ace::RefPtr<SVGeometry>>& shapes)
{
    shapes_.clear();
    shapes_ = shapes;
    LoadCustGeometry(shapes_);
}

void LumeCommon::UnLoadModel()
{
    OHOS::Ace::ACE_SCOPED_TRACE("LumeCommon::UnloadModel");
    WIDGET_LOGD("ACE-3D LumeCommon::UnloadModel");

    if (engine_ == nullptr || importedResources_.empty()) {
        return;
    }
    auto& ecs = *ecs_;
    // clean animations
    auto animationSystem = CORE_NS::GetSystem<CORE3D_NS::IAnimationSystem>(ecs);
    for (auto animation : animations_) {
        animationSystem->DestroyPlayback(animation);
    }
    animations_.clear();

    // clean scene nodes
    CORE3D_NS::INodeSystem& nodeSystem = *CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(ecs);
    CORE3D_NS::ISceneNode* sceneRoot = nodeSystem.GetNode(importedSceneEntity_);
    if (sceneRoot) {
        nodeSystem.DestroyNode(*sceneRoot);
    }
    importedSceneEntity_ = {};

    // clean imported resources
    importedResources_.pop_back();
    // run garbage collection
    ecs.ProcessEvents();
}

bool LumeCommon::IsAnimating()
{
    return animProgress_;
}

void LumeCommon::LoadAndImport(const GltfImportInfo& info, const CORE_NS::Entity& sceneEntity)
{
    OHOS::Ace::ACE_SCOPED_TRACE("LumeCommon::LoadAndImport");
    WIDGET_LOGD("ACE-3D LumeCommon::LoadAndImport() ");
    auto& ecs = *ecs_;
    auto gltf = graphicsContext_->GetGltf().LoadGLTF(info.fileName_);
    if (!gltf.success) {
        WIDGET_LOGE("ACE-3D LumeCommon::LoadAndImport() Loaded '%s' with errors:\n%s",
            info.fileName_, gltf.error.c_str());
    }
    if (!gltf.data) {
        WIDGET_LOGE("LoadAndImport gltf data is null. Error: %s ", gltf.error.c_str());
        return;
    }

    auto importer = graphicsContext_->GetGltf().CreateGLTF2Importer(ecs);
    importer->ImportGLTF(*gltf.data, info.resourceImportFlags_);

    auto const gltfImportResult = importer->GetResult();

    if (gltfImportResult.success) {
        importedResources_.push_back(gltfImportResult.data);

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
                sceneIndex, *gltf.data, gltfImportResult.data, ecs, sceneEntity, info.sceneImportFlags_);
            if (CORE_NS::EntityUtil::IsValid(importedSceneEntity)) {
                importedSceneEntity_ = importedSceneEntity;
            }
        }

        // Assume that we always animate the 1st imported scene.
        CORE_NS::Entity animationRoot = sceneEntity;
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
                auto state = CORE3D_NS::AnimationComponent::PlaybackState::PLAY;
                for (const auto& animation : gltfImportResult.data.animations) {
                    CORE3D_NS::IAnimationPlayback* playback =
                      animationSystem->CreatePlayback(animation, *animationRootNode);
                    if (playback) {
                        playback->SetPlaybackState(state);
                        playback->SetRepeatCount(Ace::ANIMATION_REPEAT_INFINITE);
                        animations_.push_back(playback);
                        state = CORE3D_NS::AnimationComponent::PlaybackState::STOP;
                    }
                }
            }
        }

        // Scale and move the scene root so that the loaded scene is on the plane
        auto* context = GetRenderContext()->GetInterface<CORE_NS::IClassRegister>();
        assert(context);

        auto picking = CORE_NS::GetInstance<CORE3D_NS::IPicking>(*context, CORE3D_NS::UID_PICKING);
        assert(picking);

        if (auto aabb = picking->GetTransformComponentAABB(importedSceneEntity_, true, ecs);
            aabb.minAABB.x < aabb.maxAABB.x) {
            const auto dim = aabb.maxAABB - aabb.minAABB;
            // Scale so that the maximum bounding dimention is 2.
            const auto scale = 2.f / BASE_NS::Math::max(dim.x, BASE_NS::Math::max(dim.y, dim.z));
            // Calculate the where the center of AABB bottom is after scaling.
            BASE_NS::Math::Vec3 offset;
            offset.y = aabb.minAABB.y;
            offset.x = (aabb.minAABB.z + aabb.maxAABB.z) * 0.5f;
            offset *= scale;
            if (auto transformHandle = transformManager_->Write(importedSceneEntity_); transformHandle) {
                transformHandle->scale = BASE_NS::Math::Vec3(scale, scale, scale);
                // Move the scene root so that it would be centered on our -1 plane.
                transformHandle->position += BASE_NS::Math::Vec3(0.f, -1.0f, 0.f) - offset;
            }
        }
    } else {
        WIDGET_LOGE("ACE-3D LumeCommon::LoadAndImport() Importing of '%s' failed: %s",
            info.fileName_, gltfImportResult.error.c_str());
    }
}

void LumeCommon::DrawFrame()
{
    OHOS::Ace::ACE_SCOPED_TRACE("LumeCommon::DrawFrame");
    // Update app scene.
    const Core::EngineTime et = engine_->GetEngineTime();

    Tick(et.totalTimeUs, et.deltaTimeUs);

    auto* ecs = ecs_.get();
    const bool needsRender = engine_->TickFrame(BASE_NS::array_view(&ecs, 1));
    if (needsRender) {
        auto render_node_graph = GetGraphicsContext()->GetRenderNodeGraphs(*ecs);
        GetRenderContext()->GetRenderer().RenderFrame(render_node_graph);
    }
}

void LumeCommon::Tick(const uint64_t aTotalTime, const uint64_t aDeltaTime)
{
    OHOS::Ace::ACE_SCOPED_TRACE("LumeCommon::Tick");
    // Apply orbit camera transform. using the scene default camera.
    if (transformManager_ && sceneManager_ &&
        CORE_NS::EntityUtil::IsValid(cameraEntity_)) {
        orbitCamera_.Update(aDeltaTime);

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

void LumeCommon::OnTouchEvent(const SceneViewerTouchEvent& event)
{
    OHOS::Ace::ACE_SCOPED_TRACE("LumeCommon::OnTouchEvent");

    auto viewWidth = textureInfo_.width_;
    auto viewHeight = textureInfo_.height_;

    if (viewWidth == 0 || viewWidth == 0) {
        return;
    }

    PointerEvent pointerEvent{};
    pointerEvent.buttonIndex_ = -1;

    pointerEvent.pointerId_ = event.GetFingerId();
    pointerEvent.x_ = event.GetLocalLocation().GetX() / viewWidth;
    pointerEvent.y_ = event.GetLocalLocation().GetY() / viewHeight;

    pointerEvent.deltaX_ = event.GetDeltaChange().GetX() / viewWidth;
    pointerEvent.deltaY_ = event.GetDeltaChange().GetY() / viewHeight;

    switch (event.GetEventType()) {
        case OHOS::Ace::TouchType::DOWN:
            pointerEvent.eventType_ = PointerEventType::PRESSED;
            break;
        case OHOS::Ace::TouchType::UP:
            pointerEvent.eventType_ = PointerEventType::RELEASED;
            break;
        case OHOS::Ace::TouchType::MOVE:
            pointerEvent.eventType_ = PointerEventType::MOVED;
            break;
        case OHOS::Ace::TouchType::CANCEL:
            pointerEvent.eventType_ = PointerEventType::CANCELLED;
            break;
        case OHOS::Ace::TouchType::PULL_DOWN:
        case OHOS::Ace::TouchType::PULL_UP:
        case OHOS::Ace::TouchType::PULL_MOVE:
        case OHOS::Ace::TouchType::PULL_IN_WINDOW:
        case OHOS::Ace::TouchType::PULL_OUT_WINDOW:
        case OHOS::Ace::TouchType::UNKNOWN:
            break;
    }

    bool perspectiveCamera = true;
    if (cameraManager_) {
        if (auto cameraHandle = cameraManager_->Read(cameraEntity_); cameraHandle) {
            perspectiveCamera = (cameraHandle->projection == CORE3D_NS::CameraComponent::Projection::PERSPECTIVE);
        }
    }

    if (perspectiveCamera) {
        orbitCamera_.HandlePointerEvent(pointerEvent);
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
    WIDGET_LOGD("ACE-3D OrbitCameraHelper::SetOrbitFromEye()");
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
    OHOS::Ace::ACE_SCOPED_TRACE("OrbitCameraHelper::Update");

    // Simple stupid pinch zoom (dolly) gesture.
    if (touchPointerCount_ == 2) {
        WIDGET_LOGD("ACE-3D OrbitCameraHelper::Update()");

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
    OHOS::Ace::ACE_SCOPED_TRACE("OrbitCameraHelper::Handle PointerEvent");

    const bool isMouse = (event.pointerId_ == -1);
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

#if MULTI_ECS_UPDATE_AT_ONCE
void LumeCommon::DeferDraw()
{
    OHOS::Ace::ACE_SCOPED_TRACE("LumeCommon::DeferDraw");

    const Core::EngineTime et = engine_->GetEngineTime();
    // Update app scene.
    Tick(et.totalTimeUs, et.deltaTimeUs);
    OHOS::Render3D::GraphicsManager::GetInstance().DrawFrame(reinterpret_cast<void *>(ecs_.get()));
}

void LumeCommon::DrawMultiEcs(const std::vector<void *> &ecss)
{
    BASE_NS::vector<RENDER_NS::RenderHandleReference> handles;
    BASE_NS::vector<CORE_NS::IEcs*> ecsInputs;

    for (auto& ecs: ecss) {
        ecsInputs.push_back(reinterpret_cast<CORE_NS::IEcs *>(ecs));
        BASE_NS::array_view<const RENDER_NS::RenderHandleReference> main =
            graphicsContext_->GetRenderNodeGraphs(*(reinterpret_cast<CORE_NS::IEcs *>(ecs)));
        for (auto handle : main) {
            handles.push_back(handle);
        }
    }

    WIDGET_LOGD("ACE-3D LumeCommon::DrawMultiEcs ecsInputs %lu, handles: %lu", ecsInputs.size(), handles.size());
    engine_->TickFrame(BASE_NS::array_view(ecsInputs.data(), ecsInputs.size()));
    GetRenderContext()->GetRenderer().RenderFrame(BASE_NS::array_view(handles.data(), handles.size()));
}
#endif

void LumeCommon::CreateEcs(uint32_t key)
{
    if (ecs_ != nullptr) {
        return;
    }
    key_ = key;
    ecs_ = engine_->CreateEcs();
    auto& ecs = *ecs_;
    auto graphFactory = CORE_NS::GetInstance<CORE_NS::ISystemGraphLoaderFactory>(
        CORE_NS::UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = graphFactory->Create(engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", ecs);

    BASE_NS::string prefix = "LumeCommon" + BASE_NS::string{std::to_string(key).c_str()};
    auto& renderStoreManager = GetRenderContext()->GetRenderDataStoreManager();

    CORE3D_NS::IRenderPreprocessorSystem::Properties props;
    props.dataStoreManager = &renderStoreManager;
    props.dataStoreScene = prefix + "RenderDataStoreDefaultScene";
    props.dataStoreCamera = prefix + "RenderDataStoreDefaultCamera";
    props.dataStoreLight = prefix + "RenderDataStoreDefaultLight";
    props.dataStoreMaterial = prefix + "RenderDataStoreDefaultMaterial";
    props.dataStoreMorph = prefix + "RenderDataStoreDefaultMorph"; ;

    auto renderPreprocessorSystem = CORE_NS::GetSystem<CORE3D_NS::IRenderPreprocessorSystem>(ecs);
    if (renderPreprocessorSystem == nullptr) {
        WIDGET_LOGE("Get renderPreprocessorSystem fail");
        return;
    }

    *CORE_NS::ScopedHandle<CORE3D_NS::IRenderPreprocessorSystem::Properties>(
        renderPreprocessorSystem->GetProperties()) = props;

    ecs.Initialize();

    transformManager_ = CORE_NS::GetManager<CORE3D_NS::ITransformComponentManager>(ecs);
    cameraManager_ = CORE_NS::GetManager<CORE3D_NS::ICameraComponentManager>(ecs);
    sceneManager_ = CORE_NS::GetManager<CORE3D_NS::IRenderConfigurationComponentManager>(ecs);
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


    // Create scene and environment
    CORE3D_NS::ISceneNode* rootNode = nodeSystem->CreateNode();
    sceneEntity_ = rootNode->GetEntity();
    sceneManager_->Create(sceneEntity_);

    CORE_NS::ScopedHandle<CORE3D_NS::RenderConfigurationComponent> sceneComponent =
        sceneManager_->Write(sceneEntity_);
    sceneComponent->environment = sceneEntity_;
    sceneComponent->renderingFlags =
        CORE3D_NS::RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT;
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
    }
}

void LumeCommon::LoadBackgroundModel(std::string modelPath, SceneViewerBackgroundType type)
{    // Update env map type
    CORE_NS::ScopedHandle<CORE3D_NS::RenderConfigurationComponent> sceneComponent =
        sceneManager_->Write(sceneEntity_);
    auto envManager = CORE_NS::GetManager<CORE3D_NS::IEnvironmentComponentManager>(*ecs_);
    envManager->Create(sceneComponent->environment);
    auto envDataHandle = envManager->Write(sceneComponent->environment);
    if (!envDataHandle) {
        WIDGET_LOGE("ACE-3D LumeCommon::LoadBackgroundMode get env manager fail");
        return;
    }

    CORE3D_NS::EnvironmentComponent& envComponent = *envDataHandle;
    switch (type) {
        case SceneViewerBackgroundType::TRANSPARENT:
            envComponent.background = CORE3D_NS::EnvironmentComponent::Background::NONE;
            break;

        case SceneViewerBackgroundType::CUBE_MAP:
        default:
            envComponent.background = CORE3D_NS::EnvironmentComponent::Background::CUBEMAP;
            break;
    }

    const BASE_NS::Math::Vec3 defaultIrradianceCoefficients[] { { 1.0f, 1.0f, 1.0f }, { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f },
        { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } };

    const size_t countOfSh = BASE_NS::countof(envComponent.irradianceCoefficients);
    CORE_ASSERT(countOfSh == BASE_NS::countof(defaultIrradianceCoefficients));
    std::copy(defaultIrradianceCoefficients, defaultIrradianceCoefficients + countOfSh,
        envComponent.irradianceCoefficients);

    auto cameraHandle = cameraManager_->Write(cameraEntity_);

    if (!cameraHandle) {
        return;
    }

    cameraHandle->pipelineFlags |= CORE3D_NS::CameraComponent::MSAA_BIT |
        CORE3D_NS::CameraComponent::ALLOW_COLOR_PRE_PASS_BIT;

    cameraHandle->renderingPipeline = CORE3D_NS::CameraComponent::RenderingPipeline::FORWARD;

    if (type == SceneViewerBackgroundType::TRANSPARENT) {
        cameraHandle->pipelineFlags |= CORE3D_NS::CameraComponent::CLEAR_COLOR_BIT;
    }

    GltfImportInfo file { modelPath.c_str(), GltfImportInfo::AnimateImportedScene,
        CORE3D_NS::CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL,
        CORE3D_NS::CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL };

    LoadAndImport(file, sceneEntity_);

    const auto environments = static_cast<CORE_NS::IComponentManager::ComponentId>(envManager->GetComponentCount());

    for (CORE_NS::IComponentManager::ComponentId id = 0; id < environments; ++id) {
        const auto entity = envManager->GetEntity(id);
        if (entity == sceneComponent->environment) {
            continue;
        }
        if (cameraHandle) {
            cameraHandle->environment = entity;
            cameraHandle->sceneFlags |= CORE3D_NS::CameraComponent::MAIN_CAMERA_BIT;
        }
    }
}

void LumeCommon::LoadSceneModel(std::string modelPath)
{
    uint32_t resourceImportFlags = CORE3D_NS::CORE_GLTF_IMPORT_RESOURCE_FLAG_BITS_ALL;
    uint32_t componentImportFlags = CORE3D_NS::CORE_GLTF_IMPORT_COMPONENT_FLAG_BITS_ALL;
    componentImportFlags &= ~CORE3D_NS::CORE_GLTF_IMPORT_COMPONENT_ENVIRONMENT;
    GltfImportInfo file { modelPath.c_str(), GltfImportInfo::AnimateImportedScene, resourceImportFlags,
        componentImportFlags};

    LoadAndImport(file, sceneEntity_);
}

void LumeCommon::SetUpPostprocess()
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

void LumeCommon::UpdateGLTFAnimations(const std::vector<OHOS::Ace::RefPtr<GLTFAnimation>>& animations)
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

    if (gltfAnimations_.empty()) {
        UpdateSingleGLTFAnimation(0, OHOS::Ace::AceType::MakeRefPtr<GLTFAnimation>
            ("", AnimationState::PLAY));
        return;
    }

    for (auto& gltfAnimation : gltfAnimations_) {
        std::string name = gltfAnimation->GetName();
        if (name == "") {
            UpdateSingleGLTFAnimation(0, gltfAnimation);
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
    int index = 0;
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

void LumeCommon::UpdateSingleGLTFAnimation(int index, const OHOS::Ace::RefPtr<GLTFAnimation>& gltfAnimation)
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
        animations_[index]->SetRepeatCount(Ace::ANIMATION_REPEAT_INFINITE);
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
    OHOS::Ace::RefPtr<SVGeometry>& svEntity, CORE3D_NS::INodeSystem& nodeSystem,
    CORE3D_NS::IRenderMeshComponentManager& rmm)
{
    auto pos = svEntity->GetPosition();

    auto scene = nodeSystem.GetNode(sceneEntity_);
    auto node = nodeSystem.CreateNode();
    node->SetPosition({pos.GetX(), pos.GetY(), pos.GetZ()});
    node->SetParent(*scene);

    auto entity = node->GetEntity();
    rmm.Create(entity);

    auto mesh = rmm.Get(entity);
    mesh.mesh = entityMesh;
    rmm.Set(entity, mesh);
}

void LumeCommon::LoadCustGeometry(std::vector<OHOS::Ace::RefPtr<SVGeometry>> &shapes)
{
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

    auto rmm = CORE_NS::GetManager<CORE3D_NS::IRenderMeshComponentManager>(*ecs_);
    auto nodeSystem = CORE_NS::GetSystem<CORE3D_NS::INodeSystem>(*ecs_);

    assert(rmm);
    assert(nodeSystem);

    auto& meshUtil = graphicsContext_->GetMeshUtil();
    auto scene = nodeSystem->GetNode(sceneEntity_);

    for (auto& entity : shapes_) {
        switch (entity->GetType()) {
            case 0:
            {
                WIDGET_LOGW("ACE-3D LumeCommon::InitializeScene() GenerateCube()");
                auto svCube = OHOS::Ace::AceType::DynamicCast<SVCube>(entity);
                auto mesh = meshUtil.GenerateCubeMesh(*ecs_, entity->GetName().c_str(), materialEntity,
                svCube->GetWidth(), svCube->GetHeight(), svCube->GetDepth());
                CreateGeometry(sceneEntity_, mesh, entity, *nodeSystem, *rmm);
                break;
            }
            case 1:
            {
                WIDGET_LOGW("ACE-3D LumeCommon::InitializeScene() GenerateSphere()");
                auto svSphere = OHOS::Ace::AceType::DynamicCast<SVSphere>(entity);
                auto mesh = meshUtil.GenerateSphereMesh(*ecs_, entity->GetName().c_str(),
                    materialEntity, svSphere->GetRadius(), svSphere->GetRings(), svSphere->GetSectors());
                CreateGeometry(sceneEntity_, mesh, entity, *nodeSystem, *rmm);
                break;
            }
            case 2:
            {
                WIDGET_LOGW("ACE-3D LumeCommon::InitializeScene() GenerateCone()");
                auto svCone = OHOS::Ace::AceType::DynamicCast<SVCone>(entity);
                auto mesh = meshUtil.GenerateConeMesh(*ecs_, entity->GetName().c_str(),
                    materialEntity, svCone->GetRadius(), svCone->GetLength(), svCone->GetSectors());
                CreateGeometry(sceneEntity_, mesh, entity, *nodeSystem, *rmm);
                break;
            }
            default:
            break;
        }
    }
}

void LumeCommon::SetUpCustomRenderTarget(const TextureInfo &info)
{
    WIDGET_LOGD("ACE-3D lumeCommon %s %d", __func__, __LINE__);
    textureInfo_ = info;
    if (ecs_ == nullptr) {
        WIDGET_LOGE("ecs has not created");
        return;
    }

    auto& ecs = *ecs_;
    auto cameraComponent = cameraManager_->Write(cameraEntity_);
    auto* rhcManager = CORE_NS::GetManager<CORE3D_NS::IRenderHandleComponentManager>(ecs);

    auto imageEntity = CORE3D_NS::GetOrCreateEntityReference(ecs.GetEntityManager(),
        *rhcManager, SetupGpuImageTarget());
    auto depthEntity = CORE3D_NS::GetOrCreateEntityReference(ecs.GetEntityManager(),
        *rhcManager, SetupGpuDepthTarget());

    cameraComponent->customColorTargets.clear();
    cameraComponent->customColorTargets.emplace_back(std::move(imageEntity));
    cameraComponent->customDepthTarget = std::move(depthEntity);
    cameraComponent->postProcess = postprocessEntity_;
}

void LumeCommon::CreateLight()
{
    WIDGET_LOGD("ACE-3D LumeCommon::CreateLight()");
    const auto& sceneUtil = graphicsContext_->GetSceneUtil();
    auto& ecs = *ecs_;

    CORE3D_NS::LightComponent lc;
    lc.type = static_cast<CORE3D_NS::LightComponent::Type>(lightInfo_.type_);
    lc.color = lightInfo_.color_;
    lc.intensity = lightInfo_.intensity_;
    lc.shadowEnabled = lightInfo_.shadow_;

    WIDGET_LOGD(" lightType: %d", (int)lc.type);
    WIDGET_LOGD(" color: %f, %f, %f", lc.color.x, lc.color.y, lc.color.z);
    WIDGET_LOGD(" intensity: %f ", lc.intensity);
    WIDGET_LOGD(" shadowEnabled: %d", lc.shadowEnabled);

    if (auto light = lightManager_->Write(lightEntity_); light) {
        WIDGET_LOGD("ACE-3D LumeCommon::CreateLight() Update existing light.");
        light->type = static_cast<CORE3D_NS::LightComponent::Type>(lightInfo_.type_);
        light->color = lightInfo_.color_;
        light->intensity = lightInfo_.intensity_;
        light->shadowEnabled = lightInfo_.shadow_;

        CORE3D_NS::TransformComponent tc;
        tc.position = lightInfo_.position_;
        tc.rotation = lightInfo_.rotation_;
        transformManager_->Set(lightEntity_, tc);
    } else {
        WIDGET_LOGD("ACE-3D LumeCommon::CreateLight() Create new light");
        lightEntity_ = sceneUtil.CreateLight(ecs, lc, lightInfo_.position_, lightInfo_.rotation_);
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
    WIDGET_LOGD("ACE-3D LumeCommon::SetupGpuDepthTarget() SetupGpuDepthTarget ");

    std::string name = "depth_target" + std::to_string(key_);
    if (gpuDepthTargetHandle_) {
        WIDGET_LOGD("  ACE-3D LumeCommon::SetupGpuDepthTarget() depth_set_ Already.");
        return gpuDepthTargetHandle_;
    }

    auto imageDesc = GetImageDesc();
    imageDesc.format = Base::Format::BASE_FORMAT_D24_UNORM_S8_UINT;
    imageDesc.usageFlags =
        Render::ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

    gpuDepthTargetHandle_ =
        GetRenderContext()->GetDevice().GetGpuResourceManager().Create(name.c_str(), imageDesc);
      // GraphicsService::Get()->GetRenderContext()->GetDevice().GetGpuResourceManager().Create(

    WIDGET_LOGD("ACE-3D LumeCommon::SetupGpuDepthTarget() End");
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

void LumeCommon::SetLightProperties(int lightType, float color[], float intensity,
    bool shadow, float position[], float rotationAngle, float rotationAxis[])
{
    WIDGET_LOGD("ACE-3D SetLightProperties");
    lightInfo_.type_ = lightType;
    lightInfo_.intensity_ = intensity;
    lightInfo_.shadow_ = shadow;
    lightInfo_.color_ = BASE_NS::Math::Vec3(color[0], color[1], color[2]);

    float radius = position[3];
    float x = (radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * position[0])));
    float z = (radius * BASE_NS::Math::cos((BASE_NS::Math::DEG2RAD * position[2])));
    float y = -(radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * position[1])));

    WIDGET_LOGD("ACE-3D LumeCommon::SetLightProperties position degrees: %f, %f, %f ", position[0],
        position[1], position[2]);
    WIDGET_LOGD("ACE-3D LumeCommon::SetLightProperties position x: %f, y: %f, z: %f", x, y, z);

    lightInfo_.position_ = BASE_NS::Math::Vec3(x, y, z);
    lightInfo_.rotation_ = BASE_NS::Math::AngleAxis((BASE_NS::Math::DEG2RAD * rotationAngle),
        BASE_NS::Math::Vec3(rotationAxis[0], rotationAxis[1], rotationAxis[2]));
}

void LumeCommon::SetUpCameraViewProjection(float zNear, float zFar, float fovDegrees)
{
    zNear_ = zNear;
    zFar_ = zFar;
    fovDegrees_ = fovDegrees;
}

void LumeCommon::SetUpCameraTransform(float position[], float rotationAngle, float rotationAxis[])
{
    float radius = position[3];
    float x = (radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * position[0])));
    float z = (radius * BASE_NS::Math::cos((BASE_NS::Math::DEG2RAD * position[2])));
    float y = -(radius * BASE_NS::Math::sin((BASE_NS::Math::DEG2RAD * position[1])));

    cameraPosition_ = BASE_NS::Math::Vec3(x, y, z);
    cameraRotation_ = BASE_NS::Math::AngleAxis(
        (BASE_NS::Math::DEG2RAD * rotationAngle),
        BASE_NS::Math::Vec3(rotationAxis[0], rotationAxis[1], rotationAxis[2]));

    // Update the Orbit camera.
    const float distance = BASE_NS::Math::Magnitude(cameraPosition_);
    orbitCamera_.SetOrbitFromEye(cameraPosition_, cameraRotation_, distance);
    cameraUpdated_ = true;
    // Needed to update Transform manager's cameraTransform.
}

void LumeCommon::SetUpCameraViewPort(uint32_t width, uint32_t height)
{
    WIDGET_LOGD("ACE-3D %s", __func__);
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
} // namespace OHOS::Render3D
