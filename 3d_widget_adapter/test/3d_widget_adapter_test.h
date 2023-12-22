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
