/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
