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

#ifndef VULKAN_GPU_SAMPLER_VK_H
#define VULKAN_GPU_SAMPLER_VK_H

#include <vulkan/vulkan_core.h>

#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "device/gpu_sampler.h"

RENDER_BEGIN_NAMESPACE()
class Device;

class GpuSamplerVk final : public GpuSampler {
public:
    GpuSamplerVk(Device& device, const GpuSamplerDesc& desc);
    ~GpuSamplerVk();

    const GpuSamplerDesc& GetDesc() const override;
    const GpuSamplerPlatformDataVk& GetPlatformData() const;

private:
    Device& device_;

    GpuSamplerPlatformDataVk plat_;
    GpuSamplerDesc desc_;
    VkSamplerYcbcrConversion samplerConversion_ { VK_NULL_HANDLE };
};
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_SAMPLER_VK_H
