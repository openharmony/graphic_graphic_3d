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

#include "gpu_image_vk.h"

#include <cinttypes>
#include <vulkan/vulkan_core.h>

#include <base/math/mathf.h>

#if (RENDER_PERF_ENABLED == 1)
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#endif

#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_desc_flag_validation.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
namespace {
#if (RENDER_PERF_ENABLED == 1)
void RecordAllocation(const int64_t alignedByteSize)
{
    if (auto* inst = CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
        inst) {
        CORE_NS::IPerformanceDataManager* pdm = inst->Get("Memory");
        pdm->UpdateData("AllGpuImages", "GPU_IMAGE", alignedByteSize);
    }
}
#endif

#if (RENDER_VALIDATION_ENABLED == 1)
void ValidateFormat(const DevicePlatformDataVk& devicePlat, const GpuImageDesc& desc)
{
    const VkFormat format = (VkFormat)desc.format;
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(devicePlat.physicalDevice, // physicalDevice
        format,                                                    // format
        &formatProperties);                                        // pFormatProperties
    const VkFormatFeatureFlags optimalTilingFeatureFlags = formatProperties.optimalTilingFeatures;
    bool valid = true;
    if (desc.usageFlags & ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) {
        if ((optimalTilingFeatureFlags & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) ==
            0) {
            valid = false;
        }
    }
    if (desc.usageFlags & ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) {
        if ((optimalTilingFeatureFlags & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT) == 0) {
            valid = false;
        }
    }
    if (desc.usageFlags & ImageUsageFlagBits::CORE_IMAGE_USAGE_STORAGE_BIT) {
        if ((optimalTilingFeatureFlags & VkFormatFeatureFlagBits::VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT) == 0) {
            valid = false;
        }
    }

    if (valid == false) {
        PLUGIN_LOG_E("Unsupported image format feature flags (CORE_FORMAT: %u)", desc.format);
    }
}
#endif

constexpr uint32_t IMAGE_VIEW_USAGE_FLAGS {
    CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
    CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT |
    CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | CORE_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT
};

BASE_NS::Format CheckDepthFormat(const DeviceVk& deviceVk, const BASE_NS::Format format)
{
    const auto& devPlat = deviceVk.GetPlatformInternalDataVk();
    for (const auto& supportedDepthFormat : devPlat.supportedDepthFormats) {
        if (format == supportedDepthFormat) {
            return format;
        }
    }
    if (!devPlat.supportedDepthFormats.empty()) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("RENDER_VALIDATION: unsupported depth format (%u), using format (%u)", format,
            devPlat.supportedDepthFormats[0]);
#endif
        return devPlat.supportedDepthFormats[0];
    } else {
        return BASE_NS::Format::BASE_FORMAT_UNDEFINED;
    }
}

inline VkImageViewType GetBaseImageViewType(const VkImageViewType imageViewType)
{
    if (imageViewType == VK_IMAGE_VIEW_TYPE_1D_ARRAY) {
        return VK_IMAGE_VIEW_TYPE_1D;
    } else if (imageViewType == VK_IMAGE_VIEW_TYPE_2D_ARRAY) {
        return VK_IMAGE_VIEW_TYPE_2D;
    } else if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        return VK_IMAGE_VIEW_TYPE_CUBE;
    } else if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE) {
        return VK_IMAGE_VIEW_TYPE_2D;
    }
    return imageViewType;
}

inline GpuResourceMemoryVk GetPlatMemory(const VmaAllocationInfo& allocationInfo, const VkMemoryPropertyFlags flags)
{
    return GpuResourceMemoryVk { allocationInfo.deviceMemory, allocationInfo.offset, allocationInfo.size,
        allocationInfo.pMappedData, allocationInfo.memoryType, flags };
}

inline bool InvalidFboSwizzle(const VkComponentMapping& componentMapping)
{
    return ((componentMapping.r != VK_COMPONENT_SWIZZLE_IDENTITY) ||
            (componentMapping.g != VK_COMPONENT_SWIZZLE_IDENTITY) ||
            (componentMapping.b != VK_COMPONENT_SWIZZLE_IDENTITY) ||
            (componentMapping.a != VK_COMPONENT_SWIZZLE_IDENTITY));
}

void FillImageDescVk(const GpuImageDesc& desc, GpuImagePlatformDataVk& plat)
{
    plat.format = static_cast<VkFormat>(desc.format);
    plat.aspectFlags = GpuImageUtilsVk::GetImageAspectFlagsFromFormat(plat.format);
    plat.usage = static_cast<VkImageUsageFlags>(desc.usageFlags);
    plat.extent = { desc.width, desc.height, desc.depth };
    plat.tiling = static_cast<VkImageTiling>(desc.imageTiling);
    plat.type = static_cast<VkImageType>(desc.imageType);
    plat.samples = static_cast<VkSampleCountFlagBits>(desc.sampleCountFlags);
    plat.mipLevels = desc.mipCount;
    plat.arrayLayers = desc.layerCount;
}

struct ImageInputStruct {
    VkImage image { VK_NULL_HANDLE };
    VkFormat format { VK_FORMAT_UNDEFINED };
    VkComponentMapping componentMapping {};
};

VkImageView CreateImageView(const VkDevice device, const VkSamplerYcbcrConversionInfo* ycbcrConversionInfo,
    const ImageInputStruct& imageInput, const VkImageViewType imageViewType, const VkImageAspectFlags imageAspectFlags,
    const uint32_t baseMipLevel, const uint32_t levelCount, const uint32_t baseArrayLayer, const uint32_t layerCount)
{
    const VkImageSubresourceRange imageSubresourceRange {
        imageAspectFlags, // aspectMask
        baseMipLevel,     // baseMipLevel
        levelCount,       // levelCount
        baseArrayLayer,   // baseArrayLayer
        layerCount        // layerCount
    };

    const VkImageViewCreateInfo imageViewCreateInfo {
        VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
        ycbcrConversionInfo,                      // pNext
        0,                                        // flags
        imageInput.image,                         // image
        imageViewType,                            // viewType
        imageInput.format,                        // format
        imageInput.componentMapping,              // components
        imageSubresourceRange,                    // subresourceRange
    };

    VkImageView imageView = VK_NULL_HANDLE;
    VALIDATE_VK_RESULT(vkCreateImageView(device, // device
        &imageViewCreateInfo,                    // pCreateInfo
        nullptr,                                 // pAllocator
        &imageView));                            // pView

    return imageView;
}
} // namespace

GpuImageVk::GpuImageVk(Device& device, const GpuImageDesc& desc) : GpuImage(), device_(device), desc_(desc)
{
    PLUGIN_ASSERT_MSG(desc_.memoryPropertyFlags & MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        "Device local memory is only memory property supported for Vulkan GpuImage (flags: %u)",
        desc_.memoryPropertyFlags);

    FillImageDescVk(desc_, plat_);
    if (plat_.aspectFlags & VK_IMAGE_ASPECT_DEPTH_BIT) {
        desc_.format = CheckDepthFormat((const DeviceVk&)device_, desc_.format);
        if (desc_.format != desc.format) {
            plat_.format = static_cast<VkFormat>(desc_.format);
            plat_.aspectFlags = GpuImageUtilsVk::GetImageAspectFlagsFromFormat(plat_.format);
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateFormat((const DevicePlatformDataVk&)device_.GetPlatformData(), desc_);
#endif

    CreateVkImage();
    if ((desc_.usageFlags & IMAGE_VIEW_USAGE_FLAGS) && plat_.image) {
        CreateVkImageViews(plat_.aspectFlags, nullptr);
    }

#if (RENDER_PERF_ENABLED == 1)
    RecordAllocation(static_cast<int64_t>(mem_.allocationInfo.size));
#endif

#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
    PLUGIN_LOG_E("gpu image id >: 0x%" PRIxPTR, (uintptr_t)plat_.image);
#endif
}

GpuImageVk::GpuImageVk(
    Device& device, const GpuImageDesc& desc, const GpuImagePlatformData& platformData, const uintptr_t hwBuffer)
    : device_(device), plat_((const GpuImagePlatformDataVk&)platformData),
      desc_(hwBuffer ? GetImageDescFromHwBufferDesc(hwBuffer) : desc), hwBuffer_(hwBuffer), ownsResources_(false)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!plat_.image && !plat_.imageView && hwBuffer) {
        PLUGIN_LOG_W("RENDER_VALIDATION: creating GpuImage without image, imageView, or hwBuffer");
    }
#endif
    FillImageDescVk(desc_, plat_);
    if (plat_.image && !plat_.imageView && (desc_.usageFlags & IMAGE_VIEW_USAGE_FLAGS)) {
        CreateVkImageViews(plat_.aspectFlags, nullptr);
    } else if (plat_.imageView) {
        plat_.imageViewBase = plat_.imageView;
    }
    if (hwBuffer) {
        CreatePlatformHwBuffer();
    }
}

GpuImageVk::~GpuImageVk()
{
    auto destroyImageViews = [](VkDevice device, auto& vec) {
        for (auto& ref : vec) {
            vkDestroyImageView(device, // device
                ref,                   // imageView
                nullptr);              // pAllocator
        }
        vec.clear();
    };
    // hw buffer variant creates image views and needs to destroy those as well
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    if (ownsResources_ || (hwBuffer_ != 0)) {
        vkDestroyImageView(device, // device
            plat_.imageView,       // imageView
            nullptr);              // pAllocator
        if (destroyImageViewBase_) {
            vkDestroyImageView(device, // device
                plat_.imageViewBase,   // imageView
                nullptr);              // pAllocator
        }
        destroyImageViews(device, platViews_.mipImageViews);
        destroyImageViews(device, platViews_.layerImageViews);
        destroyImageViews(device, platViews_.mipImageAllLayerViews);
    }

    if (ownsResources_) {
#if (RENDER_PERF_ENABLED == 1)
        RecordAllocation(-static_cast<int64_t>(mem_.allocationInfo.size));
#endif
#if (RENDER_DEBUG_GPU_RESOURCE_IDS == 1)
        PLUGIN_LOG_E("gpu image id <: 0x%" PRIxPTR, (uintptr_t)plat_.image);
#endif
        PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator();
        PLUGIN_ASSERT(gpuMemAllocator);
        if (gpuMemAllocator) {
            gpuMemAllocator->DestroyImage(plat_.image, mem_.allocation);
        }
    } else if (hwBuffer_ != 0) {
        DestroyPlatformHwBuffer();
    }
}

void GpuImageVk::CreateVkImage()
{
    const VkImageCreateInfo imageCreateInfo {
        VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,                // sType
        nullptr,                                            // pNext
        static_cast<VkImageCreateFlags>(desc_.createFlags), // flags
        plat_.type,                                         // imageType
        plat_.format,                                       // format
        plat_.extent,                                       // extent
        plat_.mipLevels,                                    // mipLevels
        plat_.arrayLayers,                                  // arrayLayers
        plat_.samples,                                      // samples
        plat_.tiling,                                       // tiling
        plat_.usage,                                        // usage
        VkSharingMode::VK_SHARING_MODE_EXCLUSIVE,           // sharingMode
        0,                                                  // queueFamilyIndexCount
        nullptr,                                            // pQueueFamilyIndices
        VkImageLayout::VK_IMAGE_LAYOUT_UNDEFINED,           // initialLayout
    };

    VkMemoryPropertyFlags memoryPropertyFlags = static_cast<VkMemoryPropertyFlags>(desc_.memoryPropertyFlags);
    const VkMemoryPropertyFlags requiredFlags =
        (memoryPropertyFlags & (~(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT |
                                   CORE_MEMORY_PROPERTY_PROTECTED_BIT)));
    const VkMemoryPropertyFlags preferredFlags = memoryPropertyFlags;

    PlatformGpuMemoryAllocator* gpuMemAllocator = device_.GetPlatformGpuMemoryAllocator();
    if (gpuMemAllocator) {
        // can be null handle -> default allocator
        const VmaPool customPool = gpuMemAllocator->GetImagePool(desc_);

        const VmaAllocationCreateInfo allocationCreateInfo {
            0, // flags
#ifdef USE_NEW_VMA
            VmaMemoryUsage::VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE, // usage
#else
            VmaMemoryUsage::VMA_MEMORY_USAGE_GPU_ONLY, // usage
#endif
            requiredFlags,  // requiredFlags
            preferredFlags, // preferredFlags
            0,              // memoryTypeBits
            customPool,     // pool
            nullptr,        // pUserData
#ifdef USE_NEW_VMA
            0.f, // priority
#endif
        };

        gpuMemAllocator->CreateImage(
            imageCreateInfo, allocationCreateInfo, plat_.image, mem_.allocation, mem_.allocationInfo);
    }

    plat_.memory = GetPlatMemory(mem_.allocationInfo, preferredFlags);
}

void GpuImageVk::CreateVkImageViews(
    VkImageAspectFlags imageAspectFlags, const VkSamplerYcbcrConversionInfo* ycbcrConversionInfo)
{
    PLUGIN_ASSERT(plat_.image);
    const VkDevice vkDevice = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

    const VkImageViewType imageViewType = (VkImageViewType)desc_.imageViewType;
    const VkImageAspectFlags shaderViewImageAspectFlags = imageAspectFlags & (~VK_IMAGE_ASPECT_STENCIL_BIT);

    const VkComponentMapping componentMapping = {
        (VkComponentSwizzle)desc_.componentMapping.r,
        (VkComponentSwizzle)desc_.componentMapping.g,
        (VkComponentSwizzle)desc_.componentMapping.b,
        (VkComponentSwizzle)desc_.componentMapping.a,
    };

    const ImageInputStruct imageInput = { plat_.image, plat_.format, componentMapping };
    // Create basic image view for sampling and general usage
    plat_.imageView = CreateImageView(vkDevice, ycbcrConversionInfo, imageInput, imageViewType,
        shaderViewImageAspectFlags, 0, plat_.mipLevels, 0, plat_.arrayLayers);
    plat_.imageViewBase = plat_.imageView;

    const bool invalidFboSwizzle = InvalidFboSwizzle(componentMapping);
    const bool notValidImageViewForAttachment = (plat_.mipLevels > 1) || (plat_.arrayLayers > 1) || invalidFboSwizzle;
    const bool usageNeedsViews = (plat_.usage & (VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_STORAGE_BIT |
                                                    VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) > 0;
    const bool separateViewNeeded = (imageAspectFlags != shaderViewImageAspectFlags);
    if (separateViewNeeded || (usageNeedsViews && notValidImageViewForAttachment)) {
        destroyImageViewBase_ = true;
        const VkImageViewType baseImageViewType = GetBaseImageViewType(imageViewType);
        {
            ImageInputStruct imageInputIdentity = imageInput;
            imageInputIdentity.componentMapping = {}; // identity needed for fbo
            plat_.imageViewBase = CreateImageView(
                vkDevice, ycbcrConversionInfo, imageInputIdentity, baseImageViewType, imageAspectFlags, 0U, 1U, 0U, 1U);
        }

        if (plat_.mipLevels > 1) {
            platViews_.mipImageViews.resize(plat_.mipLevels);
            if (plat_.arrayLayers > 1U) {
                platViews_.mipImageAllLayerViews.resize(plat_.mipLevels);
            }
            for (uint32_t mipIdx = 0; mipIdx < plat_.mipLevels; ++mipIdx) {
                platViews_.mipImageViews[mipIdx] = CreateImageView(
                    vkDevice, ycbcrConversionInfo, imageInput, baseImageViewType, imageAspectFlags, mipIdx, 1U, 0U, 1U);
                if (plat_.arrayLayers > 1U) {
                    platViews_.mipImageAllLayerViews[mipIdx] = CreateImageView(vkDevice, ycbcrConversionInfo,
                        imageInput, VK_IMAGE_VIEW_TYPE_2D_ARRAY, imageAspectFlags, mipIdx, 1U, 0U, plat_.arrayLayers);
                }
            }
        }
        if (plat_.arrayLayers > 1) {
            platViews_.layerImageViews.resize(plat_.arrayLayers);
            for (uint32_t layerIdx = 0; layerIdx < plat_.arrayLayers; ++layerIdx) {
                platViews_.layerImageViews[layerIdx] = CreateImageView(vkDevice, ycbcrConversionInfo, imageInput,
                    baseImageViewType, imageAspectFlags, 0U, 1U, layerIdx, 1U);
            }
        }
        if (imageViewType == VK_IMAGE_VIEW_TYPE_CUBE) {
            if (platViews_.mipImageAllLayerViews.empty()) {
                platViews_.mipImageAllLayerViews.resize(plat_.mipLevels);
                for (uint32_t mipIdx = 0; mipIdx < plat_.mipLevels; ++mipIdx) {
                    platViews_.mipImageAllLayerViews[mipIdx] = CreateImageView(vkDevice, ycbcrConversionInfo,
                        imageInput, VK_IMAGE_VIEW_TYPE_2D_ARRAY, imageAspectFlags, mipIdx, 1U, 0U, plat_.arrayLayers);
                }
            }
        }
    }
}

const GpuImageDesc& GpuImageVk::GetDesc() const
{
    return desc_;
}

const GpuImagePlatformData& GpuImageVk::GetBasePlatformData() const
{
    return plat_;
}

const GpuImagePlatformDataVk& GpuImageVk::GetPlatformData() const
{
    return plat_;
}

const GpuImagePlatformDataViewsVk& GpuImageVk::GetPlatformDataViews() const
{
    return platViews_;
}

const GpuImagePlatformDataConversion& GpuImageVk::GetPlaformDataConversion() const
{
    return platConversion_;
}

GpuImage::AdditionalFlags GpuImageVk::GetAdditionalFlags() const
{
    return (platConversion_.samplerConversion) ? ADDITIONAL_PLATFORM_CONVERSION_BIT : 0u;
}

namespace GpuImageUtilsVk {
VkImageAspectFlags GetImageAspectFlagsFromFormat(const VkFormat format)
{
    VkImageAspectFlags flags {};

    const bool isDepthFormat =
        ((format == VkFormat::VK_FORMAT_D16_UNORM) || (format == VkFormat::VK_FORMAT_X8_D24_UNORM_PACK32) ||
            (format == VkFormat::VK_FORMAT_D32_SFLOAT) || (format == VkFormat::VK_FORMAT_D16_UNORM_S8_UINT) ||
            (format == VkFormat::VK_FORMAT_D24_UNORM_S8_UINT))
            ? true
            : false;
    if (isDepthFormat) {
        flags |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_DEPTH_BIT;

        const bool isStencilFormat =
            ((format == VkFormat::VK_FORMAT_S8_UINT) || (format == VkFormat::VK_FORMAT_D16_UNORM_S8_UINT) ||
                (format == VkFormat::VK_FORMAT_D24_UNORM_S8_UINT) || (format == VkFormat::VK_FORMAT_D32_SFLOAT_S8_UINT))
                ? true
                : false;
        if (isStencilFormat) {
            flags |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
        }

    } else if (format == VkFormat::VK_FORMAT_S8_UINT) {
        flags |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_STENCIL_BIT;
    } else {
        flags |= VkImageAspectFlagBits::VK_IMAGE_ASPECT_COLOR_BIT;
    }

    return flags;
}
} // namespace GpuImageUtilsVk
RENDER_END_NAMESPACE()
