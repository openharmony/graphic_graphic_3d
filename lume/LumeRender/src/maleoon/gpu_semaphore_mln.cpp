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
#include "gpu_semaphore_mln.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "maleoon/device_mln.h"

RENDER_BEGIN_NAMESPACE()

GpuSemaphoreMln::GpuSemaphoreMln(Device& device) : device_(device)
{
    MLN_LOG_INIT("GpuSemaphoreMln: creating timeline semaphore");
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);

    MlnTimelineDescriptor timelineDesc {};
    timelineDesc.extensionCount = 0;
    timelineDesc.extensions = nullptr;
    timelineDesc.flags = 0;
    timelineDesc.type = MLN_TIMELINE_TYPE_BINARY;
    timelineDesc.initialValue = 0;

    timeline_ = MlnCreateTimeline(deviceMln.GetMlnDevice(), &timelineDesc);
    if (!timeline_) {
        MLN_LOG_ERR("GpuSemaphoreMln: MlnCreateTimeline failed");
    }
}

GpuSemaphoreMln::GpuSemaphoreMln(Device& device, uint64_t handle) : device_(device), ownsResources_(false)
{
    MLN_LOG_INIT("GpuSemaphoreMln: wrapping external timeline handle");
    // Wrap an externally provided timeline handle
    timeline_ = reinterpret_cast<MlnTimeline>(static_cast<uintptr_t>(handle));
}

GpuSemaphoreMln::~GpuSemaphoreMln()
{
    if (ownsResources_ && timeline_) {
        const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
        MlnDestroyTimeline(deviceMln.GetMlnDevice(), timeline_);
    }
}

uint64_t GpuSemaphoreMln::GetHandle() const
{
    return reinterpret_cast<uint64_t>(timeline_);
}

MlnTimeline GpuSemaphoreMln::GetMlnTimeline() const
{
    return timeline_;
}

RENDER_END_NAMESPACE()
