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

#include <vulkan/vulkan.h>

#include "vulkan/device_vk.h"
#include "vulkan/gpu_buffer_vk.h"
#include "vulkan/platform_hardware_buffer_util_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
void GpuBufferVk::CreatePlatformHwBuffer()
{
    const auto& deviceVk = static_cast<const DeviceVk&>(device_);
    if (!plat_.platformHwBuffer || !deviceVk.GetPlatformDeviceExtensions().externalMemoryHardwareBuffer) {
        return;
    }
    const PlatformExtFunctions& extFunctions = deviceVk.GetPlatformExtFunctions();
    if (!extFunctions.vkGetNativeBufferPropertiesOHOS) {
        return;
    }
    const auto& platData = (const DevicePlatformDataVk&)deviceVk.GetPlatformData();
    VkDevice device = platData.device;

    VkNativeBufferPropertiesOHOS bufferProperties {};
    bufferProperties.sType = VK_STRUCTURE_TYPE_NATIVE_BUFFER_PROPERTIES_OHOS;

    OH_NativeBuffer* nativeBuffer = static_cast<OH_NativeBuffer*>(reinterpret_cast<void*>(plat_.platformHwBuffer));
    VALIDATE_VK_RESULT(extFunctions.vkGetNativeBufferPropertiesOHOS(device, // device
        nativeBuffer,                                                       // buffer
        &bufferProperties));                                                // pProperties

    PlatformHardwareBufferUtil::HardwareBufferProperties hardwareBufferProperties;
    hardwareBufferProperties.allocationSize = bufferProperties.allocationSize;
    hardwareBufferProperties.memoryTypeBits = bufferProperties.memoryTypeBits;

    if (hardwareBufferProperties.allocationSize > 0) {
        PlatformHardwareBufferUtil::HardwareBufferBuffer hwBufferImage =
            PlatformHardwareBufferUtil::CreateHwPlatformBuffer(
                deviceVk, hardwareBufferProperties, desc_, plat_.platformHwBuffer);
        PLUGIN_ASSERT((hwBufferImage.buffer != VK_NULL_HANDLE) && ((hwBufferImage.deviceMemory != VK_NULL_HANDLE)));
        plat_.buffer = hwBufferImage.buffer;
        plat_.bindMemoryByteSize = hardwareBufferProperties.allocationSize;
        plat_.fullByteSize = hardwareBufferProperties.allocationSize;
        plat_.currentByteOffset = 0;
        plat_.usage = static_cast<VkBufferUsageFlags>(desc_.usageFlags);
        mem_.allocationInfo.deviceMemory = hwBufferImage.deviceMemory;
    } else {
        plat_.platformHwBuffer = 0;
    }
}

void GpuBufferVk::DestroyPlatformHwBuffer()
{
    const auto& deviceVk = (const DeviceVk&)device_;
    PlatformHardwareBufferUtil::DestroyHwPlatformBuffer(deviceVk, plat_.buffer, mem_.allocationInfo.deviceMemory);
}
RENDER_END_NAMESPACE()
