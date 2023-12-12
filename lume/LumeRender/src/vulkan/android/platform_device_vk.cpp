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

#include <vulkan/vulkan.h>

#include "util/log.h"
#include "vulkan/device_vk.h"

RENDER_BEGIN_NAMESPACE()
namespace {
void GetAndroidHwBufferExtFunctions(const VkInstance instance, PlatformExtFunctions& extFunctions)
{
    extFunctions.vkGetAndroidHardwareBufferPropertiesANDROID =
        (PFN_vkGetAndroidHardwareBufferPropertiesANDROID)vkGetInstanceProcAddr(
            instance, "vkGetAndroidHardwareBufferPropertiesANDROID");
    if (!extFunctions.vkGetAndroidHardwareBufferPropertiesANDROID) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkGetAndroidHardwareBufferPropertiesANDROID");
    }
    extFunctions.vkGetMemoryAndroidHardwareBufferANDROID =
        (PFN_vkGetMemoryAndroidHardwareBufferANDROID)vkGetInstanceProcAddr(
            instance, "vkGetMemoryAndroidHardwareBufferANDROID");
    if (!extFunctions.vkGetMemoryAndroidHardwareBufferANDROID) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkGetMemoryAndroidHardwareBufferANDROID");
    }
}
} // namespace

void DeviceVk::CreatePlatformExtFunctions()
{
    if (platformDeviceExtensions_.externalMemoryHardwareBuffer) {
        GetAndroidHwBufferExtFunctions(plat_.instance, platformExtFunctions_);
    }
}
RENDER_END_NAMESPACE()
