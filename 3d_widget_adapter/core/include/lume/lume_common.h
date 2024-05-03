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

#ifndef OHOS_RENDER_3D_LUME_COMMON_H
#define OHOS_RENDER_3D_LUME_COMMON_H

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/transform_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <3d/ecs/systems/intf_morphing_system.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_system.h>

#include <3d/implementation_uids.h>
#include <3d/gltf/gltf.h>
#include <3d/intf_graphics_context.h>

#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_picking.h>
#include <3d/util/intf_scene_util.h>

#include <base/containers/string_view.h>
#include <base/math/mathf.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>

#include <core/ecs/intf_entity_manager.h>
#include <core/engine_info.h>
#include <core/intf_engine.h>
#include <core/namespace.h>

#include <render/intf_plugin.h>
#include <render/intf_render_context.h>
#include <render/device/intf_device.h>

#include "custom/lume_custom_render.h"
#include "i_engine.h"

namespace OHOS::Render3D {
struct GltfImportInfo {
    enum AnimationTarget { AnimateMainScene, AnimateImportedScene };
    const char* fileName_;
    AnimationTarget target_;
    CORE3D_NS::GltfResourceImportFlags resourceImportFlags_;
    CORE3D_NS::GltfSceneImportFlags sceneImportFlags_;
};

struct LightInfo {
    int type_;
    float intensity_;
    bool shadow_;
    BASE_NS::Math::Vec3 color_;
    BASE_NS::Math::Vec3 position_;
    BASE_NS::Math::Quat rotation_;
};

class OrbitCameraHelper final {
public:
    OrbitCameraHelper();
    ~OrbitCameraHelper() = default;

    OrbitCameraHelper(const OrbitCameraHelper&) = delete;
    OrbitCameraHelper& operator=(const OrbitCameraHelper&) = delete;

    void SetOrbitFromEye(const BASE_NS::Math::Vec3& eyePosition, const BASE_NS::Math::Quat& rotation,
        float orbitDistance);

    void SetOrbitFromTarget(const BASE_NS::Math::Vec3& targetPosition, const BASE_NS::Math::Quat& rotation,
        float orbitDistance);

    BASE_NS::Math::Vec3 GetCameraPosition();
    BASE_NS::Math::Quat GetCameraRotation();
    void Update(uint64_t /* delta */);
    void ResetPointerEvents();
    void HandlePointerEvent(const PointerEvent& event);

private:
    void OnPress(const PointerEvent& event);
    void OnRelease(const PointerEvent& event);
    void OnMove(const PointerEvent& event);
    void UpdateCameraRotation(float dx, float dy);
    unsigned int pressedButtonsBits_;
    // Just a very simple and stupid touch gesture system.
    int touchPointerCount_;
    PointerEvent touchPointers_[2];
    // Double touch handling
    PointerEvent midPoint_;

    float orbitDistance_;
    BASE_NS::Math::Vec3 cameraTargetPosition_;
    BASE_NS::Math::Quat cameraRotation_;
};

class LumeCommon : public IEngine {
public:
    LumeCommon() = default;
    ~LumeCommon() override;
    void Clone(IEngine* proto) override;
    void UnloadEngineLib() override;
    bool LoadEngineLib() override;
    bool InitEngine(EGLContext eglContext, const PlatformData& data) override;
    void DeInitEngine() override;

    void InitializeScene(uint32_t key) override;

    void LoadEnvModel(const std::string& modelPath, BackgroundType type) override;
    void LoadSceneModel(const std::string& modelPath) override;

    void UpdateGeometries(const std::vector<std::shared_ptr<Geometry>>& shapes) override;
    void UpdateGLTFAnimations(const std::vector<std::shared_ptr<GLTFAnimation>>& animations) override;
    void UpdateLights(const std::vector<std::shared_ptr<OHOS::Render3D::Light>>& lights) override;

    void UpdateCustomRender(const std::shared_ptr<CustomRenderDescriptor>& customRender) override;
    void UpdateShaderPath(const std::string& shaderPath) override;
    void UpdateImageTexturePaths(const std::vector<std::string>& imageTextures) override;
    void UpdateShaderInputBuffer(const std::shared_ptr<OHOS::Render3D::ShaderInputBuffer>& shaderInputBuffer) override;

    void SetupCameraViewPort(uint32_t width, uint32_t height) override;
    void SetupCameraTransform(const OHOS::Render3D::Position& position, const OHOS::Render3D::Vec3& lookAt,
        const OHOS::Render3D::Vec3& up, const OHOS::Render3D::Quaternion& rotation) override;
    void SetupCameraViewProjection(float zNear, float zFar, float fovDegrees) override;

    void UnloadSceneModel() override;
    void UnloadEnvModel() override;
    void DrawFrame() override;

    void OnTouchEvent(const PointerEvent& event) override;
    void OnWindowChange(const TextureInfo& textureInfo) override;

#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    void DeferDraw() override;
    void DrawMultiEcs(const std::unordered_map<void*, void*>& ecss) override;
#endif
    bool NeedsRepaint() override;

protected:
    virtual CORE_NS::PlatformCreateInfo ToEnginePlatformData(const PlatformData& data) const = 0;
    virtual void RegisterAssertPath() = 0;
    void LoadSystemGraph(BASE_NS::string sysGraph);
    void CreateEcs(uint32_t key);
    void CreateScene();
    void DestroyScene();
    void CreateEnvScene(CORE3D_NS::EnvironmentComponent::Background type);
    void DestroyEnvScene();
    void DestroySceneNodeAndRes(CORE_NS::Entity& importedEntity, BASE_NS::vector<CORE3D_NS::GLTFResourceData>& res);
    void CreateCamera();
    void LoadCustGeometry(const std::vector<std::shared_ptr<Geometry>>& shapes);
    void SetupPostprocess();
    void AddTextureMemoryBarrrier();
    void SetupCustomRenderTarget(const TextureInfo &info);
    void CreateLight();
    void ProcessGLTFAnimations();
    int FindGLTFAnimationIndex(const std::string& name);
    void UpdateSingleGLTFAnimation(int index, const std::shared_ptr<GLTFAnimation>& gltfAnimation);
    bool LoadAndImport(const GltfImportInfo& info, CORE_NS::Entity& importedEntity,
        BASE_NS::vector<CORE3D_NS::GLTFResourceData>& res);

    bool CreateSwapchain(void* nativeWindow);
    bool DestroySwapchain();
    void DestroyResource();
    void Tick(const uint64_t deltaTime);

    CORE_NS::IEngine::Ptr CreateCoreEngine(const Core::PlatformCreateInfo &info);
    CORE_NS::IEngine::Ptr GetCoreEngine();

    RENDER_NS::RenderHandleReference SetupGpuImageTarget();
    RENDER_NS::RenderHandleReference SetupGpuDepthTarget();
    RENDER_NS::GpuImageDesc GetImageDesc();
    RENDER_NS::IRenderContext::Ptr CreateRenderContext(EGLContext gfxContext);
    RENDER_NS::IDevice* GetDevice();
    RENDER_NS::IRenderContext::Ptr GetRenderContext();

    CORE3D_NS::IGraphicsContext::Ptr CreateGfx3DContext();
    CORE3D_NS::IGraphicsContext::Ptr GetGraphicsContext();

    bool IsValidQuaternion(const OHOS::Render3D::Quaternion& quat);
    void CollectRenderHandles();
    void GetLightPositionAndRotation(const std::shared_ptr<OHOS::Render3D::Light>& light,
        BASE_NS::Math::Vec3& position, BASE_NS::Math::Quat& rotation);
    std::shared_ptr<LumeCustomRender> CustomRenderFactory(const std::string& renderNodeGraph, bool needsFrameCallback);
    CORE_NS::IEngine::Ptr engine_;
    CORE_NS::IEcs::Ptr ecs_;
    CORE_NS::Entity cameraEntity_;
    CORE_NS::Entity sceneEntity_;
    CORE_NS::Entity postprocessEntity_;
    std::vector<CORE_NS::Entity> lightEntities_;

    CORE_NS::Entity importedSceneEntity_;
    BASE_NS::vector<CORE3D_NS::GLTFResourceData> importedSceneResources_;

    CORE_NS::Entity importedEnvEntity_;
    BASE_NS::vector<CORE3D_NS::GLTFResourceData> importedEnvResources_;

    BASE_NS::vector<CORE3D_NS::IAnimationPlayback*> animations_;
    BASE_NS::Math::Vec3 cameraPosition_{ 0.0f, 0.0f, 4.0f };
    BASE_NS::Math::Quat cameraRotation_{ 0.0f, 0.0f, 0.0f, 1.0f };

    CORE3D_NS::IGraphicsContext::Ptr graphicsContext_;
    CORE3D_NS::ITransformComponentManager* transformManager_;
    CORE3D_NS::ICameraComponentManager* cameraManager_;
    CORE3D_NS::IRenderConfigurationComponentManager* sceneManager_;
    CORE3D_NS::ILightComponentManager* lightManager_;
    CORE3D_NS::IPostProcessComponentManager* postprocessManager_;

    RENDER_NS::IRenderContext::Ptr renderContext_;
    RENDER_NS::RenderHandleReference gpuResourceImgHandle_;
    RENDER_NS::RenderHandleReference gpuDepthTargetHandle_;
    RENDER_NS::IDevice *device_ = nullptr;
    RENDER_NS::RenderHandleReference swapchainHandle_;

    OrbitCameraHelper orbitCamera_;
    std::vector<std::shared_ptr<GLTFAnimation>> gltfAnimations_;
    std::vector<std::shared_ptr<Geometry>> shapes_;
    std::unordered_map<std::string, std::shared_ptr<Geometry>> shapesMap_;

    CORE3D_NS::IMaterialComponentManager* materialManager_ { nullptr };
    CORE3D_NS::IMeshComponentManager* meshManager_ { nullptr };
    CORE3D_NS::INameComponentManager* nameManager_ { nullptr };
    CORE3D_NS::IUriComponentManager* uriManager_ { nullptr };
    CORE3D_NS::IRenderHandleComponentManager* gpuHandleManager_ { nullptr };
    CORE3D_NS::INodeSystem* nodeSystem_ { nullptr };
    CORE3D_NS::IRenderMeshComponentManager* renderMeshManager_ { nullptr };

    // Shader
    std::shared_ptr<LumeCustomRender> customRender_;
    BASE_NS::vector<RENDER_NS::RenderHandleReference> renderHandles_;

    bool autoAspect_ = true;
    float originalYfov_ = BASE_NS::Math::DEG2RAD * 60.0f;
    float orthoScale_ = 3.0f;
    bool enablePostprocess_ = true;
    void *libHandle_ = nullptr;
    uint32_t key_;
    float zNear_ = 0.5f;
    float zFar_ = 200.0f;
    float fovDegrees_ = 60.0f;
    bool animProgress_ = false;
    bool cameraUpdated_ = true;
    bool needsRedraw_ = false;
    bool needsFrameCallback_ = false;
    void* nativeWindow_ = nullptr;
    bool activateWeatherPhys_ = false;

    EGLSurface eglSurface_ = EGL_NO_SURFACE;
    TextureInfo textureInfo_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_LUME_COMMON_H
