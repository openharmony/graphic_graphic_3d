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

#include "vulkan/gpu_resource_util_vk.h"

#include <vulkan/vulkan_core.h>

#include <base/containers/allocator.h>
#include <base/containers/byte_array.h>
#include <base/containers/string_view.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "vulkan/device_vk.h"
#include "vulkan/gpu_buffer_vk.h"
#include "vulkan/gpu_image_vk.h"
#include "vulkan/gpu_sampler_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace GpuResourceUtil {
void CopyGpuBufferVk(GpuBuffer& buffer, ByteArray& byteArray)
{
    GpuBufferVk& vkBuffer = (GpuBufferVk&)buffer;
    if (const void* resData = vkBuffer.MapMemory(); resData) {
        const GpuBufferDesc& desc = vkBuffer.GetDesc();
        CloneData(byteArray.GetData().data(), byteArray.GetData().size_bytes(), (const uint8_t*)resData, desc.byteSize);
        vkBuffer.Unmap();
    }
}

void DebugObjectNameVk(
    const IDevice& device, const VkObjectType objectType, const uint64_t castedHandle, const string_view name)
{
    const auto& devicePlat = static_cast<const DevicePlatformDataVk&>(device.GetPlatformData());
    const auto& funcPtrs = (static_cast<const DeviceVk&>(device)).GetDebugFunctionUtilities();
    if (castedHandle && funcPtrs.vkSetDebugUtilsObjectNameEXT) {
        const VkDebugUtilsObjectNameInfoEXT info { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr,
            objectType, castedHandle, name.data() };
        funcPtrs.vkSetDebugUtilsObjectNameEXT(devicePlat.device, &info);
    }
}

void DebugBufferNameVk(const IDevice& device, const GpuBuffer& buffer, const string_view name)
{
    const auto& devicePlat = static_cast<const DevicePlatformDataVk&>(device.GetPlatformData());
    const auto& funcPtrs = (static_cast<const DeviceVk&>(device)).GetDebugFunctionUtilities();
    if (funcPtrs.vkSetDebugUtilsObjectNameEXT) {
        const GpuBufferPlatformDataVk& plat = (static_cast<const GpuBufferVk&>(buffer)).GetPlatformData();
        if (plat.buffer) {
            const VkDebugUtilsObjectNameInfoEXT info { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr,
                VK_OBJECT_TYPE_BUFFER, VulkanHandleCast<uint64_t>(plat.buffer), name.data() };
            funcPtrs.vkSetDebugUtilsObjectNameEXT(devicePlat.device, &info);
        }
    }
}

void DebugImageNameVk(const IDevice& device, const GpuImage& image, const string_view name)
{
    const auto& devicePlat = static_cast<const DevicePlatformDataVk&>(device.GetPlatformData());
    const auto& funcPtrs = (static_cast<const DeviceVk&>(device)).GetDebugFunctionUtilities();
    if (funcPtrs.vkSetDebugUtilsObjectNameEXT) {
        const GpuImagePlatformDataVk& plat = static_cast<const GpuImagePlatformDataVk&>(image.GetBasePlatformData());
        if (plat.image) {
            const VkDebugUtilsObjectNameInfoEXT img { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr,
                VK_OBJECT_TYPE_IMAGE, VulkanHandleCast<uint64_t>(plat.image), name.data() };
            funcPtrs.vkSetDebugUtilsObjectNameEXT(devicePlat.device, &img);
        }
        if (plat.imageView) {
            const VkDebugUtilsObjectNameInfoEXT view { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr,
                VK_OBJECT_TYPE_IMAGE_VIEW, VulkanHandleCast<uint64_t>(plat.imageView), name.data() };
            funcPtrs.vkSetDebugUtilsObjectNameEXT(devicePlat.device, &view);
        }
        if (plat.imageViewBase) {
            const VkDebugUtilsObjectNameInfoEXT viewBase { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr,
                VK_OBJECT_TYPE_IMAGE_VIEW, VulkanHandleCast<uint64_t>(plat.imageViewBase), name.data() };
            funcPtrs.vkSetDebugUtilsObjectNameEXT(devicePlat.device, &viewBase);
        }
    }
}

void DebugSamplerNameVk(const IDevice& device, const GpuSampler& sampler, const string_view name)
{
    const auto& devicePlat = static_cast<const DevicePlatformDataVk&>(device.GetPlatformData());
    const auto& funcPtrs = (static_cast<const DeviceVk&>(device)).GetDebugFunctionUtilities();
    if (funcPtrs.vkSetDebugUtilsObjectNameEXT) {
        const GpuSamplerPlatformDataVk& plat = (static_cast<const GpuSamplerVk&>(sampler)).GetPlatformData();
        if (plat.sampler) {
            const VkDebugUtilsObjectNameInfoEXT info { VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT, nullptr,
                VK_OBJECT_TYPE_SAMPLER, VulkanHandleCast<uint64_t>(plat.sampler), name.data() };
            funcPtrs.vkSetDebugUtilsObjectNameEXT(devicePlat.device, &info);
        }
    }
}
} // namespace GpuResourceUtil
RENDER_END_NAMESPACE()
