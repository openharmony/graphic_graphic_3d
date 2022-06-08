/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef DEVICE_GPU_IMAGE_H
#define DEVICE_GPU_IMAGE_H

#include <cstdint>

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct GpuImageDesc;
struct GpuImagePlatformData;

/** Gpu image.
 * Immutable platform gpu image wrapper.
 */
class GpuImage {
public:
    // additional flags from creation e.g. platform conversion needed etc.
    enum AdditionalFlagBits : uint32_t {
        // platform conversion (e.g. ycbcr conversion needed, and/or special binding e.g. OES)
        ADDITIONAL_PLATFORM_CONVERSION_BIT = 1,
    };
    using AdditionalFlags = uint32_t;

    GpuImage() = default;
    virtual ~GpuImage() = default;

    GpuImage(const GpuImage&) = delete;
    GpuImage& operator=(const GpuImage&) = delete;

    virtual const GpuImageDesc& GetDesc() const = 0;
    virtual const GpuImagePlatformData& GetBasePlatformData() const = 0;

    virtual AdditionalFlags GetAdditionalFlags() const = 0;
};

RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_IMAGE_H
