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

#include "node_context_pool_manager_vk.h"

#include <cstdint>
#include <vulkan/vulkan_core.h>

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
    if (subpass.viewMask > 1U) {
        HashCombine(seed, subpass.viewMask);
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
    const LowLevelRenderPassCompatibilityDescVk& renderPassCompatibilityDesc, const RenderPassSubpassDesc& subpasses,
    const RenderPassAttachmentResourceStates& intputResourceStates)
{
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        const LowLevelRenderPassCompatibilityDescVk::Attachment& atCompatibilityDesc =
            renderPassCompatibilityDesc.attachments[idx];
        HashCombine(hash, static_cast<uint64_t>(atCompatibilityDesc.format),
            static_cast<uint64_t>(atCompatibilityDesc.sampleCountFlags));
        // render pass needs have matching stage masks
        HashCombine(hash, static_cast<uint64_t>(intputResourceStates.states[idx].pipelineStageFlags));
        if (subpasses.viewMask > 1U) {
            // with multi-view extension, renderpass updated for all mips
            HashCombine(hash, (static_cast<uint64_t>(renderPassDesc.attachments[idx].layer) << 32ULL) |
                                  (static_cast<uint64_t>(renderPassDesc.attachments[idx].mipLevel)));
        }
    }
    // NOTE: subpass resources states are not hashed
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

inline void HashRenderPassOps(uint64_t& hash, const RenderPassDesc& renderPassDesc)
{
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        const auto& attachRef = renderPassDesc.attachments[idx];
        const uint64_t opHash = (static_cast<uint64_t>(attachRef.loadOp) << 48ULL) |
                                (static_cast<uint64_t>(attachRef.storeOp) << 32ULL) |
                                (static_cast<uint64_t>(attachRef.stencilLoadOp) << 16ULL) |
                                (static_cast<uint64_t>(attachRef.stencilStoreOp));
        HashCombine(hash, opHash);
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
        beginRenderPass.subpasses[0], beginRenderPass.inputResourceStates);

    rpHashes.renderPassHash = rpHashes.renderPassCompatibilityHash; // for starting point
    HashRenderPassLayouts(rpHashes.renderPassHash, renderPassDesc, beginRenderPass.imageLayouts);
    HashRenderPassOps(rpHashes.renderPassHash, renderPassDesc);

    rpHashes.frameBufferHash = rpHashes.renderPassCompatibilityHash; // depends on the compatible render pass
    HashFramebuffer(rpHashes.frameBufferHash, renderPassDesc, gpuResourceMgr);

    return rpHashes;
}

VkFramebuffer CreateFramebuffer(const GpuResourceManager& gpuResourceMgr, const RenderPassDesc& renderPassDesc,
    const LowLevelRenderPassDataVk& renderPassData, const VkDevice device)
{
    const uint32_t attachmentCount = renderPassDesc.attachmentCount;
    PLUGIN_ASSERT(attachmentCount <= PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT);

    // the size is taken from the render pass data
    // there might e.g. fragment shading rate images whose size differ
    FBSize size { renderPassData.framebufferSize.width, renderPassData.framebufferSize.height, 1u };
    VkImageView imageViews[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT];
    uint32_t viewIndex = 0;

    bool validImageViews = true;
    for (uint32_t idx = 0; idx < attachmentCount; ++idx) {
        const RenderHandle handle = renderPassDesc.attachmentHandles[idx];
        const RenderPassDesc::AttachmentDesc& attachmentDesc = renderPassDesc.attachments[idx];
        if (const GpuImageVk* image = gpuResourceMgr.GetImage<GpuImageVk>(handle); image) {
            const GpuImagePlatformDataVk& plat = image->GetPlatformData();
            const GpuImagePlatformDataViewsVk& imagePlat = image->GetPlatformDataViews();
            imageViews[viewIndex] = plat.imageViewBase;
            if ((renderPassData.viewMask > 1u) && (plat.arrayLayers > 1u)) {
                // multi-view, we select the view with all the layers, but the layers count is 1
                if ((!imagePlat.mipImageAllLayerViews.empty()) &&
                    (attachmentDesc.mipLevel < static_cast<uint32_t>(imagePlat.mipImageAllLayerViews.size()))) {
                    imageViews[viewIndex] = imagePlat.mipImageAllLayerViews[attachmentDesc.mipLevel];
                } else {
                    imageViews[viewIndex] = plat.imageView;
                }
                size.layers = 1u;
            } else if ((attachmentDesc.mipLevel >= 1) && (attachmentDesc.mipLevel < imagePlat.mipImageViews.size())) {
                imageViews[viewIndex] = imagePlat.mipImageViews[attachmentDesc.mipLevel];
            } else if ((attachmentDesc.layer >= 1) && (attachmentDesc.layer < imagePlat.layerImageViews.size())) {
                imageViews[viewIndex] = imagePlat.layerImageViews[attachmentDesc.layer];
            }
            viewIndex++;
        }
        if (!imageViews[idx]) {
            validImageViews = false;
        }
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!validImageViews || (viewIndex != attachmentCount)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid image attachment in FBO creation");
    }
#endif
    VkFramebuffer framebuffer = VK_NULL_HANDLE;
    if (validImageViews && (viewIndex == attachmentCount)) {
        const VkFramebufferCreateInfo framebufferCreateInfo {
            VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO, // sType
            nullptr,                                   // pNext
            VkFramebufferCreateFlags { 0 },            // flags
            renderPassData.renderPassCompatibility,    // renderPass
            attachmentCount,                           // attachmentCount
            imageViews,                                // pAttachments
            size.width,                                // width
            size.height,                               // height
            size.layers,                               // layers
        };

        VALIDATE_VK_RESULT(vkCreateFramebuffer(device, // device
            &framebufferCreateInfo,                    // pCreateInfo
            nullptr,                                   // pAllocator
            &framebuffer));                            // pFramebuffer
    }

    return framebuffer;
}

ContextCommandPoolVk CreateContextCommandPool(
    const VkDevice device, const VkCommandBufferLevel cmdBufferLevel, const uint32_t queueFamilyIndex)
{
    constexpr VkCommandPoolCreateFlags commandPoolCreateFlags { 0u };
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

    ContextCommandPoolVk ctxPool;
    VALIDATE_VK_RESULT(vkCreateCommandPool(device, // device
        &commandPoolCreateInfo,                    // pCreateInfo
        nullptr,                                   // pAllocator
        &ctxPool.commandPool));                    // pCommandPool

    // pre-create command buffers and semaphores
    const VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO, // sType
        nullptr,                                        // pNext
        ctxPool.commandPool,                            // commandPool
        cmdBufferLevel,                                 // level
        1,                                              // commandBufferCount
    };

    VALIDATE_VK_RESULT(vkAllocateCommandBuffers(device, // device
        &commandBufferAllocateInfo,                     // pAllocateInfo
        &ctxPool.commandBuffer.commandBuffer));         // pCommandBuffers

    VALIDATE_VK_RESULT(vkCreateSemaphore(device, // device
        &semaphoreCreateInfo,                    // pCreateInfo
        nullptr,                                 // pAllocator
        &ctxPool.commandBuffer.semaphore));      // pSemaphore

    return ctxPool;
}
} // namespace

NodeContextPoolManagerVk::NodeContextPoolManagerVk(
    Device& device, GpuResourceManager& gpuResourceManager, const GpuQueue& gpuQueue)
    : NodeContextPoolManager(), device_ { device }, gpuResourceMgr_ { gpuResourceManager }, gpuQueue_(gpuQueue)
{
    const DeviceVk& deviceVk = static_cast<const DeviceVk&>(device_);
    const VkDevice vkDevice = static_cast<const DevicePlatformDataVk&>(device_.GetPlatformData()).device;

    const LowLevelGpuQueueVk lowLevelGpuQueue = deviceVk.GetGpuQueue(gpuQueue);
    const uint32_t bufferingCount = device_.GetCommandBufferingCount();
    if (bufferingCount > 0) {
        // prepare and create command buffers
        commandPools_.resize(bufferingCount);
        commandSecondaryPools_.resize(bufferingCount);
        const uint32_t queueFamilyIndex = lowLevelGpuQueue.queueInfo.queueFamilyIndex;
        for (uint32_t frameIdx = 0; frameIdx < commandPools_.size(); ++frameIdx) {
            commandPools_[frameIdx] = CreateContextCommandPool(
                vkDevice, VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_PRIMARY, queueFamilyIndex);
            commandSecondaryPools_[frameIdx] = CreateContextCommandPool(
                vkDevice, VkCommandBufferLevel::VK_COMMAND_BUFFER_LEVEL_SECONDARY, queueFamilyIndex);
        }
        // NOTE: cmd buffers tagged in first beginFrame
    }
}

NodeContextPoolManagerVk::~NodeContextPoolManagerVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;

    auto DestroyContextCommandPool = [](const auto& device, const auto& commandPools) {
        for (auto& cmdPoolRef : commandPools) {
            if (cmdPoolRef.commandBuffer.semaphore) {
                vkDestroySemaphore(device,              // device
                    cmdPoolRef.commandBuffer.semaphore, // semaphore
                    nullptr);                           // pAllocator
            }
            if (cmdPoolRef.commandPool) {
                vkDestroyCommandPool(device, // device
                    cmdPoolRef.commandPool,  // commandPool
                    nullptr);                // pAllocator
            }
        }
    };
    DestroyContextCommandPool(device, commandPools_);
    DestroyContextCommandPool(device, commandSecondaryPools_);

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
#if (RENDER_VALIDATION_ENABLED == 1)
    frameIndexFront_ = device_.GetFrameCount();
#endif
}

void NodeContextPoolManagerVk::BeginBackendFrame()
{
    const uint64_t frameCount = device_.GetFrameCount();

#if (RENDER_VALIDATION_ENABLED == 1)
    PLUGIN_ASSERT(frameIndexBack_ != frameCount); // prevent multiple calls per frame
    frameIndexBack_ = frameCount;
    PLUGIN_ASSERT(frameIndexFront_ == frameIndexBack_);
#endif
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    if (firstFrame_) {
        firstFrame_ = false;
        for (const auto& cmdPoolRef : commandPools_) {
            GpuResourceUtil::DebugObjectNameVk(device_, VK_OBJECT_TYPE_COMMAND_BUFFER,
                VulkanHandleCast<uint64_t>(cmdPoolRef.commandBuffer.commandBuffer), debugName_ + "_cmd_buf");
        }
        // TODO: deferred creation
        for (const auto& cmdPoolRef : commandSecondaryPools_) {
            GpuResourceUtil::DebugObjectNameVk(device_, VK_OBJECT_TYPE_COMMAND_BUFFER,
                VulkanHandleCast<uint64_t>(cmdPoolRef.commandBuffer.commandBuffer), debugName_ + "_secondary_cmd_buf");
        }
    }
#endif

    bufferingIndex_ = (bufferingIndex_ + 1) % (uint32_t)commandPools_.size();

    constexpr uint64_t additionalFrameCount { 2u };
    const auto minAge = device_.GetCommandBufferingCount() + additionalFrameCount;
    const auto ageLimit = (frameCount < minAge) ? 0 : (frameCount - minAge);

    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    {
        auto& cache = framebufferCache_.hashToElement;
        for (auto iter = cache.begin(); iter != cache.end();) {
            if (iter->second.frameUseIndex < ageLimit) {
                if (iter->second.frameBuffer) {
                    vkDestroyFramebuffer(device, iter->second.frameBuffer, nullptr);
                }
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
                if (iter->second.renderPass) {
                    renderPassCreator_.DestroyRenderPass(device, iter->second.renderPass);
                }
                iter = cache.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

const ContextCommandPoolVk& NodeContextPoolManagerVk::GetContextCommandPool() const
{
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    PLUGIN_ASSERT(frameIndexFront_ == frameIndexBack_);
#endif
    return commandPools_[bufferingIndex_];
}

const ContextCommandPoolVk& NodeContextPoolManagerVk::GetContextSecondaryCommandPool() const
{
#if (RENDER_VULKAN_VALIDATION_ENABLED == 1)
    PLUGIN_ASSERT(frameIndexFront_ == frameIndexBack_);
#endif
    PLUGIN_ASSERT(bufferingIndex_ < static_cast<uint32_t>(commandSecondaryPools_.size()));
    return commandSecondaryPools_[bufferingIndex_];
}

LowLevelRenderPassDataVk NodeContextPoolManagerVk::GetRenderPassData(
    const RenderCommandBeginRenderPass& beginRenderPass)
{
    LowLevelRenderPassDataVk renderPassData;
    renderPassData.subpassIndex = beginRenderPass.subpassStartIndex;

    PLUGIN_ASSERT(renderPassData.subpassIndex < static_cast<uint32_t>(beginRenderPass.subpasses.size()));
    const DeviceVk& deviceVk = (const DeviceVk&)device_;
    if (deviceVk.GetCommonDeviceExtensions().multiView) {
        renderPassData.viewMask = beginRenderPass.subpasses[renderPassData.subpassIndex].viewMask;
    }

    // collect render pass attachment compatibility info and default viewport/scissor
    for (uint32_t idx = 0; idx < beginRenderPass.renderPassDesc.attachmentCount; ++idx) {
        if (const GpuImageVk* image =
                gpuResourceMgr_.GetImage<const GpuImageVk>(beginRenderPass.renderPassDesc.attachmentHandles[idx]);
            image) {
            const auto& platData = image->GetPlatformData();
            renderPassData.renderPassCompatibilityDesc.attachments[idx] = { platData.format, platData.samples,
                platData.aspectFlags };
            if (idx == 0) {
                uint32_t maxFbWidth = platData.extent.width;
                uint32_t maxFbHeight = platData.extent.height;
                const auto& attachmentRef = beginRenderPass.renderPassDesc.attachments[idx];
                if ((attachmentRef.mipLevel >= 1) && (attachmentRef.mipLevel < platData.mipLevels)) {
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

    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    const uint64_t frameCount = device_.GetFrameCount();

    {
        auto& cache = renderPassCompatibilityCache_;
        if (const auto iter = cache.hashToElement.find(renderPassData.renderPassCompatibilityHash);
            iter != cache.hashToElement.cend()) {
            renderPassData.renderPassCompatibility = iter->second.renderPass;
        } else { // new
            renderPassData.renderPassCompatibility =
                renderPassCreator_.CreateRenderPassCompatibility(deviceVk, beginRenderPass, renderPassData);
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
            renderPassData.framebuffer =
                CreateFramebuffer(gpuResourceMgr_, beginRenderPass.renderPassDesc, renderPassData, device);
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
