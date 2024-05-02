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

#include "gpu_sampler_vk.h"

#include <vulkan/vulkan_core.h>

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "vulkan/device_vk.h"
#include "vulkan/platform_hardware_buffer_util_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
namespace {
VkSamplerYcbcrConversion CreateYcbcrConversion(const DeviceVk& deviceVk, const GpuSamplerDesc& desc)
{
    const DevicePlatformDataVk& devicePlat = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    VkSamplerYcbcrConversion samplerYcbcrConversion = VK_NULL_HANDLE;
    const VkDevice vkDevice = devicePlat.device;
    // NOTE: should be queried from image (hwbuffer)
    PlatformHardwareBufferUtil::HardwareBufferProperties hwBufferProperties;
    VkSamplerYcbcrConversionCreateInfo ycbcrConversionInfo;
    PlatformHardwareBufferUtil::FillYcbcrConversionInfo(deviceVk, hwBufferProperties, ycbcrConversionInfo);
    VALIDATE_VK_RESULT(deviceVk.GetExtFunctions().vkCreateSamplerYcbcrConversion(
        vkDevice, &ycbcrConversionInfo, nullptr, &samplerYcbcrConversion));

    return samplerYcbcrConversion;
}
} // namespace

GpuSamplerVk::GpuSamplerVk(Device& device, const GpuSamplerDesc& desc) : device_(device), desc_(desc)
{
    VkSamplerCreateInfo samplerCreateInfo = {
        VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,   // sType
        nullptr,                                 // pNext
        0,                                       // flags
        (VkFilter)desc.magFilter,                // magFilter
        (VkFilter)desc.minFilter,                // minFilter
        (VkSamplerMipmapMode)desc.mipMapMode,    // mipmapMode
        (VkSamplerAddressMode)desc.addressModeU, // addressModeU
        (VkSamplerAddressMode)desc.addressModeV, // addressModeV
        (VkSamplerAddressMode)desc.addressModeW, // addressModeW
        desc.mipLodBias,                         // mipLodBias
        desc.enableAnisotropy,                   // anisotropyEnable
        desc.maxAnisotropy,                      // maxAnisotropy
        desc.enableCompareOp,                    // compareEnabled
        (VkCompareOp)desc.compareOp,             // compareOp
        desc.minLod,                             // minLod
        desc.maxLod,                             // maxLod
        (VkBorderColor)desc.borderColor,         // borderColor
        desc.enableUnnormalizedCoordinates,      // unnormalizedCoordinates
    };

    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    const DevicePlatformDataVk& devicePlat = (const DevicePlatformDataVk&)device_.GetPlatformData();
    const VkDevice vkDevice = devicePlat.device;
    if ((desc.engineCreationFlags & CORE_ENGINE_SAMPLER_CREATION_YCBCR) &&
        deviceVk.GetCommonDeviceExtensions().samplerYcbcrConversion) {
        samplerConversion_ = CreateYcbcrConversion(deviceVk, desc_);
        VkSamplerYcbcrConversionInfo yCbcrConversionInfo {
            VK_STRUCTURE_TYPE_SAMPLER_YCBCR_CONVERSION_INFO, // sType
            nullptr,                                         // pNext
            samplerConversion_,                              // conversion
        };
        samplerCreateInfo.pNext = &yCbcrConversionInfo;
        VALIDATE_VK_RESULT(vkCreateSampler(vkDevice, // device
            &samplerCreateInfo,                      // pCreateInfo
            nullptr,                                 // pAllocator
            &plat_.sampler));                        // pSampler
    } else {
        VALIDATE_VK_RESULT(vkCreateSampler(vkDevice, // device
            &samplerCreateInfo,                      // pCreateInfo
            nullptr,                                 // pAllocator
            &plat_.sampler));                        // pSampler
    }
}

GpuSamplerVk::~GpuSamplerVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    if (samplerConversion_ != VK_NULL_HANDLE) {
        ((const DeviceVk&)device_)
            .GetExtFunctions()
            .vkDestroySamplerYcbcrConversion(device, // device
                samplerConversion_,                  // ycbcrConversion
                nullptr);                            // pAllocator
    }
    vkDestroySampler(device, // device
        plat_.sampler,       // sampler
        nullptr);            // pAllocator
}

const GpuSamplerDesc& GpuSamplerVk::GetDesc() const
{
    return desc_;
}

const GpuSamplerPlatformDataVk& GpuSamplerVk::GetPlatformData() const
{
    return plat_;
}
RENDER_END_NAMESPACE()
