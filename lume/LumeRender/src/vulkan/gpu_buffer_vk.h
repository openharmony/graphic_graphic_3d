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

#ifndef VULKAN_GPU_BUFFER_VK_H
#define VULKAN_GPU_BUFFER_VK_H

#include <vulkan/vulkan_core.h>

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "device/gpu_buffer.h"
#include "vulkan/gpu_memory_allocator_vk.h"

RENDER_BEGIN_NAMESPACE()
class Device;

struct GpuAccelerationStructurePlatformDataVk final {
    VkBuffer buffer { VK_NULL_HANDLE };
    uint32_t byteSize { 0u };
    uint64_t deviceAddress { 0 };
#if (RENDER_VULKAN_RT_ENABLED == 1)
    VkAccelerationStructureKHR accelerationStructure { VK_NULL_HANDLE };
#endif
};

class GpuBufferVk final : public GpuBuffer {
public:
    GpuBufferVk(Device& device, const GpuBufferDesc& desc);
    GpuBufferVk(Device& device, const GpuAccelerationStructureDesc& desc);
    ~GpuBufferVk();

    const GpuBufferDesc& GetDesc() const override;
    const GpuBufferPlatformDataVk& GetPlatformData() const;
    const GpuAccelerationStructureDesc& GetDescAccelerationStructure() const;
    const GpuAccelerationStructurePlatformDataVk& GetPlatformDataAccelerationStructure() const;

    void* Map() override;
    void* MapMemory() override;
    void Unmap() const override;

private:
    void AllocateMemory(const VkMemoryPropertyFlags requiredFlags, const VkMemoryPropertyFlags preferredFlags);
    void CreateBufferImpl();

    Device& device_;

    GpuBufferPlatformDataVk plat_;
    GpuBufferDesc desc_;

    GpuAccelerationStructurePlatformDataVk platAccel_;
    GpuAccelerationStructureDesc descAccel_;

    bool isPersistantlyMapped_ { false };
    bool isMappable_ { false };
    bool isRingBuffer_ { false };
    bool isAccelerationStructure_ { false };
    uint32_t bufferingCount_ { 1u };

    // debug assert usage only
    mutable bool isMapped_ { false };

    struct MemoryAllocation {
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    };
    MemoryAllocation mem_ {};
};
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_BUFFER_VK_H
