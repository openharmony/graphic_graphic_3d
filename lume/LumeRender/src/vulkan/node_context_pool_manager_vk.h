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

#ifndef VULKAN_NODE_CONTEXT_POOL_MANAGER_VK_H
#define VULKAN_NODE_CONTEXT_POOL_MANAGER_VK_H

#include <cstdint>
#include <vulkan/vulkan_core.h>

#include <base/containers/string.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

#include "nodecontext/node_context_pool_manager.h"
#include "vulkan/pipeline_create_functions_vk.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuResourceManager;
struct RenderCommandBeginRenderPass;
struct RenderPassAttachmentCompatibilityDesc;

struct LowLevelCommandBufferVk {
    VkCommandBuffer commandBuffer { VK_NULL_HANDLE };
    VkSemaphore semaphore { VK_NULL_HANDLE };
};

struct ContextCommandPoolVk {
    // every command buffer is taken from the single pool and pre-allocated
    VkCommandPool commandPool { VK_NULL_HANDLE };
    LowLevelCommandBufferVk commandBuffer;
};

struct ContextFramebufferCacheVk {
    struct CacheValues {
        uint64_t frameUseIndex { 0 };
        VkFramebuffer frameBuffer { VK_NULL_HANDLE };
    };
    BASE_NS::unordered_map<uint64_t, CacheValues> hashToElement;
};

struct ContextRenderPassCacheVk {
    struct CacheValues {
        uint64_t frameUseIndex { 0 };
        VkRenderPass renderPass { VK_NULL_HANDLE };
    };
    BASE_NS::unordered_map<uint64_t, CacheValues> hashToElement;
};

class NodeContextPoolManagerVk final : public NodeContextPoolManager {
public:
    explicit NodeContextPoolManagerVk(Device& device, GpuResourceManager& gpuResourceManager, const GpuQueue& gpuQueue);
    ~NodeContextPoolManagerVk();

    void BeginFrame() override;
    void BeginBackendFrame() override;

    const ContextCommandPoolVk& GetContextCommandPool() const;
    const ContextCommandPoolVk& GetContextSecondaryCommandPool() const;

    LowLevelRenderPassDataVk GetRenderPassData(const RenderCommandBeginRenderPass& beginRenderPass);

private:
    Device& device_;
    GpuResourceManager& gpuResourceMgr_;
    const GpuQueue gpuQueue_ {};

    uint32_t bufferingIndex_ { 0 };

    BASE_NS::vector<ContextCommandPoolVk> commandPools_;
    BASE_NS::vector<ContextCommandPoolVk> commandSecondaryPools_;
    ContextFramebufferCacheVk framebufferCache_;
    ContextRenderPassCacheVk renderPassCache_;
    ContextRenderPassCacheVk renderPassCompatibilityCache_;
    RenderPassCreatorVk renderPassCreator_;
#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    void SetValidationDebugName(const BASE_NS::string_view debugName) override;
    BASE_NS::string debugName_;
    bool firstFrame_ { true };
#endif
#if (RENDER_VALIDATION_ENABLED == 1)
    uint64_t frameIndexFront_ { 0 };
    uint64_t frameIndexBack_ { 0 };
#endif
};
RENDER_END_NAMESPACE()

#endif // VULKAN_NODE_CONTEXT_POOL_MANAGER_VK_H
