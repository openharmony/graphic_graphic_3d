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

#include "render_graph.h"

#include <cinttypes>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/math/mathf.h>
#include <render/namespace.h>

#include "device/device.h"
#include "device/gpu_resource_cache.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
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
    if constexpr (CORE_RENDER_GRAPH_FULL_DEBUG_PRINT) {
        switch (rc.type) {
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
            case RenderCommandType::COPY_IMAGE: {
                PLUGIN_LOG_I("rc: CopyImage");
                break;
            }
            case RenderCommandType::BLIT_IMAGE: {
                PLUGIN_LOG_I("rc: BlitImage");
                break;
            }
            case RenderCommandType::BARRIER_POINT: {
                PLUGIN_LOG_I("rc: BarrierPoint");
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
            case RenderCommandType::BUILD_ACCELERATION_STRUCTURE: {
                PLUGIN_LOG_I("rc: BuildAccelerationStructure");
                break;
            }
            case RenderCommandType::COPY_ACCELERATION_STRUCTURE_INSTANCES: {
                PLUGIN_LOG_I("rc: CopyAccelerationStructureInstances");
                break;
            }
            case RenderCommandType::CLEAR_COLOR_IMAGE: {
                PLUGIN_LOG_I("rc: ClearColorImage");
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
            case RenderCommandType::DYNAMIC_STATE_FRAGMENT_SHADING_RATE: {
                PLUGIN_LOG_I("rc: DynamicStateFragmentShadingRate");
                break;
            }
            case RenderCommandType::EXECUTE_BACKEND_FRAME_POSITION: {
                PLUGIN_LOG_I("rc: ExecuteBackendFramePosition");
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
            case RenderCommandType::BEGIN_DEBUG_MARKER: {
                PLUGIN_LOG_I("rc: BeginDebugMarker");
                break;
            }
            case RenderCommandType::END_DEBUG_MARKER: {
                PLUGIN_LOG_I("rc: EndDebugMarker");
                break;
            }
            case RenderCommandType::UNDEFINED:
            case RenderCommandType::COUNT: {
                PLUGIN_ASSERT(false && "non-valid render command");
                break;
            }
        }
    }
}

void DebugBarrierPrint(const GpuResourceManager& gpuResourceMgr, const vector<CommandBarrier>& combinedBarriers)
{
    if constexpr (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
        for (const auto& ref : combinedBarriers) {
            const RenderHandleType type = RenderHandleUtil::GetHandleType(ref.resourceHandle);
            if (type == RenderHandleType::GPU_BUFFER) {
                PLUGIN_LOG_I("barrier buffer    :: handle:0x%" PRIx64 " name:%s, src_stage:%u dst_stage:%u",
                    ref.resourceHandle.id, gpuResourceMgr.GetName(ref.resourceHandle).c_str(),
                    ref.src.pipelineStageFlags, ref.dst.pipelineStageFlags);
            } else {
                PLUGIN_ASSERT(type == RenderHandleType::GPU_IMAGE);
                PLUGIN_LOG_I("barrier image     :: handle:0x%" PRIx64
                             " name:%s, src_stage:%u dst_stage:%u, src_layout:%u dst_layout:%u",
                    ref.resourceHandle.id, gpuResourceMgr.GetName(ref.resourceHandle).c_str(),
                    ref.src.pipelineStageFlags, ref.dst.pipelineStageFlags, ref.src.optionalImageLayout,
                    ref.dst.optionalImageLayout);
            }
        }
    }
}

void DebugRenderPassLayoutPrint(const GpuResourceManager& gpuResourceMgr, const RenderCommandBeginRenderPass& rc)
{
    if constexpr (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
        for (uint32_t idx = 0; idx < rc.renderPassDesc.attachmentCount; ++idx) {
            const auto handle = rc.renderPassDesc.attachmentHandles[idx];
            const auto srcLayout = rc.imageLayouts.attachmentInitialLayouts[idx];
            const auto dstLayout = rc.imageLayouts.attachmentFinalLayouts[idx];
            PLUGIN_LOG_I("render_pass image :: handle:0x%" PRIx64
                         " name:%s, src_layout:%u dst_layout:%u (patched later)",
                handle.id, gpuResourceMgr.GetName(handle).c_str(), srcLayout, dstLayout);
        }
    }
}

void DebugPrintImageState(const GpuResourceManager& gpuResourceMgr, const RenderGraph::RenderGraphImageState& resState)
{
    if constexpr (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
        // NOTE: gpuHandle might be the same when generation index wraps around
        // and when using shallow handles (shadow -> re-use normal -> shadow -> re-use normal etc)
        const EngineResourceHandle gpuHandle = gpuResourceMgr.GetGpuHandle(resState.resource.handle);
        PLUGIN_LOG_I("image_state   :: handle:0x%" PRIx64 " name:%s, layout:%u, index:%u, gen:%u, gpu_gen:%u",
            resState.resource.handle.id, gpuResourceMgr.GetName(resState.resource.handle).c_str(),
            resState.resource.imageLayout, RenderHandleUtil::GetIndexPart(resState.resource.handle),
            RenderHandleUtil::GetGenerationIndexPart(resState.resource.handle),
            RenderHandleUtil::GetGenerationIndexPart(gpuHandle));
        // one could fetch and print vulkan handle here as well e.g.
        // 1. const GpuImagePlatformDataVk& plat =
        // 2. (const GpuImagePlatformDataVk&)gpuResourceMgr.GetImage(ref.first)->GetBasePlatformData()
        // 3. PLUGIN_LOG_I("end_frame image   :: vk_handle:0x%" PRIx64, VulkanHandleCast<uint64_t>(plat.image))
    }
}
#endif // RENDER_DEV_ENABLED

constexpr uint32_t WRITE_ACCESS_FLAGS = CORE_ACCESS_SHADER_WRITE_BIT | CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
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
}

void UpdateMultiRenderCommandListRenderPasses(Device& device, RenderGraph::MultiRenderPassStore& store)
{
    const auto renderPassCount = (uint32_t)store.renderPasses.size();
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

    // copy subpasses to first and mark if merging subpasses
    bool mergeSubpasses = false;
    for (uint32_t idx = 1; idx < renderPassCount; ++idx) {
        if ((idx < store.renderPasses.size()) && (idx < store.renderPasses[idx]->subpasses.size())) {
            firstRenderPass->subpasses[idx] = store.renderPasses[idx]->subpasses[idx];
            if (firstRenderPass->subpasses[idx].subpassFlags & SubpassFlagBits::CORE_SUBPASS_MERGE_BIT) {
                mergeSubpasses = true;
            }
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if ((idx >= store.renderPasses.size()) || (idx >= store.renderPasses[idx]->subpasses.size())) {
            PLUGIN_LOG_W("Invalid render pass subpass configuration for multi render pass");
        }
#endif
    }
    // NOTE: only use merge subpasses in vulkan at the moment
    if (device.GetBackendType() != DeviceBackendType::VULKAN) {
        mergeSubpasses = false;
    }

    uint32_t subpassCount = renderPassCount;
    if (mergeSubpasses) {
        PLUGIN_ASSERT(renderPassCount > 1U);
        // merge from back to front
        const uint32_t finalSubpass = renderPassCount - 1U;
        uint32_t mergeCount = 0U;
        for (uint32_t idx = finalSubpass; idx > 0U; --idx) {
            if (firstRenderPass->subpasses[idx].subpassFlags & SubpassFlagBits::CORE_SUBPASS_MERGE_BIT) {
                PLUGIN_ASSERT(idx > 0U);

                uint32_t prevSubpassIdx = idx - 1U;
                auto& currSubpass = firstRenderPass->subpasses[idx];
                auto& prevSubpass = firstRenderPass->subpasses[prevSubpassIdx];
                // cannot merge in these cases
                if (currSubpass.inputAttachmentCount != prevSubpass.inputAttachmentCount) {
                    currSubpass.subpassFlags &= SubpassFlagBits::CORE_SUBPASS_MERGE_BIT;
#if (RENDER_VALIDATION_ENABLED == 1)
                    PLUGIN_LOG_W(
                        "RENDER_VALIDATION: Trying to merge subpasses with input attachments, undefined results");
#endif
                }
                if (prevSubpass.resolveAttachmentCount > currSubpass.resolveAttachmentCount) {
                    currSubpass.subpassFlags &= SubpassFlagBits::CORE_SUBPASS_MERGE_BIT;
#if (RENDER_VALIDATION_ENABLED == 1)
                    PLUGIN_LOG_W("RENDER_VALIDATION: Trying to merge subpasses with different resolve counts, "
                                 "undefined results");
#endif
                }
                if ((currSubpass.subpassFlags & SubpassFlagBits::CORE_SUBPASS_MERGE_BIT) == 0) {
                    // merge failed -> continue
                    continue;
                }

                mergeCount++;
                auto& currRenderPass = store.renderPasses[idx];
                const auto& currSubpassResourceStates = currRenderPass->subpassResourceStates[idx];
                currRenderPass->subpassStartIndex = currRenderPass->subpassStartIndex - 1U;
                // can merge
                currSubpass.subpassFlags |= SubpassFlagBits::CORE_SUBPASS_MERGE_BIT;

                auto& prevRenderPass = store.renderPasses[prevSubpassIdx];
                auto& prevSubpassResourceStates = prevRenderPass->subpassResourceStates[prevSubpassIdx];
                // NOTE: at the moment copies everything from the current subpass
                CloneData(&prevSubpass, sizeof(RenderPassSubpassDesc), &currSubpass, sizeof(RenderPassSubpassDesc));
                // copy layouts and states from the current to previous
                for (uint32_t resourceIdx = 0U; resourceIdx < PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT;
                     ++resourceIdx) {
                    prevSubpassResourceStates.layouts[resourceIdx] = currSubpassResourceStates.layouts[resourceIdx];
                    prevSubpassResourceStates.states[resourceIdx] = currSubpassResourceStates.states[resourceIdx];
                }
            }
        }

        // new minimal subpass count
        subpassCount = subpassCount - mergeCount;
        firstRenderPass->renderPassDesc.subpassCount = subpassCount;
        firstRenderPass->subpasses = { firstRenderPass->subpasses.data(), subpassCount };
        // update subpass start indices
        uint32_t subpassStartIndex = 0;
        for (uint32_t idx = 1U; idx < renderPassCount; ++idx) {
            auto& currRenderPass = store.renderPasses[idx];
            if (currRenderPass->subpasses[idx].subpassFlags & SubpassFlagBits::CORE_SUBPASS_MERGE_BIT) {
                currRenderPass->subpassStartIndex = subpassStartIndex;
            } else {
                subpassStartIndex++;
            }
        }
    }

    // copy from first to following render passes
    for (uint32_t idx = 1; idx < renderPassCount; ++idx) {
        // subpass start index is the only changing variables
        auto& currRenderPass = store.renderPasses[idx];
        const uint32_t subpassStartIndex = currRenderPass->subpassStartIndex;
        currRenderPass->renderPassDesc = firstRenderPass->renderPassDesc;
        // advance subpass start index if not merging
        if (mergeSubpasses &&
            ((idx < currRenderPass->subpasses.size()) &&
                (currRenderPass->subpasses[idx].subpassFlags & SubpassFlagBits::CORE_SUBPASS_MERGE_BIT))) {
            // NOTE: subpassResourceStates are copied in this case
            currRenderPass->subpassResourceStates[subpassStartIndex] =
                firstRenderPass->subpassResourceStates[subpassStartIndex];
        }
        currRenderPass->subpassStartIndex = subpassStartIndex;
        // copy all subpasses and input resource states
        currRenderPass->subpasses = firstRenderPass->subpasses;
        currRenderPass->inputResourceStates = firstRenderPass->inputResourceStates;
        // image layouts needs to match
        currRenderPass->imageLayouts = firstRenderPass->imageLayouts;
        // NOTE: subpassResourceStates are only copied when doing merging
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

ResourceBarrier GetSrcImageBarrierMips(const GpuResourceState& state, const BindableImage& src,
    const BindableImage& dst, const RenderGraph::RenderGraphAdditionalImageState& additionalImageState)
{
    uint32_t mipLevel = 0U;
    uint32_t mipCount = PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS;
    ImageLayout srcImageLayout = src.imageLayout;
    if ((src.mip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) ||
        (dst.mip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS)) {
        if (dst.mip < RenderGraph::MAX_MIP_STATE_COUNT) {
            mipLevel = dst.mip;
            mipCount = 1U;
        } else {
            mipLevel = src.mip;
            // all mip levels
        }
        PLUGIN_ASSERT(additionalImageState.layouts);
        srcImageLayout = additionalImageState.layouts[mipLevel];
    }
    return {
        state.accessFlags,
        state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        srcImageLayout,
        0,
        PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
        { 0, mipLevel, mipCount, 0u, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS },
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

ResourceBarrier GetDstImageBarrierMips(
    const GpuResourceState& state, const BindableImage& src, const BindableImage& dst)
{
    uint32_t mipLevel = 0U;
    uint32_t mipCount = PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS;
    ImageLayout dstImageLayout = dst.imageLayout;
    if ((src.mip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) ||
        (dst.mip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS)) {
        if (dst.mip < RenderGraph::MAX_MIP_STATE_COUNT) {
            mipLevel = dst.mip;
            mipCount = 1U;
        } else {
            mipLevel = src.mip;
            // all mip levels
        }
    }
    return {
        state.accessFlags,
        state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
        dstImageLayout,
        0,
        PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
        { 0, mipLevel, mipCount, 0u, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS },
    };
}

void ModifyAdditionalImageState(
    const BindableImage& res, RenderGraph::RenderGraphAdditionalImageState& additionalStateRef)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    // NOTE: should not be called for images without CORE_RESOURCE_HANDLE_ADDITIONAL_STATE
    PLUGIN_ASSERT(RenderHandleUtil::IsDynamicAdditionalStateResource(res.handle));
#endif
    if (additionalStateRef.layouts) {
        if ((res.mip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) &&
            (res.mip < RenderGraph::MAX_MIP_STATE_COUNT)) {
            additionalStateRef.layouts[res.mip] = res.imageLayout;
        } else {
            // set layout for all mips
            for (uint32_t idx = 0; idx < RenderGraph::MAX_MIP_STATE_COUNT; ++idx) {
                additionalStateRef.layouts[idx] = res.imageLayout;
            }
        }
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E(to_hex(res.handle.id), "mip layouts missing");
#endif
    }
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

bool CheckForBarrierNeed(const unordered_map<RenderHandle, uint32_t>& handledCustomBarriers,
    const uint32_t customBarrierCount, const RenderHandle handle)
{
    bool needsBarrier = RenderHandleUtil::IsDynamicResource(handle);
    if ((customBarrierCount > 0) && needsBarrier) {
        needsBarrier = (handledCustomBarriers.count(handle) == 0);
    }
    return needsBarrier;
}

void ReleaseOwneship(RenderNodeContextData* releaseNodeRef, RenderNodeContextData* acquireNodeRef,
    const CommandBarrier transferBarrier, const uint32_t releaseNodeIdx, const uint32_t rngQueueTransferIdx,
    const uint32_t frameRenderNodeContextDataCount)
{
    // Release barriers at the end
    const uint32_t rcIndex = releaseNodeRef->renderCommandList->GetRenderCommandCount() - 1;
    const RenderCommandWithType& cmdRef = releaseNodeRef->renderCommandList->GetRenderCommands()[rcIndex];
    PLUGIN_ASSERT(cmdRef.type == RenderCommandType::BARRIER_POINT);
    const auto& rcbp = *static_cast<RenderCommandBarrierPoint*>(cmdRef.rc);
    PLUGIN_ASSERT(rcbp.renderCommandType == RenderCommandType::GPU_QUEUE_TRANSFER_RELEASE);

    const uint32_t barrierPointIndex = rcbp.barrierPointIndex;
    releaseNodeRef->renderBarrierList->AddBarriersToBarrierPoint(barrierPointIndex, { transferBarrier });

    // inform that we are patching valid barriers
    releaseNodeRef->renderCommandList->SetValidGpuQueueReleaseAcquireBarriers();

    // Set wait dependency, cross queue ownership transfer needs to be synced with semaphores
    if (releaseNodeIdx < frameRenderNodeContextDataCount) {
        const uint32_t waitIdx = acquireNodeRef->submitInfo.waitSemaphoreCount++;
        if (waitIdx < PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS) {
            acquireNodeRef->submitInfo.waitSemaphoreNodeIndices[waitIdx] = releaseNodeIdx;
            releaseNodeRef->submitInfo.signalSemaphore = true;
        } else {
            PLUGIN_LOG_E("multi-queue sync may work incorrectly, wait semaphore count exceeded (%u)",
                PipelineStateConstants::MAX_RENDER_NODE_GPU_WAIT_SIGNALS);
        }
    } else {
        acquireNodeRef->submitInfo.waitSemaphoreRenderNodeGraphIndex = rngQueueTransferIdx;
    }
}
} // namespace

RenderGraph::RenderGraph(Device& device)
    : device_(device), gpuResourceMgr_((GpuResourceManager&)device.GetGpuResourceManager())
{}

void RenderGraph::BeginFrame()
{
    stateCache_.multiRenderPassStore.renderPasses.clear();
    stateCache_.multiRenderPassStore.firstRenderPassBarrierList = nullptr;
    stateCache_.multiRenderPassStore.supportOpen = false;
    stateCache_.multiRenderPassStore.fistRenderPassCommandListIndex = { ~0u };
    stateCache_.nodeCounter = 0u;
    stateCache_.checkForBackbufferDependency = false;
    stateCache_.usesSwapchainImage = false;
}

void RenderGraph::ProcessRenderNodeGraph(
    const bool checkBackbufferDependancy, const array_view<RenderNodeGraphNodeStore*> renderNodeGraphNodeStores)
{
    stateCache_.checkForBackbufferDependency = checkBackbufferDependancy;

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
                    gpuImageAvailableIndices_.push_back(dataIdx);
                }
                gpuImageDataIndices_[arrayIndex] = INVALID_TRACK_IDX;
            } else if (arrayIndex < static_cast<uint32_t>(gpuBufferDataIndices_.size())) {
                if (const uint32_t dataIdx = gpuBufferDataIndices_[arrayIndex]; dataIdx != INVALID_TRACK_IDX) {
                    PLUGIN_ASSERT(dataIdx < static_cast<uint32_t>(gpuBufferTracking_.size()));
                    gpuBufferTracking_[dataIdx] = {}; // reset
                    gpuBufferAvailableIndices_.push_back(dataIdx);
                }
                gpuBufferDataIndices_[arrayIndex] = INVALID_TRACK_IDX;
            }
        }
    }

    gpuBufferDataIndices_.resize(gpuResourceMgr_.GetBufferHandleCount(), INVALID_TRACK_IDX);
    gpuImageDataIndices_.resize(gpuResourceMgr_.GetImageHandleCount(), INVALID_TRACK_IDX);

#if (RENDER_DEV_ENABLED == 1)
    if constexpr (CORE_RENDER_GRAPH_FULL_DEBUG_PRINT || CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES ||
                  CORE_RENDER_GRAPH_FULL_DEBUG_ATTACHMENTS) {
        static uint64_t debugFrame = 0;
        debugFrame++;
        PLUGIN_LOG_I("START RENDER GRAPH, FRAME %" PRIu64, debugFrame);
    }
#endif

    // need to store some of the resource for frame state in undefined state (i.e. reset on frame boundaries)
    ProcessRenderNodeGraphNodeStores(renderNodeGraphNodeStores, stateCache_);

    // store final state for next frame
    StoreFinalBufferState();
    StoreFinalImageState(); // processes gpuImageBackbufferState_ as well
}

RenderGraph::SwapchainStates RenderGraph::GetSwapchainResourceStates() const
{
    return swapchainStates_;
}

void RenderGraph::ProcessRenderNodeGraphNodeStores(
    const array_view<RenderNodeGraphNodeStore*>& renderNodeGraphNodeStores, StateCache& stateCache)
{
    RenderNodeGraphNodeStore* rngQueueTransferStore = nullptr;
    uint32_t rngQueueTransferGraphIdx = ~0U;
    for (uint32_t graphIdx = 0; graphIdx < renderNodeGraphNodeStores.size(); ++graphIdx) {
        RenderNodeGraphNodeStore* graphStore = renderNodeGraphNodeStores[graphIdx];

        PLUGIN_ASSERT(graphStore);
        if (!graphStore) {
            continue;
        }

        // Identify the queue transfer
        if (!rngQueueTransferStore && graphStore->renderNodeGraphName == "CORE_RNG_QUEUE_TRANSFER") {
            rngQueueTransferStore = graphStore;
        }

        if (rngQueueTransferStore && rngQueueTransferGraphIdx == ~0U) {
            auto& lastNodeRef = rngQueueTransferStore->renderNodeContextData.back();
            lastNodeRef.submitInfo.signalSemaphore = true;
            rngQueueTransferGraphIdx = graphIdx;
        }

        for (uint32_t nodeIdx = 0; nodeIdx < (uint32_t)graphStore->renderNodeContextData.size(); ++nodeIdx) {
            auto& ref = graphStore->renderNodeContextData[nodeIdx];
            ref.submitInfo.waitForSwapchainAcquireSignal = false; // reset
            stateCache.usesSwapchainImage = false;                // reset

#if (RENDER_DEV_ENABLED == 1)
            if constexpr (CORE_RENDER_GRAPH_FULL_DEBUG_PRINT) {
                PLUGIN_LOG_I("FULL NODENAME %s", graphStore->renderNodeData[nodeIdx].fullName.data());
            }
#endif

            if (stateCache.multiRenderPassStore.supportOpen && (stateCache.multiRenderPassStore.renderPasses.empty())) {
                PLUGIN_LOG_E("invalid multi render node render pass subpass stitching");
                // NOTE: add more error handling and invalidate render command lists
            }
            stateCache.multiRenderPassStore.supportOpen = ref.renderCommandList->HasMultiRenderCommandListSubpasses();
            array_view<const RenderCommandWithType> cmdListRef = ref.renderCommandList->GetRenderCommands();
            // go through commands that affect or need transitions and barriers
            ProcessRenderNodeCommands(cmdListRef, nodeIdx, ref, stateCache);

            // needs backbuffer/swapchain wait
            if (stateCache.usesSwapchainImage) {
                ref.submitInfo.waitForSwapchainAcquireSignal = true;
            }

            // patch gpu resource queue transfers
            if (!currNodeGpuResourceTransfers_.empty()) {
                PatchGpuResourceQueueTransfers(
                    rngQueueTransferStore, rngQueueTransferGraphIdx, graphStore->renderNodeContextData);
            }

            stateCache_.nodeCounter++;
        }
    }
}

void RenderGraph::PatchGpuResourceQueueTransfers(RenderNodeGraphNodeStore* rngQueueTransferStore,
    uint32_t rngQueueTransferIdx, array_view<RenderNodeContextData> frameRenderNodeContextData)
{
    for (const auto& transferRef : currNodeGpuResourceTransfers_) {
        PLUGIN_ASSERT(transferRef.acquireNodeIdx < (uint32_t)frameRenderNodeContextData.size());
        if (transferRef.acquireNodeIdx >= frameRenderNodeContextData.size()) {
            // skip
            continue;
        }

        RenderNodeContextData* acquireNodeRef = &frameRenderNodeContextData[transferRef.acquireNodeIdx];
        const GpuQueue acquireGpuQueue = acquireNodeRef->renderCommandList->GetGpuQueue();
        RenderNodeContextData* releaseNodeRef = nullptr;
        GpuQueue releaseGpuQueue = transferRef.releaseQueue;
        if (transferRef.releaseNodeIdx < (uint32_t)frameRenderNodeContextData.size()) {
            GpuQueue nodeQueue =
                frameRenderNodeContextData[transferRef.releaseNodeIdx].renderCommandList->GetGpuQueue();
            releaseNodeRef = &frameRenderNodeContextData[transferRef.releaseNodeIdx];
            PLUGIN_ASSERT(releaseGpuQueue.type == nodeQueue.type);
        } else if (rngQueueTransferStore && rngQueueTransferStore->renderNodeContextData.size() >= 2) {
            // Handles the case if there's no release node idx on the current node graph, so we release the
            // resource in the previous rendernodegraph
            if (releaseGpuQueue.type == GpuQueue::QueueType::GRAPHICS) {
                releaseNodeRef = &(rngQueueTransferStore->renderNodeContextData[0]);
            } else if (releaseGpuQueue.type == GpuQueue::QueueType::COMPUTE) {
                releaseNodeRef = &(rngQueueTransferStore->renderNodeContextData[1]);
            }
        }

        const CommandBarrier transferBarrier = GetQueueOwnershipTransferBarrier(transferRef.handle, releaseGpuQueue,
            acquireGpuQueue, transferRef.optionalReleaseImageLayout, transferRef.optionalAcquireImageLayout);

        // release ownership (NOTE: not done for previous frame)
        if (releaseNodeRef) {
            // Release barriers at the end
            ReleaseOwneship(releaseNodeRef, acquireNodeRef, transferBarrier, transferRef.releaseNodeIdx,
                rngQueueTransferIdx, (uint32_t)frameRenderNodeContextData.size());
        }

        // acquire ownership
        {
            // Acquire barriers at the end
            const uint32_t rcIndex = 0;
            const RenderCommandWithType& cmdRef = acquireNodeRef->renderCommandList->GetRenderCommands()[rcIndex];
            PLUGIN_ASSERT(cmdRef.type == RenderCommandType::BARRIER_POINT);
            const auto& rcbp = *static_cast<RenderCommandBarrierPoint*>(cmdRef.rc);
            PLUGIN_ASSERT(rcbp.renderCommandType == RenderCommandType::GPU_QUEUE_TRANSFER_ACQUIRE);

            const uint32_t barrierPointIndex = rcbp.barrierPointIndex;
            acquireNodeRef->renderBarrierList->AddBarriersToBarrierPoint(barrierPointIndex, { transferBarrier });

            // inform that we are patching valid barriers
            acquireNodeRef->renderCommandList->SetValidGpuQueueReleaseAcquireBarriers();
        }
    }
    // Clear for next use
    currNodeGpuResourceTransfers_.clear();
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
                RenderCommand(*static_cast<RenderCommandEndRenderPass*>(cmdRef.rc), stateCache);
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
            case RenderCommandType::BIND_DESCRIPTOR_SETS:
            case RenderCommandType::PUSH_CONSTANT:
            case RenderCommandType::BLIT_IMAGE:
            case RenderCommandType::BUILD_ACCELERATION_STRUCTURE:
            case RenderCommandType::CLEAR_COLOR_IMAGE:
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

void RenderGraph::StoreFinalBufferState()
{
    for (auto& ref : gpuBufferTracking_) {
        if (!RenderHandleUtil::IsDynamicResource(ref.resource.handle)) {
            ref = {};
            continue;
        }
        // NOTE: we cannot soft reset here
        // if we do so some buffer usage might overlap in the next frame
        if (RenderHandleUtil::IsResetOnFrameBorders(ref.resource.handle)) {
            // reset, but we do not reset the handle, because the gpuImageTracking_ element is not removed
            const RenderHandle handle = ref.resource.handle;
            ref = {};
            ref.resource.handle = handle;
        }

        // need to reset per frame variables for all buffers (so we do not try to patch or debug from previous
        // frames)
        ref.prevRenderNodeIndex = { ~0u };
    }
}

void RenderGraph::StoreFinalImageState()
{
    swapchainStates_ = {}; // reset

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
        if (stateCache_.checkForBackbufferDependency && RenderHandleUtil::IsSwapchain(ref.resource.handle)) {
            if (ref.prevRc.type == RenderCommandType::BEGIN_RENDER_PASS) {
                RenderCommandBeginRenderPass& beginRenderPass =
                    *static_cast<RenderCommandBeginRenderPass*>(ref.prevRc.rc);
                PatchRenderPassFinalLayout(
                    ref.resource.handle, ImageLayout::CORE_IMAGE_LAYOUT_PRESENT_SRC, beginRenderPass, ref);
            }
            // NOTE: currently we handle automatic presentation layout in vulkan backend if not in render pass
            // store final state for backbuffer
            // currently we only swapchains if they are really in use in this frame
            const uint32_t flags = ref.state.accessFlags | ref.state.shaderStageFlags | ref.state.pipelineStageFlags;
            if (flags != 0) {
                swapchainStates_.swapchains.push_back({ ref.resource.handle, ref.state, ref.resource.imageLayout });
            }
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
            const bool addMips = RenderHandleUtil::IsDynamicAdditionalStateResource(ref.resource.handle);
            // reset, but we do not reset the handle, because the gpuImageTracking_ element is not removed
            const RenderHandle handle = ref.resource.handle;
            ref = {};
            ref.resource.handle = handle;
            if (addMips) {
                PLUGIN_ASSERT(!ref.additionalState.layouts);
                ref.additionalState.layouts = make_unique<ImageLayout[]>(MAX_MIP_STATE_COUNT);
            }
        }
        // NOTE: render pass compatibility hashing with stages and access flags
        // creates quite many new graphics pipelines in the first few frames.
        // else branch with soft reset here could prevent access flags from previous frame.
        // To get this to work one could get the flags from the end of the frame to the begin as well.

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
            subpassResourceStatesRef, finalImageLayouts);

        BeginRenderPassUpdateSubpassImageStates(
            array_view(subpassRef.colorAttachmentIndices, subpassRef.colorAttachmentCount), rc.renderPassDesc,
            subpassResourceStatesRef, finalImageLayouts);

        BeginRenderPassUpdateSubpassImageStates(
            array_view(subpassRef.resolveAttachmentIndices, subpassRef.resolveAttachmentCount), rc.renderPassDesc,
            subpassResourceStatesRef, finalImageLayouts);

        if (subpassRef.depthAttachmentCount == 1u) {
            BeginRenderPassUpdateSubpassImageStates(
                array_view(&subpassRef.depthAttachmentIndex, subpassRef.depthAttachmentCount), rc.renderPassDesc,
                subpassResourceStatesRef, finalImageLayouts);
            if (subpassRef.depthResolveAttachmentCount == 1) {
                BeginRenderPassUpdateSubpassImageStates(
                    array_view(&subpassRef.depthResolveAttachmentIndex, subpassRef.depthResolveAttachmentCount),
                    rc.renderPassDesc, subpassResourceStatesRef, finalImageLayouts);
            }
        }
        if (subpassRef.fragmentShadingRateAttachmentCount == 1u) {
            BeginRenderPassUpdateSubpassImageStates(array_view(&subpassRef.fragmentShadingRateAttachmentIndex,
                                                        subpassRef.fragmentShadingRateAttachmentCount),
                rc.renderPassDesc, subpassResourceStatesRef, finalImageLayouts);
        }
    }

    if (hasRenderPassDependency) { // stitch render pass subpasses
        if (rc.subpassStartIndex > 0) {
            // stitched to behave as a nextSubpass() and not beginRenderPass()
            rc.beginType = RenderPassBeginType::RENDER_PASS_SUBPASS_BEGIN;
        } else { // first subpass
            stateCache.multiRenderPassStore.fistRenderPassCommandListIndex = commandListCommandIndex;
        }
        const bool finalSubpass = (rc.subpassStartIndex == rc.renderPassDesc.subpassCount - 1);
        if (finalSubpass) {
            UpdateMultiRenderCommandListRenderPasses(device_, stateCache.multiRenderPassStore);
            // Inserts barriers on color/depth attachments that are common between renderpasses,
            // so we execute renderpasses sequentially that rely on the resource to avoid sync issues
            InsertRenderPassBarrier(stateCache.multiRenderPassStore);
            stateCache.multiRenderPassStore.fistRenderPassCommandListIndex = ~0u;
            // multiRenderPassStore cleared in EndRenderPass
        }
    }
#if (RENDER_DEV_ENABLED == 1)
    if constexpr (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
        DebugRenderPassLayoutPrint(gpuResourceMgr_, rc);
    }
#endif
}

void RenderGraph::BeginRenderPassHandleDependency(
    BeginRenderPassParameters& params, const uint32_t commandListCommandIndex, RenderNodeContextData& nodeData)
{
    params.stateCache.multiRenderPassStore.renderPasses.push_back(&params.rc);
    // store the first begin render pass
    params.rpForCmdRef = { RenderCommandType::BEGIN_RENDER_PASS,
        params.stateCache.multiRenderPassStore.renderPasses[0] };

    if (params.rc.subpassStartIndex == 0) { // store the first render pass barrier point
#ifndef NDEBUG
        // barrier point must be previous command
        PLUGIN_ASSERT(commandListCommandIndex >= 1);
        const uint32_t prevCommandIndex = commandListCommandIndex - 1;
        const RenderCommandWithType& barrierPointCmdRef =
            nodeData.renderCommandList->GetRenderCommands()[prevCommandIndex];
        PLUGIN_ASSERT(barrierPointCmdRef.type == RenderCommandType::BARRIER_POINT);
        PLUGIN_ASSERT(static_cast<RenderCommandBarrierPoint*>(barrierPointCmdRef.rc));
#endif
        params.stateCache.multiRenderPassStore.firstRenderPassBarrierList = nodeData.renderBarrierList.get();
    }

    const auto& beginCmd = params.rc;
    const auto& beginDesc = beginCmd.renderPassDesc;
    // Update attachment state tracking that is modified via InsertRenderPassBarrier
    for (uint32_t subpassIdx = 0; subpassIdx < beginCmd.subpasses.size(); subpassIdx++) {
        const auto& subpass = beginCmd.subpasses[subpassIdx];
        // Color Attachements
        for (uint32_t i = 0; i < subpass.colorAttachmentCount; i++) {
            const uint32_t colIdx = subpass.colorAttachmentIndices[i];
            const auto& attachmentDesc = beginDesc.attachments[colIdx];
            const RenderHandle attachmentHandle = beginDesc.attachmentHandles[colIdx];

            if (attachmentDesc.loadOp != CORE_ATTACHMENT_LOAD_OP_LOAD) {
                auto& attachmentRef = GetImageResourceStateRef(attachmentHandle, {});
                attachmentRef.resource.imageLayout = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            }
        }
        // Depth Attachement
        if (subpass.depthAttachmentIndex < beginDesc.attachmentCount) {
            const uint32_t depthIdx = subpass.depthAttachmentIndex;
            const auto& attachmentDesc = beginDesc.attachments[depthIdx];
            const RenderHandle attachmentHandle = beginDesc.attachmentHandles[depthIdx];

            if (attachmentDesc.loadOp != CORE_ATTACHMENT_LOAD_OP_LOAD &&
                attachmentDesc.stencilLoadOp != CORE_ATTACHMENT_LOAD_OP_LOAD) {
                auto& attachmentRef = GetImageResourceStateRef(attachmentHandle, {});
                attachmentRef.resource.imageLayout = CORE_IMAGE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL;
            }
        }
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
        ImageLayout imgLayout = stateRef.resource.imageLayout;

        const bool addMips = RenderHandleUtil::IsDynamicAdditionalStateResource(handle);
        // image layout is undefined if automatic barriers have been disabled
        if (params.rc.enableAutomaticLayoutChanges) {
            const RenderPassDesc::AttachmentDesc& attachmentDesc = attachments[attachmentIdx];
            if (addMips && (attachmentDesc.mipLevel < RenderGraph::MAX_MIP_STATE_COUNT)) {
                if (stateRef.additionalState.layouts) {
                    imgLayout = stateRef.additionalState.layouts[attachmentDesc.mipLevel];
                } else {
#if (RENDER_VALIDATION_ENABLED == 1)
                    PLUGIN_LOG_ONCE_E(to_hex(handle.id), "mip layouts missing");
#endif
                }
            }

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
        if (params.stateCache.checkForBackbufferDependency && RenderHandleUtil::IsSwapchain(handle)) {
            params.stateCache.usesSwapchainImage = true;
        }
    }
}

void RenderGraph::BeginRenderPassUpdateSubpassImageStates(array_view<const uint32_t> attatchmentIndices,
    const RenderPassDesc& renderPassDesc, const RenderPassAttachmentResourceStates& subpassResourceStatesRef,
    array_view<ImageLayout> finalImageLayouts)
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
        const bool addMips = RenderHandleUtil::IsDynamicAdditionalStateResource(handle);

        ref.state = refState;
        ref.resource.handle = handle;
        ref.resource.imageLayout = refImgLayout;
        if (addMips) {
            const RenderPassDesc::AttachmentDesc& attachmentDesc = renderPassDesc.attachments[attachmentIndex];
            const BindableImage image {
                handle,
                attachmentDesc.mipLevel,
                attachmentDesc.layer,
                refImgLayout,
                RenderHandle {},
            };
            ModifyAdditionalImageState(image, ref.additionalState);
        }
    }
}

void RenderGraph::RenderCommand(RenderCommandEndRenderPass& rc, StateCache& stateCache)
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
    const auto& cmdListRef = nodeData.renderCommandList->GetRenderCommands();
    const auto& allDescriptorSetHandlesForBarriers = nodeData.renderCommandList->GetDescriptorSetHandles();
    const auto& nodeDescriptorSetMgrRef = *nodeData.nodeContextDescriptorSetMgr;

    parameterCachePools_.combinedBarriers.clear();
    parameterCachePools_.handledCustomBarriers.clear();
    ParameterCache parameters { parameterCachePools_.combinedBarriers, parameterCachePools_.handledCustomBarriers,
        rc.customBarrierCount, rc.vertexIndexBarrierCount, rc.indirectBufferBarrierCount, renderNodeIndex,
        nodeData.renderCommandList->GetGpuQueue(), { RenderCommandType::BARRIER_POINT, &rc }, stateCache };
    // first check custom barriers
    if (parameters.customBarrierCount > 0) {
        HandleCustomBarriers(parameters, rc.customBarrierIndexBegin, customBarrierListRef);
    }
    // then vertex / index buffer barriers in the barrier point before render pass
    if (parameters.vertexInputBarrierCount > 0) {
        PLUGIN_ASSERT(rc.renderCommandType == RenderCommandType::BEGIN_RENDER_PASS);
        HandleVertexInputBufferBarriers(parameters, rc.vertexIndexBarrierIndexBegin,
            nodeData.renderCommandList->GetRenderpassVertexInputBufferBarriers());
    }
    if (parameters.indirectBufferBarrierCount > 0U) {
        PLUGIN_ASSERT(rc.renderCommandType == RenderCommandType::BEGIN_RENDER_PASS);
        HandleRenderpassIndirectBufferBarriers(parameters, rc.indirectBufferBarrierIndexBegin,
            nodeData.renderCommandList->GetRenderpassIndirectBufferBarriers());
    }

    // in barrier point the next render command is known for which the barrier is needed
    if (rc.renderCommandType == RenderCommandType::CLEAR_COLOR_IMAGE) {
        HandleClearImage(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::BLIT_IMAGE) {
        HandleBlitImage(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::COPY_BUFFER) {
        HandleCopyBuffer(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::COPY_BUFFER_IMAGE) {
        HandleCopyBufferImage(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::COPY_IMAGE) {
        HandleCopyBufferImage(
            parameters, commandListCommandIndex, cmdListRef); // NOTE: handles image to image descriptor sets
    } else if (rc.renderCommandType == RenderCommandType::BUILD_ACCELERATION_STRUCTURE) {
        HandleBuildAccelerationStructure(parameters, commandListCommandIndex, cmdListRef);
    } else if (rc.renderCommandType == RenderCommandType::COPY_ACCELERATION_STRUCTURE_INSTANCES) {
        HandleCopyAccelerationStructureInstances(parameters, commandListCommandIndex, cmdListRef);
    } else {
        if (rc.renderCommandType == RenderCommandType::DISPATCH_INDIRECT) {
            HandleDispatchIndirect(parameters, commandListCommandIndex, cmdListRef);
        }
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
    if constexpr (CORE_RENDER_GRAPH_PRINT_RESOURCE_STATES) {
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
        auto& cb = params.combinedBarriers.emplace_back(*begin);

        // add a copy and modify if needed
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
                const bool isAddMips = RenderHandleUtil::IsDynamicAdditionalStateResource(cb.resourceHandle);
                auto& stateRef = GetImageResourceStateRef(cb.resourceHandle, params.gpuQueue);
                if (cb.src.optionalImageLayout == CORE_IMAGE_LAYOUT_MAX_ENUM) {
                    uint32_t mipLevel = 0U;
                    uint32_t mipCount = PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS;
                    ImageLayout srcImageLayout = stateRef.resource.imageLayout;
                    if (isAddMips) {
                        const uint32_t srcMip = cb.src.optionalImageSubresourceRange.baseMipLevel;
                        const uint32_t dstMip = cb.dst.optionalImageSubresourceRange.baseMipLevel;
                        if ((srcMip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) ||
                            (dstMip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS)) {
                            if (dstMip < RenderGraph::MAX_MIP_STATE_COUNT) {
                                mipLevel = dstMip;
                                mipCount = 1U;
                            } else {
                                mipLevel = srcMip;
                                // all mip levels
                            }
                            if (stateRef.additionalState.layouts) {
                                srcImageLayout = stateRef.additionalState.layouts[mipLevel];
                            } else {
#if (RENDER_VALIDATION_ENABLED == 1)
                                PLUGIN_LOG_ONCE_E(to_hex(cb.resourceHandle.id), "mip layouts missing");
#endif
                            }
                        }
                    }
                    cb.src.accessFlags = stateRef.state.accessFlags;
                    cb.src.pipelineStageFlags =
                        stateRef.state.pipelineStageFlags | PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                    cb.src.optionalImageLayout = srcImageLayout;
                    cb.src.optionalImageSubresourceRange = { 0, mipLevel, mipCount, 0u,
                        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                }
                UpdateImageResourceState(stateRef, params, cb);
                stateRef.resource.imageLayout = cb.dst.optionalImageLayout;
                if (isAddMips) {
                    const BindableImage image {
                        cb.resourceHandle,
                        cb.dst.optionalImageSubresourceRange.baseMipLevel,
                        cb.dst.optionalImageSubresourceRange.baseArrayLayer,
                        cb.dst.optionalImageLayout,
                        RenderHandle {},
                    };
                    ModifyAdditionalImageState(image, stateRef.additionalState);
                }
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
            const GpuResourceState resourceState { CORE_SHADER_STAGE_VERTEX_BIT,
                CORE_ACCESS_INDEX_READ_BIT | CORE_ACCESS_VERTEX_ATTRIBUTE_READ_BIT,
                CORE_PIPELINE_STAGE_VERTEX_INPUT_BIT, params.gpuQueue };
            UpdateStateAndCreateBarriersGpuBuffer(
                resourceState, { vbInput.bufferHandle, vbInput.bufferOffset, vbInput.byteSize }, params);
        }
    }
}

void RenderGraph::HandleRenderpassIndirectBufferBarriers(ParameterCache& params, const uint32_t barrierIndexBegin,
    const array_view<const VertexBuffer>& indirectBufferBarrierListRef)
{
    for (uint32_t idx = 0; idx < params.indirectBufferBarrierCount; ++idx) {
        const uint32_t barrierIndex = barrierIndexBegin + idx;
        PLUGIN_ASSERT(barrierIndex < (uint32_t)indirectBufferBarrierListRef.size());
        if (barrierIndex < (uint32_t)indirectBufferBarrierListRef.size()) {
            const VertexBuffer& ib = indirectBufferBarrierListRef[barrierIndex];
            const bool needsArgsBarrier =
                CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, ib.bufferHandle);
            if (needsArgsBarrier) {
                const GpuResourceState resourceState { CORE_SHADER_STAGE_VERTEX_BIT,
                    CORE_ACCESS_INDIRECT_COMMAND_READ_BIT, CORE_PIPELINE_STAGE_DRAW_INDIRECT_BIT, params.gpuQueue };
                UpdateStateAndCreateBarriersGpuBuffer(
                    resourceState, { ib.bufferHandle, ib.bufferOffset, ib.byteSize }, params);
            }
        }
    }
}

namespace {
void AddBarrier(const RenderHandle handle, const AccessFlags accessFlags, const PipelineStageFlags srcPipelineFlags,
    const PipelineStageFlags dstPipelineFlags, ImageLayout srcLayout, ImageLayout dstLayout,
    vector<CommandBarrier>& barriers)
{
    barriers.push_back(CommandBarrier { handle,
        ResourceBarrier { accessFlags, srcPipelineFlags, srcLayout, 0, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
            { 0, 0, PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0u,
                PipelineStateConstants::GPU_IMAGE_ALL_LAYERS } },
        GpuQueue {},
        ResourceBarrier { accessFlags, dstPipelineFlags, dstLayout, 0, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE,
            { 0, 0, PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0u,
                PipelineStateConstants::GPU_IMAGE_ALL_LAYERS } },
        GpuQueue {} });
}
} // namespace

void RenderGraph::InsertRenderPassBarrier(const MultiRenderPassStore& store)
{
    if (store.renderPasses.size() < 1 && store.fistRenderPassCommandListIndex == ~0u) {
        return;
    }

    const auto& beginCmd = *store.renderPasses.front();
    const auto& beginDesc = beginCmd.renderPassDesc;
    vector<CommandBarrier> barriers;
    barriers.reserve(beginDesc.attachmentCount);

    // Handle color attachment barriers
    for (uint32_t subpassIdx = 0; subpassIdx < beginCmd.subpasses.size(); subpassIdx++) {
        const auto& subpass = beginCmd.subpasses[subpassIdx];
        const auto& subpassStates = beginCmd.subpassResourceStates[subpassIdx];
        // Color Attachments
        for (uint32_t i = 0; i < subpass.colorAttachmentCount; i++) {
            const uint32_t colIdx = subpass.colorAttachmentIndices[i];
            const auto& attachmentDesc = beginDesc.attachments[colIdx];
            const RenderHandle attachmentHandle = beginDesc.attachmentHandles[colIdx];
            const ImageLayout layout = subpassStates.layouts[colIdx];

            if (attachmentDesc.loadOp != CORE_ATTACHMENT_LOAD_OP_LOAD) {
                // Basically a graphics pipeline flush, ensure prior renderpass using the attachment has finished
                AddBarrier(attachmentHandle, CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
                    CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    CORE_IMAGE_LAYOUT_UNDEFINED, layout, barriers);
            }
        }
        // Depth Attachment
        if (subpass.depthAttachmentIndex < beginDesc.attachmentCount) {
            const uint32_t depthIdx = subpass.depthAttachmentIndex;
            const auto& attachmentDesc = beginDesc.attachments[depthIdx];
            const RenderHandle attachmentHandle = beginDesc.attachmentHandles[depthIdx];
            const ImageLayout layout = subpassStates.layouts[depthIdx];

            if (attachmentDesc.loadOp != CORE_ATTACHMENT_LOAD_OP_LOAD &&
                attachmentDesc.stencilLoadOp != CORE_ATTACHMENT_LOAD_OP_LOAD) {
                // Basically a graphics pipeline flush, ensure prior renderpass using the attachment has finished
                AddBarrier(attachmentHandle, CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
                    CORE_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | CORE_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                    CORE_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT | CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                    CORE_IMAGE_LAYOUT_UNDEFINED, layout, barriers);
            }
        }
    }

    store.firstRenderPassBarrierList->AddBarriersToBarrierPoint(store.fistRenderPassCommandListIndex - 1, barriers);
}

void RenderGraph::HandleClearImage(ParameterCache& params, const uint32_t& commandListCommandIndex,
    const array_view<const RenderCommandWithType>& cmdListRef)
{
    const uint32_t nextListIdx = commandListCommandIndex + 1;
    PLUGIN_ASSERT(nextListIdx < cmdListRef.size());
    const auto& nextCmdRef = cmdListRef[nextListIdx];
    PLUGIN_ASSERT(nextCmdRef.type == RenderCommandType::CLEAR_COLOR_IMAGE);

    const RenderCommandClearColorImage& nextRc = *static_cast<RenderCommandClearColorImage*>(nextCmdRef.rc);

    const bool needsBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, nextRc.handle);
    if (needsBarrier) {
        BindableImage bRes = {};
        bRes.handle = nextRc.handle;
        bRes.imageLayout = nextRc.imageLayout;
        AddCommandBarrierAndUpdateStateCacheImage(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_TRANSFER_WRITE_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
            bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
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
            bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }

    const bool needsDstBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, nextRc.dstHandle);
    if (needsDstBarrier) {
        const BindableBuffer bRes = { nextRc.dstHandle, nextRc.bufferCopy.dstOffset, nextRc.bufferCopy.size };
        AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_TRANSFER_WRITE_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
            bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
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
    ImageSubresourceLayers srcImgLayers;
    ImageSubresourceLayers dstImgLayers;
    if (nextCmdRef.type == RenderCommandType::COPY_BUFFER_IMAGE) {
        const RenderCommandCopyBufferImage& nextRc = *static_cast<RenderCommandCopyBufferImage*>(nextCmdRef.rc);
        PLUGIN_ASSERT(nextRc.copyType != RenderCommandCopyBufferImage::CopyType::UNDEFINED);
        srcHandle = nextRc.srcHandle;
        dstHandle = nextRc.dstHandle;
        srcImgLayers = nextRc.bufferImageCopy.imageSubresource;
        dstImgLayers = nextRc.bufferImageCopy.imageSubresource;
    } else if (nextCmdRef.type == RenderCommandType::COPY_IMAGE) {
        const RenderCommandCopyImage& nextRc = *static_cast<RenderCommandCopyImage*>(nextCmdRef.rc);
        srcHandle = nextRc.srcHandle;
        dstHandle = nextRc.dstHandle;
        srcImgLayers = nextRc.imageCopy.srcSubresource;
        dstImgLayers = nextRc.imageCopy.dstSubresource;
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
                bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
        } else {
            BindableImage bRes;
            bRes.handle = srcHandle;
            bRes.mip = srcImgLayers.mipLevel;
            bRes.layer = srcImgLayers.baseArrayLayer;
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
                bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
        } else {
            BindableImage bRes;
            bRes.handle = dstHandle;
            // Transition all because later stages expect all mips on same layout
            bRes.mip = PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS;
            bRes.layer = PipelineStateConstants::GPU_IMAGE_ALL_LAYERS;
            bRes.imageLayout = CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            AddCommandBarrierAndUpdateStateCacheImage(params.renderNodeIndex,
                GpuResourceState {
                    0, CORE_ACCESS_TRANSFER_WRITE_BIT, CORE_PIPELINE_STAGE_TRANSFER_BIT, params.gpuQueue },
                bRes, params.rcWithType, params.combinedBarriers, currNodeGpuResourceTransfers_);
        }
    }
}

void RenderGraph::HandleBuildAccelerationStructure(ParameterCache& params, const uint32_t& commandListCommandIndex,
    const array_view<const RenderCommandWithType>& cmdListRef)
{
    const uint32_t nextListIdx = commandListCommandIndex + 1;
    PLUGIN_ASSERT(nextListIdx < cmdListRef.size());
    const auto& nextCmdRef = cmdListRef[nextListIdx];
    PLUGIN_ASSERT(nextCmdRef.type == RenderCommandType::BUILD_ACCELERATION_STRUCTURE);

    const RenderCommandBuildAccelerationStructure& nextRc =
        *static_cast<RenderCommandBuildAccelerationStructure*>(nextCmdRef.rc);

    for (const auto& instancesRef : nextRc.instancesView) {
        // usually the bottom level which needs to be waited to be finished
        const RenderHandle handle = instancesRef.data.handle;
        const bool needsBarrier = CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, handle);
        if (needsBarrier) {
            const BindableBuffer bRes = { handle, 0U, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
            AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
                GpuResourceState { 0, CORE_ACCESS_ACCELERATION_STRUCTURE_READ_BIT,
                    CORE_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT, params.gpuQueue },
                bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
        }
    }

    const auto& geometry = nextRc.geometry;

    // NOTE: mostly empty at the moment
    const bool needsSrcBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, geometry.srcAccelerationStructure);
    if (needsSrcBarrier) {
        const BindableBuffer bRes = { geometry.srcAccelerationStructure, 0U,
            PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
        AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_ACCELERATION_STRUCTURE_READ_BIT,
                CORE_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT, params.gpuQueue },
            bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }

    const bool needsDstBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, geometry.dstAccelerationStructure);
    if (needsDstBarrier) {
        const BindableBuffer bRes = { geometry.dstAccelerationStructure, 0U,
            PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
        AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
            GpuResourceState { 0, CORE_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT,
                CORE_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT, params.gpuQueue },
            bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }
}

void RenderGraph::HandleCopyAccelerationStructureInstances(ParameterCache& params,
    const uint32_t& commandListCommandIndex, const array_view<const RenderCommandWithType>& cmdListRef)
{
    const uint32_t nextListIdx = commandListCommandIndex + 1;
    PLUGIN_ASSERT(nextListIdx < cmdListRef.size());
    const auto& nextCmdRef = cmdListRef[nextListIdx];
    PLUGIN_ASSERT(nextCmdRef.type == RenderCommandType::COPY_ACCELERATION_STRUCTURE_INSTANCES);

    const RenderCommandCopyAccelerationStructureInstances& nextRc =
        *static_cast<RenderCommandCopyAccelerationStructureInstances*>(nextCmdRef.rc);

    // NOTE: nextRc.destination.handle will be copied on CPU, no barriers needed

    for (const auto& instancesRef : nextRc.instancesView) {
        const RenderHandle handle = instancesRef.accelerationStructure;
        const bool needsSrcBarrier =
            CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, handle);
        if (needsSrcBarrier) {
            const BindableBuffer bRes = { handle, 0U, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
            AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
                GpuResourceState { 0, CORE_ACCESS_ACCELERATION_STRUCTURE_READ_BIT,
                    CORE_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT, params.gpuQueue },
                bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
        }
    }
}

void RenderGraph::HandleDispatchIndirect(ParameterCache& params, const uint32_t& commandListCommandIndex,
    const BASE_NS::array_view<const RenderCommandWithType>& cmdListRef)
{
    const uint32_t nextListIdx = commandListCommandIndex + 1;
    PLUGIN_ASSERT(nextListIdx < cmdListRef.size());
    const auto& nextCmdRef = cmdListRef[nextListIdx];
    PLUGIN_ASSERT(nextCmdRef.type == RenderCommandType::DISPATCH_INDIRECT);

    const auto& nextRc = *static_cast<RenderCommandDispatchIndirect*>(nextCmdRef.rc);

    const bool needsArgsBarrier =
        CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, nextRc.argsHandle);
    if (needsArgsBarrier) {
        const BindableBuffer bRes = { nextRc.argsHandle, nextRc.offset, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
        AddCommandBarrierAndUpdateStateCacheBuffer(params.renderNodeIndex,
            GpuResourceState { CORE_SHADER_STAGE_COMPUTE_BIT, CORE_ACCESS_INDIRECT_COMMAND_READ_BIT,
                CORE_PIPELINE_STAGE_DRAW_INDIRECT_BIT, params.gpuQueue },
            bRes, params.combinedBarriers, currNodeGpuResourceTransfers_);
    }
}

void RenderGraph::HandleDescriptorSets(ParameterCache& params,
    const array_view<const RenderHandle>& descriptorSetHandlesForBarriers,
    const NodeContextDescriptorSetManager& nodeDescriptorSetMgrRef)
{
    for (const RenderHandle descriptorSetHandle : descriptorSetHandlesForBarriers) {
        if (RenderHandleUtil::GetHandleType(descriptorSetHandle) != RenderHandleType::DESCRIPTOR_SET) {
            continue;
        }

        // NOTE: for global descriptor sets we didn't know with render command list if it had dynamic resources
        const uint32_t additionalData = RenderHandleUtil::GetAdditionalData(descriptorSetHandle);
        if (additionalData & NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT) {
            if (!nodeDescriptorSetMgrRef.HasDynamicBarrierResources(descriptorSetHandle)) {
                continue;
            }
        }

        const auto bindingResources = nodeDescriptorSetMgrRef.GetCpuDescriptorSetData(descriptorSetHandle);
        const auto& buffers = bindingResources.buffers;
        const auto& images = bindingResources.images;
        for (const auto& refBuf : buffers) {
            const auto& ref = refBuf.desc;
            const uint32_t descriptorCount = ref.binding.descriptorCount;
            // skip, array bindings which are bound from first index, they have also descriptorCount 0
            if (descriptorCount == 0) {
                continue;
            }
            const uint32_t arrayOffset = ref.arrayOffset;
            PLUGIN_ASSERT((arrayOffset + descriptorCount - 1) <= buffers.size());
            for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                // first is the ref, starting from 1 we use array offsets
                const auto& bRes = (idx == 0) ? ref : buffers[arrayOffset + idx - 1].desc;
                if (CheckForBarrierNeed(params.handledCustomBarriers, params.customBarrierCount, ref.resource.handle)) {
                    UpdateStateAndCreateBarriersGpuBuffer(bRes.state, bRes.resource, params);
                }
            }
        }
        for (const auto& refImg : images) {
            const auto& ref = refImg.desc;
            const uint32_t descriptorCount = ref.binding.descriptorCount;
            // skip, array bindings which are bound from first index, they have also descriptorCount 0
            if (descriptorCount == 0) {
                continue;
            }
            const uint32_t arrayOffset = ref.arrayOffset;
            PLUGIN_ASSERT((arrayOffset + descriptorCount - 1) <= images.size());
            for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                // first is the ref, starting from 1 we use array offsets
                const auto& bRes = (idx == 0) ? ref : images[arrayOffset + idx - 1].desc;
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
    // NOTE: we previous patched the final render pass layouts here
    // ATM: we only path the swapchain image if needed

    const GpuResourceState& prevState = ref.state;
    const BindableImage& prevImage = ref.resource;
    const bool addMips = RenderHandleUtil::IsDynamicAdditionalStateResource(res.handle);
    const ResourceBarrier prevStateRb = addMips ? GetSrcImageBarrierMips(prevState, prevImage, res, ref.additionalState)
                                                : GetSrcImageBarrier(prevState, prevImage);

    const bool layoutChanged = (prevStateRb.optionalImageLayout != res.imageLayout);
    // NOTE: we are not interested in access flags, only write access interests us
    // not this (prevStateRb.accessFlags NOT state.accessFlags)
    const bool writeTarget = (prevStateRb.accessFlags & WRITE_ACCESS_FLAGS) || (state.accessFlags & WRITE_ACCESS_FLAGS);
    const bool inputAttachment = (state.accessFlags == CORE_ACCESS_INPUT_ATTACHMENT_READ_BIT);
    // input attachments are handled with render passes and not with barriers
    if ((layoutChanged || writeTarget) && (!inputAttachment)) {
        if ((prevState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED) &&
            (prevState.gpuQueue.type != state.gpuQueue.type)) {
            PLUGIN_ASSERT(state.gpuQueue.type != GpuQueue::QueueType::UNDEFINED);

            PLUGIN_ASSERT(ref.prevRenderNodeIndex != params.renderNodeIndex);
            currNodeGpuResourceTransfers_.push_back(
                RenderGraph::GpuQueueTransferState { res.handle, ref.prevRenderNodeIndex, params.renderNodeIndex,
                    ref.state.gpuQueue, prevImage.imageLayout, res.imageLayout });
        } else {
            const ResourceBarrier dstImageBarrier =
                addMips ? GetDstImageBarrierMips(state, prevImage, res) : GetDstImageBarrier(state, res);
            params.combinedBarriers.push_back(
                CommandBarrier { res.handle, prevStateRb, prevState.gpuQueue, dstImageBarrier, params.gpuQueue });
        }

        ref.state = state;
        ref.resource = res;
        ref.prevRc = params.rcWithType;
        ref.prevRenderNodeIndex = params.renderNodeIndex;
        if (addMips) {
            ModifyAdditionalImageState(res, ref.additionalState);
        }
    }
}

void RenderGraph::UpdateStateAndCreateBarriersGpuBuffer(
    const GpuResourceState& dstState, const BindableBuffer& res, RenderGraph::ParameterCache& params)
{
    const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(res.handle);
    if (arrayIndex >= static_cast<uint32_t>(gpuBufferDataIndices_.size())) {
        return;
    }

    // get the current state of the buffer
    auto& srcStateRef = GetBufferResourceStateRef(res.handle, dstState.gpuQueue);
    const ResourceBarrier prevStateRb = GetSrcBufferBarrier(srcStateRef.state, res);
    // if previous or current state is write -> barrier
    if ((prevStateRb.accessFlags & WRITE_ACCESS_FLAGS) || (dstState.accessFlags & WRITE_ACCESS_FLAGS)) {
        params.combinedBarriers.push_back(CommandBarrier {
            res.handle, prevStateRb, dstState.gpuQueue, GetDstBufferBarrier(dstState, res), params.gpuQueue });
    }

    // update the cached state to match the situation after the barrier
    srcStateRef.state = dstState;
    srcStateRef.resource = res;
    srcStateRef.prevRenderNodeIndex = params.renderNodeIndex;
}

void RenderGraph::AddCommandBarrierAndUpdateStateCacheBuffer(const uint32_t renderNodeIndex,
    const GpuResourceState& newGpuResourceState, const BindableBuffer& newBuffer, vector<CommandBarrier>& barriers,
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
        currNodeGpuResourceTransfer.push_back(RenderGraph::GpuQueueTransferState { newBuffer.handle,
            stateRef.prevRenderNodeIndex, renderNodeIndex, srcState.gpuQueue, ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED,
            ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED });
    } else {
        const ResourceBarrier srcBarrier = GetSrcBufferBarrier(srcState, srcBuffer);
        const ResourceBarrier dstBarrier = GetDstBufferBarrier(newGpuResourceState, newBuffer);

        barriers.push_back(CommandBarrier {
            newBuffer.handle, srcBarrier, srcState.gpuQueue, dstBarrier, newGpuResourceState.gpuQueue });
    }

    stateRef.state = newGpuResourceState;
    stateRef.resource = newBuffer;
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
    const bool addMips = RenderHandleUtil::IsDynamicAdditionalStateResource(newImage.handle);

    if ((srcState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED) &&
        (srcState.gpuQueue.type != newGpuResourceState.gpuQueue.type)) {
        PLUGIN_ASSERT(newGpuResourceState.gpuQueue.type != GpuQueue::QueueType::UNDEFINED);
        PLUGIN_ASSERT(RenderHandleUtil::GetHandleType(newImage.handle) == RenderHandleType::GPU_IMAGE);
        PLUGIN_ASSERT(stateRef.prevRenderNodeIndex != renderNodeIndex);
        currNodeGpuResourceTransfer.push_back(
            RenderGraph::GpuQueueTransferState { newImage.handle, stateRef.prevRenderNodeIndex, renderNodeIndex,
                stateRef.state.gpuQueue, srcImage.imageLayout, newImage.imageLayout });
    } else {
        const ResourceBarrier srcBarrier =
            addMips ? GetSrcImageBarrierMips(srcState, srcImage, newImage, stateRef.additionalState)
                    : GetSrcImageBarrier(srcState, srcImage);
        const ResourceBarrier dstBarrier = addMips ? GetDstImageBarrierMips(newGpuResourceState, srcImage, newImage)
                                                   : GetDstImageBarrier(newGpuResourceState, newImage);

        barriers.push_back(CommandBarrier {
            newImage.handle, srcBarrier, srcState.gpuQueue, dstBarrier, newGpuResourceState.gpuQueue });
    }

    stateRef.state = newGpuResourceState;
    stateRef.resource = newImage;
    stateRef.prevRc = rcWithType;
    stateRef.prevRenderNodeIndex = renderNodeIndex;
    if (addMips) {
        ModifyAdditionalImageState(newImage, stateRef.additionalState);
    }
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
            }
            gpuBufferDataIndices_[arrayIndex] = dataIdx;

            gpuBufferTracking_[dataIdx].resource.handle = handle;
            gpuBufferTracking_[dataIdx].state.gpuQueue = queue; // current queue for default state
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
        // NOTE: render pass attachments expected to be dynamic resources always
        PLUGIN_ASSERT(RenderHandleUtil::IsDynamicResource(handle));
        uint32_t dataIdx = gpuImageDataIndices_[arrayIndex];
        if (dataIdx == INVALID_TRACK_IDX) {
            if (!gpuImageAvailableIndices_.empty()) {
                dataIdx = gpuImageAvailableIndices_.back();
                gpuImageAvailableIndices_.pop_back();
            } else {
                dataIdx = static_cast<uint32_t>(gpuImageTracking_.size());
                gpuImageTracking_.emplace_back();
            }
            gpuImageDataIndices_[arrayIndex] = dataIdx;

            gpuImageTracking_[dataIdx].resource.handle = handle;
            gpuImageTracking_[dataIdx].state.gpuQueue = queue; // current queue for default state
            if (RenderHandleUtil::IsDynamicAdditionalStateResource(handle) &&
                (!gpuImageTracking_[dataIdx].additionalState.layouts)) {
                gpuImageTracking_[dataIdx].additionalState.layouts = make_unique<ImageLayout[]>(MAX_MIP_STATE_COUNT);
            }
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if (RenderHandleUtil::IsDynamicAdditionalStateResource(handle) &&
            (!gpuImageTracking_[dataIdx].additionalState.layouts)) {
            PLUGIN_LOG_ONCE_W("dynamic_state_mips_issue_" + to_string(handle.id),
                "RENDER_VALIDATION: Additional mip states missing (handle:%" PRIx64 ")", handle.id);
        }
#endif
        return gpuImageTracking_[dataIdx];
    }

    PLUGIN_LOG_ONCE_W("render_graph_image_state_issues", "RenderGraph: Image tracking issue with handle count");
    return defaultImageState_;
}
RENDER_END_NAMESPACE()
