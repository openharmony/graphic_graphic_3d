/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_RENDER_FRAME_SYNC_VK_H
#define VULKAN_RENDER_FRAME_SYNC_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "device/render_frame_sync.h"

RENDER_BEGIN_NAMESPACE()
class Device;

struct LowLevelFenceVk {
    VkFence fence { VK_NULL_HANDLE };
};

class RenderFrameSyncVk final : public RenderFrameSync {
public:
    explicit RenderFrameSyncVk(Device& device);
    ~RenderFrameSyncVk() override;

    void BeginFrame() override;
    void WaitForFrameFence() override;
    // set flag that the fence has been signalled
    void FrameFenceIsSignalled();

    LowLevelFenceVk GetFrameFence() const;

private:
    Device& device_;

    struct FrameFence {
        VkFence fence { VK_NULL_HANDLE };
        bool signalled { true };
    };
    BASE_NS::vector<FrameFence> frameFences_;
    uint32_t bufferingIndex_ { 0 };
};
RENDER_END_NAMESPACE()

#endif // VULKAN_RENDER_FRAME_SYNC_VK_H
