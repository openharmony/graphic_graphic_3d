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

#ifndef MALEOON_GPU_SEMAPHORE_MLN_H
#define MALEOON_GPU_SEMAPHORE_MLN_H

#include <cstdint>
#include <render/namespace.h>

#include "device/gpu_semaphore.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;

class GpuSemaphoreMln final : public GpuSemaphore {
public:
    explicit GpuSemaphoreMln(Device& device);
    GpuSemaphoreMln(Device& device, uint64_t handle);
    ~GpuSemaphoreMln() override;

    uint64_t GetHandle() const override;

    MlnTimeline GetMlnTimeline() const;

private:
    Device& device_;
    MlnTimeline timeline_{MLN_NULL_HANDLE};
    bool ownsResources_{true};
};

RENDER_END_NAMESPACE()

#endif  // MALEOON_GPU_SEMAPHORE_MLN_H
