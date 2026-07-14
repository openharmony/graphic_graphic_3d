/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MALEOON_GPU_BUFFER_MLN_H
#define MALEOON_GPU_BUFFER_MLN_H

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/maleoon/intf_device_mln.h>

#include "device/gpu_buffer.h"

RENDER_BEGIN_NAMESPACE()

class Device;

// Platform data for acceleration structure resources.
// Mirrors GpuAccelerationStructurePlatformDataVk (gpu_buffer_vk.h).
struct GpuAccelerationStructurePlatformDataMln {
    uint32_t byteSize{0u};
    uint64_t deviceAddress{0};  // AS device address from MlnGetAccelerationStructureDeviceAddress
    MlnAccelerationStructure accelerationStructure{MLN_NULL_HANDLE};
};

class GpuBufferMln final : public GpuBuffer {
public:
    GpuBufferMln(Device& device, const GpuBufferDesc& desc);
    GpuBufferMln(Device& device, const GpuAccelerationStructureDesc& desc);
    ~GpuBufferMln() override;

    const GpuBufferDesc& GetDesc() const override;
    const GpuAccelerationStructureDesc& GetDescAccelerationStructure() const;
    void* Map() override;
    void* MapMemory() override;
    void Unmap() const override;

    const GpuBufferPlatformDataMln& GetPlatformData() const;
    const GpuAccelerationStructurePlatformDataMln& GetPlatformDataAccelerationStructure() const;

    bool IsAccelerationStructure() const
    {
        return isAccelerationStructure_;
    }

private:
    void CreateBuffer();
    void AllocateAndBindMemory();
    uint32_t FindMemoryType(uint32_t memoryTypeBits, MlnMemoryPropertyFlags requiredFlags) const;

    Device& device_;
    GpuBufferDesc desc_;
    GpuBufferPlatformDataMln plat_;

    // Acceleration structure state (B-plan: lazy init)
    bool isAccelerationStructure_{false};
    GpuAccelerationStructureDesc descAccel_;
    GpuAccelerationStructurePlatformDataMln platAccel_;

    bool isMappable_{false};
    bool isPersistentlyMapped_{false};
    bool isRingBuffer_{false};
    bool ownsResources_{true};
    mutable bool isMapped_{false};
    uint32_t bufferingCount_{1u};
};

RENDER_END_NAMESPACE()

#endif  // MALEOON_GPU_BUFFER_MLN_H
