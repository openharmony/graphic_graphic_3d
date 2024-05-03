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

#include "gpu_memory_allocator_vk.h"

#include <cinttypes>
#include <vulkan/vulkan_core.h>

// Including the external library header <vk_mem_alloc.h> causes a warning. Disabling it.
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4701)
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wswitch"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-qualifiers"
#pragma GCC diagnostic ignored "-Wswitch"
#endif
// vma_mem_alloc.h will use VMA_NULLABLE and VMA_NOT_NULL as clang nullability attribtues but does a
// poor job at using them everywhere. Thus it causes lots of clang compiler warnings. We just
// disable them here by defining them to be nothing.
#ifdef VMA_NULLABLE
#undef VMA_NULLABLE
#define VMA_NULLABLE
#endif
#ifdef VMA_NOT_NULL
#undef VMA_NOT_NULL
#define VMA_NOT_NULL
#endif
#define VMA_IMPLEMENTATION
#ifdef __OHOS_PLATFORM__
#include "../../../../../../../third_party/skia/third_party/vulkanmemoryallocator/include/vk_mem_alloc.h"
#else
#include <VulkanMemoryAllocator/src/vk_mem_alloc.h>
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

#include <base/containers/vector.h>
#include <base/util/formats.h>

#if (RENDER_PERF_ENABLED == 1)
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#endif

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

#include "util/log.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GpuBufferDesc& desc)
{
    const uint64_t importantEngineCreationFlags =
        (desc.engineCreationFlags &
            RENDER_NS::EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_SINGLE_SHOT_STAGING) |
        (desc.engineCreationFlags &
            RENDER_NS::EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER);

    uint64_t seed = importantEngineCreationFlags;
    HashCombine(seed, (uint64_t)desc.usageFlags, (uint64_t)desc.memoryPropertyFlags);
    return seed;
}

template<>
uint64_t BASE_NS::hash(const RENDER_NS::GpuImageDesc& desc)
{
    const uint64_t importantImageUsageFlags =
        (desc.usageFlags & RENDER_NS::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) |
        (desc.usageFlags & RENDER_NS::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT);

    uint64_t seed = importantImageUsageFlags;
    HashCombine(seed, (uint64_t)desc.imageType, (uint64_t)desc.memoryPropertyFlags);
    return seed;
}

RENDER_BEGIN_NAMESPACE()
namespace {
#if (RENDER_PERF_ENABLED == 1)
void LogStats(VmaAllocator aAllocator)
{
    if (auto* inst = CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        inst) {
        CORE_NS::IPerformanceDataManager* pdm = inst->Get("Memory");

        VmaBudget budgets[VK_MAX_MEMORY_HEAPS] {};
        vmaGetHeapBudgets(aAllocator, budgets);
        VmaStatistics stats {};
        for (const VmaBudget& budget : budgets) {
            stats.blockBytes += budget.statistics.blockBytes;
            stats.allocationBytes += budget.statistics.allocationBytes;
        }
        pdm->UpdateData("VMA Memory", "AllGraphicsMemory", int64_t(stats.blockBytes));
        pdm->UpdateData("VMA Memory", "GraphicsMemoryInUse", int64_t(stats.allocationBytes));
        pdm->UpdateData("VMA Memory", "GraphicsMemoryNotInUse", int64_t(stats.blockBytes - stats.allocationBytes));
    }
}
#endif

VmaVulkanFunctions GetVmaVulkanFunctions(VkInstance instance, VkDevice device)
{
    VmaVulkanFunctions funs {};
#ifdef USE_NEW_VMA
    funs.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
    funs.vkGetDeviceProcAddr = vkGetDeviceProcAddr;
#endif
    funs.vkGetPhysicalDeviceProperties = vkGetPhysicalDeviceProperties;
    funs.vkGetPhysicalDeviceMemoryProperties = vkGetPhysicalDeviceMemoryProperties;
    funs.vkAllocateMemory = vkAllocateMemory;
    funs.vkFreeMemory = vkFreeMemory;
    funs.vkMapMemory = vkMapMemory;
    funs.vkUnmapMemory = vkUnmapMemory;
    funs.vkFlushMappedMemoryRanges = vkFlushMappedMemoryRanges;
    funs.vkInvalidateMappedMemoryRanges = vkInvalidateMappedMemoryRanges;
    funs.vkBindBufferMemory = vkBindBufferMemory;
    funs.vkBindImageMemory = vkBindImageMemory;
    funs.vkGetBufferMemoryRequirements = vkGetBufferMemoryRequirements;
    funs.vkGetImageMemoryRequirements = vkGetImageMemoryRequirements;
    funs.vkCreateBuffer = vkCreateBuffer;
    funs.vkDestroyBuffer = vkDestroyBuffer;
    funs.vkCreateImage = vkCreateImage;
    funs.vkDestroyImage = vkDestroyImage;
    funs.vkCmdCopyBuffer = vkCmdCopyBuffer;
#if VK_VERSION_1_1
    // NOTE: on some platforms Vulkan library has only the entrypoints for 1.0. To avoid variation just fetch the
    // function always.
    funs.vkGetBufferMemoryRequirements2KHR =
        (PFN_vkGetBufferMemoryRequirements2)vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2");
    funs.vkGetImageMemoryRequirements2KHR =
        (PFN_vkGetImageMemoryRequirements2)vkGetDeviceProcAddr(device, "vkGetImageMemoryRequirements2");
    funs.vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2)(void*)vkGetDeviceProcAddr(device, "vkBindBufferMemory2");
    funs.vkBindImageMemory2KHR = (PFN_vkBindImageMemory2)(void*)vkGetDeviceProcAddr(device, "vkBindImageMemory2");
    funs.vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2)vkGetInstanceProcAddr(
        instance, "vkGetPhysicalDeviceMemoryProperties2");
#else
    // VK_KHR_get_memory_requirements2
    funs.vkGetBufferMemoryRequirements2KHR =
        (PFN_vkGetBufferMemoryRequirements2KHR)vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements2KHR");
    funs.vkGetImageMemoryRequirements2KHR =
        (PFN_vkGetImageMemoryRequirements2KHR)vkGetDeviceProcAddr(device, "vkGetImageMemoryRequirements2KHR");
    // VK_KHR_bind_memory2
    funs.vkBindBufferMemory2KHR = (PFN_vkBindBufferMemory2KHR)vkGetDeviceProcAddr(device, "vkBindBufferMemory2KHR");
    funs.vkBindImageMemory2KHR = (PFN_vkBindImageMemory2KHR)vkGetDeviceProcAddr(device, "vkBindImageMemory2KHR");
    // VK_KHR_get_physical_device_properties2
    funs.vkGetPhysicalDeviceMemoryProperties2KHR = (PFN_vkGetPhysicalDeviceMemoryProperties2KHR)vkGetInstanceProcAddr(
        instance, "vkGetPhysicalDeviceMemoryProperties2KHR");
#endif
    return funs;
}
} // namespace

PlatformGpuMemoryAllocator::PlatformGpuMemoryAllocator(VkInstance instance, VkPhysicalDevice physicalDevice,
    VkDevice device, const GpuMemoryAllocatorCreateInfo& createInfo)
{
    {
        // 0 -> 1.0
        uint32_t vulkanApiVersion = 0;
        // synchronized by the engine
        VmaAllocatorCreateFlags vmaCreateFlags =
            VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_EXTERNALLY_SYNCHRONIZED_BIT;
#if (RENDER_VULKAN_RT_ENABLED == 1)
        vmaCreateFlags |= VmaAllocatorCreateFlagBits::VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        vulkanApiVersion = VK_API_VERSION_1_2;
#endif
        VmaVulkanFunctions funs = GetVmaVulkanFunctions(instance, device);
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.vulkanApiVersion = vulkanApiVersion;
        allocatorInfo.flags = vmaCreateFlags;
        allocatorInfo.preferredLargeHeapBlockSize = createInfo.preferredLargeHeapBlockSize;
        allocatorInfo.instance = instance;
        allocatorInfo.physicalDevice = physicalDevice;
        allocatorInfo.device = device;
        allocatorInfo.pVulkanFunctions = &funs;
        vmaCreateAllocator(&allocatorInfo, &allocator_);
    }

    // custom pools
    for (const auto& ref : createInfo.customPools) {
        PLUGIN_ASSERT(ref.resourceType == MemoryAllocatorResourceType::GPU_BUFFER ||
                      ref.resourceType == MemoryAllocatorResourceType::GPU_IMAGE);

        if (ref.resourceType == MemoryAllocatorResourceType::GPU_BUFFER) {
            CreatePoolForBuffers(ref);
        } else if (ref.resourceType == MemoryAllocatorResourceType::GPU_IMAGE) {
            CreatePoolForImages(ref);
        }
    }
}

PlatformGpuMemoryAllocator::~PlatformGpuMemoryAllocator()
{
#if (RENDER_PERF_ENABLED == 1)
    PLUGIN_ASSERT(memoryDebugStruct_.buffer == 0);
    PLUGIN_ASSERT(memoryDebugStruct_.image == 0);
#endif

    for (auto ref : customGpuBufferPools_) {
        if (ref != VK_NULL_HANDLE) {
            vmaDestroyPool(allocator_, // allocator
                ref);                  // pool
        }
    }
    for (auto ref : customGpuImagePools_) {
        if (ref != VK_NULL_HANDLE) {
            vmaDestroyPool(allocator_, // allocator
                ref);                  // pool
        }
    }

    vmaDestroyAllocator(allocator_);
}

void PlatformGpuMemoryAllocator::CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo,
    const VmaAllocationCreateInfo& allocationCreateInfo, VkBuffer& buffer, VmaAllocation& allocation,
    VmaAllocationInfo& allocationInfo)
{
    VkResult result = vmaCreateBuffer(allocator_, // allocator,
        &bufferCreateInfo,                        // pBufferCreateInfo
        &allocationCreateInfo,                    // pAllocationCreateInfo
        &buffer,                                  // pBuffer
        &allocation,                              // pAllocation
        &allocationInfo);                         // pAllocationInfo
    if (result != VK_SUCCESS) {
        if ((result == VK_ERROR_OUT_OF_DEVICE_MEMORY) && (allocationCreateInfo.pool != VK_NULL_HANDLE)) {
            PLUGIN_LOG_D(
                "Out of buffer memory with custom memory pool (i.e. block size might be exceeded), bytesize: %" PRIu64
                ", falling back to default memory pool",
                bufferCreateInfo.size);
            VmaAllocationCreateInfo fallBackAllocationCreateInfo = allocationCreateInfo;
            fallBackAllocationCreateInfo.pool = VK_NULL_HANDLE;
            result = vmaCreateBuffer(
                allocator_, &bufferCreateInfo, &fallBackAllocationCreateInfo, &buffer, &allocation, &allocationInfo);
        }
        if (result != VK_SUCCESS) {
            PLUGIN_LOG_E(
                "VKResult not VK_SUCCESS in buffer memory allocation(result : %i), (allocation bytesize : %" PRIu64 ")",
                (int32_t)result, bufferCreateInfo.size);
        }
    }

#if (RENDER_PERF_ENABLED == 1)
    if (allocation) {
        memoryDebugStruct_.buffer += (uint64_t)allocation->GetSize();
        LogStats(allocator_);
    }
#endif
}

void PlatformGpuMemoryAllocator::DestroyBuffer(VkBuffer buffer, VmaAllocation allocation)
{
#if (RENDER_PERF_ENABLED == 1)
    uint64_t byteSize = 0;
    if (allocation) {
        byteSize = (uint64_t)allocation->GetSize();
    }
#endif

    vmaDestroyBuffer(allocator_, // allocator
        buffer,                  // buffer
        allocation);             // allocation

#if (RENDER_PERF_ENABLED == 1)
    if (allocation) {
        memoryDebugStruct_.buffer -= byteSize;
        LogStats(allocator_);
    }
#endif
}

void PlatformGpuMemoryAllocator::CreateImage(const VkImageCreateInfo& imageCreateInfo,
    const VmaAllocationCreateInfo& allocationCreateInfo, VkImage& image, VmaAllocation& allocation,
    VmaAllocationInfo& allocationInfo)
{
    VkResult result = vmaCreateImage(allocator_, // allocator,
        &imageCreateInfo,                        // pImageCreateInfo
        &allocationCreateInfo,                   // pAllocationCreateInfo
        &image,                                  // pImage
        &allocation,                             // pAllocation
        &allocationInfo);                        // pAllocationInfo
    if ((result == VK_ERROR_OUT_OF_DEVICE_MEMORY) && (allocationCreateInfo.pool != VK_NULL_HANDLE)) {
        PLUGIN_LOG_D("Out of memory for image with custom memory pool (i.e. block size might be exceeded), falling "
                     "back to default memory pool");
        VmaAllocationCreateInfo fallBackAllocationCreateInfo = allocationCreateInfo;
        fallBackAllocationCreateInfo.pool = VK_NULL_HANDLE;
        result = vmaCreateImage(
            allocator_, &imageCreateInfo, &fallBackAllocationCreateInfo, &image, &allocation, &allocationInfo);
    }
    if (result != VK_SUCCESS) {
        PLUGIN_LOG_E("VKResult not VK_SUCCESS in image memory allocation(result : %i) (bytesize : %" PRIu64 ")",
            (int32_t)result, allocationInfo.size);
    }

#if (RENDER_PERF_ENABLED == 1)
    if (allocation) {
        memoryDebugStruct_.image += (uint64_t)allocation->GetSize();
        LogStats(allocator_);
    }
#endif
}

void PlatformGpuMemoryAllocator::DestroyImage(VkImage image, VmaAllocation allocation)
{
#if (RENDER_PERF_ENABLED == 1)
    uint64_t byteSize = 0;
    if (allocation) {
        byteSize = (uint64_t)allocation->GetSize();
    }
#endif

    vmaDestroyImage(allocator_, // allocator
        image,                  // image
        allocation);            // allocation

#if (RENDER_PERF_ENABLED == 1)
    if (allocation) {
        memoryDebugStruct_.image -= byteSize;
        LogStats(allocator_);
    }
#endif
}

uint32_t PlatformGpuMemoryAllocator::GetMemoryTypeProperties(const uint32_t memoryType)
{
    VkMemoryPropertyFlags memPropertyFlags = 0;
    vmaGetMemoryTypeProperties(allocator_, memoryType, &memPropertyFlags);
    return (uint32_t)memPropertyFlags;
}

void PlatformGpuMemoryAllocator::FlushAllocation(
    const VmaAllocation& allocation, const VkDeviceSize offset, const VkDeviceSize byteSize)
{
    vmaFlushAllocation(allocator_, allocation, offset, byteSize);
}

void PlatformGpuMemoryAllocator::InvalidateAllocation(
    const VmaAllocation& allocation, const VkDeviceSize offset, const VkDeviceSize byteSize)
{
    vmaInvalidateAllocation(allocator_, allocation, offset, byteSize);
}

VmaPool PlatformGpuMemoryAllocator::GetBufferPool(const GpuBufferDesc& desc) const
{
    VmaPool result = VK_NULL_HANDLE;
    const uint64_t hash = BASE_NS::hash(desc);
    if (const auto iter = hashToGpuBufferPoolIndex_.find(hash); iter != hashToGpuBufferPoolIndex_.cend()) {
        PLUGIN_ASSERT(iter->second < (uint32_t)customGpuBufferPools_.size());
        result = customGpuBufferPools_[iter->second];
    }
    return result;
}

VmaPool PlatformGpuMemoryAllocator::GetImagePool(const GpuImageDesc& desc) const
{
    VmaPool result = VK_NULL_HANDLE;
    const uint64_t hash = BASE_NS::hash(desc);
    if (const auto iter = hashToGpuImagePoolIndex_.find(hash); iter != hashToGpuImagePoolIndex_.cend()) {
        PLUGIN_ASSERT(iter->second < (uint32_t)customGpuImagePools_.size());
        result = customGpuImagePools_[iter->second];
    }
    return result;
}

void* PlatformGpuMemoryAllocator::MapMemory(VmaAllocation allocation)
{
    void* data { nullptr };
    VALIDATE_VK_RESULT(vmaMapMemory(allocator_, allocation, &data));
    return data;
}

void PlatformGpuMemoryAllocator::UnmapMemory(VmaAllocation allocation)
{
    vmaUnmapMemory(allocator_, allocation);
}

void PlatformGpuMemoryAllocator::CreatePoolForBuffers(const GpuMemoryAllocatorCustomPool& customPool)
{
    PLUGIN_ASSERT(customPool.resourceType == MemoryAllocatorResourceType::GPU_BUFFER);

    const auto& buf = customPool.gpuResourceDesc.buffer;

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.preferredFlags = (VkMemoryPropertyFlags)buf.memoryPropertyFlags;
    allocationCreateInfo.requiredFlags = allocationCreateInfo.preferredFlags;
    if (allocationCreateInfo.preferredFlags ==
        ((VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) | (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT))) {
        allocationCreateInfo.flags = VmaAllocationCreateFlagBits::VMA_ALLOCATION_CREATE_MAPPED_BIT;
    }

    VkBufferCreateInfo bufferCreateInfo = {};
    bufferCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.usage = VkBufferUsageFlagBits(buf.usageFlags);
    bufferCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
    bufferCreateInfo.size = 1024u; // default reference size

    uint32_t memoryTypeIndex = 0;
    VALIDATE_VK_RESULT(vmaFindMemoryTypeIndexForBufferInfo(allocator_, // pImageCreateInfo
        &bufferCreateInfo,                                             // pBufferCreateInfo
        &allocationCreateInfo,                                         // pAllocationCreateInfo
        &memoryTypeIndex));                                            // pMemoryTypeIndex

    VmaPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.flags = (customPool.linearAllocationAlgorithm)
                               ? VmaPoolCreateFlagBits::VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT
                               : (VkFlags)0;
    poolCreateInfo.memoryTypeIndex = memoryTypeIndex;
    poolCreateInfo.blockSize = customPool.blockSize;

    const uint64_t hash = BASE_NS::hash(buf);
    const size_t hashCount = hashToGpuBufferPoolIndex_.count(hash);

    PLUGIN_ASSERT_MSG(hashCount == 0, "similar buffer pool already created");
    if (hashCount == 0) {
        VmaPool pool = VK_NULL_HANDLE;
        VALIDATE_VK_RESULT(vmaCreatePool(allocator_, // allocator
            &poolCreateInfo,                         // pCreateInfo
            &pool));                                 // pPool

        customGpuBufferPools_.push_back(pool);
        hashToGpuBufferPoolIndex_[hash] = (uint32_t)customGpuBufferPools_.size() - 1;
#if (RENDER_PERF_ENABLED == 1)
        customGpuBufferPoolNames_.push_back(customPool.name);
#endif
    }
}

void PlatformGpuMemoryAllocator::CreatePoolForImages(const GpuMemoryAllocatorCustomPool& customPool)
{
    PLUGIN_ASSERT(customPool.resourceType == MemoryAllocatorResourceType::GPU_IMAGE);

    const auto& img = customPool.gpuResourceDesc.image;

    VmaAllocationCreateInfo allocationCreateInfo = {};
    allocationCreateInfo.preferredFlags = (VkMemoryPropertyFlags)img.memoryPropertyFlags;
    allocationCreateInfo.requiredFlags = allocationCreateInfo.preferredFlags;

    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.usage = VkImageUsageFlagBits(img.usageFlags);
    imageCreateInfo.imageType = VkImageType(img.imageType);
    imageCreateInfo.format = VkFormat::VK_FORMAT_R8G8B8A8_UNORM;
    imageCreateInfo.sharingMode = VkSharingMode::VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.extent = VkExtent3D { 1024u, 1024u, 1 }; // default reference size
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.samples = VkSampleCountFlagBits::VK_SAMPLE_COUNT_1_BIT;

    uint32_t memoryTypeIndex = 0;
    VALIDATE_VK_RESULT(vmaFindMemoryTypeIndexForImageInfo(allocator_, // pImageCreateInfo
        &imageCreateInfo,                                             // pImageCreateInfo
        &allocationCreateInfo,                                        // pAllocationCreateInfo
        &memoryTypeIndex));                                           // pMemoryTypeIndex

    VmaPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.flags = (customPool.linearAllocationAlgorithm)
                               ? VmaPoolCreateFlagBits::VMA_POOL_CREATE_LINEAR_ALGORITHM_BIT
                               : (VkFlags)0;
    poolCreateInfo.memoryTypeIndex = memoryTypeIndex;

    const uint64_t hash = BASE_NS::hash(img);
    const size_t hashCount = hashToGpuImagePoolIndex_.count(hash);

    PLUGIN_ASSERT_MSG(hashCount == 0, "similar image pool already created");
    if (hashCount == 0) {
        VmaPool pool = VK_NULL_HANDLE;
        VALIDATE_VK_RESULT(vmaCreatePool(allocator_, // allocator
            &poolCreateInfo,                         // pCreateInfo
            &pool));                                 // pPool

        customGpuImagePools_.push_back(pool);
        hashToGpuImagePoolIndex_[hash] = (uint32_t)customGpuImagePools_.size() - 1;
#if (RENDER_PERF_ENABLED == 1)
        customGpuImagePoolNames_.push_back(customPool.name);
#endif
    }
}

#if (RENDER_PERF_ENABLED == 1)
string PlatformGpuMemoryAllocator::GetBufferPoolDebugName(const GpuBufferDesc& desc) const
{
    const size_t hash = BASE_NS::hash(desc);
    if (const auto iter = hashToGpuBufferPoolIndex_.find(hash); iter != hashToGpuBufferPoolIndex_.cend()) {
        PLUGIN_ASSERT(iter->second < (uint32_t)customGpuBufferPools_.size());
        return customGpuBufferPoolNames_[iter->second];
    }
    return {};
}
string PlatformGpuMemoryAllocator::GetImagePoolDebugName(const GpuImageDesc& desc) const
{
    const size_t hash = BASE_NS::hash(desc);
    if (const auto iter = hashToGpuImagePoolIndex_.find(hash); iter != hashToGpuImagePoolIndex_.cend()) {
        PLUGIN_ASSERT(iter->second < (uint32_t)customGpuImagePools_.size());
        return customGpuImagePoolNames_[iter->second];
    }
    return {};
}
#endif
RENDER_END_NAMESPACE()
