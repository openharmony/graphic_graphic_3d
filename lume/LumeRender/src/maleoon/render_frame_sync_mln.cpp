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

#include "render_frame_sync_mln.h"
#include "mln_log.h"
#include "util/log.h"

#include <core/log.h>
#include <render/namespace.h>

#include "device/device.h"
#include "maleoon/device_mln.h"

RENDER_BEGIN_NAMESPACE()

RenderFrameSyncMln::RenderFrameSyncMln(Device& device) : device_(device)
{
    MLN_LOG_INIT("RenderFrameSyncMln: creating frame sync");
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    const uint32_t bufferingCount = device_.GetCommandBufferingCount();

    frameFences_.resize(bufferingCount);
    for (auto& fence : frameFences_) {
        MlnTimelineDescriptor desc{};
        desc.extensionCount = 0;
        desc.extensions = nullptr;
        desc.flags = 0;
        desc.type = MLN_TIMELINE_TYPE_COUNTER;
        desc.initialValue = 0;

        fence.timeline = MlnCreateTimeline(deviceMln.GetMlnDevice(), &desc);
        fence.value = 0;
        fence.signalled = true;
    }
}

RenderFrameSyncMln::~RenderFrameSyncMln()
{
    const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
    for (auto& fence : frameFences_) {
        if (fence.timeline) {
            MlnDestroyTimeline(deviceMln.GetMlnDevice(), fence.timeline);
        }
    }
}

void RenderFrameSyncMln::BeginFrame()
{
    if (!frameFences_.empty()) {
        bufferingIndex_ = (bufferingIndex_ + 1u) % static_cast<uint32_t>(frameFences_.size());
    }
}

void RenderFrameSyncMln::WaitForFrameFence()
{
    if (frameFences_.empty()) {
        return;
    }

    auto& fence = frameFences_[bufferingIndex_];
    if (!fence.signalled && fence.timeline && fence.value > 0) {
        const DeviceMln& deviceMln = static_cast<const DeviceMln&>(device_);
        MlnTimelineWaitDescriptor waitDesc{};
        waitDesc.extensionCount = 0;
        waitDesc.extensions = nullptr;
        waitDesc.flags = 0;
        waitDesc.timelineCount = 1;
        waitDesc.timelines = &fence.timeline;
        waitDesc.values = &fence.value;
        constexpr uint64_t WAIT_TIMEOUT_NS = 5000000000ULL;  // 5 seconds
        MlnStatus waitResult = MlnWaitForTimelines(deviceMln.GetMlnDevice(), &waitDesc, WAIT_TIMEOUT_NS);
        if (waitResult == MLN_STATUS_TIMEOUT) {
            MLN_LOG_ERR("WaitForFrameFence: TIMEOUT after 5s (value=%llu) — possible GPU hang",
                static_cast<unsigned long long>(fence.value));
        } else if (waitResult != MLN_STATUS_SUCCESS) {
            MLN_LOG_ERR("WaitForFrameFence: FAILED (status=%d, value=%llu)",
                static_cast<int>(waitResult),
                static_cast<unsigned long long>(fence.value));
        }
    }
    fence.signalled = true;
}

MlnTimeline RenderFrameSyncMln::GetCurrentTimeline() const
{
    if (frameFences_.empty()) {
        return MLN_NULL_HANDLE;
    }
    return frameFences_[bufferingIndex_].timeline;
}

uint64_t RenderFrameSyncMln::GetCurrentTimelineValue() const
{
    if (frameFences_.empty()) {
        return 0;
    }
    return frameFences_[bufferingIndex_].value;
}

void RenderFrameSyncMln::MarkFrameSubmitted(uint64_t signalValue)
{
    if (frameFences_.empty()) {
        return;
    }
    auto& fence = frameFences_[bufferingIndex_];
    fence.value = signalValue;
    fence.signalled = false;  // GPU work pending — WaitForFrameFence will now actually wait
}

RENDER_END_NAMESPACE()
