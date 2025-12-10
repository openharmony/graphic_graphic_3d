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

#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

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
static constexpr Math::UVec2 TEST_DATA_SIZE { 5u, 5u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(float);
float imageData[IMAGE_SIZE];
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
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

array_view<const uint8_t> CreateImageDataView()
{
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = 4.f; // R
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = 2.f; // G
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = 3.f; // B
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = 1.f; // A
        }
    }
#if RENDER_SAVE_TEST_IMAGES == 1
    UTest::SaveHdrImage("BackBufferTestInput.png", TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, imageData);
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
        imageDesc.usageFlags =
            CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
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
        refcnt_ptr<IRenderDataStorePod> dataStorePod =
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "PostProcessDataStore");
        PostProcessConfiguration ppConf;

        ppConf.enableFlags = PostProcessConfiguration::ENABLE_BLOOM_BIT | PostProcessConfiguration::ENABLE_BLUR_BIT |
                             PostProcessConfiguration::ENABLE_TAA_BIT | PostProcessConfiguration::ENABLE_FXAA_BIT;

        ppConf.bloomConfiguration.bloomQualityType = BloomConfiguration::BloomQualityType::QUALITY_TYPE_LOW;
        ppConf.bloomConfiguration.useCompute = false;

        ppConf.blurConfiguration.blurQualityType = BlurConfiguration::QUALITY_TYPE_HIGH;
        ppConf.blurConfiguration.blurType = BlurConfiguration::TYPE_NORMAL;
        ppConf.blurConfiguration.filterSize = 10.f;

        ppConf.fxaaConfiguration.quality = FxaaConfiguration::Quality::HIGH;
        ppConf.fxaaConfiguration.sharpness = FxaaConfiguration::Sharpness::SHARP;

        ppConf.taaConfiguration.quality = TaaConfiguration::Quality::HIGH;
        ppConf.taaConfiguration.sharpness = TaaConfiguration::Sharpness::SHARP;

        const array_view<const uint8_t> dataView = { reinterpret_cast<const uint8_t*>(&ppConf), sizeof(ppConf) };
        dataStorePod->CreatePod("PostProcessConfiguration", "PostProcessConfiguration", dataView);
    }
    // render node graph
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphGfxBackBufferRenderNodeTest.rng");
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
        }

        er.engine->TickFrame();
        const RenderHandleReference inputs[] = { tr.inputImageHandle0 };
        er.context->GetRenderNodeGraphManager().SetRenderNodeGraphResources(
            tr.renderNodeGraph, { inputs, countof(inputs) }, {});

        if (idx == 0) {
            er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });
        } else {
            er.context->GetRenderer().RenderFrame({});
        }
    }
}

string GetFileName(const UTest::EngineResources& er)
{
    if (er.backend == DeviceBackendType::VULKAN) {
        return "BackBufferTestOutputVulkan.png";
    } else {
        return "BackBufferTestOutputOpenGL.png";
    }
}

void ValidateDataTest(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    float* outputImageData = reinterpret_cast<float*>(td.resources.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    UTest::SaveHdrImage(GetFileName(td.engine), TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, outputImageData);
#endif // RENDER_SAVE_TEST_IMAGES
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u];
            float G = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u];
            float B = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u];
            float A = outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u];

            ASSERT_EQ(2.f, R);
            ASSERT_EQ(3.f, G);
            ASSERT_EQ(4.f, B);
            ASSERT_EQ(1.f, A);
        }
    }
}

void TestBackBufferRenderNode(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    testData.engine.enableMultiQueue = true;
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
 * @tc.name: GfxBackBufferRenderNodeTestVulkan
 * @tc.desc: Tests that backbuffer is written to via custom rendernode and shader. An input image
 * is uploaded to the GPU, then it is sampled in the shader and written to the backbuffer. Lastly, the input image and
 * backbuffer image are expected to be the same. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxBackBufferRenderNodeTest, GfxBackBufferRenderNodeTestVulkan, testing::ext::TestSize.Level1)
{
    TestBackBufferRenderNode(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxBackBufferRenderNodeTestOpenGL
 * @tc.desc: Tests that backbuffer is written to via custom rendernode and shader. An input image
 * is uploaded to the GPU, then it is sampled in the shader and written to the backbuffer. Lastly, the input image and
 * backbuffer image are expected to be the same. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxBackBufferRenderNodeTest, GfxBackBufferRenderNodeTestOpenGL, testing::ext::TestSize.Level1)
{
    TestBackBufferRenderNode(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
