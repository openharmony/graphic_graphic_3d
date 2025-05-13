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

#ifndef VULKAN_PLATFORM_HARDWARE_BUFFER_UTIL_VK_H
#define VULKAN_PLATFORM_HARDWARE_BUFFER_UTIL_VK_H

#include <vulkan/vulkan_core.h>

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class DeviceVk;
struct GpuBufferDesc;
struct GpuImageDesc;

namespace PlatformHardwareBufferUtil {
struct HardwareBufferProperties {
    VkDeviceSize allocationSize { 0 };
    uint32_t memoryTypeBits { 0 };

    VkFormat format { VK_FORMAT_UNDEFINED };
    uint64_t externalFormat { 0 };
    VkFormatFeatureFlags formatFeatures { 0 };
    VkComponentMapping samplerYcbcrConversionComponents { VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY, VK_COMPONENT_SWIZZLE_IDENTITY };
    VkSamplerYcbcrModelConversion suggestedYcbcrModel { VK_SAMPLER_YCBCR_MODEL_CONVERSION_RGB_IDENTITY };
    VkSamplerYcbcrRange suggestedYcbcrRange { VK_SAMPLER_YCBCR_RANGE_ITU_FULL };
    VkChromaLocation suggestedXChromaOffset { VK_CHROMA_LOCATION_COSITED_EVEN };
    VkChromaLocation suggestedYChromaOffset { VK_CHROMA_LOCATION_COSITED_EVEN };
};

HardwareBufferProperties QueryHwBufferFormatProperties(const DeviceVk& deviceVk, uintptr_t hwBuffer);
uint32_t GetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties,
    uint32_t memoryTypeBits, VkMemoryPropertyFlags memoryPropertyFlags);
VkImageCreateInfo GetHwBufferImageCreateInfo(const GpuImageDesc& desc);
VkMemoryRequirements GetImageMemoryRequirements(
    const DeviceVk& deviceVk, VkImage image, VkImageAspectFlags imageAspectFlags, bool useMemoryRequirements2);

struct HardwareBufferImage {
    VkImage image;
    VkDeviceMemory deviceMemory;
};
HardwareBufferImage CreateHwPlatformImage(const DeviceVk& deviceVk, const HardwareBufferProperties& hwBufferProperties,
    const GpuImageDesc& desc, uintptr_t hwBuffer);
void DestroyHwPlatformImage(const DeviceVk& deviceVk, VkImage image, VkDeviceMemory deviceMemory);

void FillYcbcrConversionInfo(
    const HardwareBufferProperties& hwBufferProperties, VkSamplerYcbcrConversionCreateInfo& ycbcrConversionCreateInfo);

struct HardwareBufferBuffer {
    VkBuffer buffer;
    VkDeviceMemory deviceMemory;
};
HardwareBufferBuffer CreateHwPlatformBuffer(const DeviceVk& deviceVk,
    const HardwareBufferProperties& hwBufferProperties, const GpuBufferDesc& desc, uintptr_t hwBuffer);
void DestroyHwPlatformBuffer(const DeviceVk& deviceVk, VkBuffer buffer, VkDeviceMemory deviceMemory);
} // namespace PlatformHardwareBufferUtil
RENDER_END_NAMESPACE()

#endif // VULKAN_PLATFORM_HARDWARE_BUFFER_UTIL_VK_H
