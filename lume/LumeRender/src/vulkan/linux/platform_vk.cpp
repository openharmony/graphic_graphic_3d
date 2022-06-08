/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */
#include "platform_vk.h"

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
using BASE_NS::string_view;
using BASE_NS::vector;

bool CanDevicePresent(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily)
{
#ifdef defined(VK_USE_PLATFORM_XCB_KHR)
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
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    // NSView is always presentable
    return true;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    // Metal layers are always presentable
    return true;
#else
#warning "Undefined WSI platform!"
    return false;
#endif
}

const char* GetPlatformSurfaceName()
{
#if defined(VK_KHR_XCB_SURFACE_EXTENSION_NAME)
    return VK_KHR_XCB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
    return VK_KHR_XLIB_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
    return VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    return VK_EXT_METAL_SURFACE_EXTENSION_NAME;
#else
#error Missing platform surface type.
#endif
}
RENDER_END_NAMESPACE()