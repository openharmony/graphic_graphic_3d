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
#include "vulkan/gpu_image_vk.h"
#include "vulkan/platform_hardware_buffer_util_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
namespace {
VkSamplerCreateInfo CreateYcbcrSamplerCreateInfo()
{
    return VkSamplerCreateInfo {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, // sType
        nullptr,                               // pNext
        0,                                     // flags
        VK_FILTER_NEAREST,                     // magFilter
        VK_FILTER_NEAREST,                     // minFilter
        VK_SAMPLER_MIPMAP_MODE_NEAREST,        // mipmapMode
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
        VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
        0.0f,                                  // mipLodBias
        false,                                 // anisotropyEnable
        1.0f,                                  // maxAnisotropy
        false,                                 // compareEnabled
        VK_COMPARE_OP_NEVER,                   // compareOp
        0.0f,                                  // minLod
        0.0f,                                  // maxLod
        VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,    // borderColor
        false,                                 // unnormalizedCoordinates
    };
}
} // namespace

void GpuImageVk::CreatePlatformHwBuffer()
{
    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    const PlatformDeviceExtensions& deviceExtensions = deviceVk.GetPlatformDeviceExtensions();
    if ((hwBuffer_ != 0) && deviceExtensions.externalMemoryHardwareBuffer) {
        const PlatformHardwareBufferUtil::HardwareBufferProperties hwBufferProperties =
            PlatformHardwareBufferUtil::QueryHwBufferFormatProperties(deviceVk, hwBuffer_);
        if (hwBufferProperties.allocationSize > 0) {
            PlatformHardwareBufferUtil::HardwareBufferImage hwBufferImage =
                PlatformHardwareBufferUtil::CreateHwPlatformImage(deviceVk, hwBufferProperties, desc_, hwBuffer_);
            PLUGIN_ASSERT((hwBufferImage.image != VK_NULL_HANDLE) && ((hwBufferImage.deviceMemory != VK_NULL_HANDLE)));
            plat_.image = hwBufferImage.image;
            mem_.allocationInfo.deviceMemory = hwBufferImage.deviceMemory;

            if (plat_.format == VK_FORMAT_UNDEFINED) {
                // with external format, chained conversion info is needed
                VkSamplerYcbcrConversionCreateInfo ycbcrConversionCreateInfo;
                PlatformHardwareBufferUtil::FillYcbcrConversionInfo(
                    deviceVk, hwBufferProperties, ycbcrConversionCreateInfo);

                VkExternalFormatOHOS externalFormat {
                    VK_STRUCTURE_TYPE_EXTERNAL_FORMAT_OHOS, // sType
                    nullptr,                                // pNext
                    hwBufferProperties.externalFormat,      // externalFormat
                };
                ycbcrConversionCreateInfo.pNext = &externalFormat;

                const DevicePlatformDataVk& devicePlat = (const DevicePlatformDataVk&)device_.GetPlatformData();
                const VkDevice vkDevice = devicePlat.device;

                VALIDATE_VK_RESULT(deviceVk.GetExtFunctions().vkCreateSamplerYcbcrConversion(
                    vkDevice, &ycbcrConversionCreateInfo, nullptr, &platConversion_.samplerConversion));

                const VkSamplerYcbcrConversionInfo yCbcrConversionInfo {
                    VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO, // sType
                    nullptr,                                         // pNext
                    platConversion_.samplerConversion,               // conversion
                };

                VkSamplerCreateInfo samplerCreateInfo = CreateYcbcrSamplerCreateInfo();
                samplerCreateInfo.pNext = &yCbcrConversionInfo;
                VALIDATE_VK_RESULT(vkCreateSampler(vkDevice, // device
                    &samplerCreateInfo,                      // pCreateInfo
                    nullptr,                                 // pAllocator
                    &platConversion_.sampler));              // pSampler

                CreateVkImageViews(VK_IMAGE_ASPECT_COLOR_BIT, &yCbcrConversionInfo);
            } else {
                CreateVkImageViews(VK_IMAGE_ASPECT_COLOR_BIT, nullptr);
            }
        } else {
            hwBuffer_ = 0;
        }
    }
}

void GpuImageVk::DestroyPlatformHwBuffer()
{
    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    PlatformHardwareBufferUtil::DestroyHwPlatformImage(deviceVk, plat_.image, mem_.allocationInfo.deviceMemory);
    const DevicePlatformDataVk& devicePlat = (const DevicePlatformDataVk&)device_.GetPlatformData();
    const VkDevice device = devicePlat.device;
    if (platConversion_.samplerConversion != VK_NULL_HANDLE) {
        deviceVk.GetExtFunctions().vkDestroySamplerYcbcrConversion(device, // device
            platConversion_.samplerConversion,                             // ycbcrConversion
            nullptr);                                                      // pAllocator
    }
    if (platConversion_.sampler != VK_NULL_HANDLE) {
        vkDestroySampler(device,     // device
            platConversion_.sampler, // sampler
            nullptr);                // pAllocator
    }
}
RENDER_END_NAMESPACE()
