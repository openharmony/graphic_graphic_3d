/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_I_ENGINE_H
#define OHOS_RENDER_3D_I_ENGINE_H

#define ACE_SCENE_VIEW_DEBUG
#include "data_type/constants.h"
#include "data_type/geometry/cube.h"
#include "data_type/geometry/sphere.h"
#include "data_type/geometry/cone.h"
#include "data_type/gltf_animation.h"
#include "data_type/scene_viewer_touch_event.h"

#include "platform_data.h"
#include "texture_info.h"
#include <cstdint>
#include <string>

#include <EGL/egl.h>
#include <GLES/gl.h>

namespace OHOS::Render3D {
class IEngine {
public:
    virtual ~IEngine() = default;
    virtual void Clone(IEngine* proto) = 0;
    virtual bool LoadEngineLib() = 0;
    virtual bool InitEngine(EGLContext eglContext, const PlatformData& data) = 0;
    virtual void DeInitEngine() = 0;
    virtual void UnLoadEngineLib() = 0;

    virtual void CreateEcs(uint32_t key) = 0;
    virtual void CreateScene() = 0;
    virtual void CreateCamera() = 0;
    virtual void SetUpPostprocess() = 0;
    virtual void LoadCustGeometry(std::vector<OHOS::Ace::RefPtr<SVGeometry>> &shapes) = 0;

    virtual void SetUpCustomRenderTarget(const TextureInfo &info) = 0;
    virtual void SetUpCameraViewPort(uint32_t width, uint32_t height) = 0;
    virtual void SetUpCameraTransform(float position[], float rotationAngle, float rotationAxis[]) = 0;
    virtual void SetUpCameraViewProjection(float zNear, float zFar, float fovDegrees) = 0;

    virtual void CreateLight() = 0;
    virtual void SetLightProperties(int lightType, float color[], float intensity, bool shadow, float position[],
        float rotationAngle, float rotationAxis[]) = 0;

    virtual void LoadSceneModel(std::string modelPath) = 0;
    virtual void LoadBackgroundModel(std::string modelPath, SceneViewerBackgroundType type) = 0;
    virtual void UnLoadModel() = 0;

    virtual void OnTouchEvent(const SceneViewerTouchEvent& event) = 0;
    virtual bool IsAnimating() = 0;
    virtual void DrawFrame() = 0;
    virtual void Tick(const uint64_t aTotalTime, const uint64_t aDeltaTime) = 0;

    virtual void AddGeometries(const std::vector<OHOS::Ace::RefPtr<SVGeometry>>& shapes) = 0;
    virtual void UpdateGLTFAnimations(const std::vector<OHOS::Ace::RefPtr<GLTFAnimation>>& animations) = 0;

#if MULTI_ECS_UPDATE_AT_ONCE
    virtual void DeferDraw() = 0 ;
    virtual void DrawMultiEcs(const std::vector<void *> &ecss) = 0;
#endif
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_I_ENGINE_H
