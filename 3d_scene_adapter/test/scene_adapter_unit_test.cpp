/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <cmath>

#include "ohos/texture_layer.h"
#include "scene_adapter/scene_adapter.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {
static constexpr BASE_NS::Uid ENGINE_THREAD { "2070e705-d061-40e4-bfb7-90fad2c280af" };

class SceneAdapterUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase()
    {
        SceneAdapter::ShutdownPluginRegistry();
    }

    void SetUp() {}
    void TearDown() {}
};

class SceneAdapterTester : public SceneAdapter {
public:
    uint32_t GetKey();
    bool GetReceiveBuffer();
};

uint32_t SceneAdapterTester::GetKey()
{
    return key_;
}

bool SceneAdapterTester::GetReceiveBuffer()
{
    return receiveBuffer_;
}

namespace {
static WindowChangeInfo g_windowChangeInfo {
    .offsetX = 0.0,
    .offsetY = 0.0,
    .width = 1080.0,
    .height = 1920.0,
    .scale = 1.0,
    .widthScale = 0.6,
    .heightScale = 0.6,
    .recreateWindow =false,
    .surfaceType = SurfaceType::SURFACE_WINDOW,
    .transformType = 0
};

static constexpr float EPSILON = 0.0001f;

/**
 * @tc.name: LoadPluginsAndInit001
 * @tc.desc: test LoadPluginsAndInit001
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, LoadPluginsAndInit001, TestSize.Level1)
{
    WIDGET_LOGD("LoadPluginsAndInit001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: LoadPluginsAndInit002
 * @tc.desc: test LoadPluginsAndInit002
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, LoadPluginsAndInit002, TestSize.Level1)
{
    WIDGET_LOGD("LoadPluginsAndInit002");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: CreateTextureLayer001
 * @tc.desc: test CreateTextureLayer001
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, CreateTextureLayer001, TestSize.Level1)
{
    WIDGET_LOGD("CreateTextureLayer001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    auto textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);
    auto textureInfo = textureLayer->GetTextureInfo();
    ASSERT_EQ(textureInfo.width_, 0U);
    ASSERT_EQ(textureInfo.height_, 0U);
    ASSERT_EQ(textureInfo.textureId_, 0U);
    ASSERT_EQ(textureInfo.nativeWindow_, nullptr);
    ASSERT_EQ(textureInfo.widthScale_, 1.0f);
    ASSERT_EQ(textureInfo.heightScale_, 1.0f);
    ASSERT_EQ(textureInfo.customRatio_, 0.1f);
    ASSERT_EQ(textureInfo.recreateWindow, true);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: CreateTextureLayer002
 * @tc.desc: test CreateTextureLayer002
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, CreateTextureLayer002, TestSize.Level1)
{
    WIDGET_LOGD("CreateTextureLayer002");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    uint32_t key = adapter->GetKey();
    ASSERT_EQ(key, 0);
    auto textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);
    key = adapter->GetKey();
    ASSERT_EQ(key, 1);
    textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);
    key = adapter->GetKey();
    ASSERT_EQ(key, 2);
    textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);
    key = adapter->GetKey();
    ASSERT_EQ(key, 3);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: CreateTextureLayer003
 * @tc.desc: test CreateTextureLayer003
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, CreateTextureLayer003, TestSize.Level1)
{
    WIDGET_LOGD("CreateTextureLayer003");
    auto adapter = std::make_unique<SceneAdapterTester>();
    auto textureLayer = adapter->CreateTextureLayer();
    ASSERT_EQ(textureLayer, nullptr);
}

/**
 * @tc.name: OnWindowChange001
 * @tc.desc: test OnWindowChange001
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, OnWindowChange001, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    auto textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);
    adapter->OnWindowChange(g_windowChangeInfo);

    auto textureInfo = textureLayer->GetTextureInfo();
    ASSERT_LT(std::fabs(textureInfo.width_ - 1080.0), EPSILON);
    ASSERT_LT(std::fabs(textureInfo.height_ - 1920.0), EPSILON);
    ASSERT_LT(std::fabs(textureInfo.widthScale_ - 0.6), EPSILON);
    ASSERT_LT(std::fabs(textureInfo.heightScale_ - 0.6), EPSILON);
    ASSERT_EQ(textureInfo.recreateWindow, false);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: OnWindowChange002
 * @tc.desc: test OnWindowChange002
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, OnWindowChange002, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange002");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    adapter->OnWindowChange(g_windowChangeInfo);

    // ecpect no dump calling OnWindowChange
    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: OnWindowChange003
 * @tc.desc: test OnWindowChange003
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, OnWindowChange003, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange003");
    auto adapter = std::make_unique<SceneAdapterTester>();
    adapter->OnWindowChange(g_windowChangeInfo);
    // ecpect no dump calling OnWindowChange
}

/**
 * @tc.name: RenderFrame001
 * @tc.desc: test RenderFrame001
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, RenderFrame001, TestSize.Level1)
{
    WIDGET_LOGD("RenderFrame001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    
    adapter->RenderFrame();
    adapter->RenderFrame(true);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: RenderFrame002
 * @tc.desc: test RenderFrame002
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, RenderFrame002, TestSize.Level1)
{
    WIDGET_LOGD("RenderFrame002");
    auto adapter = std::make_unique<SceneAdapterTester>();
    adapter->RenderFrame();
    adapter->RenderFrame(true);
    // ecpect no dump calling RenderFrame
}

/**
 * @tc.name: Deinit001
 * @tc.desc: test Deinit001
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, Deinit001, TestSize.Level1)
{
    WIDGET_LOGD("Deinit001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    bool needsRepaint = adapter->NeedsRepaint();
    ASSERT_EQ(needsRepaint, true);
    adapter->Deinit();
    adapter->DeinitRenderThread();

    needsRepaint = adapter->NeedsRepaint();
    ASSERT_EQ(needsRepaint, false);
}

/**
 * @tc.name: NeedsRepaint001
 * @tc.desc: test NeedsRepaint001
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, NeedsRepaint001, TestSize.Level1)
{
    WIDGET_LOGD("NeedsRepaint001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool needsRepaint = adapter->NeedsRepaint();
    ASSERT_EQ(needsRepaint, true);

    adapter->SetNeedsRepaint(false);

    needsRepaint = adapter->NeedsRepaint();
    ASSERT_EQ(needsRepaint, false);
}

static int32_t  g_fd = 0;

void MyCallBack(int32_t fd)
{
    g_fd = fd;
}

static SurfaceBufferInfo g_surfaceBufferInfo {
    .sfBuffer_ = nullptr,
    .acquireFence_ = OHOS::SyncFence::INVALID_FENCE,
    .transformMatrix_ = { 16, 1.0 },
    .needsTrans_ = true,
    .fn_ = MyCallBack
};

/**
 * @tc.name: AcquireImage001
 * @tc.desc: test AcquireImage001
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, AcquireImage001, TestSize.Level1)
{
    WIDGET_LOGD("AcquireImage001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    auto syncCB = META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>([]() {
        WIDGET_LOGi("wait util async function finish");
        return META_NS::IAny::Ptr{};
    });

    bool receiveBuffer = adapter->GetReceiveBuffer();
    ASSERT_EQ(receiveBuffer, false);
    ASSERT_EQ(g_fd, 0);

    adapter->AcquireImage(g_surfaceBufferInfo);
    META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD)->AddWaitableTask(syncCB)->Wait();
    receiveBuffer = adapter->GetReceiveBuffer();
    ASSERT_EQ(receiveBuffer, true);

    adapter->RenderFrame(true);
    receiveBuffer = adapter->GetReceiveBuffer();

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: AcquireImage002
 * @tc.desc: test AcquireImage002
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, AcquireImage002, TestSize.Level1)
{
    WIDGET_LOGD("AcquireImage002");
    auto adapter = std::make_unique<SceneAdapterTester>();

    adapter->AcquireImage(g_surfaceBufferInfo);
    // ecpect no dump calling AcquireImage
}

/**
 * @tc.name: SetNeedsRepaint001
 * @tc.desc: test SetNeedsRepaint
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, SetNeedsRepaint001, TestSize.Level1)
{
    WIDGET_LOGD("SetNeedsRepaint001");
    auto adapter = std::make_unique<SceneAdapterTester>();

    adapter->SetNeedsRepaint(true);
    bool needsRepaint = adapter->NeedsRepaint();
    EXPECT_EQ(needsRepaint, true);

    adapter->SetNeedsRepaint(false);
    needsRepaint = adapter->NeedsRepaint();
    EXPECT_EQ(needsRepaint, false);
}

/**
 * @tc.name: GetEcs001
 * @tc.desc: test GetEcs returns null when no scene
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, GetEcs001, TestSize.Level1)
{
    WIDGET_LOGD("GetEcs001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    // Without CreateEmptyScene, GetEcs should return null
    auto ecs = adapter->GetEcs();
    EXPECT_EQ(ecs, nullptr);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: GetEcs002
 * @tc.desc: test GetEcs without init
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, GetEcs002, TestSize.Level1)
{
    WIDGET_LOGD("GetEcs002");
    auto adapter = std::make_unique<SceneAdapterTester>();

    // Without LoadPluginsAndInit, GetEcs should return null
    auto ecs = adapter->GetEcs();
    EXPECT_EQ(ecs, nullptr);
}

/**
 * @tc.name: LoadPluginsAndInit003
 * @tc.desc: test LoadPluginsAndInit multiple times
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, LoadPluginsAndInit003, TestSize.Level1)
{
    WIDGET_LOGD("LoadPluginsAndInit003");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    ret = adapter->LoadPluginsAndInit();
    EXPECT_EQ(ret, true);

    ret = adapter->LoadPluginsAndInit();
    EXPECT_EQ(ret, true);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: OnWindowChange004
 * @tc.desc: test OnWindowChange with recreateWindow
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, OnWindowChange004, TestSize.Level1)
{
    WIDGET_LOGD("OnWindowChange004");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    auto textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);

    WindowChangeInfo info;
    info.offsetX = 10.0;
    info.offsetY = 20.0;
    info.width = 1920.0;
    info.height = 1080.0;
    info.scale = 2.0;
    info.widthScale = 0.8;
    info.heightScale = 0.8;
    info.recreateWindow = true;
    info.surfaceType = SurfaceType::SURFACE_TEXTURE;
    info.transformType = 1;

    adapter->OnWindowChange(info);

    auto textureInfo = textureLayer->GetTextureInfo();
    EXPECT_LT(std::fabs(textureInfo.width_ - 1920.0), EPSILON);
    EXPECT_LT(std::fabs(textureInfo.height_ - 1080.0), EPSILON);
    EXPECT_LT(std::fabs(textureInfo.widthScale_ - 0.8), EPSILON);
    EXPECT_LT(std::fabs(textureInfo.heightScale_ - 0.8), EPSILON);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: Deinit002
 * @tc.desc: test Deinit without init
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, Deinit002, TestSize.Level1)
{
    WIDGET_LOGD("Deinit002");
    auto adapter = std::make_unique<SceneAdapterTester>();

    // Deinit without LoadPluginsAndInit
    adapter->Deinit();
    adapter->DeinitRenderThread();

    // GetEcs should return null after deinit without proper init
    auto ecs = adapter->GetEcs();
    EXPECT_EQ(ecs, nullptr);

    // GetSceneObj should return null
    auto sceneObj = adapter->GetSceneObj();
    EXPECT_EQ(sceneObj, nullptr);

    // NeedsRepaint should be false without proper init
    bool needsRepaint = adapter->NeedsRepaint();
    EXPECT_EQ(needsRepaint, true);
}

/**
 * @tc.name: GetSceneObj001
 * @tc.desc: test GetSceneObj returns null when no scene
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, GetSceneObj001, TestSize.Level1)
{
    WIDGET_LOGD("GetSceneObj001");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);

    // Without CreateEmptyScene, GetSceneObj should return null
    auto sceneObj = adapter->GetSceneObj();
    EXPECT_EQ(sceneObj, nullptr);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}

/**
 * @tc.name: GetSceneObj002
 * @tc.desc: test GetSceneObj without init
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, GetSceneObj002, TestSize.Level1)
{
    WIDGET_LOGD("GetSceneObj002");
    auto adapter = std::make_unique<SceneAdapterTester>();

    // Without LoadPluginsAndInit, GetSceneObj should return null
    auto sceneObj = adapter->GetSceneObj();
    EXPECT_EQ(sceneObj, nullptr);
}

/**
 * @tc.name: ShutdownPluginRegistry001
 * @tc.desc: test ShutdownPluginRegistry
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, ShutdownPluginRegistry001, TestSize.Level1)
{
    WIDGET_LOGD("ShutdownPluginRegistry001");

    // Initialize engine first
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    EXPECT_EQ(SceneAdapter::IsEngineInitSuccessful(), true);

    adapter->Deinit();
    adapter->DeinitRenderThread();

    // Shutdown multiple times - should handle gracefully
    SceneAdapter::ShutdownPluginRegistry();

    // After shutdown, engine init flag remains true (static flag not reset)
    EXPECT_EQ(SceneAdapter::IsEngineInitSuccessful(), true);

    // Shutdown again - should not crash
    SceneAdapter::ShutdownPluginRegistry();
    SceneAdapter::ShutdownPluginRegistry();

    // Engine init flag still true
    EXPECT_EQ(SceneAdapter::IsEngineInitSuccessful(), true);
}

/**
 * @tc.name: RenderFrame003
 * @tc.desc: test RenderFrame after Deinit
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, RenderFrame003, TestSize.Level1)
{
    WIDGET_LOGD("RenderFrame003");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    auto textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);
    adapter->OnWindowChange(g_windowChangeInfo);

    bool needsRepaint = adapter->NeedsRepaint();
    EXPECT_EQ(needsRepaint, true);

    adapter->Deinit();

    // After deinit, needsRepaint should be false
    needsRepaint = adapter->NeedsRepaint();
    EXPECT_EQ(needsRepaint, false);

    // Render after deinit - should handle gracefully
    adapter->RenderFrame();
    adapter->RenderFrame(true);

    // needsRepaint should still be false
    needsRepaint = adapter->NeedsRepaint();
    EXPECT_EQ(needsRepaint, false);
}

/**
 * @tc.name: AcquireImage003
 * @tc.desc: test AcquireImage with various buffer info
 * @tc.type: FUNC
 */
HWTEST_F(SceneAdapterUT, AcquireImage003, TestSize.Level1)
{
    WIDGET_LOGD("AcquireImage003");
    auto adapter = std::make_unique<SceneAdapterTester>();
    bool ret = adapter->LoadPluginsAndInit();
    ASSERT_EQ(ret, true);
    auto textureLayer = adapter->CreateTextureLayer();
    ASSERT_NE(textureLayer, nullptr);

    // Initially, should not have received buffer
    bool receiveBuffer = adapter->GetReceiveBuffer();
    EXPECT_EQ(receiveBuffer, false);

    SurfaceBufferInfo bufferInfo;
    bufferInfo.sfBuffer_ = nullptr;
    bufferInfo.acquireFence_ = nullptr;
    bufferInfo.needsTrans_ = false;
    bufferInfo.transformMatrix_ = {16, 1.0};
    bufferInfo.fn_ = nullptr;

    adapter->AcquireImage(bufferInfo);

    // AcquireImage with null buffer should be handled gracefully
    // The adapter should still be in a valid state
    auto ecs = adapter->GetEcs();
    EXPECT_EQ(ecs, nullptr);

    auto sceneObj = adapter->GetSceneObj();
    EXPECT_EQ(sceneObj, nullptr);

    adapter->Deinit();
    adapter->DeinitRenderThread();
}
} // namespace
}