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

using namespace BASE_NS;
using CORE_NS::IEngine;
using namespace RENDER_NS;

#define TEST_WRITE_PNG 0

namespace {
static constexpr Math::UVec2 TEST_DATA_SIZE { 2, 2 };
static constexpr Math::UVec2 TEST_OUTPUT_SIZE { 2, 2 };
// red | green
// red | green
static constexpr uint32_t TEST_IMAGE_COLOR[] = { 0xFF0000ff, 0xFF00ff00, 0xFF0000ff, 0xFF00ff00 };
static constexpr string_view TEST_IMAGE_NAME { "TestImage" };
static constexpr string_view TEST_BUFFER_NAME { "TestBuffer" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";

struct TestResources {
    RenderHandleReference cpuCopyBufferHandle;
    RenderHandleReference outputImageHandle;
    RenderHandleReference testImageHandle;
    RenderHandleReference optionalGpuMapBufferHandle;

    unique_ptr<ByteArray> byteArray;
    GpuImageDesc imageDesc;
};

struct TestData {
    UTest::EngineResources engine;
    RenderHandleReference renderNodeGraph;
    TestResources resources;

    uint32_t windowWidth { 0u };
    uint32_t windowHeight { 0u };
    uint32_t elementByteSize { 4u };
};

TestResources CreateTestResources2(TestData& td)
{
    TestResources res;
    // output
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

    GpuBufferDesc bufferDesc;
    bufferDesc.byteSize = td.windowWidth * td.windowHeight * td.elementByteSize;
    bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const vector<uint8_t> bufferDefaultData(bufferDesc.byteSize, 0);
    res.cpuCopyBufferHandle =
        td.engine.device->GetGpuResourceManager().Create("TestOutputBuffer", bufferDesc, { bufferDefaultData });

    res.byteArray = make_unique<ByteArray>(bufferDesc.byteSize);
#if (TEST_CREATE_WINDOW != 1)
    IRenderFrameUtil::BackBufferConfiguration bbc;
    bbc.backBufferType = IRenderFrameUtil::BackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY;
    bbc.present = false;
    bbc.backBufferHandle = res.outputImageHandle;
    bbc.gpuBufferHandle = res.cpuCopyBufferHandle;
    td.engine.context->GetRenderUtil().GetRenderFrameUtil().SetBackBufferConfiguration(bbc);
#endif
    return res;
}

TestResources CreateTestResources3(TestData& td)
{
    TestResources res;
    auto& gpuResourceMgr = td.engine.device->GetGpuResourceManager();
    // output
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
        res.outputImageHandle = gpuResourceMgr.Create("OutputImage", imageDesc);
    }
    // input buffers
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = bufferDesc.byteSize = td.windowWidth * td.windowHeight * td.elementByteSize;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
                                         CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY |
                                         CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        res.optionalGpuMapBufferHandle = gpuResourceMgr.Create("TestMapBuffer", bufferDesc);

        if (void* mapData = gpuResourceMgr.MapBufferMemory(res.optionalGpuMapBufferHandle); mapData) {
            uint32_t* data = reinterpret_cast<uint32_t*>(mapData);
            for (uint32_t idx = 0; idx < (bufferDesc.byteSize / 4); ++idx) {
                *(data + idx) = TEST_IMAGE_COLOR[idx];
            }
            gpuResourceMgr.UnmapBuffer(res.optionalGpuMapBufferHandle);
        } else {
            CORE_LOG_E("CreateTestResources3: ERROR mapping data");
        }
    }

    GpuBufferDesc bufferDesc;
    bufferDesc.byteSize = td.windowWidth * td.windowHeight * td.elementByteSize;
    bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
    bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    const vector<uint8_t> bufferDefaultData(bufferDesc.byteSize, 0);
    res.cpuCopyBufferHandle =
        td.engine.device->GetGpuResourceManager().Create("TestOutputBuffer", bufferDesc, { bufferDefaultData });

    res.byteArray = make_unique<ByteArray>(bufferDesc.byteSize);
#if (TEST_CREATE_WINDOW != 1)
    {
        IRenderFrameUtil::BackBufferConfiguration bbc;
        bbc.backBufferType = IRenderFrameUtil::BackBufferConfiguration::BackBufferType::GPU_IMAGE_BUFFER_COPY;
        bbc.present = false;
        bbc.backBufferHandle = res.outputImageHandle;
        bbc.gpuBufferHandle = res.cpuCopyBufferHandle;
        td.engine.context->GetRenderUtil().GetRenderFrameUtil().SetBackBufferConfiguration(bbc);
    }
#endif
    return res;
}

void CreateScaledImage(TestData& td, const Math::UVec2& size)
{
    // input images
    {
        GpuImageDesc desc;
        desc.width = size.x;
        desc.height = size.y;
        desc.depth = 1;
        desc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_SCALE; // NOTE!
        const auto rgbDataView = array_view(
            reinterpret_cast<const uint8_t*>(TEST_IMAGE_COLOR), sizeof(uint32_t) * countof(TEST_IMAGE_COLOR));
        const auto name = TEST_IMAGE_NAME;
        const BufferImageCopy bufferImageCopy {
            0,                                                                        // bufferOffset
            TEST_DATA_SIZE.x,                                                         // bufferRowLength
            TEST_DATA_SIZE.y,                                                         // bufferImageHeight
            { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1 }, // imageSubresource
            { 0, 0, 0 },                                                              // imageOffset
            { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u },                               // imageExtent
        };
        td.resources.testImageHandle = td.engine.device->GetGpuResourceManager().Create(
            name, desc, rgbDataView, array_view<const BufferImageCopy>(&bufferImageCopy, 1u));
    }
}

void CreateImageTest3(TestData& td, const Math::UVec2& size)
{
    // input images
    {
        GpuImageDesc desc;
        desc.width = size.x;
        desc.height = size.y;
        desc.depth = 1;
        desc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        const auto name = TEST_IMAGE_NAME;
        td.resources.testImageHandle = td.engine.device->GetGpuResourceManager().Create(name, desc);
    }
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.cpuCopyBufferHandle = {};
    tr.outputImageHandle = {};
    tr.testImageHandle = {};
    tr.optionalGpuMapBufferHandle = {};

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
#if (TEST_CREATE_WINDOW == 1)
        if (!RunTestMessageLoop())
            break;
#endif
        er.engine->TickFrame();

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
    }
}

Math::Vec4 GetLinearSaturateColor(const uint32_t channel)
{
    return {
        static_cast<float>((channel & 0x000000ff) >> 0) * (1.0f / 255.0f),
        static_cast<float>((channel & 0x0000ff00) >> 8) * (1.0f / 255.0f),
        static_cast<float>((channel & 0x00FF0000) >> 16) * (1.0f / 255.0f),
        static_cast<float>((channel & 0xFF000000) >> 24) * (1.0f / 255.0f),
    };
}

bool OwnCloseEnough(const float expected, const float real)
{
    return (expected - real < (1.0f / 255.0f));
}

void ValidateData1x1(const TestData& td)
{
    const Math::Vec4 fTestColor0 = GetLinearSaturateColor(TEST_IMAGE_COLOR[0]);
    const Math::Vec4 fTestColor1 = GetLinearSaturateColor(TEST_IMAGE_COLOR[1]);
    const Math::Vec4 fExpectedColor = (fTestColor0 + fTestColor1) * 0.5f;

    const uint8_t* dataPtr = (reinterpret_cast<uint8_t*>(td.resources.byteArray->GetData().data()));
    const uint32_t* colorPtr = (reinterpret_cast<const uint32_t*>(dataPtr));
    const Math::Vec4 fColor0 = GetLinearSaturateColor(colorPtr[0]);
    const Math::Vec4 fColor1 = GetLinearSaturateColor(colorPtr[1]);
    const Math::Vec4 fColor2 = GetLinearSaturateColor(colorPtr[2]);
    const Math::Vec4 fColor3 = GetLinearSaturateColor(colorPtr[3]);
    for (uint32_t idx = 0; idx < 4u; ++idx) {
        ASSERT_TRUE(OwnCloseEnough(fExpectedColor[idx], fColor0[idx]));
        ASSERT_TRUE(OwnCloseEnough(fExpectedColor[idx], fColor1[idx]));
        ASSERT_TRUE(OwnCloseEnough(fExpectedColor[idx], fColor2[idx]));
        ASSERT_TRUE(OwnCloseEnough(fExpectedColor[idx], fColor3[idx]));
    }
}

void ValidateData4x4(const TestData& td)
{
    const Math::Vec4 fTestColor0 = GetLinearSaturateColor(TEST_IMAGE_COLOR[0]);
    const Math::Vec4 fTestColor1 = GetLinearSaturateColor(TEST_IMAGE_COLOR[1]);

    // 4x4 pixel colors (row major) c0 left
    const Math::Vec4 c0 = fTestColor0;
    const Math::Vec4 c1 = (c0 + ((fTestColor0 + fTestColor1) * 0.5f)) * 0.5f;
    const Math::Vec4 c3 = fTestColor1;
    const Math::Vec4 c2 = (c3 + ((fTestColor0 + fTestColor1) * 0.5f)) * 0.5f;

    const Math::Vec4 fTestLeft = (c0 + c1) * 0.5f;
    const Math::Vec4 fTestRight = (c2 + c3) * 0.5f;

    const uint8_t* dataPtr = (reinterpret_cast<uint8_t*>(td.resources.byteArray->GetData().data()));
    const uint32_t* colorPtr = (reinterpret_cast<const uint32_t*>(dataPtr));
    const Math::Vec4 fColor0 = GetLinearSaturateColor(colorPtr[0]);
    const Math::Vec4 fColor1 = GetLinearSaturateColor(colorPtr[1]);
    const Math::Vec4 fColor2 = GetLinearSaturateColor(colorPtr[2]);
    const Math::Vec4 fColor3 = GetLinearSaturateColor(colorPtr[3]);
    for (uint32_t idx = 0; idx < 4u; ++idx) {
        ASSERT_TRUE(OwnCloseEnough(fTestLeft[idx], fColor0[idx]));
        ASSERT_TRUE(OwnCloseEnough(fTestRight[idx], fColor1[idx]));
        ASSERT_TRUE(OwnCloseEnough(fTestLeft[idx], fColor2[idx]));
        ASSERT_TRUE(OwnCloseEnough(fTestRight[idx], fColor3[idx]));
    }
}

void ValidateDataTest3(const TestData& td)
{
    constexpr uint32_t byteOffset = 4u;
    for (uint32_t idx = 0; idx < 4; ++idx) {
        const uint32_t expectedColor = TEST_IMAGE_COLOR[idx];
        const uint8_t* dataPtr =
            (reinterpret_cast<uint8_t*>(td.resources.byteArray->GetData().data())) + byteOffset * idx;
        const uint32_t realColor = *(reinterpret_cast<const uint32_t*>(dataPtr));
        ASSERT_EQ(expectedColor, realColor);
    }
}

void TestResourceManager2(DeviceBackendType backend)
{
    TestData testData;
    testData.windowWidth = TEST_OUTPUT_SIZE.x;
    testData.windowHeight = TEST_OUTPUT_SIZE.y;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources2(testData);
        testData.renderNodeGraph =
            CreateRenderNodeGraph(*testData.engine.context, "test://renderNodeGraphGfxGpuResourceManagerTest2.rng");
    }
    CreateScaledImage(testData, { 1u, 1u });
    TickTest(testData, 1);
    ValidateData1x1(testData);
#if (TEST_WRITE_PNG == 1)
    test::WritePng("./GfxGpuResourceManagerTest2_1x1.png", testData.windowWidth, testData.windowHeight, 4,
        testData.resources.dataBlob->Data(), testData.windowWidth * 4);
#endif
    CreateScaledImage(testData, { 4u, 4u });
    TickTest(testData, 1);
    ValidateData4x4(testData);
#if (TEST_WRITE_PNG == 1)
    test::WritePng("./GfxGpuResourceManagerTest2_4x4.png", testData.windowWidth, testData.windowHeight, 4,
        testData.resources.dataBlob->Data(), testData.windowWidth * 4);
#endif
    {
        testData.renderNodeGraph = {};
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}

void TestResourceManager3(DeviceBackendType backend)
{
    TestData testData;
    testData.windowWidth = TEST_OUTPUT_SIZE.x;
    testData.windowHeight = TEST_OUTPUT_SIZE.y;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources3(testData);
        testData.renderNodeGraph =
            CreateRenderNodeGraph(*testData.engine.context, "test://renderNodeGraphGfxGpuResourceManagerTest2.rng");
    }
    CreateImageTest3(testData, { 2u, 2u });
    {
        refcnt_ptr<IRenderDataStoreDefaultStaging> staging =
            testData.engine.context->GetRenderDataStoreManager().GetRenderDataStore(
                RENDER_DATA_STORE_DEFAULT_STAGING.data());
        const BufferImageCopy bufferImageCopy {
            0,                  // bufferOffset
            TEST_OUTPUT_SIZE.x, // bufferRowLength
            TEST_OUTPUT_SIZE.y, // bufferImageHeight
            {
                CORE_IMAGE_ASPECT_COLOR_BIT,              // imageAspectFlags
                0,                                        // mipLevel
                0,                                        // baseArrayLayer
                1u,                                       // layerCount
            },                                            // imageSubresource
            {},                                           // imageOffset
            { TEST_OUTPUT_SIZE.x, TEST_OUTPUT_SIZE.y, 1 } // imageExtend
        };
        staging->CopyBufferToImage(
            testData.resources.optionalGpuMapBufferHandle, testData.resources.testImageHandle, bufferImageCopy);
    }
    TickTest(testData, 1);
    ValidateDataTest3(testData);
#if (TEST_WRITE_PNG == 1)
    test::WritePng("./GfxGpuResourceManagerTest3_4x4.png", testData.windowWidth, testData.windowHeight, 4,
        testData.resources.dataBlob->Data(), testData.windowWidth * 4);
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
 * @tc.name: GpuResourceManagerTestVulkan2
 * @tc.desc: Tests IGpuResourceManager for image creation using scaling: image dimentions inferred from buffer data.
 * Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest2, GpuResourceManagerTestVulkan2, testing::ext::TestSize.Level1)
{
    TestResourceManager2(DeviceBackendType::VULKAN);
}
/**
 * @tc.name: GpuResourceManagerTestVulkan3
 * @tc.desc: Tests IGpuResourceManager for image creation by uploading image data explcitly after image creation via
 * IRenderDataStoreDefaultStaging. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest3, GpuResourceManagerTestVulkan3, testing::ext::TestSize.Level1)
{
    TestResourceManager3(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuResourceManagerTestOpenGL2
 * @tc.desc: Tests IGpuResourceManager for image creation using scaling: image dimentions inferred from buffer data.
 * Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest2, GpuResourceManagerTestOpenGL2, testing::ext::TestSize.Level1)
{
    TestResourceManager2(UTest::GetOpenGLBackend());
}
/**
 * @tc.name: GpuResourceManagerTestOpenGL3
 * @tc.desc: Tests IGpuResourceManager for image creation by uploading image data explcitly after image creation via
 * IRenderDataStoreDefaultStaging. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceManagerTest3, GpuResourceManagerTestOpenGL3, testing::ext::TestSize.Level1)
{
    TestResourceManager3(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
