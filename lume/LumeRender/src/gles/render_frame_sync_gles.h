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

#ifndef GLES_RENDER_FRAME_SYNC_GLES_H
#define GLES_RENDER_FRAME_SYNC_GLES_H

#include <cstdint>

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "device/render_frame_sync.h"

RENDER_BEGIN_NAMESPACE()
class Device;

struct LowLevelFenceGLES {
    void* aFence { nullptr };
};

class RenderFrameSyncGLES final : public RenderFrameSync {
public:
    explicit RenderFrameSyncGLES(Device& device);
    virtual ~RenderFrameSyncGLES();

    void BeginFrame() override;
    void WaitForFrameFence() override;

    const LowLevelFenceGLES& GetFrameFence();

private:
    BASE_NS::vector<LowLevelFenceGLES> frameFences_;
    uint32_t bufferingIndex_ { 0 };
};
RENDER_END_NAMESPACE()

#endif // GLES_RENDER_FRAME_SYNC_GLES_H
