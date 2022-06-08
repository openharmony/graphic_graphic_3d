/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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

class GpuBufferVk final : public GpuBuffer {
public:
    GpuBufferVk(Device& device, const GpuBufferDesc& desc);
    ~GpuBufferVk();

    const GpuBufferDesc& GetDesc() const override;
    const GpuBufferPlatformDataVk& GetPlatformData() const;

    void* Map() override;
    void* MapMemory() override;
    void Unmap() const override;

private:
    void AllocateMemory(const VkMemoryPropertyFlags requiredFlags, const VkMemoryPropertyFlags preferredFlags);

    Device& device_;

    GpuBufferPlatformDataVk plat_;
    GpuBufferDesc desc_;

    bool isPersistantlyMapped_ { false };
    bool isMappable_ { false };
    bool isRingBuffer_ { false };
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
