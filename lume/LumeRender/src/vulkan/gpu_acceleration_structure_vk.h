/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_GPU_ACCELERATION_STRUCTURE_VK_H
#define VULKAN_GPU_ACCELERATION_STRUCTURE_VK_H

#include <vulkan/vulkan_core.h>

#include <base/containers/unique_ptr.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "device/gpu_acceleration_structure.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuBufferVk;

struct GpuAccelerationStructurePlatformDataVk final : public GpuAccelerationStructurePlatformData {
    VkBuffer buffer { VK_NULL_HANDLE };
    uint32_t byteSize { 0u };
    uint64_t deviceAddress { 0 };
#if (RENDER_VULKAN_RT_ENABLED == 1)
    VkAccelerationStructureKHR accelerationStructure { VK_NULL_HANDLE };
#endif
};

class GpuAccelerationStructureVk final : public GpuAccelerationStructure {
public:
    GpuAccelerationStructureVk(Device& device, const GpuAccelerationStructureDesc& desc);
    ~GpuAccelerationStructureVk();

    const GpuAccelerationStructureDesc& GetDesc() const override;
    const GpuAccelerationStructurePlatformDataVk& GetPlatformData() const;

private:
    Device& device_;

    GpuAccelerationStructurePlatformDataVk plat_;
    GpuAccelerationStructureDesc desc_;

    BASE_NS::unique_ptr<GpuBufferVk> gpuBuffer_;
};
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_ACCELERATION_STRUCTURE_VK_H
