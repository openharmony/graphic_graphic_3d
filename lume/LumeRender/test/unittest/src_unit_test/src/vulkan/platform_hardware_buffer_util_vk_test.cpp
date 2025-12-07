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

#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/device_vk.h>
#include <vulkan/gpu_image_vk.h>
#include <vulkan/platform_hardware_buffer_util_vk.h>
#endif // RENDER_HAS_VULKAN_BACKEND

#include <device/device.h>

#include <render/device/gpu_resource_desc.h>

#include "test_framework.h"
#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

#if RENDER_HAS_VULKAN_BACKEND
/**
 * @tc.name: PlatformHardwareBufferUtilTestVulkan
 * @tc.desc: Tests Vulkan PlatformHardwareBufferUtil by converting engine resource descriptions to Vulkan descriptions.
 * Also tests that correct memory type indces are returned for different memory property flags.
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_PlatformHardwareBufferUtil, PlatformHardwareBufferUtilTestVulkan, testing::ext::TestSize.Level1)
{
    UTest::EngineResources engine;
    engine.backend = DeviceBackendType::VULKAN;
    UTest::CreateEngineSetup(engine);
    Device& device = *static_cast<Device*>(engine.device);
    DeviceVk& deviceVk = static_cast<DeviceVk&>(device);
    {
        GpuImageDesc desc;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.format = BASE_FORMAT_R8G8B8A8_SRGB;
        desc.width = 16u;
        desc.height = 10u;
        desc.depth = 1u;
        desc.mipCount = 1u;
        desc.layerCount = 1u;
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_16_BIT;
        desc.imageTiling = CORE_IMAGE_TILING_LINEAR;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        auto createInfo = PlatformHardwareBufferUtil::GetHwBufferImageCreateInfo(desc);
        ASSERT_EQ(VK_IMAGE_TYPE_2D, createInfo.imageType);
        ASSERT_EQ(VK_FORMAT_R8G8B8A8_SRGB, createInfo.format);
        ASSERT_EQ(16u, createInfo.extent.width);
        ASSERT_EQ(10u, createInfo.extent.height);
        ASSERT_EQ(1u, createInfo.extent.depth);
        ASSERT_EQ(1u, createInfo.mipLevels);
        ASSERT_EQ(1u, createInfo.arrayLayers);
        ASSERT_EQ(VK_SAMPLE_COUNT_16_BIT, createInfo.samples);
        ASSERT_EQ(VK_IMAGE_TILING_LINEAR, createInfo.tiling);
        ASSERT_EQ(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, createInfo.usage);
    }
    {
        PlatformHardwareBufferUtil::HardwareBufferProperties properties;
        properties.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        VkSamplerYcbcrConversionCreateInfo ycbcrCreateInfo;
        PlatformHardwareBufferUtil::FillYcbcrConversionInfo(properties, ycbcrCreateInfo);
        ASSERT_EQ(properties.format, ycbcrCreateInfo.format);
        ASSERT_EQ(VK_COMPONENT_SWIZZLE_IDENTITY, ycbcrCreateInfo.components.r);
        ASSERT_EQ(VK_COMPONENT_SWIZZLE_IDENTITY, ycbcrCreateInfo.components.g);
        ASSERT_EQ(VK_COMPONENT_SWIZZLE_IDENTITY, ycbcrCreateInfo.components.b);
        ASSERT_EQ(VK_COMPONENT_SWIZZLE_IDENTITY, ycbcrCreateInfo.components.a);
    }
    {
        VkPhysicalDeviceMemoryProperties properties;
        properties.memoryTypeCount = 2u;
        properties.memoryTypes[0].propertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        properties.memoryTypes[1].propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                                  VK_MEMORY_PROPERTY_HOST_CACHED_BIT;
        {
            uint32_t index = PlatformHardwareBufferUtil::GetMemoryTypeIndex(
                properties, ~0u, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
            ASSERT_EQ(1u, index);
        }
        {
            uint32_t index =
                PlatformHardwareBufferUtil::GetMemoryTypeIndex(properties, ~0u, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
            ASSERT_EQ(0u, index);
        }
    }
    {
        GpuImageDesc desc;
        desc.imageType = CORE_IMAGE_TYPE_2D;
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        desc.format = BASE_FORMAT_R8G8B8A8_UINT;
        desc.width = 16u;
        desc.height = 10u;
        desc.depth = 1u;
        desc.mipCount = 1u;
        desc.layerCount = 1u;
        desc.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        GpuImageVk image { device, desc };
        auto memoryRequirements = PlatformHardwareBufferUtil::GetImageMemoryRequirements(
            deviceVk, image.GetPlatformData().image, VK_IMAGE_ASPECT_COLOR_BIT, false);
        ASSERT_LE(desc.width * desc.height * 4u, memoryRequirements.size);
    }
    UTest::DestroyEngine(engine);
}
#endif // RENDER_HAS_VULKAN_BACKEND
