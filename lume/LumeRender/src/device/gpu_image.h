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
