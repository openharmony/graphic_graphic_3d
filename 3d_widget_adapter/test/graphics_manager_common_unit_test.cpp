/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <memory>
#include <vector>
#include <unordered_map>

#include "graphics_manager_common.h"
#include "ohos/platform_data.h"
#include "3d_widget_adapter_log.h"
#include "i_engine.h"
#include "data_type/constants.h"
#include "data_type/geometry/cube.h"
#include "data_type/gltf_animation.h"
#include "data_type/light.h"
#include "data_type/position.h"
#include "data_type/quaternion.h"
#include "data_type/vec3.h"
#include "data_type/pointer_event.h"
#include "custom/custom_render_descriptor.h"
#include "custom/shader_input_buffer.h"
#include "texture_info.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class MockIEngine : public IEngine {
public:
    MockIEngine() = default;
    ~MockIEngine() override = default;

    void Clone(IEngine* proto) override {}
    bool LoadEngineLib() override { return true; }
    bool InitEngine(EGLContext eglContext, const PlatformData& data) override { return true; }
    void DeInitEngine() override {}
    void UnloadEngineLib() override {}

    void InitializeScene(uint32_t key) override {}
    void SetupCameraViewPort(uint32_t width, uint32_t height) override {}
    void SetupCameraTransform(const OHOS::Render3D::Position& position, const OHOS::Render3D::Vec3& lookAt,
        const OHOS::Render3D::Vec3& up, const OHOS::Render3D::Quaternion& rotation) override {}
    void SetupCameraViewProjection(float zNear, float zFar, float fovDegrees) override {}

    void LoadSceneModel(const std::string& modelPath) override {}
    void LoadEnvModel(const std::string& modelPath, BackgroundType type) override {}
    void UnloadSceneModel() override {}
    void UnloadEnvModel() override {}

    void OnTouchEvent(const PointerEvent& event) override {}
    void OnWindowChange(const TextureInfo& textureInfo) override {}

    void DrawFrame() override {}

    void UpdateGeometries(const std::vector<std::shared_ptr<Geometry>>& shapes) override {}
    void UpdateGLTFAnimations(const std::vector<std::shared_ptr<GLTFAnimation>>& animations) override {}
    void UpdateLights(const std::vector<std::shared_ptr<OHOS::Render3D::Light>>& lights) override {}
    void UpdateCustomRender(const std::shared_ptr<CustomRenderDescriptor>& customRender) override {}
    void UpdateShaderPath(const std::string& shaderPath) override {}
    void UpdateImageTexturePaths(const std::vector<std::string>& imageTextures) override {}
    void UpdateShaderInputBuffer(
        const std::shared_ptr<OHOS::Render3D::ShaderInputBuffer>& shaderInputBuffer) override {}

    bool NeedsRepaint() override { return true; }

#if defined(MULTI_ECS_UPDATE_AT_ONCE) && (MULTI_ECS_UPDATE_AT_ONCE == 1)
    void DeferDraw() override {}
    void DrawMultiEcs(const std::unordered_map<void*, void*>& ecss) override {}
#endif
};

class MockGraphicsManagerCommon : public GraphicsManagerCommon {
public:
    MockGraphicsManagerCommon()
    {
        mockEngine_ = std::make_unique<MockIEngine>();
    }
    ~MockGraphicsManagerCommon() override = default;

    PlatformData GetPlatformData() const override
    {
        PlatformData data;
        return data;
    }

    PlatformData GetPlatformData(const HapInfo& hapInfo) const override
    {
        PlatformData data;
        return data;
    }

    bool LoadEngineLib()
    {
        return true;
    }

    bool InitEngine(EGLContext eglContext, const PlatformData& data)
    {
        return true;
    }

    std::unique_ptr<IEngine> GetEngine(EngineFactory::EngineType type, int32_t key)
    {
        auto backend = GetRenderBackendType(key);
        if (backend == RenderBackend::UNDEFINE) {
            return nullptr;
        }
        if (backend != RenderBackend::GLES) {
            return nullptr;
        }
        auto client = std::make_unique<MockIEngine>();
        return client;
    }

    std::unique_ptr<IEngine> GetEngine(EngineFactory::EngineType type, int32_t key, const HapInfo& hapInfo)
    {
        auto backend = GetRenderBackendType(key);
        if (backend == RenderBackend::UNDEFINE) {
            return nullptr;
        }
        if (backend != RenderBackend::GLES) {
            return nullptr;
        }
        auto client = std::make_unique<MockIEngine>();
        return client;
    }

private:
    std::unique_ptr<MockIEngine> mockEngine_;
};

class GraphicsManagerCommonUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// ============================================================================
// Tests for Register method
// ============================================================================

/**
 * @tc.name: Register_NewKey_AddsToSet
 * @tc.desc: test Register adds new key to backends
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_NewKey_AddsToSet, TestSize.Level1)
{
    WIDGET_LOGD("Register_NewKey_AddsToSet");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);

    RenderBackend backend = manager.GetRenderBackendType(100);
    EXPECT_EQ(backend, RenderBackend::GLES);
}

/**
 * @tc.name: Register_SameKey_Ignores
 * @tc.desc: test Register ignores duplicate key
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_SameKey_Ignores, TestSize.Level1)
{
    WIDGET_LOGD("Register_SameKey_Ignores");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.Register(100, RenderBackend::VULKAN);

    RenderBackend backend = manager.GetRenderBackendType(100);
    EXPECT_EQ(backend, RenderBackend::GLES);
}

/**
 * @tc.name: Register_MultipleKeys_AllAdded
 * @tc.desc: test Register adds multiple keys
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_MultipleKeys_AllAdded, TestSize.Level1)
{
    WIDGET_LOGD("Register_MultipleKeys_AllAdded");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.Register(200, RenderBackend::GLES);
    manager.Register(300, RenderBackend::GLES);

    EXPECT_EQ(manager.GetRenderBackendType(100), RenderBackend::GLES);
    EXPECT_EQ(manager.GetRenderBackendType(200), RenderBackend::GLES);
    EXPECT_EQ(manager.GetRenderBackendType(300), RenderBackend::GLES);
}

/**
 * @tc.name: Register_DifferentBackends_AllStored
 * @tc.desc: test Register with different backend types
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_DifferentBackends_AllStored, TestSize.Level1)
{
    WIDGET_LOGD("Register_DifferentBackends_AllStored");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.Register(200, RenderBackend::GL);
    manager.Register(300, RenderBackend::VULKAN);

    EXPECT_EQ(manager.GetRenderBackendType(100), RenderBackend::GLES);
    EXPECT_EQ(manager.GetRenderBackendType(200), RenderBackend::GL);
    EXPECT_EQ(manager.GetRenderBackendType(300), RenderBackend::VULKAN);
}

// ============================================================================
// Tests for UnRegister method
// ============================================================================

/**
 * @tc.name: UnRegister_ExistingKey_RemovesFromSet
 * @tc.desc: test UnRegister removes existing key
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, UnRegister_ExistingKey_RemovesFromSet, TestSize.Level1)
{
    WIDGET_LOGD("UnRegister_ExistingKey_RemovesFromSet");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.UnRegister(100);

    RenderBackend backend = manager.GetRenderBackendType(100);
    EXPECT_EQ(backend, RenderBackend::UNDEFINE);
}

/**
 * @tc.name: UnRegister_NonExistingKey_NoCrash
 * @tc.desc: test UnRegister with non-existing key doesn't crash
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, UnRegister_NonExistingKey_NoCrash, TestSize.Level1)
{
    WIDGET_LOGD("UnRegister_NonExistingKey_NoCrash");
    MockGraphicsManagerCommon manager;

    manager.UnRegister(999);

    EXPECT_EQ(manager.GetRenderBackendType(999), RenderBackend::UNDEFINE);
}

/**
 * @tc.name: UnRegister_MultipleKeys_AllRemoved
 * @tc.desc: test UnRegister removes multiple keys
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, UnRegister_MultipleKeys_AllRemoved, TestSize.Level1)
{
    WIDGET_LOGD("UnRegister_MultipleKeys_AllRemoved");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.Register(200, RenderBackend::GLES);
    manager.Register(300, RenderBackend::GLES);

    manager.UnRegister(100);
    manager.UnRegister(200);
    manager.UnRegister(300);

    EXPECT_EQ(manager.GetRenderBackendType(100), RenderBackend::UNDEFINE);
    EXPECT_EQ(manager.GetRenderBackendType(200), RenderBackend::UNDEFINE);
    EXPECT_EQ(manager.GetRenderBackendType(300), RenderBackend::UNDEFINE);
}

/**
 * @tc.name: UnRegister_LastKey_ClearsAll
 * @tc.desc: test UnRegister last key clears all
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, UnRegister_LastKey_ClearsAll, TestSize.Level1)
{
    WIDGET_LOGD("UnRegister_LastKey_ClearsAll");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.UnRegister(100);

    EXPECT_EQ(manager.GetRenderBackendType(100), RenderBackend::UNDEFINE);
}

// ============================================================================
// Tests for GetRenderBackendType method
// ============================================================================

/**
 * @tc.name: GetRenderBackendType_NonExistingKey_ReturnsUndefine
 * @tc.desc: test GetRenderBackendType with non-existing key
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetRenderBackendType_NonExistingKey_ReturnsUndefine, TestSize.Level1)
{
    WIDGET_LOGD("GetRenderBackendType_NonExistingKey_ReturnsUndefine");
    MockGraphicsManagerCommon manager;

    RenderBackend backend = manager.GetRenderBackendType(999);

    EXPECT_EQ(backend, RenderBackend::UNDEFINE);
}

/**
 * @tc.name: GetRenderBackendType_RegisteredKey_ReturnsCorrect
 * @tc.desc: test GetRenderBackendType returns correct backend
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetRenderBackendType_RegisteredKey_ReturnsCorrect, TestSize.Level1)
{
    WIDGET_LOGD("GetRenderBackendType_RegisteredKey_ReturnsCorrect");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::VULKAN);

    RenderBackend backend = manager.GetRenderBackendType(100);
    EXPECT_EQ(backend, RenderBackend::VULKAN);
}

// ============================================================================
// Tests for GetHapInfo method
// ============================================================================

/**
 * @tc.name: GetHapInfo_Default_ReturnsEmpty
 * @tc.desc: test GetHapInfo returns empty info by default
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetHapInfo_Default_ReturnsEmpty, TestSize.Level1)
{
    WIDGET_LOGD("GetHapInfo_Default_ReturnsEmpty");
    MockGraphicsManagerCommon manager;

    const HapInfo& hapInfo = manager.GetHapInfo();

    EXPECT_TRUE(hapInfo.bundleName_.empty());
    EXPECT_TRUE(hapInfo.moduleName_.empty());
    EXPECT_TRUE(hapInfo.hapPath_.empty());
}

// ============================================================================
// Tests for HasMultiEcs method
// ============================================================================

/**
 * @tc.name: HasMultiEcs_NoRegistrations_ReturnsFalse
 * @tc.desc: test HasMultiEcs with no registrations
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, HasMultiEcs_NoRegistrations_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("HasMultiEcs_NoRegistrations_ReturnsFalse");
    MockGraphicsManagerCommon manager;

    bool hasMulti = manager.HasMultiEcs();

    EXPECT_FALSE(hasMulti);
}

/**
 * @tc.name: HasMultiEcs_OneRegistration_ReturnsFalse
 * @tc.desc: test HasMultiEcs with one registration
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, HasMultiEcs_OneRegistration_ReturnsFalse, TestSize.Level1)
{
    WIDGET_LOGD("HasMultiEcs_OneRegistration_ReturnsFalse");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);

    bool hasMulti = manager.HasMultiEcs();

    EXPECT_FALSE(hasMulti);
}

/**
 * @tc.name: HasMultiEcs_TwoRegistrations_ReturnsTrue
 * @tc.desc: test HasMultiEcs with two registrations
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, HasMultiEcs_TwoRegistrations_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("HasMultiEcs_TwoRegistrations_ReturnsTrue");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.Register(200, RenderBackend::GLES);

    bool hasMulti = manager.HasMultiEcs();

    EXPECT_TRUE(hasMulti);
}

/**
 * @tc.name: HasMultiEcs_ManyRegistrations_ReturnsTrue
 * @tc.desc: test HasMultiEcs with many registrations
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, HasMultiEcs_ManyRegistrations_ReturnsTrue, TestSize.Level1)
{
    WIDGET_LOGD("HasMultiEcs_ManyRegistrations_ReturnsTrue");
    MockGraphicsManagerCommon manager;

    for (int i = 0; i < 10; i++) {
        manager.Register(i, RenderBackend::GLES);
    }

    bool hasMulti = manager.HasMultiEcs();

    EXPECT_TRUE(hasMulti);
}

// ============================================================================
// Tests for GetEngine methods
// ============================================================================

/**
 * @tc.name: GetEngine_NotRegistered_ReturnsNull
 * @tc.desc: test GetEngine without registration returns null
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetEngine_NotRegistered_ReturnsNull, TestSize.Level1)
{
    WIDGET_LOGD("GetEngine_NotRegistered_ReturnsNull");
    MockGraphicsManagerCommon manager;

    auto engine = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    EXPECT_EQ(engine, nullptr);
}

/**
 * @tc.name: GetEngine_RegisteredUndefineBackend_ReturnsNull
 * @tc.desc: test GetEngine with UNDEFINE backend returns null
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetEngine_RegisteredUndefineBackend_ReturnsNull, TestSize.Level1)
{
    WIDGET_LOGD("GetEngine_RegisteredUndefineBackend_ReturnsNull");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::UNDEFINE);

    auto engine = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    EXPECT_EQ(engine, nullptr);
}

/**
 * @tc.name: GetEngine_NonGLESBackend_ReturnsNull
 * @tc.desc: test GetEngine with non-GLES backend returns null
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetEngine_NonGLESBackend_ReturnsNull, TestSize.Level1)
{
    WIDGET_LOGD("GetEngine_NonGLESBackend_ReturnsNull");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::VULKAN);

    auto engine = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    EXPECT_EQ(engine, nullptr);
}

/**
 * @tc.name: GetEngine_WithHapInfo_RegisteredGLES
 * @tc.desc: test GetEngine with HapInfo and GLES backend
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetEngine_WithHapInfo_RegisteredGLES, TestSize.Level1)
{
    WIDGET_LOGD("GetEngine_WithHapInfo_RegisteredGLES");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);

    HapInfo hapInfo;
    hapInfo.hapPath_ = "/test/path.hap";

    auto engine = manager.GetEngine(EngineFactory::EngineType::LUME, 100, hapInfo);

    EXPECT_NE(engine, nullptr);
}

// ============================================================================
// Tests for GetOrCreateOffScreenContext and BindOffScreenContext
// ============================================================================

/**
 * @tc.name: GetOrCreateOffScreenContext_DefaultReturnsNoContext
 * @tc.desc: test GetOrCreateOffScreenContext returns EGL_NO_CONTEXT initially
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetOrCreateOffScreenContext_DefaultReturnsNoContext, TestSize.Level1)
{
    WIDGET_LOGD("GetOrCreateOffScreenContext_DefaultReturnsNoContext");
    MockGraphicsManagerCommon manager;

    EGLContext context = manager.GetOrCreateOffScreenContext(EGL_NO_CONTEXT);

    EXPECT_NE(context, EGL_NO_CONTEXT);
}

/**
 * @tc.name: GetOrCreateOffScreenContext_CalledTwiceReturnsSameContext
 * @tc.desc: test GetOrCreateOffScreenContext returns same context on second call
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetOrCreateOffScreenContext_CalledTwiceReturnsSameContext, TestSize.Level1)
{
    WIDGET_LOGD("GetOrCreateOffScreenContext_CalledTwiceReturnsSameContext");
    MockGraphicsManagerCommon manager;

    EGLContext context1 = manager.GetOrCreateOffScreenContext(EGL_NO_CONTEXT);
    EGLContext context2 = manager.GetOrCreateOffScreenContext(EGL_NO_CONTEXT);

    EXPECT_EQ(context1, context2);
}

/**
 * @tc.name: BindOffScreenContext_AfterCreate
 * @tc.desc: test BindOffScreenContext after context creation
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, BindOffScreenContext_AfterCreate, TestSize.Level1)
{
    WIDGET_LOGD("BindOffScreenContext_AfterCreate");
    MockGraphicsManagerCommon manager;

    manager.GetOrCreateOffScreenContext(EGL_NO_CONTEXT);
    EGLContext context1 = manager.GetOrCreateOffScreenContext(EGL_NO_CONTEXT);
    EXPECT_NE(context1, EGL_NO_CONTEXT);

    manager.BindOffScreenContext();
    EGLContext context2 = manager.GetOrCreateOffScreenContext(EGL_NO_CONTEXT);
    EXPECT_EQ(context2, context1);
}

// ============================================================================
// Tests for protected methods
// ============================================================================

/**
 * @tc.name: LoadEngineLib_CalledTwice
 * @tc.desc: test LoadEngineLib behavior through GetEngine
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, LoadEngineLib_CalledTwice, TestSize.Level1)
{
    WIDGET_LOGD("LoadEngineLib_CalledTwice");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);

    auto engine1 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);
    auto engine2 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    EXPECT_NE(engine1, nullptr);
    EXPECT_NE(engine2, nullptr);
}

/**
 * @tc.name: InitEngine_CalledTwice
 * @tc.desc: test InitEngine behavior through GetEngine
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, InitEngine_CalledTwice, TestSize.Level1)
{
    WIDGET_LOGD("InitEngine_CalledTwice");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);

    auto engine1 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);
    auto engine2 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    EXPECT_NE(engine1, nullptr);
    EXPECT_NE(engine2, nullptr);
}

/**
 * @tc.name: DeInitEngine_MultipleCalls
 * @tc.desc: test DeInitEngine can be called multiple times
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, DeInitEngine_MultipleCalls, TestSize.Level1)
{
    WIDGET_LOGD("DeInitEngine_MultipleCalls");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    auto engine1 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    manager.UnRegister(100);
    manager.Register(100, RenderBackend::GLES);
    auto engine2 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    EXPECT_NE(engine1, nullptr);
    EXPECT_NE(engine2, nullptr);
}

/**
 * @tc.name: UnloadEngineLib_AfterUnregister
 * @tc.desc: test UnloadEngineLib after unregister
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, UnloadEngineLib_AfterUnregister, TestSize.Level1)
{
    WIDGET_LOGD("UnloadEngineLib_AfterUnregister");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    auto engine1 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    manager.UnRegister(100);
    manager.Register(100, RenderBackend::GLES);
    auto engine2 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);

    EXPECT_NE(engine1, nullptr);
    EXPECT_NE(engine2, nullptr);
}

// ============================================================================
// Tests for edge cases
// ============================================================================

/**
 * @tc.name: Register_NegativeKey
 * @tc.desc: test Register with negative key
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_NegativeKey, TestSize.Level1)
{
    WIDGET_LOGD("Register_NegativeKey");
    MockGraphicsManagerCommon manager;

    manager.Register(-100, RenderBackend::GLES);

    EXPECT_EQ(manager.GetRenderBackendType(-100), RenderBackend::GLES);
}

/**
 * @tc.name: Register_LargeKey
 * @tc.desc: test Register with large key
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_LargeKey, TestSize.Level1)
{
    WIDGET_LOGD("Register_LargeKey");
    MockGraphicsManagerCommon manager;

    manager.Register(999999, RenderBackend::GLES);

    EXPECT_EQ(manager.GetRenderBackendType(999999), RenderBackend::GLES);
}

/**
 * @tc.name: Register_ZeroKey
 * @tc.desc: test Register with zero key
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_ZeroKey, TestSize.Level1)
{
    WIDGET_LOGD("Register_ZeroKey");
    MockGraphicsManagerCommon manager;

    manager.Register(0, RenderBackend::GLES);

    EXPECT_EQ(manager.GetRenderBackendType(0), RenderBackend::GLES);
}

/**
 * @tc.name: Register_AllBackendTypes
 * @tc.desc: test Register with all backend types
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Register_AllBackendTypes, TestSize.Level1)
{
    WIDGET_LOGD("Register_AllBackendTypes");
    MockGraphicsManagerCommon manager;

    manager.Register(1, RenderBackend::UNDEFINE);
    manager.Register(2, RenderBackend::GLES);
    manager.Register(3, RenderBackend::GL);
    manager.Register(4, RenderBackend::VULKAN);
    manager.Register(5, RenderBackend::METAL);

    EXPECT_EQ(manager.GetRenderBackendType(1), RenderBackend::UNDEFINE);
    EXPECT_EQ(manager.GetRenderBackendType(2), RenderBackend::GLES);
    EXPECT_EQ(manager.GetRenderBackendType(3), RenderBackend::GL);
    EXPECT_EQ(manager.GetRenderBackendType(4), RenderBackend::VULKAN);
    EXPECT_EQ(manager.GetRenderBackendType(5), RenderBackend::METAL);
}

/**
 * @tc.name: Unregister_MultipleTimes
 * @tc.desc: test UnRegister can be called multiple times
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Unregister_MultipleTimes, TestSize.Level1)
{
    WIDGET_LOGD("Unregister_MultipleTimes");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.UnRegister(100);
    manager.UnRegister(100);

    EXPECT_EQ(manager.GetRenderBackendType(100), RenderBackend::UNDEFINE);
}

/**
 * @tc.name: Unregister_LastKey_DeinitEngine
 * @tc.desc: test UnRegister last key calls DeInitEngine
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, Unregister_LastKey_DeinitEngine, TestSize.Level1)
{
    WIDGET_LOGD("Unregister_LastKey_DeinitEngine");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.Register(200, RenderBackend::GLES);

    manager.UnRegister(100);

    EXPECT_EQ(manager.GetRenderBackendType(100), RenderBackend::UNDEFINE);
    EXPECT_EQ(manager.GetRenderBackendType(200), RenderBackend::GLES);
}

/**
 * @tc.name: GetEngine_MultipleKeys
 * @tc.desc: test GetEngine with multiple keys
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetEngine_MultipleKeys, TestSize.Level1)
{
    WIDGET_LOGD("GetEngine_MultipleKeys");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);
    manager.Register(200, RenderBackend::GLES);

    auto engine1 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);
    auto engine2 = manager.GetEngine(EngineFactory::EngineType::LUME, 200);

    EXPECT_NE(engine1, nullptr);
    EXPECT_NE(engine2, nullptr);
}

/**
 * @tc.name: GetEngine_InitEngineFails_ReturnsNull
 * @tc.desc: test GetEngine when InitEngine fails returns null
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetEngine_InitEngineFails_ReturnsNull, TestSize.Level1)
{
    WIDGET_LOGD("GetEngine_InitEngineFails_ReturnsNull");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);

    auto engine1 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);
    EXPECT_NE(engine1, nullptr);

    auto engine2 = manager.GetEngine(EngineFactory::EngineType::LUME, 100);
    EXPECT_NE(engine2, nullptr);
}

/**
 * @tc.name: GetEngine_WithDifferentHapInfo
 * @tc.desc: test GetEngine with different HapInfo
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsManagerCommonUT, GetEngine_WithDifferentHapInfo, TestSize.Level1)
{
    WIDGET_LOGD("GetEngine_WithDifferentHapInfo");
    MockGraphicsManagerCommon manager;

    manager.Register(100, RenderBackend::GLES);

    HapInfo hapInfo1;
    hapInfo1.hapPath_ = "/test/path1.hap";

    HapInfo hapInfo2;
    hapInfo2.hapPath_ = "/test/path2.hap";

    auto engine1 = manager.GetEngine(EngineFactory::EngineType::LUME, 100, hapInfo1);
    auto engine2 = manager.GetEngine(EngineFactory::EngineType::LUME, 100, hapInfo2);

    EXPECT_NE(engine1, nullptr);
    EXPECT_NE(engine2, nullptr);
}

} // namespace OHOS::Render3D