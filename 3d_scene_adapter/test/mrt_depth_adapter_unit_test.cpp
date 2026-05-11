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
#include "scene_adapter/intf_mrt_depth_adapter.h"

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

static const std::string UID_MRT_PLUGIN { "a1b2c3d4-e5f6-7890-abcd-ef123456daca" };
static uint32_t g_width = 512;
static uint32_t g_height = 512;

using CameraIntrinsics = OHOS::Render3D::OffscreenCameraIntrinsics;
using CameraConfigs = OHOS::Render3D::OffscreenCameraConfigs;
using OHOS::Render3D::IMrtDepthAdapter;

// Khronos gltf sample with one triangle
// CC0 license and free to use
static const std::string g_GltfContent = "R(
{
  "scene" : 0,
  "scenes" : [
    {
      "nodes" : [ 0 ]
    }
  ],
  
  "nodes" : [
    {
      "mesh" : 0
    }
  ],
  
  "meshes" : [
    {
      "primitives" : [ {
        "attributes" : {
          "POSITION" : 1
        },
        "indices" : 0
      } ]
    }
  ],

  "buffers" : [
    {
      "uri" : "Triangle.bin",
      "byteLength" : 44
    }
  ],
  "bufferViews" : [
    {
      "buffer" : 0,
      "byteOffset" : 0,
      "byteLength" : 6,
      "target" : 34963
    },
    {
      "buffer" : 0,
      "byteOffset" : 8,
      "byteLength" : 36,
      "target" : 34962
    }
  ],
  "accessors" : [
    {
      "bufferView" : 0,
      "byteOffset" : 0,
      "componentType" : 5123,
      "count" : 3,
      "type" : "SCALAR",
      "max" : [ 2 ],
      "min" : [ 0 ]
    },
    {
      "bufferView" : 1,
      "byteOffset" : 0,
      "componentType" : 5126,
      "count" : 3,
      "type" : "VEC3",
      "max" : [ 1.0, 1.0, 0.0 ],
      "min" : [ 0.0, 0.0, 0.0 ]
    }
  ],
  
  "asset" : {
    "version" : "2.0"
  }
}
)";

static const std::string validUri = "/data/storage/very/long/file/path/to/uri/expect/no/duplicate/testModel.gltf";

// helper writing function

static bool writeGltfFile(const std::string& uri, const std::string& content)
{
    std::ofstream outFile(uri);
    outFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
    try {
      fs::path filePath(uri);
      fs::create_directories(filePath.parent_path());
      outFile.open(uri);
    } catch (std::exception& e) {
      WIDGET_LOGE("Failed to create file: %{public}s, error: %{public}s", uri.c_str(), e.what());
      return false;
    }

    outFile << content;
    outFile.close();
    if (!outFile) {
        return false;
    }  
    return true;
}

class BufferContextManager : public OHOS::Rosen::IConsumerListenerClazz {
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

  WindowChangeInfo CreateWindowChangeInfo(const std::string& sName = "consumerSurface") const
  {
    consumerSurface_ = OHOS::IConsumerSurface::Create(sName.c_str());
    EXPECT_NE(consumerSurface_, nullptr);

    auto producer = consumerSurface_->GetProducer();
    EXPECT_NE(producer, nullptr);
    auto producerSurface = OHOS::Surface::CreateSurfaceAsProducer(producer);
    auto utils = OHOS::SurfaceUtils::GetInstance();
    
    offscreenBufferHeight_ = g_height;
    offscreenBufferWidth_ = g_width;

    producerSurface->SetQueueSize(5);
    producerSurface->SetUserData("SURFACE_STRIDE_AIGNMENT", "8");
    producerSurface->SetUserData("SURFACE_FORMAT", std::to_string("OHOS::GRAPHIC_PIXEL_FMT_RGBA_8888"));
    producerSurface->SetUserData("SURFACE_WIDTH", std::to_string(offscreenBufferWidth_));
    producerSurface->SetUserData("SURFACE_HEIGHT", std::to_string(offscreenBufferHeight_));

    producerSurfaceId_ = producerSurface->GetUniqueId();
    utils->Add(producerSurfaceId_, producerSurface);
    consumerSurface_->RegisterConsumerListener(this);
    
    OHOS::Render3D::WindowChangeInfo info;
    info.width = static_cast<float>(offscreenBufferWidth_);
    info.height = static_cast<float>(offscreenBufferHeight_);

    info.producerSurfaceId = producerSurfaceId_;

    auto window = TextureLayer::SurfaceToNativeWindow(&producerSurface);

    info.customNativeWin = reinterpret_cast<void*>(window);
    window_ = window;
    return info;
  }

  uint64_t producerSurfaceId_ = 0;
  uint32_t offscreenBufferWidth_ = 0;
  uint32_t offscreenBufferHeight_ = 0;
  OHOS::sptr<OHOS::IConsumerSurface> consumerSurface_ = nullptr;
  OHOS::sptr<OHOS::ISurface> producerSurface_ = nullptr;

  void* window_ = nullptr;
};

class RenderSession {
public:
    RenderSession()
    {
      mrtScene_ = OHOS::Render3D::GetMrtDepthAdapterInstance();
      EXPECT_NE(mrtScene_, nullptr);
    }
    ~RenderSession()
    {
      if (mrtScene_) {
        mrtScene_->Deinit(true);
      }
    }
    Base::shared_ptr<IMrtDepthAdapter> mrtScene_ = nullptr;
    OHOS::Render3D::CameraConfigs camComfigs_ {};
    std::string gltfFilePath_ {};
    BufferContextManager rgbBufferCtx{};
    BufferContextManager depthBufferCtx{};

    void ConfigRender()
    {
      CameraIntrinsics intr {
        1, // fov
        0.0001, // near
        10000 //far
      };

      camComfigs_.position_ = OHOS::Render3D::Vector3f({-0.5, 2, 1});
      camComfigs_.rotation_ = OHOS::Render3D::Vector4f({1, 0, 0, 0});
      camComfigs_.intrinsics_ = intr;
      camConfigs_.clearColor_ = OHOS::Render3D::Vector4f({0, 1, 1, 1});
      auto info = rgbBufferCtx.CreateWindowChangeInfo("rgbBuffer");
      auto info2 = depthBufferCtx.CreateWindowChangeInfo("depthBuffer");
      std::vector<WindowChangeInfo> vInfo{};
      vInfo.push_back(info);
      vInfo.push_back(info2);

      mrtScene_->CreateSceneByGltfUri(gltfFilePath_);
      mrtScene_->OnWindowChange(vInfo);
    }

    void RenderFrame(int i)
    {
      camComfigs_.intrinsics_.position_ = OHOS::Render3D::Vector3f({0, 0, -4 + 0.02 * i});
      mrtScene_->SetCameraConfigs(camComfigs_);
      mrtScene_->RenderFrame();
    }
}


/**
 * @tc.name: CreateSceneByGltfUri
 * @tc.desc: test CreateSceneByGltfUri
 * @tc.type: FUNC
 */
HWTEST_F(MrtDepthAdapterUT, CreateSceneByGltfUri, TestSize.Level1)
{
    auto mrtScene = OHOS::Render3D::GetMrtDepthAdapterInstance();
    EXPECT_NE(mrtScene, nullptr);

    std::string uri = "invalidUri";

    mrtScene->CreateSceneByGltfUri(uri);
    EXPECT_FALSE(mrtScene->IsSceneValid());

    writeGltfFile(validUri, g_GltfContent);
    mrtScene->CreateSceneByGltfUri(validUri);
    EXPECT_TRUE(mrtScene->IsSceneValid());
    mrtScene->Deinit(true);
}

/**
 * @tc.name: bU
 * @tc.desc: test offscreen render with mrt depth adapter
 * @tc.type: FUNC
*/
HWTEST_F(MrtDepthAdapterUT, BufferQueueTest, TestSize.Level1)
{
  writeGltfFile(validUri, g_GltfContent);

  RenderSession session;
  session.gltfFilePath_ = validUri;
  session.ConfigRender();
  EXPECT_TRUE(session.mrtScene_->IsSceneValid());

  session.RenderFrame(0);
  static constexpr int TOTAL_FRAMES = 20;
  for (int i = 1; i <= TOTAL_FRAMES; ++i) {
    session.RenderFrame(i);
  }
  // expect no crash
}

} // namespace OHOS::Render3D