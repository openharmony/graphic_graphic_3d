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
#ifdef VK_USE_PLATFORM_WIN32_KHR
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) reinterpret_cast<void*>(
            vkGetInstanceProcAddr(instance, "vkCreateWin32SurfaceKHR"));
        if (!vkCreateWin32SurfaceKHR) {
            PLUGIN_LOG_E("Missing VK_KHR_win32_surface extension");
            return surface;
        }
        VkWin32SurfaceCreateInfoKHR const surfaceCreateInfo {
            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, // sType
            nullptr,                                         // pNext
            0,                                               // flags
            HINSTANCE(nativeWindow.instance),                // hinstance
            HWND(nativeWindow.window)                        // hwnd
        };
        VALIDATE_VK_RESULT(vkCreateWin32SurfaceKHR(instance, &surfaceCreateInfo, nullptr, &surface));
#else
#error Missing platform surface type.
#endif
    }

    return surface;
}
RENDER_END_NAMESPACE()
