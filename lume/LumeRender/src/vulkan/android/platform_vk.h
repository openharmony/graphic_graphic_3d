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

#ifndef VULKAN_ANDROID_PLATFORM_VK_H
#define VULKAN_ANDROID_PLATFORM_VK_H

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct PlatformDeviceExtensions {
    // android hardware buffers supported
    bool externalMemoryHardwareBuffer { false };
};

struct PlatformExtFunctions {
    // VK_ANDROID_external_memory_android_hardware_buffer
    PFN_vkGetAndroidHardwareBufferPropertiesANDROID vkGetAndroidHardwareBufferPropertiesANDROID { nullptr };
    PFN_vkGetMemoryAndroidHardwareBufferANDROID vkGetMemoryAndroidHardwareBufferANDROID { nullptr };
};

void GetPlatformDeviceExtensions(BASE_NS::vector<BASE_NS::string_view>& extensions);

PlatformDeviceExtensions GetEnabledPlatformDeviceExtensions(
    const BASE_NS::unordered_map<BASE_NS::string, uint32_t>& enabledDeviceExtensions);

inline const char* GetPlatformSurfaceName()
{
    return VK_KHR_ANDROID_SURFACE_EXTENSION_NAME;
}

inline bool CanDevicePresent(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily)
{
    // [31.4.1.] On Android, all physical devices and queue families must be capable of presentation with any native
    // window.
    return true;
}
RENDER_END_NAMESPACE()
#endif // VULKAN_ANDROID_PLATFORM_VK_H