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

#include <base/math/mathf.h>
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
static constexpr Math::UVec2 TEST_DATA_SIZE { 2, 2 };
static constexpr Math::UVec2 TEST_DATA_SIZE_1 { 16, 16 };
static constexpr uint32_t TEST_ELEMENT_BYTE_SIZE { 1 };
static constexpr uint8_t TEST_INPUT_IMAGE_COLOR[] = { 0x1, 0x9, 0xf, 0xff };
static constexpr uint32_t BYTE_SIZE = TEST_DATA_SIZE_1.x * TEST_DATA_SIZE_1.y * TEST_ELEMENT_BYTE_SIZE;
static uint8_t TEST_IMAGE_DATA[BYTE_SIZE];
static constexpr string_view TEST_IMAGE_NAME_0 { "TestImage0" };
static constexpr string_view TEST_IMAGE_NAME_1 { "TestImage1" };
static constexpr string_view TEST_IMAGE_NAME_2 { "TestImage2" };
static constexpr string_view TEST_IMAGE_NAME_3 { "TestImage3" };
static constexpr string_view TEST_IMAGE_NAME_4 { "TestImage4" };
static constexpr string_view TEST_BUFFER_NAME_0 { "TestBuffer0" };
static constexpr string_view TEST_BUFFER_NAME_1 { "TestBuffer1" };
static constexpr string_view TEST_BUFFER_NAME_2 { "TestBuffer2" };
static constexpr string_view TEST_BUFFER_NAME_3 { "TestBuffer3" };
static constexpr string_view TEST_BUFFER_NAME_4 { "TestBuffer4" };
static constexpr string_view TEST_BUFFER_NAME_5 { "TestBuffer5" };
static constexpr string_view TEST_BUFFER_NAME_6 { "TestBuffer6" };
static constexpr string_view TEST_BUFFER_NAME_7 { "TestBuffer7" };

constexpr const string_view RENDER_DATA_STORE_DEFAULT_STAGING = "RenderDataStoreDefaultStaging";
constexpr const string_view RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY = "RenderDataStoreDefaultGpuResourceDataCopy";

struct TestResources {
    RenderHandleReference copyBufferHandle0;
    RenderHandleReference copyBufferHandle1;
    RenderHandleReference copyBufferHandle2;
    RenderHandleReference copyBufferHandle3;
    RenderHandleReference copyBufferHandle4;
    RenderHandleReference copyBufferHandle5;
    RenderHandleReference copyBufferHandle6;
    RenderHandleReference copyBufferHandle7;
    RenderHandleReference testImageHandle0;
    RenderHandleReference testImageHandle1;
    RenderHandleReference testImageHandle2;
    RenderHandleReference testImageHandle3;
    RenderHandleReference testImageHandle4;

    unique_ptr<ByteArray> byteArray0;
    unique_ptr<ByteArray> byteArray1;
    unique_ptr<ByteArray> byteArray2;
    unique_ptr<ByteArray> byteArray3;
    unique_ptr<ByteArray> byteArray4;

    GpuImageDesc imageDesc;
};

struct TestData {
    UTest::EngineResources engine;
    TestResources resources;
};

TestResources CreateTestResources(UTest::EngineResources& er, bool useMipmap)
{
    TestResources res;
    // input image 0
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        const array_view<const uint8_t> dataView = { TEST_INPUT_IMAGE_COLOR, countof(TEST_INPUT_IMAGE_COLOR) };
        res.testImageHandle0 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_0, imageDesc, dataView);
    }
    // test image 1
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE.x;
        imageDesc.height = TEST_DATA_SIZE.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.testImageHandle1 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_1, imageDesc);
    }
    // test image 2
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE_1.x;
        imageDesc.height = TEST_DATA_SIZE_1.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags =
            CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS;
        if (useMipmap) {
            imageDesc.mipCount = 4;
        }
        imageDesc.format = BASE_FORMAT_R8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.testImageHandle2 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_2, imageDesc);
    }
    // test image 3
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE_1.x;
        imageDesc.height = TEST_DATA_SIZE_1.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.testImageHandle3 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_3, imageDesc);
    }
    // test image 4
    {
        GpuImageDesc imageDesc;
        imageDesc.width = TEST_DATA_SIZE_1.x;
        imageDesc.height = TEST_DATA_SIZE_1.y;
        imageDesc.depth = 1;
        imageDesc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        imageDesc.format = BASE_FORMAT_R8_UNORM;
        imageDesc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        imageDesc.imageType = CORE_IMAGE_TYPE_2D;
        imageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        imageDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        imageDesc.usageFlags = CORE_IMAGE_USAGE_TRANSFER_SRC_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        res.testImageHandle4 = er.device->GetGpuResourceManager().Create(TEST_IMAGE_NAME_4, imageDesc);
    }

    const uint32_t byteSize = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * TEST_ELEMENT_BYTE_SIZE;
    const uint32_t byteSize1 = TEST_DATA_SIZE_1.x * TEST_DATA_SIZE_1.y * TEST_ELEMENT_BYTE_SIZE;
    const uint32_t byteSize2 = byteSize1 / 4u;
    const vector<uint8_t> bufferDefaultData(byteSize, 1);
    // buffer 0
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle0 =
            er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_0, bufferDesc, { bufferDefaultData });
    }
    // buffer 1
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize;
        bufferDesc.engineCreationFlags =
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
            CORE_ENGINE_BUFFER_CREATION_MAP_OUTSIDE_RENDERER | CORE_ENGINE_BUFFER_CREATION_DEFERRED_DESTROY;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_SRC_BIT | CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle1 =
            er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_1, bufferDesc, { bufferDefaultData });
    }
    // buffer 2
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize1;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle2 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_2, bufferDesc);
    }
    // buffer 3
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize1;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT | CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle3 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_3, bufferDesc);
    }
    // buffer 4
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize1;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle4 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_4, bufferDesc);
    }
    // buffer 5
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize2;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        res.copyBufferHandle5 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_5, bufferDesc);
    }
    // buffer 6
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize1;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT | CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        res.copyBufferHandle6 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_6, bufferDesc);
    }
    // buffer 7
    {
        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = byteSize1;
        bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        bufferDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT | CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        res.copyBufferHandle7 = er.device->GetGpuResourceManager().Create(TEST_BUFFER_NAME_7, bufferDesc);
    }

    res.byteArray0 = make_unique<ByteArray>(byteSize);
    res.byteArray1 = make_unique<ByteArray>(byteSize1);
    res.byteArray2 = make_unique<ByteArray>(byteSize1);
    res.byteArray3 = make_unique<ByteArray>(byteSize2);
    res.byteArray4 = make_unique<ByteArray>(byteSize1);

    return res;
}

void DestroyTestResources(IDevice& device, TestResources& tr)
{
    auto& gpuResourceMgr = device.GetGpuResourceManager();
    tr.copyBufferHandle0 = {};
    tr.copyBufferHandle1 = {};
    tr.copyBufferHandle2 = {};
    tr.copyBufferHandle3 = {};
    tr.copyBufferHandle4 = {};
    tr.copyBufferHandle5 = {};
    tr.copyBufferHandle6 = {};
    tr.testImageHandle0 = {};
    tr.testImageHandle1 = {};
    tr.testImageHandle2 = {};
    tr.testImageHandle3 = {};
    tr.testImageHandle4 = {};

    tr.byteArray0.reset();
    tr.byteArray1.reset();
    tr.byteArray2.reset();
    tr.byteArray3.reset();
    tr.byteArray4.reset();
}

array_view<const uint8_t> GetTestImageData()
{
    for (size_t i = 0; i < TEST_DATA_SIZE_1.x; ++i) {
        for (size_t j = 0; j < TEST_DATA_SIZE_1.y; ++j) {
            TEST_IMAGE_DATA[i * TEST_DATA_SIZE_1.y + j] = ((i + j) % 2 == 0) ? 0x00 : 0xff;
        }
    }
    return { TEST_IMAGE_DATA, countof(TEST_IMAGE_DATA) };
}

void TickTest(TestData& td, int32_t frameCountToTick, bool useMipmap)
{
    UTest::EngineResources& er = td.engine;
    TestResources& tr = td.resources;

    for (int32_t idx = 0; idx < frameCountToTick; ++idx) {
        er.engine->TickFrame();

        if (idx == 1) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                {
                    ImageCopy imageCopy;
                    imageCopy.srcSubresource = { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                        1u };
                    imageCopy.srcOffset = { 0, 0, 0 };
                    imageCopy.dstSubresource = { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                        1u };
                    imageCopy.dstOffset = { 0, 0, 0 };
                    imageCopy.extent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };
                    dsStaging->CopyImageToImage(tr.testImageHandle0, tr.testImageHandle1, imageCopy);
                }
                {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = 0;
                    bufferImageCopy.bufferImageHeight = 0;
                    bufferImageCopy.imageSubresource =
                        ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                            1u };
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };
                    dsStaging->CopyImageToBuffer(tr.testImageHandle0, tr.copyBufferHandle0, bufferImageCopy);
                }
            }
        }

        if (idx == 2) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY)) {
                IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                dataCopy.gpuHandle = tr.copyBufferHandle0;
                dataCopy.byteArray = tr.byteArray0.get();
                dataStoreDataCopy->AddCopyOperation(dataCopy);
            }

            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                BufferImageCopy bufferImageCopy;
                bufferImageCopy.bufferOffset = 0;
                bufferImageCopy.bufferRowLength = 0;
                bufferImageCopy.bufferImageHeight = 0;
                bufferImageCopy.imageSubresource =
                    ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u };
                bufferImageCopy.imageOffset = { 0, 0, 0 };
                bufferImageCopy.imageExtent = { TEST_DATA_SIZE.x, TEST_DATA_SIZE.y, 1u };
                dsStaging->CopyImageToBuffer(tr.testImageHandle1, tr.copyBufferHandle1, bufferImageCopy);
            }
        }

        if (idx == 3) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE_1.x;
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE_1.y;
                    bufferImageCopy.imageSubresource =
                        ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                            1u };
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE_1.x, TEST_DATA_SIZE_1.y, 1u };
                    auto dataView = GetTestImageData();
                    dsStaging->CopyDataToImage(dataView, tr.testImageHandle2, bufferImageCopy);
                }
                {
                    BufferCopy bufferCopy;
                    bufferCopy.srcOffset = 0;
                    bufferCopy.dstOffset = 0;
                    bufferCopy.size = BYTE_SIZE;
                    auto dataView = GetTestImageData();
                    dsStaging->CopyDataToBufferOnCpu(dataView, tr.copyBufferHandle3, bufferCopy);
                }
                {
                    BufferCopy bufferCopy;
                    bufferCopy.srcOffset = 0;
                    bufferCopy.dstOffset = 0;
                    bufferCopy.size = BYTE_SIZE;
                    auto dataView = GetTestImageData();
                    dsStaging->CopyDataToBuffer(dataView, tr.copyBufferHandle6, bufferCopy);
                }
                {
                    dsStaging->ClearColorImage(tr.testImageHandle4, ClearColorValue(1.f, 0.f, 0.f, 1.f));
                }
            }
        }

        if (idx == 4) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE_1.x;
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE_1.y;
                    bufferImageCopy.imageSubresource =
                        ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                            1u };
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE_1.x, TEST_DATA_SIZE_1.y, 1u };
                    dsStaging->CopyImageToBuffer(tr.testImageHandle2, tr.copyBufferHandle2, bufferImageCopy);
                }
                {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE_1.x;
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE_1.y;
                    bufferImageCopy.imageSubresource =
                        ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                            1u };
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE_1.x, TEST_DATA_SIZE_1.y, 1u };
                    dsStaging->CopyBufferToImage(tr.copyBufferHandle3, tr.testImageHandle3, bufferImageCopy);
                }
                {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE_1.x;
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE_1.y;
                    bufferImageCopy.imageSubresource =
                        ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                            1u };
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE_1.x, TEST_DATA_SIZE_1.y, 1u };
                    dsStaging->CopyBufferToImage(tr.copyBufferHandle3, tr.testImageHandle4, { &bufferImageCopy, 1 });
                }
                if (useMipmap) {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE_1.x / 2u;
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE_1.y / 2u;
                    bufferImageCopy.imageSubresource =
                        ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 1, 0,
                            1u };
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE_1.x / 2u, TEST_DATA_SIZE_1.y / 2u, 1u };
                    dsStaging->CopyImageToBuffer(tr.testImageHandle2, tr.copyBufferHandle5, bufferImageCopy);
                }
            }
        }

        if (idx == 5) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY)) {
                {
                    IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                    dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                    dataCopy.gpuHandle = tr.copyBufferHandle2;
                    dataCopy.byteArray = tr.byteArray1.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
                if (useMipmap) {
                    IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                    dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                    dataCopy.gpuHandle = tr.copyBufferHandle5;
                    dataCopy.byteArray = tr.byteArray3.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
            }
            if (refcnt_ptr<IRenderDataStoreDefaultStaging> dsStaging =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_STAGING)) {
                {
                    BufferImageCopy bufferImageCopy;
                    bufferImageCopy.bufferOffset = 0;
                    bufferImageCopy.bufferRowLength = TEST_DATA_SIZE_1.x;
                    bufferImageCopy.bufferImageHeight = TEST_DATA_SIZE_1.y;
                    bufferImageCopy.imageSubresource =
                        ImageSubresourceLayers { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0,
                            1u };
                    bufferImageCopy.imageOffset = { 0, 0, 0 };
                    bufferImageCopy.imageExtent = { TEST_DATA_SIZE_1.x, TEST_DATA_SIZE_1.y, 1u };
                    dsStaging->CopyImageToBuffer(tr.testImageHandle3, tr.copyBufferHandle4, bufferImageCopy);
                }
                {
                    BufferCopy bufferCopy;
                    bufferCopy.srcOffset = 0;
                    bufferCopy.dstOffset = 0;
                    bufferCopy.size = BYTE_SIZE;
                    dsStaging->CopyBufferToBuffer(tr.copyBufferHandle6, tr.copyBufferHandle7, bufferCopy);
                }
            }
        }

        if (idx == 6) {
            auto& rdsMgr = er.context->GetRenderDataStoreManager();
            if (refcnt_ptr<IRenderDataStoreDefaultGpuResourceDataCopy> dataStoreDataCopy =
                    rdsMgr.GetRenderDataStore(RENDER_DATA_STORE_DEFAULT_RESOURCE_DATA_COPY)) {
                {
                    IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                    dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                    dataCopy.gpuHandle = tr.copyBufferHandle4;
                    dataCopy.byteArray = tr.byteArray2.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
                {
                    IRenderDataStoreDefaultGpuResourceDataCopy::GpuResourceDataCopy dataCopy;
                    dataCopy.copyType = IRenderDataStoreDefaultGpuResourceDataCopy::CopyType::WAIT_FOR_IDLE;
                    dataCopy.gpuHandle = tr.copyBufferHandle7;
                    dataCopy.byteArray = tr.byteArray4.get();
                    dataStoreDataCopy->AddCopyOperation(dataCopy);
                }
            }
        }

        auto renderStatus = er.context->GetRenderer().GetFrameStatus();
        const uint64_t rsIndex = er.context->GetRenderer().RenderFrame({});
        ASSERT_EQ(renderStatus.frontEndIndex, (rsIndex - 1));
        ASSERT_EQ(renderStatus.backEndIndex, (rsIndex - 1));
        ASSERT_EQ(renderStatus.presentIndex, (rsIndex - 1));

        renderStatus = er.context->GetRenderer().GetFrameStatus();
        ASSERT_EQ(renderStatus.frontEndIndex, (rsIndex));
        ASSERT_EQ(renderStatus.backEndIndex, (rsIndex));
        ASSERT_EQ(renderStatus.presentIndex, (rsIndex));
    }
}

void ValidateDataTest(const TestData& td, bool useMipmap)
{
    const uint32_t byteSize = TEST_DATA_SIZE.x * TEST_DATA_SIZE.y * TEST_ELEMENT_BYTE_SIZE;
    {
        const uint32_t blobByteSize = static_cast<uint32_t>(td.resources.byteArray0->GetData().size_bytes());
        const auto& dataView = td.resources.byteArray0->GetData();
        CORE_ASSERT(byteSize == blobByteSize);
        ASSERT_EQ(byteSize, blobByteSize);
        for (uint32_t idx = 0; idx < byteSize; ++idx) {
            const uint8_t expectedColor = TEST_INPUT_IMAGE_COLOR[idx];
            const uint8_t realColor = dataView[idx];
            ASSERT_EQ(expectedColor, realColor);
        }
    }
    {
        IGpuResourceManager& gpuResourceMgr = td.engine.device->GetGpuResourceManager();
        const GpuBufferDesc bufferDesc = gpuResourceMgr.GetBufferDescriptor(td.resources.copyBufferHandle1);
        const uint32_t bufferByteSize = bufferDesc.byteSize;
        const uint32_t minByteSize = BASE_NS::Math::min(byteSize, bufferByteSize);
        if (const uint8_t* bufferData =
                static_cast<uint8_t*>(gpuResourceMgr.MapBufferMemory(td.resources.copyBufferHandle1));
            bufferData) {
            for (uint32_t idx = 0; idx < minByteSize; ++idx) {
                const uint8_t expectedColor = TEST_INPUT_IMAGE_COLOR[idx];
                const uint8_t realColor = bufferData[idx];
                ASSERT_EQ(expectedColor, realColor);
            }
            gpuResourceMgr.UnmapBuffer(td.resources.copyBufferHandle1);
        } else {
            ASSERT_FALSE(false);
        }
    }
    {
        const auto& dataView = td.resources.byteArray1->GetData();
        for (size_t i = 0; i < TEST_DATA_SIZE_1.x; ++i) {
            for (size_t j = 0; j < TEST_DATA_SIZE_1.y; ++j) {
                const uint8_t expectedColor = ((i + j) % 2 == 0) ? 0x00 : 0xff;
                const uint8_t realColor = dataView[i * TEST_DATA_SIZE_1.y + j];
                ASSERT_EQ(expectedColor, realColor);
            }
        }
    }
    {
        const auto& dataView = td.resources.byteArray2->GetData();
        for (size_t i = 0; i < TEST_DATA_SIZE_1.x; ++i) {
            for (size_t j = 0; j < TEST_DATA_SIZE_1.y; ++j) {
                const uint8_t expectedColor = ((i + j) % 2 == 0) ? 0x00 : 0xff;
                const uint8_t realColor = dataView[i * TEST_DATA_SIZE_1.y + j];
                ASSERT_EQ(expectedColor, realColor);
            }
        }
    }
    if (useMipmap) {
        const auto& dataView = td.resources.byteArray3->GetData();
        for (size_t i = 0; i < TEST_DATA_SIZE_1.x / 2u; ++i) {
            for (size_t j = 0; j < TEST_DATA_SIZE_1.y / 2u; ++j) {
                const uint8_t expectedColor = 0x80;
                const uint8_t realColor = dataView[i * (TEST_DATA_SIZE_1.y / 2u) + j];
                constexpr int threshold = 1;
                ASSERT_LE(abs(static_cast<int>(expectedColor) - static_cast<int>(realColor)), threshold);
            }
        }
    }
    {
        const auto& dataView = td.resources.byteArray4->GetData();
        for (size_t i = 0; i < TEST_DATA_SIZE_1.x; ++i) {
            for (size_t j = 0; j < TEST_DATA_SIZE_1.y; ++j) {
                const uint8_t expectedColor = ((i + j) % 2 == 0) ? 0x00 : 0xff;
                const uint8_t realColor = dataView[i * TEST_DATA_SIZE_1.y + j];
                ASSERT_EQ(expectedColor, realColor);
            }
        }
    }
}

void TestGpuResourceCopy(DeviceBackendType backend, bool useMipmap)
{
    TestData testData;
    testData.engine.backend = backend;
    if (backend == DeviceBackendType::OPENGL) {
        testData.engine.createWindow = true;
    }
    {
        CreateEngineSetup(testData.engine);
        testData.resources = CreateTestResources(testData.engine, useMipmap);
    }
    TickTest(testData, 7, useMipmap);
    ValidateDataTest(testData, useMipmap);
    {
        DestroyTestResources(*testData.engine.device, testData.resources);
        DestroyEngine(testData.engine);
    }
}
} // namespace

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GpuResourceCopyTestVulkan
 * @tc.desc: Tests for GPU image-image, image-buffer, buffer-image, buffer-buffer with IRenderDataStoreDefaultStaging
 * and IRenderDataStoreDefaultGpuResourceDataCopy. Also tests staging operations: copying data from GPU VRAM to CPU RAM,
 * and data upload from CPU RAM to GPU VRAM. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, GpuResourceCopyTestVulkan, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopy(DeviceBackendType::VULKAN, false);
}

/**
 * @tc.name: GpuResourceMipmapCopyTestVulkan
 * @tc.desc:  * @tc.name: GpuResourceCopyTestVulkan
 * @tc.desc: Tests for GPU image-image, image-buffer, buffer-image, buffer-buffer with IRenderDataStoreDefaultStaging
 * and IRenderDataStoreDefaultGpuResourceDataCopy. Also tests staging operations: copying data from GPU VRAM to CPU RAM,
 * and data upload from CPU RAM to GPU VRAM. The test takes into account mipmap copies. Backend is Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, GpuResourceMipmapCopyTestVulkan, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopy(DeviceBackendType::VULKAN, true);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuResourceCopyTestOpenGL
 * @tc.desc: Tests for GPU image-image, image-buffer, buffer-image, buffer-buffer with IRenderDataStoreDefaultStaging
 * and IRenderDataStoreDefaultGpuResourceDataCopy. Also tests staging operations: copying data from GPU VRAM to CPU RAM,
 * and data upload from CPU RAM to GPU VRAM. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, GpuResourceCopyTestOpenGL, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopy(UTest::GetOpenGLBackend(), false);
}
/**
 * @tc.name: GpuResourceMipmapCopyTestOpenGL
 * @tc.desc: Tests for GPU image-image, image-buffer, buffer-image, buffer-buffer with IRenderDataStoreDefaultStaging
 * and IRenderDataStoreDefaultGpuResourceDataCopy. Also tests staging operations: copying data from GPU VRAM to CPU RAM,
 * and data upload from CPU RAM to GPU VRAM. The test takes into account mipmap copies. Backend is OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(API_GfxGpuResourceCopyTest, GpuResourceMipmapCopyTestOpenGL, testing::ext::TestSize.Level1)
{
    TestGpuResourceCopy(UTest::GetOpenGLBackend(), true);
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
