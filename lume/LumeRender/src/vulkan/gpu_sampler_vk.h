/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
