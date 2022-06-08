/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef DEVICE_RENDER_FRAME_SYNC_H
#define DEVICE_RENDER_FRAME_SYNC_H

#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class RenderFrameSync {
public:
    RenderFrameSync() = default;
    virtual ~RenderFrameSync() = default;

    /* Begin frame, i.e. advance to next buffer. */
    virtual void BeginFrame() = 0;
    /* Wait for current frame's fence and reset. */
    virtual void WaitForFrameFence() = 0;
};
RENDER_END_NAMESPACE()

#endif // DEVICE_RENDER_FRAME_SYNC_H
