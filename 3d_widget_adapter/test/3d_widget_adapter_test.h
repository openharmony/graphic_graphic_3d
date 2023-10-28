/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_3D_WIDGET_ADAPTER_TEST_H
#define OHOS_RENDER_3D_3D_WIDGET_ADAPTER_TEST_H

#include <gtest/gtest.h>
#include <memory>

#include "i_engine.h"
#include "scene_viewer_adapter.h"

namespace OHOS::Render3D {
class WidgetAdapter3DTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

class EngineTest : public IEngine {
public:
    EngineTest() = default;
    ~EngineTest() = default;
    void Clone(IEngine* proto) override {};
    bool LoadEngineLib() override;
    bool InitEngine(EGLContext eglContext, const PlatformData& data) override;
    void DeInitEngine() override {};
    void UnLoadEngineLib() override {};

    void CreateEcs(uint32_t key) override;
    void CreateScene() override {};
    void CreateCamera() override {};
    void SetUpPostprocess() override {};
    void LoadCustGeometry(std::vector<OHOS::Ace::RefPtr<SVGeometry>> &shapes) override;

    void SetUpCustomRenderTarget(const TextureInfo &info) override;
    void SetUpCameraViewPort(uint32_t width, uint32_t height) override;
    void SetUpCameraTransform(float position[], float rotationAngle, float rotationAxis[]) override;
    void SetUpCameraViewProjection(float zNear, float zFar, float fovDegrees) override;

    void CreateLight() override {};
    void SetLightProperties(int lightType, float color[], float intensity, bool shadow, float position[],
        float rotationAngle, float rotationAxis[]) override;

    void LoadSceneModel(std::string modelPath) override;
    void LoadBackgroundModel(std::string modelPath, SceneViewerBackgroundType type) override;
    void UnLoadModel() override {};

    void OnTouchEvent(const SceneViewerTouchEvent& event) override;
    bool IsAnimating() override;
    void DrawFrame() override {};
    void Tick(const uint64_t aTotalTime, const uint64_t aDeltaTime) override;
    bool HandlesNotReady() override;

    void UpdateGeometries(const std::vector<OHOS::Ace::RefPtr<SVGeometry>>& shapes) override;
    void UpdateGLTFAnimations(const std::vector<OHOS::Ace::RefPtr<GLTFAnimation>>& animations) override;
    void AddTextureMemoryBarrrier() override;
    void AddLights(const std::vector<OHOS::Ace::RefPtr<SVLight>>& lights) override;
    void AddCustomRenders(const std::vector<OHOS::Ace::RefPtr<SVCustomRenderDescriptor>>& customRenders) override;

#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    void DeferDraw() override {};
    void DrawMultiEcs(const std::vector<void *> &ecss) override {};
#endif
};
} // namespace OHOS::Render3D
#endif // OHOS_RENDER_3D_3D_WIDGET_ADAPTER_TEST_H
