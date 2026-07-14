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

#include "gpu_image_mln.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "maleoon/device_mln.h"
#include "mln_convert.h"
#include "mln_log.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {
MlnImageAspectFlags GetImageAspectFlags(Format format)
{
    // Depth/stencil formats
    switch (format) {
        case BASE_FORMAT_D16_UNORM:
        case BASE_FORMAT_D32_SFLOAT:
            return MLN_IMAGE_ASPECT_DEPTH_BIT;
        case BASE_FORMAT_S8_UINT:
            return MLN_IMAGE_ASPECT_STENCIL_BIT;
        case BASE_FORMAT_D16_UNORM_S8_UINT:
        case BASE_FORMAT_D24_UNORM_S8_UINT:
        case BASE_FORMAT_D32_SFLOAT_S8_UINT:
            return MLN_IMAGE_ASPECT_DEPTH_BIT | MLN_IMAGE_ASPECT_STENCIL_BIT;
        default:
            return MLN_IMAGE_ASPECT_COLOR_BIT;
    }
}
}  // namespace

GpuImageMln::GpuImageMln(Device& device, const GpuImageDesc& desc) : device_(device), desc_(desc)
{
    MLN_LOG_INIT(
        "GpuImageMln: creating image (%ux%u, fmt=%u)", desc.width, desc.height, static_cast<uint32_t>(desc.format));
    CreateImage();
    if (!plat_.resource) {
        MLN_LOG_ERR("GpuImageMln: CreateImage failed — skipping memory and views (%ux%u, fmt=%u)",
            desc.width,
            desc.height,
            static_cast<uint32_t>(desc.format));
        return;
    }
    AllocateAndBindMemory();
    if (!plat_.memory) {
        MLN_LOG_ERR("GpuImageMln: AllocateAndBindMemory failed — skipping views (%ux%u, fmt=%u)",
            desc.width,
            desc.height,
            static_cast<uint32_t>(desc.format));
        return;
    }
    CreateImageViews();
}

GpuImageMln::GpuImageMln(Device& device, const GpuImageDesc& desc, const GpuImagePlatformData& platformData)
    : device_(device), desc_(desc)
{
    MLN_LOG_INIT("GpuImageMln: wrapping platform image (%ux%u, fmt=%u, depth=%u, mip=%u, layers=%u)",
        desc.width,
        desc.height,
        static_cast<uint32_t>(desc.format),
        desc.depth,
        desc.mipCount,
        desc.layerCount);
    // Wrapping an existing platform image -- don't create/own the underlying resource
    ownsImage_ = false;
    ownsResources_ = false;

    const auto& mlnPlatData = static_cast<const GpuImagePlatformDataMln&>(platformData);
    plat_.resource = mlnPlatData.resource;
    plat_.format = ToMlnFormat(desc_.format);
    plat_.extent = {desc_.width, desc_.height, desc_.depth};
    plat_.imageType = ToMlnImageType(desc_.imageType);
    plat_.usage = ToMlnImageUsageFlags(desc_.usageFlags);
    plat_.samples = ToMlnSampleCountFlags(desc_.sampleCountFlags);
    plat_.tiling = ToMlnImageTiling(desc_.imageTiling);
    plat_.mipLevels = desc_.mipCount;
    plat_.arrayLayers = desc_.layerCount;

    if (mlnPlatData.resourceView) {
        // Pre-created view provided (e.g. from SwapchainMln) — use it directly, don't own it
        plat_.resourceView = mlnPlatData.resourceView;
        plat_.resourceViewBase = mlnPlatData.resourceView;
        ownsImageViews_ = false;
        MLN_LOG_INIT("GpuImageMln: using pre-created resourceView (ownsImageViews_=false)");
    } else {
        // No pre-created view — create our own
        CreateImageViews();
        MLN_LOG_INIT("GpuImageMln: created own resourceView (ownsImageViews_=true)");
    }
}

GpuImageMln::~GpuImageMln()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    if (ownsImageViews_) {
        for (auto view : platViews_.mipImageViews) {
            if (view) {
                MlnDestroyResourceView(mlnDevice, view);
            }
        }
        for (auto view : platViews_.mipImageAllLayerViews) {
            if (view) {
                MlnDestroyResourceView(mlnDevice, view);
            }
        }
        for (auto view : platViews_.layerImageViews) {
            if (view) {
                MlnDestroyResourceView(mlnDevice, view);
            }
        }
        if (plat_.resourceViewBase && plat_.resourceViewBase != plat_.resourceView) {
            MlnDestroyResourceView(mlnDevice, plat_.resourceViewBase);
        }
        if (plat_.resourceView) {
            MlnDestroyResourceView(mlnDevice, plat_.resourceView);
        }
    }
    if (ownsResources_) {
        if (plat_.memory) {
            MlnFreeMemory(mlnDevice, plat_.memory);
        }
        if (ownsImage_ && plat_.resource) {
            MlnDestroyResource(mlnDevice, plat_.resource);
        }
    }
}

void GpuImageMln::CreateImage()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    MlnImageDescriptor imgDesc{};
    imgDesc.extensionCount = 0;
    imgDesc.extensions = nullptr;
    imgDesc.flags = 0;
    imgDesc.imageType = ToMlnImageType(desc_.imageType);
    imgDesc.format = ToMlnFormat(desc_.format);
    imgDesc.size = {desc_.width, desc_.height, desc_.depth};
    imgDesc.mipLevels = desc_.mipCount;
    imgDesc.arrayLayers = desc_.layerCount;
    imgDesc.samples = ToMlnSampleCountFlags(desc_.sampleCountFlags);
    imgDesc.tiling = ToMlnImageTiling(desc_.imageTiling);
    imgDesc.usage = ToMlnImageUsageFlags(desc_.usageFlags);
    imgDesc.initialLayout = MLN_IMAGE_LAYOUT_UNDEFINED;

    // Cube compatible
    if (desc_.createFlags & CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) {
        imgDesc.flags |= MLN_IMAGE_DESCRIPTOR_CUBE_COMPATIBLE_BIT;
    }

    plat_.resource = MlnCreateImageResource(deviceMln.GetMlnDevice(), &imgDesc);
    if (!plat_.resource) {
        MLN_LOG_ERR("GpuImageMln: MlnCreateImageResource failed (%ux%ux%u, fmt=%u)",
            desc_.width,
            desc_.height,
            desc_.depth,
            static_cast<uint32_t>(desc_.format));
        return;
    }

    plat_.format = imgDesc.format;
    plat_.extent = imgDesc.size;
    plat_.imageType = imgDesc.imageType;
    plat_.usage = imgDesc.usage;
    plat_.samples = imgDesc.samples;
    plat_.tiling = imgDesc.tiling;
    plat_.mipLevels = imgDesc.mipLevels;
    plat_.arrayLayers = imgDesc.arrayLayers;
}

void GpuImageMln::CreateImageViews()
{
    if (!plat_.resource) {
        return;
    }
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    const MlnImageAspectFlags aspectFlags = GetImageAspectFlags(desc_.format);

    // Main image view (all mips, all layers)
    MlnImageViewDescriptor viewDesc{};
    viewDesc.extensionCount = 0;
    viewDesc.extensions = nullptr;
    viewDesc.flags = 0;
    viewDesc.imageResource = plat_.resource;
    viewDesc.viewType = ToMlnImageViewType(desc_.imageViewType);
    viewDesc.format = plat_.format;
    viewDesc.components.r = ToMlnComponentSwizzle(desc_.componentMapping.r);
    viewDesc.components.g = ToMlnComponentSwizzle(desc_.componentMapping.g);
    viewDesc.components.b = ToMlnComponentSwizzle(desc_.componentMapping.b);
    viewDesc.components.a = ToMlnComponentSwizzle(desc_.componentMapping.a);
    viewDesc.subresourceRange.aspectMask = aspectFlags;
    viewDesc.subresourceRange.baseMipLevel = 0;
    viewDesc.subresourceRange.levelCount = plat_.mipLevels;
    viewDesc.subresourceRange.baseArrayLayer = 0;
    viewDesc.subresourceRange.layerCount = plat_.arrayLayers;

    MLN_LOG_INIT("GpuImageMln::CreateImageViews: BEFORE MlnCreateImageResourceView "
                 "(viewType=%d, fmt=%d, aspect=0x%x, mips=%u, layers=%u)",
        static_cast<int>(viewDesc.viewType),
        static_cast<int>(viewDesc.format),
        static_cast<uint32_t>(viewDesc.subresourceRange.aspectMask),
        viewDesc.subresourceRange.levelCount,
        viewDesc.subresourceRange.layerCount);
    plat_.resourceView = MlnCreateImageResourceView(mlnDevice, &viewDesc);
    MLN_LOG_INIT("GpuImageMln::CreateImageViews: AFTER MlnCreateImageResourceView");
    if (!plat_.resourceView) {
        MLN_LOG_ERR("GpuImageMln: MlnCreateImageResourceView failed (fmt=%d, %ux%u)",
            static_cast<int>(plat_.format),
            plat_.extent.width,
            plat_.extent.height);
        return;
    }

    // Determine if we need per-mip/layer views (same condition as VK backend)
    const bool usageNeedsViews = (plat_.usage & (MLN_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | MLN_IMAGE_USAGE_STORAGE_BIT |
                                                    MLN_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)) != 0;
    const bool needsBaseView = plat_.mipLevels > 1 || plat_.arrayLayers > 1;
    // VK GetBaseImageViewType: 1D_ARRAY→1D, 2D_ARRAY→2D, CUBE_ARRAY→CUBE, CUBE→2D
    MlnImageViewType baseViewType = viewDesc.viewType;
    if (desc_.imageViewType == CORE_IMAGE_VIEW_TYPE_CUBE) {
        baseViewType = MLN_IMAGE_VIEW_TYPE_2D;
    } else if (desc_.imageViewType == CORE_IMAGE_VIEW_TYPE_2D_ARRAY) {
        baseViewType = MLN_IMAGE_VIEW_TYPE_2D;
    } else if (desc_.imageViewType == CORE_IMAGE_VIEW_TYPE_CUBE_ARRAY) {
        baseViewType = MLN_IMAGE_VIEW_TYPE_CUBE;
    } else if (desc_.imageViewType == CORE_IMAGE_VIEW_TYPE_1D_ARRAY) {
        baseViewType = MLN_IMAGE_VIEW_TYPE_1D;
    }

    // Base image view (mip 0, layer 0) for attachment use
    if (needsBaseView) {
        MlnImageViewDescriptor baseViewDesc = viewDesc;
        baseViewDesc.subresourceRange.levelCount = 1;
        baseViewDesc.subresourceRange.layerCount = 1;
        baseViewDesc.viewType = baseViewType;
        plat_.resourceViewBase = MlnCreateImageResourceView(mlnDevice, &baseViewDesc);
    } else {
        plat_.resourceViewBase = plat_.resourceView;
    }

    // Per-mip and per-layer views (matching VK backend's GpuImagePlatformDataViewsVk)
    if (usageNeedsViews || needsBaseView) {
        if (plat_.mipLevels > 1) {
            platViews_.mipImageViews.resize(plat_.mipLevels);
            if (plat_.arrayLayers > 1u) {
                platViews_.mipImageAllLayerViews.resize(plat_.mipLevels);
            }
            for (uint32_t mipIdx = 0; mipIdx < plat_.mipLevels; ++mipIdx) {
                MlnImageViewDescriptor mipViewDesc = viewDesc;
                mipViewDesc.viewType = baseViewType;
                mipViewDesc.subresourceRange.baseMipLevel = mipIdx;
                mipViewDesc.subresourceRange.levelCount = 1;
                mipViewDesc.subresourceRange.baseArrayLayer = 0;
                mipViewDesc.subresourceRange.layerCount = 1;
                platViews_.mipImageViews[mipIdx] = MlnCreateImageResourceView(mlnDevice, &mipViewDesc);
                if (!platViews_.mipImageViews[mipIdx]) {
                    MLN_LOG_ERR("GpuImageMln: mipImageViews[%u] creation failed (%ux%u fmt=%u)",
                        mipIdx,
                        plat_.extent.width,
                        plat_.extent.height,
                        static_cast<uint32_t>(plat_.format));
                }
                if (plat_.arrayLayers > 1u) {
                    MlnImageViewDescriptor mipAllLayerDesc = viewDesc;
                    mipAllLayerDesc.viewType = MLN_IMAGE_VIEW_TYPE_2D_ARRAY;
                    mipAllLayerDesc.subresourceRange.baseMipLevel = mipIdx;
                    mipAllLayerDesc.subresourceRange.levelCount = 1;
                    mipAllLayerDesc.subresourceRange.baseArrayLayer = 0;
                    mipAllLayerDesc.subresourceRange.layerCount = plat_.arrayLayers;
                    platViews_.mipImageAllLayerViews[mipIdx] = MlnCreateImageResourceView(mlnDevice, &mipAllLayerDesc);
                    if (!platViews_.mipImageAllLayerViews[mipIdx]) {
                        MLN_LOG_ERR("GpuImageMln: mipImageAllLayerViews[%u] creation failed (%ux%u fmt=%u layers=%u)",
                            mipIdx,
                            plat_.extent.width,
                            plat_.extent.height,
                            static_cast<uint32_t>(plat_.format),
                            plat_.arrayLayers);
                    }
                }
            }
        }
        if (plat_.arrayLayers > 1) {
            platViews_.layerImageViews.resize(plat_.arrayLayers);
            for (uint32_t layerIdx = 0; layerIdx < plat_.arrayLayers; ++layerIdx) {
                MlnImageViewDescriptor layerViewDesc = viewDesc;
                layerViewDesc.viewType = baseViewType;
                layerViewDesc.subresourceRange.baseMipLevel = 0;
                layerViewDesc.subresourceRange.levelCount = 1;
                layerViewDesc.subresourceRange.baseArrayLayer = layerIdx;
                layerViewDesc.subresourceRange.layerCount = 1;
                platViews_.layerImageViews[layerIdx] = MlnCreateImageResourceView(mlnDevice, &layerViewDesc);
                if (!platViews_.layerImageViews[layerIdx]) {
                    MLN_LOG_ERR("GpuImageMln: layerImageViews[%u] creation failed (%ux%u fmt=%u)",
                        layerIdx,
                        plat_.extent.width,
                        plat_.extent.height,
                        static_cast<uint32_t>(plat_.format));
                }
            }
        }
        // Cube maps also need mipImageAllLayerViews even if arrayLayers == 6
        if (desc_.imageViewType == CORE_IMAGE_VIEW_TYPE_CUBE && platViews_.mipImageAllLayerViews.empty()) {
            platViews_.mipImageAllLayerViews.resize(plat_.mipLevels);
            for (uint32_t mipIdx = 0; mipIdx < plat_.mipLevels; ++mipIdx) {
                MlnImageViewDescriptor mipAllLayerDesc = viewDesc;
                mipAllLayerDesc.viewType = MLN_IMAGE_VIEW_TYPE_2D_ARRAY;
                mipAllLayerDesc.subresourceRange.baseMipLevel = mipIdx;
                mipAllLayerDesc.subresourceRange.levelCount = 1;
                mipAllLayerDesc.subresourceRange.baseArrayLayer = 0;
                mipAllLayerDesc.subresourceRange.layerCount = plat_.arrayLayers;
                platViews_.mipImageAllLayerViews[mipIdx] = MlnCreateImageResourceView(mlnDevice, &mipAllLayerDesc);
                if (!platViews_.mipImageAllLayerViews[mipIdx]) {
                    MLN_LOG_ERR("GpuImageMln: cube mipImageAllLayerViews[%u] creation failed", mipIdx);
                }
            }
        }
    }
}

void GpuImageMln::AllocateAndBindMemory()
{
    if (!plat_.resource) {
        return;
    }
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    MlnMemoryRequirements memReqs{};
    MlnResourceMemoryRequirementsDescriptor memReqDesc{};
    memReqDesc.extensionCount = 0;
    memReqDesc.extensions = nullptr;
    memReqDesc.resource = plat_.resource;
    memReqDesc.aspectMask = GetImageAspectFlags(desc_.format);
    MlnGetResourceMemoryRequirements(mlnDevice, &memReqDesc, &memReqs);

    const MlnMemoryPropertyFlags requiredFlags = ToMlnMemoryPropertyFlags(desc_.memoryPropertyFlags);
    const uint32_t memTypeIndex = FindMemoryType(memReqs.memoryTypeBits, requiredFlags);

    MlnMemoryAllocateDescriptor allocDesc{};
    allocDesc.extensionCount = 0;
    allocDesc.extensions = nullptr;
    allocDesc.allocationSize = memReqs.size;
    allocDesc.memoryTypeIndex = memTypeIndex;

    plat_.memory = MlnAllocateMemory(mlnDevice, &allocDesc);
    if (!plat_.memory) {
        MLN_LOG_ERR("GpuImageMln: MlnAllocateMemory failed (size=%llu)", static_cast<unsigned long long>(memReqs.size));
        return;
    }

    MlnBindResourceMemoryDescriptor bindDesc{};
    bindDesc.extensionCount = 0;
    bindDesc.extensions = nullptr;
    bindDesc.resource = plat_.resource;
    bindDesc.aspectMask = GetImageAspectFlags(desc_.format);
    bindDesc.memory = plat_.memory;
    bindDesc.offset = 0;
    MlnStatus result = MlnBindResourceMemory(mlnDevice, &bindDesc);
    if (result != MLN_STATUS_SUCCESS) {
        MLN_LOG_ERR("GpuImageMln: MlnBindResourceMemory failed");
    }
}

uint32_t GpuImageMln::FindMemoryType(uint32_t memoryTypeBits, MlnMemoryPropertyFlags requiredFlags) const
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    const auto& memProps = deviceMln.GetCachedMemoryProperties();

    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if ((memoryTypeBits & (1u << i)) && (memProps.memoryTypes[i].propertyFlags & requiredFlags) == requiredFlags) {
            return i;
        }
    }
    for (uint32_t i = 0; i < memProps.memoryTypeCount; ++i) {
        if (memoryTypeBits & (1u << i)) {
            return i;
        }
    }
    MLN_LOG_ERR("GpuImageMln: no suitable memory type found");
    return 0;
}

const GpuImageDesc& GpuImageMln::GetDesc() const
{
    return desc_;
}

const GpuImagePlatformData& GpuImageMln::GetBasePlatformData() const
{
    return plat_;
}

GpuImage::AdditionalFlags GpuImageMln::GetAdditionalFlags() const
{
    return 0u;
}

const GpuImagePlatformDataMln& GpuImageMln::GetPlatformData() const
{
    return plat_;
}

const GpuImagePlatformDataViewsMln& GpuImageMln::GetPlatformDataViews() const
{
    return platViews_;
}

RENDER_END_NAMESPACE()
