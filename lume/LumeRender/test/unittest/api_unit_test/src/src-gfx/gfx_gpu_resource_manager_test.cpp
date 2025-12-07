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
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
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

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

#define TEST_WRITE_PNG 0

namespace {
static constexpr uint32_t TEST_IMAGE_COUNT { 2u };
static constexpr uint32_t TEST_COLOR_COUNT { 5u };
// red, green, blue, yellow, violet
static constexpr uint32_t TEST_IMAGE_COLORS[] = { 0xFF0000ff, 0xFF00ff00, 0xFFff0000, 0xFF00ffff, 0xFFFFff00 };
static constexpr string_view TEST_IMAGE_NAME { "TestImage" };
static constexpr string_view TEST_BUFFER_NAME { "TestBuffer" };

struct TestImage {
    uint32_t color { 0u };
    RenderHandleReference imageHandle;
};
struct TestResources {
    RenderHandleReference cpuCopyBufferHandle;
    RenderHandleReference outputImageHandle;
    TestImage images[TEST_IMAGE_COUNT];

    unique_ptr<ByteArray> byteArray;
    GpuImageDesc imageDesc;
    GpuBufferDesc bufferDesc;
};

struct TestData {
    UTest::EngineResources engine;
    RenderHandleReference renderNodeGraph;
    TestResources resources;

    uint32_t currentColorIndex = 0u;
    inline uint32_t GetTestColor()
    {
        currentColorIndex = (currentColorIndex + 1) % TEST_COLOR_COUNT;
        return TEST_IMAGE_COLORS[currentColorIndex];
    };

    uint32_t windowWidth { 0u };
    uint32_t windowHeight { 0u };
    uint32_t elementByteSize { 4u };
};

TestResources CreateTestResources(TestData& td)
{
    TestResources res;
    // Output
    {
        GpuImageDesc imageDesc;
        imageDesc.width = td.windowWidth;
        imageDesc.height = td.windowHeight;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;
        res.outputImageHandle = td.engine.device->GetGpuResourceManager().Create("OutputImage", imageDesc);
    }
    // Input images
    {
        auto& imageDesc = res.imageDesc;
        imageDesc.width = 2u;
        imageDesc.height = 2u;
        imageDesc.depth = 1;
        imageDesc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        for (uint32_t idx = 0; idx < TEST_IMAGE_COUNT; ++idx) {
            auto& image = res.images[idx];
            const uint32_t col = td.GetTestColor();
            image.color = col;
            const uint32_t rgbData[4u] = { col, col, col, col };
            const auto rgbDataView =
                array_view(reinterpret_cast<const uint8_t*>(rgbData), sizeof(uint32_t) * countof(rgbData));
            const auto name = TEST_IMAGE_NAME + to_string(idx);
            image.imageHandle = td.engine.device->GetGpuResourceManager().Create(name, imageDesc, rgbDataView);
        }
    }
    // Input buffers
    {
        auto& bufferDesc = res.bufferDesc;
        bufferDesc.byteSize = td.elementByteSize * 4;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    GpuBufferDesc bufferDesc;
    bufferDesc.byteSize = td.windowWidth * td.windowHeight * td.elementByteSize;
    bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const vector<uint8_t> bufferDefaultData(bufferDesc.byteSize, 0);
    res.cpuCopyBufferHandle =
        td.engine.device->GetGpuResourceManager().Create("TestOutputBuffer", bufferDesc, { bufferDefaultData });

    auto& rdsMgr = td.engine.context->GetRenderDataStoreManager();

    refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
        rdsMgr.GetRenderDataStore("RenderDataStoreDefaultGpuResourceDataCopy");
    refcnt_ptr<IRenderDataStorePod> dataStorePod = rdsMgr.GetRenderDataStore("RenderDataStorePod");

    {
        IRenderFrameUtil::BackBufferConfiguration bb;
        bb.backBufferType = IRenderFrameUtil::BackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY;
        bb.present = false;
        bb.backBufferHandle = res.outputImageHandle;
        bb.gpuBufferHandle = res.cpuCopyBufferHandle;

        td.engine.context->GetRenderUtil().GetRenderFrameUtil().SetBackBufferConfiguration(bb);
    }

    res.byteArray = make_unique<ByteArray>(bufferDesc.byteSize);

    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.cpuCopyBufferHandle = {};
    tr.outputImageHandle = {};
    for (uint32_t idx = 0; idx < TEST_IMAGE_COUNT; ++idx) {
        tr.images[idx].imageHandle = {};
    }

    // TODO: check delete count

    tr.byteArray.reset();
}

RenderHandleReference CreateRenderNodeGraph(IRenderContext& context, const string_view fileName)
{
    IRenderNodeGraphManager& graphManager = context.GetRenderNodeGraphManager();
    auto& loader = graphManager.GetRenderNodeGraphLoader();
    if (auto const result = loader.Load(fileName); !result.error.empty()) {
        CORE_LOG_E("Loading render node graph failed: %s", result.error.c_str());
        return {};
    } else {
        return graphManager.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, result.desc);
    }
}

void TickTest(TestData& td, int32_t frameCountToTick)
{
    UTest::EngineResources& er = td.engine;

    // Tick for 4 frames and collect imaging for comparison
    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        er.engine->TickFrame();

        // Re-do images every frame -> if invalid command lists etc, images might have wrong colors
        for (uint32_t imageIdx = 0; imageIdx < TEST_IMAGE_COUNT; ++imageIdx) {
            auto& image = td.resources.images[imageIdx];
            const auto name = TEST_IMAGE_NAME + to_string(imageIdx);
            for (uint32_t newIdx = 0; newIdx < 3; ++newIdx) {
                const uint32_t col = td.GetTestColor();
                image.color = col;
                const uint32_t rgbData[4u] = { col, col, col, col };
                const auto rgbDataView =
                    array_view(reinterpret_cast<const uint8_t*>(rgbData), sizeof(uint32_t) * countof(rgbData));
                RenderHandleReference newHandle =
                    er.device->GetGpuResourceManager().Create(name, td.resources.imageDesc, rgbDataView);
                ASSERT_TRUE(
                    RenderHandleUtil::IsTheSameWithoutGeneration(newHandle.GetHandle(), image.imageHandle.GetHandle()));
            }
        }

        if (idx == frameCountToTick - 1) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore("RenderDataStoreDefaultGpuResourceDataCopy")) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = td.resources.cpuCopyBufferHandle;
                dataCopy.byteArray = td.resources.byteArray.get();
                dataStoreDataCopy->AddCopyOperation(dataCopy);
            }
        }

        const RenderHandleReference renderNodeGraphs[] = { td.renderNodeGraph };
        er.context->GetRenderer().RenderFrame({ renderNodeGraphs, 1u });

#ifdef __OHOS__
        if (td.engine.backend != DeviceBackendType::VULKAN) {
            er.device->WaitForIdle();
        }
#endif // __OHOS__

        if (idx == frameCountToTick - 1) {
            er.device->GetGpuResourceManager().WaitForIdleAndDestroyGpuResources();
        }

        // TODO:
        {
            // Gpu image test create and destroy in the same frame
            const uint32_t col = td.GetTestColor();
            const uint32_t rgbData[4u] = { col, col, col, col };
            const auto rgbDataView =
                array_view(reinterpret_cast<const uint8_t*>(rgbData), sizeof(uint32_t) * countof(rgbData));
            const auto name = TEST_IMAGE_NAME + to_string(1000 + idx);
            RenderHandleReference newHandle =
                er.device->GetGpuResourceManager().Create(name, td.resources.imageDesc, rgbDataView);
            newHandle = {};
            // Test for creating same resource over and over
            newHandle = er.device->GetGpuResourceManager().Create(name, td.resources.imageDesc, rgbDataView);
            newHandle = er.device->GetGpuResourceManager().Create(name, td.resources.imageDesc, rgbDataView);
            newHandle = {};
            // Prints core validation error
            newHandle = er.device->GetGpuResourceManager().Create(newHandle, td.resources.imageDesc);
            newHandle = er.device->GetGpuResourceManager().Create(newHandle, td.resources.imageDesc);
            newHandle = {};
            newHandle = {};
        }
        {
            // Gpu buffer test create and destroy in the same frame
            const uint32_t col = td.GetTestColor();
            const uint32_t rgbData[4u] = { col, col, col, col };
            const auto rgbDataView =
                array_view(reinterpret_cast<const uint8_t*>(rgbData), sizeof(uint32_t) * countof(rgbData));
            const auto name = TEST_BUFFER_NAME + to_string(1000 + idx);
            RenderHandleReference newHandle =
                er.device->GetGpuResourceManager().Create(name, td.resources.bufferDesc, rgbDataView);
            newHandle = {};
            // Test for creating same resource over and over
            newHandle = er.device->GetGpuResourceManager().Create(name, td.resources.bufferDesc, rgbDataView);
            newHandle = er.device->GetGpuResourceManager().Create(name, td.resources.bufferDesc, rgbDataView);
            newHandle = {};
            // Prints core validation error
            newHandle = er.device->GetGpuResourceManager().Create(newHandle, td.resources.bufferDesc);
            newHandle = er.device->GetGpuResourceManager().Create(newHandle, td.resources.bufferDesc);
            newHandle = {};
            newHandle = {};
        }
    }
}

void ValidateData(const TestData& td)
{
    const uint32_t byteOffset = (td.windowWidth * td.elementByteSize) / TEST_IMAGE_COUNT;
    for (uint32_t idx = 0; idx < TEST_IMAGE_COUNT; ++idx) {
        const uint32_t expectedColor = td.resources.images[idx].color;
        const uint8_t* dataPtr =
            (reinterpret_cast<uint8_t*>(td.resources.byteArray->GetData().data())) + byteOffset * idx;
        const uint32_t realColor = *(reinterpret_cast<const uint32_t*>(dataPtr));
        ASSERT_EQ(expectedColor, realColor);
    }
}

void TestGpuResourceManager(DeviceBackendType backend)
{
    TestData testData;
    testData.windowWidth = 4 * TEST_IMAGE_COUNT;
    testData.windowHeight = 4;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);

        testData.resources = CreateTestResources(testData);
        testData.renderNodeGraph =
            CreateRenderNodeGraph(*testData.engine.context, "test://renderNodeGraphGfxGpuResourceManagerTest.rng");
    }
    TickTest(testData, 10);
    ValidateData(testData);
#if (TEST_WRITE_PNG == 1)
    UTest::WritePng("./GfxGpuResourceManagerTest.png", testData.windowWidth, testData.windowHeight, 4,
        testData.resources.byteArray.get(), testData.windowWidth * 4);
#endif
    {
        testData.renderNodeGraph = {};
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GpuResourceManagerTestVulkan
 * @tc.desc: Tests IGpuResourceManager for resource creation and destruction on the fly in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest, GpuResourceManagerTestVulkan, testing::ext::TestSize.Level1)
{
    TestGpuResourceManager(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuResourceManagerTestOpenGL
 * @tc.desc: Tests IGpuResourceManager for resource creation and destruction on the fly in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest, GpuResourceManagerTestOpenGL, testing::ext::TestSize.Level1)
{
    TestGpuResourceManager(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
