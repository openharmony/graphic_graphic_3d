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

#include "gpu_buffer_mln.h"
#include "mln_log.h"
#include "util/log.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "maleoon/device_mln.h"
#include "mln_convert.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {
constexpr uint32_t GetAlignedByteSize(const uint32_t byteSize, const uint32_t alignment)
{
    return (byteSize + alignment - 1) & (~(alignment - 1));
}
// Maleoon runs on top of Vulkan driver — same hardware alignment requirements apply.
// ARM GPU (HiSilicon/Mali) typical: minUniformBufferOffsetAlignment=256, minStorageBufferOffsetAlignment=16.
// Use 256 to cover both UBO and SSBO alignment for ring buffer offset stepping.
constexpr uint32_t MIN_BUFFER_ALIGNMENT = 256u;
} // namespace

GpuBufferMln::GpuBufferMln(Device& device, const GpuBufferDesc& desc) : device_(device), desc_(desc)
{
    MLN_LOG_INIT("GpuBufferMln: creating buffer (size=%u, usage=0x%x, memFlags=0x%x, ringBuf=%d)", desc.byteSize,
        desc.usageFlags, desc.memoryPropertyFlags,
        (desc.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER) ? 1 : 0);
    PLUGIN_ASSERT(desc_.byteSize > 0);

    const bool isDynamicRingBuffer = (desc_.engineCreationFlags & CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER) != 0;
    isRingBuffer_ = isDynamicRingBuffer;
    isMappable_ = (desc_.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
    isPersistentlyMapped_ = isMappable_ && (desc_.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;

    if (isRingBuffer_) {
        bufferingCount_ = device_.GetCommandBufferingCount();
        if (bufferingCount_ < 1u) {
            bufferingCount_ = 1u;
        }
    }

    const uint32_t bindByteSize = desc_.byteSize;
    // Align bindMemoryByteSize for ring buffers / persistently mapped buffers.
    // Ring buffer currentByteOffset steps by bindMemoryByteSize each frame;
    // unaligned steps violate hardware minUniformBufferOffsetAlignment (typically 256).
    const uint32_t alignment = (isRingBuffer_ || isPersistentlyMapped_) ? MIN_BUFFER_ALIGNMENT : 1u;
    plat_.bindMemoryByteSize = GetAlignedByteSize(bindByteSize, alignment);
    plat_.fullByteSize = plat_.bindMemoryByteSize * bufferingCount_;
    plat_.currentByteOffset = 0u;
    plat_.usage = ToMlnBufferUsageFlags(desc_.usageFlags);

    CreateBuffer();
    if (!plat_.resource) {
        MLN_LOG_ERR("GpuBufferMln: CreateBuffer failed — skipping memory allocation (size=%u, usage=0x%x)",
            plat_.fullByteSize, static_cast<uint32_t>(plat_.usage));
        return;
    }
    AllocateAndBindMemory();
    if (!plat_.memory) {
        MLN_LOG_ERR("GpuBufferMln: AllocateAndBindMemory failed — skipping map (size=%u)", plat_.fullByteSize);
        return;
    }

    if (isPersistentlyMapped_ && plat_.memory) {
        MlnMemoryMapDescriptor mapDesc{};
        mapDesc.flags = 0;
        mapDesc.memory = plat_.memory;
        mapDesc.offset = 0;
        mapDesc.size = plat_.fullByteSize;
        void* data = nullptr;
        const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
        if (MlnMapMemory(deviceMln.GetMlnDevice(), &mapDesc, &data) == MLN_STATUS_SUCCESS) {
            plat_.mappedData = data;
        }
    }
}

GpuBufferMln::GpuBufferMln(Device& device, const GpuAccelerationStructureDesc& desc)
    : device_(device), desc_(desc.bufferDesc), isAccelerationStructure_(true), descAccel_(desc)
{
    MLN_LOG_INIT("GpuBufferMln(AS): creating AS buffer (type=%u, size=%u)",
        static_cast<uint32_t>(desc.accelerationStructureType), desc.bufferDesc.byteSize);
    PLUGIN_ASSERT(desc_.byteSize > 0);

    // AS buffers are device-local, not ring/mappable.
    isMappable_ = false;
    isPersistentlyMapped_ = false;
    isRingBuffer_ = false;

    plat_.bindMemoryByteSize = desc_.byteSize;
    plat_.fullByteSize = desc_.byteSize;
    plat_.currentByteOffset = 0u;
    plat_.usage = ToMlnBufferUsageFlags(desc_.usageFlags);

    CreateBuffer();
    if (!plat_.resource) {
        MLN_LOG_ERR("GpuBufferMln(AS): CreateBuffer failed");
        return;
    }
    AllocateAndBindMemory();
    if (!plat_.memory) {
        MLN_LOG_ERR("GpuBufferMln(AS): AllocateAndBindMemory failed");
        return;
    }

    // Get buffer device address (needed for scratch data and geometry input).
    plat_.deviceAddress =
        MlnGetBufferResourceAddress(static_cast<const DeviceMln&>(device_).GetMlnDevice(), plat_.resource);

    // Create MlnAccelerationStructure immediately (mirrors Vulkan's vkCreateAccelerationStructureKHR
    // in GpuBufferVk constructor). primitiveCount comes from GpuAccelerationStructureDesc, which
    // the upper layer fills from geometry info before calling CreateGpuBuffer.
    platAccel_.byteSize = desc_.byteSize;
    const MlnAccelerationStructureType mlnType =
        (desc.accelerationStructureType == AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL)
            ? MLN_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL
            : MLN_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    const MlnAccelerationStructureDescriptor asDesc{
        0,                                               // extensionCount
        nullptr,                                         // extensions
        0,                                               // flags
        0,                             // primitiveCount (Maleoon-specific, driver needs this)
        plat_.resource,                                  // buffer
        0,                                               // offset
        static_cast<MlnDeviceSize>(platAccel_.byteSize), // size
        mlnType,                                         // type
        plat_.deviceAddress,                             // deviceAddress
    };

    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    platAccel_.accelerationStructure = MlnCreateAccelerationStructure(deviceMln.GetMlnDevice(), &asDesc);
    if (platAccel_.accelerationStructure) {
        platAccel_.deviceAddress =
            MlnGetAccelerationStructureDeviceAddress(deviceMln.GetMlnDevice(), platAccel_.accelerationStructure);
        MLN_LOG_INIT("GpuBufferMln(AS): MlnCreateAccelerationStructure OK (addr=0x%llx)",
            static_cast<unsigned long long>(platAccel_.deviceAddress));
    } else {
        MLN_LOG_ERR(
            "GpuBufferMln(AS): MlnCreateAccelerationStructure FAILED "
            "size=%u, type=%u)",
            platAccel_.byteSize, static_cast<uint32_t>(mlnType));
    }
}

GpuBufferMln::~GpuBufferMln()
{
    MLN_LOG_INIT("GpuBufferMln: destroying buffer (isAS=%d)", isAccelerationStructure_ ? 1 : 0);
    if (!ownsResources_) {
        return;
    }
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    // Destroy acceleration structure handle (if created via EnsureAccelerationStructure)
    if (isAccelerationStructure_ && platAccel_.accelerationStructure) {
        MlnDestroyAccelerationStructure(mlnDevice, platAccel_.accelerationStructure);
        platAccel_.accelerationStructure = MLN_NULL_HANDLE;
    }

    if (plat_.mappedData && isPersistentlyMapped_ && plat_.memory) {
        MlnMemoryUnmapDescriptor unmapDesc{};
        unmapDesc.flags = 0;
        unmapDesc.memory = plat_.memory;
        MlnUnmapMemory(mlnDevice, &unmapDesc);
        plat_.mappedData = nullptr;
    }
    if (plat_.memory) {
        MlnFreeMemory(mlnDevice, plat_.memory);
        plat_.memory = MLN_NULL_HANDLE;
    }
    if (plat_.resource) {
        MlnDestroyResource(mlnDevice, plat_.resource);
        plat_.resource = MLN_NULL_HANDLE;
    }
}

void GpuBufferMln::CreateBuffer()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    MlnBufferDescriptor bufDesc{};
    bufDesc.extensionCount = 0;
    bufDesc.extensions = nullptr;
    bufDesc.flags = 0;
    bufDesc.size = plat_.fullByteSize;
    bufDesc.usage = plat_.usage;

    plat_.resource = MlnCreateBufferResource(deviceMln.GetMlnDevice(), &bufDesc);
    if (!plat_.resource) {
        MLN_LOG_ERR("GpuBufferMln: MlnCreateBufferResource failed (size=%u)", plat_.fullByteSize);
    }
}

void GpuBufferMln::AllocateAndBindMemory()
{
    if (!plat_.resource) {
        return;
    }
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    MlnMemoryRequirements memReqs{};
    MlnResourceMemoryRequirementsDescriptor memReqDesc{};
    memReqDesc.extensionCount = 0;
    memReqDesc.extensions = nullptr;
    memReqDesc.resource = plat_.resource;
    memReqDesc.aspectMask = 0;
    MlnGetResourceMemoryRequirements(mlnDevice, &memReqDesc, &memReqs);

    const MlnMemoryPropertyFlags requiredFlags = ToMlnMemoryPropertyFlags(desc_.memoryPropertyFlags);
    const uint32_t memTypeIndex = FindMemoryType(memReqs.memoryTypeBits, requiredFlags);

    MlnMemoryAllocateDescriptor allocDesc{};
    allocDesc.extensionCount = 0;
    allocDesc.extensions = nullptr;
    allocDesc.allocationSize = memReqs.size;
    allocDesc.memoryTypeIndex = memTypeIndex;

    plat_.memory = MlnAllocateMemory(mlnDevice, &allocDesc);
    if (!plat_.memory) {
        MLN_LOG_ERR(
            "GpuBufferMln: MlnAllocateMemory failed (size=%llu)", static_cast<unsigned long long>(memReqs.size));
        return;
    }

    MlnBindResourceMemoryDescriptor bindDesc{};
    bindDesc.extensionCount = 0;
    bindDesc.extensions = nullptr;
    bindDesc.resource = plat_.resource;
    bindDesc.aspectMask = 0;
    bindDesc.memory = plat_.memory;
    bindDesc.offset = 0;
    MlnStatus result = MlnBindResourceMemory(mlnDevice, &bindDesc);
    if (result != MLN_STATUS_SUCCESS) {
        MLN_LOG_ERR("GpuBufferMln: MlnBindResourceMemory failed");
    }
}

uint32_t GpuBufferMln::FindMemoryType(uint32_t memoryTypeBits, MlnMemoryPropertyFlags requiredFlags) const
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    const auto& memProps = deviceMln.GetCachedMemoryProperties();

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memoryTypeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
            return i;
        }
    }
    // Fallback: return first compatible type
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if (memoryTypeBits & (1u << i)) {
            return i;
        }
    }
    MLN_LOG_ERR("GpuBufferMln: no suitable memory type found");
    return 0;
}

const GpuBufferDesc& GpuBufferMln::GetDesc() const
{
    return desc_;
}

void* GpuBufferMln::Map()
{
    if (!isMappable_) {
        return nullptr;
    }
    PLUGIN_ASSERT(!isMapped_);
    isMapped_ = true;

    // Advance ring buffer offset
    if (isRingBuffer_ && bufferingCount_ > 1u) {
        plat_.currentByteOffset = (plat_.currentByteOffset + plat_.bindMemoryByteSize) % plat_.fullByteSize;
    }

    if (isPersistentlyMapped_ && plat_.mappedData) {
        return static_cast<uint8_t*>(plat_.mappedData) + plat_.currentByteOffset;
    }

    // On-demand mapping
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnMemoryMapDescriptor mapDesc{};
    mapDesc.flags = 0;
    mapDesc.memory = plat_.memory;
    mapDesc.offset = plat_.currentByteOffset;
    mapDesc.size = plat_.bindMemoryByteSize;
    void* data = nullptr;
    if (MlnMapMemory(deviceMln.GetMlnDevice(), &mapDesc, &data) == MLN_STATUS_SUCCESS) {
        return data;
    }
    return nullptr;
}

void* GpuBufferMln::MapMemory()
{
    if (!isMappable_) {
        return nullptr;
    }
    PLUGIN_ASSERT(!isMapped_);
    isMapped_ = true;

    if (isPersistentlyMapped_ && plat_.mappedData) {
        return plat_.mappedData;
    }

    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnMemoryMapDescriptor mapDesc{};
    mapDesc.flags = 0;
    mapDesc.memory = plat_.memory;
    mapDesc.offset = 0;
    mapDesc.size = plat_.fullByteSize;
    void* data = nullptr;
    if (MlnMapMemory(deviceMln.GetMlnDevice(), &mapDesc, &data) == MLN_STATUS_SUCCESS) {
        return data;
    }
    return nullptr;
}

void GpuBufferMln::Unmap() const
{
    isMapped_ = false;

    if (isPersistentlyMapped_) {
        // Flush non-coherent persistently mapped memory to make writes visible to GPU
        if (plat_.memory && !(desc_.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
            const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
            MlnMappedMemoryRangeDescriptor range{};
            range.memory = plat_.memory;
            range.offset = plat_.currentByteOffset;
            range.size = plat_.bindMemoryByteSize;
            MlnFlushMappedMemoryRanges(deviceMln.GetMlnDevice(), 1, &range);
        }
        return; // stays mapped
    }

    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    // Flush non-coherent memory before unmapping
    if (plat_.memory && !(desc_.memoryPropertyFlags & CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT)) {
        MlnMappedMemoryRangeDescriptor range{};
        range.memory = plat_.memory;
        range.offset = plat_.currentByteOffset;
        range.size = plat_.bindMemoryByteSize;
        MlnFlushMappedMemoryRanges(deviceMln.GetMlnDevice(), 1, &range);
    }

    MlnMemoryUnmapDescriptor unmapDesc{};
    unmapDesc.flags = 0;
    unmapDesc.memory = plat_.memory;
    MlnUnmapMemory(deviceMln.GetMlnDevice(), &unmapDesc);
}

const GpuBufferPlatformDataMln& GpuBufferMln::GetPlatformData() const
{
    return plat_;
}

const GpuAccelerationStructureDesc& GpuBufferMln::GetDescAccelerationStructure() const
{
    return descAccel_;
}

const GpuAccelerationStructurePlatformDataMln& GpuBufferMln::GetPlatformDataAccelerationStructure() const
{
    return platAccel_;
}

RENDER_END_NAMESPACE()
