/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "platform_vk.h"

#include <vulkan/vulkan.h>

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::vector;

bool CanDevicePresent(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily)
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
    PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR vkGetPhysicalDeviceWin32PresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceWin32PresentationSupportKHR) reinterpret_cast<void*>(
            vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceWin32PresentationSupportKHR"));
    if (!vkGetPhysicalDeviceWin32PresentationSupportKHR) {
        PLUGIN_LOG_E("Missing VK_KHR_win32_surface extension");
        return false;
    }

    return vkGetPhysicalDeviceWin32PresentationSupportKHR(device, queuefamily) == VK_TRUE;
#else
#warning "Undefined WSI platform!"
    return false;
#endif
}

const char* GetPlatformSurfaceName()
{
#ifdef VK_USE_PLATFORM_WIN32_KHR
    return VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
#else
#error Missing platform surface type.
#endif
}
RENDER_END_NAMESPACE()