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

#include <vulkan/vulkan.h>

#include <base/util/formats.h>

#include "vulkan/device_vk.h"
#include "vulkan/platform_hardware_buffer_util_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
namespace PlatformHardwareBufferUtil {
HardwareBufferProperties QueryHwBufferFormatProperties(const DeviceVk& deviceVk, uintptr_t hwBuffer)
{
    HardwareBufferProperties hardwareBufferProperties;

    const DevicePlatformDataVk& devicePlat = ((const DevicePlatformDataVk&)deviceVk.GetPlatformData());
    const VkDevice device = devicePlat.device;
    const PlatformExtFunctions& extFunctions = deviceVk.GetPlatformExtFunctions();

    AHardwareBuffer* aHwBuffer = static_cast<AHardwareBuffer*>(reinterpret_cast<void*>(hwBuffer));
    if (aHwBuffer && extFunctions.vkGetAndroidHardwareBufferPropertiesANDROID &&
        extFunctions.vkGetMemoryAndroidHardwareBufferANDROID) {
        VkAndroidHardwareBufferFormatPropertiesANDROID hwFormatProperties;
        hwFormatProperties.sType = VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_FORMAT_PROPERTIES_ANDROID;
        hwFormatProperties.pNext = nullptr;

        VkAndroidHardwareBufferPropertiesANDROID hwProperties {
            VK_STRUCTURE_TYPE_ANDROID_HARDWARE_BUFFER_PROPERTIES_ANDROID, // sType
            &hwFormatProperties,                                          // pNext
            0,                                                            // allocationSize
            0,                                                            // memoryTypeBits
        };
        VALIDATE_VK_RESULT(extFunctions.vkGetAndroidHardwareBufferPropertiesANDROID(device, // device
            aHwBuffer,                                                                      // buffer
            &hwProperties));                                                                // pProperties

        PLUGIN_ASSERT_MSG(hwProperties.allocationSize > 0, "android hw buffer allocation size is zero");
        PLUGIN_ASSERT_MSG(hwFormatProperties.externalFormat != 0, "android hw buffer externalFormat cannot be 0");

        hardwareBufferProperties.allocationSize = hwProperties.allocationSize;
        hardwareBufferProperties.memoryTypeBits = hwProperties.memoryTypeBits;

        hardwareBufferProperties.format = hwFormatProperties.format;
        hardwareBufferProperties.externalFormat = hwFormatProperties.externalFormat;
        hardwareBufferProperties.formatFeatures = hwFormatProperties.formatFeatures;
        hardwareBufferProperties.samplerYcbcrConversionComponents = hwFormatProperties.samplerYcbcrConversionComponents;
        hardwareBufferProperties.suggestedYcbcrModel = hwFormatProperties.suggestedYcbcrModel;
        hardwareBufferProperties.suggestedYcbcrRange = hwFormatProperties.suggestedYcbcrRange;
        hardwareBufferProperties.suggestedXChromaOffset = hwFormatProperties.suggestedXChromaOffset;
        hardwareBufferProperties.suggestedYChromaOffset = hwFormatProperties.suggestedYChromaOffset;
    }

    return hardwareBufferProperties;
}

HardwareBufferImage CreateHwPlatformImage(const DeviceVk& deviceVk, const HardwareBufferProperties& hwBufferProperties,
    const GpuImageDesc& desc, uintptr_t hwBuffer)
{
    HardwareBufferImage hwBufferImage;
    GpuImageDesc validDesc = desc;
    const bool useExternalFormat = (validDesc.format == BASE_NS::BASE_FORMAT_UNDEFINED);
    if (useExternalFormat) {
        validDesc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT;
    }
    VkImageCreateInfo imageCreateInfo = GetHwBufferImageCreateInfo(validDesc);

    VkExternalFormatANDROID externalFormat {
        VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_ANDROID, // sType
        nullptr,                                   // pNext
        hwBufferProperties.externalFormat,         // externalFormat
    };
    VkExternalMemoryImageCreateInfo externalMemoryImageCreateInfo {
        VK_STRUCTURE_TYPE_EXTERNAL_MEMORY_IMAGE_CREATE_INFO,                // sType
        nullptr,                                                            // pNext
        VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID, // handleTypes
    };
    imageCreateInfo.pNext = &externalMemoryImageCreateInfo;
    if (useExternalFormat) { // chain external format
        externalMemoryImageCreateInfo.pNext = &externalFormat;
    }

    const DevicePlatformDataVk& platData = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    VkDevice device = platData.device;
    VALIDATE_VK_RESULT(vkCreateImage(device, // device
        &imageCreateInfo,                    // pCreateInfo
        nullptr,                             // pAllocator
        &hwBufferImage.image));              // pImage

    // some older version of validation layers required calling vkGetImageMemoryRequirements before
    // vkAllocateMemory. with at least 1.3.224.1 it's an error to call vkGetImageMemoryRequirements:
    // "If image was created with the VK_EXTERNAL_MEMORY_HANDLE_TYPE_ANDROID_HARDWARE_BUFFER_BIT_ANDROID external memory
    // handle type, then image must be bound to memory."

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

    AHardwareBuffer* aHwBuffer = static_cast<AHardwareBuffer*>(reinterpret_cast<void*>(hwBuffer));
    PLUGIN_ASSERT(aHwBuffer);
    VkMemoryDedicatedAllocateInfo dedicatedMemoryAllocateInfo {
        VK_STRUCTURE_TYPE_MEMORY_DEDICATED_ALLOCATE_INFO, // sType;
        nullptr,                                          // pNext
        hwBufferImage.image,                              // image
        VK_NULL_HANDLE,                                   // buffer
    };
    VkImportAndroidHardwareBufferInfoANDROID importHardwareBufferInfoAndroid {
        VK_STRUCTURE_TYPE_IMPORT_ANDROID_HARDWARE_BUFFER_INFO_ANDROID, // sType
        &dedicatedMemoryAllocateInfo,                                  // pNext
        aHwBuffer,                                                     // buffer
    };
    memoryAllocateInfo.pNext = &importHardwareBufferInfoAndroid;

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
