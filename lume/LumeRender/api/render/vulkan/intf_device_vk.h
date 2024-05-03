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

#ifndef API_RENDER_VULKAN_IDEVICE_VK_H
#define API_RENDER_VULKAN_IDEVICE_VK_H

#include <cstdint>

#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#if RENDER_HAS_VULKAN_BACKEND
// if intf_device_vk.h is included by the app or plugin, vulkan needs to be located
#include <vulkan/vulkan_core.h>
#endif

// Platform / Backend specific typedefs.
RENDER_BEGIN_NAMESPACE()
/** \addtogroup group_gfx_vulkan_idevicevk
 *  @{
 */
#if RENDER_HAS_VULKAN_BACKEND || DOXYGEN
/** Backend extra vulkan */
struct BackendExtraVk final : public BackendExtra {
    /* Enable multiple gpu queues for usage */
    bool enableMultiQueue { false };

    VkInstance instance { VK_NULL_HANDLE };
    VkPhysicalDevice physicalDevice { VK_NULL_HANDLE };
    VkDevice device { VK_NULL_HANDLE };

    struct DeviceExtensions {
        BASE_NS::vector<BASE_NS::string_view> extensionNames;
        VkPhysicalDeviceFeatures2* physicalDeviceFeaturesToEnable { nullptr };
    };
    /* Additional extensions */
    DeviceExtensions extensions;

    struct GpuMemoryAllocatorSizes {
        /* Set default allocation block size in bytes, used if the value is not ~0u */
        uint32_t defaultAllocationBlockSize { ~0u };
        /* Set custom dynamic (ring buffer) UBO allocation block size in bytes , used if the value is not ~0u */
        uint32_t customAllocationDynamicUboBlockSize { ~0u };
    };
    /* Memory sizes might not be used if the sizes are not valid/sane */
    GpuMemoryAllocatorSizes gpuMemoryAllocatorSizes;
};

/** Physical device properties vulkan */
struct PhysicalDevicePropertiesVk final {
    /** Physical device properties */
    VkPhysicalDeviceProperties physicalDeviceProperties;
    /** Physical device features */
    VkPhysicalDeviceFeatures physicalDeviceFeatures;
    /** Physical device memory properties */
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
};

/** Device platform data vulkan */
struct DevicePlatformDataVk final : DevicePlatformData {
    /** Instance */
    VkInstance instance { VK_NULL_HANDLE };
    /** Physical device */
    VkPhysicalDevice physicalDevice { VK_NULL_HANDLE };
    /** Device */
    VkDevice device { VK_NULL_HANDLE };
    /** Physical device properties */
    PhysicalDevicePropertiesVk physicalDeviceProperties {};
    /** Available physical device extensions */
    BASE_NS::vector<VkExtensionProperties> physicalDeviceExtensions {};

    /** Enabled physical device features */
    VkPhysicalDeviceFeatures enabledPhysicalDeviceFeatures {};

    uint32_t deviceApiMajor { 0u };
    uint32_t deviceApiMinor { 0u };

    VkPipelineCache pipelineCache { VK_NULL_HANDLE };
};

struct RenderBackendRecordingStateVk final : public RenderBackendRecordingState {
    VkCommandBuffer commandBuffer { VK_NULL_HANDLE };

    VkRenderPass renderPass { VK_NULL_HANDLE };
    VkFramebuffer framebuffer { VK_NULL_HANDLE };
    VkExtent2D framebufferSize { 0, 0 };
    uint32_t subpassIndex { 0u };

    VkPipelineLayout pipelineLayout { VK_NULL_HANDLE };
};

/** Image desc vulkan for creating engine GpuImage based on given data */
struct ImageDescVk final : BackendSpecificImageDesc {
    /** Image */
    VkImage image { VK_NULL_HANDLE };
    /** Image view */
    VkImageView imageView { VK_NULL_HANDLE };

    /** Platform specific hardware buffer */
    uintptr_t platformHwBuffer { 0u };
};

/** Buffer descriptor vulkan for creating engine GpuBuffer based on given data */
struct BufferDescVk : BackendSpecificBufferDesc {
    /** Buffer */
    VkBuffer buffer { VK_NULL_HANDLE };
};

/** Low level vk memory access. (Usable only with low level engine use-cases) */
struct GpuResourceMemoryVk final {
    /* Vulkan memory handle */
    VkDeviceMemory deviceMemory { VK_NULL_HANDLE };
    /* Offset into deviceMemory object to the beginning of this allocation */
    VkDeviceSize offset { 0 };
    /* Size of this allocation */
    VkDeviceSize size { 0 };
    /* Null if not mappable */
    void* mappedData { nullptr };

    /* Memory type */
    uint32_t memoryTypeIndex { 0 };
    /* Memory flags */
    VkMemoryPropertyFlags memoryPropertyFlags { 0 };
};

/** Low level vk buffer access. (Usable only with low level engine use-cases) */
struct GpuBufferPlatformDataVk final : public GpuBufferPlatformData {
    /* Buffer handle */
    VkBuffer buffer { VK_NULL_HANDLE };

    /* Bindable memory block byte size */
    uint32_t bindMemoryByteSize { 0u };
    /* Full byte size of this buffer, i.e. might be 3 x bindMemoryByteSize for dynamic ring buffers.
     * If no buffering fullByteSize == bindMemoryByteSize.
     */
    uint32_t fullByteSize { 0u };
    /* Current offset with ring buffers (advanced with map), otherwise 0 */
    uint32_t currentByteOffset { 0u };

    /* Usage flags */
    VkBufferUsageFlags usage { 0 };

    /* Memory */
    GpuResourceMemoryVk memory;
};

/** Low level vk image access. (Usable only with low level engine use-cases) */
struct GpuImagePlatformDataVk final : public GpuImagePlatformData {
    /* Image handle */
    VkImage image { VK_NULL_HANDLE };
    /* Image view */
    VkImageView imageView { VK_NULL_HANDLE };
    /* Image view base for mip level 0 and layer 0 for attachments */
    VkImageView imageViewBase { VK_NULL_HANDLE };

    /* Format */
    VkFormat format { VK_FORMAT_UNDEFINED };
    /* Extent */
    VkExtent3D extent { 0u, 0u, 0u };
    /* Image type */
    VkImageType type { VK_IMAGE_TYPE_2D };
    /* Aspect flags */
    VkImageAspectFlags aspectFlags { 0 };
    /* Usage flags */
    VkImageUsageFlags usage { 0 };
    /* Sample count flag bits */
    VkSampleCountFlagBits samples { VK_SAMPLE_COUNT_1_BIT };
    /* Image tiling */
    VkImageTiling tiling { VK_IMAGE_TILING_OPTIMAL };
    /* Mip levels */
    uint32_t mipLevels { 0u };
    /* Layer count */
    uint32_t arrayLayers { 0u };

    /* Memory */
    GpuResourceMemoryVk memory;
};

/** Low level vk sampler access. (Usable only with low level engine use-cases) */
struct GpuSamplerPlatformDataVk final : public GpuSamplerPlatformData {
    /* Sampler handle */
    VkSampler sampler { VK_NULL_HANDLE };
};

/** Provides interface for low-level access.
 * Resource access only valid with specific methods in IRenderBackendNode and IRenderDataStore.
 */
class ILowLevelDeviceVk : public ILowLevelDevice {
public:
    virtual const DevicePlatformDataVk& GetPlatformDataVk() const = 0;

    /** Get vulkan buffer. Valid access only during rendering with node and data store methods. */
    virtual GpuBufferPlatformDataVk GetBuffer(RenderHandle handle) const = 0;
    /** Get vulkan image. Valid access only during rendering with node and data store methods. */
    virtual GpuImagePlatformDataVk GetImage(RenderHandle handle) const = 0;
    /** Get vulkan sampler. Valid access only during rendering with node and data store methods. */
    virtual GpuSamplerPlatformDataVk GetSampler(RenderHandle handle) const = 0;

protected:
    ILowLevelDeviceVk() = default;
    ~ILowLevelDeviceVk() = default;
};

#endif // RENDER_HAS_VULKAN_BACKEND

/** Helper for converting between engine and Vulkan handles.
 * On 32 bit platforms Vulkan handles are uint64_t, but on 64 bit platforms they are pointers. For the engine handles
 * are always stored as uint64_t regardless of the platform. This helper selects the correct cast for the conversion.
 * @param handle Handle to convert.
 * @return Handle cast to the desired type.
 */
template<typename OutHandle, typename InHandle>
inline OutHandle VulkanHandleCast(InHandle handle)
{
    // based on current use-cases we could assert that is_pointer_v<OutHandle> != is_pointer_v<InHandle> and not cover
    // the last two cases.
    if constexpr (BASE_NS::is_same_v<OutHandle, InHandle>) {
        // engine<->vulkan, on 32 bit platforms
        return handle;
    } else if constexpr (BASE_NS::is_pointer_v<OutHandle> && !BASE_NS::is_pointer_v<InHandle>) {
        // engine -> vulkan, on 64 bit platforms
        return reinterpret_cast<OutHandle>(static_cast<uintptr_t>(handle));
    } else if constexpr (!BASE_NS::is_pointer_v<OutHandle> && BASE_NS::is_pointer_v<InHandle>) {
        // engine <- vulkan, on 64 bit platforms
        return reinterpret_cast<OutHandle>(handle);
    } else if constexpr (BASE_NS::is_pointer_v<OutHandle> && BASE_NS::is_pointer_v<InHandle>) {
        return reinterpret_cast<OutHandle>(handle);
    } else {
        return static_cast<OutHandle>(handle);
    }
}
/** @} */
RENDER_END_NAMESPACE()

#endif // API_RENDER_VULKAN_IDEVICE_VK_H
