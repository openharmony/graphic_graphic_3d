/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_GPU_IMAGE_VK_H
#define VULKAN_GPU_IMAGE_VK_H

#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "device/gpu_image.h"
#include "vulkan/gpu_memory_allocator_vk.h"

RENDER_BEGIN_NAMESPACE()
struct GpuImagePlatformDataViewsVk final {
    // generated only for color attachments
    BASE_NS::vector<VkImageView> mipImageViews;
    BASE_NS::vector<VkImageView> layerImageViews;
};

struct GpuImagePlatformDataConversion final {
    // Sampler conversion object, used only with immutable samplers and created sampler conversion
    VkSampler sampler { VK_NULL_HANDLE };
    // Sampler conversion object, used only with immutable samplers and views
    VkSamplerYcbcrConversion samplerConversion { VK_NULL_HANDLE };
};

class Device;

class GpuImageVk final : public GpuImage {
public:
    GpuImageVk(Device& device, const GpuImageDesc& desc);
    GpuImageVk(
        Device& device, const GpuImageDesc& desc, const GpuImagePlatformData& platformData, const uintptr_t hwBuffer);
    ~GpuImageVk();

    const GpuImageDesc& GetDesc() const override;
    const GpuImagePlatformData& GetBasePlatformData() const override;
    const GpuImagePlatformDataVk& GetPlatformData() const;
    const GpuImagePlatformDataViewsVk& GetPlatformDataViews() const;
    const GpuImagePlatformDataConversion& GetPlaformDataConversion() const;

    AdditionalFlags GetAdditionalFlags() const override;

private:
    void CreateVkImage();
    void CreateVkImageViews(
        VkImageAspectFlags imageAspectFlags, const VkSamplerYcbcrConversionInfo* ycbcrConversionInfo);
    void CreatePlatformHwBuffer();
    void DestroyPlatformHwBuffer();

    Device& device_;

    GpuImagePlatformDataVk plat_;
    GpuImagePlatformDataViewsVk platViews_;
    GpuImagePlatformDataConversion platConversion_;
    GpuImageDesc desc_;
    uintptr_t hwBuffer_ { 0 };
    // in normal situations owns all the vulkan resources
    bool ownsResources_ { true };
    bool destroyImageViewBase_ { false };

    struct MemoryAllocation {
        VmaAllocation allocation;
        VmaAllocationInfo allocationInfo;
    };
    MemoryAllocation mem_ {};
};

namespace GpuImageUtilsVk {
VkImageAspectFlags GetImageAspectFlagsFromFormat(const VkFormat format);
} // namespace GpuImageUtilsVk
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_IMAGE_VK_H
