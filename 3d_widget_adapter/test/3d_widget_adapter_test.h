/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef OHOS_RENDER_3D_3D_WIDGET_ADAPTER_TEST_H
#define OHOS_RENDER_3D_3D_WIDGET_ADAPTER_TEST_H

#include <gtest/gtest.h>
#include <memory>

#include "i_engine.h"
#include "widget_adapter.h"

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
    void Clone(IEngine *proto) override{};
    bool LoadEngineLib() override;
    bool InitEngine(EGLContext eglContext, const PlatformData &data) override;
    void DeInitEngine() override{};
    void UnloadEngineLib() override{};

    void InitializeScene(uint32_t key) override;
    void SetupCameraViewPort(uint32_t width, uint32_t height) override;
    void SetupCameraTransform(
        const Position &position, const Vec3 &lookAt, const Vec3 &up, const Quaternion &rotation) override;
    void SetupCameraViewProjection(float zNear, float zFar, float fovDegrees) override;

    void LoadSceneModel(const std::string &modelPath) override;
    void LoadEnvModel(const std::string &modelPath, BackgroundType type) override;
    void UnloadSceneModel() override{};
    void UnloadEnvModel() override{};

    void OnTouchEvent(const PointerEvent &event) override;
    void OnWindowChange(const TextureInfo &textureInfo) override;

    void DrawFrame() override{};

    void UpdateGeometries(const std::vector<std::shared_ptr<Geometry>> &shapes) override;
    void UpdateGLTFAnimations(const std::vector<std::shared_ptr<GLTFAnimation>> &animations) override;
    void UpdateLights(const std::vector<std::shared_ptr<Light>> &lights) override;
    void UpdateCustomRender(const std::shared_ptr<CustomRenderDescriptor> &customRender) override;
    void UpdateShaderPath(const std::string &shaderPath) override;
    void UpdateImageTexturePaths(const std::vector<std::string> &imageTextures) override;
    void UpdateShaderInputBuffer(const std::shared_ptr<ShaderInputBuffer> &shaderInputBuffer) override;

    bool NeedsRepaint() override;

#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    void DeferDraw() override{};
    void DrawMultiEcs(const std::unordered_map<void *, void *> &ecss) override{};
#endif
};
}  // namespace OHOS::Render3D
#endif  // OHOS_RENDER_3D_3D_WIDGET_ADAPTER_TEST_H
