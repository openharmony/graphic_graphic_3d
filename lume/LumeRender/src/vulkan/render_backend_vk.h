/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_RENDER_BACKEND_VK_H
#define VULKAN_RENDER_BACKEND_VK_H

#include <vulkan/vulkan_core.h>

#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/threading/intf_thread_pool.h>
#include <render/namespace.h>

#include "nodecontext/render_command_list.h"
#include "render_backend.h"
#include "vulkan/device_vk.h"
#include "vulkan/pipeline_create_functions_vk.h"

#if (RENDER_PERF_ENABLED == 1)
#include "device/gpu_buffer.h"
#include "perf/cpu_timer.h"
#include "perf/gpu_query_manager.h"
#endif

CORE_BEGIN_NAMESPACE()
class IThreadPool;
CORE_END_NAMESPACE()
RENDER_BEGIN_NAMESPACE()
class Device;
class DeviceVk;
class GpuResourceManager;
class NodeContextPsoManager;
class NodeContextPoolManager;
class RenderBarrierList;
class RenderCommandList;
struct LowLevelCommandBufferVk;
struct RenderCommandContext;
struct RenderPerfTimings;

struct NodeGraphBackBufferConfiguration;

class IRenderBackendNode;

struct CommandBufferSubmitter {
    struct CommandBuffer {
        VkCommandBuffer commandBuffer { VK_NULL_HANDLE };
        VkSemaphore semaphore { VK_NULL_HANDLE };
    };

    VkSemaphore presentationWaitSemaphore { VK_NULL_HANDLE };
    BASE_NS::vector<CommandBuffer> commandBuffers;
};

#if (RENDER_PERF_ENABLED == 1)
struct PerfCounters {
    uint32_t drawCount { 0u };
    uint32_t drawIndirectCount { 0u };
    uint32_t dispatchCount { 0u };
    uint32_t dispatchIndirectCount { 0u };

    uint32_t bindPipelineCount { 0u };
    uint32_t renderPassCount { 0u };

    uint32_t updateDescriptorSetCount { 0u };
    uint32_t bindDescriptorSetCount { 0u };

    uint32_t triangleCount { 0u };
    uint32_t instanceCount { 0u };
};
#endif

/**
RenderBackVk.
Vulkan render backend.
Gets a list of intermediate render command lists and creates a Vulkan command buffers in parallel.
Generally one render command list will generate one vulkan command buffer.
**/
class RenderBackendVk final : public RenderBackend {
public:
    RenderBackendVk(Device& dev, GpuResourceManager& gpuResourceManager, const CORE_NS::IParallelTaskQueue::Ptr& queue);
    ~RenderBackendVk() = default;

    void Render(RenderCommandFrameData& renderCommandFrameData,
        const RenderBackendBackBufferConfiguration& backBufferConfig) override;
    void Present(const RenderBackendBackBufferConfiguration& backBufferConfig) override;

private:
    struct StateCache {
        const RenderCommandBeginRenderPass* renderCommandBeginRenderPass { nullptr };
        LowLevelRenderPassDataVk lowLevelRenderPassData;
        LowLevelPipelineLayoutDataVk lowLevelPipelineLayoutData;

        IRenderBackendNode* backendNode { nullptr };

        // matching pso handle to pipeline layout
        RenderHandle psoHandle;
        VkPipelineLayout pipelineLayout { VK_NULL_HANDLE };

        // has bitmask for immutable samplers with special handling needed resources
        // every descriptor set uses 16 bits (for max descriptor set bindings)
        uint64_t pipelineDescSetHash { 0u };

#if (RENDER_PERF_ENABLED == 1)
        mutable PerfCounters perfCounters;
#endif
    };

    struct MultiRenderCommandListDesc {
        RenderCommandContext* baseContext { nullptr };

        uint32_t multiRenderCommandListIndex { 0 };
        uint32_t multiRenderCommandListCount { 0 };
    };

    struct DebugNames {
        BASE_NS::string_view renderCommandListName;
        BASE_NS::string_view renderCommandBufferName; // multi-renderpass has the first render command list name
    };

    // called once in the beginning of rendering to acquire and setup possible swapchain
    void AcquirePresentationInfo(
        RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig);

    void RenderProcessCommandLists(RenderCommandFrameData& renderCommandFrameData);
    void RenderProcessSubmitCommandLists(
        RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig);
    void RenderSingleCommandList(RenderCommandContext& renderCommandCtx, const uint32_t cmdBufIdx,
        const MultiRenderCommandListDesc& multiRenderCommandListDesc, const DebugNames& debugNames);

    void RenderCommand(const RenderCommandDraw& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDrawIndirect& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDispatch& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDispatchIndirect& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    void RenderCommand(const RenderCommandBindPipeline& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, StateCache& stateCache);
    void RenderCommand(const RenderCommandBindVertexBuffers& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandBindIndexBuffer& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    void RenderCommand(const RenderCommandBeginRenderPass& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, StateCache& stateCache);
    void RenderCommand(const RenderCommandNextSubpass& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandEndRenderPass& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, StateCache& stateCache);

    void RenderCommand(const RenderCommandCopyBuffer& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandCopyBufferImage& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandCopyImage& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    void RenderCommand(const RenderCommandBarrierPoint& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache,
        const RenderBarrierList& rbl);

    void RenderCommand(const RenderCommandBlitImage& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    void RenderCommand(const RenderCommandUpdateDescriptorSets& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache,
        NodeContextDescriptorSetManager& ncdsm);
    void RenderCommand(const RenderCommandBindDescriptorSets& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, StateCache& stateCache,
        NodeContextDescriptorSetManager& ncdsm);

    void RenderCommand(const RenderCommandPushConstant& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    void RenderCommand(const RenderCommandBuildAccelerationStructure& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    // dynamic states
    void RenderCommand(const RenderCommandDynamicStateViewport& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDynamicStateScissor& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDynamicStateLineWidth& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDynamicStateDepthBias& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDynamicStateBlendConstants& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDynamicStateDepthBounds& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);
    void RenderCommand(const RenderCommandDynamicStateStencil& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    void RenderCommand(const RenderCommandExecuteBackendFramePosition& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    // queries
    void RenderCommand(const RenderCommandWriteTimestamp& renderCmd, const LowLevelCommandBufferVk& cmdBuf,
        NodeContextPsoManager& psoMgr, const NodeContextPoolManager& poolMgr, const StateCache& stateCache);

    Device& device_;
    DeviceVk& deviceVk_;
    GpuResourceManager& gpuResourceMgr_;
    CORE_NS::IParallelTaskQueue* queue_;

    CommandBufferSubmitter commandBufferSubmitter_;

    struct PresentationInfo {
        bool useSwapchain { false };
        bool validAcquire { false };
        uint32_t swapchainImageIndex { ~0u };
        VkSemaphore swapchainSemaphore { VK_NULL_HANDLE };
        bool presentationLayoutChangeNeeded { false };
        uint32_t renderNodeCommandListIndex { ~0u };
        GpuResourceState renderGraphProcessedState;
        ImageLayout imageLayout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
        VkImage swapchainImage { VK_NULL_HANDLE };
    };
    PresentationInfo presentationInfo_;

    // internal presentationInfo_ state change (called 0 or 1 time from random thread for presentation layout change)
    void RenderPresentationLayout(const LowLevelCommandBufferVk& cmdBuf);

#if (RENDER_PERF_ENABLED == 1)

    void StartFrameTimers(RenderCommandFrameData& aRenderCommandFrameData);
    void EndFrameTimers();

    void WritePerfTimeStamp(const LowLevelCommandBufferVk& cmdBuf, const BASE_NS::string_view name,
        const uint32_t queryIndex, const VkPipelineStageFlagBits stageFlagBits);
    void CopyPerfTimeStamp(
        const LowLevelCommandBufferVk& cmdBuf, const BASE_NS::string_view name, const StateCache& cache);

    BASE_NS::unique_ptr<GpuQueryManager> gpuQueryMgr_;
    struct PerfDataSet {
        EngineResourceHandle gpuHandle;
        CpuTimer cpuTimer;

        uint32_t gpuBufferOffset { 0 };
    };
    BASE_NS::unordered_map<BASE_NS::string, PerfDataSet> timers_;
#if (RENDER_GPU_TIMESTAMP_QUERIES_ENABLED == 1)
    struct PerfGpuTimerData {
        uint32_t fullByteSize { 0 };
        uint32_t frameByteSize { 0 };
        uint32_t currentOffset { 0 };
        BASE_NS::unique_ptr<GpuBuffer> gpuBuffer;
        void* mappedData { nullptr };
    };
    PerfGpuTimerData perfGpuTimerData_;
#endif
    struct CommonBackendCpuTimers {
        CpuTimer full;
        CpuTimer acquire;
        CpuTimer execute;
        CpuTimer submit;
        CpuTimer present;
    };
    CommonBackendCpuTimers commonCpuTimers_;
#endif
};
RENDER_END_NAMESPACE()

#endif // VULKAN_RENDER_BACKEND_VK_H
