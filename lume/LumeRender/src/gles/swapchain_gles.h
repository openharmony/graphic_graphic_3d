/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
    uintptr_t surface; // currently EGLDisplay (on GLES) or HDC (on GL/Windows)
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

    uint32_t GetNextImage();

private:
    DeviceGLES& device_;

    GpuImageDesc desc_ {};
    GpuImageDesc descDepthBuffer_ {};
    SwapchainPlatformDataGL plat_ {};
    uint32_t flags_ { 0u };
};
RENDER_END_NAMESPACE()

#endif // GLES_SWAPCHAIN_GLES_H
