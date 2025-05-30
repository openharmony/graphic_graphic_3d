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

#ifndef GLES_GPU_SEMAPHORE_GLES_H
#define GLES_GPU_SEMAPHORE_GLES_H

#include <render/device/gpu_resource_desc.h>
#include <render/gles/intf_device_gles.h>
#include <render/namespace.h>

#include "device/gpu_semaphore.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceGLES;
struct GpuSemaphorePlatformDataGles final {
    // GLsync object
    uint64_t sync { 0 };
};

class GpuSemaphoreGles final : public GpuSemaphore {
public:
    explicit GpuSemaphoreGles(Device& device);
    GpuSemaphoreGles(Device& device, uint64_t handle);
    ~GpuSemaphoreGles() override;

    uint64_t GetHandle() const override;
    const GpuSemaphorePlatformDataGles& GetPlatformData() const;

private:
    DeviceGLES& device_;

    bool ownsResources_ { true };
    GpuSemaphorePlatformDataGles plat_;
};
RENDER_END_NAMESPACE()

#endif // GLES_GPU_SEMAPHORE_GLES_H
