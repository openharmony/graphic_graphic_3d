/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef VULKAN_GPU_SEMAPHORE_VK_H
#define VULKAN_GPU_SEMAPHORE_VK_H

#include <vulkan/vulkan_core.h>

#include <render/namespace.h>
#include <render/vulkan/intf_device_vk.h>

#include "device/gpu_semaphore.h"

RENDER_BEGIN_NAMESPACE()
class Device;

struct GpuSemaphorePlatformDataVk final {
    VkSemaphore semaphore { VK_NULL_HANDLE };
};

class GpuSemaphoreVk final : public GpuSemaphore {
public:
    explicit GpuSemaphoreVk(Device& device);
    GpuSemaphoreVk(Device& device, uint64_t handle);
    ~GpuSemaphoreVk() override;

    uint64_t GetHandle() const override;
    const GpuSemaphorePlatformDataVk& GetPlatformData() const;

private:
    Device& device_;

    bool ownsResources_ { true };
    GpuSemaphorePlatformDataVk plat_;
};
RENDER_END_NAMESPACE()
#endif // VULKAN_GPU_SEMAPHORE_VK_H
