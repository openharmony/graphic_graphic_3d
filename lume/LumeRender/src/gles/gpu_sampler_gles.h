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
