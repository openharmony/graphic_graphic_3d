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
    bool SetEngine(std::unique_ptr<IEngine> engine);
    void DeInitEngine();
    SceneViewerAdapter(const SceneViewerAdapter&) = delete;
    SceneViewerAdapter& operator=(const SceneViewerAdapter&) = delete;

    bool SetUpSceneViewer(const TextureInfo &info, std::string src,
        std::string backgroundSrc, SceneViewerBackgroundType bgType);

    bool SetUpCameraTransform(float position[], float rotationAngle, float rotationAxis[]);
    bool SetUpCameraViewProjection(float zNear, float zFar, float fovDegrees);

    bool SetLightProperties(int lightType, float color[], float intensity,
        bool shadow, float position[], float rotationAngle, float rotationAxis[]);

    bool CreateLight();
    bool SetUpCustomRenderTarget(const TextureInfo &info);
    bool UnLoadModel();

    bool OnTouchEvent(const SceneViewerTouchEvent& event);
    bool IsAnimating();

    bool DrawFrame();
    bool Tick(const uint64_t aTotalTime, const uint64_t aDeltaTime);
    bool AddGeometries(const std::vector<OHOS::Ace::RefPtr<SVGeometry>>& shapes);
    bool UpdateGLTFAnimations(const std::vector<OHOS::Ace::RefPtr<GLTFAnimation>>& animations);

private:
    std::unique_ptr<IEngine> engine_ = nullptr;
#if MULTI_ECS_UPDATE_AT_ONCE
    bool firstFrame_ = true;
#endif
    uint32_t key_;
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_SCENE_VIEW_ADAPTER_H
