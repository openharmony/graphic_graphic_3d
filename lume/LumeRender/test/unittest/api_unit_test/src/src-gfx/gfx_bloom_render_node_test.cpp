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

#include <base/math/float_packer.h>
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
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
static constexpr Math::UVec2 TEST_DATA_SIZE { 512u, 512u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(uint16_t);
uint16_t imageData[IMAGE_SIZE];
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
static constexpr string_view COPY_BUFFER_NAME_0 { "CopyBuffer0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;
    RenderHandleReference copyBufferHandle0;

    unique_ptr<ByteArray> byteArray;

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
            if (i < TEST_DATA_SIZE.x / 2) {
                float value = 15.f;
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(value); // R
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(value); // G
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(value); // B
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f);  // A
            } else {
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(0.0f); // R
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(0.0f); // G
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(0.0f); // B
                imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(1.0f); // A
            }
        }
    }
#if RENDER_SAVE_TEST_IMAGES == 1
    // convert uint16_t data to float
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(imageData[ii]);
    }
    // For OHOS write here: /data/storage/el2/base/haps/entry/files/{FILE}.png
    // Read the file: hdc file recv /data/app/el2/100/base/com.huawei.render_api_test_runner/haps/entry/files/{FILE}.png
    UTest::SaveHdrImage("BloomTestInput.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

TestResources CreateTestResources(UTest::EngineResources& er, bool useCompute)
{
    TestResources res;
    // Input image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataView();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
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
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "BloomDataStore").get()));
        PostProcessConfiguration ppConf;
        ppConf.enableFlags = PostProcessConfiguration::ENABLE_BLOOM_BIT;
        ppConf.bloomConfiguration.bloomQualityType = BloomConfiguration::BloomQualityType::QUALITY_TYPE_NORMAL;
        ppConf.bloomConfiguration.useCompute = useCompute;
        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        res.dataStorePod->CreatePod("PostProcessConfiguration", "BloomConfiguration", dataView);
    }
    // Render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphGfxBloomRenderNodeTest.rng");
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

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        if (idx == 1) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                BufferImageCopy bufferImageCopy;
                bufferImageCopy.bufferOffset = 0;
                bufferImageCopy.bufferRowLength = TEST_DATA_SIZE.x;
                bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE.y;
                bufferImageCopy.imageSubresource =
                    ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u };
                bufferImageCopy.imageOffset = { 0, 0, 0 };
                bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };

                IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
                const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);

                dsStaging->CopyImageToBuffer(outputImageHandle0, tr.copyBufferHandle0, bufferImageCopy);
            }
        }

        if (idx == 2) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY)) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = tr.copyBufferHandle0;
                dataCopy.byteArray = tr.byteArray.get();
                dataStoreDataCopy->AddCopyOperation(dataCopy);
            }

            {
                IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
                const RenderHandleReference imageHandle = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);
                IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
                rfUtil.CopyToCpu(imageHandle, IRenderFrameUtil::CopyFlagBits::WAIT_FOR_IDLE);
            }
        }

        er.engine->TickFrame();
        if (idx == 0) {
            er.context->GetRenderer().RenderDeferred({ &tr.renderNodeGraph, 1u });
            er.context->GetRenderer().RenderDeferredFrame();
        } else {
            er.context->GetRenderer().RenderFrame({});
        }

        if (idx == 2) {
            IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
            IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
            const RenderHandleReference imageHandle = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME_0);
            const IRenderFrameUtil::FrameCopyData& data = rfUtil.GetFrameCopyData(imageHandle);
            ASSERT_NE(0, data.byteBuffer->GetData().size());
        }
    }
}

string GetFileName(const UTest::EngineResources& er, bool useCompute)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        if (useCompute) {
            return "BloomTestOutputVulkanCompute.png";
        } else {
            return "BloomTestOutputVulkanGraphics.png";
        }
    } else {
        if (useCompute) {
            return "BloomTestOutputOpenGLCompute.png";
        } else {
            return "BloomTestOutputOpenGLGraphics.png";
        }
    }
}

void ValidateDataTest(const TestData& td, bool useCompute)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    //  convert uint16_t to float for saving
    float* floatData = new float[IMAGE_SIZE];
    for (size_t ii = 0; ii < IMAGE_SIZE; ii++) {
        floatData[ii] = BASE_NS::Math::F16ToF32(outputImageData[ii]);
    }
    UTest::SaveHdrImage(GetFileName(td.engine, useCompute), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, floatData);
    delete[] floatData;
#endif // RENDER_SAVE_TEST_IMAGES
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]));
            float G = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]));
            float B = UTest::FromHDR(BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]));
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);

            if (i < TEST_DATA_SIZE.x / 2) {
                ASSERT_LE(0.9f, R);
                ASSERT_LE(0.9f, G);
                ASSERT_LE(0.9f, B);
            } else {
                float threshold = static_cast<float>(TEST_DATA_SIZE.x - i) / (TEST_DATA_SIZE.x / 2) - 0.1f;
                ASSERT_LE(threshold, R);
                ASSERT_LE(threshold, G);
                ASSERT_LE(threshold, B);
            }

            ASSERT_EQ(1.0f, A);
        }
    }
}

void TestBloomRenderNode(DeviceBackendType backend, bool useCompute)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, useCompute);
    }
    TickTest(testData, 3);
    ValidateDataTest(testData, useCompute);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GfxBloomRenderNodeTestVulkan
 * @tc.desc: Tests for the bloom post process to work correctly on the Vulkan backend using a fragment shader.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxBloomRenderNodeTest, GfxBloomRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestBloomRenderNode(DeviceBackendType::VULKAN, false);
}
/**
 * @tc.name: ComputeBloomRenderNodeTestVulkan
 * @tc.desc: Tests for the bloom post process to work correctly on the Vulkan backend using a compute shader.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxBloomRenderNodeTest, ComputeBloomRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestBloomRenderNode(DeviceBackendType::VULKAN, true);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxBloomRenderNodeTestOpenGL
 * @tc.desc: Tests for the bloom post process to work correctly on the OpenGL backend using a fragment shader.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxBloomRenderNodeTest, GfxBloomRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    TestBloomRenderNode(UTest::GetOpenGLBackend(), false);
}
/**
 * @tc.name: ComputeBloomRenderNodeTestOpenGL
 * @tc.desc: Tests for the bloom post process to work correctly on the OpenGL backend using a compute shader.
 * @tc.type: FUNC
 */
#ifdef __OHOS__
// Harmony OS compute bloom with GLES fail. Fails because the threshold is exceeded.
UNIT_TEST(API_GfxBloomRenderNodeTest, DISABLED_ComputeBloomRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
#else
UNIT_TEST(API_GfxBloomRenderNodeTest, ComputeBloomRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
#endif // __OHOS__
{
    TestBloomRenderNode(UTest::GetOpenGLBackend(), true);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
