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

#ifndef MALEOON_GPU_SAMPLER_MLN_H
#define MALEOON_GPU_SAMPLER_MLN_H

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/maleoon/intf_device_mln.h>

#include "device/gpu_sampler.h"

RENDER_BEGIN_NAMESPACE()

class Device;

class GpuSamplerMln final : public GpuSampler {
public:
    GpuSamplerMln(Device& device, const GpuSamplerDesc& desc);
    ~GpuSamplerMln() override;

    const GpuSamplerDesc& GetDesc() const override;

    const GpuSamplerPlatformDataMln& GetPlatformData() const;

private:
    Device& device_;
    GpuSamplerDesc desc_;
    GpuSamplerPlatformDataMln plat_;
};

RENDER_END_NAMESPACE()

#endif  // MALEOON_GPU_SAMPLER_MLN_H
