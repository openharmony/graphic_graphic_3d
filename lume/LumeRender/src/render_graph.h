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

#ifndef RENDER_GRAPH_H
#define RENDER_GRAPH_H

#include <cstdint>

#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_handle_util.h"
#include "nodecontext/render_command_list.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuResourceManager;
class RenderBarrierList;

struct RenderNodeGraphNodeStore;
struct RenderNodeContextData;

/**
RenderGraph.
Creates dependencies between resources (used in render nodes) and queues.
Automatically creates transitions and barriers to command lists.
*/
class RenderGraph final {
public:
    explicit RenderGraph(GpuResourceManager& gpuResourceMgr);
    ~RenderGraph() = default;

    RenderGraph(const RenderGraph&) = delete;
    RenderGraph operator=(const RenderGraph&) = delete;

    void BeginFrame();

    /** Process all render nodes and patch needed barriers.
     * backbufferHandle Backbuffer handle for automatic backbuffer/swapchain dependency.
     * renderNodeGraphNodeStore All render node graph render nodes.
     */
    void ProcessRenderNodeGraph(const bool checkBackbufferDependancy,
        const BASE_NS::array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores);

    struct RenderGraphBufferState {
        GpuResourceState state;
        BindableBuffer resource;
        RenderCommandWithType prevRc;
        uint32_t prevRenderNodeIndex { ~0u };
    };
    static constexpr uint32_t MAX_MIP_STATE_COUNT { 16u };
    struct RenderGraphAdditionalImageState {
        BASE_NS::unique_ptr<ImageLayout[]> layouts;
        // NOTE: layers not handled yet
    };
    struct RenderGraphImageState {
        GpuResourceState state;
        BindableImage resource;
        RenderCommandWithType prevRc;
        uint32_t prevRenderNodeIndex { ~0u };
        RenderGraphAdditionalImageState additionalState;
    };
    struct MultiRenderPassStore {
        BASE_NS::vector<RenderCommandBeginRenderPass*> renderPasses;

        // batch barriers to the first render pass
        RenderBarrierList* firstRenderPassBarrierList { nullptr };
        uint32_t firstBarrierPointIndex { ~0u };
        bool supportOpen { false };
    };

    struct GpuQueueTransferState {
        RenderHandle handle;
        uint32_t releaseNodeIdx { 0 };
        uint32_t acquireNodeIdx { 0 };

        ImageLayout optionalReleaseImageLayout { CORE_IMAGE_LAYOUT_UNDEFINED };
        ImageLayout optionalAcquireImageLayout { CORE_IMAGE_LAYOUT_UNDEFINED };
    };

    struct SwapchainStates {
        struct SwapchainState {
            RenderHandle handle;
            // state after render node graph processing
            GpuResourceState state;
            // layout after render node graph processing
            ImageLayout layout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
        };
        BASE_NS::vector<SwapchainState> swapchains;
    };

    /** Get backbuffer resource state after render node graph for further processing.
     * There might different configurations, or state where backbuffer has not been touched, but we want to present.
     */
    SwapchainStates GetSwapchainResourceStates() const;

private:
    struct StateCache {
        MultiRenderPassStore multiRenderPassStore;

        uint32_t nodeCounter { 0u };

        bool checkForBackbufferDependency { false };
        bool usesSwapchainImage { false };
    };
    StateCache stateCache_;

    struct BeginRenderPassParameters {
        RenderCommandBeginRenderPass& rc;
        StateCache& stateCache;
        RenderCommandWithType rpForCmdRef;
    };
    void ProcessRenderNodeGraphNodeStores(
        const BASE_NS::array_view<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores, StateCache& stateCache);
    void ProcessRenderNodeCommands(BASE_NS::array_view<const RenderCommandWithType>& cmdListRef,
        const uint32_t& nodeIdx, RenderNodeContextData& ref, StateCache& stateCache);
    void StoreFinalBufferState();
    // handles backbuffer layouts as well
    void StoreFinalImageState();

    void RenderCommand(const uint32_t renderNodeIndex, const uint32_t commandListCommandIndex,
        RenderNodeContextData& nodeData, RenderCommandBeginRenderPass& rc, StateCache& stateCache);
    void BeginRenderPassHandleDependency(
        BeginRenderPassParameters& params, const uint32_t commandListCommandIndex, RenderNodeContextData& nodeData);
    void BeginRenderPassUpdateImageStates(BeginRenderPassParameters& params, const GpuQueue& gpuQueue,
        BASE_NS::array_view<ImageLayout>& finalImageLayouts, const uint32_t renderNodeIndex);
    void BeginRenderPassUpdateSubpassImageStates(BASE_NS::array_view<const uint32_t> attatchmentIndices,
        const RenderPassDesc& renderPassDesc, const RenderPassAttachmentResourceStates& subpassResourceStatesRef,
        BASE_NS::array_view<ImageLayout> finalImageLayouts, StateCache& stateCache);

    void RenderCommand(const uint32_t renderNodeIndex, const uint32_t commandListCommandIndex,
        const RenderNodeContextData& nodeData, RenderCommandEndRenderPass& rc, StateCache& stateCache);
    void RenderCommand(const uint32_t renderNodeIndex, const uint32_t commandListCommandIndex,
        RenderNodeContextData& nodeData, RenderCommandBarrierPoint& rc, StateCache& stateCache);

    struct ParameterCacheAllocOpt {
        BASE_NS::vector<CommandBarrier> combinedBarriers;
        BASE_NS::unordered_map<RenderHandle, uint32_t> handledCustomBarriers;
    };
    ParameterCacheAllocOpt parameterCachePools_;
    struct ParameterCache {
        BASE_NS::vector<CommandBarrier>& combinedBarriers;
        BASE_NS::unordered_map<RenderHandle, uint32_t>& handledCustomBarriers;
        const uint32_t customBarrierCount;
        const uint32_t vertexInputBarrierCount;
        const uint32_t indirectBufferBarrierCount;
        const uint32_t renderNodeIndex;
        const GpuQueue gpuQueue;
        const RenderCommandWithType rcWithType;
        StateCache& stateCache;
    };
    static void UpdateBufferResourceState(
        RenderGraphBufferState& stateRef, const ParameterCache& params, const CommandBarrier& cb);
    static void UpdateImageResourceState(
        RenderGraphImageState& stateRef, const ParameterCache& params, const CommandBarrier& cb);

    void HandleCustomBarriers(ParameterCache& params, const uint32_t barrierIndexBegin,
        const BASE_NS::array_view<const CommandBarrier>& customBarrierListRef);
    void HandleVertexInputBufferBarriers(ParameterCache& params, const uint32_t barrierIndexBegin,
        const BASE_NS::array_view<const VertexBuffer>& vertexInputBufferBarrierListRef);
    void HandleRenderpassIndirectBufferBarriers(ParameterCache& params, const uint32_t barrierIndexBegin,
        const BASE_NS::array_view<const VertexBuffer>& indirectBufferBarrierListRef);

    void HandleClearImage(ParameterCache& params, const uint32_t& commandListCommandIndex,
        const BASE_NS::array_view<const RenderCommandWithType>& cmdListRef);
    void HandleBlitImage(ParameterCache& params, const uint32_t& commandListCommandIndex,
        const BASE_NS::array_view<const RenderCommandWithType>& cmdListRef);
    void HandleCopyBuffer(ParameterCache& params, const uint32_t& commandListCommandIndex,
        const BASE_NS::array_view<const RenderCommandWithType>& cmdListRef);
    void HandleCopyBufferImage(ParameterCache& params, const uint32_t& commandListCommandIndex,
        const BASE_NS::array_view<const RenderCommandWithType>& cmdListRef);
    void HandleDispatchIndirect(ParameterCache& params, const uint32_t& commandListCommandIndex,
        const BASE_NS::array_view<const RenderCommandWithType>& cmdListRef);

    void HandleDescriptorSets(ParameterCache& params,
        const BASE_NS::array_view<const RenderHandle>& descriptorSetHandlesForBarriers,
        const NodeContextDescriptorSetManager& nodeDescriptorSetMgrRef);

    void UpdateStateAndCreateBarriersGpuImage(
        const GpuResourceState& resourceState, const BindableImage& res, RenderGraph::ParameterCache& params);

    void UpdateStateAndCreateBarriersGpuBuffer(
        const GpuResourceState& resourceState, const BindableBuffer& res, RenderGraph::ParameterCache& params);

    void AddCommandBarrierAndUpdateStateCache(const uint32_t renderNodeIndex,
        const GpuResourceState& newGpuResourceState, const RenderCommandWithType& rcWithType,
        BASE_NS::vector<CommandBarrier>& barriers, BASE_NS::vector<GpuQueueTransferState>& currNodeGpuResourceTransfer);

    void AddCommandBarrierAndUpdateStateCacheBuffer(const uint32_t renderNodeIndex,
        const GpuResourceState& newGpuResourceState, const BindableBuffer& newBuffer,
        const RenderCommandWithType& rcWithType, BASE_NS::vector<CommandBarrier>& barriers,
        BASE_NS::vector<GpuQueueTransferState>& currNodeGpuResourceTransfer);

    void AddCommandBarrierAndUpdateStateCacheImage(const uint32_t renderNodeIndex,
        const GpuResourceState& newGpuResourceState, const BindableImage& newImage,
        const RenderCommandWithType& rcWithType, BASE_NS::vector<CommandBarrier>& barriers,
        BASE_NS::vector<GpuQueueTransferState>& currNodeGpuResourceTransfer);

    // if there's no state ATM, a undefined/invalid starting state is created
    // do not call this method with non dynamic trackable resources
    RenderGraphBufferState& GetBufferResourceStateRef(const RenderHandle handle, const GpuQueue& queue);
    RenderGraphImageState& GetImageResourceStateRef(const RenderHandle handle, const GpuQueue& queue);

    GpuResourceManager& gpuResourceMgr_;

    // stored every time at the end of the frame
    SwapchainStates swapchainStates_;

    // helper to access current render node transfers
    BASE_NS::vector<GpuQueueTransferState> currNodeGpuResourceTransfers_;

    // ~0u is invalid index, i.e. no state tracking
    BASE_NS::vector<uint32_t> gpuBufferDataIndices_;
    // ~0u is invalid index, i.e. no state tracking
    BASE_NS::vector<uint32_t> gpuImageDataIndices_;

    // resources that are tracked
    BASE_NS::vector<RenderGraphBufferState> gpuBufferTracking_;
    BASE_NS::vector<RenderGraphImageState> gpuImageTracking_;

    // available positions from gpuXXXTracking_
    BASE_NS::vector<uint32_t> gpuBufferAvailableIndices_;
    BASE_NS::vector<uint32_t> gpuImageAvailableIndices_;

    RenderGraphBufferState defaultBufferState_ {};
    RenderGraphImageState defaultImageState_ {};
};
RENDER_END_NAMESPACE()

#endif // RENDER_GRAPH_H
