/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "platform_hardware_buffer_util_vk.h"

#include <algorithm>
#include <vulkan/vulkan_core.h>

#include <base/math/mathf.h>
#include <render/namespace.h>

#include "device/device.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
namespace PlatformHardwareBufferUtil {
uint32_t GetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    const uint32_t memoryTypeBits, const VkMemoryPropertyFlags memoryPropertyFlags)
{
    uint32_t memTypeIndex = ~0u;
    // first explicit check
    for (uint32_t idx = 0; idx < physicalDeviceMemoryProperties.memoryTypeCount; ++idx) {
        if ((memoryTypeBits & (1 << idx)) &&
            (physicalDeviceMemoryProperties.memoryTypes[idx].propertyFlags == memoryPropertyFlags)) {
            memTypeIndex = idx;
        }
    }
    // then non explicit check
    if (memTypeIndex == ~0u) {
        for (uint32_t idx = 0; idx < physicalDeviceMemoryProperties.memoryTypeCount; ++idx) {
            if ((memoryTypeBits & (1 << idx)) && ((physicalDeviceMemoryProperties.memoryTypes[idx].propertyFlags &
                                                      memoryPropertyFlags) == memoryPropertyFlags)) {
                memTypeIndex = idx;
            }
        }
    }
    PLUGIN_ASSERT_MSG(memTypeIndex != ~0u, "requested memory not found for hwbuffer");
    return memTypeIndex;
}

VkImageCreateInfo GetHwBufferImageCreateInfo(const GpuImageDesc& desc)
{
    // NOTE: undefined layout
    return VkImageCreateInfo {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,            // sType
        nullptr,                                        // pNext
        0,                                              // flags
        (VkImageType)desc.imageType,                    // imageType
        (VkFormat)desc.format,                          // format
        { desc.width, desc.height, desc.depth },        // extent
        desc.mipCount,                                  // mipLevels
        desc.layerCount,                                // arrayLayers
        (VkSampleCountFlagBits)(desc.sampleCountFlags), // samples
        (VkImageTiling)desc.imageTiling,                // tiling
        (VkImageUsageFlags)desc.usageFlags,             // usage
        VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,       // sharingMode
        0,                                              // queueFamilyIndexCount
        nullptr,                                        // pQueueFamilyIndices
        VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,       // initialLayout
    };
}

VkMemoryRequirements GetImageMemoryRequirements(const DeviceVk& deviceVk, const VkImage image,
    const VkImageAspectFlags imageAspectFlags, const bool useMemoryRequirements2)
{
    VkMemoryRequirements memoryRequirements {
        0, // size
        0, // alignment
        0, // memoryTypeBits
    };
    const DevicePlatformDataVk& devicePlat = ((const DevicePlatformDataVk&)deviceVk.GetPlatformData());
    VkDevice device = devicePlat.device;

    const DeviceVk::CommonDeviceExtensions& deviceExtensions = deviceVk.GetCommonDeviceExtensions();
    const DeviceVk::ExtFunctions& extFunctions = deviceVk.GetExtFunctions();
    if (deviceExtensions.getMemoryRequirements2 && useMemoryRequirements2) {
        if (extFunctions.vkGetImageMemoryRequirements2) {
            VkImagePlaneMemoryRequirementsInfo planeMemoryRequirementsInfo {
                VK_STRUCTURE_TYPE_IMAGE_PLANE_MEMORY_REQUIREMENTS_INFO, // sType
                nullptr,                                                // pNext
                (VkImageAspectFlagBits)(imageAspectFlags),              // planeAspect
            };
            VkImageMemoryRequirementsInfo2 imageMemoryRequirementsInfo {
                VK_STRUCTURE_TYPE_IMAGE_MEMORY_REQUIREMENTS_INFO_2, // sType
                &planeMemoryRequirementsInfo,                       // pNext
                image,                                              // image
            };
            VkMemoryRequirements2 memoryRequirements2 {
                VK_STRUCTURE_TYPE_MEMORY_REQUIREMENTS_2, // sType
                nullptr,                                 // pNext
                {},                                      // memoryRequirements
            };

            extFunctions.vkGetImageMemoryRequirements2(device, // device
                &imageMemoryRequirementsInfo,                  // pInfo
                &memoryRequirements2);                         // pMemoryRequirements
            memoryRequirements = memoryRequirements2.memoryRequirements;
        }
    } else {
        vkGetImageMemoryRequirements(device, // device
            image,                           // image
            &memoryRequirements);            // pMemoryRequirements
    }

    return memoryRequirements;
}

void DestroyHwPlatformImage(const DeviceVk& deviceVk, VkImage image, VkDeviceMemory deviceMemory)
{
    VkDevice device = ((const DevicePlatformDataVk&)deviceVk.GetPlatformData()).device;
    vkDestroyImage(device, // device
        image,             // image
        nullptr);          // pAllocator
    vkFreeMemory(device,   // device
        deviceMemory,      // memory
        nullptr);          // pAllocator
}

void FillYcbcrConversionInfo(const DeviceVk& deviceVk, const HardwareBufferProperties& hwBufferProperties,
    VkSamplerYcbcrConversionCreateInfo& ycbcrConversionCreateInfo)
{
    constexpr VkComponentMapping componentMapping {
        VK_COMPONENT_SWIZZLE_IDENTITY, // r
        VK_COMPONENT_SWIZZLE_IDENTITY, // g
        VK_COMPONENT_SWIZZLE_IDENTITY, // b
        VK_COMPONENT_SWIZZLE_IDENTITY, // a
    };
    // NOTE: might not support linear (needs to be checked)
    constexpr VkFilter hardcodedFilter = VK_FILTER_NEAREST;
    ycbcrConversionCreateInfo = {
        VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_CREATE_INFO, // sType
        nullptr,                                                // pNext
        hwBufferProperties.format,                              // format
        hwBufferProperties.suggestedYcbcrModel,                 // ycbcrModel
        hwBufferProperties.suggestedYcbcrRange,                 // ycbcrRange
        componentMapping,                                       // components
        hwBufferProperties.suggestedXChromaOffset,              // xChromaOffset
        hwBufferProperties.suggestedYChromaOffset,              // yChromaOffset
        hardcodedFilter,                                        // chromaFilter
        false,                                                  // forceExplicitReconstruction
    };
}
} // namespace PlatformHardwareBufferUtil
RENDER_END_NAMESPACE()
