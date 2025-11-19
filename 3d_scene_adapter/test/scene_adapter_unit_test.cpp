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

} // namespace
}