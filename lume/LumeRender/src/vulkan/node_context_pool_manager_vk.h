/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
    bool isRecorded { false };
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

    const ContextCommandPoolVk& GetContextCommandPool() const;

    LowLevelRenderPassDataVk GetRenderPassData(const RenderCommandBeginRenderPass& beginRenderPass);

private:
    Device& device_;
    GpuResourceManager& gpuResourceMgr_;

    uint32_t bufferingIndex_ { 0 };

    BASE_NS::vector<ContextCommandPoolVk> commandPools_;
    ContextFramebufferCacheVk framebufferCache_;
    ContextRenderPassCacheVk renderPassCache_;
    ContextRenderPassCacheVk renderPassCompatibilityCache_;
    RenderPassCreatorVk renderPassCreator_;
#if ((RENDER_VALIDATION_ENABLED == 1) || (RENDER_VULKAN_VALIDATION_ENABLED == 1))
    void SetValidationDebugName(const BASE_NS::string_view debugName) override;
    BASE_NS::string debugName_;
    bool firstFrame_ { true };
#endif
};
RENDER_END_NAMESPACE()

#endif // VULKAN_NODE_CONTEXT_POOL_MANAGER_VK_H
