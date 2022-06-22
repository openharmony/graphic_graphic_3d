/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_SCENE_VIEW_ADAPTER_H
#define OHOS_RENDER_3D_SCENE_VIEW_ADAPTER_H

#include "data_type/constants.h"
#include "data_type/geometry/geometry.h"
#include "data_type/geometry/geometry.h"
#include "data_type/geometry/cube.h"
#include "data_type/geometry/sphere.h"
#include "data_type/geometry/cone.h"
#include "data_type/gltf_animation.h"

#include "i_engine.h"
#include <memory>
#include <GLES/gl.h>
#include <EGL/egl.h>

namespace OHOS::Render3D {
class SceneViewerAdapter {
public:
    explicit SceneViewerAdapter(uint32_t key);
    virtual ~SceneViewerAdapter();
    void SetEngine(std::unique_ptr<IEngine> engine);
    void DeInitEngine();
    SceneViewerAdapter(const SceneViewerAdapter&) = delete;
    SceneViewerAdapter& operator=(const SceneViewerAdapter&) = delete;

    void SetUpSceneViewer(const TextureInfo &info, std::string src,
        std::string backgroundSrc, SceneViewerBackgroundType bgType);

    void SetUpCameraTransform(float position[], float rotationAngle, float rotationAxis[]);
    void SetUpCameraViewProjection(float zNear, float zFar, float fovDegrees);

    void SetLightProperties(int lightType, float color[], float intensity,
        bool shadow, float position[], float rotationAngle, float rotationAxis[]);

    void CreateLight();
    void SetUpCustomRenderTarget(const TextureInfo &info);
    void UnLoadModel();

    void OnTouchEvent(const SceneViewerTouchEvent& event);
    bool IsAnimating();

    void DrawFrame();
    void Tick(const uint64_t aTotalTime, const uint64_t aDeltaTime);
    void AddGeometries(const std::vector<OHOS::Ace::RefPtr<SVGeometry>>& shapes);
    void UpdateGLTFAnimations(const std::vector<OHOS::Ace::RefPtr<GLTFAnimation>>& animations);

private:
    std::unique_ptr<IEngine> engine_ = nullptr;
#if MULTI_ECS_UPDATE_AT_ONCE
    bool firstFrame_ = true;
#endif
    uint32_t key_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SCENE_VIEW_ADAPTER_H
