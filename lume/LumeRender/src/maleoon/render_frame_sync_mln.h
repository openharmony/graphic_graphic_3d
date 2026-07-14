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

#ifndef MALEOON_RENDER_FRAME_SYNC_MLN_H
#define MALEOON_RENDER_FRAME_SYNC_MLN_H

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "device/render_frame_sync.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;

class RenderFrameSyncMln final : public RenderFrameSync {
public:
    explicit RenderFrameSyncMln(Device& device);
    ~RenderFrameSyncMln() override;

    void BeginFrame() override;
    void WaitForFrameFence() override;

    MlnTimeline GetCurrentTimeline() const;
    uint64_t GetCurrentTimelineValue() const;

    // Called by RenderBackendMln::SubmitFrame after MlnQueueSubmit signals this timeline.
    // Sets the expected signal value and marks the fence as pending (not yet completed).
    void MarkFrameSubmitted(uint64_t signalValue);

private:
    Device& device_;

    struct FrameFence {
        MlnTimeline timeline{MLN_NULL_HANDLE};
        uint64_t value{0};
        bool signalled{true};
    };

    BASE_NS::vector<FrameFence> frameFences_;
    uint32_t bufferingIndex_{0};
};

RENDER_END_NAMESPACE()

#endif  // MALEOON_RENDER_FRAME_SYNC_MLN_H
