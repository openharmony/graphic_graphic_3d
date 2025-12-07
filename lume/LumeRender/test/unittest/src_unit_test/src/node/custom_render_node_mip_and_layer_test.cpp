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

#include <nodecontext/render_node_graph_manager.h>
#include <nodecontext/render_node_manager.h>

#include <render/datastore/intf_render_data_store_default_gpu_resource_data_copy.h>
#include <render/datastore/intf_render_data_store_default_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/intf_renderer.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#include "node/nodes/render_node_mip_and_layer.h"
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
static constexpr string_view GRAPHICS_IMAGE_NAME { "GraphicsImage" };
static constexpr string_view COMPUTE_IMAGE_NAME { "ComputeImage" };
static constexpr uint32_t WIDTH = 2U;
static constexpr uint32_t HEIGHT = 2U;
static constexpr uint32_t MIP_COUNT = 2U;
static constexpr uint32_t NUM_BYTES = WIDTH * HEIGHT * 4U;

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestData {
    UTest::EngineResources engine;
    RenderHandleReference renderNodeGraph;
    RenderHandleReference graphicsImageHandle;
    RenderHandleReference computeImageHandle;
    RenderHandleReference copyBufferGraphicsHandle;
    RenderHandleReference copyBufferComputeHandle;
    unique_ptr<ByteArray> byteArrayGraphics;
    unique_ptr<ByteArray> byteArrayCompute;
};

constexpr uint32_t GetOutputColorReference()
{
    return 0x800000FF;
}

void CreateTestResources(TestData& td)
{
    // Add render node factories
    {
        RenderNodeGraphManager& renderNodeGraphMgr =
            static_cast<RenderNodeGraphManager&>(td.engine.context->GetRenderNodeGraphManager());
        RenderNodeManager& renderNodeMgr = renderNodeGraphMgr.GetRenderNodeManager();
        {
            RenderNodeTypeInfo info { { RenderNodeMipAndLayer::UID }, RenderNodeMipAndLayer::UID,
                RenderNodeMipAndLayer::TYPE_NAME, RenderNodeMipAndLayer::Create, RenderNodeMipAndLayer::Destroy,
                RenderNodeMipAndLayer::BACKEND_FLAGS, RenderNodeMipAndLayer::CLASS_TYPE };
            renderNodeMgr.AddRenderNodeFactory(info);
        }
    }
    // Create render node graph
    {
        td.renderNodeGraph = td.engine.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphRenderNodeMipAndLayerTest.rng");
    }
    // Graphics image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = WIDTH;
        imageDesc.height = HEIGHT;
        imageDesc.depth = 1;
        imageDesc.mipCount = MIP_COUNT;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        td.graphicsImageHandle = td.engine.device->GetGpuResourceManager().Create(GRAPHICS_IMAGE_NAME, imageDesc);
    }
    // Compute image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = WIDTH;
        imageDesc.height = HEIGHT;
        imageDesc.depth = 1;
        imageDesc.mipCount = MIP_COUNT;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_STORAGE_BIT;
        td.computeImageHandle = td.engine.device->GetGpuResourceManager().Create(COMPUTE_IMAGE_NAME, imageDesc);
    }
    // Copy buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = NUM_BYTES;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        td.copyBufferGraphicsHandle = td.engine.device->GetGpuResourceManager().Create(bufferDesc);
        td.copyBufferComputeHandle = td.engine.device->GetGpuResourceManager().Create(bufferDesc);
    }
    td.byteArrayGraphics = make_unique<ByteArray>(NUM_BYTES);
    td.byteArrayCompute = make_unique<ByteArray>(NUM_BYTES);
}

void DestroyTestResources(IDevice& device, TestData& td)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    td.renderNodeGraph = {};
    td.graphicsImageHandle = {};
    td.computeImageHandle = {};
    td.copyBufferGraphicsHandle = {};
    td.copyBufferComputeHandle = {};
    td.byteArrayGraphics.reset();
    td.byteArrayCompute.reset();
}

void TickTest(const TestData& td, int32_t frameCountToTick)
{
    const UTest::EngineResources& er = td.engine;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        er.engine->TickFrame();

        if (idx == (frameCountToTick - 2)) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                BufferImageCopy bufferImageCopy;
                bufferImageCopy.bufferOffset = 0;
                bufferImageCopy.bufferRowLength = WIDTH;
                bufferImageCopy.bufferImageHeight = HEIGHT;
                bufferImageCopy.imageSubresource =
                    ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u };
                bufferImageCopy.imageOffset = { 0, 0, 0 };
                bufferImageCopy.imageExtent = { WIDTH, HEIGHT, 1u };

                IGpuResourceManager& gpuResourceMgr = er.context->GetDevice().GetGpuResourceManager();
                {
                    const RenderHandleReference outputImageHandle = gpuResourceMgr.GetImageHandle(GRAPHICS_IMAGE_NAME);
                    dsStaging->CopyImageToBuffer(outputImageHandle, td.copyBufferGraphicsHandle, bufferImageCopy);
                }
                {
                    const RenderHandleReference outputImageHandle = gpuResourceMgr.GetImageHandle(COMPUTE_IMAGE_NAME);
                    dsStaging->CopyImageToBuffer(outputImageHandle, td.copyBufferComputeHandle, bufferImageCopy);
                }
            }
        }

        if (idx == (frameCountToTick - 1)) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY)) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                {
                    dataCopy.gpuHandle = td.copyBufferGraphicsHandle;
                    dataCopy.byteArray = td.byteArrayGraphics.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
                {
                    dataCopy.gpuHandle = td.copyBufferComputeHandle;
                    dataCopy.byteArray = td.byteArrayCompute.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
            }
        }

        if (idx < (frameCountToTick - 2)) {
            er.context->GetRenderer().RenderFrame({ &td.renderNodeGraph, 1 });
        } else {
            er.context->GetRenderer().RenderFrame({});
        }
    }
}

string GetFileName(const UTest::EngineResources& engine)
{
    if (engine.backend == DeviceBackendType::VULKAN) {
        return "RenderNodeMipAndLayerTestVulkan.png";
    } else {
        return "RenderNodeMipAndLayerTestOpenGL.png";
    }
}

void Validate(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.byteArrayGraphics->GetData().size());
    ASSERT_EQ(NUM_BYTES, td.byteArrayCompute->GetData().size());
    {
        const array_view<uint8_t> data = td.byteArrayGraphics->GetData();
        const array_view<const uint32_t> outputImageData = { reinterpret_cast<const uint32_t*>(data.data()),
            data.size_bytes() / 4U };
#if RENDER_SAVE_TEST_IMAGES == 1
        UTest::WritePng(GetFileName(td.engine), WIDTH, HEIGHT, 4U, outputImageData.data(), outputImageData.size());
#endif // RENDER_SAVE_TEST_IMAGES
        for (uint32_t idx = 0; idx < 4U; ++idx) {
            const uint32_t col = outputImageData[idx];
            ASSERT_EQ(GetOutputColorReference(), col);
        }
    }
    {
        const array_view<uint8_t> data = td.byteArrayCompute->GetData();
        const array_view<const uint32_t> outputImageData = { reinterpret_cast<const uint32_t*>(data.data()),
            data.size_bytes() / 4U };
        for (uint32_t idx = 0; idx < 4U; ++idx) {
            const uint32_t col = outputImageData[idx];
            ASSERT_EQ(GetOutputColorReference(), col);
        }
    }
}

void TestRenderNodeMipAndLayer(DeviceBackendType backend)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        UTest::CreateEngineSetup(testData.engine);
        CreateTestResources(testData);
    }
    TickTest(testData, 4);
    Validate(testData);
    {
        DestroyTestResources(*testData.engine.device, testData);
        UTest::DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: RenderNodeMipAndLayerTestVulkan
 * @tc.desc: Tests rendering to an image mipmap from a renderpass with a custom render node in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeMipAndLayerTestVulkan, testing::ext::TestSize.Level1)
{
    TestRenderNodeMipAndLayer(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: RenderNodeMipAndLayerTestOpenGL
 * @tc.desc: Tests rendering to an image mipmap from a renderpass with a custom render node in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeMipAndLayerTestOpenGL, testing::ext::TestSize.Level1)
{
    TestRenderNodeMipAndLayer(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
