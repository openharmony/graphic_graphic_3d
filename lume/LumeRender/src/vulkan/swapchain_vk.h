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

#ifndef VULKAN_SWAPCHAIN_VK_H
#define VULKAN_SWAPCHAIN_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

#include "device/swapchain.h"

RENDER_BEGIN_NAMESPACE()
class Device;
struct SwapchainCreateInfo;

struct SwapchainImagesVk {
    BASE_NS::vector<VkImage> images;
    BASE_NS::vector<VkImageView> imageViews;

    uint32_t width { 0 };
    uint32_t height { 0 };

    BASE_NS::vector<VkSemaphore> semaphores;
};

struct SwapchainPlatformDataVk final {
    VkSwapchainKHR swapchain { VK_NULL_HANDLE };
    SwapchainImagesVk swapchainImages;
};

class SwapchainVk final : public Swapchain {
public:
    SwapchainVk(Device& device, const SwapchainCreateInfo& swapchainCreateInfo);
    ~SwapchainVk();

    const SwapchainPlatformDataVk& GetPlatformData() const;
    const GpuImageDesc& GetDesc() const override;
    const GpuImageDesc& GetDescDepthBuffer() const override;

    uint32_t GetFlags() const override;
    SurfaceTransformFlags GetSurfaceTransformFlags() const override;
    uint64_t GetSurfaceHandle() const override;

    // only for locked backend usage to get always the next semaphore index in image acquire
    uint32_t GetNextAcquireSwapchainSemaphoreIndex() const;

private:
    Device& device_;

    VkSurfaceKHR surface_ {};
    bool ownsSurface_ { false };

    GpuImageDesc desc_;
    GpuImageDesc descDepthBuffer_;
    SwapchainPlatformDataVk plat_;
    uint32_t flags_ { 0u };
    SurfaceTransformFlags surfaceTransformFlags_ { 0u };

    // mutable object for locked backend usage only
    mutable uint32_t currSemaphoreIdx_ { 0U };
};
RENDER_END_NAMESPACE()

#endif // VULKAN_SWAPCHAIN_VK_H
