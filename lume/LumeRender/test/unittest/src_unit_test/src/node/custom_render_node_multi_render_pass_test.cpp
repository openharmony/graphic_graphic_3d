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

#include <default_engine_constants.h>
#include <device/device.h>
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

#include "node/nodes/render_node_draw.h"
#include "node/nodes/render_node_draw_indexed.h"
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
static constexpr string_view OUTPUT_IMAGE_NAME { "OutputImage" };
static constexpr string_view COPY_BUFFER_NAME { "CopyBuffer" };
static constexpr string_view VERTEX_BUFFER_NAME { "VertexBuffer" };
static constexpr string_view INDEX_BUFFER_NAME { "IndexBuffer" };
static constexpr uint32_t WIDTH = 201u;
static constexpr uint32_t HEIGHT = 201u;
static constexpr uint32_t NUM_BYTES = WIDTH * HEIGHT * 4u * 4u;

static constexpr float VERTEX_BUFFER_DATA[] = { -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f };
static constexpr uint32_t INDEX_BUFFER_DATA[] = { 0, 1, 2, 2, 3, 0 };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestData {
    UTest::EngineResources engine;
    RenderHandleReference renderNodeGraph;
    RenderHandleReference outputImageHandle;
    RenderHandleReference copyBufferHandle;
    RenderHandleReference vertexBufferHandle;
    RenderHandleReference indexBufferHandle;
    unique_ptr<ByteArray> byteArray;
};

void CreateTestResources(TestData& td, bool useSwapchain)
{
    // Add render node factories
    {
        RenderNodeGraphManager& renderNodeGraphMgr =
            static_cast<RenderNodeGraphManager&>(td.engine.context->GetRenderNodeGraphManager());
        RenderNodeManager& renderNodeMgr = renderNodeGraphMgr.GetRenderNodeManager();
        {
            RenderNodeTypeInfo info { { RenderNodeDrawIndexed::UID }, RenderNodeDrawIndexed::UID,
                RenderNodeDrawIndexed::TYPE_NAME, RenderNodeDrawIndexed::Create, RenderNodeDrawIndexed::Destroy,
                RenderNodeDrawIndexed::BACKEND_FLAGS, RenderNodeDrawIndexed::CLASS_TYPE };
            renderNodeMgr.AddRenderNodeFactory(info);
        }
        {
            RenderNodeTypeInfo info { { RenderNodeDraw::UID }, RenderNodeDraw::UID, RenderNodeDraw::TYPE_NAME,
                RenderNodeDraw::Create, RenderNodeDraw::Destroy, RenderNodeDraw::BACKEND_FLAGS,
                RenderNodeDraw::CLASS_TYPE };
            renderNodeMgr.AddRenderNodeFactory(info);
        }
    }
    // Create render node graph
    {
        if (useSwapchain) {
            td.renderNodeGraph = td.engine.context->GetRenderNodeGraphManager().LoadAndCreate(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                "test://renderNodeGraphMultiRenderPassWithSwapchain.rng");
        } else {
            if (td.engine.backend == DeviceBackendType::OPENGL) {
                td.renderNodeGraph = td.engine.context->GetRenderNodeGraphManager().LoadAndCreate(
                    IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                    "test://renderNodeGraphMultiRenderPassInline.rng");
            } else {
                td.renderNodeGraph = td.engine.context->GetRenderNodeGraphManager().LoadAndCreate(
                    IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                    "test://renderNodeGraphMultiRenderPass.rng");
            }
        }
    }
    // Output image
    {
        if (useSwapchain) {
            td.outputImageHandle = td.engine.device->GetGpuResourceManager().GetImageHandle(
                DefaultEngineGpuResourceConstants::CORE_DEFAULT_BACKBUFFER);
        } else {
            GpuImageDesc imageDesc;
            imageDesc.width = WIDTH;
            imageDesc.height = HEIGHT;
            imageDesc.depth = 1;
            imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
            imageDesc.format = BASE_FORMAT_R32G32B32A32_SFLOAT;
            imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
            imageDesc.imageType = CORE_IMAGE_TYPE_2D;
            imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
            imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                                   CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            td.outputImageHandle = td.engine.device->GetGpuResourceManager().Create(OUTPUT_IMAGE_NAME, imageDesc);
        }
    }
    // Copy buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = NUM_BYTES;
        if (td.engine.backend == DeviceBackendType::VULKAN) {
            bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        }
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        td.copyBufferHandle = td.engine.device->GetGpuResourceManager().Create(COPY_BUFFER_NAME, bufferDesc);
    }
    // Vertex buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = sizeof(VERTEX_BUFFER_DATA);
        bufferDesc.engineCreationFlags =
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_VERTEX_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        td.vertexBufferHandle = td.engine.device->GetGpuResourceManager().Create(VERTEX_BUFFER_NAME, bufferDesc,
            { reinterpret_cast<const uint8_t*>(VERTEX_BUFFER_DATA), sizeof(VERTEX_BUFFER_DATA) });
    }
    // Index buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = sizeof(INDEX_BUFFER_DATA);
        bufferDesc.engineCreationFlags =
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
            CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER | CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT |
                                CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        td.indexBufferHandle = td.engine.device->GetGpuResourceManager().Create(INDEX_BUFFER_NAME, bufferDesc,
            { reinterpret_cast<const uint8_t*>(INDEX_BUFFER_DATA), sizeof(INDEX_BUFFER_DATA) });
    }
    td.byteArray = make_unique<ByteArray>(NUM_BYTES);
}

void DestroyTestResources(IDevice& device, TestData& td)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    td.renderNodeGraph = {};
    td.outputImageHandle = {};
    td.copyBufferHandle = {};
    td.vertexBufferHandle = {};
    td.indexBufferHandle = {};
    td.byteArray.reset();
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        er.engine->TickFrame();

        if (idx == 1) {
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
                const RenderHandleReference outputImageHandle0 = gpuResourceMgr.GetImageHandle(OUTPUT_IMAGE_NAME);

                dsStaging->CopyImageToBuffer(outputImageHandle0, td.copyBufferHandle, bufferImageCopy);
            }
        }

        if (idx == 2) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY)) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = td.copyBufferHandle;
                dataCopy.byteArray = td.byteArray.get();
                dataStoreDataCopy->AddCopyOperation(dataCopy);
            }
            td.outputImageHandle = {};
        }

        if (idx == 3) {
            td.copyBufferHandle = {};
        }

        if (idx == 0) {
            er.context->GetRenderer().RenderFrame({ &td.renderNodeGraph, 1 });
        } else {
            er.context->GetRenderer().RenderFrame({});
        }
    }
}

string GetFileName(const UTest::EngineResources& engine)
{
    if (engine.backend == DeviceBackendType::VULKAN) {
        return "MultiRenderPassOutputVulkan.png";
    } else {
        return "MultiRenderPassOutputOpenGL.png";
    }
}

void Validate(const TestData& td)
{
    ASSERT_EQ(NUM_BYTES, td.byteArray->GetData().size());
    float* outputImageData = reinterpret_cast<float*>(td.byteArray->GetData().data());
#if RENDER_SAVE_TEST_IMAGES == 1
    UTest::SaveHdrImage(GetFileName(td.engine), WIDTH, HEIGHT, outputImageData);
#endif // RENDER_SAVE_TEST_IMAGES
    int32_t x = WIDTH / 2;
    int32_t y = HEIGHT / 2;
    for (int32_t i = 0; i < WIDTH; ++i) {
        for (int32_t j = 0; j < HEIGHT; ++j) {
            float R = outputImageData[i * WIDTH * 4u + j * 4u + 0u];
            float G = outputImageData[i * WIDTH * 4u + j * 4u + 1u];
            float B = outputImageData[i * WIDTH * 4u + j * 4u + 2u];
            float A = outputImageData[i * WIDTH * 4u + j * 4u + 3u];

            if ((WIDTH - i - 1) + j <= (WIDTH + HEIGHT) / 2) {
                continue;
            }

            if (abs(x - i) <= WIDTH / 6 && abs(y - j) <= HEIGHT / 6) {
                ASSERT_EQ(0.5f, R);
                ASSERT_EQ(0.5f, G);
                ASSERT_EQ(0.5f, B);
                ASSERT_EQ(1.0f, A);
            } else if ((abs(x - i) > WIDTH / 3 || abs(y - j) > HEIGHT / 3)) {
                ASSERT_EQ(0.f, R);
                ASSERT_EQ(0.f, G);
                ASSERT_EQ(0.f, B);
                ASSERT_EQ(0.f, A);
            }
        }
    }
}

void TestMultiRenderPass(DeviceBackendType backend, bool useSwapchain)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL || useSwapchain) {
        testData.engine.createWindow = true;
        testData.engine.width = WIDTH;
        testData.engine.height = HEIGHT;
    }
    UTest::CreateEngineSetup(testData.engine);
    uint64_t surfaceHandle = 0;
    if (useSwapchain) {
        SwapchainCreateInfo swapchainInfo {};
#if defined(__OHOS__)
        swapchainInfo.window.window = reinterpret_cast<uintptr_t>(::Test::g_ohosApp->GetWindowHandle());
        ASSERT_NE(::Test::g_ohosApp->GetWindowHandle(), nullptr);
#else
        surfaceHandle = UTest::CreateSurface(testData.engine);
        ASSERT_NE(surfaceHandle, 0);
#endif
        swapchainInfo.surfaceHandle = surfaceHandle;
        swapchainInfo.swapchainFlags = RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_COLOR_BUFFER_BIT |
                                       RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_DEPTH_BUFFER_BIT |
                                       RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_VSYNC_BIT |
                                       RENDER_NS::SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT;

        testData.engine.device->CreateSwapchain(swapchainInfo);
        const RenderHandle defaultSwapchain;
        ASSERT_NE(nullptr, ((Device*)testData.engine.device)->GetSwapchain(defaultSwapchain));
    }
    CreateTestResources(testData, useSwapchain);
    TickTest(testData, 4);
    if (!useSwapchain) {
        Validate(testData);
    }
    DestroyTestResources(*testData.engine.device, testData);
    if (useSwapchain) {
        testData.engine.device->DestroySwapchain();
        UTest::DestroySurface(testData.engine, surfaceHandle);
    }
    UTest::DestroyEngine(testData.engine);
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: RenderNodeMultiRenderPassTestVulkan
 * @tc.desc: Tests for multiple renderpasses in a render node graph outputting to the same image in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeMultiRenderPassTestVulkan, testing::ext::TestSize.Level1)
{
    TestMultiRenderPass(DeviceBackendType::VULKAN, false);
}

/**
 * @tc.name: RenderNodeMultiRenderPassWithSwapchainTestVulkan
 * @tc.desc: Tests for multiple renderpasses in a render node graph outputting to the swapchain image in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeMultiRenderPassWithSwapchainTestVulkan, testing::ext::TestSize.Level1)
{
    TestMultiRenderPass(DeviceBackendType::VULKAN, true);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: RenderNodeMultiRenderPassTestOpenGL
 * @tc.desc: Tests for multiple renderpasses in a render node graph outputting to the same image in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeMultiRenderPassTestOpenGL, testing::ext::TestSize.Level1)
{
    TestMultiRenderPass(UTest::GetOpenGLBackend(), false);
}
/**
 * @tc.name: RenderNodeMultiRenderPassWithSwapchainTestOpenGL
 * @tc.desc: Tests for multiple renderpasses in a render node graph outputting to the swapchain image in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_CustomRenderNode, RenderNodeMultiRenderPassWithSwapchainTestOpenGL, testing::ext::TestSize.Level1)
{
    TestMultiRenderPass(UTest::GetOpenGLBackend(), true);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
