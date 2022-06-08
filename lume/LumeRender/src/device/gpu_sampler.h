/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef DEVICE_GPU_SAMPLER_H
#define DEVICE_GPU_SAMPLER_H

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
struct GpuSamplerDesc;

class GpuSampler {
public:
    GpuSampler() = default;
    virtual ~GpuSampler() = default;

    GpuSampler(const GpuSampler&) = delete;
    GpuSampler& operator=(const GpuSampler&) = delete;

    virtual const GpuSamplerDesc& GetDesc() const = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_GPU_SAMPLER_H
