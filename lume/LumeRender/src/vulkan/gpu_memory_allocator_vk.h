/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_GPU_MEMORY_ALLOCATOR_VK_H
#define VULKAN_GPU_MEMORY_ALLOCATOR_VK_H

#include <VulkanMemoryAllocator/src/vk_mem_alloc.h>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/string.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
/** Gpu memory allocator.
 * Wrapper around Vulkan Memory Allocator (GPU Open) which is internally synchronized.
 */
class PlatformGpuMemoryAllocator final {
public:
    enum class MemoryAllocatorResourceType : uint8_t {
        UNDEFINED = 0, // not supported ATM, needs to be buffer or image
        GPU_BUFFER = 1,
        GPU_IMAGE = 2,
    };
    struct GpuMemoryAllocatorCustomPool {
        BASE_NS::string name;

        MemoryAllocatorResourceType resourceType { MemoryAllocatorResourceType::UNDEFINED };
        uint32_t blockSize { 0 }; // zero fallbacks to default
        bool linearAllocationAlgorithm { false };

        union GpuResourceDesc {
            GpuBufferDesc buffer;
            GpuImageDesc image;
        };
        GpuResourceDesc gpuResourceDesc;
    };

    struct GpuMemoryAllocatorCreateInfo {
        // set to zero for default (vma default 256 MB)
        uint32_t preferredLargeHeapBlockSize { 32 * 1024 * 1024 };

        BASE_NS::vector<GpuMemoryAllocatorCustomPool> customPools;
    };

    PlatformGpuMemoryAllocator(VkInstance instance, VkPhysicalDevice physicalDevice, VkDevice device,
        const GpuMemoryAllocatorCreateInfo& createInfo);
    ~PlatformGpuMemoryAllocator();

    void CreateBuffer(const VkBufferCreateInfo& bufferCreateInfo, const VmaAllocationCreateInfo& allocationCreateInfo,
        VkBuffer& buffer, VmaAllocation& allocation, VmaAllocationInfo& allocationInfo);
    void DestroyBuffer(VkBuffer buffer, VmaAllocation allocation);

    void CreateImage(const VkImageCreateInfo& imageCreateInfo, const VmaAllocationCreateInfo& allocationCreateInfo,
        VkImage& image, VmaAllocation& allocation, VmaAllocationInfo& allocationInfo);
    void DestroyImage(VkImage image, VmaAllocation allocation);

    void* MapMemory(VmaAllocation allocation);
    void UnmapMemory(VmaAllocation allocation);

    void FlushAllocation(const VmaAllocation& allocation, const VkDeviceSize offset, const VkDeviceSize byteSize);
    void InvalidateAllocation(const VmaAllocation& allocation, const VkDeviceSize offset, const VkDeviceSize byteSize);
    uint32_t GetMemoryTypeProperties(const uint32_t memoryType);

    VmaPool GetBufferPool(const GpuBufferDesc& desc) const;
    VmaPool GetImagePool(const GpuImageDesc& desc) const;

#if (RENDER_PERF_ENABLED == 1)
    BASE_NS::string GetBufferPoolDebugName(const GpuBufferDesc& desc) const;
    BASE_NS::string GetImagePoolDebugName(const GpuImageDesc& desc) const;
#endif

private:
    void CreatePoolForBuffers(const GpuMemoryAllocatorCustomPool& customPool);
    void CreatePoolForImages(const GpuMemoryAllocatorCustomPool& customPool);
    VmaAllocator allocator_ {};

    BASE_NS::vector<VmaPool> customGpuBufferPools_;
    BASE_NS::vector<VmaPool> customGpuImagePools_;

    BASE_NS::unordered_map<uint64_t, uint32_t> hashToGpuBufferPoolIndex_;
    BASE_NS::unordered_map<uint64_t, uint32_t> hashToGpuImagePoolIndex_;

#if (RENDER_PERF_ENABLED == 1)
    BASE_NS::vector<BASE_NS::string> customGpuBufferPoolNames_;
    BASE_NS::vector<BASE_NS::string> customGpuImagePoolNames_;

    struct MemoryDebugStruct {
        uint64_t buffer { 0 };
        uint64_t image { 0 };
    };
    MemoryDebugStruct memoryDebugStruct_;
#endif
};
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_MEMORY_ALLOCATOR_VK_H
