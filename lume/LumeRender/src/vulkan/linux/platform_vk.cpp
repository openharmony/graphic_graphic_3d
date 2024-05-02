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

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::vector;

bool CanDevicePresent(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily)
{
#if defined(VK_USE_PLATFORM_XCB_KHR)
    PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR vkGetPhysicalDeviceXcbPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXcbPresentationSupportKHR)vkGetInstanceProcAddr(
            instance, "vkGetPhysicalDeviceXcbPresentationSupportKHR");
    if (!vkGetPhysicalDeviceXcbPresentationSupportKHR) {
        PLUGIN_LOG_E("Missing VK_KHR_xcb_surface extension");
        return false;
    }

    return false;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR vkGetPhysicalDeviceXlibPresentationSupportKHR =
        (PFN_vkGetPhysicalDeviceXlibPresentationSupportKHR)vkGetInstanceProcAddr(
            instance, "vkGetPhysicalDeviceXlibPresentationSupportKHR");
    if (!vkGetPhysicalDeviceXlibPresentationSupportKHR) {
        PLUGIN_LOG_E("Missing VK_KHR_xlib_surface extension");
        return false;
    }

    return false;
#else
#warning "Undefined WSI platform!"
    return false;
#endif
}

const char* GetPlatformSurfaceName()
{
#if defined(VK_USE_PLATFORM_XCB_KHR)
    return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#else
#error Missing platform surface type.
#endif
}
RENDER_END_NAMESPACE()