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

#include "swapchain_mln.h"
#include "mln_log.h"
#include "util/log.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "maleoon/device_mln.h"
#include "mln_convert.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

SwapchainMln::SwapchainMln(Device& device, const SwapchainCreateInfo& swapchainCreateInfo) : device_(device)
{
    flags_ = swapchainCreateInfo.swapchainFlags;

    if ((swapchainCreateInfo.surfaceHandle == 0) && swapchainCreateInfo.window.window) {
        CreateDisplaySurface(swapchainCreateInfo);
    }
    if (plat_.displaySurface) {
        surfaceHandle_ = reinterpret_cast<uint64_t>(plat_.displaySurface);
    } else {
        surfaceHandle_ = swapchainCreateInfo.surfaceHandle;
        plat_.displaySurface = reinterpret_cast<MlnDisplaySurface>(surfaceHandle_);
    }
    if (plat_.displaySurface) {
        CreateSwapchain(swapchainCreateInfo);
    }
    if (plat_.swapchain) {
        GetSwapchainImages();
        valid_ = (plat_.imageCount > 0);
    }

    if (valid_) {
        MLN_LOG_INIT("SwapchainMln: created (%ux%u, images=%u)", desc_.width, desc_.height, plat_.imageCount);
    } else {
        MLN_LOG_ERR("SwapchainMln: creation failed");
    }
}

SwapchainMln::~SwapchainMln()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnDevice mlnDevice = deviceMln.GetMlnDevice();

    for (auto const imageView : plat_.imageViews) {
        if (imageView) {
            MlnDestroyResourceView(mlnDevice, imageView);
        }
    }
    if (plat_.swapchain) {
        MlnDestroySwapchain(mlnDevice, plat_.swapchain);
    }
    if (plat_.displaySurface) {
        MlnDestroyDisplaySurface(mlnDevice, plat_.displaySurface);
    }
}

void SwapchainMln::CreateDisplaySurface(const SwapchainCreateInfo& createInfo)
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    MlnDisplaySurfaceDescriptor dsDesc{};
    dsDesc.extensionCount = 0;
    dsDesc.extensions = nullptr;
    // New API: nativeWindow is OHNativeWindow* (same underlying type, just pointer cast)
    dsDesc.nativeWindow = reinterpret_cast<OHNativeWindow*>(createInfo.window.window);

    MLN_LOG_INIT("BEFORE MlnCreateDisplaySurface");
    plat_.displaySurface = MlnCreateDisplaySurface(deviceMln.GetMlnDevice(), &dsDesc);
    MLN_LOG_INIT("AFTER  MlnCreateDisplaySurface");
    if (!plat_.displaySurface) {
        MLN_LOG_ERR("SwapchainMln: MlnCreateDisplaySurface FAILED");
    }
}

void SwapchainMln::CreateSwapchain(const SwapchainCreateInfo& createInfo)
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    const auto& config = device_.GetDeviceConfiguration();

    // Query capabilities to get actual width/height (new API: MlnDisplaySurfaceCapabilities)
    MlnDisplaySurfaceCapabilities caps{};
    MlnStatus capsResult = MlnGetDisplaySurfaceCapabilities(deviceMln.GetMlnDevice(), plat_.displaySurface, &caps);
    if (capsResult != MLN_STATUS_SUCCESS) {
        MLN_LOG_ERR("SwapchainMln: MlnGetDisplaySurfaceCapabilities FAILED (status=%d)", static_cast<int>(capsResult));
        return;
    }
    MLN_LOG_INIT("caps: extent=%ux%u, minImg=%u, maxImg=%u, currentTransform=0x%x",
        caps.currentExtent.width,
        caps.currentExtent.height,
        caps.minImageCount,
        caps.maxImageCount,
        static_cast<uint32_t>(caps.currentTransform));

    desc_.imageType = CORE_IMAGE_TYPE_2D;
    desc_.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    desc_.width = caps.currentExtent.width;
    desc_.height = caps.currentExtent.height;
    desc_.depth = 1;
    desc_.mipCount = 1;
    desc_.layerCount = 1;
    desc_.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
    desc_.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
    desc_.usageFlags = createInfo.imageUsageFlags;
    desc_.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    desc_.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS |
                                EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;

    // Determine format — query display surface supported formats (mirrors Vulkan GetColorFormat).
    // Priority: HDR formats (if HDR flag) > SRGB > non-SRGB > fallback.
    {
        const bool wantHdr = (createInfo.swapchainFlags & CORE_SWAPCHAIN_HDR_BIT) != 0;
        const bool wantSrgb = (createInfo.swapchainFlags & CORE_SWAPCHAIN_SRGB_BIT) != 0;

        // Query supported surface formats
        u32 fmtCount = 0;
        MlnStatus fmtResult =
            MlnGetDisplaySurfaceFormats(deviceMln.GetMlnDevice(), plat_.displaySurface, &fmtCount, nullptr);
        BASE_NS::vector<MlnDisplaySurfaceFormat> surfaceFormats;
        if (fmtResult == MLN_STATUS_SUCCESS && fmtCount > 0) {
            surfaceFormats.resize(fmtCount);
            MlnGetDisplaySurfaceFormats(
                deviceMln.GetMlnDevice(), plat_.displaySurface, &fmtCount, surfaceFormats.data());
        }

        // HDR preferred formats (same order as Vulkan backend)
        constexpr Format hdrFormats[] = {
            Format::BASE_FORMAT_A2B10G10R10_UNORM_PACK32,
            Format::BASE_FORMAT_A2R10G10B10_UNORM_PACK32,
            Format::BASE_FORMAT_R16G16B16A16_SFLOAT,
        };
        // SRGB preferred formats
        constexpr Format srgbFormats[] = {
            Format::BASE_FORMAT_R8G8B8A8_SRGB,
            Format::BASE_FORMAT_B8G8R8A8_SRGB,
            Format::BASE_FORMAT_R8G8B8A8_UNORM,
            Format::BASE_FORMAT_B8G8R8A8_UNORM,
        };
        // Non-SRGB preferred formats
        constexpr Format nonSrgbFormats[] = {
            Format::BASE_FORMAT_R8G8B8A8_UNORM,
            Format::BASE_FORMAT_B8G8R8A8_UNORM,
            Format::BASE_FORMAT_R8G8B8A8_SRGB,
            Format::BASE_FORMAT_B8G8R8A8_SRGB,
        };

        auto findFormat = [&surfaceFormats](const Format* candidates, uint32_t count) -> Format {
            for (uint32_t i = 0; i < count; ++i) {
                const MlnFormat mlnFmt = ToMlnFormat(candidates[i]);
                for (const auto& sf : surfaceFormats) {
                    if (sf.format == mlnFmt) {
                        return candidates[i];
                    }
                }
            }
            return Format::BASE_FORMAT_UNDEFINED;
        };

        Format selectedFormat = Format::BASE_FORMAT_UNDEFINED;

        // Try HDR first if requested
        if (wantHdr) {
            selectedFormat = findFormat(hdrFormats, sizeof(hdrFormats) / sizeof(hdrFormats[0]));
            if (selectedFormat != Format::BASE_FORMAT_UNDEFINED) {
                MLN_LOG_INIT("Swapchain HDR format selected: %u", static_cast<uint32_t>(selectedFormat));
            }
        }

        // Fall back to SRGB / non-SRGB
        if (selectedFormat == Format::BASE_FORMAT_UNDEFINED) {
            const Format* candidates = wantSrgb ? srgbFormats : nonSrgbFormats;
            const uint32_t candidateCount = sizeof(srgbFormats) / sizeof(srgbFormats[0]);
            selectedFormat = findFormat(candidates, candidateCount);
        }

        // Ultimate fallback (surface format query unavailable or no match)
        if (selectedFormat == Format::BASE_FORMAT_UNDEFINED) {
            selectedFormat = wantSrgb ? Format::BASE_FORMAT_R8G8B8A8_SRGB : Format::BASE_FORMAT_R8G8B8A8_UNORM;
            MLN_LOG_INIT("Swapchain format: using fallback %u (query returned %u formats)",
                static_cast<uint32_t>(selectedFormat),
                fmtCount);
        }

        desc_.format = selectedFormat;
    }

    // Depth buffer desc
    if (createInfo.swapchainFlags & CORE_SWAPCHAIN_DEPTH_BUFFER_BIT) {
        descDepthBuffer_.imageType = CORE_IMAGE_TYPE_2D;
        descDepthBuffer_.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
        descDepthBuffer_.format = Format::BASE_FORMAT_D24_UNORM_S8_UINT;
        descDepthBuffer_.width = caps.currentExtent.width;
        descDepthBuffer_.height = caps.currentExtent.height;
        descDepthBuffer_.depth = 1;
        descDepthBuffer_.mipCount = 1;
        descDepthBuffer_.layerCount = 1;
        descDepthBuffer_.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
        descDepthBuffer_.imageTiling = CORE_IMAGE_TILING_OPTIMAL;
        descDepthBuffer_.usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        descDepthBuffer_.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    // Maleoon 0324: only MLN_PRESENT_MODE_FIFO is defined (MAILBOX removed from spec)
    MlnPresentMode presentMode = MLN_PRESENT_MODE_FIFO;

    // New API: MlnSwapchainDescriptor completely restructured
    MlnDisplaySurfaceFormat surfaceFormat{};
    surfaceFormat.format = ToMlnFormat(desc_.format);
    surfaceFormat.colorSpace = MLN_COLOR_SPACE_SRGB_NONLINEAR;

    // Clamp minImageCount to [caps.minImageCount, caps.maxImageCount]
    uint32_t imageCount = config.swapchainImageCount;
    if (imageCount < caps.minImageCount) {
        imageCount = caps.minImageCount;
    }
    if (caps.maxImageCount > 0 && imageCount > caps.maxImageCount) {
        imageCount = caps.maxImageCount;
    }

    MlnSwapchainDescriptor scDesc{};
    scDesc.extensionCount = 0;
    scDesc.extensions = nullptr;
    scDesc.flags = 0;
    scDesc.displaySurface = plat_.displaySurface;
    scDesc.minImageCount = imageCount;
    scDesc.surfaceFormat = &surfaceFormat;
    scDesc.imageExtent = {desc_.width, desc_.height};
    scDesc.imageArrayLayers = 1;
    scDesc.imageUsage = ToMlnImageUsageFlags(createInfo.imageUsageFlags);
    scDesc.preTransform = MLN_DISPLAY_SURFACE_TRANSFORM_IDENTITY_BIT;
    scDesc.compositeAlpha = MLN_COMPOSITE_ALPHA_INHERIT_BIT;
    scDesc.presentMode = presentMode;
    scDesc.clipped = 1;

    MLN_LOG_INIT("BEFORE MlnCreateSwapchain: %ux%u, imgs=%u(req=%u), fmt=%d, "
                 "preTransform=0x%x, compositeAlpha=0x%x, presentMode=%d, usage=0x%x",
        desc_.width,
        desc_.height,
        imageCount,
        config.swapchainImageCount,
        static_cast<int>(surfaceFormat.format),
        static_cast<uint32_t>(scDesc.preTransform),
        static_cast<uint32_t>(scDesc.compositeAlpha),
        static_cast<int>(presentMode),
        static_cast<uint32_t>(scDesc.imageUsage));
    plat_.swapchain = MlnCreateSwapchain(deviceMln.GetMlnDevice(), &scDesc);
}

void SwapchainMln::GetSwapchainImages()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    uint32_t imageCount = 0;
    MlnStatus status1 = MlnGetSwapchainImages(deviceMln.GetMlnDevice(), plat_.swapchain, &imageCount, nullptr);
    MLN_LOG_INIT("AFTER  MlnGetSwapchainImages query: status=%d, count=%u", static_cast<int>(status1), imageCount);
    if (status1 != MLN_STATUS_SUCCESS || imageCount == 0) {
        MLN_LOG_ERR("SwapchainMln: MlnGetSwapchainImages query FAILED (status=%d, count=%u)",
            static_cast<int>(status1),
            imageCount);
        return;
    }

    plat_.images.resize(imageCount);
    MLN_LOG_INIT("BEFORE MlnGetSwapchainImages (fetch %u images)", imageCount);
    MlnStatus status2 =
        MlnGetSwapchainImages(deviceMln.GetMlnDevice(), plat_.swapchain, &imageCount, plat_.images.data());
    MLN_LOG_INIT("AFTER  MlnGetSwapchainImages fetch: status=%d, count=%u", static_cast<int>(status2), imageCount);
    if (status2 != MLN_STATUS_SUCCESS) {
        MLN_LOG_ERR("SwapchainMln: MlnGetSwapchainImages fetch FAILED (status=%d)", static_cast<int>(status2));
        return;
    }
    plat_.imageCount = imageCount;

    // Pre-create MlnImageResourceView for each swapchain image (same pattern as VK backend).
    // This avoids redundant view creation/destruction in temporary GpuImageMln objects.
    MlnDevice mlnDev = deviceMln.GetMlnDevice();
    plat_.imageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; ++i) {
        MlnImageViewDescriptor viewDesc{};
        viewDesc.extensionCount = 0;
        viewDesc.extensions = nullptr;
        viewDesc.flags = 0;
        viewDesc.imageResource = plat_.images[i];
        viewDesc.viewType = MLN_IMAGE_VIEW_TYPE_2D;
        viewDesc.format = ToMlnFormat(desc_.format);
        viewDesc.components.r = MLN_COMPONENT_SWIZZLE_IDENTITY;
        viewDesc.components.g = MLN_COMPONENT_SWIZZLE_IDENTITY;
        viewDesc.components.b = MLN_COMPONENT_SWIZZLE_IDENTITY;
        viewDesc.components.a = MLN_COMPONENT_SWIZZLE_IDENTITY;
        viewDesc.subresourceRange.aspectMask = MLN_IMAGE_ASPECT_COLOR_BIT;
        viewDesc.subresourceRange.baseMipLevel = 0;
        viewDesc.subresourceRange.levelCount = 1;
        viewDesc.subresourceRange.baseArrayLayer = 0;
        viewDesc.subresourceRange.layerCount = 1;

        plat_.imageViews[i] = MlnCreateImageResourceView(mlnDev, &viewDesc);
    }
}

const GpuImageDesc& SwapchainMln::GetDesc() const
{
    return desc_;
}

const GpuImageDesc& SwapchainMln::GetDescDepthBuffer() const
{
    return descDepthBuffer_;
}

uint32_t SwapchainMln::GetFlags() const
{
    return flags_;
}

SurfaceTransformFlags SwapchainMln::GetSurfaceTransformFlags() const
{
    return surfaceTransformFlags_;
}

uint64_t SwapchainMln::GetSurfaceHandle() const
{
    return surfaceHandle_;
}

bool SwapchainMln::IsValid() const
{
    return valid_;
}

const SwapchainPlatformDataMln& SwapchainMln::GetPlatformData() const
{
    return plat_;
}

MlnStatus SwapchainMln::AcquireNextImage(uint32_t& imageIndex, MlnTimeline signalTimeline, uint64_t signalValue)
{
    MLN_LOG_FRAME("SwapchainMln::AcquireNextImage (signalValue=%llu)", static_cast<unsigned long long>(signalValue));
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    MlnStatus result = MlnAcquireNextImage(
        deviceMln.GetMlnDevice(), plat_.swapchain, UINT64_MAX, signalTimeline, signalValue, &imageIndex);
    if (result == MLN_STATUS_SUCCESS || result == MLN_STATUS_SUBOPTIMAL) {
        if (result == MLN_STATUS_SUBOPTIMAL) {
            MLN_LOG_FRAME("SwapchainMln::AcquireNextImage: SUBOPTIMAL (swapchain usable but not optimal)");
        }
        plat_.currentImageIndex = imageIndex;
    } else if (result == MLN_STATUS_OUT_OF_DATE) {
        MLN_LOG_ERR("SwapchainMln::AcquireNextImage: OUT_OF_DATE (swapchain needs recreation)");
    }
    return result;
}

MlnStatus SwapchainMln::Present(uint32_t imageIndex, MlnTimeline waitTimeline, uint64_t waitValue)
{
    MLN_LOG_FRAME("SwapchainMln::Present (imgIdx=%u)", imageIndex);
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    MlnPresentDescriptor presentDesc{};
    presentDesc.swapchain = plat_.swapchain;
    presentDesc.imageIndex = imageIndex;

    if (waitTimeline != MLN_NULL_HANDLE) {
        presentDesc.waitTimelines = &waitTimeline;
        presentDesc.waitValues = &waitValue;
        presentDesc.waitCount = 1;
    } else {
        presentDesc.waitTimelines = nullptr;
        presentDesc.waitValues = nullptr;
        presentDesc.waitCount = 0;
    }
    presentDesc.presentTimeline = nullptr;
    presentDesc.presentValue = 0;

    return MlnQueuePresentSwapchain(deviceMln.GetMlnQueue(), &presentDesc);
}

RENDER_END_NAMESPACE()
