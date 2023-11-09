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
#ifndef VULKAN_WINDOWS_PLATFORM_VK_H
#define VULKAN_WINDOWS_PLATFORM_VK_H

#include <vulkan/vulkan_core.h>

#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/namespace.h>

#include "vulkan/platform_hardware_buffer_util_vk.h"

RENDER_BEGIN_NAMESPACE()
class DeviceVk;
struct PlatformDeviceExtensions {};
struct PlatformExtFunctions {};

inline void GetPlatformDeviceExtensions(BASE_NS::vector<BASE_NS::string_view>& extensions) {}

inline PlatformDeviceExtensions GetEnabledPlatformDeviceExtensions(
    const BASE_NS::unordered_map<BASE_NS::string, uint32_t>& enabledDeviceExtensions)
{
    return {};
}

bool CanDevicePresent(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily);

const char* GetPlatformSurfaceName();
RENDER_END_NAMESPACE()
#endif // VULKAN_WINDOWS_PLATFORM_VK_H