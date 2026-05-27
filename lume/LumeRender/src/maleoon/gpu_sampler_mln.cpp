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

#include "util/log.h"
#include "mln_log.h"
#include "gpu_sampler_mln.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "maleoon/device_mln.h"
#include "mln_convert.h"

RENDER_BEGIN_NAMESPACE()

GpuSamplerMln::GpuSamplerMln(Device& device, const GpuSamplerDesc& desc) : device_(device), desc_(desc)
{
    MLN_LOG_INIT("GpuSamplerMln: creating sampler (mag=%u, min=%u, mip=%u, addrUVW=%u/%u/%u)",
        static_cast<uint32_t>(desc.magFilter), static_cast<uint32_t>(desc.minFilter),
        static_cast<uint32_t>(desc.mipMapMode),
        static_cast<uint32_t>(desc.addressModeU), static_cast<uint32_t>(desc.addressModeV),
        static_cast<uint32_t>(desc.addressModeW));
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    MlnSamplerDescriptor samplerDesc {};
    samplerDesc.extensionCount = 0;
    samplerDesc.extensions = nullptr;
    samplerDesc.flags = 0;
    samplerDesc.magFilter = ToMlnFilter(desc_.magFilter);
    samplerDesc.minFilter = ToMlnFilter(desc_.minFilter);
    samplerDesc.mipmapMode = ToMlnSamplerMipmapMode(desc_.mipMapMode);
    samplerDesc.addressModeU = ToMlnSamplerAddressMode(desc_.addressModeU);
    samplerDesc.addressModeV = ToMlnSamplerAddressMode(desc_.addressModeV);
    samplerDesc.addressModeW = ToMlnSamplerAddressMode(desc_.addressModeW);
    samplerDesc.mipLodBias = desc_.mipLodBias;
    samplerDesc.anisotropyEnable = desc_.enableAnisotropy ? 1u : 0u;
    samplerDesc.maxAnisotropy = desc_.maxAnisotropy;
    samplerDesc.compareEnable = desc_.enableCompareOp ? 1u : 0u;
    samplerDesc.compareOp = ToMlnCompareOp(desc_.compareOp);
    samplerDesc.minLod = desc_.minLod;
    samplerDesc.maxLod = desc_.maxLod;
    samplerDesc.borderColor = ToMlnBorderColor(desc_.borderColor);
    samplerDesc.unnormalizedCoordinates = desc_.enableUnnormalizedCoordinates ? 1u : 0u;

    plat_.sampler = MlnCreateSampler(deviceMln.GetMlnDevice(), &samplerDesc);
    if (!plat_.sampler) {
        MLN_LOG_ERR("GpuSamplerMln: MlnCreateSampler failed");
    }
}

GpuSamplerMln::~GpuSamplerMln()
{
    if (plat_.sampler) {
        const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
        MlnDestroySampler(deviceMln.GetMlnDevice(), plat_.sampler);
    }
}

const GpuSamplerDesc& GpuSamplerMln::GetDesc() const
{
    return desc_;
}

const GpuSamplerPlatformDataMln& GpuSamplerMln::GetPlatformData() const
{
    return plat_;
}

RENDER_END_NAMESPACE()
