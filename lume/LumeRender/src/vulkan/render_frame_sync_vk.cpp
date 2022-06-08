/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_frame_sync_vk.h"

#include <cinttypes>
#include <cstdint>
#include <vulkan/vulkan.h>

#include <render/namespace.h>

#include "device/device.h"
#include "device/render_frame_sync.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
RenderFrameSyncVk::RenderFrameSyncVk(Device& dev) : RenderFrameSync(), device_ { dev }
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

    frameFences_.resize(device_.GetCommandBufferingCount());

    constexpr VkFenceCreateFlags fenceCreateFlags { VK_FENCE_CREATE_SIGNALED_BIT };
    const VkFenceCreateInfo fenceCreateInfo {
        VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, // sType
        nullptr,                             // pNext
        fenceCreateFlags,                    // flags
    };
    for (auto& ref : frameFences_) {
        VALIDATE_VK_RESULT(vkCreateFence(device, // device
            &fenceCreateInfo,                    // pCreateInfo
            nullptr,                             // pAllocator
            &ref.fence));                        // pFence
    }
}

RenderFrameSyncVk::~RenderFrameSyncVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

    for (auto& ref : frameFences_) {
        vkDestroyFence(device, // device
            ref.fence,         // fence
            nullptr);          // pAllocator
    }
}

void RenderFrameSyncVk::BeginFrame()
{
    bufferingIndex_ = (bufferingIndex_ + 1) % (uint32_t)frameFences_.size();
}

void RenderFrameSyncVk::WaitForFrameFence()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

    if (frameFences_[bufferingIndex_].signalled) {
        VkFence fence = frameFences_[bufferingIndex_].fence;
        const VkResult res = vkWaitForFences(device, // device
            1,                                       // fenceCount
            &fence,                                  // pFences
            VK_TRUE,                                 // waitAll
            UINT64_MAX);                             // timeout
        if (res == VK_SUCCESS) {
            VALIDATE_VK_RESULT(vkResetFences(device, // device
                1,                                   // fenceCount
                &fence));                            // pFences

            frameFences_[bufferingIndex_].signalled = false;
        } else {
            PLUGIN_LOG_E("vkWaitForFences VkResult: %i. Frame count: %" PRIu64 ". Device invalidated.", res,
                device_.GetFrameCount());
            device_.SetDeviceStatus(false);
            PLUGIN_ASSERT(false);
        }
    }
}

LowLevelFenceVk RenderFrameSyncVk::GetFrameFence() const
{
    return { frameFences_[bufferingIndex_].fence };
}

void RenderFrameSyncVk::FrameFenceIsSignalled()
{
    frameFences_[bufferingIndex_].signalled = true;
}
RENDER_END_NAMESPACE()
