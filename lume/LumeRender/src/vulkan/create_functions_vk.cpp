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

#include "create_functions_vk.h"

#include <algorithm>
#include <cinttypes>
#include <cstring>
#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>

#include "platform_vk.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr const bool CORE_ENABLE_VULKAN_PHYSICAL_DEVICE_PRINT = true;

#define SPLIT_VK_VERSION(version) VK_VERSION_MAJOR(version), VK_VERSION_MINOR(version), VK_VERSION_PATCH(version)

uint32_t GetInstanceApiVersion()
{
    uint32_t apiVersion = VK_VERSION_1_0;
    PFN_vkEnumerateInstanceVersion vkEnumerateInstanceVersionFunc =
        (PFN_vkEnumerateInstanceVersion) reinterpret_cast<void*>(
            vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkEnumerateInstanceVersion"));
    if (vkEnumerateInstanceVersionFunc) {
        const VkResult result = vkEnumerateInstanceVersionFunc(&apiVersion);
        if (result != VK_SUCCESS) {
            apiVersion = VK_VERSION_1_0;
            PLUGIN_LOG_D("vkEnumerateInstanceVersion error: %i", result);
        }
    }
    apiVersion = VK_MAKE_VERSION(VK_VERSION_MAJOR(apiVersion), VK_VERSION_MINOR(apiVersion), 0);
    PLUGIN_LOG_D("enumerated api version for instance creation %u.%u", VK_VERSION_MAJOR(apiVersion),
        VK_VERSION_MINOR(apiVersion));
    return apiVersion;
}

inline void LogPhysicalDeviceProperties(const VkPhysicalDeviceProperties& physicalDeviceProperties)
{
    PLUGIN_LOG_D("api version: %u.%u.%u", SPLIT_VK_VERSION(physicalDeviceProperties.apiVersion));
    PLUGIN_LOG_D("driver version: %u.%u.%u", SPLIT_VK_VERSION(physicalDeviceProperties.driverVersion));
    PLUGIN_LOG_D("vendor id: %x", physicalDeviceProperties.vendorID);
    PLUGIN_LOG_D("device id: %x", physicalDeviceProperties.deviceID);
    PLUGIN_LOG_D("device name: %s", physicalDeviceProperties.deviceName);
    PLUGIN_LOG_D("device type: %d", physicalDeviceProperties.deviceType);
    PLUGIN_LOG_D("timestampPeriod: %f", physicalDeviceProperties.limits.timestampPeriod);
}

inline void LogQueueFamilyProperties(
    VkInstance instance, VkPhysicalDevice physicalDevice, const vector<VkQueueFamilyProperties>& queueFamilyProperties)
{
    PLUGIN_LOG_D("queue count: %zu", queueFamilyProperties.size());
    for (uint32_t idx = 0; idx < queueFamilyProperties.size(); ++idx) {
        const bool canPresent = CanDevicePresent(instance, physicalDevice, idx);
        PLUGIN_UNUSED(canPresent);
        PLUGIN_LOG_D("queue flags: %x", queueFamilyProperties[idx].queueFlags);
        PLUGIN_LOG_D("queue timestampValitBits: %x", queueFamilyProperties[idx].timestampValidBits);
        PLUGIN_LOG_D("queue can present: %d", canPresent);
    }
}

string GetMemoryPropertyFlagsStr(const VkMemoryType memoryType)
{
    const uint32_t flags = memoryType.propertyFlags;
    string flagsStr;
    if (flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
        flagsStr += "VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT ";
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
        flagsStr += "VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT ";
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
        flagsStr += "VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ";
    }
    if (flags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT) {
        flagsStr += "VK_MEMORY_PROPERTY_HOST_CACHED_BIT ";
    }
    if (flags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT) {
        flagsStr += "VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT ";
    }
    if (flags & VK_MEMORY_PROPERTY_PROTECTED_BIT) {
        flagsStr += "VK_MEMORY_PROPERTY_PROTECTED_BIT ";
    }
    return flagsStr;
}

void LogPhysicalDeviceMemoryProperties(const VkPhysicalDeviceMemoryProperties& physicalDeviceMemoryProperties)
{
    PLUGIN_LOG_D("physical device memory properties (for memory property type count %u ): ",
        physicalDeviceMemoryProperties.memoryTypeCount);
    for (uint32_t idx = 0; idx < physicalDeviceMemoryProperties.memoryTypeCount; ++idx) {
        const string flagsString = GetMemoryPropertyFlagsStr(physicalDeviceMemoryProperties.memoryTypes[idx]);
        PLUGIN_LOG_D("%u: memory property flags: %s, (from heap of size: %" PRIu64 ")", idx, flagsString.c_str(),
            physicalDeviceMemoryProperties.memoryHeaps[physicalDeviceMemoryProperties.memoryTypes[idx].heapIndex].size);
    }
}

static vector<VkQueueFamilyProperties> GetQueueFamilieProperties(VkPhysicalDevice physicalDevice)
{
    uint32_t queueFamilyPropertyCount = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, // physicalDevice
        &queueFamilyPropertyCount,                           // pQueueFamilyPropertyCount
        nullptr);                                            // pQueueFamilyProperties

    vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, // physicalDevice
        &queueFamilyPropertyCount,                           // pQueueFamilyPropertyCount
        queueFamilyProperties.data());                       // pQueueFamilyProperties

    return queueFamilyProperties;
}

struct SuitableQueueVk {
    uint32_t queueFamilyIndex { ~0u };
    uint32_t queueCount { 0u };
};

static SuitableQueueVk FindSuitableQueue(
    const vector<VkQueueFamilyProperties>& queueFamilyProperties, const QueueProperties& queueProperties)
{
    for (uint32_t idx = 0; idx < queueFamilyProperties.size(); ++idx) {
        if (queueProperties.explicitFlags) {
            if (!(queueFamilyProperties[idx].queueFlags == queueProperties.requiredFlags)) {
                continue;
            }
        } else {
            if ((queueFamilyProperties[idx].queueFlags & queueProperties.requiredFlags) !=
                queueProperties.requiredFlags) {
                continue;
            }
        }

        return { idx, queueFamilyProperties[idx].queueCount };
    }
    return {};
}
} // namespace

vector<LowLevelQueueInfo> CreateFunctionsVk::GetAvailableQueues(
    VkPhysicalDevice physicalDevice, const vector<QueueProperties>& queueProperties)
{
    vector<LowLevelQueueInfo> availableQueues;
    availableQueues.reserve(queueProperties.size());

    const vector<VkQueueFamilyProperties> queueFamilyProperties = GetQueueFamilieProperties(physicalDevice);

    for (const auto& ref : queueProperties) {
        const SuitableQueueVk suitableQueue = FindSuitableQueue(queueFamilyProperties, ref);
        if (suitableQueue.queueCount > 0) {
            const uint32_t maxQueueCount = std::min(suitableQueue.queueCount, ref.count);
            availableQueues.push_back(
                LowLevelQueueInfo { ref.requiredFlags, suitableQueue.queueFamilyIndex, maxQueueCount, ref.priority });
        }
    }
    return availableQueues;
}

vector<VkExtensionProperties> GetInstanceExtensions()
{
    uint32_t instanceExtensionPropertyCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, // pLayerName
        &instanceExtensionPropertyCount,            // propertyCount
        nullptr);                                   // pProperties
    vector<VkExtensionProperties> instanceExtensions(instanceExtensionPropertyCount);
    vkEnumerateInstanceExtensionProperties(nullptr, // pLayerName
        &instanceExtensionPropertyCount,            // propertyCount
        instanceExtensions.data());                 // pProperties
    return instanceExtensions;
}

vector<VkLayerProperties> GetInstancLayers()
{
    uint32_t instanceLayerPropertyCount = 0;
    vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, // pPropertyCount
        nullptr);                                                   // pProperties
    vector<VkLayerProperties> instanceLayers(instanceLayerPropertyCount);
    vkEnumerateInstanceLayerProperties(&instanceLayerPropertyCount, // pPropertyCount
        instanceLayers.data());                                     // pProperties
    return instanceLayers;
}

InstanceWrapper CreateFunctionsVk::GetWrapper(VkInstance instance)
{
    InstanceWrapper wrapper;
    wrapper.instance = instance;

#if (!defined(NDEBUG) || RENDER_VULKAN_VALIDATION_ENABLED == 1)
    vector<VkExtensionProperties> instanceExtensions = GetInstanceExtensions();

#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    const char* debugExtension = nullptr;
#endif
    for (const auto& ref : instanceExtensions) {
        PLUGIN_LOG_V("%s", ref.extensionName);
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
        if (strcmp(ref.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            wrapper.debugUtilsSupported = true;
            debugExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        } else if (!debugExtension && strcmp(ref.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            wrapper.debugReportSupported = true;
            debugExtension = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        }
#endif
    }
#endif
    const uint32_t apiVersion = GetInstanceApiVersion();
    wrapper.apiMajor = VK_VERSION_MAJOR(apiVersion);
    wrapper.apiMinor = VK_VERSION_MINOR(apiVersion);
    return wrapper;
}

InstanceWrapper CreateFunctionsVk::CreateInstance(const VersionInfo& engineInfo, const VersionInfo& appInfo)
{
    InstanceWrapper wrapper;

    vector<VkExtensionProperties> instanceExtensions = GetInstanceExtensions();

    // NOTE: check extensions...
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    const char* debugExtension = nullptr;
#endif
#if (!defined(NDEBUG) || RENDER_VULKAN_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_V("Vulkan: available extensions:");
    for (const auto& ref : instanceExtensions) {
        PLUGIN_LOG_V("%s", ref.extensionName);
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
        if (strcmp(ref.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            wrapper.debugUtilsSupported = true;
            debugExtension = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
        } else if (!debugExtension && strcmp(ref.extensionName, VK_EXT_DEBUG_REPORT_EXTENSION_NAME) == 0) {
            wrapper.debugReportSupported = true;
            debugExtension = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
        }
#endif
    }
#endif

    vector<const char*> extensions = { VK_KHR_SURFACE_EXTENSION_NAME, GetPlatformSurfaceName() };
#ifdef __APPLE__
    extensions.push_back("VK_KHR_portability_enumeration");
#endif
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    if (debugExtension) {
        extensions.push_back(debugExtension);
    }
#endif
    if (!std::all_of(extensions.begin(), extensions.end(), [&instanceExtensions](auto const requiredExtension) {
            const bool supported = std::any_of(instanceExtensions.begin(), instanceExtensions.end(),
                [&requiredExtension](const auto& supportedExtension) {
                    return (std::strcmp(supportedExtension.extensionName, requiredExtension) == 0);
                });
            if (!supported) {
                PLUGIN_LOG_E("some extensions are not supported! Extension name: %s", requiredExtension);
            }
            return supported;
        })) {
    }

    vector<VkLayerProperties> instanceLayers = GetInstancLayers();
#if (!defined(NDEBUG) || RENDER_VULKAN_VALIDATION_ENABLED == 1)
    PLUGIN_LOG_D("Vulkan: available layers:");
    for (const auto& ref : instanceLayers) {
        PLUGIN_LOG_D("%s", ref.layerName);
    }
#endif

    vector<const char*> layers = {
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
        "VK_LAYER_KHRONOS_validation",
#endif
    };
    layers.erase(std::remove_if(layers.begin(), layers.end(),
                     [&instanceLayers](const char* requiredLayer) {
                         const bool supported = std::any_of(instanceLayers.begin(), instanceLayers.end(),
                             [&requiredLayer](const VkLayerProperties& supportedLayer) {
                                 return (std::strcmp(supportedLayer.layerName, requiredLayer) == 0);
                             });
                         if (!supported) {
                             PLUGIN_LOG_E("some layers are not supported! Layer name: %s", requiredLayer);
                         }
                         return !supported;
                     }),
        layers.cend());

    const uint32_t apiVersion = GetInstanceApiVersion();
    wrapper.apiMajor = VK_VERSION_MAJOR(apiVersion);
    wrapper.apiMinor = VK_VERSION_MINOR(apiVersion);

    const uint32_t engineVersion =
        VK_MAKE_VERSION(engineInfo.versionMajor, engineInfo.versionMinor, engineInfo.versionPatch);
    const uint32_t appVersion = VK_MAKE_VERSION(appInfo.versionMajor, appInfo.versionMinor, appInfo.versionPatch);
    const VkApplicationInfo applicationInfo {
        VK_STRUCTURE_TYPE_APPLICATION_INFO, // sType
        nullptr,                            // pNext
        appInfo.name.c_str(),               // pApplicationName
        appVersion,                         // applicationVersion
        engineInfo.name.c_str(),            // pEngineName
        engineVersion,                      // engineVersion
        apiVersion,                         // apiVersion
    };

    const VkInstanceCreateInfo instanceCreateInfo {
        VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO, // sType
        nullptr,                                // pNext
#ifdef __APPLE__
        VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR, // flags
#else
        0, // flags
#endif
        &applicationInfo,            // pApplicationInfo
        (uint32_t)layers.size(),     // enabledLayerCount
        layers.data(),               // ppEnabledLayerNames
        (uint32_t)extensions.size(), // enabledExtensionCount
        extensions.data(),           // ppEnabledExtensionNames
    };

    VALIDATE_VK_RESULT(vkCreateInstance(&instanceCreateInfo, // pCreateInfo
        nullptr,                                             // pAllocator
        &wrapper.instance));                                 // pInstance
    return wrapper;
}

void CreateFunctionsVk::DestroyInstance(VkInstance instance)
{
    PLUGIN_ASSERT_MSG(instance, "null instance in DestroyInstance()");
    vkDestroyInstance(instance, // instance
        nullptr);               // pAllocator
}

VkDebugReportCallbackEXT CreateFunctionsVk::CreateDebugCallback(
    VkInstance instance, PFN_vkDebugReportCallbackEXT callbackFunction)
{
    VkDebugReportCallbackEXT debugReport { VK_NULL_HANDLE };
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT =
        (PFN_vkCreateDebugReportCallbackEXT) reinterpret_cast<void*>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
    if (!vkCreateDebugReportCallbackEXT) {
        PLUGIN_LOG_W("Missing VK_EXT_debug_report extension");
        return debugReport;
    }

    VkDebugReportCallbackCreateInfoEXT const callbackCreateInfo
    {
        VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT, // sType
            nullptr,                                             // pNext
#if (RENDER_VULKAN_VALIDATION_ENABLE_INFORMATION == 1)
            VK_DEBUG_REPORT_INFORMATION_BIT_EXT |
#endif
#if (RENDER_VULKAN_VALIDATION_ENABLE_WARNINGS == 1)
                VK_DEBUG_REPORT_WARNING_BIT_EXT |
#endif
                VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_DEBUG_REPORT_ERROR_BIT_EXT, // flags
            callbackFunction,                                                                // pfnCallback
            nullptr                                                                          // pUserData
    };
    VALIDATE_VK_RESULT(vkCreateDebugReportCallbackEXT(instance, // instance
        &callbackCreateInfo,                                    // pCreateInfo
        nullptr,                                                // pAllocator
        &debugReport));                                         // pCallback
#endif
    return debugReport;
}

void CreateFunctionsVk::DestroyDebugCallback(VkInstance instance, VkDebugReportCallbackEXT debugReport)
{
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    if (!debugReport) {
        return;
    }
    PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT =
        (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
    if (!vkDestroyDebugReportCallbackEXT) {
        PLUGIN_LOG_W("Missing VK_EXT_debug_report extension");
        return;
    }
    vkDestroyDebugReportCallbackEXT(instance, debugReport, nullptr);
#endif
}

VkDebugUtilsMessengerEXT CreateFunctionsVk::CreateDebugMessenger(
    VkInstance instance, PFN_vkDebugUtilsMessengerCallbackEXT callbackFunction)
{
    VkDebugUtilsMessengerEXT debugMessenger { VK_NULL_HANDLE };
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    PFN_vkCreateDebugUtilsMessengerEXT vkCreateDebugUtilsMessengerEXT =
        (PFN_vkCreateDebugUtilsMessengerEXT) reinterpret_cast<void*>(
            vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    if (!vkCreateDebugUtilsMessengerEXT) {
        PLUGIN_LOG_W("Missing VK_EXT_debug_utils extension");
        return debugMessenger;
    }

    VkDebugUtilsMessengerCreateInfoEXT const messengerCreateInfo {
        VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT, // sType
        nullptr,                                                 // pNext
        0,                                                       // flags
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT, // messageSeverity
        VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT, // messageType
        callbackFunction,                                    // pfnUserCallback
        nullptr                                              // pUserData
    };
    VALIDATE_VK_RESULT(vkCreateDebugUtilsMessengerEXT(instance, // instance
        &messengerCreateInfo,                                   // pCreateInfo
        nullptr,                                                // pAllocator
        &debugMessenger));                                      // pMessenger

#endif
    return debugMessenger;
}

void CreateFunctionsVk::DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger)
{
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    if (!debugMessenger) {
        return;
    }

    PFN_vkDestroyDebugUtilsMessengerEXT vkDestroyDebugUtilsMessengerEXT =
        (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (!vkDestroyDebugUtilsMessengerEXT) {
        PLUGIN_LOG_W("Missing VK_EXT_debug_utils extension");
        return;
    }

    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
}

PhysicalDeviceWrapper CreateFunctionsVk::GetWrapper(VkPhysicalDevice physicalDevice)
{
    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    vkGetPhysicalDeviceFeatures(physicalDevice, &physicalDeviceFeatures);

    uint32_t extensionPropertyCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, nullptr);
    vector<VkExtensionProperties> extensions(extensionPropertyCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionPropertyCount, extensions.data());
    std::sort(extensions.begin(), extensions.end(), [](const VkExtensionProperties& lhs, VkExtensionProperties& rhs) {
        return std::strcmp(lhs.extensionName, rhs.extensionName) < 0;
    });
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &physicalDeviceMemoryProperties);
    if constexpr (CORE_ENABLE_VULKAN_PHYSICAL_DEVICE_PRINT) {
        LogPhysicalDeviceProperties(physicalDeviceProperties);
        for (uint32_t idx = 0; idx < extensions.size(); ++idx) {
            PLUGIN_LOG_V(
                "physical device extension: %s %u", extensions[idx].extensionName, extensions[idx].specVersion);
        }
#ifndef NDEBUG
        LogPhysicalDeviceMemoryProperties(physicalDeviceMemoryProperties);
#endif
    }

    return { physicalDevice, move(extensions),
        { move(physicalDeviceProperties), move(physicalDeviceFeatures), move(physicalDeviceMemoryProperties) } };
}

PhysicalDeviceWrapper CreateFunctionsVk::CreatePhysicalDevice(
    VkInstance instance, QueueProperties const& queueProperties)
{
    uint32_t physicalDeviceCount = 0;
    VALIDATE_VK_RESULT(vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr));

    uint32_t usedPhysicalDeviceCount { 1 }; // only one device, the first
    // some drivers write out physicalDeviceCount instead of usedPhysicalDeviceCount VkPhysicalDevices so we need enough
    // space
    vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount, VK_NULL_HANDLE);
    const VkResult result = vkEnumeratePhysicalDevices(instance, &usedPhysicalDeviceCount, physicalDevices.data());
    PLUGIN_UNUSED(result);
    PLUGIN_ASSERT_MSG((result == VK_SUCCESS || result == VK_INCOMPLETE), "vulkan device enumeration failed");

    const VkPhysicalDevice physicalDevice = physicalDevices[0];

    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);
    vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    if constexpr (CORE_ENABLE_VULKAN_PHYSICAL_DEVICE_PRINT) {
        PLUGIN_LOG_D("physical device count: %u", physicalDeviceCount);
        LogQueueFamilyProperties(instance, physicalDevice, queueFamilyProperties);
    }

    const auto suitableQueue = FindSuitableQueue(queueFamilyProperties, queueProperties);
    if (suitableQueue.queueCount == 0) {
        PLUGIN_LOG_E("No device maching required queues %x or present capabilities %u", queueProperties.requiredFlags,
            queueProperties.canPresent);
        return {};
    }
    return GetWrapper(physicalDevice);
}

bool CreateFunctionsVk::HasExtension(
    array_view<const VkExtensionProperties> physicalDeviceExtensions, string_view extension)
{
    VkExtensionProperties value;
    value.extensionName[extension.copy(
        value.extensionName, Math::min(countof(value.extensionName), extension.size()), 0U)] = '\0';
    value.specVersion = 1;
    return std::binary_search(physicalDeviceExtensions.cbegin(), physicalDeviceExtensions.cend(), value,
        [](const VkExtensionProperties& element, const VkExtensionProperties& value) {
            return std::strcmp(element.extensionName, value.extensionName) < 0;
        });
}

DeviceWrapper CreateFunctionsVk::CreateDevice(VkInstance instance, VkPhysicalDevice physicalDevice,
    const vector<VkExtensionProperties>& physicalDeviceExtensions, const VkPhysicalDeviceFeatures& featuresToEnable,
    const VkPhysicalDeviceFeatures2* physicalDeviceFeatures2, const vector<LowLevelQueueInfo>& availableQueues,
    const vector<string_view>& preferredDeviceExtensions)
{
    PLUGIN_ASSERT_MSG(instance, "null instance in CreateDevice()");
    PLUGIN_ASSERT_MSG(physicalDevice, "null physical device in CreateDevice()");

    DeviceWrapper deviceWrapper;

    vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    queueCreateInfos.reserve(availableQueues.size());
    constexpr uint32_t maxQueuePriorityCount { 8 };
    float queuePriorities[maxQueuePriorityCount];
    uint32_t priorityIndex = 0;
    PLUGIN_LOG_D("creating device with queue(s):");
    vector<LowLevelGpuQueueVk> lowLevelQueues;
    for (const auto& ref : availableQueues) {
        const uint32_t priorityStartIndex = priorityIndex;
        for (uint32_t priorityIdx = 0; priorityIdx < ref.queueCount; ++priorityIdx) {
            PLUGIN_ASSERT(priorityIndex < maxQueuePriorityCount);
            queuePriorities[priorityIndex] = ref.priority;
            priorityIndex++;
        }
        queueCreateInfos.push_back(VkDeviceQueueCreateInfo {
            VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO, // sType
            nullptr,                                    // pNext
            0,                                          // flags
            ref.queueFamilyIndex,                       // queueFamilyIndex
            ref.queueCount,                             // queueCount
            &queuePriorities[priorityStartIndex],       // pQueuePriorities
        });
        PLUGIN_LOG_D(
            "queue(s), flags: %u, family index: %u, count: %u", ref.queueFlags, ref.queueFamilyIndex, ref.queueCount);
    }

    const vector<const char*> layers = {
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
        "VK_LAYER_KHRONOS_validation",
#endif
    };
    vector<const char*> extensions;
    for (const string_view& preferredDeviceExtension : preferredDeviceExtensions) {
        if (HasExtension(physicalDeviceExtensions, preferredDeviceExtension)) {
            extensions.push_back(preferredDeviceExtension.data());
            deviceWrapper.extensions.push_back(preferredDeviceExtension.data());
        }
    }
    if constexpr (CORE_ENABLE_VULKAN_PHYSICAL_DEVICE_PRINT) {
        PLUGIN_LOG_D("enabled extensions:");
        for (uint32_t idx = 0; idx < extensions.size(); ++idx) {
            PLUGIN_LOG_D("%s", extensions[idx]);
        }
    }

    VkDeviceCreateInfo const createInfo {
        VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,                  // sType;
        physicalDeviceFeatures2,                               // pNext;
        0,                                                     // flags;
        (uint32_t)queueCreateInfos.size(),                     // queueCreateInfoCount;
        queueCreateInfos.data(),                               // pQueueCreateInfos;
        uint32_t(layers.size()),                               // enabledLayerCount;
        layers.data(),                                         // ppEnabledLayerNames;
        uint32_t(extensions.size()),                           // enabledExtensionCount;
        extensions.data(),                                     // ppEnabledExtensionNames;
        physicalDeviceFeatures2 ? nullptr : &featuresToEnable, // pEnabledFeatures
    };
    VALIDATE_VK_RESULT(vkCreateDevice(physicalDevice, // physicalDevice
        &createInfo,                                  // pCreateInfo
        nullptr,                                      // pAllocator
        &deviceWrapper.device));                      // pDevice
    return deviceWrapper;
}

void CreateFunctionsVk::DestroyDevice(VkDevice device)
{
    PLUGIN_ASSERT_MSG(device, "null device in DestroyDevice()");
    vkDestroyDevice(device, // device
        nullptr);           // pAllocator
}

void CreateFunctionsVk::DestroySurface(VkInstance instance, VkSurfaceKHR surface)
{
    PLUGIN_ASSERT_MSG(instance, "null instance in DestroySurface()");
    PLUGIN_ASSERT_MSG(surface, "null surface in DestroySurface()");
    PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR =
        (PFN_vkDestroySurfaceKHR)vkGetInstanceProcAddr(instance, "vkDestroySurfaceKHR");
    if (!vkDestroySurfaceKHR) {
        PLUGIN_LOG_E("Missing VK_KHR_surface extension");
        return;
    }

    vkDestroySurfaceKHR(instance, // instance
        surface,                  // surface
        nullptr);                 // pAllocator
}

void CreateFunctionsVk::DestroySwapchain(VkDevice device, VkSwapchainKHR swapchain)
{
    if (device && swapchain) {
        vkDestroySwapchainKHR(device, swapchain, nullptr);
    }
}

VkPipelineCache CreateFunctionsVk::CreatePipelineCache(VkDevice device, array_view<const uint8_t> initialData)
{
    VkPipelineCache pipelineCache = VK_NULL_HANDLE;

    PLUGIN_ASSERT_MSG(device, "null device in CreatePipelineCache()");

    const auto info = VkPipelineCacheCreateInfo {
        VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO, // sType
        nullptr,                                      // pNext
        0,                                            // flags
        initialData.size(),                           // initialDataSize
        initialData.data(),                           // pInitialData
    };
    VALIDATE_VK_RESULT(vkCreatePipelineCache(device, &info, nullptr, &pipelineCache));

    return pipelineCache;
}

void CreateFunctionsVk::DestroyPipelineCache(VkDevice device, VkPipelineCache pipelineCache)
{
    PLUGIN_ASSERT_MSG(device, "null device in DestroyPipelineCache()");
    PLUGIN_ASSERT_MSG(pipelineCache, "null pipelineCache in DestroyPipelineCache()");

    vkDestroyPipelineCache(device, pipelineCache, nullptr);
}
RENDER_END_NAMESPACE()
