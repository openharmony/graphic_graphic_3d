/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef DEVICE_SWAPCHAIN_H
#define DEVICE_SWAPCHAIN_H

#include <cstdint>

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct GpuImageDesc;

class Swapchain {
public:
    Swapchain() = default;
    virtual ~Swapchain() = default;

    Swapchain(const Swapchain&) = delete;
    Swapchain& operator=(const Swapchain&) = delete;

    virtual const GpuImageDesc& GetDesc() const = 0;
    virtual const GpuImageDesc& GetDescDepthBuffer() const = 0;

    virtual uint32_t GetFlags() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_SWAPCHAIN_H
