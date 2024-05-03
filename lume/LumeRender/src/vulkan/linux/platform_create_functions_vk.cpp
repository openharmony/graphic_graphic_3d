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

#include <vulkan/vulkan.h>

#include "util/log.h"
#include "vulkan/create_functions_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
VkSurfaceKHR CreateFunctionsVk::CreateSurface(VkInstance instance, Window const& nativeWindow)
{
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    PLUGIN_ASSERT_MSG(instance, "null instance in CreateSurface()");

    if (nativeWindow.window && nativeWindow.instance) {
#if defined(VK_USE_PLATFORM_XCB_KHR)
        PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR =
            (PFN_vkCreateXcbSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXcbSurfaceKHR");
        if (!vkCreateXcbSurfaceKHR) {
            PLUGIN_LOG_E("Missing VK_KHR_xcb_surface extension");
            return surface;
        }
        VkXcbSurfaceCreateInfoKHR const surfaceCreateInfo {
            VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,              // sType
            nullptr,                                                    // pNext
            0,                                                          // flags
            reinterpret_cast<xcb_connection_t*>(nativeWindow.instance), // connection
            (xcb_window_t)nativeWindow.window,                          // window
        };
        VALIDATE_VK_RESULT(vkCreateXcbSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));
#elif defined(VK_USE_PLATFORM_XLIB_KHR)
        PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR =
            (PFN_vkCreateXlibSurfaceKHR)vkGetInstanceProcAddr(instance, "vkCreateXlibSurfaceKHR");
        if (!vkCreateXlibSurfaceKHR) {
            PLUGIN_LOG_E("Missing VK_KHR_xlib_surface extension");
            return surface;
        }
        VkXlibSurfaceCreateInfoKHR const surfaceCreateInfo {
            VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,    // sType
            nullptr,                                           // pNext
            0,                                                 // flags
            reinterpret_cast<Display*>(nativeWindow.instance), // dpy
            nativeWindow.window                                // window
        };
        VALIDATE_VK_RESULT(vkCreateXlibSurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));
#elif defined(VK_USE_PLATFORM_MACOS_MVK)
        PFN_vkCreateMacOSSurfaceMVK vkCreateMacOSSurfaceMVK =
            (PFN_vkCreateMacOSSurfaceMVK)vkGetInstanceProcAddr(instance, "vkCreateMacOSSurfaceMVK");
        if (!vkCreateMacOSSurfaceMVK) {
            PLUGIN_LOG_E("Missing VK_MVK_macos_surface extension");
            return surface;
        }
        VkMacOSSurfaceCreateInfoMVK const surfaceCreateInfo {
            VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK, // sType
            nullptr,                                         // pNext
            0,                                               // flags
            reinterpret_cast<void*>(nativeWindow.window),    // window
        };
        VALIDATE_VK_RESULT(vkCreateMacOSSurfaceMVK(instance, &surfaceCreateInfo, nullptr, &surface));
#else
#error Missing platform surface type.
#endif
    }

    return surface;
}
RENDER_END_NAMESPACE()
