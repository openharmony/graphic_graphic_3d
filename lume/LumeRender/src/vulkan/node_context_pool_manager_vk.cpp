/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "node_context_pool_manager_vk.h"

#include <cstdint>
#include <vulkan/vulkan.h>

#include <base/containers/fixed_string.h>
#include <base/math/mathf.h>
#include <base/util/compile_time_hashes.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_image.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "nodecontext/node_context_pool_manager.h"
#include "nodecontext/render_command_list.h"
#include "util/log.h"
#include "vulkan/device_vk.h"
#include "vulkan/gpu_image_vk.h"
#include "vulkan/gpu_resource_util_vk.h"
#include "vulkan/pipeline_create_functions_vk.h"
#include "vulkan/validate_vk.h"

using namespace BASE_NS;

template<>
uint64_t BASE_NS::hash(const RENDER_NS::ImageLayout& val)
{
    return static_cast<uint64_t>(val);
}
template<>
uint64_t BASE_NS::hash(const RENDER_NS::RenderPassSubpassDesc& subpass)
{
    uint64_t seed = 0;
    HashRange(seed, subpass.inputAttachmentIndices, subpass.inputAttachmentIndices + subpass.inputAttachmentCount);
    HashRange(seed, subpass.colorAttachmentIndices, subpass.colorAttachmentIndices + subpass.colorAttachmentCount);
    HashRange(
        seed, subpass.resolveAttachmentIndices, subpass.resolveAttachmentIndices + subpass.resolveAttachmentCount);
    if (subpass.depthAttachmentCount) {
        HashCombine(seed, static_cast<uint64_t>(subpass.depthAttachmentIndex));
    }
    return seed;
}

RENDER_BEGIN_NAMESPACE()
namespace {
struct FBSize {
    uint32_t width { 0 };
    uint32_t height { 0 };
    uint32_t layers { 1 };
};

inline void HashRenderPassCompatibility(uint64_t& hash, const RenderPassDesc& renderPassDesc,
    const LowLevelRenderPassCompatibilityDescVk& renderPassCompatibilityDesc, const RenderPassSubpassDesc& subpasses)
{
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        const LowLevelRenderPassCompatibilityDescVk::Attachment& atCompatibilityDesc =
            renderPassCompatibilityDesc.attachments[idx];
        HashCombine(hash, static_cast<uint64_t>(atCompatibilityDesc.format),
            static_cast<uint64_t>(atCompatibilityDesc.sampleCountFlags));
    }
    HashRange(hash, &subpasses, &subpasses + renderPassDesc.subpassCount);
}

inline void HashRenderPassLayouts(
    uint64_t& hash, const RenderPassDesc& renderPassDesc, const RenderPassImageLayouts& renderPassImageLayouts)
{
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        HashCombine(hash, renderPassImageLayouts.attachmentInitialLayouts[idx],
            renderPassImageLayouts.attachmentFinalLayouts[idx]);
    }
}

inline void HashFramebuffer(
    uint64_t& hash, const RenderPassDesc& renderPassDesc, const GpuResourceManager& gpuResourceMgr)
{
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        const RenderPassDesc::AttachmentDesc& atDesc = renderPassDesc.attachments[idx];
        // with generation index
        const EngineResourceHandle gpuHandle = gpuResourceMgr.GetGpuHandle(renderPassDesc.attachmentHandles[idx]);
        HashCombine(hash, gpuHandle.id, static_cast<uint64_t>(atDesc.layer), static_cast<uint64_t>(atDesc.mipLevel));
    }
}

struct RenderPassHashes {
    uint64_t renderPassCompatibilityHash { 0 };
    uint64_t renderPassHash { 0 };  // continued hashing from compatibility
    uint64_t frameBufferHash { 0 }; // only framebuffer related hash
};

inline RenderPassHashes HashBeginRenderPass(const RenderCommandBeginRenderPass& beginRenderPass,
    const LowLevelRenderPassCompatibilityDescVk& renderPassCompatibilityDesc, const GpuResourceManager& gpuResourceMgr)
{
    RenderPassHashes rpHashes;

    const auto& renderPassDesc = beginRenderPass.renderPassDesc;

    PLUGIN_ASSERT(renderPassDesc.subpassCount > 0);
    HashRenderPassCompatibility(rpHashes.renderPassCompatibilityHash, renderPassDesc, renderPassCompatibilityDesc,
        beginRenderPass.subpasses[0]);

    rpHashes.renderPassHash = rpHashes.renderPassCompatibilityHash; // for starting point
    HashRenderPassLayouts(rpHashes.renderPassHash, renderPassDesc, beginRenderPass.imageLayouts);

    rpHashes.frameBufferHash = rpHashes.renderPassCompatibilityHash; // depends on the compatible render pass
    HashFramebuffer(rpHashes.frameBufferHash, renderPassDesc, gpuResourceMgr);

    return rpHashes;
}

VkFramebuffer CreateFramebuffer(const GpuResourceManager& gpuResourceMgr, const RenderPassDesc& renderPassDesc,
    const VkDevice device, const VkRenderPass compatibleRenderPass)
{
    const uint32_t attachmentCount = renderPassDesc.attachmentCount;
    PLUGIN_ASSERT(attachmentCount <= PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT);

    FBSize size;
    VkImageView imageViews[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    uint32_t viewIndex = 0;

    for (uint32_t idx = 0; idx < attachmentCount; ++idx) {
        const RenderHandle handle = renderPassDesc.attachmentHandles[idx];
        const RenderPassDesc::AttachmentDesc& attachmentDesc = renderPassDesc.attachments[idx];
        const GpuImageVk* image = gpuResourceMgr.GetImage<GpuImageVk>(handle);
        PLUGIN_ASSERT(image);
        if (image) {
            const GpuImageDesc& imageDesc = image->GetDesc();
            size.width = imageDesc.width;
            size.height = imageDesc.height;

            const GpuImagePlatformDataVk& plat = image->GetPlatformData();
            const GpuImagePlatformDataViewsVk& imagePlat = image->GetPlatformDataViews();
            imageViews[viewIndex] = plat.imageViewBase;
            if ((attachmentDesc.mipLevel >= 1) && (attachmentDesc.mipLevel < imagePlat.mipImageViews.size())) {
                imageViews[viewIndex] = imagePlat.mipImageViews[attachmentDesc.mipLevel];
                size.width = Math::max(1u, size.width >> attachmentDesc.mipLevel);
                size.height = Math::max(1u, size.height >> attachmentDesc.mipLevel);
            } else if ((attachmentDesc.layer >= 1) && (attachmentDesc.layer < imagePlat.layerImageViews.size())) {
                imageViews[viewIndex] = imagePlat.layerImageViews[attachmentDesc.layer];
            }
            viewIndex++;
        }
    }
    PLUGIN_ASSERT(viewIndex == attachmentCount);

    const VkFramebufferCreateInfo framebufferCreateInfo {
        VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
        nullptr,                                   // pNext
        VkFramebufferCreateFlags { 0 },            // flags
        compatibleRenderPass,                      // renderPass
        attachmentCount,                           // attachmentCount
        imageViews,                                // pAttachments
        size.width,                                // width
        size.height,                               // height
        size.layers,                               // layers
    };

    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    VALIDATE_VK_RESULT(vkCreateFramebuffer(device, // device
        &framebufferCreateInfo,                    // pCreateInfo
        nullptr,                                   // pAllocator
        &framebuffer));                            // pFramebuffer

    return framebuffer;
}
} // namespace

NodeContextPoolManagerVk::NodeContextPoolManagerVk(
    Device& device, GpuResourceManager& gpuResourceManager, const GpuQueue& gpuQueue)
    : NodeContextPoolManager(), device_ { device }, gpuResourceMgr_ { gpuResourceManager }
{
    const DeviceVk& deviceVk = static_cast<const DeviceVk&>(device_);
    const VkDevice vkDevice = static_cast<const DevicePlatformDataVk&>(device_.GetPlatformData()).device;

    const LowLevelGpuQueueVk lowLevelGpuQueue = deviceVk.GetGpuQueue(gpuQueue);

    const uint32_t bufferingCount = device_.GetCommandBufferingCount();
    if (bufferingCount > 0) {
        // prepare and create command buffers
        commandPools_.resize(bufferingCount);

        constexpr VkCommandPoolCreateFlags commandPoolCreateFlags { 0u };
        const uint32_t queueFamilyIndex = lowLevelGpuQueue.queueInfo.queueFamilyIndex;
        const VkCommandPoolCreateInfo commandPoolCreateInfo {
            VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO, // sType
            nullptr,                                    // pNext
            commandPoolCreateFlags,                     // flags
            queueFamilyIndex,                           // queueFamilyIndexlayers
        };
        constexpr VkSemaphoreCreateFlags semaphoreCreateFlags { 0 };
        constexpr VkSemaphoreCreateInfo semaphoreCreateInfo {
            VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, // sType
            nullptr,                                 // pNext
            semaphoreCreateFlags,                    // flags
        };

        for (uint32_t frameIdx = 0; frameIdx < commandPools_.size(); ++frameIdx) {
            auto& cmdPool = commandPools_[frameIdx];
            VALIDATE_VK_RESULT(vkCreateCommandPool(vkDevice, // device
                &commandPoolCreateInfo,                      // pCreateInfo
                nullptr,                                     // pAllocator
                &cmdPool.commandPool));                      // pCommandPool

            // pre-create command buffers and semaphores
            constexpr VkCommandBufferLevel commandBufferLevel { VK_COMMAND_BUFFER_LEVEL_PRIMARY };
            const VkCommandBufferAllocateInfo commandBufferAllocateInfo {
                VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType
                nullptr,                                        // pNext
                cmdPool.commandPool,                            // commandPool
                commandBufferLevel,                             // level
                1,                                              // commandBufferCount
            };

            VALIDATE_VK_RESULT(vkAllocateCommandBuffers(vkDevice, // device
                &commandBufferAllocateInfo,                       // pAllocateInfo
                &cmdPool.commandBuffer.commandBuffer));           // pCommandBuffers

            VALIDATE_VK_RESULT(vkCreateSemaphore(vkDevice, // device
                &semaphoreCreateInfo,                      // pCreateInfo
                nullptr,                                   // pAllocator
                &cmdPool.commandBuffer.semaphore));        // pSemaphore

            // NOTE: cmd buffers taged in first beginFrame
        }
    }
}

NodeContextPoolManagerVk::~NodeContextPoolManagerVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

    for (auto& cmdPoolRef : commandPools_) {
        vkDestroySemaphore(device,              // device
            cmdPoolRef.commandBuffer.semaphore, // semaphore
            nullptr);                           // pAllocator
        vkDestroyCommandPool(device,            // device
            cmdPoolRef.commandPool,             // commandPool
            nullptr);                           // pAllocator
    }

    for (auto& ref : framebufferCache_.hashToElement) {
        if (ref.second.frameBuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(device, // device
                ref.second.frameBuffer,  // framebuffer
                nullptr);                // pAllocator
        }
    }
    for (auto& ref : renderPassCache_.hashToElement) {
        if (ref.second.renderPass != VK_NULL_HANDLE) {
            renderPassCreator_.DestroyRenderPass(device, ref.second.renderPass);
        }
    }
    for (auto& ref : renderPassCompatibilityCache_.hashToElement) {
        if (ref.second.renderPass != VK_NULL_HANDLE) {
            renderPassCreator_.DestroyRenderPass(device, ref.second.renderPass);
        }
    }
}

void NodeContextPoolManagerVk::BeginFrame()
{
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    if (firstFrame_) {
        firstFrame_ = false;
        for (const auto& cmdPoolRef : commandPools_) {
            GpuResourceUtil::DebugObjectNameVk(device_, VK_OBJECT_TYPE_COMMAND_BUFFER,
                VulkanHandleCast<uint64_t>(cmdPoolRef.commandBuffer.commandBuffer), debugName_ + "_cmd_buf");
        }
    }
#endif

    bufferingIndex_ = (bufferingIndex_ + 1) % (uint32_t)commandPools_.size();

    constexpr uint64_t additionalFrameCount { 2u };
    const auto minAge = device_.GetCommandBufferingCount() + additionalFrameCount;
    const auto ageLimit = (device_.GetFrameCount() < minAge) ? 0 : (device_.GetFrameCount() - minAge);

    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    {
        auto& cache = framebufferCache_.hashToElement;
        for (auto iter = cache.begin(); iter != cache.end();) {
            if (iter->second.frameUseIndex < ageLimit) {
                PLUGIN_ASSERT(iter->second.frameBuffer != VK_NULL_HANDLE);
                vkDestroyFramebuffer(device, iter->second.frameBuffer, nullptr);
                iter = cache.erase(iter);
            } else {
                ++iter;
            }
        }
    }
    {
        auto& cache = renderPassCache_.hashToElement;
        for (auto iter = cache.begin(); iter != cache.end();) {
            if (iter->second.frameUseIndex < ageLimit) {
                PLUGIN_ASSERT(iter->second.renderPass != VK_NULL_HANDLE);
                renderPassCreator_.DestroyRenderPass(device, iter->second.renderPass);
                iter = cache.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

const ContextCommandPoolVk& NodeContextPoolManagerVk::GetContextCommandPool() const
{
    return commandPools_[bufferingIndex_];
}

LowLevelRenderPassDataVk NodeContextPoolManagerVk::GetRenderPassData(
    const RenderCommandBeginRenderPass& beginRenderPass)
{
    LowLevelRenderPassDataVk renderPassData;
    renderPassData.subpassIndex = beginRenderPass.subpassStartIndex;

    // collect render pass attachment compatibility info and default viewport/scissor
    for (uint32_t idx = 0; idx < beginRenderPass.renderPassDesc.attachmentCount; ++idx) {
        const GpuImage* image = gpuResourceMgr_.GetImage(beginRenderPass.renderPassDesc.attachmentHandles[idx]);
        PLUGIN_ASSERT(image);
        if (image) {
            const auto& imageDesc = image->GetDesc();
            renderPassData.renderPassCompatibilityDesc.attachments[idx] = { (VkFormat)imageDesc.format,
                (VkSampleCountFlagBits)imageDesc.sampleCountFlags };
            if (idx == 0) {
                uint32_t maxFbWidth = imageDesc.width;
                uint32_t maxFbHeight = imageDesc.height;
                const auto& attachmentRef = beginRenderPass.renderPassDesc.attachments[idx];
                if ((attachmentRef.mipLevel >= 1) && (attachmentRef.mipLevel < imageDesc.mipCount)) {
                    maxFbWidth = Math::max(1u, maxFbWidth >> attachmentRef.mipLevel);
                    maxFbHeight = Math::max(1u, maxFbHeight >> attachmentRef.mipLevel);
                }
                renderPassData.viewport = { 0.0f, 0.0f, static_cast<float>(maxFbWidth), static_cast<float>(maxFbHeight),
                    0.0f, 1.0f };
                renderPassData.scissor = { { 0, 0 }, { maxFbWidth, maxFbHeight } };
                renderPassData.framebufferSize = { maxFbWidth, maxFbHeight };
            }
        }
    }

    {
        const RenderPassHashes rpHashes =
            HashBeginRenderPass(beginRenderPass, renderPassData.renderPassCompatibilityDesc, gpuResourceMgr_);
        renderPassData.renderPassCompatibilityHash = rpHashes.renderPassCompatibilityHash;
        renderPassData.renderPassHash = rpHashes.renderPassHash;
        renderPassData.frameBufferHash = rpHashes.frameBufferHash;
    }

    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    const uint64_t frameCount = device_.GetFrameCount();

    {
        auto& cache = renderPassCompatibilityCache_;
        if (const auto iter = cache.hashToElement.find(renderPassData.renderPassCompatibilityHash);
            iter != cache.hashToElement.cend()) {
            renderPassData.renderPassCompatibility = iter->second.renderPass;
        } else { // new
            renderPassData.renderPassCompatibility = renderPassCreator_.CreateRenderPassCompatibility(
                deviceVk, beginRenderPass.renderPassDesc, renderPassData, beginRenderPass.subpasses);
            cache.hashToElement[renderPassData.renderPassCompatibilityHash] = { 0,
                renderPassData.renderPassCompatibility };
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
            GpuResourceUtil::DebugObjectNameVk(device_, VK_OBJECT_TYPE_RENDER_PASS,
                VulkanHandleCast<uint64_t>(renderPassData.renderPassCompatibility), debugName_ + "_rp_compatibility");
#endif
        }
    }

    {
        auto& cache = framebufferCache_;
        if (auto iter = cache.hashToElement.find(renderPassData.frameBufferHash); iter != cache.hashToElement.cend()) {
            iter->second.frameUseIndex = frameCount;
            renderPassData.framebuffer = iter->second.frameBuffer;
        } else { // new
            renderPassData.framebuffer = CreateFramebuffer(
                gpuResourceMgr_, beginRenderPass.renderPassDesc, device, renderPassData.renderPassCompatibility);
            cache.hashToElement[renderPassData.frameBufferHash] = { frameCount, renderPassData.framebuffer };
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
            GpuResourceUtil::DebugObjectNameVk(device_, VK_OBJECT_TYPE_FRAMEBUFFER,
                VulkanHandleCast<uint64_t>(renderPassData.framebuffer),
                debugName_ + "_fbo_" + to_string(renderPassData.framebufferSize.width) + "_" +
                    to_string(renderPassData.framebufferSize.height));
#endif
        }
    }

    {
        auto& cache = renderPassCache_;
        if (const auto iter = cache.hashToElement.find(renderPassData.renderPassHash);
            iter != cache.hashToElement.cend()) {
            iter->second.frameUseIndex = frameCount;
            renderPassData.renderPass = iter->second.renderPass;
        } else { // new
            renderPassData.renderPass = renderPassCreator_.CreateRenderPass(deviceVk, beginRenderPass, renderPassData);
            cache.hashToElement[renderPassData.renderPassHash] = { frameCount, renderPassData.renderPass };
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
            GpuResourceUtil::DebugObjectNameVk(device_, VK_OBJECT_TYPE_RENDER_PASS,
                VulkanHandleCast<uint64_t>(renderPassData.renderPass), debugName_ + "_rp");
#endif
        }
    }

    return renderPassData;
}

#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
void NodeContextPoolManagerVk::SetValidationDebugName(const string_view debugName)
{
    debugName_ = debugName;
}
#endif
RENDER_END_NAMESPACE()
