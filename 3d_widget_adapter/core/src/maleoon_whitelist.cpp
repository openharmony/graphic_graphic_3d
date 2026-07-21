/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "maleoon_whitelist.h"

#include <fstream>
#include <set>
#include <string>
#include <unordered_set>

#include <vulkan/vulkan_core.h>

#include <parameter.h>
#include <parameters.h>
#include "param/sys_param.h"

#include "3d_widget_adapter_log.h"

namespace OHOS::Render3D {
namespace {
const std::unordered_set<std::string> kMaleoonAllowedProcesses = {
    "com.huawei.hmos.maps.app",
    "com.kiri.remy",
};

const std::set<std::string> supportedDeviceType = {"phone"};
constexpr uint32_t MALEOON_VENDOR_ID = 0x19e5;
constexpr uint32_t MALEOON_DEVICE_ID_MIN = 0x20001000;

std::string GetCurrentProcessName()
{
    std::ifstream f("/proc/self/cmdline");
    if (!f) {
        return {};
    }
    std::string cmdline;
    std::getline(f, cmdline, '\0');
    return cmdline;
}

bool IsMaleoonAllowedForCurrentProcess()
{
    const std::string proc = GetCurrentProcessName();
    return !proc.empty() && (kMaleoonAllowedProcesses.count(proc) > 0);
}

bool IsDeviceSupportMaleoon(uint32_t vendorId, uint32_t deviceId)
{
    return (vendorId == MALEOON_VENDOR_ID) && (deviceId >= MALEOON_DEVICE_ID_MIN);
}

bool QueryDeviceSupportMaleoon()
{
    bool supportMaleoon = false;
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "RenderConfig";
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        WIDGET_LOGE("vkCreateInstance result: %d", result);
        return supportMaleoon;
    }

    uint32_t physicalDeviceCount = 0;
    result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    if (result != VK_SUCCESS || physicalDeviceCount <= 0) {
        WIDGET_LOGE("vkEnumeratePhysicalDevices fail or physicalDeviceCount <= 0");
        return supportMaleoon;
    }
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    if (result == VK_SUCCESS && !physicalDevices.empty()) {
        VkPhysicalDevice physicalDevice = physicalDevices[0];
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        supportMaleoon = IsDeviceSupportMaleoon(properties.vendorID, properties.deviceID);
    }
    if (instance != VK_NULL_HANDLE) {
        vkDestroyInstance(instance, nullptr);
    }
    return supportMaleoon;
}
}  // namespace

bool IsSupportMaleoon()
{
    const std::string proc = GetCurrentProcessName();
    const std::string device = OHOS::system::GetParameter("const.build.product", "0");
    const bool processAllowed = IsMaleoonAllowedForCurrentProcess();
    const bool deviceSupported = QueryDeviceSupportMaleoon();
    const std::string deviceType = OHOS::system::GetParameter("const.product.devicetype", "0");
    const bool deviceTypeSupported = (supportedDeviceType.count(deviceType) > 0);
    if (processAllowed && deviceSupported && deviceTypeSupported) {
        WIDGET_LOGI("[MLNSwitchState] process '%{public}s' IN whitelist, device '%{public}s' IS supported, "
                    "backend can be maleoon",
            proc.c_str(),
            device.c_str());
        return true;
    } else {
        WIDGET_LOGI("[MLNSwitchState] process '%{public}s', device '%{public}s', "
                    "backend can not be maleoon",
            proc.c_str(),
            device.c_str());
        return false;
    }
}

}  // namespace OHOS::Render3D
