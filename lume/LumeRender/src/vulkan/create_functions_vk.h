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

#ifndef VULKAN_CREATE_FUNCTIONS_VK_H
#define VULKAN_CREATE_FUNCTIONS_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

RENDER_BEGIN_NAMESPACE()
struct LowLevelQueueInfo;
struct VersionInfo;

struct InstanceWrapper {
    VkInstance instance { VK_NULL_HANDLE };
    bool debugReportSupported { false };
    bool debugUtilsSupported { false };
    uint32_t apiMajor { 0u };
    uint32_t apiMinor { 0u };
};

struct PhysicalDeviceWrapper {
    VkPhysicalDevice physicalDevice { VK_NULL_HANDLE };
    BASE_NS::vector<VkExtensionProperties> physicalDeviceExtensions;
    PhysicalDevicePropertiesVk physicalDeviceProperties;
};

struct DeviceWrapper {
    VkDevice device { VK_NULL_HANDLE };
    BASE_NS::vector<BASE_NS::string> extensions;
};

struct QueueProperties {
    VkQueueFlags requiredFlags { 0 };
    uint32_t count { 0 };
    float priority { 1.0f };
    bool explicitFlags { false };
    bool canPresent { false };
};

class CreateFunctionsVk {
public:
    struct Window {
        // Win: hinstance
        // Linux: connection
        // Mac: display
        uintptr_t instance;
        uintptr_t window;
    };

    static InstanceWrapper CreateInstance(const VersionInfo& engineInfo, const VersionInfo& appInfo);
    static InstanceWrapper GetWrapper(VkInstance instance);
    static void DestroyInstance(VkInstance instance);

    static VkDebugReportCallbackEXT CreateDebugCallback(
        VkInstance instance, PFN_vkDebugReportCallbackEXT callbackFunction);
    static void DestroyDebugCallback(VkInstance instance, VkDebugReportCallbackEXT debugReport);

    static VkDebugUtilsMessengerEXT CreateDebugMessenger(
        VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callbackFunction);
    static void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger);

    static PhysicalDeviceWrapper CreatePhysicalDevice(VkInstance instance, QueueProperties const& queueProperties);
    static PhysicalDeviceWrapper GetWrapper(VkPhysicalDevice physicalDevice);
    static bool HasExtension(
        BASE_NS::array_view<const VkExtensionProperties> physicalDeviceExtensions, BASE_NS::string_view extension);

    static DeviceWrapper CreateDevice(VkInstance instance, VkPhysicalDevice physicalDevice,
        const BASE_NS::vector<VkExtensionProperties>& physicalDeviceExtensions,
        const VkPhysicalDeviceFeatures& featuresToEnable, const VkPhysicalDeviceFeatures2* physicalDeviceFeatures2,
        const BASE_NS::vector<LowLevelQueueInfo>& availableQueues,
        const BASE_NS::vector<BASE_NS::string_view>& preferredDeviceExtensions);
    static void DestroyDevice(VkDevice device);

    static VkSurfaceKHR CreateSurface(VkInstance instance, Window const& nativeWindow);
    static void DestroySurface(VkInstance instance, VkSurfaceKHR surface);

    static void DestroySwapchain(VkDevice device, VkSwapchainKHR swapchain);

    static BASE_NS::vector<LowLevelQueueInfo> GetAvailableQueues(
        VkPhysicalDevice physicalDevice, const BASE_NS::vector<QueueProperties>& queueProperties);

    static VkPipelineCache CreatePipelineCache(VkDevice device, BASE_NS::array_view<const uint8_t> initialData);
    static void DestroyPipelineCache(VkDevice device, VkPipelineCache cache);
};
RENDER_END_NAMESPACE()

#endif // VULKAN_CREATE_FUNCTIONS_VK_H
