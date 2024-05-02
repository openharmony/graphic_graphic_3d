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

#include "render_frame_sync_vk.h"

#include <cinttypes>
#include <cstdint>
#include <vulkan/vulkan_core.h>

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
        if (ref.fence) {
            vkDestroyFence(device, // device
                ref.fence,         // fence
                nullptr);          // pAllocator
        }
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
        if (fence) {
            const VkResult res = vkWaitForFences(device, // device
                1U,                                      // fenceCount
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
            }
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
