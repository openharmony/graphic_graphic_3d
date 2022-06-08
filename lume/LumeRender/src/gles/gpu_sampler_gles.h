/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef GLES_GPU_SAMPLER_GLES_H
#define GLES_GPU_SAMPLER_GLES_H

#include <render/device/gpu_resource_desc.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

#include "device/gpu_sampler.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
struct GpuSamplerPlatformDataGL final : public GpuSamplerPlatformData {
    uint32_t sampler { 0 };
};

struct GpuSamplerDesc;

class GpuSamplerGLES final : public GpuSampler {
public:
    GpuSamplerGLES(Device& device, const GpuSamplerDesc& desc);
    ~GpuSamplerGLES();

    const GpuSamplerDesc& GetDesc() const override;
    const GpuSamplerPlatformDataGL& GetPlatformData() const;

private:
    DeviceGLES& device_;

    GpuSamplerPlatformDataGL plat_;
    GpuSamplerDesc desc_;
};
RENDER_END_NAMESPACE()

#endif // GLES_GPU_SAMPLER_GLES_H
