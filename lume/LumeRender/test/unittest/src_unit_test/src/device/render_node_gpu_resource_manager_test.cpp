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

#include <device/gpu_resource_manager.h>
#include <util/render_util.h>

#include <render/intf_renderer.h>

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
void Validate(const UTest::EngineResources& er)
{
    GpuResourceManager& gpuResourceMgr = static_cast<GpuResourceManager&>(er.device->GetGpuResourceManager());
    RenderNodeGpuResourceManager renderNodeGpuResourceMgr { gpuResourceMgr };
    ASSERT_EQ(&gpuResourceMgr, &renderNodeGpuResourceMgr.GetGpuResourceManager());
    {
        auto handle = renderNodeGpuResourceMgr.Get({});
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle = renderNodeGpuResourceMgr.GetBufferHandle("invalidName");
        ASSERT_EQ(RenderHandle {}, handle);
    }
    {
        GpuBufferDesc desc;
        desc.byteSize = 16;
        desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        desc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
        auto handle = renderNodeGpuResourceMgr.Create(desc);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));
        GpuBufferDesc realDesc = renderNodeGpuResourceMgr.GetBufferDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.byteSize, realDesc.byteSize);
        ASSERT_EQ(desc.engineCreationFlags, realDesc.engineCreationFlags);
        ASSERT_EQ(desc.memoryPropertyFlags, realDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.usageFlags, realDesc.usageFlags);
        desc.usageFlags = CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        auto handle1 = renderNodeGpuResourceMgr.Create(desc);
        IGpuResourceManager::BufferHandleInfo bhInfo = { 0, CORE_BUFFER_USAGE_INDEX_BUFFER_BIT };
        vector<RenderHandle> handles;
        renderNodeGpuResourceMgr.GetBufferHandles(bhInfo, handles);
        uint32_t invalidCount = std::count(handles.begin(), handles.end(), RenderHandle {});
        // There's already 1 default buffer with all possible usage flags, so even though we created 2 buffers, theres
        // one more
        ASSERT_EQ(gpuResourceMgr.GetBufferCount(), 3); // 3: buffer count
        ASSERT_EQ(handles.size(), 3);
        ASSERT_EQ(invalidCount, 1);
    }
    {
        GpuBufferDesc desc;
        desc.byteSize = 16;
        desc.engineCreationFlags =
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
        desc.format = BASE_FORMAT_R8_UINT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT | CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        desc.usageFlags = CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
        uint8_t rawData[16] = { 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u };
        auto handle = renderNodeGpuResourceMgr.Create("buffer0", desc, { rawData, countof(rawData) });
        {
            const auto sameHandle = gpuResourceMgr.GetOrCreate("buffer0", desc);
            ASSERT_EQ(sameHandle.GetHandle().id, handle.GetHandle().id);
        }
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_EQ("buffer0", renderNodeGpuResourceMgr.GetName(handle.GetHandle()));
        ASSERT_TRUE(gpuResourceMgr.HasBuffer("buffer0"));
        ASSERT_FALSE(gpuResourceMgr.HasImage("buffer0"));
        ASSERT_FALSE(gpuResourceMgr.HasSampler("buffer0"));

        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));
        GpuBufferDesc realDesc = renderNodeGpuResourceMgr.GetBufferDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.byteSize, realDesc.byteSize);
        ASSERT_EQ(desc.engineCreationFlags, realDesc.engineCreationFlags);
        ASSERT_EQ(desc.memoryPropertyFlags, realDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.usageFlags, realDesc.usageFlags);
        er.context->GetRenderer().RenderFrame({});
        void* data = renderNodeGpuResourceMgr.MapBufferMemory(handle.GetHandle());
        ASSERT_NE(nullptr, data);
        uint8_t* uintData = reinterpret_cast<uint8_t*>(data);
        for (uint32_t i = 0; i < countof(rawData); ++i) {
            ASSERT_EQ(rawData[i], uintData[i]);
        }
        renderNodeGpuResourceMgr.UnmapBuffer(handle.GetHandle());
    }
    {
        GpuImageDesc desc;
        desc.height = 16;
        desc.width = 16;
        desc.depth = 1;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        desc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        auto handle = renderNodeGpuResourceMgr.Create(desc);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));
        GpuImageDesc realDesc = renderNodeGpuResourceMgr.GetImageDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.height, realDesc.height);
        ASSERT_EQ(desc.width, realDesc.width);
        ASSERT_EQ(desc.depth, realDesc.depth);
        ASSERT_EQ(desc.engineCreationFlags, realDesc.engineCreationFlags);
        ASSERT_EQ(desc.usageFlags, realDesc.usageFlags);
        ASSERT_EQ(desc.memoryPropertyFlags, realDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.imageType, realDesc.imageType);
        desc.usageFlags = CORE_IMAGE_USAGE_STORAGE_BIT;
        auto handle1 = renderNodeGpuResourceMgr.Create(desc);
        IGpuResourceManager::ImageHandleInfo ihInfo = { 0, CORE_IMAGE_USAGE_STORAGE_BIT };
        vector<RenderHandle> handles;
        renderNodeGpuResourceMgr.GetImageHandles(ihInfo, handles);
        uint32_t invalidCount = std::count(handles.begin(), handles.end(), RenderHandle {});
        ASSERT_EQ(gpuResourceMgr.GetImageCount(), 6);
        ASSERT_EQ(handles.size(), 6);
        ASSERT_EQ(invalidCount, 5);
    }
    {
        GpuImageDesc desc;
        desc.height = 4;
        desc.width = 4;
        desc.depth = 1;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        desc.format = BASE_FORMAT_R8_UINT;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        uint8_t rawData[16] = { 0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 11u, 12u, 13u, 14u, 15u };
        auto handle = renderNodeGpuResourceMgr.Create("image0", desc, { rawData, countof(rawData) });
        {
            const auto sameHandle = gpuResourceMgr.GetOrCreate("image0", desc);
            ASSERT_EQ(sameHandle.GetHandle().id, handle.GetHandle().id);
        }
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));

        ASSERT_FALSE(gpuResourceMgr.HasBuffer("image0"));
        ASSERT_TRUE(gpuResourceMgr.HasImage("image0"));
        ASSERT_FALSE(gpuResourceMgr.HasSampler("image0"));

        GpuImageDesc realDesc = renderNodeGpuResourceMgr.GetImageDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.height, realDesc.height);
        ASSERT_EQ(desc.width, realDesc.width);
        ASSERT_EQ(desc.depth, realDesc.depth);
        ASSERT_EQ(desc.engineCreationFlags, realDesc.engineCreationFlags);
        ASSERT_EQ(desc.usageFlags, realDesc.usageFlags);
        ASSERT_EQ(desc.memoryPropertyFlags, realDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.imageType, realDesc.imageType);
        auto props = renderNodeGpuResourceMgr.GetFormatProperties(handle.GetHandle());
        auto realProps = gpuResourceMgr.GetFormatProperties(handle.GetHandle());
        ASSERT_EQ(realProps.bufferFeatures, props.bufferFeatures);
        ASSERT_EQ(realProps.bytesPerPixel, props.bytesPerPixel);
        ASSERT_EQ(realProps.linearTilingFeatures, props.linearTilingFeatures);
        ASSERT_EQ(realProps.optimalTilingFeatures, props.optimalTilingFeatures);
    }
    {
        GpuSamplerDesc desc;
        desc.minFilter = Filter::CORE_FILTER_LINEAR;
        desc.magFilter = Filter::CORE_FILTER_NEAREST;
        desc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        desc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        desc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        desc.enableAnisotropy = true;
        auto handle = renderNodeGpuResourceMgr.Create("sampler0", desc);
        {
            const auto sameHandle = gpuResourceMgr.GetOrCreate("sampler0", desc);
            ASSERT_EQ(sameHandle.GetHandle().id, handle.GetHandle().id);
        }
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));

        ASSERT_FALSE(gpuResourceMgr.HasBuffer("sampler0"));
        ASSERT_FALSE(gpuResourceMgr.HasImage("sampler0"));
        ASSERT_TRUE(gpuResourceMgr.HasSampler("sampler0"));

        GpuSamplerDesc realDesc = renderNodeGpuResourceMgr.GetSamplerDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.minFilter, realDesc.minFilter);
        ASSERT_EQ(desc.magFilter, realDesc.magFilter);
        ASSERT_EQ(desc.addressModeU, realDesc.addressModeU);
        ASSERT_EQ(desc.addressModeV, realDesc.addressModeV);
        ASSERT_EQ(desc.addressModeW, realDesc.addressModeW);
        ASSERT_EQ(desc.enableAnisotropy, realDesc.enableAnisotropy);

        IGpuResourceManager::SamplerHandleInfo shInfo = { 0 };
        vector<RenderHandle> handles;
        renderNodeGpuResourceMgr.GetSamplerHandles(shInfo, handles);
        uint32_t invalidCount = std::count(handles.begin(), handles.end(), RenderHandle {});
        ASSERT_EQ(gpuResourceMgr.GetSamplerCount(), 8);
        ASSERT_EQ(handles.size(), 8);
        ASSERT_EQ(invalidCount, 0); // Theres no  field to filter out samplers
    }
    {
        GpuSamplerDesc desc;
        desc.minFilter = Filter::CORE_FILTER_LINEAR;
        desc.magFilter = Filter::CORE_FILTER_NEAREST;
        desc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        desc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        desc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
        desc.enableAnisotropy = true;
        auto handle = renderNodeGpuResourceMgr.Create(desc);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));
        GpuSamplerDesc realDesc = renderNodeGpuResourceMgr.GetSamplerDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.minFilter, realDesc.minFilter);
        ASSERT_EQ(desc.magFilter, realDesc.magFilter);
        ASSERT_EQ(desc.addressModeU, realDesc.addressModeU);
        ASSERT_EQ(desc.addressModeV, realDesc.addressModeV);
        ASSERT_EQ(desc.addressModeW, realDesc.addressModeW);
        ASSERT_EQ(desc.enableAnisotropy, realDesc.enableAnisotropy);
    }
    {
        GpuAccelerationStructureDesc desc;
        desc.accelerationStructureType = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        desc.bufferDesc.byteSize = 16;
        desc.bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        desc.bufferDesc.format = BASE_FORMAT_R8_SINT;
        desc.bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.bufferDesc.usageFlags = CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT |
                                     CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT;
        auto handle = renderNodeGpuResourceMgr.Create(desc);
        // device address added automatically by gpu resource mgr
        desc.bufferDesc.usageFlags |= CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;

        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));
        GpuAccelerationStructureDesc realDesc =
            renderNodeGpuResourceMgr.GetAccelerationStructureDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.accelerationStructureType, realDesc.accelerationStructureType);
        ASSERT_EQ(desc.bufferDesc.byteSize, realDesc.bufferDesc.byteSize);
        ASSERT_EQ(desc.bufferDesc.engineCreationFlags, realDesc.bufferDesc.engineCreationFlags);
        ASSERT_EQ(desc.bufferDesc.memoryPropertyFlags, realDesc.bufferDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.bufferDesc.usageFlags, realDesc.bufferDesc.usageFlags);
    }
    {
        GpuAccelerationStructureDesc desc;
        desc.accelerationStructureType = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        desc.bufferDesc.byteSize = 16;
        desc.bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        desc.bufferDesc.format = BASE_FORMAT_R8_SINT;
        desc.bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.bufferDesc.usageFlags = CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT |
                                     CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT |
                                     CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        auto handle = renderNodeGpuResourceMgr.Create("accStruct0", desc);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));
        GpuAccelerationStructureDesc realDesc =
            renderNodeGpuResourceMgr.GetAccelerationStructureDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.accelerationStructureType, realDesc.accelerationStructureType);
        ASSERT_EQ(desc.bufferDesc.byteSize, realDesc.bufferDesc.byteSize);
        ASSERT_EQ(desc.bufferDesc.engineCreationFlags, realDesc.bufferDesc.engineCreationFlags);
        ASSERT_EQ(desc.bufferDesc.memoryPropertyFlags, realDesc.bufferDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.bufferDesc.usageFlags, realDesc.bufferDesc.usageFlags);
    }
    {
        GpuAccelerationStructureDesc desc;
        desc.accelerationStructureType = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        desc.bufferDesc.byteSize = 16;
        desc.bufferDesc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        desc.bufferDesc.format = BASE_FORMAT_R8_SINT;
        desc.bufferDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.bufferDesc.usageFlags = CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT |
                                     CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT |
                                     CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        auto bufferHandle = renderNodeGpuResourceMgr.Create(desc);
        auto handle = renderNodeGpuResourceMgr.Create(bufferHandle, desc);
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
        ASSERT_TRUE(renderNodeGpuResourceMgr.IsGpuBuffer(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuImage(handle.GetHandle()));
        ASSERT_FALSE(renderNodeGpuResourceMgr.IsGpuSampler(handle.GetHandle()));
        GpuAccelerationStructureDesc realDesc =
            renderNodeGpuResourceMgr.GetAccelerationStructureDescriptor(handle.GetHandle());
        ASSERT_EQ(desc.accelerationStructureType, realDesc.accelerationStructureType);
        ASSERT_EQ(desc.bufferDesc.byteSize, realDesc.bufferDesc.byteSize);
        ASSERT_EQ(desc.bufferDesc.engineCreationFlags, realDesc.bufferDesc.engineCreationFlags);
        ASSERT_EQ(desc.bufferDesc.memoryPropertyFlags, realDesc.bufferDesc.memoryPropertyFlags);
        ASSERT_EQ(desc.bufferDesc.usageFlags, realDesc.bufferDesc.usageFlags);
    }
    {
        auto props = renderNodeGpuResourceMgr.GetFormatProperties(BASE_FORMAT_R16G16B16A16_SNORM);
        auto realProps = gpuResourceMgr.GetFormatProperties(BASE_FORMAT_R16G16B16A16_SNORM);
        ASSERT_EQ(realProps.bufferFeatures, props.bufferFeatures);
        ASSERT_EQ(realProps.bytesPerPixel, props.bytesPerPixel);
        ASSERT_EQ(realProps.linearTilingFeatures, props.linearTilingFeatures);
        ASSERT_EQ(realProps.optimalTilingFeatures, props.optimalTilingFeatures);
    }
    {
        auto& cache = renderNodeGpuResourceMgr.GetGpuResourceCache();
        auto& realCache = gpuResourceMgr.GetGpuResourceCache();
        ASSERT_EQ(&realCache, &cache);
    }
}
} // namespace

/**
 * @tc.name: RenderNodeGpuResourceManagerTest
 * @tc.desc: Tests RenderNodeGpuResourceManager by creating different resources, checking resource existance and
 * veryfying that the created resources are created with correct descriptions.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_RenderNodeGpuResourceManager, RenderNodeGpuResourceManagerTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    UTest::CreateEngineSetup(engine);
    Validate(engine);
    UTest::DestroyEngine(engine);
}
