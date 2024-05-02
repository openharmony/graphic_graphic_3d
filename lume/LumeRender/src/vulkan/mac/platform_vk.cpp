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

static constexpr string_view DEVICE_EXTENSION_PORTABILITY_SUBSET { "VK_KHR_portability_subset" };

void GetPlatformDeviceExtensions(vector<string_view>& extensions)
{
    extensions.push_back(DEVICE_EXTENSION_PORTABILITY_SUBSET);
}

bool CanDevicePresent(VkInstance instance, VkPhysicalDevice device, uint32_t queuefamily)
{
#if defined(VK_USE_PLATFORM_MACOS_MVK)
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
#if defined(VK_USE_PLATFORM_MACOS_MVK)
    return VK_MVK_MACOS_SURFACE_EXTENSION_NAME;
#elif defined(VK_USE_PLATFORM_METAL_EXT)
    return VK_EXT_METAL_SURFACE_EXTENSION_NAME;
#else
#error Missing platform surface type.
#endif
}
RENDER_END_NAMESPACE()