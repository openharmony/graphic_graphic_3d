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

    if (nativeWindow.window) {
#if defined(VK_USE_PLATFORM_MACOS_MVK)
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
#elif defined(VK_USE_PLATFORM_METAL_EXT)
        PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT =
            (PFN_vkCreateMetalSurfaceEXT)vkGetInstanceProcAddr(instance, "vkCreateMetalSurfaceEXT");
        if (!vkCreateMetalSurfaceEXT) {
            PLUGIN_LOG_E("Missing VK_MVK_macos_surface extension");
            return surface;
        }
        VkMetalSurfaceCreateInfoEXT const surfaceCreateInfo {
            VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT, // sType
            nullptr,                                         // pNext
            0,                                               // flags
            reinterpret_cast<void*>(nativeWindow.window),    // window
        };
        VALIDATE_VK_RESULT(vkCreateMetalSurfaceEXT(instance, &surfaceCreateInfo, nullptr, &surface));
#else
#error Missing platform surface type.
#endif
    }

    return surface;
}
RENDER_END_NAMESPACE()
