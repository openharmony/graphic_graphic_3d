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

#include <device/device.h>
#include <device/gpu_resource_manager.h>

#include <core/image/intf_image_loader_manager.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/device_vk.h>
#include <vulkan/gpu_image_vk.h>
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#include <gles/device_gles.h>
#include <gles/gpu_image_gles.h>
#endif // RENDER_HAS_GL_BACKEND

#undef LoadImage

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

/**
 * @tc.name: DefaultCreationFlagsTest
 * @tc.desc: Tests IGpuResourceManager by creating a buffer and image with default creation flags and verifying the
 * default flags are added when creating the resource.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, DefaultCreationFlagsTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources er;
    // er.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(er);
    ASSERT_TRUE(er.device != nullptr);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();

    // Add default flags for buffer.
    {
        GpuBufferDesc inputDesc;
        inputDesc.byteSize = 4;
        inputDesc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        inputDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        inputDesc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        RenderHandleReference handle = gpuResourceMgr.Create(inputDesc);
        GpuBufferDesc currDesc = gpuResourceMgr.GetBufferDescriptor(handle);
        ASSERT_TRUE(inputDesc.byteSize == currDesc.byteSize);
        ASSERT_TRUE(inputDesc.usageFlags == currDesc.usageFlags);
        ASSERT_TRUE(inputDesc.memoryPropertyFlags == currDesc.memoryPropertyFlags);
        ASSERT_TRUE(inputDesc.engineCreationFlags == currDesc.engineCreationFlags);
        ASSERT_TRUE(inputDesc.format == currDesc.format);

        auto properties = gpuResourceMgr.GetFormatProperties(handle);
        ASSERT_EQ(0, properties.bufferFeatures);

        auto gpuHandle = ((GpuResourceManager&)gpuResourceMgr).GetGpuHandle(handle.GetHandle());
        ASSERT_FALSE(RenderHandleUtil::IsValid(gpuHandle));

        auto data = ((GpuResourceManager&)gpuResourceMgr).MapBuffer(handle);
        ASSERT_EQ(nullptr, data);

        // change default flags
        constexpr BufferUsageFlags defaultFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_DST_BIT;
        gpuResourceMgr.SetDefaultGpuBufferCreationFlags(defaultFlags);
        gpuResourceMgr.Create(handle, inputDesc);
        currDesc = gpuResourceMgr.GetBufferDescriptor(handle);
        ASSERT_TRUE((inputDesc.usageFlags | defaultFlags) == currDesc.usageFlags);

        // reset default flags
        gpuResourceMgr.SetDefaultGpuBufferCreationFlags(0);
        gpuResourceMgr.Create(handle, inputDesc);
        currDesc = gpuResourceMgr.GetBufferDescriptor(handle);
        ASSERT_TRUE((inputDesc.usageFlags & defaultFlags) == 0);
    }

    // Add default flags for image.
    {
        GpuImageDesc inputDesc;
        inputDesc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
        inputDesc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
        inputDesc.width = 1;
        inputDesc.height = 1;
        inputDesc.depth = 1;
        inputDesc.format = Format::BASE_FORMAT_R8G8B8A8_UNORM;
        inputDesc.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_STORAGE_BIT;
        inputDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        inputDesc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        RenderHandleReference handle = gpuResourceMgr.Create(inputDesc);
        GpuImageDesc currDesc = gpuResourceMgr.GetImageDescriptor(handle);
        ASSERT_TRUE(inputDesc.width == currDesc.width);
        ASSERT_TRUE(inputDesc.height == currDesc.height);
        ASSERT_TRUE(inputDesc.depth == currDesc.depth);
        ASSERT_TRUE(inputDesc.usageFlags == currDesc.usageFlags);
        ASSERT_TRUE(inputDesc.memoryPropertyFlags == currDesc.memoryPropertyFlags);
        ASSERT_TRUE(inputDesc.engineCreationFlags == currDesc.engineCreationFlags);
        ASSERT_TRUE(inputDesc.format == currDesc.format);
        ASSERT_TRUE(inputDesc.imageType == currDesc.imageType);
        ASSERT_TRUE(inputDesc.imageTiling == currDesc.imageTiling);

        // change default flags
        constexpr ImageUsageFlags defaultFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        gpuResourceMgr.SetDefaultGpuImageCreationFlags(defaultFlags);
        gpuResourceMgr.Create(handle, inputDesc);
        currDesc = gpuResourceMgr.GetImageDescriptor(handle);
        ASSERT_TRUE((inputDesc.usageFlags | defaultFlags) == currDesc.usageFlags);

        // reset default flags
        gpuResourceMgr.SetDefaultGpuImageCreationFlags(0);
        gpuResourceMgr.Create(handle, inputDesc);
        currDesc = gpuResourceMgr.GetImageDescriptor(handle);
        ASSERT_TRUE((inputDesc.usageFlags & defaultFlags) == 0);
    }

    UTest::DestroyEngine(er);
}

/**
 * @tc.name: SamplerCreationTest
 * @tc.desc: Tests IGpuResourceManager for creating samplers and verifies that created description matches the input
 * description.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, SamplerCreationTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources er;
    // er.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(er);
    ASSERT_TRUE(er.device != nullptr);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();

    {
        GpuSamplerDesc inputDesc;
        inputDesc.magFilter = Filter::CORE_FILTER_LINEAR;
        inputDesc.minFilter = Filter::CORE_FILTER_LINEAR;
        inputDesc.mipMapMode = Filter::CORE_FILTER_LINEAR;
        inputDesc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        inputDesc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        inputDesc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        inputDesc.engineCreationFlags = 0;
        inputDesc.mipLodBias = 0.0f;
        inputDesc.enableAnisotropy = false;
        inputDesc.maxAnisotropy = 1.0f;
        inputDesc.enableCompareOp = false;
        inputDesc.compareOp = CompareOp::CORE_COMPARE_OP_NEVER;
        inputDesc.minLod = 0.0f;
        inputDesc.maxLod = 0.0f;
        inputDesc.borderColor = BorderColor::CORE_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        inputDesc.enableUnnormalizedCoordinates = false;
        RenderHandleReference handle = gpuResourceMgr.Create("Samp0", inputDesc);
        RenderHandleReference handle2 = gpuResourceMgr.Create(inputDesc);
        ASSERT_TRUE(gpuResourceMgr.IsGpuSampler(handle));
        ASSERT_TRUE(gpuResourceMgr.IsGpuSampler(handle2));
        GpuSamplerDesc currDesc = gpuResourceMgr.GetSamplerDescriptor(handle);
        ASSERT_EQ(inputDesc.magFilter, currDesc.magFilter);
        ASSERT_EQ(inputDesc.minFilter, currDesc.minFilter);
        ASSERT_EQ(inputDesc.mipMapMode, currDesc.mipMapMode);
        ASSERT_EQ(inputDesc.addressModeU, currDesc.addressModeU);
        ASSERT_EQ(inputDesc.addressModeV, currDesc.addressModeV);
        ASSERT_EQ(inputDesc.addressModeW, currDesc.addressModeW);
        ASSERT_EQ(inputDesc.engineCreationFlags, currDesc.engineCreationFlags);
        ASSERT_EQ(inputDesc.mipLodBias, currDesc.mipLodBias);
        ASSERT_EQ(inputDesc.enableAnisotropy, currDesc.enableAnisotropy);
        ASSERT_EQ(inputDesc.maxAnisotropy, currDesc.maxAnisotropy);
        ASSERT_EQ(inputDesc.enableCompareOp, currDesc.enableCompareOp);
        ASSERT_EQ(inputDesc.compareOp, currDesc.compareOp);
        ASSERT_EQ(inputDesc.minLod, currDesc.minLod);
        ASSERT_EQ(inputDesc.maxLod, currDesc.maxLod);
        ASSERT_EQ(inputDesc.borderColor, currDesc.borderColor);
        ASSERT_EQ(inputDesc.enableUnnormalizedCoordinates, currDesc.enableUnnormalizedCoordinates);

        vector<RenderHandleReference> handles = gpuResourceMgr.GetSamplerHandles();
        bool found = false;
        for (size_t idx = 0; idx < handles.size(); ++idx) {
            if (handles[idx].GetHandle() == handle.GetHandle()) {
                found = true;
            }
        }
        ASSERT_TRUE(found);

        // re-creating the handle
        inputDesc.magFilter = Filter::CORE_FILTER_NEAREST;
        gpuResourceMgr.Create(handle, inputDesc);
        currDesc = gpuResourceMgr.GetSamplerDescriptor(handle);
        ASSERT_EQ(inputDesc.magFilter, currDesc.magFilter);

        // generation counter changes
        RenderHandleReference result = gpuResourceMgr.Get(handle.GetHandle());
        ASSERT_TRUE(RenderHandleUtil::IsTheSameWithoutGeneration(handle.GetHandle(), result.GetHandle()));

        auto gpuHandle = ((GpuResourceManager&)gpuResourceMgr).GetGpuHandle(handle.GetHandle());
        ASSERT_EQ(INVALID_RESOURCE_HANDLE, gpuHandle.id);
        ASSERT_EQ(result.GetHandle(), ((GpuResourceManager&)gpuResourceMgr).GetSamplerHandle("Samp0").GetHandle());
    }

    UTest::DestroyEngine(er);
}

/**
 * @tc.name: AccelerationStructureCreationTest
 * @tc.desc: Tests IGpuResourceManager for creating an AccelerationStructure and verifying the created description
 * matches the input description.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, AccelerationStructureCreationTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources er;
    // er.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(er);
    ASSERT_TRUE(er.device != nullptr);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();

    {
        GpuAccelerationStructureDesc inputDesc;
        inputDesc.accelerationStructureType = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        // NOTE: the flag is currently required
        inputDesc.bufferDesc.usageFlags =
            CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT | CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        inputDesc.bufferDesc.memoryPropertyFlags = 0;
        inputDesc.bufferDesc.engineCreationFlags = 0;
        inputDesc.bufferDesc.byteSize = 4;
        inputDesc.bufferDesc.format = BASE_NS::Format::BASE_FORMAT_UNDEFINED;
        RenderHandleReference handle = gpuResourceMgr.Create("Acc0", inputDesc);
        RenderHandleReference handle2 = gpuResourceMgr.Create(inputDesc);
        ASSERT_TRUE(gpuResourceMgr.IsGpuAccelerationStructure(handle));
        ASSERT_TRUE(gpuResourceMgr.IsGpuAccelerationStructure(handle2));
        GpuAccelerationStructureDesc currDesc = gpuResourceMgr.GetAccelerationStructureDescriptor(handle);
        ASSERT_EQ(inputDesc.accelerationStructureType, currDesc.accelerationStructureType);
        ASSERT_EQ(inputDesc.bufferDesc.usageFlags, currDesc.bufferDesc.usageFlags);
        ASSERT_EQ(inputDesc.bufferDesc.memoryPropertyFlags, currDesc.bufferDesc.memoryPropertyFlags);
        ASSERT_EQ(inputDesc.bufferDesc.engineCreationFlags, currDesc.bufferDesc.engineCreationFlags);
        ASSERT_EQ(inputDesc.bufferDesc.byteSize, currDesc.bufferDesc.byteSize);
        ASSERT_EQ(inputDesc.bufferDesc.format, currDesc.bufferDesc.format);
        {
            // needs to match
            GpuBufferDesc bufferDesc = gpuResourceMgr.GetBufferDescriptor(handle);
            ASSERT_EQ(bufferDesc.usageFlags, currDesc.bufferDesc.usageFlags);
            ASSERT_EQ(bufferDesc.memoryPropertyFlags, currDesc.bufferDesc.memoryPropertyFlags);
            ASSERT_EQ(bufferDesc.engineCreationFlags, currDesc.bufferDesc.engineCreationFlags);
            ASSERT_EQ(bufferDesc.byteSize, currDesc.bufferDesc.byteSize);
            ASSERT_EQ(bufferDesc.format, currDesc.bufferDesc.format);
        }

        // re-creating the handle
        inputDesc.bufferDesc.byteSize = 16;
        gpuResourceMgr.Create(handle, inputDesc);
        currDesc = gpuResourceMgr.GetAccelerationStructureDescriptor(handle);
        ASSERT_EQ(inputDesc.bufferDesc.byteSize, currDesc.bufferDesc.byteSize);

        // generation counter changes
        RenderHandleReference result = gpuResourceMgr.Get(handle.GetHandle());
        ASSERT_TRUE(RenderHandleUtil::IsTheSameWithoutGeneration(handle.GetHandle(), result.GetHandle()));

        ASSERT_EQ(nullptr, ((GpuResourceManager&)gpuResourceMgr).GetBuffer(handle.GetHandle()));

        auto gpuHandle = ((GpuResourceManager&)gpuResourceMgr).GetGpuHandle(handle.GetHandle());
        ASSERT_FALSE(RenderHandleUtil::IsValid(gpuHandle));

        ASSERT_EQ("Acc0", gpuResourceMgr.GetName(handle));
        ASSERT_EQ(result.GetHandle(), ((GpuResourceManager&)gpuResourceMgr).GetBufferRawHandle("Acc0"));
        ASSERT_EQ(RenderHandle {}, ((GpuResourceManager&)gpuResourceMgr).GetBufferRawHandle(""));
        ASSERT_EQ(RenderHandle {}, ((GpuResourceManager&)gpuResourceMgr).GetBufferHandle("").GetHandle());
        ASSERT_EQ(result.GetHandle(), ((GpuResourceManager&)gpuResourceMgr).GetBufferHandle("Acc0").GetHandle());
    }

    UTest::DestroyEngine(er);
}

/**
 * @tc.name: ImageCreationTest
 * @tc.desc: Tests IGpuResourceManager for creating an Image and verifying the created description
 * matches the input description. Also tests IImageLoaderManager for loading images from a file.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, ImageCreationTest, testing::ext::TestSize.Level1)
{
    UTest::EngineResources er;
    UTest::CreateEngineSetup(er);
    ASSERT_TRUE(er.device != nullptr);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();
    IImageLoaderManager& imgLoaderMgr = er.engine->GetImageLoaderManager();

    {
        auto loadResult = imgLoaderMgr.LoadImage(
            "test://images/ImageCreationTest_16x16.jpg", IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
        ASSERT_TRUE(loadResult.success);
        IImageContainer::Ptr imageContainer { move(loadResult.image) };

        GpuImageDesc inputDesc;
        inputDesc = gpuResourceMgr.CreateGpuImageDesc(imageContainer->GetImageDesc());

        RenderHandleReference handle = gpuResourceMgr.Create(inputDesc, move(imageContainer));
        ASSERT_TRUE(((GpuResourceManager&)gpuResourceMgr).IsValid(handle.GetHandle()));
        ASSERT_FALSE(gpuResourceMgr.IsMappableOutsideRender(handle));

        GpuImageDesc currDesc = gpuResourceMgr.GetImageDescriptor(handle);
        ASSERT_TRUE(inputDesc.width == currDesc.width);
        ASSERT_TRUE(inputDesc.height == currDesc.height);
        ASSERT_TRUE(inputDesc.depth == currDesc.depth);
        ASSERT_TRUE(inputDesc.usageFlags == currDesc.usageFlags);
        ASSERT_TRUE(inputDesc.memoryPropertyFlags == currDesc.memoryPropertyFlags);
        ASSERT_TRUE(inputDesc.engineCreationFlags == currDesc.engineCreationFlags);
        ASSERT_TRUE(inputDesc.format == currDesc.format);
        ASSERT_TRUE(inputDesc.imageType == currDesc.imageType);
        ASSERT_TRUE(inputDesc.imageTiling == currDesc.imageTiling);

        auto properties = gpuResourceMgr.GetFormatProperties(handle);
        ASSERT_EQ(4, properties.bytesPerPixel);

        auto gpuHandle = ((GpuResourceManager&)gpuResourceMgr).GetGpuHandle(handle.GetHandle());
        ASSERT_EQ(INVALID_RESOURCE_HANDLE, gpuHandle.id);
        gpuHandle = ((GpuResourceManager&)gpuResourceMgr).GetGpuHandle({});
        ASSERT_EQ(INVALID_RESOURCE_HANDLE, gpuHandle.id);
    }
    {
        auto loadResult = imgLoaderMgr.LoadImage("test://images/kloppenheim_evening_skybox_8x8.ktx", 0u);
        ASSERT_TRUE(loadResult.success);
        IImageContainer::Ptr imageContainer { move(loadResult.image) };
        GpuImageDesc inputDesc;
        inputDesc = gpuResourceMgr.CreateGpuImageDesc(imageContainer->GetImageDesc());
        ASSERT_TRUE(inputDesc.createFlags & ImageCreateFlagBits::CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
        RenderHandleReference handle = gpuResourceMgr.Create(inputDesc, move(imageContainer));
        ASSERT_TRUE(handle);
    }
    {
        UTest::DestroyEngine(er);
    }
}

/**
 * @tc.name: LargeStagingSize
 * @tc.desc: Tests IGpuResourceManager for staging gigabytes of resources.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, LargeStagingSize, testing::ext::TestSize.Level1)
{
    UTest::EngineResources er;
    er.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(er);
    ASSERT_TRUE(er.device != nullptr);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();

    constexpr uint32_t targetStagingSize = 1024 * 1024 * 512; // 512MB for images and buffer each, so 1GB in total

    // 2048x2048x4 floats is ~67MB
    constexpr uint32_t arraySize = 2048 * 2048;

    constexpr uint32_t arrayByteSize = arraySize * sizeof(Math::Vec4);
    constexpr uint32_t numLoops = targetStagingSize / arrayByteSize;

    const unique_ptr<uint8_t[]> rawMem = make_unique<uint8_t[]>(arrayByteSize);
    memset((void*)rawMem.get(), 0, arrayByteSize);

    array_view<const uint8_t> memView = { reinterpret_cast<const uint8_t*>(rawMem.get()), arrayByteSize };
    vector<RenderHandleReference> resourceRefs = {};

    GpuImageDesc imgDesc = {};
    imgDesc.width = 2048;
    imgDesc.height = 2048;
    imgDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    imgDesc.format = Format::BASE_FORMAT_R32G32B32A32_SFLOAT;
    imgDesc.usageFlags =
        CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;

    GpuBufferDesc bufDesc = {};
    bufDesc.byteSize = arrayByteSize;
    bufDesc.format = Format::BASE_FORMAT_R32_SFLOAT;
    bufDesc.usageFlags = CORE_BUFFER_USAGE_TRANSFER_DST_BIT | CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT;
    bufDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    for (uint32_t i = 0; i < numLoops; i++) {
        string name = string("Image") + to_string(i);

        RenderHandleReference handle = resourceRefs.emplace_back(gpuResourceMgr.Create(name, imgDesc, memView));
        ASSERT_TRUE(((GpuResourceManager&)gpuResourceMgr).IsValid(handle.GetHandle()));
        ASSERT_FALSE(gpuResourceMgr.IsMappableOutsideRender(handle));
    }
    for (uint32_t i = 0; i < numLoops; i++) {
        string name = string("Buffer") + to_string(i);

        RenderHandleReference handle = resourceRefs.emplace_back(gpuResourceMgr.Create(name, bufDesc, memView));
        ASSERT_TRUE(((GpuResourceManager&)gpuResourceMgr).IsValid(handle.GetHandle()));
        ASSERT_FALSE(gpuResourceMgr.IsMappableOutsideRender(handle));
    }

    {
        er.context->GetRenderer().RenderFrame({});
        gpuResourceMgr.WaitForIdleAndDestroyGpuResources();

        UTest::DestroyEngine(er);
    }
}

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: ImageViewCreationTestVulkan
 * @tc.desc: Tests IGpuResourceManager for creating an image view from an image in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, ImageViewCreationTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources er;
    er.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(er);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();
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
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.componentMapping.r = CORE_COMPONENT_SWIZZLE_B;
        desc.componentMapping.g = CORE_COMPONENT_SWIZZLE_G;
        desc.componentMapping.b = CORE_COMPONENT_SWIZZLE_R;
        desc.componentMapping.a = CORE_COMPONENT_SWIZZLE_ONE;

        ImageDescVk backendDesc;
        auto image = ((DeviceVk*)er.device)->CreateGpuImage(desc);
        backendDesc.image = ((GpuImageVk&)*image).GetPlatformData().image;

        auto viewHandle = gpuResourceMgr.CreateView("view0", desc, backendDesc);
        ASSERT_NE(RenderHandle {}, viewHandle.GetHandle());
        ASSERT_EQ(RenderHandleType::GPU_IMAGE, viewHandle.GetHandleType());
    }
    UTest::DestroyEngine(er);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
#ifdef DISABLED_TESTS_ON
#if NDEBUG
/**
 * @tc.name: ImageViewCreationTestOpenGL
 * @tc.desc: Tests for Image View Creation Test Open Gl. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, ImageViewCreationTestOpenGL, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: ImageViewCreationTestOpenGL
 * @tc.desc: Tests IGpuResourceManager for creating an image view from an image in OpenGL.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, DISABLED_ImageViewCreationTestOpenGL, testing::ext::TestSize.Level1)
#endif // NDEBUG
{
    UTest::EngineResources er;
    er.createWindow = true;
    er.backend = UTest::GetOpenGLBackend();
    UTest::CreateEngineSetup(er);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();

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
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.componentMapping.r = CORE_COMPONENT_SWIZZLE_B;
        desc.componentMapping.g = CORE_COMPONENT_SWIZZLE_G;
        desc.componentMapping.b = CORE_COMPONENT_SWIZZLE_R;
        desc.componentMapping.a = CORE_COMPONENT_SWIZZLE_ONE;
#if RENDER_HAS_GL_BACKEND
        ImageDescGL backendDesc;
#elif RENDER_HAS_GLES_BACKEND
        ImageDescGLES backendDesc;
#endif

        auto image = ((DeviceGLES*)er.device)->CreateGpuImage(desc);
        backendDesc.image = ((GpuImageGLES&)*image).GetPlatformData().image;

        auto viewHandle = gpuResourceMgr.CreateView("view0", desc, backendDesc);
        ASSERT_NE(RenderHandle {}, viewHandle.GetHandle());
        ASSERT_EQ(RenderHandleType::GPU_IMAGE, viewHandle.GetHandleType());
    }
    {
        UTest::DestroyEngine(er);
    }
}
#endif // DISABLED_TESTS_ON
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND

void TestGpuResourceManager(DeviceBackendType backend)
{
    UTest::EngineResources er;
    if (backend == DeviceBackendType::OPENGL) {
        er.createWindow = true;
    }
    er.backend = backend;
    UTest::CreateEngineSetup(er);
    ASSERT_TRUE(er.device != nullptr);

    IGpuResourceManager& gpuResourceMgr = er.device->GetGpuResourceManager();
    IImageLoaderManager& imgLoaderMgr = er.engine->GetImageLoaderManager();

    {
        auto loadResult = imgLoaderMgr.LoadImage(
            "test://images/ImageCreationTest_16x16.jpg", IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
        ASSERT_TRUE(loadResult.success);
        IImageContainer::Ptr imageContainer1 { move(loadResult.image) };
        GpuImageDesc inputDesc;
        inputDesc = gpuResourceMgr.CreateGpuImageDesc(imageContainer1->GetImageDesc());
        RenderHandleReference handle1 = gpuResourceMgr.Create(inputDesc, move(imageContainer1));

        loadResult = imgLoaderMgr.LoadImage(
            "test://images/ImageCreationTest_16x16.jpg", IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
        ASSERT_TRUE(loadResult.success);
        IImageContainer::Ptr imageContainer2 { move(loadResult.image) };
        inputDesc = gpuResourceMgr.CreateGpuImageDesc(imageContainer2->GetImageDesc());
        RenderHandleReference handle2 = gpuResourceMgr.Create(inputDesc, move(imageContainer2));

        ((GpuResourceManager&)gpuResourceMgr)
            .RenderBackendImmediateRemapGpuImageHandle(handle1.GetHandle(), handle2.GetHandle());

        auto imageHandles = gpuResourceMgr.GetImageHandles();
        ASSERT_EQ(6, imageHandles.size());
        auto bufferHandles = gpuResourceMgr.GetBufferHandles();
        ASSERT_EQ(1, bufferHandles.size());

        ASSERT_EQ("", gpuResourceMgr.GetName(handle1));
        ASSERT_EQ(GpuAccelerationStructureDesc {}.accelerationStructureType,
            gpuResourceMgr.GetAccelerationStructureDescriptor(handle1).accelerationStructureType);
        ASSERT_EQ(
            GpuSamplerDesc {}.engineCreationFlags, gpuResourceMgr.GetSamplerDescriptor(handle1).engineCreationFlags);
        ASSERT_EQ(
            GpuBufferDesc {}.engineCreationFlags, gpuResourceMgr.GetBufferDescriptor(handle1).engineCreationFlags);

        GpuBufferDesc bufferDesc;
        bufferDesc.byteSize = 4;
        bufferDesc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        bufferDesc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        bufferDesc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS;
        RenderHandleReference handle3 = gpuResourceMgr.Create(bufferDesc);

        ASSERT_EQ(GpuImageDesc {}.engineCreationFlags, gpuResourceMgr.GetImageDescriptor(handle3).engineCreationFlags);
    }
    {
        GpuBufferDesc desc;
        desc.usageFlags = 0;
        desc.memoryPropertyFlags = 0;
        desc.byteSize = 0;
        auto handle0 = gpuResourceMgr.Create("buffer0", desc, array_view<const uint8_t> {});
        desc.usageFlags = ~0;
        desc.memoryPropertyFlags = ~0;
        auto handle1 = gpuResourceMgr.Create("buffer1", desc, array_view<const uint8_t> {});

        auto rechandle0 = gpuResourceMgr.GetRawHandle(handle0.GetHandle());
        ASSERT_EQ(handle0.GetHandle().id, rechandle0.id);
        auto rechandle1 = gpuResourceMgr.GetRawHandle(handle1.GetHandle());
        ASSERT_EQ(handle1.GetHandle().id, rechandle1.id);
    }
    {
        GpuImageDesc desc;
        desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
        desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
        desc.layerCount = 5u;
        desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
        desc.usageFlags = 0;
        desc.memoryPropertyFlags = 0;
        desc.depth = 0;
        desc.height = ~0u;
        desc.width = 0;
        desc.mipCount = 0;
        desc.createFlags = ~0;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_GENERATE_MIPS;
        desc.format = BASE_FORMAT_B8G8R8_UINT;
        auto handle = gpuResourceMgr.Create("image0", desc);
        auto rechandle = gpuResourceMgr.GetRawHandle(handle.GetHandle());
        ASSERT_EQ(handle.GetHandle().id, rechandle.id);
    }
    {
        GpuImageDesc desc;
        desc.memoryPropertyFlags = ~0;
        desc.width = 1u;
        desc.height = 1u;
        desc.depth = 1u;
        desc.usageFlags = CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        desc.format = BASE_FORMAT_B8G8R8_UINT;
        auto handle0 = gpuResourceMgr.Create("", desc);
        desc.usageFlags = ~0;
        auto handle1 = gpuResourceMgr.Create("", desc);

        auto rechandle0 = gpuResourceMgr.GetRawHandle(handle0.GetHandle());
        ASSERT_EQ(handle0.GetHandle().id, rechandle0.id);
        auto rechandle1 = gpuResourceMgr.GetRawHandle(handle1.GetHandle());
        ASSERT_EQ(handle1.GetHandle().id, rechandle1.id);
    }
    {
        GpuImageDesc desc;
        desc.memoryPropertyFlags = ~0;
        desc.width = 1u;
        desc.height = 1u;
        desc.depth = 1u;
        desc.format = BASE_FORMAT_B8G8R8_UINT;
        desc.usageFlags = ~0;
        BufferImageCopy copy;
        copy.imageOffset.depth = 5u;
        auto handle = gpuResourceMgr.Create("", desc, {}, { &copy, 1 });
        auto rechandle = gpuResourceMgr.GetRawHandle(handle.GetHandle());
        ASSERT_EQ(handle.GetHandle().id, rechandle.id);
    }
    {
        for (uint32_t i = 0; i < 2u; ++i) {
            GpuImageDesc desc;
            desc.memoryPropertyFlags = ~0;
            desc.width = 32u;
            desc.height = 32u;
            desc.depth = 1u;
            desc.format = BASE_FORMAT_B8G8R8_UINT;
            desc.usageFlags = ~0;
            desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_SCALE;
            BufferImageCopy copy;
            copy.imageExtent.width = 16u;
            copy.imageExtent.height = 16u;
            copy.imageOffset.depth = 1u;
            auto handle = gpuResourceMgr.Create("", desc, {}, { &copy, 1 });
            auto rechandle = gpuResourceMgr.GetRawHandle(handle.GetHandle());
            ASSERT_EQ(handle.GetHandle().id, rechandle.id);
        }
    }
    {
        for (uint32_t i = 0; i < 2u; ++i) {
            auto loadResult = imgLoaderMgr.LoadImage(
                "test://images/ImageCreationTest_16x16.jpg", IImageLoaderManager::IMAGE_LOADER_GENERATE_MIPS);
            ASSERT_TRUE(loadResult.success);
            IImageContainer::Ptr imageContainer1 { move(loadResult.image) };
            GpuImageDesc inputDesc;
            inputDesc = gpuResourceMgr.CreateGpuImageDesc(imageContainer1->GetImageDesc());
            inputDesc.width = 32u;
            inputDesc.height = 32u;
            inputDesc.engineCreationFlags |= CORE_ENGINE_IMAGE_CREATION_SCALE;
            auto handle = gpuResourceMgr.Create(inputDesc, move(imageContainer1));
        }
    }
    {
        auto handle = gpuResourceMgr.Get(RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 1000u));
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle = gpuResourceMgr.Get(RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 1000u, 0u,
            RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_ACCELERATION_STRUCTURE));
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle = gpuResourceMgr.Get(RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 1000u));
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle = gpuResourceMgr.Get(RenderHandleUtil::CreateHandle(RenderHandleType::GPU_SAMPLER, 1000u));
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::COMPUTE_PSO, 15u, 0u,
            RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_ACCELERATION_STRUCTURE);
        RenderHandleReference handleRef { handle, {} };
        {
            GpuBufferDesc desc;
            desc.byteSize = 16u;
            desc.usageFlags = CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            gpuResourceMgr.Create(handleRef, desc);
        }
        {
            GpuImageDesc desc;
            desc.width = 4u;
            desc.height = 4u;
            desc.depth = 1u;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            desc.format = BASE_FORMAT_R8G8B8A8_UNORM;
            gpuResourceMgr.Create(handleRef, desc);
        }
        {
            GpuSamplerDesc desc;
            gpuResourceMgr.Create(handleRef, desc);
        }
        {
            GpuAccelerationStructureDesc desc;
            gpuResourceMgr.Create(handleRef, desc);
        }
        ((GpuResourceManager&)gpuResourceMgr).RemapGpuImageHandle(handle, {});
    }
    {
        GpuBufferDesc desc;
        desc.byteSize = 16u;
        desc.usageFlags = CORE_BUFFER_USAGE_INDEX_BUFFER_BIT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        auto handle = gpuResourceMgr.Create(desc, {});
        ASSERT_NE(RenderHandle {}, handle.GetHandle());
    }
    {
        auto handle = gpuResourceMgr.Create("", GpuImageDesc {}, IImageContainer::Ptr {});
        ASSERT_EQ(RenderHandle {}, handle.GetHandle());
    }
    {
        GpuImageDesc desc;
        desc.width = 4u;
        desc.height = 4u;
        desc.depth = 1u;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        desc.format = BASE_FORMAT_R8G8B8A8_UNORM;
        {
            auto handle =
                gpuResourceMgr.Create(desc, array_view<const uint8_t> {}, array_view<const BufferImageCopy> {});
            ASSERT_NE(RenderHandle {}, handle.GetHandle());
        }
        {
            auto handle = gpuResourceMgr.Create(desc, array_view<const uint8_t> {});
            ASSERT_NE(RenderHandle {}, handle.GetHandle());
        }
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 0u, 15u);
        RenderHandleReference handleRef { handle, {} };
        auto bufferDesc = gpuResourceMgr.GetBufferDescriptor(handleRef);
        ASSERT_NE(0, bufferDesc.byteSize);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 100u);
        RenderHandleReference handleRef { handle, {} };
        auto bufferDesc = gpuResourceMgr.GetBufferDescriptor(handleRef);
        ASSERT_EQ(0, bufferDesc.byteSize);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 0u, 15u);
        RenderHandleReference handleRef { handle, {} };
        auto imageDesc = gpuResourceMgr.GetImageDescriptor(handleRef);
        ASSERT_NE(1, imageDesc.width);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_IMAGE, 100u);
        RenderHandleReference handleRef { handle, {} };
        auto imageDesc = gpuResourceMgr.GetImageDescriptor(handleRef);
        ASSERT_EQ(1, imageDesc.width);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_SAMPLER, 0u, 15u);
        RenderHandleReference handleRef { handle, {} };
        auto samplerDesc = gpuResourceMgr.GetSamplerDescriptor(handleRef);
        ASSERT_EQ(0, samplerDesc.engineCreationFlags);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_SAMPLER, 100u);
        RenderHandleReference handleRef { handle, {} };
        auto samplerDesc = gpuResourceMgr.GetSamplerDescriptor(handleRef);
        ASSERT_EQ(0, samplerDesc.engineCreationFlags);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 0u, 15u,
            RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_ACCELERATION_STRUCTURE);
        RenderHandleReference handleRef { handle, {} };
        auto accDesc = gpuResourceMgr.GetAccelerationStructureDescriptor(handleRef);
        ASSERT_EQ(CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL, accDesc.accelerationStructureType);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 100u, 0u,
            RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_ACCELERATION_STRUCTURE);
        RenderHandleReference handleRef { handle, {} };
        auto accDesc = gpuResourceMgr.GetAccelerationStructureDescriptor(handleRef);
        ASSERT_EQ(CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL, accDesc.accelerationStructureType);
    }
    {
        auto handle = RenderHandleUtil::CreateHandle(RenderHandleType::GPU_BUFFER, 0u, 15u,
            RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_MAP_OUTSIDE_RENDERER |
                RenderHandleInfoFlagBits::CORE_RESOURCE_HANDLE_IMMEDIATELY_CREATED);
        RenderHandleReference handleRef { handle, {} };
        ASSERT_EQ(nullptr, gpuResourceMgr.MapBufferMemory(handleRef));
        gpuResourceMgr.UnmapBuffer(handleRef);
    }
    {
        GpuBufferDesc desc;
        desc.byteSize = 16u;
        desc.engineCreationFlags = CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT;
        auto handle0 = gpuResourceMgr.Create("buf", desc);
        auto handle1 = gpuResourceMgr.Create(handle0, desc);
        ASSERT_TRUE(RenderHandleUtil::IsTheSameWithoutGeneration(handle0.GetHandle(), handle1.GetHandle()));
    }
#if RENDER_HAS_VULKAN_BACKEND
    if (er.backend == DeviceBackendType::VULKAN) {
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
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.componentMapping.r = CORE_COMPONENT_SWIZZLE_B;
        desc.componentMapping.g = CORE_COMPONENT_SWIZZLE_G;
        desc.componentMapping.b = CORE_COMPONENT_SWIZZLE_R;
        desc.componentMapping.a = CORE_COMPONENT_SWIZZLE_ONE;

        ImageDescVk backendDesc;
        auto image = ((DeviceVk*)er.device)->CreateGpuImage(desc);
        backendDesc.image = ((GpuImageVk&)*image).GetPlatformData().image;

        string viewName = "imgView";
        {
            auto viewHandle = gpuResourceMgr.CreateView(viewName, desc, backendDesc);
            ASSERT_NE(RenderHandle {}, viewHandle.GetHandle());
            ASSERT_EQ(RenderHandleType::GPU_IMAGE, viewHandle.GetHandleType());
        }
        {
            auto viewHandle = gpuResourceMgr.CreateView(viewName, desc, backendDesc);
            ASSERT_NE(RenderHandle {}, viewHandle.GetHandle());
            ASSERT_EQ(RenderHandleType::GPU_IMAGE, viewHandle.GetHandleType());
        }
    }
#endif // RENDER_HAS_VULKAN_BACKEND
    {
        ((GpuResourceManager&)gpuResourceMgr).RenderBackendImmediateRemapGpuImageHandle({}, {});
        gpuResourceMgr.WaitForIdleAndDestroyGpuResources();
    }
    {
        UTest::DestroyEngine(er);
    }
}

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: GpuResourceManagerTestVulkan
 * @tc.desc: Tests IGpuResourceManager and IImageLoaderManager for creating gpu resources and verifies their properties
 * are as expected after creation in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, GpuResourceManagerTestVulkan, testing::ext::TestSize.Level1)
{
    TestGpuResourceManager(DeviceBackendType::VULKAN);
}
#endif // RENDER_HAS_VULKAN_BACKEND

#if RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
/**
 * @tc.name: GpuResourceManagerTestOpenGL
 * @tc.desc: Tests IGpuResourceManager and IImageLoaderManager for creating gpu resources and verifies their properties
 * are as expected after creation in Vulkan.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_GpuResourceManager, GpuResourceManagerTestOpenGL, testing::ext::TestSize.Level1)
{
    TestGpuResourceManager(UTest::GetOpenGLBackend());
}
#endif // RENDER_HAS_GL_BACKEND || RENDER_HAS_GLES_BACKEND
