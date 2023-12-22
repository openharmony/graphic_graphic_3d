/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
