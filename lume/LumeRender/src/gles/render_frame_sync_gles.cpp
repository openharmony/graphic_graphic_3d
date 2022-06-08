/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_frame_sync_gles.h"

#include <render/namespace.h>

#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
RenderFrameSyncGLES::RenderFrameSyncGLES(Device& device) : RenderFrameSync()
{
    frameFences_.resize(device.GetCommandBufferingCount());
    for (auto& ref : frameFences_) {
        ref.aFence = nullptr;
    }
}

RenderFrameSyncGLES::~RenderFrameSyncGLES()
{
    for (auto& ref : frameFences_) {
        GLsync fence = (GLsync)ref.aFence;
        if (fence) {
            glDeleteSync(fence);
        }
    }
}
void RenderFrameSyncGLES::BeginFrame()
{
    PLUGIN_ASSERT(frameFences_[bufferingIndex_].aFence == nullptr);
    frameFences_[bufferingIndex_].aFence = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    bufferingIndex_ = (bufferingIndex_ + 1) % (uint32_t)frameFences_.size();
}

void RenderFrameSyncGLES::WaitForFrameFence()
{
    if (frameFences_[bufferingIndex_].aFence) {
        GLsync fence = (GLsync)(frameFences_[bufferingIndex_].aFence);
        const GLenum result = glClientWaitSync(fence, GL_SYNC_FLUSH_COMMANDS_BIT, UINT64_MAX);
        if ((result != GL_ALREADY_SIGNALED) && (result != GL_CONDITION_SATISFIED)) {
            PLUGIN_LOG_E("glClientWaitSync returned %x", result);
        }
        glDeleteSync(fence);
        frameFences_[bufferingIndex_].aFence = nullptr;
    } else {
        // no-fence, no wait.
    }
}

const LowLevelFenceGLES& RenderFrameSyncGLES::GetFrameFence()
{
    return frameFences_[bufferingIndex_];
}
RENDER_END_NAMESPACE()
