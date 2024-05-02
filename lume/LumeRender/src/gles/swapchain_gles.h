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

#ifndef GLES_SWAPCHAIN_GLES_H
#define GLES_SWAPCHAIN_GLES_H

#include <cstdint>

#include <base/containers/vector.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>

#include "device/swapchain.h"
#include "gles/gpu_image_gles.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
struct SwapchainCreateInfo;

struct SwapchainImagesGLES {
    BASE_NS::vector<uint32_t> images;

    uint32_t width { 0 };
    uint32_t height { 0 };
};

struct SwapchainPlatformDataGL final {
    uintptr_t surface; // currently EGLSurface (on GLES) or HDC (on GL/Windows)
#if RENDER_GL_FLIP_Y_SWAPCHAIN
    BASE_NS::vector<uint32_t> fbos; // FBO for the swapchain.
#endif
    uint32_t swapchainImageIndex;
    SwapchainImagesGLES swapchainImages;
    bool vsync;
    GpuImagePlatformDataGL depthPlatformData;
};

class SwapchainGLES final : public Swapchain {
public:
    SwapchainGLES(Device& device, const SwapchainCreateInfo& swapchainCreateInfo);
    ~SwapchainGLES() override;

    const SwapchainPlatformDataGL& GetPlatformData() const;
    const GpuImageDesc& GetDesc() const override;
    const GpuImageDesc& GetDescDepthBuffer() const override;

    uint32_t GetFlags() const override;
    SurfaceTransformFlags GetSurfaceTransformFlags() const override
    {
        return 0U; // nothing should be done
    }
    uint64_t GetSurfaceHandle() const override;

    uint32_t GetNextImage() const;

private:
    DeviceGLES& device_;

    GpuImageDesc desc_ {};
    GpuImageDesc descDepthBuffer_ {};
    SwapchainPlatformDataGL plat_ {};
    uint32_t flags_ { 0u };
    bool ownsSurface_ { false };
};
RENDER_END_NAMESPACE()

#endif // GLES_SWAPCHAIN_GLES_H
