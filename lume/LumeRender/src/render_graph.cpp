/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "render_graph.h"

#include <cinttypes>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/math/mathf.h>
#include <render/namespace.h>

#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "nodecontext/render_barrier_list.h"
#include "nodecontext/render_command_list.h"
#include "nodecontext/render_node_graph_node_store.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t INVALID_TRACK_IDX { ~0u };

#if (RENDER_DEV_ENABLED == 1)
constexpr const bool CORE_RENDER_GRAPH_FULL_DEBUG_PRINT = false;
constexpr const bool CORE_RENDER_GRAPH_FULL_DEBUG_ATTACHMENTS = false;
constexpr const bool CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES = false;

void DebugPrintCommandListCommand(const RenderCommandWithType& rc, GpuResourceManager& aMgr)
{
    switch (rc.type) {
        case RenderCommandType::BARRIER_POINT: {
            PLUGIN_LOG_I("rc: BarrierPoint");
            break;
        }
        case RenderCommandType::DRAW: {
            PLUGIN_LOG_I("rc: Draw");
            break;
        }
        case RenderCommandType::DRAW_INDIRECT: {
            PLUGIN_LOG_I("rc: DrawIndirect");
            break;
        }
        case RenderCommandType::DISPATCH: {
            PLUGIN_LOG_I("rc: Dispatch");
            break;
        }
        case RenderCommandType::DISPATCH_INDIRECT: {
            PLUGIN_LOG_I("rc: DispatchIndirect");
            break;
        }
        case RenderCommandType::BIND_PIPELINE: {
            PLUGIN_LOG_I("rc: BindPipeline");
            break;
        }
        case RenderCommandType::BEGIN_RENDER_PASS: {
            PLUGIN_LOG_I("rc: BeginRenderPass");
            if constexpr (CORE_RENDER_GRAPH_FULL_DEBUG_ATTACHMENTS) {
                const auto& beginRenderPass = *static_cast<RenderCommandBeginRenderPass*>(rc.rc);
                for (uint32_t idx = 0; idx < beginRenderPass.renderPassDesc.attachmentCount; ++idx) {
                    const RenderHandle handle = beginRenderPass.renderPassDesc.attachmentHandles[idx];
                    PLUGIN_LOG_I("    attachment idx: %u name: %s", idx, aMgr.GetName(handle).c_str());
                }
                PLUGIN_LOG_I("    subpass count: %u, subpass start idx: %u",
                    (uint32_t)beginRenderPass.renderPassDesc.subpassCount, beginRenderPass.subpassStartIndex);
            }
            break;
        }
        case RenderCommandType::NEXT_SUBPASS: {
            PLUGIN_LOG_I("rc: NextSubpass");
            break;
        }
        case RenderCommandType::END_RENDER_PASS: {
            PLUGIN_LOG_I("rc: EndRenderPass");
            break;
        }
        case RenderCommandType::BIND_VERTEX_BUFFERS: {
            PLUGIN_LOG_I("rc: BindVertexBuffers");
            break;
        }
        case RenderCommandType::BIND_INDEX_BUFFER: {
            PLUGIN_LOG_I("rc: BindIndexBuffer");
            break;
        }
        case RenderCommandType::COPY_BUFFER: {
            PLUGIN_LOG_I("rc: CopyBuffer");
            break;
        }
        case RenderCommandType::COPY_BUFFER_IMAGE: {
            PLUGIN_LOG_I("rc: CopyBufferImage");
            break;
        }
        case RenderCommandType::UPDATE_DESCRIPTOR_SETS: {
            PLUGIN_LOG_I("rc: UpdateDescriptorSets");
            break;
        }
        case RenderCommandType::BIND_DESCRIPTOR_SETS: {
            PLUGIN_LOG_I("rc: BindDescriptorSets");
            break;
        }
        case RenderCommandType::PUSH_CONSTANT: {
            PLUGIN_LOG_I("rc: PushConstant");
            break;
        }
        case RenderCommandType::BLIT_IMAGE: {
            PLUGIN_LOG_I("rc: BlitImage");
            break;
        }
            // dynamic states
        case RenderCommandType::DYNAMIC_STATE_VIEWPORT: {
            PLUGIN_LOG_I("rc: DynamicStateViewport");
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_SCISSOR: {
            PLUGIN_LOG_I("rc: DynamicStateScissor");
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_LINE_WIDTH: {
            PLUGIN_LOG_I("rc: DynamicStateLineWidth");
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_DEPTH_BIAS: {
            PLUGIN_LOG_I("rc: DynamicStateDepthBias");
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_BLEND_CONSTANTS: {
            PLUGIN_LOG_I("rc: DynamicStateBlendConstants");
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_DEPTH_BOUNDS: {
            PLUGIN_LOG_I("rc: DynamicStateDepthBounds");
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_STENCIL: {
            PLUGIN_LOG_I("rc: DynamicStateStencil");
            break;
        }
        case RenderCommandType::WRITE_TIMESTAMP: {
            PLUGIN_LOG_I("rc: WriteTimestamp");
            break;
        }
        case RenderCommandType::GPU_QUEUE_TRANSFER_RELEASE: {
            PLUGIN_LOG_I("rc: GpuQueueTransferRelease");
            break;
        }
        case RenderCommandType::GPU_QUEUE_TRANSFER_ACQUIRE: {
            PLUGIN_LOG_I("rc: GpuQueueTransferAcquire");
            break;
        }
        case RenderCommandType::UNDEFINED:
        default: {
            PLUGIN_ASSERT(false && "non-valid render command");
            break;
        }
    }
}

void DebugBarrierPrint(const GpuResourceManager& gpuResourceMgr, const vector<CommandBarrier>& combinedBarriers)
{
    PLUGIN_ASSERT(CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES); // do not call this function normally
    for (const auto& ref : combinedBarriers) {
        const RenderHandleType type = RenderHandleUtil::GetHandleType(ref.resourceHandle);
        if (type == RenderHandleType::GPU_BUFFER) {
            PLUGIN_LOG_I("barrier buffer    :: handle:0x%" PRIx64 " name:%s, src_stage:%u dst_stage:%u",
                ref.resourceHandle.id, gpuResourceMgr.GetName(ref.resourceHandle).c_str(), ref.src.pipelineStageFlags,
                ref.dst.pipelineStageFlags);
        } else {
            PLUGIN_ASSERT(type == RenderHandleType::GPU_IMAGE);
            PLUGIN_LOG_I("barrier image     :: handle:0x%" PRIx64
                         " name:%s, src_stage:%u dst_stage:%u, src_layout:%u dst_layout:%u",
                ref.resourceHandle.id, gpuResourceMgr.GetName(ref.resourceHandle).c_str(), ref.src.pipelineStageFlags,
                ref.dst.pipelineStageFlags, ref.src.optionalImageLayout, ref.dst.optionalImageLayout);
        }
    }
}

void DebugRenderPassLayoutPrint(const GpuResourceManager& gpuResourceMgr, const RenderCommandBeginRenderPass& rc)
{
    PLUGIN_ASSERT(CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES); // do not call this function normally
    for (uint32_t idx = 0; idx < rc.renderPassDesc.attachmentCount; ++idx) {
        const auto handle = rc.renderPassDesc.attachmentHandles[idx];
        const auto srcLayout = rc.imageLayouts.attachmentInitialLayouts[idx];
        const auto dstLayout = rc.imageLayouts.attachmentFinalLayouts[idx];
        PLUGIN_LOG_I("render_pass image :: handle:0x%" PRIx64 " name:%s, src_layout:%u dst_layout:%u (patched later)",
            handle.id, gpuResourceMgr.GetName(handle).c_str(), srcLayout, dstLayout);
    }
}

void DebugPrintImageState(const GpuResourceManager& gpuResourceMgr, const RenderGraph::RenderGraphImageState& resState)
{
    PLUGIN_ASSERT(CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES); // do not call this function normally
    const EngineResourceHandle gpuHandle = gpuResourceMgr.GetGpuHandle(resState.resource.handle);
    PLUGIN_LOG_I("image_state   :: handle:0x%" PRIx64 " name:%s, layout:%u, index:%u, gen:%u, gpu_gen:%u",
        resState.resource.handle.id, gpuResourceMgr.GetName(resState.resource.handle).c_str(),
        resState.resource.imageLayout, RenderHandleUtil::GetIndexPart(resState.resource.handle),
        RenderHandleUtil::GetGenerationIndexPart(resState.resource.handle),
        RenderHandleUtil::GetGenerationIndexPart(gpuHandle));
    // one could fetch and print vulkan handle here as well e.g.
    // 1. const GpuImagePlatformDataVk& plat = (const
    // 2. GpuImagePlatformDataVk&)gpuResourceMgr.GetImage(ref.first)->GetBasePlatformData();
    // 3. PLUGIN_LOG_I("end_frame image   :: vk_handle:0x%" PRIx64, VulkanHandleCast<uint64_t>(plat.image));
}
#endif // RENDER_DEV_ENABLED

static constexpr uint32_t WRITE_ACCESS_FLAGS = CORE_ACCESS_SHADER_WRITE_BIT | CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                                               CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
                                               CORE_ACCESS_TRANSFER_WRITE_BIT | CORE_ACCESS_HOST_WRITE_BIT |
                                               CORE_ACCESS_MEMORY_WRITE_BIT;

void PatchRenderPassFinalLayout(const RenderHandle handle, const ImageLayout imageLayout,
    RenderCommandBeginRenderPass& beginRenderPass, RenderGraph::RenderGraphImageState& storeState)
{
    const uint32_t attachmentCount = beginRenderPass.renderPassDesc.attachmentCount;
    for (uint32_t attachmentIdx = 0; attachmentIdx < attachmentCount; ++attachmentIdx) {
        if (beginRenderPass.renderPassDesc.attachmentHandles[attachmentIdx].id == handle.id) {
            beginRenderPass.imageLayouts.attachmentFinalLayouts[attachmentIdx] = imageLayout;
            storeState.resource.imageLayout = imageLayout;
        }
    }
};

void UpdateMultiRenderCommandListRenderPasses(RenderGraph::MultiRenderPassStore& store)
{
    const uint32_t renderPassCount = (uint32_t)store.renderPasses.size();
    PLUGIN_ASSERT(renderPassCount > 1);

    RenderCommandBeginRenderPass* firstRenderPass = store.renderPasses[0];
    PLUGIN_ASSERT(firstRenderPass);
    PLUGIN_ASSERT(firstRenderPass->subpasses.size() >= renderPassCount);
    const RenderCommandBeginRenderPass* lastRenderPass = store.renderPasses[renderPassCount - 1];
    PLUGIN_ASSERT(lastRenderPass);

    const uint32_t attachmentCount = firstRenderPass->renderPassDesc.attachmentCount;

    // take attachment loads from the first one, and stores from the last one
    // take initial layouts from the first one, and final layouts from the last one (could take the next layout)
    // initial store the correct render pass description to first render pass and then copy to others
    // resource states are copied from valid subpasses to another render command list subpasses
    for (uint32_t fromRpIdx = 0; fromRpIdx < renderPassCount; ++fromRpIdx) {
        const auto& fromRenderPass = *(store.renderPasses[fromRpIdx]);
        const uint32_t fromRpSubpassStartIndex = fromRenderPass.subpassStartIndex;
        const auto& fromRpSubpassResourceStates = fromRenderPass.subpassResourceStates[fromRpSubpassStartIndex];
        for (uint32_t toRpIdx = 0; toRpIdx < renderPassCount; ++toRpIdx) {
            if (fromRpIdx != toRpIdx) {
                auto& toRenderPass = *(store.renderPasses[toRpIdx]);
                auto& toRpSubpassResourceStates = toRenderPass.subpassResourceStates[fromRpSubpassStartIndex];
                for (uint32_t idx = 0; idx < attachmentCount; ++idx) {
                    toRpSubpassResourceStates.states[idx] = fromRpSubpassResourceStates.states[idx];
                    toRpSubpassResourceStates.layouts[idx] = fromRpSubpassResourceStates.layouts[idx];
                }
            }
        }
    }

    for (uint32_t idx = 0; idx < firstRenderPass->renderPassDesc.attachmentCount; ++idx) {
        firstRenderPass->renderPassDesc.attachments[idx].storeOp =
            lastRenderPass->renderPassDesc.attachments[idx].storeOp;
        firstRenderPass->renderPassDesc.attachments[idx].stencilStoreOp =
            lastRenderPass->renderPassDesc.attachments[idx].stencilStoreOp;

        firstRenderPass->imageLayouts.attachmentFinalLayouts[idx] =
            lastRenderPass->imageLayouts.attachmentFinalLayouts[idx];
    }

    // copy subpasses to first
    for (uint32_t idx = 1; idx < renderPassCount; ++idx) {
        firstRenderPass->subpasses[idx] = store.renderPasses[idx]->subpasses[idx];
    }

    // copy from first to following render passes
    for (uint32_t idx = 1; idx < renderPassCount; ++idx) {
        // subpass start index is the only changing variables
        const uint32_t subpassStartIndex = store.renderPasses[idx]->subpassStartIndex;
        store.renderPasses[idx]->renderPassDesc = firstRenderPass->renderPassDesc;
        store.renderPasses[idx]->subpassStartIndex = subpassStartIndex;

        // image layouts needs to match
        store.renderPasses[idx]->imageLayouts = firstRenderPass->imageLayouts;
        PLUGIN_ASSERT(store.renderPasses[idx]->subpasses.size() >= renderPassCount);
        // copy all subpasses
        if (!CloneData(store.renderPasses[idx]->subpasses.data(), sizeof(RenderPassSubpassDesc) * renderPassCount,
                firstRenderPass->subpasses.data(), sizeof(RenderPassSubpassDesc) * renderPassCount)) {
            PLUGIN_LOG_E("Copying of renderPasses failed.");
        }
    }
}

ResourceBarrier GetSrcBufferBarrier(const GpuResourceState& state, const BindableBuffer& res)
{
    return {
        state.accessFlags,
        state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED,
        res.byteOffset,
        res.byteSize,
    };
}

ResourceBarrier GetSrcImageBarrier(const GpuResourceState& state, const BindableImage& res)
{
    return {
        state.accessFlags,
        state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        res.imageLayout,
        0,
        PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
    };
}

ResourceBarrier GetDstBufferBarrier(const GpuResourceState& state, const BindableBuffer& res)
{
    return {
        state.accessFlags,
        state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED,
        res.byteOffset,
        res.byteSize,
    };
}

ResourceBarrier GetDstImageBarrier(const GpuResourceState& state, const BindableImage& res)
{
    return {
        state.accessFlags,
        state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        res.imageLayout,
        0,
        PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
    };
}

CommandBarrier GetQueueOwnershipTransferBarrier(const RenderHandle handle, const GpuQueue& srcGpuQueue,
    const GpuQueue& dstGpuQueue, const ImageLayout srcImageLayout, const ImageLayout dstImageLayout)
{
    return {
        handle,

        ResourceBarrier {
            0,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
            srcImageLayout,
            0,
            PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
            ImageSubresourceRange {},
        },
        srcGpuQueue,

        ResourceBarrier {
            0,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            dstImageLayout,
            0,
            PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
            ImageSubresourceRange {},
        },
        dstGpuQueue,
    };
}

void PatchGpuResourceQueueTransfers(array_view<const RenderNodeContextData> frameRenderNodeContextData,
    array_view<const RenderGraph::GpuQueueTransferState> currNodeGpuResourceTransfers)
{
    for (const auto& transferRef : currNodeGpuResourceTransfers) {
        PLUGIN_ASSERT(transferRef.acquireNodeIdx < (uint32_t)frameRenderNodeContextData.size());

        auto& acquireNodeRef = frameRenderNodeContextData[transferRef.acquireNodeIdx];
        const GpuQueue acquireGpuQueue = acquireNodeRef.renderCommandList->GetGpuQueue();
        GpuQueue releaseGpuQueue = acquireGpuQueue;

        if (transferRef.releaseNodeIdx < (uint32_t)frameRenderNodeContextData.size()) {
            auto& releaseNodeRef = frameRenderNodeContextData[transferRef.releaseNodeIdx];
            releaseGpuQueue = releaseNodeRef.renderCommandList->GetGpuQueue();
        }

        const CommandBarrier transferBarrier = GetQueueOwnershipTransferBarrier(transferRef.handle, releaseGpuQueue,
            acquireGpuQueue, transferRef.optionalReleaseImageLayout, transferRef.optionalAcquireImageLayout);

        // release ownership (NOTE: not done for previous frame)
        if (transferRef.releaseNodeIdx < (uint32_t)frameRenderNodeContextData.size()) {
            auto& releaseNodeRef = frameRenderNodeContextData[transferRef.releaseNodeIdx];
            const uint32_t rcIndex = releaseNodeRef.renderCommandList->GetRenderCommandCount() - 1;
            const RenderCommandWithType& cmdRef = releaseNodeRef.renderCommandList->GetRenderCommands()[rcIndex];
            PLUGIN_ASSERT(cmdRef.type == RenderCommandType::BARRIER_POINT);
            const auto& rcbp = *static_cast<RenderCommandBarrierPoint*>(cmdRef.rc);
            PLUGIN_ASSERT(rcbp.renderCommandType == RenderCommandType::GPU_QUEUE_TRANSFER_RELEASE);

            const uint32_t barrierPointIndex = rcbp.barrierPointIndex;
            releaseNodeRef.renderBarrierList->AddBarriersToBarrierPoint(barrierPointIndex, { transferBarrier });

            // inform that we are patching valid barriers
            releaseNodeRef.renderCommandList->SetValidGpuQueueReleaseAcquireBarriers();
        }
        // acquire ownership
        {
            const uint32_t rcIndex = 0;
            const RenderCommandWithType& cmdRef = acquireNodeRef.renderCommandList->GetRenderCommands()[rcIndex];
            PLUGIN_ASSERT(cmdRef.type == RenderCommandType::BARRIER_POINT);
            const auto& rcbp = *static_cast<RenderCommandBarrierPoint*>(cmdRef.rc);
            PLUGIN_ASSERT(rcbp.renderCommandType == RenderCommandType::GPU_QUEUE_TRANSFER_ACQUIRE);

            const uint32_t barrierPointIndex = rcbp.barrierPointIndex;
            acquireNodeRef.renderBarrierList->AddBarriersToBarrierPoint(barrierPointIndex, { transferBarrier });

            // inform that we are patching valid barriers
            acquireNodeRef.renderCommandList->SetValidGpuQueueReleaseAcquireBarriers();
        }
    }
}

bool CheckForBarrierNeed(const unordered_map<RenderHandle, uint32_t>& handledCustomBarriers,
    const uint32_t customBarrierCount, const RenderHandle handle)
{
    bool needsBarrier = RenderHandleUtil::IsDynamicResource(handle);
    if ((customBarrierCount > 0) && needsBarrier) {
        needsBarrier = (handledCustomBarriers.count(handle) > 0) ? false : true;
    }
    return needsBarrier;
}
} // namespace

RenderGraph::RenderGraph(GpuResourceManager& gpuResourceMgr) : gpuResourceMgr_(gpuResourceMgr) {}

void RenderGraph::BeginFrame()
{
    stateCache_.multiRenderPassStore.renderPasses.clear();
    stateCache_.multiRenderPassStore.firstRenderPassBarrierList = nullptr;
    stateCache_.multiRenderPassStore.firstBarrierPointIndex = ~0u;
    stateCache_.multiRenderPassStore.supportOpen = false;
    stateCache_.backbufferHandle = {};
    stateCache_.checkForBackbufferDependency = false;
    stateCache_.firstBackBufferUseNodeIdx = ~0u;
}

void RenderGraph::ProcessRenderNodeGraph(
    const RenderHandle backBufferHandle, const array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores)
{
    // NOTE: separate gpu buffers and gpu images due to larger structs, layers, mips in images
    // all levels of mips and layers are not currently tracked -> needs more fine grained modifications
    // handles:
    // gpu images in descriptor sets, render passes, blits, and custom barriers
    // gpu buffers in descriptor sets, and custom barriers

    {
        // remove resources that will not be tracked anymore and release available slots
        const GpuResourceManager::StateDestroyConsumeStruct stateResetData = gpuResourceMgr_.ConsumeStateDestroyData();
        for (const auto& handle : stateResetData.resources) {
            const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
            const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
            if ((handleType == RenderHandleType::GPU_IMAGE) &&
                (arrayIndex < static_cast<uint32_t>(gpuImageDataIndices_.size()))) {
                if (const uint32_t dataIdx = gpuImageDataIndices_[arrayIndex]; dataIdx != INVALID_TRACK_IDX) {
                    PLUGIN_ASSERT(dataIdx < static_cast<uint32_t>(gpuImageTracking_.size()));
                    gpuImageTracking_[dataIdx] = {}; // reset
                    gpuImageAvailableIndices_.emplace_back(dataIdx);
                }
                gpuImageDataIndices_[arrayIndex] = INVALID_TRACK_IDX;
            } else if (arrayIndex < static_cast<uint32_t>(gpuBufferDataIndices_.size())) {
                if (const uint32_t dataIdx = gpuBufferDataIndices_[arrayIndex]; dataIdx != INVALID_TRACK_IDX) {
                    PLUGIN_ASSERT(dataIdx < static_cast<uint32_t>(gpuBufferTracking_.size()));
                    gpuBufferTracking_[dataIdx] = {}; // reset
                    gpuBufferAvailableIndices_.emplace_back(dataIdx);
                }
                gpuBufferDataIndices_[arrayIndex] = INVALID_TRACK_IDX;
            }
        }
    }

    auto ResizeAndInvalidate = [](auto& dataIndices, const size_t resizeSize) {
        const size_t prevSize = dataIndices.size();
        dataIndices.resize(resizeSize);
        const size_t newSize = dataIndices.size();
        if (prevSize < newSize) {
            std::fill(dataIndices.begin() + (uint32_t)prevSize, dataIndices.end(), INVALID_TRACK_IDX);
        }
    };
    ResizeAndInvalidate(gpuBufferDataIndices_, gpuResourceMgr_.GetBufferHandleCount());
    ResizeAndInvalidate(gpuImageDataIndices_, gpuResourceMgr_.GetImageHandleCount());

#if (RENDER_DEV_ENABLED == 1)
    if constexpr (CORE_RENDER_GRAPH_FULL_DEBUG_PRINT || CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES ||
                  CORE_RENDER_GRAPH_FULL_DEBUG_ATTACHMENTS) {
        static uint64_t debugFrame = 0;
        debugFrame++;
        PLUGIN_LOG_I("START RENDER GRAPH, FRAME %" PRIu64, debugFrame);
    }
#endif

    // need to store some of the resource for frame state in undefined state (i.e. reset on frame boundaries)

    stateCache_.backbufferHandle = backBufferHandle;
    if (RenderHandleUtil::IsValid(stateCache_.backbufferHandle)) {
        stateCache_.checkForBackbufferDependency = true;
    }
    ProcessRenderNodeGraphNodeStores(renderNodeGraphNodeStores, stateCache_);

    // store final state for next frame
    StoreFinalBufferState(stateCache_);
    StoreFinalImageState(stateCache_); // processes gpuImageBackbufferState_ as well
}

RenderGraph::BackbufferState RenderGraph::GetBackbufferResourceState() const
{
    return gpuImageBackbufferState_;
}

void RenderGraph::ProcessRenderNodeGraphNodeStores(
    const array_view<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores, StateCache& stateCache)
{
    for (RenderNodeGraphNodeStore* graphStore : renderNodeGraphNodeStores) {
        PLUGIN_ASSERT(graphStore);
        if (!graphStore) {
            continue;
        }

        for (uint32_t nodeIdx = 0; nodeIdx < (uint32_t)graphStore->renderNodeContextData.size(); ++nodeIdx) {
            auto& ref = graphStore->renderNodeContextData[nodeIdx];
            ref.submitInfo.waitForSwapchainAcquireSignal = false; // reset

#if (RENDER_DEV_ENABLED == 1)
            if constexpr (CORE_RENDER_GRAPH_FULL_DEBUG_PRINT) {
                PLUGIN_LOG_I("FULL NODENAME %s", graphStore->renderNodeData[nodeIdx].fullName.data());
            }
#endif

            if (stateCache.multiRenderPassStore.supportOpen &&
                (stateCache.multiRenderPassStore.renderPasses.size() == 0)) {
                PLUGIN_LOG_E("invalid multi render node render pass subpass stitching");
                // NOTE: add more error handling and invalidate render command lists
            }
            stateCache.multiRenderPassStore.supportOpen = ref.renderCommandList->HasMultiRenderCommandListSubpasses();
            array_view<const RenderCommandWithType> cmdListRef = ref.renderCommandList->GetRenderCommands();
            // go through commands that affect or need transitions and barriers
            ProcessRenderNodeCommands(cmdListRef, nodeIdx, ref, stateCache);

            // needs backbuffer/swapchain wait
            if (stateCache.firstBackBufferUseNodeIdx == nodeIdx) {
                ref.submitInfo.waitForSwapchainAcquireSignal = true;
                stateCache.checkForBackbufferDependency = false;
                stateCache.firstBackBufferUseNodeIdx = ~0u;
            }

            // patch gpu resource queue transfers
            if (!currNodeGpuResourceTransfers_.empty()) {
                PatchGpuResourceQueueTransfers(graphStore->renderNodeContextData, currNodeGpuResourceTransfers_);
                // clear for next use
                currNodeGpuResourceTransfers_.clear();
            }
        }
    }
}

void RenderGraph::ProcessRenderNodeCommands(array_view<const RenderCommandWithType>& cmdListRef,
    const uint32_t& nodeIdx, RenderNodeContextData& ref, StateCache& stateCache)
{
    for (uint32_t listIdx = 0; listIdx < (uint32_t)cmdListRef.size(); ++listIdx) {
        auto& cmdRef = cmdListRef[listIdx];

#if (RENDER_DEV_ENABLED == 1)
        if constexpr (CORE_RENDER_GRAPH_FULL_DEBUG_PRINT) {
            DebugPrintCommandListCommand(cmdRef, gpuResourceMgr_);
        }
#endif

        // most of the commands are handled within BarrierPoint
        switch (cmdRef.type) {
            case RenderCommandType::BARRIER_POINT:
                RenderCommand(nodeIdx, listIdx, ref, *static_cast<RenderCommandBarrierPoint*>(cmdRef.rc), stateCache);
                break;

            case RenderCommandType::BEGIN_RENDER_PASS:
                RenderCommand(
                    nodeIdx, listIdx, ref, *static_cast<RenderCommandBeginRenderPass*>(cmdRef.rc), stateCache);
                break;

            case RenderCommandType::END_RENDER_PASS:
                RenderCommand(nodeIdx, listIdx, ref, *static_cast<RenderCommandEndRenderPass*>(cmdRef.rc), stateCache);
                break;

            case RenderCommandType::NEXT_SUBPASS:
            case RenderCommandType::DRAW:
            case RenderCommandType::DRAW_INDIRECT:
            case RenderCommandType::DISPATCH:
            case RenderCommandType::DISPATCH_INDIRECT:
            case RenderCommandType::BIND_PIPELINE:
            case RenderCommandType::BIND_VERTEX_BUFFERS:
            case RenderCommandType::BIND_INDEX_BUFFER:
            case RenderCommandType::COPY_BUFFER:
            case RenderCommandType::COPY_BUFFER_IMAGE:
            case RenderCommandType::COPY_IMAGE:
            case RenderCommandType::UPDATE_DESCRIPTOR_SETS:
            case RenderCommandType::BIND_DESCRIPTOR_SETS:
            case RenderCommandType::PUSH_CONSTANT:
            case RenderCommandType::BLIT_IMAGE:
                // dynamic states
            case RenderCommandType::DYNAMIC_STATE_VIEWPORT:
            case RenderCommandType::DYNAMIC_STATE_SCISSOR:
            case RenderCommandType::DYNAMIC_STATE_LINE_WIDTH:
            case RenderCommandType::DYNAMIC_STATE_DEPTH_BIAS:
            case RenderCommandType::DYNAMIC_STATE_BLEND_CONSTANTS:
            case RenderCommandType::DYNAMIC_STATE_DEPTH_BOUNDS:
            case RenderCommandType::DYNAMIC_STATE_STENCIL:
            case RenderCommandType::WRITE_TIMESTAMP:
            case RenderCommandType::GPU_QUEUE_TRANSFER_RELEASE:
            case RenderCommandType::GPU_QUEUE_TRANSFER_ACQUIRE:
            case RenderCommandType::UNDEFINED:
            default: {
                // nop
                break;
            }
        }
    } // end command for
}

void RenderGraph::StoreFinalBufferState(const StateCache& stateCache)
{
    for (auto& ref : gpuBufferTracking_) {
        if (!RenderHandleUtil::IsDynamicResource(ref.resource.handle)) {
            ref = {};
            continue;
        }
        if (RenderHandleUtil::IsResetOnFrameBorders(ref.resource.handle)) {
            // reset, but we do not reset the handle, because the gpuImageTracking_ element is not removed
            const RenderHandle handle = ref.resource.handle;
            ref = {};
            ref.resource.handle = handle;
        }
        // need to reset per frame variables for all buffers (so we do not try to patch or debug from previous frames)
        ref.prevRc = {};
        ref.prevRenderNodeIndex = { ~0u };
    }
}

void RenderGraph::StoreFinalImageState(StateCache& stateCache)
{
    gpuImageBackbufferState_ = {}; // reset

#if (RENDER_DEV_ENABLED == 1)
    if constexpr (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
        PLUGIN_LOG_I("end_frame image_state:");
    }
#endif
    for (auto& ref : gpuImageTracking_) {
        // if resource is not dynamic, we do not track and care
        if (!RenderHandleUtil::IsDynamicResource(ref.resource.handle)) {
            ref = {};
            continue;
        }
        // handle automatic presentation layout
        if ((ref.resource.handle.id == stateCache.backbufferHandle.id) &&
            RenderHandleUtil::IsGpuImage(stateCache.backbufferHandle)) {
            if (ref.prevRc.type == RenderCommandType::BEGIN_RENDER_PASS) {
                RenderCommandBeginRenderPass& beginRenderPass =
                    *static_cast<RenderCommandBeginRenderPass*>(ref.prevRc.rc);
                PatchRenderPassFinalLayout(
                    ref.resource.handle, ImageLayout::CORE_IMAGE_LAYOUT_PRESENT_SRC_KHR, beginRenderPass, ref);
            }
            // NOTE: currently we handle automatic presentation layout in vulkan backend if not in render pass
            // store final state for backbuffer
            gpuImageBackbufferState_.state = ref.state;
            gpuImageBackbufferState_.layout = ref.resource.imageLayout;
        }
#if (RENDER_DEV_ENABLED == 1)
        // print before reset for next frame
        if constexpr (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
            DebugPrintImageState(gpuResourceMgr_, ref);
        }
#endif
        // shallow resources are not tracked
        // they are always in undefined state in the beging of the frame
        if (RenderHandleUtil::IsResetOnFrameBorders(ref.resource.handle)) {
            // reset, but we do not reset the handle, because the gpuImageTracking_ element is not removed
            const RenderHandle handle = ref.resource.handle;
            ref = {};
            ref.resource.handle = handle;
        }

        // need to reset per frame variables for all images (so we do not try to patch from previous frames)
        ref.prevRc = {};
        ref.prevRenderNodeIndex = { ~0u };
    }
}

void RenderGraph::RenderCommand(const uint32_t renderNodeIndex, const uint32_t commandListCommandIndex,
    RenderNodeContextData& nodeData, RenderCommandBeginRenderPass& rc, StateCache& stateCache)
{
    // update layouts for attachments to gpu image state
    BeginRenderPassParameters params { rc, stateCache, { RenderCommandType::BEGIN_RENDER_PASS, &rc } };

    PLUGIN_ASSERT(rc.renderPassDesc.subpassCount > 0);

    const bool hasRenderPassDependency = stateCache.multiRenderPassStore.supportOpen;
    if (hasRenderPassDependency) { // stitch render pass subpasses
        BeginRenderPassHandleDependency(params, commandListCommandIndex, nodeData);
    }

    const GpuQueue gpuQueue = nodeData.renderCommandList->GetGpuQueue();

    auto finalImageLayouts =
        array_view(rc.imageLayouts.attachmentFinalLayouts, countof(rc.imageLayouts.attachmentFinalLayouts));

    BeginRenderPassUpdateImageStates(params, gpuQueue, finalImageLayouts, renderNodeIndex);

    for (uint32_t subpassIdx = 0; subpassIdx < rc.renderPassDesc.subpassCount; ++subpassIdx) {
        const auto& subpassRef = rc.subpasses[subpassIdx];
        const auto& subpassResourceStatesRef = rc.subpassResourceStates[subpassIdx];

        BeginRenderPassUpdateSubpassImageStates(
            array_view(subpassRef.inputAttachmentIndices, subpassRef.inputAttachmentCount), rc.renderPassDesc,
            subpassResourceStatesRef, finalImageLayouts, stateCache);

        BeginRenderPassUpdateSubpassImageStates(
            array_view(subpassRef.colorAttachmentIndices, subpassRef.colorAttachmentCount), rc.renderPassDesc,
            subpassResourceStatesRef, finalImageLayouts, stateCache);

        BeginRenderPassUpdateSubpassImageStates(
            array_view(subpassRef.resolveAttachmentIndices, subpassRef.resolveAttachmentCount), rc.renderPassDesc,
            subpassResourceStatesRef, finalImageLayouts, stateCache);

        if (subpassRef.depthAttachmentCount == 1) {
            BeginRenderPassUpdateSubpassImageStates(
                array_view(&subpassRef.depthAttachmentIndex, subpassRef.depthAttachmentCount), rc.renderPassDesc,
                subpassResourceStatesRef, finalImageLayouts, stateCache);
            if (subpassRef.depthResolveAttachmentCount == 1) {
                BeginRenderPassUpdateSubpassImageStates(
                    array_view(&subpassRef.depthResolveAttachmentIndex, subpassRef.depthResolveAttachmentCount),
                    rc.renderPassDesc, subpassResourceStatesRef, finalImageLayouts, stateCache);
            }
        }
    }

    if (hasRenderPassDependency) { // stitch render pass subpasses
        if (rc.subpassStartIndex > 0) {
            // stitched to behave as a nextSubpass() and not beginRenderPass()
            rc.beginType = RenderPassBeginType::RENDER_PASS_SUBPASS_BEGIN;
        }
        const bool finalSubpass = (rc.subpassStartIndex == rc.renderPassDesc.subpassCount - 1);
        if (finalSubpass) {
            UpdateMultiRenderCommandListRenderPasses(stateCache.multiRenderPassStore);
            // multiRenderPassStore cleared in EndRenderPass
        }
    }
#if (RENDER_DEV_ENABLED == 1)
    if (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
        DebugRenderPassLayoutPrint(gpuResourceMgr_, rc);
    }
#endif
}

void RenderGraph::BeginRenderPassHandleDependency(
    BeginRenderPassParameters& params, const uint32_t commandListCommandIndex, RenderNodeContextData& nodeData)
{
    params.stateCache.multiRenderPassStore.renderPasses.emplace_back(&params.rc);
    // store the first begin render pass
    params.rpForCmdRef = { RenderCommandType::BEGIN_RENDER_PASS,
        params.stateCache.multiRenderPassStore.renderPasses[0] };

    if (params.rc.subpassStartIndex == 0) { // store the first render pass barrier point
        // barrier point must be previous command
        PLUGIN_ASSERT(commandListCommandIndex >= 1);
        const uint32_t prevCommandIndex = commandListCommandIndex - 1;

        const RenderCommandWithType& barrierPointCmdRef =
            nodeData.renderCommandList->GetRenderCommands()[prevCommandIndex];
        PLUGIN_ASSERT(barrierPointCmdRef.type == RenderCommandType::BARRIER_POINT);
        PLUGIN_ASSERT(static_cast<RenderCommandBarrierPoint*>(barrierPointCmdRef.rc));

        params.stateCache.multiRenderPassStore.firstRenderPassBarrierList = nodeData.renderBarrierList.get();
        params.stateCache.multiRenderPassStore.firstBarrierPointIndex =
            static_cast<RenderCommandBarrierPoint*>(barrierPointCmdRef.rc)->barrierPointIndex;
    }
}

void RenderGraph::BeginRenderPassUpdateImageStates(BeginRenderPassParameters& params, const GpuQueue& gpuQueue,
    array_view<ImageLayout>& finalImageLayouts, const uint32_t renderNodeIndex)
{
    auto& initialImageLayouts = params.rc.imageLayouts.attachmentInitialLayouts;
    const auto& attachmentHandles = params.rc.renderPassDesc.attachmentHandles;
    auto& attachments = params.rc.renderPassDesc.attachments;
    auto& attachmentInputResourceStates = params.rc.inputResourceStates;

    for (uint32_t attachmentIdx = 0; attachmentIdx < params.rc.renderPassDesc.attachmentCount; ++attachmentIdx) {
        const RenderHandle handle = attachmentHandles[attachmentIdx];
        // NOTE: invalidate invalid handle commands already in render command list
        if (!RenderHandleUtil::IsGpuImage(handle)) {
#ifdef _DEBUG
            PLUGIN_LOG_E("invalid handle in render node graph");
#endif
            continue;
        }
        auto& stateRef = GetImageResourceStateRef(handle, gpuQueue);
        const ImageLayout imgLayout = stateRef.resource.imageLayout;
        // image layout is undefined if automatic barriers have been disabled
        if (params.rc.enableAutomaticLayoutChanges) {
            initialImageLayouts[attachmentIdx] = imgLayout;
        }
        // undefined layout with load_op_load -> we modify to dont_care (and remove validation warning)
        if ((imgLayout == ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED) &&
            (attachments[attachmentIdx].loadOp == AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD)) {
            // dont care (user needs to be sure what is wanted, i.e. in first frame one should clear)
            attachments[attachmentIdx].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
        finalImageLayouts[attachmentIdx] = imgLayout;
        attachmentInputResourceStates.states[attachmentIdx] = stateRef.state;
        attachmentInputResourceStates.layouts[attachmentIdx] = imgLayout;

        // store render pass for final layout patching
        stateRef.prevRc = params.rpForCmdRef;
        stateRef.prevRenderNodeIndex = renderNodeIndex;

        // flag for backbuffer use
        if (params.stateCache.checkForBackbufferDependency && (params.stateCache.backbufferHandle == handle)) {
            params.stateCache.firstBackBufferUseNodeIdx = renderNodeIndex;
            params.stateCache.checkForBackbufferDependency = false; // we do not test anymore
        }
    }
}

void RenderGraph::BeginRenderPassUpdateSubpassImageStates(array_view<const uint32_t> attatchmentIndices,
    const RenderPassDesc& renderPassDesc, const RenderPassAttachmentResourceStates& subpassResourceStatesRef,
    array_view<ImageLayout> finalImageLayouts, StateCache& stateCache)
{
    for (const uint32_t attachmentIndex : attatchmentIndices) {
        // NOTE: handle invalid commands already in render command list and invalidate draws etc.
        PLUGIN_ASSERT(attachmentIndex < renderPassDesc.attachmentCount);
        const RenderHandle handle = renderPassDesc.attachmentHandles[attachmentIndex];
        PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GPU_IMAGE);
        const GpuResourceState& refState = subpassResourceStatesRef.states[attachmentIndex];
        const ImageLayout& refImgLayout = subpassResourceStatesRef.layouts[attachmentIndex];
        // NOTE: we should support non dynamicity and GENERAL

        finalImageLayouts[attachmentIndex] = refImgLayout;
        auto& ref = GetImageResourceStateRef(handle, refState.gpuQueue);
        ref.state = refState;
        ref.resource.handle = handle;
        ref.resource.imageLayout = refImgLayout;
    }
}

void RenderGraph::RenderCommand(const uint32_t renderNodeIndex, const uint32_t commandListCommandIndex,
    const RenderNodeContextData& nodeData, RenderCommandEndRenderPass& rc, StateCache& stateCache)
{
    const bool hasRenderPassDependency = stateCache.multiRenderPassStore.supportOpen;
    if (hasRenderPassDependency) {
        const bool finalSubpass = (rc.subpassCount == (uint32_t)stateCache.multiRenderPassStore.renderPasses.size());
        if (finalSubpass) {
            if (rc.subpassStartIndex != (rc.subpassCount - 1)) {
                PLUGIN_LOG_E("RenderGraph: error in multi render node render pass subpass ending");
                // NOTE: add more error handling and invalidate render command lists
            }
            rc.endType = RenderPassEndType::END_RENDER_PASS;
            stateCache.multiRenderPassStore.renderPasses.clear();
            stateCache.multiRenderPassStore.firstRenderPassBarrierList = nullptr;
            stateCache.multiRenderPassStore.firstBarrierPointIndex = ~0u;
            stateCache.multiRenderPassStore.supportOpen = false;
        } else {
            rc.endType = RenderPassEndType::END_SUBPASS;
        }
    }
}

void RenderGraph::RenderCommand(const uint32_t renderNodeIndex, const uint32_t commandListCommandIndex,
    RenderNodeContextData& nodeData, RenderCommandBarrierPoint& rc, StateCache& stateCache)
{
    // go through required descriptors for current upcoming event
    const auto& customBarrierListRef = nodeData.renderCommandList->GetCustomBarriers();
    const auto& vertexInputBufferBarrierListRef = nodeData.renderCommandList->GetVertexInputBufferBarriers();
    const auto& cmdListRef = nodeData.renderCommandList->GetRenderCommands();
    const auto& allDescriptorSetHandlesForBarriers = nodeData.renderCommandList->GetDescriptorSetHandles();
    const auto& nodeDescriptorSetMgrRef = *nodeData.nodeContextDescriptorSetMgr;

    parameterCachePools_.combinedBarriers.clear();
    parameterCachePools_.handledCustomBarriers.clear();
    ParameterCache parameters { parameterCachePools_.combinedBarriers, parameterCachePools_.handledCustomBarriers,
        rc.customBarrierCount, rc.vertexIndexBarrierCount, renderNodeIndex, nodeData.renderCommandList->GetGpuQueue(),
        { RenderCommandType::BARRIER_POINT, &rc }, stateCache };
    // first check custom barriers
    if (parameters.customBarrierCount > 0) {
        HandleCustomBarriers(parameters, rc.customBarrierIndexBegin, customBarrierListRef);
    }
    // then vertex / index buffer barriers in the barrier point before render pass
    if (parameters.vertexInputBarrierCount > 0) {
        PLUGIN_ASSERT(rc.renderCommandType == RenderCommandType::BEGIN_RENDER_PASS);
        HandleVertexInputBufferBarriers(parameters, rc.vertexIndexBarrierIndexBegin, vertexInputBufferBarrierListRef);
    }

    // in barrier point the next render command is known for which the barrier is needed
    if (rc.renderCommandType == RenderCommandType::BLIT_IMAGE) {
        HandleBlitImage(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::COPY_BUFFER) {
        HandleCopyBuffer(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::COPY_BUFFER_IMAGE) {
        HandleCopyBufferImage(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::COPY_IMAGE) {
        HandleCopyBufferImage(parameters, commandListCommandIndex, cmdListRef); // NOTE: handles image to image
    } else {                                                                    // descriptor sets
        const uint32_t descriptorSetHandleBeginIndex = rc.descriptorSetHandleIndexBegin;
        const uint32_t descriptorSetHandleEndIndex = descriptorSetHandleBeginIndex + rc.descriptorSetHandleCount;
        const uint32_t descriptorSetHandleMaxIndex =
            Math::min(descriptorSetHandleEndIndex, static_cast<uint32_t>(allDescriptorSetHandlesForBarriers.size()));
        const auto descriptorSetHandlesForBarriers =
            array_view(allDescriptorSetHandlesForBarriers.data() + descriptorSetHandleBeginIndex,
                allDescriptorSetHandlesForBarriers.data() + descriptorSetHandleMaxIndex);
        HandleDescriptorSets(parameters, descriptorSetHandlesForBarriers, nodeDescriptorSetMgrRef);
    }

    if (!parameters.combinedBarriers.empty()) {
        // use first render pass barrier point with following subpasses
        // firstRenderPassBarrierPoint is null for the first subpass
        const bool renderPassHasDependancy = stateCache.multiRenderPassStore.supportOpen;
        if (renderPassHasDependancy && stateCache.multiRenderPassStore.firstRenderPassBarrierList) {
            PLUGIN_ASSERT(!stateCache.multiRenderPassStore.renderPasses.empty());
            stateCache.multiRenderPassStore.firstRenderPassBarrierList->AddBarriersToBarrierPoint(
                rc.barrierPointIndex, parameters.combinedBarriers);
        } else {
            nodeData.renderBarrierList->AddBarriersToBarrierPoint(rc.barrierPointIndex, parameters.combinedBarriers);
        }
    }
#if (RENDER_DEV_ENABLED == 1)
    if (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
        DebugBarrierPrint(gpuResourceMgr_, parameters.combinedBarriers);
    }
#endif
}

inline void RenderGraph::UpdateBufferResourceState(
    RenderGraphBufferState& stateRef, const ParameterCache& params, const CommandBarrier& cb)
{
    stateRef.resource.handle = cb.resourceHandle;
    stateRef.state.shaderStageFlags = 0;
    stateRef.state.accessFlags = cb.dst.accessFlags;
    stateRef.state.pipelineStageFlags = cb.dst.pipelineStageFlags;
    stateRef.state.gpuQueue = params.gpuQueue;
    stateRef.prevRc = params.rcWithType;
    stateRef.prevRenderNodeIndex = params.renderNodeIndex;
}

inline void RenderGraph::UpdateImageResourceState(
    RenderGraphImageState& stateRef, const ParameterCache& params, const CommandBarrier& cb)
{
    stateRef.resource.handle = cb.resourceHandle;
    stateRef.state.shaderStageFlags = 0;
    stateRef.state.accessFlags = cb.dst.accessFlags;
    stateRef.state.pipelineStageFlags = cb.dst.pipelineStageFlags;
    stateRef.state.gpuQueue = params.gpuQueue;
    stateRef.prevRc = params.rcWithType;
    stateRef.prevRenderNodeIndex = params.renderNodeIndex;
}

void RenderGraph::HandleCustomBarriers(ParameterCache& params, const uint32_t barrierIndexBegin,
    const array_view<const CommandBarrier>& customBarrierListRef)
{
    params.handledCustomBarriers.reserve(params.customBarrierCount);
    PLUGIN_ASSERT(barrierIndexBegin + params.customBarrierCount <= customBarrierListRef.size());
    for (auto begin = (customBarrierListRef.begin() + barrierIndexBegin),
              end = Math::min(customBarrierListRef.end(), begin + params.customBarrierCount);
         begin != end; ++begin) {
        // add a copy and modify if needed
        auto& cb = params.combinedBarriers.emplace_back(*begin);

        // NOTE: undefined type is for non-resource memory/pipeline barriers
        const RenderHandleType type = RenderHandleUtil::GetHandleType(cb.resourceHandle);
        const bool isDynamicTrack = RenderHandleUtil::IsDynamicResource(cb.resourceHandle);
        PLUGIN_ASSERT((type == RenderHandleType::UNDEFINED) || (type == RenderHandleType::GPU_BUFFER) ||
                      (type == RenderHandleType::GPU_IMAGE));
        if (type == RenderHandleType::GPU_BUFFER) {
            if (isDynamicTrack) {
                auto& stateRef = GetBufferResourceStateRef(cb.resourceHandle, params.gpuQueue);
                UpdateBufferResourceState(stateRef, params, cb);
            }
            params.handledCustomBarriers[cb.resourceHandle] = 0;
        } else if (type == RenderHandleType::GPU_IMAGE) {
            if (isDynamicTrack) {
                auto& stateRef = GetImageResourceStateRef(cb.resourceHandle, params.gpuQueue);
                if (cb.src.optionalImageLayout == CORE_IMAGE_LAYOUT_MAX_ENUM) {
                    cb.src.accessFlags = stateRef.state.accessFlags;
                    cb.src.pipelineStageFlags =
                        stateRef.state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    cb.src.optionalImageLayout = stateRef.resource.imageLayout;
                }
                UpdateImageResourceState(stateRef, params, cb);
                stateRef.resource.imageLayout = cb.dst.optionalImageLayout;
            }
            params.handledCustomBarriers[cb.resourceHandle] = 0;
        }
    }
}

void RenderGraph::HandleVertexInputBufferBarriers(ParameterCache& params, const uint32_t barrierIndexBegin,
    const array_view<const VertexBuffer>& vertexInputBufferBarrierListRef)
{
    for (uint32_t idx = 0; idx < params.vertexInputBarrierCount; ++idx) {
        const uint32_t barrierIndex = barrierIndexBegin + idx;
        PLUGIN_ASSERT(barrierIndex < (uint32_t)vertexInputBufferBarrierListRef.size());
        if (barrierIndex < (uint32_t)vertexInputBufferBarrierListRef.size()) {
            const VertexBuffer& vbInput = vertexInputBufferBarrierListRef[barrierIndex];
            GpuResourceState resourceState { CORE_SHADER_STAGE_VERTEX_BIT, 0, CORE_PIPELINE_STAGE_VERTEX_INPUT_BIT,
                params.gpuQueue };
            UpdateStateAndCreateBarriersGpuBuffer(
                resourceState, { vbInput.bufferHandle, vbInput.bufferOffset, vbInput.byteSize }, params);
        }
    }
}

void RenderGraph::HandleBlitImage(ParameterCache& params, const uint32_t& commandListCommandIndex,
    const array_view<const RenderCommandWithType>& cmdListRef)
{
    const uint32_t nextListIdx = commandListCommandIndex + 1;
    PLUGIN_ASSERT(nextListIdx < cmdListRef.size());
    const auto& nextCmdRef = cmdListRef[nextListIdx];
    PLUGIN_ASSERT(nextCmdRef.type == RenderCommandType::BLIT_IMAGE);

    const RenderCommandBlitImage& nextRc = *static_cast<RenderCommandBlitImage*>(nextCmdRef.rc);

    const bool needsSrcBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, nextRc.srcHandle);
    if (needsSrcBarrier) {
        BindableImage bRes = {};
        bRes.handle = nextRc.srcHandle;
        bRes.imageLayout = nextRc.srcImageLayout;
        AddCommandBarrierAndUpdateStateCacheImage(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_TRANSFER_READ_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
            bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }

    const bool needsDstBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, nextRc.dstHandle);
    if (needsDstBarrier) {
        BindableImage bRes = {};
        bRes.handle = nextRc.dstHandle;
        bRes.imageLayout = nextRc.dstImageLayout;
        AddCommandBarrierAndUpdateStateCacheImage(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_TRANSFER_WRITE_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
            bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }
}

void RenderGraph::HandleCopyBuffer(ParameterCache& params, const uint32_t& commandListCommandIndex,
    const array_view<const RenderCommandWithType>& cmdListRef)
{
    const uint32_t nextListIdx = commandListCommandIndex + 1;
    PLUGIN_ASSERT(nextListIdx < cmdListRef.size());
    const auto& nextCmdRef = cmdListRef[nextListIdx];
    PLUGIN_ASSERT(nextCmdRef.type == RenderCommandType::COPY_BUFFER);

    const RenderCommandCopyBuffer& nextRc = *static_cast<RenderCommandCopyBuffer*>(nextCmdRef.rc);

    const bool needsSrcBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, nextRc.srcHandle);
    if (needsSrcBarrier) {
        const BindableBuffer bRes = { nextRc.srcHandle, nextRc.bufferCopy.srcOffset, nextRc.bufferCopy.size };
        AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_TRANSFER_READ_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
            bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }

    const bool needsDstBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, nextRc.dstHandle);
    if (needsDstBarrier) {
        const BindableBuffer bRes = { nextRc.dstHandle, nextRc.bufferCopy.dstOffset, nextRc.bufferCopy.size };
        AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_TRANSFER_WRITE_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
            bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }
}

void RenderGraph::HandleCopyBufferImage(ParameterCache& params, const uint32_t& commandListCommandIndex,
    const array_view<const RenderCommandWithType>& cmdListRef)
{
    const uint32_t nextListIdx = commandListCommandIndex + 1;
    PLUGIN_ASSERT(nextListIdx < cmdListRef.size());
    const auto& nextCmdRef = cmdListRef[nextListIdx];
    PLUGIN_ASSERT((nextCmdRef.type == RenderCommandType::COPY_BUFFER_IMAGE) ||
                  (nextCmdRef.type == RenderCommandType::COPY_IMAGE));

    // NOTE: two different command types supported
    RenderHandle srcHandle;
    RenderHandle dstHandle;
    if (nextCmdRef.type == RenderCommandType::COPY_BUFFER_IMAGE) {
        const RenderCommandCopyBufferImage& nextRc = *static_cast<RenderCommandCopyBufferImage*>(nextCmdRef.rc);
        PLUGIN_ASSERT(nextRc.copyType != RenderCommandCopyBufferImage::CopyType::UNDEFINED);
        srcHandle = nextRc.srcHandle;
        dstHandle = nextRc.dstHandle;
    } else if (nextCmdRef.type == RenderCommandType::COPY_IMAGE) {
        const RenderCommandCopyImage& nextRc = *static_cast<RenderCommandCopyImage*>(nextCmdRef.rc);
        srcHandle = nextRc.srcHandle;
        dstHandle = nextRc.dstHandle;
    }

    const bool needsSrcBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, srcHandle);
    if (needsSrcBarrier) {
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(srcHandle);
        PLUGIN_UNUSED(handleType);
        PLUGIN_ASSERT(handleType == RenderHandleType::GPU_IMAGE || handleType == RenderHandleType::GPU_BUFFER);
        if (handleType == RenderHandleType::GPU_BUFFER) {
            BindableBuffer bRes;
            bRes.handle = srcHandle;
            AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
                GpuResourceState {
                    0, CORE_ACCESS_TRANSFER_READ_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
                bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
        } else {
            BindableImage bRes;
            bRes.handle = srcHandle;
            bRes.imageLayout = CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            AddCommandBarrierAndUpdateStateCacheImage(params.renderNodeIndex,
                GpuResourceState {
                    0, CORE_ACCESS_TRANSFER_READ_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
                bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
        }
    }

    const bool needsDstBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, dstHandle);
    if (needsDstBarrier) {
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(dstHandle);
        PLUGIN_UNUSED(handleType);
        PLUGIN_ASSERT(handleType == RenderHandleType::GPU_IMAGE || handleType == RenderHandleType::GPU_BUFFER);
        if (handleType == RenderHandleType::GPU_BUFFER) {
            BindableBuffer bRes;
            bRes.handle = dstHandle;
            AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
                GpuResourceState {
                    0, CORE_ACCESS_TRANSFER_WRITE_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
                bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
        } else {
            BindableImage bRes;
            bRes.handle = dstHandle;
            bRes.imageLayout = CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            AddCommandBarrierAndUpdateStateCacheImage(params.renderNodeIndex,
                GpuResourceState {
                    0, CORE_ACCESS_TRANSFER_WRITE_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
                bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
        }
    }
}

void RenderGraph::HandleDescriptorSets(ParameterCache& params,
    const array_view<const RenderHandle>& descriptorSetHandlesForBarriers,
    const NodeContextDescriptorSetManager& nodeDescriptorSetMgrRef)
{
    for (const RenderHandle descriptorSetHandle : descriptorSetHandlesForBarriers) {
        PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(descriptorSetHandle) == RenderHandleType::DESCRIPTOR_SET);

        const auto bindingResources = nodeDescriptorSetMgrRef.GetCpuDescriptorSetData(descriptorSetHandle);
        const auto& buffers = bindingResources.buffers;
        const auto& images = bindingResources.images;
        for (const auto& ref : buffers) {
            const uint32_t descriptorCount = ref.binding.descriptorCount;
            // skip, array bindings which are bound from first index, they have also descriptorCount 0
            if (descriptorCount == 0) {
                continue;
            }
            const uint32_t arrayOffset = ref.arrayOffset;
            PLUGIN_ASSERT((arrayOffset + descriptorCount - 1) <= buffers.size());
            for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                // first is the ref, starting from 1 we use array offsets
                const auto& bRes = (idx == 0) ? ref : buffers[arrayOffset + idx - 1];
                if (CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, ref.resource.handle)) {
                    UpdateStateAndCreateBarriersGpuBuffer(bRes.state, bRes.resource, params);
                }
            }
        }
        for (const auto& ref : images) {
            const uint32_t descriptorCount = ref.binding.descriptorCount;
            // skip, array bindings which are bound from first index, they have also descriptorCount 0
            if (descriptorCount == 0) {
                continue;
            }
            const uint32_t arrayOffset = ref.arrayOffset;
            PLUGIN_ASSERT((arrayOffset + descriptorCount - 1) <= images.size());
            for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                // first is the ref, starting from 1 we use array offsets
                const auto& bRes = (idx == 0) ? ref : images[arrayOffset + idx - 1];
                if (CheckForBarrierNeed(
                        params.handledCustomBarriers, params.customBarrierCount, bRes.resource.handle)) {
                    UpdateStateAndCreateBarriersGpuImage(bRes.state, bRes.resource, params);
                }
            }
        }
    } // end for
}

void RenderGraph::UpdateStateAndCreateBarriersGpuImage(
    const GpuResourceState& state, const BindableImage& res, RenderGraph::ParameterCache& params)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(res.handle);
    if (arrayIndex >= static_cast<uint32_t>(gpuImageDataIndices_.size())) {
        return;
    }

    auto& ref = GetImageResourceStateRef(res.handle, state.gpuQueue);
    // check if we can update the image layout with previous render pass
    if (ref.prevRc.type == RenderCommandType::BEGIN_RENDER_PASS) {
        auto& beginRenderPass = *static_cast<RenderCommandBeginRenderPass*>(ref.prevRc.rc);
        PatchRenderPassFinalLayout(res.handle, res.imageLayout, beginRenderPass, ref);
    }

    const GpuResourceState& prevState = ref.state;
    const BindableImage& prevImage = ref.resource;
    const ResourceBarrier prevStateRb = GetSrcImageBarrier(prevState, prevImage);

    const bool layoutChanged = (prevStateRb.optionalImageLayout != res.imageLayout);
    const bool accessFlagsChanged = (prevStateRb.accessFlags != state.accessFlags);
    const bool writeTarget = (prevStateRb.accessFlags & WRITE_ACCESS_FLAGS);
    const bool inputAttachment = (state.accessFlags == CORE_ACCESS_INPUT_ATTACHMENT_READ_BIT);
    // input attachments are handled with render passes and not with barriers
    if ((layoutChanged || accessFlagsChanged || writeTarget) && (!inputAttachment)) {
        if ((prevState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED) &&
            (prevState.gpuQueue.type != state.gpuQueue.type)) {
            PLUGIN_ASSERT(state.gpuQueue.type != GpuQueue::QueueType::UNDEFINED);

            PLUGIN_ASSERT(ref.prevRenderNodeIndex != params.renderNodeIndex);
            currNodeGpuResourceTransfers_.emplace_back(RenderGraph::GpuQueueTransferState {
                res.handle, ref.prevRenderNodeIndex, params.renderNodeIndex, prevImage.imageLayout, res.imageLayout });
        } else {
            params.combinedBarriers.emplace_back(CommandBarrier {
                res.handle, prevStateRb, prevState.gpuQueue, GetDstImageBarrier(state, res), params.gpuQueue });
        }

        ref.state = state;
        ref.resource = res;
        ref.prevRc = params.rcWithType;
        ref.prevRenderNodeIndex = params.renderNodeIndex;
    }
}

void RenderGraph::UpdateStateAndCreateBarriersGpuBuffer(
    const GpuResourceState& state, const BindableBuffer& res, RenderGraph::ParameterCache& params)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(res.handle);
    if (arrayIndex >= static_cast<uint32_t>(gpuBufferDataIndices_.size())) {
        return;
    }

    auto& stateRef = GetBufferResourceStateRef(res.handle, state.gpuQueue);
    const GpuResourceState& prevState = state;
    const ResourceBarrier prevStateRb = GetSrcBufferBarrier(prevState, res);
    if ((prevStateRb.accessFlags != prevState.accessFlags) || (prevStateRb.accessFlags & WRITE_ACCESS_FLAGS)) {
        params.combinedBarriers.emplace_back(CommandBarrier {
            res.handle, prevStateRb, state.gpuQueue, GetDstBufferBarrier(prevState, res), params.gpuQueue });
    }

    stateRef.state = prevState;
    stateRef.resource = res;
    stateRef.prevRc = params.rcWithType;
    stateRef.prevRenderNodeIndex = params.renderNodeIndex;
}

void RenderGraph::AddCommandBarrierAndUpdateStateCacheBuffer(const uint32_t renderNodeIndex,
    const GpuResourceState& newGpuResourceState, const BindableBuffer& newBuffer,
    const RenderCommandWithType& rcWithType, vector<CommandBarrier>& barriers,
    vector<RenderGraph::GpuQueueTransferState>& currNodeGpuResourceTransfer)
{
    auto& stateRef = GetBufferResourceStateRef(newBuffer.handle, newGpuResourceState.gpuQueue);
    const GpuResourceState srcState = stateRef.state;
    const BindableBuffer srcBuffer = stateRef.resource;

    if ((srcState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED) &&
        (srcState.gpuQueue.type != newGpuResourceState.gpuQueue.type)) {
        PLUGIN_ASSERT(newGpuResourceState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED);
        PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(newBuffer.handle) == RenderHandleType::GPU_IMAGE);
        PLUGIN_ASSERT(stateRef.prevRenderNodeIndex != renderNodeIndex);
        currNodeGpuResourceTransfer.emplace_back(
            RenderGraph::GpuQueueTransferState { newBuffer.handle, stateRef.prevRenderNodeIndex, renderNodeIndex,
                ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED, ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED });
    } else {
        const ResourceBarrier srcBarrier = GetSrcBufferBarrier(srcState, srcBuffer);
        const ResourceBarrier dstBarrier = GetDstBufferBarrier(newGpuResourceState, newBuffer);

        barriers.emplace_back(CommandBarrier {
            newBuffer.handle, srcBarrier, srcState.gpuQueue, dstBarrier, newGpuResourceState.gpuQueue });
    }

    stateRef.state = newGpuResourceState;
    stateRef.resource = newBuffer;
    stateRef.prevRc = rcWithType;
    stateRef.prevRenderNodeIndex = renderNodeIndex;
}

void RenderGraph::AddCommandBarrierAndUpdateStateCacheImage(const uint32_t renderNodeIndex,
    const GpuResourceState& newGpuResourceState, const BindableImage& newImage, const RenderCommandWithType& rcWithType,
    vector<CommandBarrier>& barriers, vector<RenderGraph::GpuQueueTransferState>& currNodeGpuResourceTransfer)
{
    // newGpuResourceState has queue transfer image layout in old optionalImageLayout

    auto& stateRef = GetImageResourceStateRef(newImage.handle, newGpuResourceState.gpuQueue);
    const GpuResourceState srcState = stateRef.state;
    const BindableImage srcImage = stateRef.resource;

    if ((srcState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED) &&
        (srcState.gpuQueue.type != newGpuResourceState.gpuQueue.type)) {
        PLUGIN_ASSERT(newGpuResourceState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED);
        PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(newImage.handle) == RenderHandleType::GPU_IMAGE);
        PLUGIN_ASSERT(stateRef.prevRenderNodeIndex != renderNodeIndex);
        currNodeGpuResourceTransfer.emplace_back(RenderGraph::GpuQueueTransferState { newImage.handle,
            stateRef.prevRenderNodeIndex, renderNodeIndex, srcImage.imageLayout, newImage.imageLayout });
    } else {
        const ResourceBarrier srcBarrier = GetSrcImageBarrier(srcState, srcImage);
        const ResourceBarrier dstBarrier = GetDstImageBarrier(newGpuResourceState, newImage);

        barriers.emplace_back(CommandBarrier {
            newImage.handle, srcBarrier, srcState.gpuQueue, dstBarrier, newGpuResourceState.gpuQueue });
    }

    stateRef.state = newGpuResourceState;
    stateRef.resource = newImage;
    stateRef.prevRc = rcWithType;
    stateRef.prevRenderNodeIndex = renderNodeIndex;
}

RenderGraph::RenderGraphBufferState& RenderGraph::GetBufferResourceStateRef(
    const RenderHandle handle, const GpuQueue& queue)
{
    // NOTE: Do not call with non dynamic trackable
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GPU_BUFFER);
    if (arrayIndex < gpuBufferDataIndices_.size()) {
        PLUGIN_ASSERT(RenderHandleUtil::IsDynamicResource(handle));
        uint32_t dataIdx = gpuBufferDataIndices_[arrayIndex];
        if (dataIdx == INVALID_TRACK_IDX) {
            if (!gpuBufferAvailableIndices_.empty()) {
                dataIdx = gpuBufferAvailableIndices_.back();
                gpuBufferAvailableIndices_.pop_back();
            } else {
                dataIdx = static_cast<uint32_t>(gpuBufferTracking_.size());
                gpuBufferTracking_.emplace_back();
                gpuBufferTracking_[dataIdx].resource.handle = handle;
                gpuBufferTracking_[dataIdx].state.gpuQueue = queue; // current queue for default state
            }
            gpuBufferDataIndices_[arrayIndex] = dataIdx;
        }
        return gpuBufferTracking_[dataIdx];
    }

    return defaultBufferState_;
}

RenderGraph::RenderGraphImageState& RenderGraph::GetImageResourceStateRef(
    const RenderHandle handle, const GpuQueue& queue)
{
    // NOTE: Do not call with non dynamic trackable
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
    PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(handle) == RenderHandleType::GPU_IMAGE);
    if (arrayIndex < gpuImageDataIndices_.size()) {
        PLUGIN_ASSERT(RenderHandleUtil::IsDynamicResource(handle));
        uint32_t dataIdx = gpuImageDataIndices_[arrayIndex];
        if (dataIdx == INVALID_TRACK_IDX) {
            if (!gpuImageAvailableIndices_.empty()) {
                dataIdx = gpuImageAvailableIndices_.back();
                gpuImageAvailableIndices_.pop_back();
            } else {
                dataIdx = static_cast<uint32_t>(gpuImageTracking_.size());
                gpuImageTracking_.emplace_back();
                gpuImageTracking_[dataIdx].resource.handle = handle;
                gpuImageTracking_[dataIdx].state.gpuQueue = queue; // current queue for default state
            }
            gpuImageDataIndices_[arrayIndex] = dataIdx;
        }
        return gpuImageTracking_[dataIdx];
    }

    return defaultImageState_;
}
RENDER_END_NAMESPACE()
