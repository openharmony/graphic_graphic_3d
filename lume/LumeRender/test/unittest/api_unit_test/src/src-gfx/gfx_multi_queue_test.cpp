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
static constexpr Math::UVec2 TEST_DATA_SIZE { 2u, 2u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
static constexpr size_t NUM_BYTES = IMAGE_SIZE * sizeof(uint16_t);
uint16_t imageData[IMAGE_SIZE];
static constexpr string_view TEST_IMAGE_NAME_0 { "TestImage0" };
static constexpr string_view COPY_BUFFER_NAME_0 { "CopyBuffer0" };

static constexpr Math::Vec4 COLOR0 = Math::Vec4(0.2f, 0.3f, 0.5f, 1.0f);
static constexpr Math::Vec4 COLOR1 = Math::Vec4(0.4f, 0.7f, -0.2f, 0.0f);
static constexpr Math::Vec4 EXPECTED_COLOR = COLOR0 + COLOR1;

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
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u] = BASE_NS::Math::F32ToF16(0.0f); // R
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u] = BASE_NS::Math::F32ToF16(0.0f); // G
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u] = BASE_NS::Math::F32ToF16(0.0f); // B
            imageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u] = BASE_NS::Math::F32ToF16(0.0f); // A
        }
    }
    return array_view<const uint8_t> { reinterpret_cast<const uint8_t*>(imageData), sizeof(imageData) };
}

TestResources CreateTestResources(UTest::EngineResources& er)
{
    TestResources res;
    // TestImage
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
        imageDesc.usageFlags =
            CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT | CORE_IMAGE_USAGE_STORAGE_BIT;
        const array_view<const uint8_t> dataView = CreateImageDataView();
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_0, imageDesc, dataView);
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
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphMultiQueueTest.rng");
    }
    {
        res.dataStorePod = refcnt_ptr<IRenderDataStorePod>(
            er.context->GetRenderDataStoreManager().Create(IRenderDataStorePod::UID, "dataStore"));
        res.dataStorePod->CreatePod(
            "color0", "color0", { reinterpret_cast<const uint8_t*>(&COLOR0), sizeof(Math::Vec4) });
        res.dataStorePod->CreatePod(
            "color1", "color1", { reinterpret_cast<const uint8_t*>(&COLOR1), sizeof(Math::Vec4) });
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
                const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(TEST_IMAGE_NAME_0);

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

void Validate(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.resources.byteArray->GetData().size());
    uint16_t* outputImageData = reinterpret_cast<uint16_t*>(td.resources.byteArray->GetData().data());
    for (size_t i = 0; i < TEST_DATA_SIZE.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE.y; ++j) {
            float R = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 0u]);
            float G = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 1u]);
            float B = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 2u]);
            float A = BASE_NS::Math::F16ToF32(outputImageData[i * TEST_DATA_SIZE.y * 4u + j * 4u + 3u]);

            EXPECT_NEAR(R, EXPECTED_COLOR.x, 0.001);
            EXPECT_NEAR(G, EXPECTED_COLOR.y, 0.001);
            EXPECT_NEAR(B, EXPECTED_COLOR.z, 0.001);
            EXPECT_NEAR(A, EXPECTED_COLOR.w, 0.001);
        }
    }
}

void TestMultiQueue()
{
    TestData testData;
    testData.engine.backend = DeviceBackendType::VULKAN; // Vulkan only test
    testData.engine.enableMultiQueue = true;

    // Would be nice to skip if there are no two queues,
    // but current device interface doesn't allow to query for queue count

    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine);
    }
    TickTest(testData, 3);
    Validate(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GfxMultiQueueTestVulkan
 * @tc.desc: Tests for multi queue feature by issuing compute passes in separate a graphics and compute queue, and each
 * queue adds to the image a push constant supplied color, and then is validated that output image has the added colors.
 * Backend is Vulkan.
 */
UNIT_TEST(API_GfxMultiQueueTest, GfxMultiQueueTestVulkan, testing::ext::TestSize.Level1)
{
    TestMultiQueue();
}
#endif // RENDER_HAS_VULKAN_BACKEND
