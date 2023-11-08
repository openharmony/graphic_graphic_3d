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

#include "device_vk.h"

#include <algorithm>
#include <cinttypes>
#include <cstdint>
#include <vulkan/vulkan.h>

#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <core/engine_info.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_program_util.h"
#include "device/gpu_resource_manager.h"
#include "device/shader_manager.h"
#include "device/shader_module.h"
#include "platform_vk.h"
#include "util/log.h"
#include "vulkan/create_functions_vk.h"
#include "vulkan/gpu_acceleration_structure_vk.h"
#include "vulkan/gpu_buffer_vk.h"
#include "vulkan/gpu_image_vk.h"
#include "vulkan/gpu_memory_allocator_vk.h"
#include "vulkan/gpu_program_vk.h"
#include "vulkan/gpu_query_vk.h"
#include "vulkan/gpu_sampler_vk.h"
#include "vulkan/node_context_descriptor_set_manager_vk.h"
#include "vulkan/node_context_pool_manager_vk.h"
#include "vulkan/pipeline_state_object_vk.h"
#include "vulkan/render_backend_vk.h"
#include "vulkan/render_frame_sync_vk.h"
#include "vulkan/shader_module_vk.h"
#include "vulkan/swapchain_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
static constexpr string_view DEVICE_EXTENSION_SWAPCHAIN { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

// promoted to 1.2, requires VK_KHR_create_renderpass2
static constexpr string_view DEVICE_EXTENSION_DEPTH_STENCIL_RESOLVE { VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME };
static constexpr string_view DEVICE_EXTENSION_CREATE_RENDERPASS2 { VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME };

static constexpr string_view DEVICE_EXTENSION_EXTERNAL_MEMORY { VK_KHR_EXTERNAL_MEMORY_EXTENSION_NAME };
static constexpr string_view DEVICE_EXTENSION_GET_MEMORY_REQUIREMENTS2 {
    VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
};
static constexpr string_view DEVICE_EXTENSION_SAMPLER_YCBCR_CONVERSION {
    VK_KHR_SAMPLER_YCBCR_CONVERSION_EXTENSION_NAME
};
static constexpr string_view DEVICE_EXTENSION_QUEUE_FAMILY_FOREIGN { VK_EXT_QUEUE_FAMILY_FOREIGN_EXTENSION_NAME };

void GetYcbcrExtFunctions(const VkInstance instance, DeviceVk::ExtFunctions& extFunctions)
{
    extFunctions.vkCreateSamplerYcbcrConversion =
        (PFN_vkCreateSamplerYcbcrConversion)vkGetInstanceProcAddr(instance, "vkCreateSamplerYcbcrConversion");
    if (!extFunctions.vkCreateSamplerYcbcrConversion) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkCreateSamplerYcbcrConversion");
    }
    extFunctions.vkDestroySamplerYcbcrConversion =
        (PFN_vkDestroySamplerYcbcrConversion)vkGetInstanceProcAddr(instance, "vkDestroySamplerYcbcrConversion");
    if (!extFunctions.vkDestroySamplerYcbcrConversion) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkDestroySamplerYcbcrConversion");
    }
}

// ray-tracing
#if (RENDER_VULKAN_RT_ENABLED == 1)
static constexpr string_view DEVICE_EXTENSION_ACCELERATION_STRUCTURE { "VK_KHR_acceleration_structure" };
static constexpr string_view DEVICE_EXTENSION_RAY_QUERY { "VK_KHR_ray_query" };
static constexpr string_view DEVICE_EXTENSION_DEFERRED_HOST_OPERATIONS { "VK_KHR_deferred_host_operations" };
static constexpr string_view DEVICE_EXTENSION_RAY_TRACING_PIPELINE { "VK_KHR_ray_tracing_pipeline" };
static constexpr string_view DEVICE_EXTENSION_PIPELINE_LIBRARY { "VK_KHR_pipeline_library" };
#endif

constexpr uint32_t MIN_ALLOCATION_BLOCK_SIZE { 4u * 1024u * 1024u };
constexpr uint32_t MAX_ALLOCATION_BLOCK_SIZE { 1024u * 1024u * 1024u };
static constexpr const QueueProperties DEFAULT_QUEUE {
    VK_QUEUE_GRAPHICS_BIT, // requiredFlags
    1,                     // count
    1.0f,                  // priority
    false,                 // explicitFlags
    true,                  // canPresent
};

PlatformGpuMemoryAllocator::GpuMemoryAllocatorCreateInfo GetAllocatorCreateInfo(const BackendExtraVk* backendExtra)
{
    // create default pools
    PlatformGpuMemoryAllocator::GpuMemoryAllocatorCreateInfo createInfo;
    uint32_t dynamicUboByteSize = 16u * 1024u * 1024u;
    if (backendExtra) {
        const auto& sizes = backendExtra->gpuMemoryAllocatorSizes;
        if (sizes.defaultAllocationBlockSize != ~0u) {
            createInfo.preferredLargeHeapBlockSize = Math::min(
                MAX_ALLOCATION_BLOCK_SIZE, Math::max(sizes.defaultAllocationBlockSize, MIN_ALLOCATION_BLOCK_SIZE));
        }
        if (sizes.customAllocationDynamicUboBlockSize != ~0u) {
            dynamicUboByteSize = Math::min(MAX_ALLOCATION_BLOCK_SIZE,
                Math::max(sizes.customAllocationDynamicUboBlockSize, MIN_ALLOCATION_BLOCK_SIZE));
        }
    }

    // staging
    {
        GpuBufferDesc desc;
        desc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_SINGLE_SHOT_STAGING;
        desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                   MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_TRANSFER_SRC_BIT;
        createInfo.customPools.push_back({
            "STAGING_GPU_BUFFER",
            PlatformGpuMemoryAllocator::MemoryAllocatorResourceType::GPU_BUFFER,
            0u,
            // if linear allocator is used, depending clients usage pattern, memory can be easily wasted.
            false,
            { move(desc) },
        });
    }
    // dynamic uniform ring buffers
    {
        GpuBufferDesc desc;
        desc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER;
        desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                   MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        createInfo.customPools.push_back({
            "DYNAMIC_UNIFORM_GPU_BUFFER",
            PlatformGpuMemoryAllocator::MemoryAllocatorResourceType::GPU_BUFFER,
            dynamicUboByteSize,
            false,
            { move(desc) },
        });
    }

    return createInfo;
}

VkBool32 VKAPI_PTR DebugMessengerCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    if (pCallbackData && pCallbackData->pMessage) {
        if ((VkDebugUtilsMessageSeverityFlagsEXT)messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            PLUGIN_LOG_E("%s: %s", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        } else if ((VkDebugUtilsMessageSeverityFlagsEXT)messageSeverity &
                   (VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
            PLUGIN_LOG_W("%s: %s", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        } else if ((VkDebugUtilsMessageSeverityFlagsEXT)messageSeverity &
                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
            PLUGIN_LOG_I("%s: %s", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        } else if ((VkDebugUtilsMessageSeverityFlagsEXT)messageSeverity &
                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) {
            PLUGIN_LOG_V("%s: %s", pCallbackData->pMessageIdName, pCallbackData->pMessage);
        }
        PLUGIN_ASSERT_MSG(
            ((VkDebugUtilsMessageSeverityFlagsEXT)messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) == 0,
            "VALIDATION ERROR");
    }

    // The application should always return VK_FALSE.
    return VK_FALSE;
}

VkBool32 VKAPI_PTR DebugReportCallback(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT, uint64_t, size_t,
    int32_t, const char*, const char* pMessage, void*)
{
    if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        PLUGIN_LOG_E("%s", pMessage);
    } else if (flags & (VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)) {
        PLUGIN_LOG_W("%s", pMessage);
    } else if (flags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        PLUGIN_LOG_I("%s", pMessage);
    } else if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        PLUGIN_LOG_D("%s", pMessage);
    }
    PLUGIN_ASSERT_MSG((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) == 0, "VALIDATION ERROR");
    return VK_TRUE;
}

void EmplaceDeviceQueue(
    const VkDevice device, const LowLevelQueueInfo& aQueueInfo, vector<LowLevelGpuQueueVk>& aLowLevelQueues)
{
    for (uint32_t idx = 0; idx < aQueueInfo.queueCount; ++idx) {
        VkQueue queue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device,         // device
            aQueueInfo.queueFamilyIndex, // queueFamilyIndex
            idx,                         // queueIndex
            &queue);                     // pQueue
        aLowLevelQueues.push_back(LowLevelGpuQueueVk { queue, aQueueInfo });
    }
}

void CheckValidDepthFormats(const DevicePlatformDataVk& devicePlat, DevicePlatformInternalDataVk& dataInternal)
{
    constexpr uint32_t DEPTH_FORMAT_COUNT { 4 };
    constexpr Format DEPTH_FORMATS[DEPTH_FORMAT_COUNT] = { BASE_FORMAT_D24_UNORM_S8_UINT, BASE_FORMAT_D32_SFLOAT,
        BASE_FORMAT_D16_UNORM, BASE_FORMAT_X8_D24_UNORM_PACK32 };
    for (uint32_t idx = 0; idx < DEPTH_FORMAT_COUNT; ++idx) {
        VkFormatProperties formatProperties;
        Format format = DEPTH_FORMATS[idx];
        vkGetPhysicalDeviceFormatProperties(devicePlat.physicalDevice, // physicalDevice
            (VkFormat)format,                                          // format
            &formatProperties);                                        // pFormatProperties
        const VkFormatFeatureFlags optimalTilingFeatureFlags = formatProperties.optimalTilingFeatures;
        if (optimalTilingFeatureFlags & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            dataInternal.supportedDepthFormats.push_back(format);
        }
    }
}

vector<string_view> GetPreferredDeviceExtensions(const BackendExtraVk* backendExtra)
{
    vector<string_view> extensions { DEVICE_EXTENSION_SWAPCHAIN };
    extensions.push_back(DEVICE_EXTENSION_CREATE_RENDERPASS2);
    extensions.push_back(DEVICE_EXTENSION_DEPTH_STENCIL_RESOLVE);
    GetPlatformDeviceExtensions(extensions);
#if (RENDER_VULKAN_RT_ENABLED == 1)
    extensions.push_back(DEVICE_EXTENSION_ACCELERATION_STRUCTURE);
    extensions.push_back(DEVICE_EXTENSION_RAY_TRACING_PIPELINE);
    extensions.push_back(DEVICE_EXTENSION_RAY_QUERY);
    extensions.push_back(DEVICE_EXTENSION_PIPELINE_LIBRARY);
    extensions.push_back(DEVICE_EXTENSION_DEFERRED_HOST_OPERATIONS);
#endif
    if (backendExtra) {
        for (const auto str : backendExtra->extensions.extensionNames) {
            extensions.push_back(str);
        }
    }
    return extensions;
}

DeviceVk::CommonDeviceExtensions GetEnabledCommonDeviceExtensions(
    const unordered_map<string, uint32_t>& enabledDeviceExtensions)
{
    DeviceVk::CommonDeviceExtensions extensions;
    extensions.swapchain = enabledDeviceExtensions.contains(DEVICE_EXTENSION_SWAPCHAIN);
    // renderpass2 required on 1.2, we only use renderpass 2 when we need depth stencil resolve
    extensions.renderPass2 = enabledDeviceExtensions.contains(DEVICE_EXTENSION_DEPTH_STENCIL_RESOLVE) &&
                             enabledDeviceExtensions.contains(DEVICE_EXTENSION_CREATE_RENDERPASS2);
    extensions.externalMemory = enabledDeviceExtensions.contains(DEVICE_EXTENSION_EXTERNAL_MEMORY);
    extensions.getMemoryRequirements2 = enabledDeviceExtensions.contains(DEVICE_EXTENSION_GET_MEMORY_REQUIREMENTS2);
    extensions.queueFamilyForeign = enabledDeviceExtensions.contains(DEVICE_EXTENSION_QUEUE_FAMILY_FOREIGN);
    extensions.samplerYcbcrConversion = enabledDeviceExtensions.contains(DEVICE_EXTENSION_SAMPLER_YCBCR_CONVERSION);

    return extensions;
}

void PreparePhysicalDeviceFeaturesForEnabling(const BackendExtraVk* backendExtra, DevicePlatformDataVk& plat)
{
    // enable all by default and then disable few
    plat.enabledPhysicalDeviceFeatures = plat.physicalDeviceProperties.physicalDeviceFeatures;
    // prepare feature disable for core engine
    plat.enabledPhysicalDeviceFeatures.geometryShader = VK_FALSE;
    plat.enabledPhysicalDeviceFeatures.tessellationShader = VK_FALSE;
    plat.enabledPhysicalDeviceFeatures.sampleRateShading = VK_FALSE;
    plat.enabledPhysicalDeviceFeatures.occlusionQueryPrecise = VK_FALSE;
    plat.enabledPhysicalDeviceFeatures.pipelineStatisticsQuery = VK_FALSE;
    plat.enabledPhysicalDeviceFeatures.shaderTessellationAndGeometryPointSize = VK_FALSE;
    plat.enabledPhysicalDeviceFeatures.inheritedQueries = VK_FALSE;
    if (backendExtra) {
        // check for support and prepare enabling
        if (backendExtra->extensions.physicalDeviceFeaturesToEnable) {
            const size_t valueCount = sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32);
            const array_view<const VkBool32> supported(
                reinterpret_cast<VkBool32*>(&plat.physicalDeviceProperties.physicalDeviceFeatures), valueCount);
            VkPhysicalDeviceFeatures* wantedFeatures =
                (&backendExtra->extensions.physicalDeviceFeaturesToEnable->features);
            const array_view<const VkBool32> wanted(reinterpret_cast<VkBool32*>(wantedFeatures), valueCount);

            array_view<VkBool32> enabledPhysicalDeviceFeatures(
                reinterpret_cast<VkBool32*>(&plat.enabledPhysicalDeviceFeatures), valueCount);
            for (size_t idx = 0; idx < valueCount; ++idx) {
                if (supported[idx] && wanted[idx]) {
                    enabledPhysicalDeviceFeatures[idx] = VK_TRUE;
                } else if (wanted[idx]) {
                    PLUGIN_LOG_W(
                        "physical device feature not supported/enabled from idx: %u", static_cast<uint32_t>(idx));
                }
            }
        }
    }
}

FormatProperties FillDeviceFormatSupport(VkPhysicalDevice physicalDevice, const Format format)
{
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, // physicalDevice
        (VkFormat)format,                               // format
        &formatProperties);                             // pFormatProperties
    return FormatProperties {
        (FormatFeatureFlags)formatProperties.linearTilingFeatures,
        (FormatFeatureFlags)formatProperties.optimalTilingFeatures,
        (FormatFeatureFlags)formatProperties.bufferFeatures,
        GpuProgramUtil::FormatByteSize(format),
    };
}

void FillFormatSupport(VkPhysicalDevice physicalDevice, vector<FormatProperties>& formats)
{
    const uint32_t fullSize = DeviceFormatSupportConstants::LINEAR_FORMAT_MAX_COUNT +
                              DeviceFormatSupportConstants::ADDITIONAL_FORMAT_MAX_COUNT;
    formats.resize(fullSize);
    for (uint32_t idx = 0; idx < DeviceFormatSupportConstants::LINEAR_FORMAT_MAX_COUNT; ++idx) {
        formats[idx] = FillDeviceFormatSupport(physicalDevice, static_cast<Format>(idx));
    }
    // pre-build additional formats
    for (uint32_t idx = 0; idx < DeviceFormatSupportConstants::ADDITIONAL_FORMAT_MAX_COUNT; ++idx) {
        const uint32_t currIdx = idx + DeviceFormatSupportConstants::ADDITIONAL_FORMAT_BASE_IDX;
        PLUGIN_ASSERT(currIdx < static_cast<uint32_t>(formats.size()));
        const uint32_t formatIdx = idx + DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER;
        formats[currIdx] = FillDeviceFormatSupport(physicalDevice, static_cast<Format>(formatIdx));
    }
}
} // namespace

DeviceVk::DeviceVk(RenderContext& renderContext, DeviceCreateInfo const& createInfo) : Device(renderContext, createInfo)
{
    // assume instance and device will be created internally
    ownInstanceAndDevice_ = true;

    const BackendExtraVk* backendExtra = static_cast<const BackendExtraVk*>(createInfo.backendConfiguration);
    // update internal state based the optional backend configuration given by the client. the size of queuProperties
    // will depend on the enableMultiQueue setting.
    const auto queueProperties = CheckExternalConfig(backendExtra);

    // client didn't give the vulkan intance so create own
    if (ownInstanceAndDevice_) {
        CreateInstanceAndPhysicalDevice();
    }
    const auto availableQueues = CreateFunctionsVk::GetAvailableQueues(plat_.physicalDevice, queueProperties);
    if (ownInstanceAndDevice_) {
        CreateDevice(backendExtra, availableQueues);
        CreateDebugFunctions();
    }
    CreateExtFunctions();
    CreatePlatformExtFunctions();
    SortAvailableQueues(availableQueues);

    CheckValidDepthFormats(plat_, platInternal_);
    FillFormatSupport(plat_.physicalDevice, formatProperties_);

    PLUGIN_ASSERT_MSG(!lowLevelGpuQueues_.graphicsQueues.empty(), "default queue not initialized");
    if (!lowLevelGpuQueues_.graphicsQueues.empty()) {
        lowLevelGpuQueues_.defaultQueue = lowLevelGpuQueues_.graphicsQueues[0];
    } else {
        PLUGIN_LOG_E("default vulkan queue not initialized");
    }

    gpuQueueCount_ =
        static_cast<uint32_t>(lowLevelGpuQueues_.computeQueues.size() + lowLevelGpuQueues_.graphicsQueues.size() +
                              lowLevelGpuQueues_.transferQueues.size());

    const PlatformGpuMemoryAllocator::GpuMemoryAllocatorCreateInfo allocatorCreateInfo =
        GetAllocatorCreateInfo(backendExtra);
    platformGpuMemoryAllocator_ = make_unique<PlatformGpuMemoryAllocator>(
        plat_.instance, plat_.physicalDevice, plat_.device, allocatorCreateInfo);

    if (queueProperties.size() > 1) {
        PLUGIN_LOG_I("gpu queue count: %u", gpuQueueCount_);
    }

    SetDeviceStatus(true);

    const GpuResourceManager::CreateInfo grmCreateInfo {
        GpuResourceManager::GPU_RESOURCE_MANAGER_OPTIMIZE_STAGING_MEMORY,
    };
    gpuResourceMgr_ = make_unique<GpuResourceManager>(*this, grmCreateInfo);
    shaderMgr_ = make_unique<ShaderManager>(*this);

    lowLevelDevice_ = make_unique<LowLevelDeviceVk>(*this);
}

DeviceVk::~DeviceVk()
{
    WaitForIdle();

    gpuResourceMgr_.reset();
    shaderMgr_.reset();

    platformGpuMemoryAllocator_.reset();
    swapchain_.reset();

    if (plat_.pipelineCache) {
        CreateFunctionsVk::DestroyPipelineCache(plat_.device, plat_.pipelineCache);
    }

    if (ownInstanceAndDevice_) {
        CreateFunctionsVk::DestroyDevice(plat_.device);
        CreateFunctionsVk::DestroyDebugMessenger(plat_.instance, debugFunctionUtilities_.debugMessenger);
        CreateFunctionsVk::DestroyDebugCallback(plat_.instance, debugFunctionUtilities_.debugCallback);
        CreateFunctionsVk::DestroyInstance(plat_.instance);
    }
}

void DeviceVk::CreateInstanceAndPhysicalDevice()
{
    const VersionInfo engineInfo { "core_prototype", 0, 1, 0 };
    const VersionInfo appInfo { "core_prototype_app", 0, 1, 0 };

    const auto instanceWrapper = CreateFunctionsVk::CreateInstance(engineInfo, appInfo);
    plat_.instance = instanceWrapper.instance;
    if (instanceWrapper.debugUtilsSupported) {
        debugFunctionUtilities_.debugMessenger =
            CreateFunctionsVk::CreateDebugMessenger(plat_.instance, DebugMessengerCallback);
    }
    if (!debugFunctionUtilities_.debugMessenger && instanceWrapper.debugReportSupported) {
        debugFunctionUtilities_.debugCallback =
            CreateFunctionsVk::CreateDebugCallback(plat_.instance, DebugReportCallback);
    }
    auto physicalDeviceWrapper = CreateFunctionsVk::CreatePhysicalDevice(plat_.instance, DEFAULT_QUEUE);
    const uint32_t physicalDeviceApiMajor =
        VK_VERSION_MAJOR(physicalDeviceWrapper.physicalDeviceProperties.physicalDeviceProperties.apiVersion);
    const uint32_t physicalDeviceApiMinor =
        VK_VERSION_MINOR(physicalDeviceWrapper.physicalDeviceProperties.physicalDeviceProperties.apiVersion);
    plat_.deviceApiMajor = std::min(instanceWrapper.apiMajor, physicalDeviceApiMajor);
    plat_.deviceApiMinor = std::min(instanceWrapper.apiMinor, physicalDeviceApiMinor);
    PLUGIN_LOG_D("device api version %u.%u", plat_.deviceApiMajor, plat_.deviceApiMinor);

    plat_.physicalDevice = physicalDeviceWrapper.physicalDevice;
    plat_.physicalDeviceProperties = move(physicalDeviceWrapper.physicalDeviceProperties);
    plat_.physicalDeviceExtensions = move(physicalDeviceWrapper.physicalDeviceExtensions);
    const auto& memoryProperties = plat_.physicalDeviceProperties.physicalDeviceMemoryProperties;
    deviceSharedMemoryPropertyFlags_ =
        (memoryProperties.memoryTypeCount > 0) ? (MemoryPropertyFlags)memoryProperties.memoryTypes[0].propertyFlags : 0;
    for (uint32_t idx = 1; idx < memoryProperties.memoryTypeCount; ++idx) {
        const MemoryPropertyFlags memoryPropertyFlags =
            (MemoryPropertyFlags)memoryProperties.memoryTypes[idx].propertyFlags;
        // do not compare lazily allocated or protected memory blocks
        if ((memoryPropertyFlags & (CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | CORE_MEMORY_PROPERTY_PROTECTED_BIT)) ==
            0) {
            deviceSharedMemoryPropertyFlags_ &= memoryPropertyFlags;
        }
    }
}

void DeviceVk::CreateDevice(const BackendExtraVk* backendExtra, const vector<LowLevelQueueInfo>& availableQueues)
{
    vector<string_view> preferredExtensions = GetPreferredDeviceExtensions(backendExtra);
    PreparePhysicalDeviceFeaturesForEnabling(backendExtra, plat_);

    VkPhysicalDeviceFeatures2* physicalDeviceFeatures2Ptr = nullptr;
    VkPhysicalDeviceSamplerYcbcrConversionFeatures ycbcrConversionFeatures {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SAMPLER_YCBCR_CONVERSION_FEATURES, // sType
        nullptr,                                                             // pNext
        true,                                                                // samplerYcbcrConversion
    };
    VkPhysicalDeviceFeatures2 physicalDeviceFeatures2 {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2, // sType
        &ycbcrConversionFeatures,                     // pNext
        {},                                           // features
    };
    void* pNextForBackendExtra = ycbcrConversionFeatures.pNext;
#if (RENDER_VULKAN_RT_ENABLED == 1)
    VkPhysicalDeviceBufferDeviceAddressFeatures pdBufferDeviceAddressFeatures {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES, // sType
        nullptr,                                                          // pNext
        true,                                                             // bufferDeviceAddress;
        false,                                                            // bufferDeviceAddressCaptureReplay
        false,                                                            // bufferDeviceAddressMultiDevice
    };
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR pdRayTracingPipelineFeatures {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR, // sType
        &pdBufferDeviceAddressFeatures,                                      // pNext
        true,                                                                // rayTracingPipeline;
        false, // rayTracingPipelineShaderGroupHandleCaptureReplay;
        false, // rayTracingPipelineShaderGroupHandleCaptureReplayMixed;
        false, // rayTracingPipelineTraceRaysIndirect;
        false, // rayTraversalPrimitiveCulling;
    };
    VkPhysicalDeviceAccelerationStructureFeaturesKHR pdAccelerationStructureFeatures {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR, // sType
        &pdRayTracingPipelineFeatures,                                         // pNext
        true,                                                                  // accelerationStructure;
        false,                                                                 // accelerationStructureCaptureReplay
        false,                                                                 // accelerationStructureIndirectBuild
        false,                                                                 // accelerationStructureHostCommands
        false, // descriptorBindingAccelerationStructureUpdateAfterBind
    };
    VkPhysicalDeviceRayQueryFeaturesKHR pdRayQueryFeatures {
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR, // sType
        &pdAccelerationStructureFeatures,                         // pNext
        true,                                                     // rayQuery
    };

    // ray tracing to pNext first
    ycbcrConversionFeatures.pNext = &pdRayQueryFeatures;
    // backend extra will be put to pNext of ray tracing extensions
    pNextForBackendExtra = pdBufferDeviceAddressFeatures.pNext;
#endif
    if ((plat_.deviceApiMajor >= 1) && (plat_.deviceApiMinor >= 1)) {
        // pipe user extension physical device features
        if (backendExtra) {
            if (backendExtra->extensions.physicalDeviceFeaturesToEnable) {
                pNextForBackendExtra = backendExtra->extensions.physicalDeviceFeaturesToEnable->pNext;
            }
        }
        // NOTE: on some platforms Vulkan library has only the entrypoints for 1.0. To avoid variation just fetch the
        // function always.
        PFN_vkGetPhysicalDeviceFeatures2 vkGetPhysicalDeviceFeatures2 =
            (PFN_vkGetPhysicalDeviceFeatures2)vkGetInstanceProcAddr(plat_.instance, "vkGetPhysicalDeviceFeatures2");
        vkGetPhysicalDeviceFeatures2(plat_.physicalDevice, &physicalDeviceFeatures2);

        // vkGetPhysicalDeviceFeatures has already filled this and PreparePhysicalDeviceFeaturesForEnabling
        // disabled/ enabled some features.
        physicalDeviceFeatures2.features = plat_.enabledPhysicalDeviceFeatures;
        physicalDeviceFeatures2Ptr = &physicalDeviceFeatures2;
    }
    const DeviceWrapper deviceWrapper =
        CreateFunctionsVk::CreateDevice(plat_.instance, plat_.physicalDevice, plat_.physicalDeviceExtensions,
            plat_.enabledPhysicalDeviceFeatures, physicalDeviceFeatures2Ptr, availableQueues, preferredExtensions);
    plat_.device = deviceWrapper.device;
    for (const auto& ref : deviceWrapper.extensions) {
        extensions_[ref] = 1u;
    }
    commonDeviceExtensions_ = GetEnabledCommonDeviceExtensions(extensions_);
    platformDeviceExtensions_ = GetEnabledPlatformDeviceExtensions(extensions_);
}

vector<QueueProperties> DeviceVk::CheckExternalConfig(const BackendExtraVk* backendConfiguration)
{
    vector<QueueProperties> queueProperties;
    queueProperties.push_back(DEFAULT_QUEUE);

    if (!backendConfiguration) {
        return queueProperties;
    }

    const auto& extra = *backendConfiguration;
    if (extra.enableMultiQueue) {
        queueProperties.push_back(QueueProperties {
            VK_QUEUE_COMPUTE_BIT, // requiredFlags
            1,                    // count
            1.0f,                 // priority
            true,                 // explicitFlags
            false,                // canPresent
        });
        PLUGIN_LOG_I("trying to enable gpu multi-queue, with queue count: %u", (uint32_t)queueProperties.size());
    }

    if (extra.instance != VK_NULL_HANDLE) {
        PLUGIN_LOG_D("trying to use application given vulkan instance, device, and physical device");
        PLUGIN_ASSERT((extra.instance && extra.physicalDevice && extra.device));
        plat_.instance = extra.instance;
        plat_.physicalDevice = extra.physicalDevice;
        plat_.device = extra.device;
        ownInstanceAndDevice_ = false; // everything given from the application

        const auto myDevice = plat_.physicalDevice;
        auto& myProperties = plat_.physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(myDevice, &myProperties.physicalDeviceProperties);
        vkGetPhysicalDeviceFeatures(myDevice, &myProperties.physicalDeviceFeatures);
        vkGetPhysicalDeviceMemoryProperties(myDevice, &myProperties.physicalDeviceMemoryProperties);
    }
    return queueProperties;
}

void DeviceVk::SortAvailableQueues(const vector<LowLevelQueueInfo>& availableQueues)
{
    for (const auto& ref : availableQueues) {
        if (ref.queueFlags == VkQueueFlagBits::VK_QUEUE_COMPUTE_BIT) {
            EmplaceDeviceQueue(plat_.device, ref, lowLevelGpuQueues_.computeQueues);
        } else if (ref.queueFlags == VkQueueFlagBits::VK_QUEUE_GRAPHICS_BIT) {
            EmplaceDeviceQueue(plat_.device, ref, lowLevelGpuQueues_.graphicsQueues);
        } else if (ref.queueFlags == VkQueueFlagBits::VK_QUEUE_TRANSFER_BIT) {
            EmplaceDeviceQueue(plat_.device, ref, lowLevelGpuQueues_.transferQueues);
        }
    }
}

DeviceBackendType DeviceVk::GetBackendType() const
{
    return DeviceBackendType::VULKAN;
}

const DevicePlatformData& DeviceVk::GetPlatformData() const
{
    return plat_;
}

const DevicePlatformDataVk& DeviceVk::GetPlatformDataVk() const
{
    return plat_;
}

const DevicePlatformInternalDataVk& DeviceVk::GetPlatformInternalDataVk() const
{
    return platInternal_;
}

ILowLevelDevice& DeviceVk::GetLowLevelDevice() const
{
    return *lowLevelDevice_;
}

FormatProperties DeviceVk::GetFormatProperties(const Format format) const
{
    const uint32_t formatSupportSize = static_cast<uint32_t>(formatProperties_.size());
    const uint32_t formatIdx = static_cast<uint32_t>(format);
    if (formatIdx < formatSupportSize) {
        return formatProperties_[formatIdx];
    } else if ((formatIdx >= DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER) &&
               (formatIdx <= DeviceFormatSupportConstants::ADDITIONAL_FORMAT_END_NUMBER)) {
        const uint32_t currIdx = formatIdx - DeviceFormatSupportConstants::ADDITIONAL_FORMAT_START_NUMBER;
        PLUGIN_UNUSED(currIdx);
        PLUGIN_ASSERT(currIdx < formatSupportSize);
        return formatProperties_[formatIdx];
    }
    return {};
}

AccelerationStructureBuildSizes DeviceVk::GetAccelerationStructureBuildSizes(
    const AccelerationStructureBuildGeometryInfo& geometry,
    BASE_NS::array_view<const AccelerationStructureGeometryTrianglesInfo> triangles,
    BASE_NS::array_view<const AccelerationStructureGeometryAabbsInfo> aabbs,
    BASE_NS::array_view<const AccelerationStructureGeometryInstancesInfo> instances) const
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    const VkDevice device = plat_.device;

    const size_t arraySize = triangles.size() + aabbs.size() + instances.size();
    vector<VkAccelerationStructureGeometryKHR> geometryData(arraySize);
    vector<uint32_t> maxPrimitiveCounts(arraySize);
    uint32_t arrayIndex = 0;
    for (const auto& trianglesRef : triangles) {
        geometryData[arrayIndex] = VkAccelerationStructureGeometryKHR {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, // sType
            nullptr,                                               // pNext
            VkGeometryTypeKHR::VK_GEOMETRY_TYPE_TRIANGLES_KHR,     // geometryType
            {},                                                    // geometry;
            VkGeometryFlagsKHR(trianglesRef.geometryFlags),        // flags
        };
        geometryData[arrayIndex].geometry.triangles = VkAccelerationStructureGeometryTrianglesDataKHR {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR, // sType
            nullptr,                                                              // pNext
            VkFormat(trianglesRef.vertexFormat),                                  // vertexFormat
            {},                                                                   // vertexData
            VkDeviceSize(trianglesRef.vertexStride),                              // vertexStride
            trianglesRef.maxVertex,                                               // maxVertex
            VkIndexType(trianglesRef.indexType),                                  // indexType
            {},                                                                   // indexData
            {},                                                                   // transformData
        };
        maxPrimitiveCounts[arrayIndex] = trianglesRef.indexCount / 3u; // triangles;
        arrayIndex++;
    }
    for (const auto& aabbsRef : aabbs) {
        geometryData[arrayIndex] = VkAccelerationStructureGeometryKHR {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, // sType
            nullptr,                                               // pNext
            VkGeometryTypeKHR::VK_GEOMETRY_TYPE_AABBS_KHR,         // geometryType
            {},                                                    // geometry;
            0,                                                     // flags
        };
        geometryData[arrayIndex].geometry.aabbs = VkAccelerationStructureGeometryAabbsDataKHR {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR, // sType
            nullptr,                                                          // pNext
            {},                                                               // data
            aabbsRef.stride,                                                  // stride
        };
        maxPrimitiveCounts[arrayIndex] = 1u;
        arrayIndex++;
    }
    for (const auto& instancesRef : instances) {
        geometryData[arrayIndex] = VkAccelerationStructureGeometryKHR {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR, // sType
            nullptr,                                               // pNext
            VkGeometryTypeKHR::VK_GEOMETRY_TYPE_INSTANCES_KHR,     // geometryType
            {},                                                    // geometry;
            0,                                                     // flags
        };
        geometryData[arrayIndex].geometry.instances = VkAccelerationStructureGeometryInstancesDataKHR {
            VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR, // sType
            nullptr,                                                              // pNext
            instancesRef.arrayOfPointers,                                         // arrayOfPointers
            {},                                                                   // data
        };
        maxPrimitiveCounts[arrayIndex] = 1u;
        arrayIndex++;
    }

    const VkAccelerationStructureBuildGeometryInfoKHR geometryInfoVk {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR, // sType
        nullptr,                                                          // pNext
        VkAccelerationStructureTypeKHR(geometry.type),                    // type
        VkBuildAccelerationStructureFlagsKHR(geometry.flags),             // flags
        VkBuildAccelerationStructureModeKHR(geometry.mode),               // mode
        VK_NULL_HANDLE,                                                   // srcAccelerationStructure
        VK_NULL_HANDLE,                                                   // dstAccelerationStructure
        arrayIndex,                                                       // geometryCount
        geometryData.data(),                                              // pGeometries
        nullptr,                                                          // ppGeometries
        {},                                                               // scratchData
    };

    VkAccelerationStructureBuildSizesInfoKHR buildSizesInfo {
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR, // sType
        nullptr,                                                       // pNext
        0,                                                             // accelerationStructureSize
        0,                                                             // updateScratchSize
        0,                                                             // buildScratchSize
    };
    if ((arrayIndex > 0) && extFunctions_.vkGetAccelerationStructureBuildSizesKHR) {
        extFunctions_.vkGetAccelerationStructureBuildSizesKHR(device, // device
            VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,          // buildType,
            &geometryInfoVk,                                          // pBuildInfo
            maxPrimitiveCounts.data(),                                // pMaxPrimitiveCounts
            &buildSizesInfo);                                         // pSizeInfo
    }

    return AccelerationStructureBuildSizes {
        static_cast<uint32_t>(buildSizesInfo.accelerationStructureSize),
        static_cast<uint32_t>(buildSizesInfo.updateScratchSize),
        static_cast<uint32_t>(buildSizesInfo.buildScratchSize),
    };
#else
    return AccelerationStructureBuildSizes { 0, 0, 0 };
#endif
}

void DeviceVk::CreateDeviceSwapchain(const SwapchainCreateInfo& swapchainCreateInfo)
{
    WaitForIdle();
    swapchain_.reset();
    swapchain_ = make_unique<SwapchainVk>(*this, swapchainCreateInfo);
}

void DeviceVk::DestroyDeviceSwapchain()
{
    WaitForIdle();
    swapchain_.reset();
}

PlatformGpuMemoryAllocator* DeviceVk::GetPlatformGpuMemoryAllocator()
{
    return platformGpuMemoryAllocator_.get();
}

GpuQueue DeviceVk::GetValidGpuQueue(const GpuQueue& gpuQueue) const
{
    const auto getSpecificQueue = [](const uint32_t queueIndex, const GpuQueue::QueueType queueType,
                                      const vector<LowLevelGpuQueueVk>& specificQueues, const GpuQueue& defaultQueue) {
        const uint32_t queueCount = (uint32_t)specificQueues.size();
        if (queueIndex < queueCount) {
            return GpuQueue { queueType, queueIndex };
        } else if (queueCount > 0) {
            return GpuQueue { queueType, 0 };
        }
        return defaultQueue;
    };

    GpuQueue defaultQueue { GpuQueue::QueueType::GRAPHICS, 0 };
    if (gpuQueue.type == GpuQueue::QueueType::COMPUTE) {
        return getSpecificQueue(
            gpuQueue.index, GpuQueue::QueueType::COMPUTE, lowLevelGpuQueues_.computeQueues, defaultQueue);
    } else if (gpuQueue.type == GpuQueue::QueueType::GRAPHICS) {
        return getSpecificQueue(
            gpuQueue.index, GpuQueue::QueueType::GRAPHICS, lowLevelGpuQueues_.graphicsQueues, defaultQueue);
    } else if (gpuQueue.type == GpuQueue::QueueType::TRANSFER) {
        return getSpecificQueue(
            gpuQueue.index, GpuQueue::QueueType::TRANSFER, lowLevelGpuQueues_.transferQueues, defaultQueue);
    } else {
        return defaultQueue;
    }
}

uint32_t DeviceVk::GetGpuQueueCount() const
{
    return gpuQueueCount_;
}

void DeviceVk::InitializePipelineCache(array_view<const uint8_t> initialData)
{
    if (plat_.pipelineCache) {
        CreateFunctionsVk::DestroyPipelineCache(plat_.device, plat_.pipelineCache);
    }
    struct CacheHeader {
        uint32_t bytes;
        uint32_t version;
        uint32_t vendorId;
        uint32_t deviceId;
        uint8_t pipelineCacheUUID[VK_UUID_SIZE];
    };
    if (initialData.size() > sizeof(CacheHeader)) {
        CacheHeader header;
        CloneData(&header, sizeof(header), initialData.data(), sizeof(header));
        const auto& props = plat_.physicalDeviceProperties.physicalDeviceProperties;
        if (header.version != VkPipelineCacheHeaderVersion::VK_PIPELINE_CACHE_HEADER_VERSION_ONE ||
            header.vendorId != props.vendorID || header.deviceId != props.deviceID ||
            memcmp(header.pipelineCacheUUID, props.pipelineCacheUUID, VK_UUID_SIZE)) {
            initialData = {};
        }
    }

    plat_.pipelineCache = CreateFunctionsVk::CreatePipelineCache(plat_.device, initialData);
}

vector<uint8_t> DeviceVk::GetPipelineCache() const
{
    vector<uint8_t> deviceData;
    if (plat_.pipelineCache) {
        size_t dataSize = 0u;
        if (auto result = vkGetPipelineCacheData(plat_.device, plat_.pipelineCache, &dataSize, nullptr);
            result == VK_SUCCESS && dataSize) {
            deviceData.resize(dataSize);
            dataSize = deviceData.size();
            result = vkGetPipelineCacheData(plat_.device, plat_.pipelineCache, &dataSize, deviceData.data());
            if (result == VK_SUCCESS) {
                deviceData.resize(dataSize);
            } else {
                deviceData.clear();
            }
        }
    }
    return deviceData;
}

LowLevelGpuQueueVk DeviceVk::GetGpuQueue(const GpuQueue& gpuQueue) const
{
    // 1. tries to return the typed queue with given index
    // 2. tries to return the typed queue with an index 0
    // 3. returns the default queue
    const auto getSpecificQueue = [](const uint32_t queueIndex, const vector<LowLevelGpuQueueVk>& specificQueues,
                                      const LowLevelGpuQueueVk& defaultQueue) {
        const uint32_t queueCount = (uint32_t)specificQueues.size();
        if (queueIndex < queueCount) {
            return specificQueues[queueIndex];
        } else if (queueCount > 0) {
            return specificQueues[0];
        }
        return defaultQueue;
    };

    if (gpuQueue.type == GpuQueue::QueueType::COMPUTE) {
        return getSpecificQueue(gpuQueue.index, lowLevelGpuQueues_.computeQueues, lowLevelGpuQueues_.defaultQueue);
    } else if (gpuQueue.type == GpuQueue::QueueType::GRAPHICS) {
        return getSpecificQueue(gpuQueue.index, lowLevelGpuQueues_.graphicsQueues, lowLevelGpuQueues_.defaultQueue);
    } else if (gpuQueue.type == GpuQueue::QueueType::TRANSFER) {
        return getSpecificQueue(gpuQueue.index, lowLevelGpuQueues_.transferQueues, lowLevelGpuQueues_.defaultQueue);
    } else {
        return lowLevelGpuQueues_.defaultQueue;
    }
}

LowLevelGpuQueueVk DeviceVk::GetPresentationGpuQueue() const
{
    // NOTE: expected presentation
    return GetGpuQueue(GpuQueue { GpuQueue::QueueType::GRAPHICS, 0 });
}

vector<LowLevelGpuQueueVk> DeviceVk::GetLowLevelGpuQueues() const
{
    vector<LowLevelGpuQueueVk> gpuQueues;
    gpuQueues.reserve(gpuQueueCount_);
    gpuQueues.insert(gpuQueues.end(), lowLevelGpuQueues_.computeQueues.begin(), lowLevelGpuQueues_.computeQueues.end());
    gpuQueues.insert(
        gpuQueues.end(), lowLevelGpuQueues_.graphicsQueues.begin(), lowLevelGpuQueues_.graphicsQueues.end());
    gpuQueues.insert(
        gpuQueues.end(), lowLevelGpuQueues_.transferQueues.begin(), lowLevelGpuQueues_.transferQueues.end());
    return gpuQueues;
}

void DeviceVk::WaitForIdle()
{
    if (plat_.device) {
        if (!isRenderbackendRunning_) {
            PLUGIN_LOG_D("Device - WaitForIdle");
            vkDeviceWaitIdle(plat_.device); // device
        } else {
            PLUGIN_LOG_E("Device WaitForIdle can only called when render backend is not running");
        }
    }
}

void DeviceVk::Activate() {}

void DeviceVk::Deactivate() {}

bool DeviceVk::AllowThreadedProcessing() const
{
    return true;
}

const DeviceVk::FeatureConfigurations& DeviceVk::GetFeatureConfigurations() const
{
    return featureConfigurations_;
}

const DeviceVk::CommonDeviceExtensions& DeviceVk::GetCommonDeviceExtensions() const
{
    return commonDeviceExtensions_;
}

const PlatformDeviceExtensions& DeviceVk::GetPlatformDeviceExtensions() const
{
    return platformDeviceExtensions_;
}

bool DeviceVk::HasDeviceExtension(const string_view extensionName) const
{
    return extensions_.contains(extensionName);
}

unique_ptr<Device> CreateDeviceVk(RenderContext& renderContext, DeviceCreateInfo const& createInfo)
{
    return make_unique<DeviceVk>(renderContext, createInfo);
}

unique_ptr<GpuBuffer> DeviceVk::CreateGpuBuffer(const GpuBufferDesc& desc)
{
    return make_unique<GpuBufferVk>(*this, desc);
}

unique_ptr<GpuImage> DeviceVk::CreateGpuImage(const GpuImageDesc& desc)
{
    return make_unique<GpuImageVk>(*this, desc);
}

unique_ptr<GpuImage> DeviceVk::CreateGpuImageView(
    const GpuImageDesc& desc, const GpuImagePlatformData& platformData, const uintptr_t hwBuffer)
{
    return make_unique<GpuImageVk>(*this, desc, platformData, hwBuffer);
}

unique_ptr<GpuImage> DeviceVk::CreateGpuImageView(const GpuImageDesc& desc, const GpuImagePlatformData& platformData)
{
    return CreateGpuImageView(desc, platformData, 0);
}

vector<unique_ptr<GpuImage>> DeviceVk::CreateGpuImageViews(const Swapchain& swapchain)
{
    const GpuImageDesc& desc = swapchain.GetDesc();
    const auto& swapchainPlat = static_cast<const SwapchainVk&>(swapchain).GetPlatformData();

    vector<unique_ptr<GpuImage>> gpuImages(swapchainPlat.swapchainImages.images.size());
    for (size_t idx = 0; idx < gpuImages.size(); ++idx) {
        GpuImagePlatformDataVk gpuImagePlat;
        gpuImagePlat.image = swapchainPlat.swapchainImages.images[idx];
        gpuImagePlat.imageView = swapchainPlat.swapchainImages.imageViews[idx];
        gpuImages[idx] = this->CreateGpuImageView(desc, gpuImagePlat);
    }
    return gpuImages;
}

unique_ptr<GpuImage> DeviceVk::CreateGpuImageView(
    const GpuImageDesc& desc, const BackendSpecificImageDesc& platformData)
{
    const ImageDescVk& imageDesc = (const ImageDescVk&)platformData;
    GpuImagePlatformDataVk platData;
    platData.image = imageDesc.image;
    platData.imageView = imageDesc.imageView;
    return CreateGpuImageView(desc, platData, imageDesc.platformHwBuffer);
}

unique_ptr<GpuSampler> DeviceVk::CreateGpuSampler(const GpuSamplerDesc& desc)
{
    return make_unique<GpuSamplerVk>(*this, desc);
}

unique_ptr<GpuAccelerationStructure> DeviceVk::CreateGpuAccelerationStructure(const GpuAccelerationStructureDesc& desc)
{
    return make_unique<GpuAccelerationStructureVk>(*this, desc);
}

unique_ptr<RenderFrameSync> DeviceVk::CreateRenderFrameSync()
{
    return make_unique<RenderFrameSyncVk>(*this);
}

unique_ptr<RenderBackend> DeviceVk::CreateRenderBackend(
    GpuResourceManager& gpuResourceMgr, const CORE_NS::IParallelTaskQueue::Ptr& queue)
{
    return make_unique<RenderBackendVk>(*this, gpuResourceMgr, queue);
}

unique_ptr<ShaderModule> DeviceVk::CreateShaderModule(const ShaderModuleCreateInfo& data)
{
    return make_unique<ShaderModuleVk>(*this, data);
}

unique_ptr<ShaderModule> DeviceVk::CreateComputeShaderModule(const ShaderModuleCreateInfo& data)
{
    return make_unique<ShaderModuleVk>(*this, data);
}

unique_ptr<GpuShaderProgram> DeviceVk::CreateGpuShaderProgram(const GpuShaderProgramCreateData& data)
{
    return make_unique<GpuShaderProgramVk>(*this, data);
}

unique_ptr<GpuComputeProgram> DeviceVk::CreateGpuComputeProgram(const GpuComputeProgramCreateData& data)
{
    return make_unique<GpuComputeProgramVk>(*this, data);
}

unique_ptr<NodeContextDescriptorSetManager> DeviceVk::CreateNodeContextDescriptorSetManager()
{
    return make_unique<NodeContextDescriptorSetManagerVk>(*this);
}

unique_ptr<NodeContextPoolManager> DeviceVk::CreateNodeContextPoolManager(
    GpuResourceManager& gpuResourceMgr, const GpuQueue& gpuQueue)
{
    return make_unique<NodeContextPoolManagerVk>(*this, gpuResourceMgr, gpuQueue);
}

unique_ptr<GraphicsPipelineStateObject> DeviceVk::CreateGraphicsPipelineStateObject(const GpuShaderProgram& gpuProgram,
    const GraphicsState& graphicsState, const PipelineLayout& pipelineLayout,
    const VertexInputDeclarationView& vertexInputDeclaration,
    const ShaderSpecializationConstantDataView& specializationConstants, const DynamicStateFlags dynamicStateFlags,
    const RenderPassDesc& renderPassDesc, const array_view<const RenderPassSubpassDesc>& renderPassSubpassDescs,
    const uint32_t subpassIndex, const LowLevelRenderPassData* renderPassData,
    const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    PLUGIN_ASSERT(renderPassData);
    PLUGIN_ASSERT(pipelineLayoutData);
    return make_unique<GraphicsPipelineStateObjectVk>(*this, gpuProgram, graphicsState, pipelineLayout,
        vertexInputDeclaration, specializationConstants, dynamicStateFlags, renderPassDesc, renderPassSubpassDescs,
        subpassIndex, *renderPassData, *pipelineLayoutData);
}

unique_ptr<ComputePipelineStateObject> DeviceVk::CreateComputePipelineStateObject(const GpuComputeProgram& gpuProgram,
    const PipelineLayout& pipelineLayout, const ShaderSpecializationConstantDataView& specializationConstants,
    const LowLevelPipelineLayoutData* pipelineLayoutData)
{
    PLUGIN_ASSERT(pipelineLayoutData);
    return make_unique<ComputePipelineStateObjectVk>(
        *this, gpuProgram, pipelineLayout, specializationConstants, *pipelineLayoutData);
}

const DebugFunctionUtilitiesVk& DeviceVk::GetDebugFunctionUtilities() const
{
    return debugFunctionUtilities_;
}

void DeviceVk::CreateDebugFunctions()
{
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    debugFunctionUtilities_.vkSetDebugUtilsObjectNameEXT =
        (PFN_vkSetDebugUtilsObjectNameEXT)vkGetDeviceProcAddr(plat_.device, "vkSetDebugUtilsObjectNameEXT");
#endif
#if (RENDER_DEBUG_MARKERS_ENABLED == 1) || (RENDER_DEBUG_COMMAND_MARKERS_ENABLED == 1)
    debugFunctionUtilities_.vkCmdBeginDebugUtilsLabelEXT =
        (PFN_vkCmdBeginDebugUtilsLabelEXT)vkGetInstanceProcAddr(plat_.instance, "vkCmdBeginDebugUtilsLabelEXT");
    debugFunctionUtilities_.vkCmdEndDebugUtilsLabelEXT =
        (PFN_vkCmdEndDebugUtilsLabelEXT)vkGetInstanceProcAddr(plat_.instance, "vkCmdEndDebugUtilsLabelEXT");
#endif
}

const DeviceVk::ExtFunctions& DeviceVk::GetExtFunctions() const
{
    return extFunctions_;
}

const PlatformExtFunctions& DeviceVk::GetPlatformExtFunctions() const
{
    return platformExtFunctions_;
}

void DeviceVk::CreateExtFunctions()
{
    if (commonDeviceExtensions_.renderPass2) {
        extFunctions_.vkCreateRenderPass2KHR =
            (PFN_vkCreateRenderPass2KHR)vkGetInstanceProcAddr(plat_.instance, "vkCreateRenderPass2KHR");
        if (!extFunctions_.vkCreateRenderPass2KHR) {
            commonDeviceExtensions_.renderPass2 = false;
            PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkCreateRenderPass2KHR");
        }
    }

    if (commonDeviceExtensions_.getMemoryRequirements2) {
        extFunctions_.vkGetImageMemoryRequirements2 = (PFN_vkGetImageMemoryRequirements2)vkGetInstanceProcAddr(
            plat_.instance, "vkGetImageMemoryRequirements2KHR");
        if (!extFunctions_.vkGetImageMemoryRequirements2) {
            PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkGetImageMemoryRequirements2");
        }
    }

    if (commonDeviceExtensions_.samplerYcbcrConversion) {
        GetYcbcrExtFunctions(plat_.instance, extFunctions_);
    }

    extFunctions_.vkAcquireNextImageKHR =
        (PFN_vkAcquireNextImageKHR)vkGetInstanceProcAddr(plat_.instance, "vkAcquireNextImageKHR");

#if (RENDER_VULKAN_RT_ENABLED == 1)
    extFunctions_.vkGetAccelerationStructureBuildSizesKHR =
        (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetInstanceProcAddr(
            plat_.instance, "vkGetAccelerationStructureBuildSizesKHR");
    if (!extFunctions_.vkGetAccelerationStructureBuildSizesKHR) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkGetAccelerationStructureBuildSizesKHR");
    }
    extFunctions_.vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetInstanceProcAddr(
        plat_.instance, "vkCmdBuildAccelerationStructuresKHR");
    if (!extFunctions_.vkCmdBuildAccelerationStructuresKHR) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkCmdBuildAccelerationStructuresKHR");
    }
    extFunctions_.vkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetInstanceProcAddr(plat_.instance, "vkCreateAccelerationStructureKHR");
    if (!extFunctions_.vkCreateAccelerationStructureKHR) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkCreateAccelerationStructureKHR");
    }
    extFunctions_.vkDestroyAccelerationStructureKHR = (PFN_vkDestroyAccelerationStructureKHR)vkGetInstanceProcAddr(
        plat_.instance, "vkDestroyAccelerationStructureKHR");
    if (!extFunctions_.vkDestroyAccelerationStructureKHR) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkDestroyAccelerationStructureKHR");
    }
    extFunctions_.vkGetAccelerationStructureDeviceAddressKHR =
        (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetInstanceProcAddr(
            plat_.instance, "vkGetAccelerationStructureDeviceAddressKHR");
    if (!extFunctions_.vkGetAccelerationStructureDeviceAddressKHR) {
        PLUGIN_LOG_E("vkGetInstanceProcAddr failed for vkGetAccelerationStructureDeviceAddressKHR");
    }
#endif
}

LowLevelDeviceVk::LowLevelDeviceVk(DeviceVk& deviceVk)
    : deviceVk_(deviceVk), gpuResourceMgr_(static_cast<GpuResourceManager&>(deviceVk_.GetGpuResourceManager()))
{}

DeviceBackendType LowLevelDeviceVk::GetBackendType() const
{
    return DeviceBackendType::VULKAN;
}

const DevicePlatformDataVk& LowLevelDeviceVk::GetPlatformDataVk() const
{
    return deviceVk_.GetPlatformDataVk();
}

GpuBufferPlatformDataVk LowLevelDeviceVk::GetBuffer(RenderHandle handle) const
{
    if (deviceVk_.GetLockResourceBackendAccess()) {
        GpuBufferVk* buffer = gpuResourceMgr_.GetBuffer<GpuBufferVk>(handle);
        if (buffer) {
            return buffer->GetPlatformData();
        }
    } else {
        PLUGIN_LOG_E("low level device methods can only be used within specific methods");
    }
    return {};
}

GpuImagePlatformDataVk LowLevelDeviceVk::GetImage(RenderHandle handle) const
{
    if (deviceVk_.GetLockResourceBackendAccess()) {
        GpuImageVk* image = gpuResourceMgr_.GetImage<GpuImageVk>(handle);
        if (image) {
            return image->GetPlatformData();
        }
    } else {
        PLUGIN_LOG_E("low level device methods can only be used within specific methods");
    }
    return {};
}

GpuSamplerPlatformDataVk LowLevelDeviceVk::GetSampler(RenderHandle handle) const
{
    if (deviceVk_.GetLockResourceBackendAccess()) {
        GpuSamplerVk* sampler = gpuResourceMgr_.GetSampler<GpuSamplerVk>(handle);
        if (sampler) {
            return sampler->GetPlatformData();
        }
    } else {
        PLUGIN_LOG_E("low level device methods can be only used within specific methods");
    }
    return {};
}
RENDER_END_NAMESPACE()
