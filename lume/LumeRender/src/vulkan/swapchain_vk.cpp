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

#include "swapchain_vk.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <base/util/formats.h>
#include <core/intf_engine.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "util/log.h"
#include "vulkan/create_functions_vk.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
Format GetValidDepthFormat(const DeviceVk& deviceVk)
{
    constexpr uint32_t PREFERRED_FORMAT_COUNT { 3 };
    constexpr Format preferredFormats[PREFERRED_FORMAT_COUNT] = { BASE_FORMAT_D24_UNORM_S8_UINT, BASE_FORMAT_D32_SFLOAT,
        BASE_FORMAT_D16_UNORM };
#ifndef NDEBUG
    constexpr string_view PREFERRED_FORMAT_NAMES[PREFERRED_FORMAT_COUNT] = { "BASE_FORMAT_D24_UNORM_S8_UINT",
        "BASE_FORMAT_D32_SFLOAT", "BASE_FORMAT_D16_UNORM" };
#endif
    Format finalFormat = BASE_FORMAT_UNDEFINED;
    const auto& devPlat = deviceVk.GetPlatformInternalDataVk();
    for (uint32_t idx = 0; idx < PREFERRED_FORMAT_COUNT; ++idx) {
        finalFormat = preferredFormats[idx];
        for (const auto& supportedDepthFormat : devPlat.supportedDepthFormats) {
            if (finalFormat == supportedDepthFormat) {
#ifndef NDEBUG
                PLUGIN_LOG_D(
                    "selected CORE_DEFAULT_BACKBUFFER format: %s", string(PREFERRED_FORMAT_NAMES[idx]).c_str());
#endif
                idx = PREFERRED_FORMAT_COUNT;
                break;
            }
        }
    }
    return finalFormat;
}

struct ColorInfo {
    VkFormat format { VK_FORMAT_UNDEFINED };
    VkColorSpaceKHR colorSpace { VK_COLOR_SPACE_MAX_ENUM_KHR };
};

VkFormat GetColorFormat(const uint32_t flags, const vector<VkSurfaceFormatKHR>& surfaceFormats)
{
    constexpr uint32_t preferredFormatCount { 4u };
    constexpr VkFormat srgbFormats[preferredFormatCount] = { VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB,
        VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM };
    constexpr VkFormat nonSrgbFormats[preferredFormatCount] = { VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8A8_UNORM,
        VK_FORMAT_R8G8B8A8_SRGB, VK_FORMAT_B8G8R8A8_SRGB };

    const bool preferSrgbFormat = (flags & SwapchainFlagBits::CORE_SWAPCHAIN_SRGB_BIT);
    const array_view<const VkFormat> formats =
        (preferSrgbFormat) ? array_view<const VkFormat> { srgbFormats, preferredFormatCount }
                           : array_view<const VkFormat> { nonSrgbFormats, preferredFormatCount };

    // If pSurfaceFormats includes just one entry, whose value for format is VK_FORMAT_UNDEFINED,
    // surface has no preferred format. In this case, the application can use any valid VkFormat value.
    if (surfaceFormats[0].format == VK_FORMAT_UNDEFINED && surfaceFormats.size() == 1) {
        return VK_FORMAT_R8G8B8A8_SRGB;
    }

    for (size_t idx = 0; idx < formats.size(); ++idx) {
        for (size_t jdx = 0; jdx < surfaceFormats.size(); ++jdx) {
            if (formats[idx] == surfaceFormats[jdx].format) {
                return surfaceFormats[jdx].format;
            }
        }
    }
    return VK_FORMAT_UNDEFINED;
}

ColorInfo GetColorInfo(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface, const uint32_t flags)
{
    // Pick a color format for the swapchain.
    uint32_t surfaceFormatsCount = 0;
    VALIDATE_VK_RESULT(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, nullptr));

    vector<VkSurfaceFormatKHR> surfaceFormats(surfaceFormatsCount);
    VALIDATE_VK_RESULT(
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &surfaceFormatsCount, surfaceFormats.data()));

    ColorInfo ci;
    ci.format = GetColorFormat(flags, surfaceFormats);
    ci.colorSpace = VK_COLOR_SPACE_MAX_ENUM_KHR;
    for (size_t idx = 0; idx < surfaceFormats.size(); ++idx) {
        if (surfaceFormats[idx].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            ci.colorSpace = surfaceFormats[idx].colorSpace;
            break;
        }
    }
    PLUGIN_ASSERT_MSG(ci.colorSpace != VK_COLOR_SPACE_MAX_ENUM_KHR, "colorspace not correct");

    PLUGIN_ASSERT_MSG(ci.format != VK_FORMAT_UNDEFINED, "colorformat not correct");
    PLUGIN_LOG_D("swapchainColorFormat: %u swapchainColorSpace %u", ci.format, ci.colorSpace);

    return ci;
}

VkPresentModeKHR GetPresentMode(const VkPhysicalDevice physicalDevice, const VkSurfaceKHR surface, const uint32_t flags)
{
    // Pick a present mode for the swapchain.
    uint32_t presentModeCount;
    VALIDATE_VK_RESULT(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr));

    vector<VkPresentModeKHR> presentModes(presentModeCount);
    VALIDATE_VK_RESULT(
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, presentModes.data()));

    VkPresentModeKHR swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR; // FIFO must be supported by the specification.
    if ((flags & SwapchainFlagBits::CORE_SWAPCHAIN_VSYNC_BIT) != SwapchainFlagBits::CORE_SWAPCHAIN_VSYNC_BIT) {
        // immediate is really without vsync, but it might not be supported, so we also check for mailbox.
        if (std::any_of(presentModes.cbegin(), presentModes.cend(),
                [](const VkPresentModeKHR& supported) { return supported == VK_PRESENT_MODE_IMMEDIATE_KHR; })) {
            swapchainPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        } else if (std::any_of(presentModes.cbegin(), presentModes.cend(),
                       [](const VkPresentModeKHR& supported) { return supported == VK_PRESENT_MODE_MAILBOX_KHR; })) {
            swapchainPresentMode = VK_PRESENT_MODE_MAILBOX_KHR;
        }
    }

#if (RENDER_DEV_ENABLED == 1)
    constexpr uint32_t strArraySize { 4 };
    constexpr string_view presentModeStrings[strArraySize] = {
        "VK_PRESENT_MODE_IMMEDIATE_KHR",
        "VK_PRESENT_MODE_MAILBOX_KHR",
        "VK_PRESENT_MODE_FIFO_KHR",
        "VK_PRESENT_MODE_FIFO_RELAXED_KHR",
    };

    PLUGIN_LOG_I("Available swapchain present modes:");
    for (auto const presentMode : presentModes) {
        if ((uint32_t)presentMode < strArraySize) {
            PLUGIN_LOG_I("  %s", presentModeStrings[presentMode].data());
        }
    }
    PLUGIN_LOG_I("Selected swapchain present modes:");
    if ((uint32_t)swapchainPresentMode < strArraySize) {
        PLUGIN_LOG_I("  %s", presentModeStrings[swapchainPresentMode].data());
    }
#else
    PLUGIN_LOG_D("swapchainPresentMode: %x", swapchainPresentMode);
#endif

    return swapchainPresentMode;
}

void ClampSwapchainExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities, VkExtent2D& extent)
{
    extent.width =
        std::clamp(extent.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width);
    extent.height =
        std::clamp(extent.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height);

    PLUGIN_LOG_D("swapchainExtent: %u x %u", extent.width, extent.height);
    if ((extent.width == 0) || (extent.height == 0)) {
        PLUGIN_LOG_E(
            "zero sized swapchain cannot be created in vulkan (width: %u, height: %u)", extent.width, extent.height);
        PLUGIN_LOG_E("using 1x1 swapchain");
        PLUGIN_ASSERT(false);
        extent.width = 1;
        extent.height = 1;
    }
}

constexpr GpuImageDesc GetColorDesc(
    const uint32_t width, const uint32_t height, const Format format, const ImageUsageFlags imageUsageFlags)
{
    return {
        ImageType::CORE_IMAGE_TYPE_2D,                                 // imageType
        ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,                        // imageViewType
        format,                                                        // format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,                        // imageTiling
        imageUsageFlags,                                               // usageFlags
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // memoryPropertyFlags
        0,                                                             // createFlags
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS |
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS, // engineCreationFlags
        width,                                                                                    // width
        height,                                                                                   // height
        1,                                                                                        // depth
        1,                                                                                        // mipCount
        1,                                                                                        // layerCount
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,                                             // sampleCountFlags
        {},                                                                                       // componentMapping
    };
}

constexpr GpuImageDesc GetDepthDesc(const uint32_t width, const uint32_t height, const Format format)
{
    return {
        ImageType::CORE_IMAGE_TYPE_2D,          // imageType
        ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, // imageViewType
        format,                                 // format
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL, // imageTiling
        ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT, // usageFlags
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,    // memoryPropertyFlags
        0,                                                                        // createFlags
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, // engineCreationFlags
        width,                                                                    // width
        height,                                                                   // height
        1,                                                                        // depth
        1,                                                                        // mipCount
        1,                                                                        // layerCount
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,                             // sampleCountFlags
        {},                                                                       // componentMapping
    };
}
} // namespace

SwapchainVk::SwapchainVk(Device& device, const SwapchainCreateInfo& swapchainCreateInfo)
    : device_(device), flags_(swapchainCreateInfo.swapchainFlags)
{
    const auto& devicePlatformData = (const DevicePlatformDataVk&)device_.GetPlatformData();
    auto const physicalDevice = devicePlatformData.physicalDevice;
    // check for surface creation automatically
    if ((swapchainCreateInfo.surfaceHandle == 0) && swapchainCreateInfo.window.window) {
        CreateFunctionsVk::Window win;
        win.instance = swapchainCreateInfo.window.instance;
        win.window = swapchainCreateInfo.window.window;
        surface_ = CreateFunctionsVk::CreateSurface(devicePlatformData.instance, win);
        ownsSurface_ = true;
    } else {
        surface_ = VulkanHandleCast<VkSurfaceKHR>(swapchainCreateInfo.surfaceHandle);
    }

    if (surface_ != VK_NULL_HANDLE) {
        auto const vkDevice = devicePlatformData.device;

        // Sanity check that the device can use the surface.
        // NOTE: queuFamilyIndex hardcoded, should come via devicePlatformData?
        VkBool32 surfaceSupported = VK_FALSE;
        VALIDATE_VK_RESULT(vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, 0, surface_, &surfaceSupported));
        PLUGIN_ASSERT_MSG(surfaceSupported != VK_FALSE, "physicalDevice doesn't support given surface");

        const ColorInfo ci = GetColorInfo(physicalDevice, surface_, flags_);

        const VkPresentModeKHR swapchainPresentMode = GetPresentMode(physicalDevice, surface_, flags_);
        // Pick an extent, image count, and transform for the swapchain.
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        VALIDATE_VK_RESULT(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface_, &surfaceCapabilities));

        // NOTE: how do we handle the special case of 0xffffffffff which means the extent should be defined by the
        // swapchain?
        VkExtent2D swapchainExtent = surfaceCapabilities.currentExtent;
        ClampSwapchainExtent(surfaceCapabilities, swapchainExtent);
        plat_.swapchainImages.width = swapchainExtent.width;
        plat_.swapchainImages.height = swapchainExtent.height;

        const DeviceConfiguration deviceConfig = device_.GetDeviceConfiguration();
        // surfaceCapabilities.maxImageCount of zero means that there is no limit
        const uint32_t imageCount =
            (surfaceCapabilities.maxImageCount == 0)
                ? (Math::max(surfaceCapabilities.minImageCount, deviceConfig.swapchainImageCount))
                : (Math::min(surfaceCapabilities.maxImageCount,
                    Math::max(surfaceCapabilities.minImageCount, deviceConfig.swapchainImageCount)));
        PLUGIN_LOG_D("swapchainImageCount: %u", imageCount);

        const VkSurfaceTransformFlagsKHR swapchainTransform =
            (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
                ? VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR
                : surfaceCapabilities.currentTransform;

        const VkImageUsageFlags desiredUsageFlags = static_cast<VkImageUsageFlags>(swapchainCreateInfo.imageUsageFlags);
        const VkImageUsageFlags imageUsageFlags = desiredUsageFlags & surfaceCapabilities.supportedUsageFlags;
        PLUGIN_LOG_D("swapchain usage flags, selected: %u, desired: %u, capabilities: %u", imageUsageFlags,
            desiredUsageFlags, surfaceCapabilities.supportedUsageFlags);

        VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
        if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
            compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        }

        VkSwapchainCreateInfoKHR const vkSwapchainCreateInfo {
            VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,       // sType
            nullptr,                                           // pNext
            0,                                                 // flags
            surface_,                                          // surface
            imageCount,                                        // minImageCount
            ci.format,                                         // imageFormat
            ci.colorSpace,                                     // imageColorSpace
            swapchainExtent,                                   // imageExtent
            1,                                                 // imageArrayLayers
            imageUsageFlags,                                   // imageUsage
            VK_SHARING_MODE_EXCLUSIVE,                         // imageSharingMode
            0,                                                 // queueFamilyIndexCount
            nullptr,                                           // pQueueFamilyIndices
            (VkSurfaceTransformFlagBitsKHR)swapchainTransform, // preTransform
            compositeAlpha,                                    // compositeAlpha
            swapchainPresentMode,                              // presentMode
            VK_TRUE,                                           // clipped
            VK_NULL_HANDLE,                                    // oldSwapchain
        };

        VALIDATE_VK_RESULT(vkCreateSwapchainKHR(vkDevice, &vkSwapchainCreateInfo, nullptr, &plat_.swapchain));

        {
            uint32_t realImageCount = 0;
            VALIDATE_VK_RESULT(vkGetSwapchainImagesKHR(vkDevice, // device
                plat_.swapchain,                                 // swapchain
                &realImageCount,                                 // pSwapchainImageCount
                nullptr));                                       // pSwapchainImages

            PLUGIN_LOG_D("swapchain realImageCount: %u", realImageCount);

            plat_.swapchainImages.images.resize(realImageCount);
            plat_.swapchainImages.imageViews.resize(realImageCount);
            plat_.swapchainImages.semaphores.resize(realImageCount);

            VALIDATE_VK_RESULT(vkGetSwapchainImagesKHR(vkDevice, // device
                plat_.swapchain,                                 // swapchain
                &realImageCount,                                 // pSwapchainImageCount
                plat_.swapchainImages.images.data()));           // pSwapchainImages

            constexpr VkComponentMapping componentMapping {
                VK_COMPONENT_SWIZZLE_IDENTITY, // r
                VK_COMPONENT_SWIZZLE_IDENTITY, // g
                VK_COMPONENT_SWIZZLE_IDENTITY, // b
                VK_COMPONENT_SWIZZLE_IDENTITY, // a
            };
            constexpr VkImageSubresourceRange imageSubresourceRange {
                VK_IMAGE_ASPECT_COLOR_BIT, // aspectMask
                0,                         // baseMipLevel
                1,                         // levelCount
                0,                         // baseArrayLayer
                1,                         // layerCount
            };

            constexpr VkSemaphoreCreateFlags semaphoreCreateFlags { 0 };
            const VkSemaphoreCreateInfo semaphoreCreateInfo {
                VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, // sType
                nullptr,                                 // pNext
                semaphoreCreateFlags,                    // flags
            };
            for (uint32_t idx = 0; idx < plat_.swapchainImages.imageViews.size(); ++idx) {
                const VkImageViewCreateInfo imageViewCreateInfo {
                    VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO, // sType
                    nullptr,                                  // pNext
                    0,                                        // flags
                    plat_.swapchainImages.images[idx],        // image
                    VK_IMAGE_VIEW_TYPE_2D,                    // viewType
                    ci.format,                                // format
                    componentMapping,                         // components
                    imageSubresourceRange                     // subresourceRange;
                };
                VALIDATE_VK_RESULT(vkCreateImageView(vkDevice, // device
                    &imageViewCreateInfo,                      // pCreateInfo
                    nullptr,                                   // pAllocator
                    &plat_.swapchainImages.imageViews[idx]));  // pView
                VALIDATE_VK_RESULT(vkCreateSemaphore(vkDevice, // device
                    &semaphoreCreateInfo,                      // pCreateInfo
                    nullptr,                                   // pAllocator
                    &plat_.swapchainImages.semaphores[idx]));  // pSemaphore
            }
        }

        desc_ = GetColorDesc(plat_.swapchainImages.width, plat_.swapchainImages.height, (Format)ci.format,
            (ImageUsageFlags)imageUsageFlags);

        if (flags_ & 0x2) {
            const Format depthFormat = GetValidDepthFormat((const DeviceVk&)device_);
            descDepthBuffer_ = GetDepthDesc(plat_.swapchainImages.width, plat_.swapchainImages.height, depthFormat);
        }
    } else {
        PLUGIN_LOG_E("Invalid surface in swapchain creation");
    }
}

SwapchainVk::~SwapchainVk()
{
    const auto& devicePlatformData = (const DevicePlatformDataVk&)device_.GetPlatformData();
    const VkDevice device = devicePlatformData.device;
    for (auto const imageView : plat_.swapchainImages.imageViews) {
        if (imageView) {
            vkDestroyImageView(device, // device
                imageView,             // imageView
                nullptr);              // pAllocator
        }
    }
    for (const auto semaphore : plat_.swapchainImages.semaphores) {
        if (semaphore) {
            vkDestroySemaphore(device, // device
                semaphore,             // semaphore
                nullptr);              // pAllocator
        }
    }

    CreateFunctionsVk::DestroySwapchain(device, plat_.swapchain);
    if (ownsSurface_ && surface_) {
        CreateFunctionsVk::DestroySurface(devicePlatformData.instance, surface_);
    }
}

const SwapchainPlatformDataVk& SwapchainVk::GetPlatformData() const
{
    return plat_;
}

const GpuImageDesc& SwapchainVk::GetDesc() const
{
    return desc_;
}

const GpuImageDesc& SwapchainVk::GetDescDepthBuffer() const
{
    return descDepthBuffer_;
}

uint32_t SwapchainVk::GetFlags() const
{
    return flags_;
}

SurfaceTransformFlags SwapchainVk::GetSurfaceTransformFlags() const
{
    return surfaceTransformFlags_;
}

uint64_t SwapchainVk::GetSurfaceHandle() const
{
    return VulkanHandleCast<uint64_t>(surface_);
}

uint32_t SwapchainVk::GetNextAcquireSwapchainSemaphoreIndex() const
{
    currSemaphoreIdx_ = (currSemaphoreIdx_ + 1) % plat_.swapchainImages.semaphores.size();
    return currSemaphoreIdx_;
}
RENDER_END_NAMESPACE()
