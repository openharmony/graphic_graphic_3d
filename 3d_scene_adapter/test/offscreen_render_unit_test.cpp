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

// for buffer queue
#include <surface_buffer.h>
#include <surface_utils.h>
#include <native_buffer.h>
#include <iconsumer_surface.h>
#include <window.h>

// render source
#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_render_resource.h>

// widget adapter
#include "texture_info.h"
#include "ohos/3d_widget_adapter_log.h" // for log
// scene adapter
#include "scene_adapter/intf_scene_adapter.h"
#include "scene_adapter/intf_offscreen_scene.h"

// scene pointer
#include <meta/interface/intf_object.h>
#include <scene/interface/intf_scene.h>

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class OffscreenRenderUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

static constexpr BASE_NS::Uid UID_DOTFIELD_PLUGIN { "8430021d-a57f-d9ff-7770-c24fb3f6321c" };
static uint32_t g_width = 512;
static uint32_t g_height = 512;

using CameraIntrinsics = OHOS::Render3D::OffscreenCameraIntrinsics;
using CameraConfigs = OHOS::Render3D::OffscreenCameraConfigs;
using OHOS::Render3D::IOffScreenScene;

// if we want to test the codes for attaching swapchain,
// we must register a consumer to release the buffer
// we thus provide this class for utility

class OffscreenRenderSession : public OHOS::Rosen::IConsumerListenerClazz {
public:
    void OnBufferAvailable() override
    {
        EXPECT_NE(consumerSurface_, nullptr);
        OHOS::sptr<OHOS::SurfaceBuffer> buffer;
        int32_t fence = -1;
        int64_t timestamp = 0;
        OHOS::Rect damage;

        // get buffer
        auto success = consumerSurface_->AcquireBuffer(buffer, fence, timestamp, damage);
        EXPECT_NE(buffer, nullptr);
        EXPECT_EQ(success, OHOS::SURFACE_ERROR_OK);
        
        OH_NativeBuffer *nativeBuffer = buffer->SurfaceBufferToNativaBuffer(); // GPU buffer
        EXPECT_NE(nativeBuffer, nullptr);

        consumerSurface_->ReleaseBuffer(buffer, fence);
    }
    OHOS::sptr<OHOS::IConsumerSurface> consumerSurface_;
    OHOS::sptr<OHOS::ISurface> producerSurface_;

    void RenderFrame(int i)
    {
        CameraIntrinsics intr{
            0.3,    // fov
            0.05,   // near
            10      // far
        };
        camConfigs_.position_ = OHOS::Render3D::Vector3f({0.1 * i, -1, 2});
        
        camConfigs_.rotation_ = OHOS::Render3D::Vector4f({0.0, 0.0, 0.0, 1});
        camConfigs_.intrinsics_ = intr;
        bool ret = offscreenScene_->SetCameraConfigs(camConfigs_);
        EXPECT_TRUE(ret);
        offscreenScene_->RenderFrame();
    }

    void ConfigRender()
    {
        consumerSurface_ = OHOS::IConsumerSurface::Create("OffscreenRenderSurfaceConsumer");
        EXPECT_NE(consumerSurface_, nullptr);

        auto producer = consumerSurface_->GetProducer();
        EXPECT_NE(producer, nullptr);

        auto producerSurface_ = OHOS::Surface::CreateSurfaceAsProducer(producer);
        auto utils = OHOS::SurfaceUtils::GetInstance();

        EXPECT_NE(utils, nullptr);

        this->offscreenBufferWidth_ = g_width;
        this->offscreenBufferHeight_ = g_height;

        // config buffer
        producerSurface_->SetQueueSize(5);
        producerSurface_->SetUserData("SURFACE_STRIDE_ALIGNMENT", "8");
        producerSurface_->SetUserData("SURFACE_FORMAT", std::to_string(GRAPHIC_PIXEL_FMT_RGBA_8888));
        producerSurface_->SetUserData("SURFACE_WIDTH", std::to_string(offscreenBufferWidth_));
        producerSurface_->SetUserData("SURFACE_HEIGHT", std::to_string(offscreenBufferHeight_));

        utils->Add(producerSurface_->GetUniqueId(), producerSurface_);

        consumerSurface_->RegisterConsumerListener(this);
        this->producerSurfaceId_ = producerSurface_->GetUniqueId();

        // pass to AGP
        OHOS::Render3D::WindowChangeInfo info;
        info.width = static_cast<float>(offscreenBufferWidth_);
        info.height = static_cast<float>(offscreenBufferHeight_);

        int.producerSurfaceId = this->producerSurfaceId_;

        CameraIntrinsics intr{
            1,          // fov
            0.0001,     // near
            10000       // far
        };
        
        camConfigs_.position_ = OHOS::Render3D::Vector3f({-0.5, 2, 1});
        camConfigs_.rotation_ = OHOS::Render3D::Vector4f({0.0, 0.0, 0.0, 1});
        camConfigs_.intrinsics_ = intr;

        std::string str{UID_DOTFIELD_PLUGIN};
        offscreenScene_->LoadPluginByUid(str);
        offscreenScene_->CreateCamera(camConfigs_);

        offscreenScene_->OnWindowChange(info);

    }

    uint64_t producerSurfaceId_ = 0;
    uint32_t offscreenBufferWidth_ = 0;
    uint32_t offscreenBufferHeight_ = 0;

    OHOS::Render3D::OffscreenCameraConfigs camConfigs_;
    BASE_NS::shared_ptr<IOffScreenScene> offscreenScene_;

    OffscreenRenderSession()
    {
        offscreenScene_ = OHOS::Render3D::GetOffscreenSceneInstance();
    }
    ~OffscreenRenderSession()
    {
        offscreenScene_->Deinit(true);
    }
};

/**
 * @tc.name: LoadPluginsAndInit001
 * @tc.desc: test LoadPluginsAndInit001
 * @tc.type: FUNC
 */
HWTEST_F(OffscreenRenderUT, LoadPluginByUid, TestSize.Level1)
{
    auto offScreenScene = GetOffscreenSceneInstance();
    EXPECT_NE(offScreenScene, nullptr);

    std::string uid = "invalid UID";
    EXPECT_FALSE(offScreenScene->LoadPluginByUid(uid));
    std::string uid2{UID_DOTFIELD_PLUGIN}; // a valid uid
    EXPECT_TRUE(offScreenScene->LoadPluginByUid(uid2));
}

/**
 * @tc.name: CameraConfig
 * @tc.desc: Verify CreateCamera and SetCameraConfig and EngineTickFrame
 * @tc.type: FUNC
 */
HWTEST_F(OffscreenRenderUT, CameraConfig, TestSize.Level1)
{
    auto offScreenScene = GetOffscreenSceneInstance();
    EXPECT_NE(offScreenScene, nullptr);

    std::string uid{UID_DOTFIELD_PLUGIN};
    bool ret = offScreenScene->LoadPluginByUid(uid);
    EXPECT_TRUE(ret);
    EXPECT_EQ(offScreenScene->GetCamera(), nullptr);
    OffscreenCameraConfigs p;
    
    bool ret2 = offScreenScene->SetCameraConfigs(p);
    EXPECT_FALSE(ret2);
    offScreenScene->CreateCamera(p);
    EXPECT_NE(offScreenScene->GetCamera(), nullptr);
    auto far1 = offScreenScene->GetCamera()->FarPlane()->GetValue();

    OffscreenCameraConfigs p2;
    p2.intrinsics_.far_ = 10000;
    offScreenScene->SetCameraConfigs(p2);
    auto far2 = offScreenScene->GetCamera()->FarPlane()->GetValue();
    EXPECT_NE(far1, far2);

    bool ret3 = offScreenScene->EngineTickFrame(offScreenScene->GetEcs());
    EXPECT_TRUE(ret3);
}

/**
 * @tc.name: OffscreenRender
 * @tc.desc: Verify offscreen render with buffer queue
 * @tc.type: FUNC
 */
HWTEST_F(OffscreenRenderUT, OffscreenRender, TestSize.Level1)
{
    OffscreenRenderSession renderSession;
    renderSession.ConfigRender();

    const int frameCount = 5;
    for (int i = 0; i < frameCount; ++i) {
        renderSession.RenderFrame(i); // expect no crash inside
    }
}

} // namespace