/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#ifndef VULKAN_LINUX_PLATFORM_VK_H
#define VULKAN_LINUX_PLATFORM_VK_H

#include <vulkan/vulkan.h>

#include <base/containers/string_view.h>
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
#endif // VULKAN_LINUX_PLATFORM_VK_H