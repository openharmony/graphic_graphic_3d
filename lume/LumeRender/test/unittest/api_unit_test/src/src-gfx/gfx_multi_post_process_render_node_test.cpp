/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <core/image/intf_image_loader_manager.h>
#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/util/intf_render_frame_util.h>
#include <render/util/intf_render_util.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace RENDER_NS;
using namespace CORE_NS;

#undef LoadImage
#define RENDER_SAVE_TEST_IMAGES 0

namespace {
static constexpr Math::UVec2 TEST_DATA_SIZE { 128u, 128u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(float);
float imageData[IMAGE_SIZE];
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
static constexpr string_view COPY_BUFFER_NAME_0 { "CopyBuffer0" };
static constexpr string_view HISTORY_IMAGE_NAME_0 { "HistoryImage0" };
static constexpr string_view DEPTH_IMAGE_NAME_0 { "DepthImage0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;
    RenderHandleReference copyBufferHandle0;
    RenderHandleReference historyImageHandle0;
    RenderHandleReference depthImageHandle0;

    unique_ptr<ByteArray> byteArray;

    std::vector<float> validateData;

    RenderHandleReference renderNodeGraph;

    refcnt_ptr<IRenderDataStorePod> dataStorePod;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

array_view<const uint8_t> CreateImageDataView()
{
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            const float u = float(j) / float(TEST_DATA_SIZE.x - 1u);
            const float v = float(i) / float(TEST_DATA_SIZE.y - 1u);

            float r = u;
            float g = v;
            float b = 0.f;
            float a = 1.f;

            // Checkerboard to test fxaa
            if (((i >> 5u) ^ (j >> 5u)) & 1u) {
                r = g = b = 1.0f;
            }
            // HDR patch to test tonemapping
            if (j > 48u && j < 80u && i > 48u && i < 80u) {
                r = g = b = 4.0f;
            }
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = r; // R
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = g; // G
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = b; // B
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = a; // A
        }
    }
#if RENDER_SAVE_TEST_IMAGES == 1
    UTest::SaveImage("MultiTestInput.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, imageData);
#endif // RENDER_SAVE_TEST_IMAGES
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

TestResources CreateTestResources(UTest::EngineResources& er)
{
    TestResources res;
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataView();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
    // History image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        BASE_NS::vector<BASE_NS::Math::Vec4> historyData(imageDesc.width * imageDesc.height, BASE_NS::Math::Vec4());
        const array_view<const uint8_t> dataView =
            BASE_NS::array_view(reinterpret_cast<const uint8_t*>(historyData.data()), historyData.size_in_bytes());
        res.historyImageHandle0 = er.device->GetGpuResourceManager().Create(HISTORY_IMAGE_NAME_0, imageDesc, dataView);
    }
    // Depth image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_D32_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        BASE_NS::vector<float> historyData(imageDesc.width * imageDesc.height, 1.f);
        const array_view<const uint8_t> dataView =
            BASE_NS::array_view(reinterpret_cast<const uint8_t*>(historyData.data()), historyData.size_in_bytes());
        res.depthImageHandle0 = er.device->GetGpuResourceManager().Create(DEPTH_IMAGE_NAME_0, imageDesc, dataView);
    }
    // Copy buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = NUM_BYTES;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle0 = er.device->GetGpuResourceManager().Create(COPY_BUFFER_NAME_0, bufferDesc);
    }
    // Post processing configuration
    {
        res.dataStorePod.reset(static_cast<IRenderDataStorePod*>(
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore").get()));
        PostProcessConfiguration ppConf;
        ppConf.enableFlags =
            PostProcessConfiguration::ENABLE_MOTION_BLUR_BIT | PostProcessConfiguration::ENABLE_TONEMAP_BIT |
            PostProcessConfiguration::ENABLE_VIGNETTE_BIT | PostProcessConfiguration::ENABLE_FXAA_BIT |
            PostProcessConfiguration::ENABLE_DOF_BIT | PostProcessConfiguration::ENABLE_LENS_FLARE_BIT |
            PostProcessConfiguration::ENABLE_UPSCALE_BIT;
        ppConf.dofConfiguration.farBlur = 1.f;
        ppConf.dofConfiguration.nearBlur = 1.f;
        ppConf.lensFlareConfiguration.intensity = 0.f;
        ppConf.upscaleConfiguration.ratio = 0.999999999f;
        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        res.dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView);
    }
    // Render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphGfxMultiPostProcessRenderNodeTest.rng");
    }
    {
        IImageLoaderManager& imgLoaderMgr = er.engine->GetImageLoaderManager();
        IImageLoaderManager::LoadResult loadResult = imgLoaderMgr.LoadImage(
            "test://images/MultiPPValidateSample.png", IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
        IImageContainer::Ptr imageContainer { move(loadResult.image) };

        const auto bytes = imageContainer->GetData();
        const uint32_t w = TEST_DATA_SIZE.x;
        const uint32_t h = TEST_DATA_SIZE.y;

        res.validateData.resize(w * h * 4u);

        const float inv255 = 1.0f / 255.0f;
        for (uint32_t p = 0u; p < w * h; ++p) {
            res.validateData[p * 4u + 0u] = bytes[p * 4u + 0u] * inv255; // R
            res.validateData[p * 4u + 1u] = bytes[p * 4u + 1u] * inv255; // G
            res.validateData[p * 4u + 2u] = bytes[p * 4u + 2u] * inv255; // B
            res.validateData[p * 4u + 3u] = 1.0f;                        // A
        }
        // UTest::SaveImage("test.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, res.validateData.data());
    }
    res.byteArray = make_unique<ByteArray>(NUM_BYTES);
    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.inputImageHandle0 = {};
    tr.copyBufferHandle0 = {};
    tr.renderNodeGraph = {};

    tr.byteArray.reset();
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;
    IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        if (idx == 1) {
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);
            const IRenderFrameUtil::CopyFlags copyFlags = IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE;
            rfUtil.CopyToCpu(outputImageHandle0, copyFlags);
        }

        er.engine->TickFrame();
        const RenderHandleReference inputs[] = { tr.inputImageHandle0, tr.historyImageHandle0, tr.depthImageHandle0 };
        er.context->GetRenderNodeGraphManager().SetRenderNodeGraphResources(
            tr.renderNodeGraph, { inputs, countof(inputs) }, {});

        if (idx == 0) {
            er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });
        } else {
            er.context->GetRenderer().RenderFrame({});
        }

        if (idx == 1) {
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(outputImageHandle0);
            const GpuBufferDesc desc = gpuResourceMgr.GetBufferDescriptor(data.bufferHandle);
            const uint32_t minSize = Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));

            EXPECT_EQ(NUM_BYTES, data.byteBuffer->GetData().size());
            if (data.byteBuffer && (data.copyFlags & IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE)) {
                const uint32_t minSize =
                    Math::min(desc.byteSize, static_cast<uint32_t>(data.byteBuffer->GetData().size()));
                // map the GPU buffer memory and copy to CPU ByteArray buffer
                if (void* mem = gpuResourceMgr.MapBufferMemory(data.bufferHandle); mem) {
                    BASE_NS::CloneData(
                        td.resources.byteArray->GetData().data(), data.byteBuffer->GetData().size(), mem, minSize);
                }
            }
        }
    }
}

string GetFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "MultiPPTestOutputVulkanGraphics.png";
    } else {
        return "MultiPPTestOutputOpenGLGraphics.png";
    }
}

inline float luminance(const float r, const float g, const float b)
{
    return 0.2126f * r + 0.7152f * g + 0.0722f * b;
}

void ValidateDataTest(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    float* outputImageData = reinterpret_cast<float*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    UTest::SaveImage(GetFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, outputImageData);
#endif // RENDER_SAVE_TEST_IMAGES
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u];
            float G = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u];
            float B = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u];
            float A = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u];

            float expectedR = td.resources.validateData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u];
            float expectedG = td.resources.validateData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u];
            float expectedB = td.resources.validateData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u];
            float expectedA = td.resources.validateData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u];

            float epsilon = 0.005f;
            ASSERT_NEAR(R, expectedR, epsilon);
            ASSERT_NEAR(G, expectedG, epsilon);
            ASSERT_NEAR(B, expectedB, epsilon);

            ASSERT_EQ(1.0f, A);
        }
    }
}

void TestMultiRenderNode(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine);
    }
    TickTest(testData, 3);
    ValidateDataTest(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GfxMultiRenderNodeTestVulkan
 * @tc.desc: Tests for multiple enabled post processing passes (FXAA, DOF, TONEMAP, MOTION BLUR, VIGNETTE),
 * and compares it to an expected image in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxMultiRenderNodeTest, GfxMultiRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestMultiRenderNode(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxMultiRenderNodeTestOpenGL
 * @tc.desc: Tests for multiple enabled post processing passes (FXAA, DOF, TONEMAP, MOTION BLUR, VIGNETTE),
 * and compares it to an expected image in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxMultiRenderNodeTest, GfxMultiRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    TestMultiRenderNode(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
