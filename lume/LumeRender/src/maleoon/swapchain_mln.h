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

#ifndef MALEOON_SWAPCHAIN_MLN_H
#define MALEOON_SWAPCHAIN_MLN_H

#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_device.h>
#include <render/namespace.h>

#include "device/swapchain.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;

struct SwapchainPlatformDataMln {
    MlnDisplaySurface displaySurface { MLN_NULL_HANDLE };
    MlnSwapchain swapchain { MLN_NULL_HANDLE };
    BASE_NS::vector<MlnResource> images;
    BASE_NS::vector<MlnResourceView> imageViews;
    uint32_t imageCount { 0 };
    uint32_t currentImageIndex { 0 };
};

class SwapchainMln final : public Swapchain {
public:
    SwapchainMln(Device& device, const SwapchainCreateInfo& swapchainCreateInfo);
    ~SwapchainMln() override;

    const GpuImageDesc& GetDesc() const override;
    const GpuImageDesc& GetDescDepthBuffer() const override;
    uint32_t GetFlags() const override;
    SurfaceTransformFlags GetSurfaceTransformFlags() const override;
    uint64_t GetSurfaceHandle() const override;
    bool IsValid() const override;

    const SwapchainPlatformDataMln& GetPlatformData() const;

    MlnStatus AcquireNextImage(uint32_t& imageIndex,
        MlnTimeline signalTimeline = MLN_NULL_HANDLE, uint64_t signalValue = 0);
    MlnStatus Present(uint32_t imageIndex,
        MlnTimeline waitTimeline = MLN_NULL_HANDLE, uint64_t waitValue = 0);

private:
    void CreateDisplaySurface(const SwapchainCreateInfo& createInfo);
    void CreateSwapchain(const SwapchainCreateInfo& createInfo);
    void GetSwapchainImages();

    Device& device_;
    GpuImageDesc desc_;
    GpuImageDesc descDepthBuffer_;
    SwapchainPlatformDataMln plat_;
    uint32_t flags_ { 0u };
    SurfaceTransformFlags surfaceTransformFlags_ { 0u };
    uint64_t surfaceHandle_ { 0u };
    bool valid_ { false };
};

RENDER_END_NAMESPACE()

#endif // MALEOON_SWAPCHAIN_MLN_H
