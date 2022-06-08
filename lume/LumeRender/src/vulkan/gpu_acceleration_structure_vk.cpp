/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gpu_acceleration_structure_vk.h"

#include <vulkan/vulkan_core.h>

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "vulkan/device_vk.h"
#include "vulkan/gpu_buffer_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
GpuAccelerationStructureVk::GpuAccelerationStructureVk(Device& device, const GpuAccelerationStructureDesc& desc)
    : device_(device), desc_(desc)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    PLUGIN_ASSERT(desc.bufferDesc.usageFlags & CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT);
    gpuBuffer_ = BASE_NS::make_unique<GpuBufferVk>(device_, desc.bufferDesc);
    const GpuBufferPlatformDataVk& gpuBufferPlat = gpuBuffer_->GetPlatformData();
    plat_.buffer = gpuBufferPlat.buffer;
    plat_.byteSize = gpuBufferPlat.fullByteSize;

    constexpr VkFlags createFlags = 0;
    const VkAccelerationStructureTypeKHR accelerationStructureType =
        static_cast<VkAccelerationStructureTypeKHR>(desc_.accelerationStructureType);
    VkAccelerationStructureCreateInfoKHR createInfo {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR, // sType
        nullptr,                                                  // pNext
        createFlags,                                              // createFlags
        plat_.buffer,                                             // buffer
        0,                                                        // offset
        (VkDeviceSize)plat_.byteSize,                             // size
        accelerationStructureType,                                // type
        0,                                                        // deviceAddress
    };

    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    const DevicePlatformDataVk& devicePlat = (const DevicePlatformDataVk&)device_.GetPlatformData();
    const VkDevice vkDevice = devicePlat.device;

    const DeviceVk::ExtFunctions& extFunctions = deviceVk.GetExtFunctions();
    if (extFunctions.vkCreateAccelerationStructureKHR && extFunctions.vkGetAccelerationStructureDeviceAddressKHR) {
        VALIDATE_VK_RESULT(extFunctions.vkCreateAccelerationStructureKHR(vkDevice, // device
            &createInfo,                                                           // pCreateInfo
            nullptr,                                                               // pAllocator
            &plat_.accelerationStructure));                                        // pAccelerationStructure

        if (plat_.accelerationStructure) {
            const VkAccelerationStructureDeviceAddressInfoKHR addressInfo {
                VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR, // sType
                nullptr,                                                          // pNext
                plat_.accelerationStructure,                                      // accelerationStructure
            };
            plat_.deviceAddress = extFunctions.vkGetAccelerationStructureDeviceAddressKHR(vkDevice, // device
                &addressInfo);                                                                      // pInfo
        }
    }
#endif
}

GpuAccelerationStructureVk::~GpuAccelerationStructureVk()
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    const DeviceVk::ExtFunctions& extFunctions = deviceVk.GetExtFunctions();
    if (extFunctions.vkDestroyAccelerationStructureKHR) {
        extFunctions.vkDestroyAccelerationStructureKHR(device, // device
            plat_.accelerationStructure,                       // accelerationStructure
            nullptr);                                          // pAllocator
    }
    gpuBuffer_.reset();
#endif
}

const GpuAccelerationStructureDesc& GpuAccelerationStructureVk::GetDesc() const
{
    return desc_;
}

const GpuAccelerationStructurePlatformDataVk& GpuAccelerationStructureVk::GetPlatformData() const
{
    return plat_;
}
RENDER_END_NAMESPACE()
