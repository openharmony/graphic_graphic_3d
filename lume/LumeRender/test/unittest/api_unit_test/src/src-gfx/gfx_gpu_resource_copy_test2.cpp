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
static constexpr Math::UVec2 TEST_DATA_SIZE { 4u, 4u };
static constexpr uint32_t NUM_LAYERS { 6 };
static constexpr uint32_t TEST_ELEMENT_BYTE_SIZE[2] { 1u, 1u };
static constexpr uint32_t NUM_BLOCKS = 1u;
static constexpr uint32_t BLOCK_BYTE_SIZE = 8u;
static constexpr string_view TEST_IMAGE_NAME_0 { "TestImage0" };
static constexpr string_view TEST_IMAGE_NAME_1 { "TestImage1" };
static constexpr string_view TEST_BUFFER_NAME_0 { "TestBuffer0" };
static constexpr string_view TEST_BUFFER_NAME_1 { "TestBuffer1" };
static constexpr string_view TEST_BUFFER_NAME_2 { "TestBuffer2" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference copyBufferHandle0;
    RenderHandleReference copyBufferHandle1;
    RenderHandleReference copyBufferHandle2;
    RenderHandleReference testImageHandle0;
    RenderHandleReference testImageHandle1;

    unique_ptr<ByteArray> byteArray0;
    unique_ptr<ByteArray> byteArray1;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

bool isOpenGLBackend(DeviceBackendType type)
{
    return type == DeviceBackendType::OPENGL || type == DeviceBackendType::OPENGLES;
}

TestResources CreateTestResources(UTest::EngineResources& er, bool compressed)
{
    TestResources res;
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        if (!compressed) {
            imageDesc.format = BASE_FORMAT_R8_UNORM;
        } else {
            imageDesc.format = BASE_FORMAT_EAC_R11_UNORM_BLOCK;
        }
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D_ARRAY;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageDesc.layerCount = NUM_LAYERS;
        res.testImageHandle0 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_0, imageDesc);
    }
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.createFlags = CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        if (!compressed) {
            imageDesc.format = BASE_FORMAT_R8_UNORM;
        } else {
            imageDesc.format = BASE_FORMAT_EAC_R11_UNORM_BLOCK;
        }
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_CUBE;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageDesc.layerCount = NUM_LAYERS;
        res.testImageHandle1 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_1, imageDesc);
    }

    const uint32_t byteSize = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * TEST_ELEMENT_BYTE_SIZE[compressed] * NUM_LAYERS;
    const vector<uint8_t> bufferDefaultData(byteSize, 100u);
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle0 =
            er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_0, bufferDesc, { bufferDefaultData });
    }
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS |
                                         CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
                                         CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle1 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_1, bufferDesc);
    }
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS |
                                         CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
                                         CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle2 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_1, bufferDesc);
    }
    res.byteArray0 = make_unique<ByteArray>(byteSize);
    res.byteArray1 = make_unique<ByteArray>(byteSize);
    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.copyBufferHandle0 = {};
    tr.copyBufferHandle1 = {};
    tr.testImageHandle0 = {};
    tr.testImageHandle1 = {};

    tr.byteArray0.reset();
}

void TickTest(TestData& td, int32_t frameCountToTick, bool compressed)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        er.engine->TickFrame();

        auto& rdsMgr = er.context->GetRenderDataStoreManager();
        refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
            rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING);
        refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
            rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY);

        if (idx == 1) {
            if (dsStaging) {
                BufferImageCopy bufferImageCopy;
                bufferImageCopy.bufferOffset = 0;
                bufferImageCopy.bufferRowLength = TEST_DATA_SIZE.x * TEST_ELEMENT_BYTE_SIZE[compressed];
                bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE.y;
                bufferImageCopy.imageSubresource.imageAspectFlags = ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT;
                bufferImageCopy.imageSubresource.mipLevel = 0u;
                bufferImageCopy.imageSubresource.baseArrayLayer = 0u;
                bufferImageCopy.imageSubresource.layerCount = NUM_LAYERS;
                bufferImageCopy.imageOffset = { 0, 0, 0 };
                bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };
                dsStaging->CopyBufferToImage(tr.copyBufferHandle0, tr.testImageHandle0, bufferImageCopy);
            }
        }

        if (idx == 2) {
            if (dsStaging) {
                BufferImageCopy bufferImageCopy;
                bufferImageCopy.bufferOffset = 0;
                bufferImageCopy.bufferRowLength = TEST_DATA_SIZE.x * TEST_ELEMENT_BYTE_SIZE[compressed];
                bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE.y;
                bufferImageCopy.imageSubresource.imageAspectFlags = ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT;
                bufferImageCopy.imageSubresource.mipLevel = 0u;
                bufferImageCopy.imageSubresource.baseArrayLayer = 0u;
                bufferImageCopy.imageSubresource.layerCount = NUM_LAYERS;
                bufferImageCopy.imageOffset = { 0, 0, 0 };
                bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };
                dsStaging->CopyBufferToImage(tr.copyBufferHandle0, tr.testImageHandle1, bufferImageCopy);
            }
        }

        if (idx == 3) {
            if (dsStaging) {
                if (!isOpenGLBackend(er.backend) || !compressed) {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE.x * TEST_ELEMENT_BYTE_SIZE[compressed];
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE.y;
                    bufferImageCopy.imageSubresource.imageAspectFlags =
                        ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT;
                    bufferImageCopy.imageSubresource.mipLevel = 0u;
                    bufferImageCopy.imageSubresource.baseArrayLayer = 0u;
                    if (isOpenGLBackend(er.backend)) {
                        bufferImageCopy.imageSubresource.layerCount = 1;
                    } else {
                        bufferImageCopy.imageSubresource.layerCount = NUM_LAYERS;
                    }
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };
                    dsStaging->CopyImageToBuffer(tr.testImageHandle0, tr.copyBufferHandle1, bufferImageCopy);
                }
                if (!isOpenGLBackend(er.backend) || !compressed) {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE.x * TEST_ELEMENT_BYTE_SIZE[compressed];
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE.y;
                    bufferImageCopy.imageSubresource.imageAspectFlags =
                        ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT;
                    bufferImageCopy.imageSubresource.mipLevel = 0u;
                    bufferImageCopy.imageSubresource.baseArrayLayer = 0u;
                    if (isOpenGLBackend(er.backend)) {
                        bufferImageCopy.imageSubresource.layerCount = 1;
                    } else {
                        bufferImageCopy.imageSubresource.layerCount = NUM_LAYERS;
                    }
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };
                    dsStaging->CopyImageToBuffer(tr.testImageHandle1, tr.copyBufferHandle2, bufferImageCopy);
                }
            }
        }

        if (idx == 4) {
            if (dataStoreDataCopy) {
                {
                    IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                    dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                    dataCopy.gpuHandle = tr.copyBufferHandle1;
                    dataCopy.byteArray = tr.byteArray0.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
                {
                    IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                    dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                    dataCopy.gpuHandle = tr.copyBufferHandle2;
                    dataCopy.byteArray = tr.byteArray1.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
            }
        }

        er.context->GetRenderer().RenderFrame({});
    }
}

void ValidateDataTest(const TestData& td, bool compressed)
{
    const uint32_t byteSize = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * TEST_ELEMENT_BYTE_SIZE[compressed] * NUM_LAYERS;
    {
        const uint32_t blobByteSize = static_cast<uint32_t>(td.resources.byteArray0->GetData().size_bytes());
        const auto& dataView = td.resources.byteArray0->GetData();
        ASSERT_EQ(byteSize, blobByteSize);
        uint32_t checkTo = compressed ? NUM_BLOCKS * BLOCK_BYTE_SIZE * NUM_LAYERS : byteSize;
        if (td.engine.backend == DeviceBackendType::VULKAN) {
            for (uint32_t idx = 0; idx < checkTo; ++idx) {
                const uint8_t expectedColor = 100u;
                const uint8_t realColor = dataView[idx];
                if (expectedColor != realColor) {
                    CORE_LOG_E("Index %d", idx);
                }
                ASSERT_EQ(expectedColor, realColor);
            }
        }
    }
    {
        const uint32_t blobByteSize = static_cast<uint32_t>(td.resources.byteArray1->GetData().size_bytes());
        const auto& dataView = td.resources.byteArray1->GetData();
        ASSERT_EQ(byteSize, blobByteSize);
        uint32_t checkTo = compressed ? NUM_BLOCKS * BLOCK_BYTE_SIZE * NUM_LAYERS : byteSize;
        if (td.engine.backend != DeviceBackendType::VULKAN) {
            checkTo = compressed ? 0 : byteSize / NUM_LAYERS;
        }
        for (uint32_t idx = 0; idx < checkTo; ++idx) {
            const uint8_t expectedColor = 100u;
            const uint8_t realColor = dataView[idx];
            ASSERT_EQ(expectedColor, realColor);
        }
    }
}

void TestGpuResourceCopyLayers(DeviceBackendType backend, bool compressed)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, compressed);
    }
    TickTest(testData, 5, compressed);
    ValidateDataTest(testData, compressed);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GpuResourceCopyLayersTestVulkan
 * @tc.desc: Tests mainly for GPU buffer-image, image-buffer copies with array layers. Readback on CPU to validate that
 * all copies were successful. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, GpuResourceCopyLayersTestVulkan, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopyLayers(DeviceBackendType::VULKAN, false);
}
#ifdef DISABLED_TESTS_ON
/**
 * @tc.name: GpuResourceCopyLayersCompressedTestVulkan
 * @tc.desc: Tests mainly for GPU buffer-image, image-buffer copies with array layers and compressed image formats.
 * Readback on CPU to validate that all copies were successful. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, DISABLED_GpuResourceCopyLayersCompressedTestVulkan, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopyLayers(DeviceBackendType::VULKAN, true);
}
#endif // DISABLED_TESTS_ON
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuResourceCopyLayersTestOpenGL
 * @tc.desc: Tests mainly for GPU buffer-image, image-buffer copies with array layers. Readback on CPU to validate that
 * all copies were successful. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, GpuResourceCopyLayersTestOpenGL, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopyLayers(UTest::GetOpenGLBackend(), false);
}
/**
 * @tc.name: GpuResourceCopyLayersCompressedTestOpenGL
 * @tc.desc: Tests mainly for GPU buffer-image, image-buffer copies with array layers and compressed image formats.
 * Readback on CPU to validate that all copies were successful. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, GpuResourceCopyLayersCompressedTestOpenGL, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopyLayers(UTest::GetOpenGLBackend(), true);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
