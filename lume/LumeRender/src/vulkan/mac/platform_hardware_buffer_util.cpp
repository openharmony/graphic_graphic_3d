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

#include <vulkan/vulkan.h>

#include "vulkan/device_vk.h"
#include "vulkan/platform_hardware_buffer_util_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
namespace PlatformHardwareBufferUtil {
HardwareBufferProperties QueryHwBufferFormatProperties(const DeviceVk& deviceVk, uintptr_t hwBuffer)
{
    return HardwareBufferProperties {};
}

HardwareBufferImage CreateHwPlatformImage(const DeviceVk& deviceVk, const HardwareBufferProperties& hwBufferProperties,
    const GpuImageDesc& desc, uintptr_t hwBuffer)
{
    HardwareBufferImage hwBufferImage;
    VkImageCreateInfo imageCreateInfo = GetHwBufferImageCreateInfo(desc);
    const bool useExternalFormat = (imageCreateInfo.format == VK_FORMAT_UNDEFINED);

    const DevicePlatformDataVk& platData = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    VkDevice device = platData.device;
    VALIDATE_VK_RESULT(vkCreateImage(device, // device
        &imageCreateInfo,                    // pCreateInfo
        nullptr,                             // pAllocator
        &hwBufferImage.image));              // pImage

    if (!useExternalFormat) {
        // NOTE: image aspect flags should be queried
        const VkImageAspectFlags imageAspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        // requirements are just queried to pass validation (hw buffer requirements are in hwBufferProperties)
        PlatformHardwareBufferUtil::GetImageMemoryRequirements(deviceVk, hwBufferImage.image, imageAspectFlags, false);
    }
    // get memory type index based on
    const uint32_t memoryTypeIndex =
        GetMemoryTypeIndex(platData.physicalDeviceProperties.physicalDeviceMemoryProperties,
            hwBufferProperties.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    VkMemoryAllocateInfo memoryAllocateInfo {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, // sType
        nullptr,                                // pNext
        hwBufferProperties.allocationSize,      // allocationSize
        memoryTypeIndex,                        // memoryTypeIndex
    };

    VALIDATE_VK_RESULT(vkAllocateMemory(device,  // device
        &memoryAllocateInfo,                     // pAllocateInfo
        nullptr,                                 // pAllocator
        &hwBufferImage.deviceMemory));           // pMemory
    VALIDATE_VK_RESULT(vkBindImageMemory(device, // device
        hwBufferImage.image,                     // image
        hwBufferImage.deviceMemory,              // memory
        0));                                     // memoryOffset

    return hwBufferImage;
}
} // namespace PlatformHardwareBufferUtil
RENDER_END_NAMESPACE()
