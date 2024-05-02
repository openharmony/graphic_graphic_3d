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

#include "gpu_semaphore_vk.h"

#include <cinttypes>
#include <vulkan/vulkan_core.h>

#include <render/namespace.h>

#include "device/device.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
GpuSemaphoreVk::GpuSemaphoreVk(Device& device) : device_(device)
{
    constexpr VkSemaphoreCreateFlags semaphoreCreateFlags { 0 };
    constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
        VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, // sType
        nullptr,                                 // pNext
        semaphoreCreateFlags,                    // flags
    };

    const VkDevice vkDev = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    VALIDATE_VK_RESULT(vkCreateSemaphore(vkDev, // device
        &semaphoreCreateInfo,                   // pCreateInfo
        nullptr,                                // pAllocator
        &plat_.semaphore));                     // pSemaphore
}

GpuSemaphoreVk::GpuSemaphoreVk(Device& device, const uint64_t handle) : device_(device), ownsResources_(false)
{
    // do not let invalid inputs in
    PLUGIN_ASSERT(handle != 0);
    if (handle) {
        plat_.semaphore = VulkanHandleCast<VkSemaphore>(handle);
    }
}

GpuSemaphoreVk::~GpuSemaphoreVk()
{
    if (ownsResources_ && plat_.semaphore) {
        const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

        vkDestroySemaphore(device, // device
            plat_.semaphore,       // semaphore
            nullptr);              // pAllocator
    }
    plat_.semaphore = VK_NULL_HANDLE;
}

uint64_t GpuSemaphoreVk::GetHandle() const
{
    return VulkanHandleCast<uint64_t>(plat_.semaphore);
}

const GpuSemaphorePlatformDataVk& GpuSemaphoreVk::GetPlatformData() const
{
    return plat_;
}
RENDER_END_NAMESPACE()
