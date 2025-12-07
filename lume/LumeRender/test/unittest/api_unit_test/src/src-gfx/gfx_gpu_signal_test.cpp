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
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
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

#define TEST_GL_WAIT_SYNC 0

#if RENDER_HAS_GL_BACKEND

#if (TEST_GL_WAIT_SYNC == 1)
#include <GL/glcorearb.h>

#include <render/gles/intf_device_gles.h>
namespace {
static PFNGLCLIENTWAITSYNCPROC g_glClientWaitSync = nullptr;
}
#endif

#endif

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

namespace {
static constexpr Math::UVec2 TEST_DATA_SIZE { 2u, 2u };
static constexpr size_t IMAGE_SIZE = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y;
static constexpr size_t IMAGE_BYTES = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * 4u;
// red, green, blue, yellow, violet
static constexpr uint32_t TEST_IMAGE_COLORS[] = { 0xFF0000ff, 0xFF00ff00, 0xFFff0000, 0xFF00ffff, 0xFFFFff00 };
static constexpr string_view INPUT_IMAGE_NAME_0 { "InputImage0" };
static constexpr string_view COPY_BUFFER_NAME_0 { "CopyBuffer0" };
// NOTE: created in render node graph
static constexpr string_view OUTPUT_IMAGE_NAME_0 { "OutputImage0" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference inputImageHandle0;
    RenderHandleReference outputImageHandle0;
    RenderHandleReference copyBufferHandle0;

    unique_ptr<ByteArray> byteArray;

    RenderHandleReference renderNodeGraph;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

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
        imageDesc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT |
                               CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        const array_view<const uint8_t> dataView = { (uint8_t*)TEST_IMAGE_COLORS, sizeof(TEST_IMAGE_COLORS) };
        res.inputImageHandle0 = er.device->GetGpuResourceManager().Create(INPUT_IMAGE_NAME_0, imageDesc, dataView);
    }
    // Output image
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags =
            CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        res.outputImageHandle0 = er.device->GetGpuResourceManager().Create(OUTPUT_IMAGE_NAME_0, imageDesc);
    }
    // Copy buffer
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = IMAGE_BYTES;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle0 = er.device->GetGpuResourceManager().Create(COPY_BUFFER_NAME_0, bufferDesc);
    }
    {
        res.renderNodeGraph = er.context->GetRenderNodeGraphManager().LoadAndCreate(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
            "test://renderNodeGraphGfxGpuSignalTest.rng");
    }
    res.byteArray = make_unique<ByteArray>(IMAGE_BYTES);

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

                dsStaging->CopyImageToBuffer(td.resources.outputImageHandle0, tr.copyBufferHandle0, bufferImageCopy);
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

        if (idx == 0) {
            const DeviceBackendType backendType = er.context->GetDevice().GetBackendType();
            // add signals
            {
                IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
                rfUtil.AddGpuSignal({});
            }

            er.context->GetRenderer().RenderFrame({ &tr.renderNodeGraph, 1u });

            // wait for signals
            {
                IRenderFrameUtil& rfUtil = er.context->GetRenderUtil().GetRenderFrameUtil();
                const auto& gpuSignals = rfUtil.GetFrameGpuSignalData();
                ASSERT_EQ(gpuSignals.size(), 1);

                const auto& firstSignal = gpuSignals[0u];
                ASSERT_EQ(firstSignal.signaled, true);
                ASSERT_NE(firstSignal.gpuSignalResourceHandle, 0);
                if (backendType == DeviceBackendType::VULKAN) {
#if RENDER_HAS_VULKAN_BACKEND
                    ASSERT_EQ(firstSignal.signalResourceType, IRenderFrameUtil::SignalResourceType::GPU_SEMAPHORE);
#endif
                } else {
#if RENDER_HAS_GL_BACKEND
                    ASSERT_EQ(firstSignal.signalResourceType, IRenderFrameUtil::SignalResourceType::GPU_FENCE);
#if (TEST_GL_WAIT_SYNC == 1)
                    auto plat = static_cast<const DevicePlatformDataGL&>(er.context->GetDevice().GetPlatformData());
                    wglMakeCurrent(plat.display, plat.context);

                    if (g_glClientWaitSync == nullptr) {
                        *(reinterpret_cast<void**>(&g_glClientWaitSync)) =
                            reinterpret_cast<void*>(wglGetProcAddress("glClientWaitSync"));
                    }
                    // if (g_glClientWaitSync == nullptr) {
                    //     *(reinterpret_cast<void**>(&g_glClientWaitSync)) =
                    //         reinterpret_cast<void*>(GetProcAddress(glModule_, "glClientWaitSync"));
                    // }
                    ASSERT_NE(g_glClientWaitSync, nullptr);

                    if (g_glClientWaitSync &&
                        (firstSignal.signalResourceType == IRenderFrameUtil::SignalResourceType::GPU_FENCE)) {
                        GLsync sync = (GLsync)(firstSignal.gpuSignalResourceHandle);
                        GLenum waitSyncRes = g_glClientWaitSync(sync, GL_SYNC_FLUSH_COMMANDS_BIT, 0);
                        ASSERT_EQ(waitSyncRes, GL_ALREADY_SIGNALED);
                        // ASSERT_NE(waitSyncRes, GL_WAIT_FAILED);
                        // ASSERT_NE(waitSyncRes, GL_ALREADY_SIGNALED);
                        // ASSERT_NE(waitSyncRes, GL_TIMEOUT_EXPIRED);
                    }
                    wglMakeCurrent(nullptr, nullptr);
#endif

#endif
                }
            }
        } else {
            er.context->GetRenderer().RenderFrame({});
        }
    }
}

void Validate(const TestData& td)
{
    ASSERT_EQ(IMAGE_BYTES, td.resources.byteArray->GetData().size());
    const uint32_t* outputData = reinterpret_cast<const uint32_t*>(td.resources.byteArray->GetData().data());
    for (uint32_t idx = 0; idx < IMAGE_SIZE; ++idx) {
        const uint32_t expectedColor = TEST_IMAGE_COLORS[idx];
        const uint32_t realColor = outputData[idx];
        ASSERT_EQ(expectedColor, realColor);
    }
}

void TestGpuSignal(DeviceBackendType backend)
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
    Validate(testData);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GfxGpuSignalTestVulkan
 * @tc.desc: Tests GPU signals in Vulkan by creating a signal before rendering the frame and waiting for the signal
 * before copying the rendered frame.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuSignal, GfxGpuSignalTestVulkan, testing::ext::TestSize.Level1)
{
    TestGpuSignal(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GfxGpuSignalTestOpenGL
 * @tc.desc: Tests GPU signals in OpenGL by creating a signal before rendering the frame and waiting for the signal
 * before copying the rendered frame.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuSignal, GfxGpuSignalTestOpenGL, testing::ext::TestSize.Level1)
{
    TestGpuSignal(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
