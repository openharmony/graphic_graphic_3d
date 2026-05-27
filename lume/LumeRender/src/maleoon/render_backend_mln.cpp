/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "render_backend_mln.h"

#include <cstring>
#include <functional>
#include <securec.h>

#include <base/containers/array_view.h>
#include <base/util/hash.h>
#include <core/log.h>
#include <render/maleoon/intf_device_mln.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_backend_node.h>
#include <render/nodecontext/intf_render_command_list.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_manager.h"
#include "device/render_frame_sync.h"
#include "maleoon/descriptor_set_manager_mln.h"
#include "maleoon/device_mln.h"
#include "maleoon/gpu_buffer_mln.h"
#include "maleoon/gpu_image_mln.h"
#include "maleoon/gpu_sampler_mln.h"
#include "maleoon/mln_convert.h"
#include "maleoon/node_context_descriptor_set_manager_mln.h"
#include "maleoon/pipeline_state_object_mln.h"
#include "maleoon/render_frame_sync_mln.h"
#include "maleoon/swapchain_mln.h"
#include "mln_log.h"
#include "nodecontext/node_context_descriptor_set_manager.h"
#include "nodecontext/node_context_pool_manager.h"
#include "nodecontext/node_context_pso_manager.h"
#include "nodecontext/render_barrier_list.h"
#include "nodecontext/render_command_list.h"
#include "nodecontext/render_node_graph_node_store.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

// Maleoon categorized log flags (read once in constructor, restart to change)
MlnLogFlags g_mlnLog{};

void MlnLogRefreshFlags()
{
    g_mlnLog.init = OHOS::system::GetParameter("persist.agp.mln.log.init", "0") == "1";
    g_mlnLog.frame = OHOS::system::GetParameter("persist.agp.mln.log.frame", "0") == "1";
    g_mlnLog.graph = OHOS::system::GetParameter("persist.agp.mln.log.graph", "0") == "1";
    g_mlnLog.trans = OHOS::system::GetParameter("persist.agp.mln.log.trans", "0") == "1";
    g_mlnLog.comp = OHOS::system::GetParameter("persist.agp.mln.log.comp", "0") == "1";
    // Feature flags: read once at init, app restart to take effect.
    // (Log flags above are also init-only; kept in same function for simplicity.)
    g_mlnLog.ogPendingStage = OHOS::system::GetParameter("persist.agp.mln.og.pending", "1") != "0";
    g_mlnLog.ogDirectBuild = OHOS::system::GetParameter("persist.agp.mln.og.direct_build", "1") != "0";
}

namespace {

// Frame counter for diagnostic log format strings
static uint32_t g_debugFrameCount = 0;

constexpr uint32_t MAX_TOTAL_DYNAMIC_OFFSETS =
    PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT * PipelineLayoutConstants::MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT;

MlnAttachmentLoadOp ToMlnLoadOp(AttachmentLoadOp op)
{
    switch (op) {
        case CORE_ATTACHMENT_LOAD_OP_LOAD:
            return MLN_ATTACHMENT_LOAD_OP_LOAD;
        case CORE_ATTACHMENT_LOAD_OP_CLEAR:
            return MLN_ATTACHMENT_LOAD_OP_CLEAR;
        case CORE_ATTACHMENT_LOAD_OP_DONT_CARE:
            return MLN_ATTACHMENT_LOAD_OP_DONT_CARE;
        default:
            return MLN_ATTACHMENT_LOAD_OP_DONT_CARE;
    }
}

MlnAttachmentStoreOp ToMlnStoreOp(AttachmentStoreOp op)
{
    switch (op) {
        case CORE_ATTACHMENT_STORE_OP_STORE:
            return MLN_ATTACHMENT_STORE_OP_STORE;
        case CORE_ATTACHMENT_STORE_OP_DONT_CARE:
            return MLN_ATTACHMENT_STORE_OP_DONT_CARE;
        default:
            return MLN_ATTACHMENT_STORE_OP_DONT_CARE;
    }
}

bool IsDepthStencilFormat(MlnFormat format)
{
    switch (static_cast<uint32_t>(format)) {
        case MLN_FORMAT_D16_UNORM:
        case MLN_FORMAT_D32_SFLOAT:
        case MLN_FORMAT_S8_UINT:
        case MLN_FORMAT_D16_UNORM_S8_UINT:
        case MLN_FORMAT_D24_UNORM_S8_UINT:
        case MLN_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default:
            return false;
    }
}

bool HasStencilComponent(MlnFormat format)
{
    switch (static_cast<uint32_t>(format)) {
        case MLN_FORMAT_S8_UINT:
        case MLN_FORMAT_D16_UNORM_S8_UINT:
        case MLN_FORMAT_D24_UNORM_S8_UINT:
        case MLN_FORMAT_D32_SFLOAT_S8_UINT:
            return true;
        default:
            return false;
    }
}

// Collected state while scanning a command list
struct MlnRenderState {
    const GraphicsPipelineStateObjectMln* graphicsPso{nullptr};
    const ComputePipelineStateObjectMln* computePso{nullptr};
    RenderHandle graphicsPsoHandle{};
    bool hasGraphicsPsoHandle{false};

    // Vertex buffers
    MlnResource vertexBuffers[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT]{};
    MlnDeviceSize vertexBufferOffsets[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT]{};
    MlnDeviceSize vertexBufferSizes[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT]{};
    uint32_t vertexBufferCount{0};

    // Index buffer
    MlnResource indexBuffer{MLN_NULL_HANDLE};
    MlnDeviceSize indexBufferOffset{0};
    MlnIndexType indexType{MLN_INDEX_TYPE_UINT32};

    // Descriptor set bindings (tracked but MlnBindingSet resolved separately)
    MlnBindingSet bindingSets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};
    uint32_t firstSet{0};
    uint32_t endSet{0}; // exclusive upper bound (firstSet + count)
    // Per-set dynamic offsets: accumulated across multiple BIND_DESCRIPTOR_SETS calls.
    // Flattened in set order at snapshot time for the ObjectGroup.
    struct PerSetDynOffsets {
        uint32_t count{0};
        uint32_t offsets[PipelineLayoutConstants::MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT]{};
    };
    PerSetDynOffsets perSetDynOffsets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};

    // Storage resource outputs: collected at BIND_DESCRIPTOR_SETS by scanning each
    // bound set for STORAGE_IMAGE / STORAGE_BUFFER / STORAGE_TEXEL_BUFFER bindings.
    // Used by BuildComputeDg to populate compute DG outputs so the SchedulingGraph
    // can declare proper transfer/compute → consumer dependencies (memory visibility
    // barriers) on resources written by compute shaders.

    // Per-set storage so a partial rebind of a single set replaces only that set's
    // entries, leaving other already-bound sets intact.
    struct PerSetStorageOutputs {
        BASE_NS::vector<MlnPassNodeResourceDescriptor> entries;
    };
    PerSetStorageOutputs perSetStorage[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};

    // Push constant data
    uint8_t pushConstantData[PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE]{};
    uint32_t pushConstantOffset{0};
    uint32_t pushConstantSize{0};

    // Dynamic state
    bool hasViewport{false};
    MlnViewport viewport{};
    bool hasScissor{false};
    MlnRect2D scissor{};
    bool hasLineWidth{false};
    float lineWidth{1.0f};
    bool hasDepthBias{false};
    float depthBiasConstantFactor{0.0f};
    float depthBiasClamp{0.0f};
    float depthBiasSlopeFactor{0.0f};
    bool hasBlendConstants{false};
    float blendConstants[4u]{0.0f, 0.0f, 0.0f, 0.0f};
    bool hasDepthBounds{false};
    float minDepthBounds{0.0f};
    float maxDepthBounds{1.0f};
    bool hasStencilState{false};
    uint32_t stencilFrontCompareMask{0};
    uint32_t stencilFrontWriteMask{0};
    uint32_t stencilFrontReference{0};
    uint32_t stencilBackCompareMask{0};
    uint32_t stencilBackWriteMask{0};
    uint32_t stencilBackReference{0};
};

// A single draw call group (one ObjectGroup)
struct DrawCallGroup {
    MlnProgram program{MLN_NULL_HANDLE};
    // [SDK] programInterface removed from Mln*ObjectGroupDescriptor; no longer needed
    // in per-draw snapshot. Program creation (PSO side) still uses it, but OG path does not.
    PipelineStateObjectPlatformDataMln psoPlat{}; // Cached PSO state for stateSet

    MlnResource vertexBuffers[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT]{};
    MlnDeviceSize vertexBufferOffsets[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT]{};
    MlnDeviceSize vertexBufferSizes[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT]{};
    uint32_t vertexBufferCount{0};

    MlnResource indexBuffer{MLN_NULL_HANDLE};
    MlnDeviceSize indexBufferOffset{0};
    MlnIndexType indexType{MLN_INDEX_TYPE_UINT32};

    MlnBindingSet bindingSets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};
    uint32_t firstSet{0};
    uint32_t bindingSetCount{0};
    uint32_t bindingSetDynamicOffsetCount{0};
    uint32_t bindingSetDynamicOffsets[MAX_TOTAL_DYNAMIC_OFFSETS]{};

    uint8_t pushConstantData[PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE]{};
    uint32_t pushConstantOffset{0};
    uint32_t pushConstantSize{0};

    bool hasViewport{false};
    MlnViewport viewport{};
    bool hasScissor{false};
    MlnRect2D scissor{};

    // Additional dynamic states
    bool hasLineWidth{false};
    float lineWidth{1.0f};
    bool hasDepthBias{false};
    float depthBiasConstantFactor{0.0f};
    float depthBiasClamp{0.0f};
    float depthBiasSlopeFactor{0.0f};
    bool hasBlendConstants{false};
    float blendConstants[4u]{0.0f, 0.0f, 0.0f, 0.0f};
    bool hasDepthBounds{false};
    float minDepthBounds{0.0f};
    float maxDepthBounds{1.0f};
    bool hasStencilState{false};
    uint32_t stencilFrontCompareMask{0};
    uint32_t stencilFrontWriteMask{0};
    uint32_t stencilFrontReference{0};
    uint32_t stencilBackCompareMask{0};
    uint32_t stencilBackWriteMask{0};
    uint32_t stencilBackReference{0};

    // Draw parameters
    uint32_t subpassIndex{0};
    bool isIndexed{false};
    bool isIndirect{false};
    MlnDrawCommand drawCmd{};
    MlnDrawIndexedCommand drawIndexedCmd{};
    MlnDrawIndirectCommand drawIndirectCmd{};
};

// Compute dispatch group
struct ComputeDispatchGroup {
    MlnProgram program{MLN_NULL_HANDLE};
    // [SDK] programInterface removed from Mln*ObjectGroupDescriptor; no longer needed.

    MlnBindingSet bindingSets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};
    uint32_t firstSet{0};
    uint32_t bindingSetCount{0};
    uint32_t bindingSetDynamicOffsetCount{0};
    uint32_t bindingSetDynamicOffsets[MAX_TOTAL_DYNAMIC_OFFSETS]{};

    uint8_t pushConstantData[PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE]{};
    uint32_t pushConstantOffset{0};
    uint32_t pushConstantSize{0};

    uint32_t groupCountX{0};
    uint32_t groupCountY{0};
    uint32_t groupCountZ{0};

    bool isIndirect{false};
    MlnResource indirectBuffer{MLN_NULL_HANDLE};
    MlnDeviceSize indirectOffset{0};

    // Storage resource outputs (storage images/buffers bound to this dispatch).
    // Flattened from MlnRenderState::perSetStorage[] at snapshot time. Used by
    // BuildComputeDg to populate compute DG outputs for SG dependency tracking.
    BASE_NS::vector<MlnPassNodeResourceDescriptor> storageOutputs;
};

// Transfer operation
struct TransferOp {
    MlnTransferType type{};
    // Only one of these is valid per TransferOp
    MlnCopyBufferDescriptor copyBuffer{};
    MlnBufferCopyRegion copyBufferRegion{};
    MlnCopyBufferToImageDescriptor copyBufferToImage{};
    MlnCopyImageToBufferDescriptor copyImageToBuffer{};
    MlnBufferImageCopyRegion bufferImageRegion{};
    MlnCopyImageDescriptor copyImage{};
    MlnImageCopyRegion imageCopyRegion{};
    MlnBlitImageDescriptor blitImage{};
    MlnImageBlitRegion blitRegion{};
    MlnClearImageDescriptor clearImage{};
    MlnClearImageRegion clearImageRegion{};
};

struct RenderPassSegment {
    const RenderCommandBeginRenderPass* beginCmd{nullptr};
    uint32_t subpassStartIndex{0};
    LowLevelRenderPassDataMln renderPassData{};
    // drawGroups: used when ogDirectBuild == false (fallback path).
    // secondaryOGHandles: used when ogDirectBuild == true (optimized path).
    // Both paths are mutually exclusive per-rpSeg but struct holds both fields to
    // allow runtime toggle. Under normal operation, only one is populated per rpSeg.
    vector<DrawCallGroup> drawGroups;
    vector<MlnObjectGroup> secondaryOGHandles;
};

// Walker segment state enum — tracks which type of command segment is currently open.
enum class OpenSegment : uint8_t {
    NONE,
    GRAPHICS,
    TRANSFER,
};

// Primary ctx in-flight render pass container.
// Replaces RenderPassSegment.drawGroups (vector<DrawCallGroup>, ~1200B each)
// with direct OG handles (vector<MlnObjectGroup>, 8B each). OG is built
// immediately at DRAW command via BuildOGFromState (Step 5b).

// NOTE: Secondary ctx still uses RenderPassSegment with DrawCallGroup
// (§4.2.1) — primary ctx lazy-merges secondary drawGroups into OGs at
// BEGIN_RENDER_PASS via MergeStitchedRpSegs (Step 6).
struct PendingRenderPass {
    const RenderCommandBeginRenderPass* beginCmd{nullptr};
    uint32_t subpassStartIndex{0};
    LowLevelRenderPassDataMln renderPassData{};
    vector<MlnObjectGroup> objectGroups;
};

// Shared context for streaming walker.
// Threaded through Walk*/Handle*/Build* helpers instead of passing
// dozens of individual parameters. Fields added incrementally in Steps 6/7a.
struct StreamingCtx {
    // Placeholder — fields added in Step 6 (Handle*) and Step 7a (WalkPrimaryCtx).
    // Keeping empty now so Step 1 compile check stays isolated.
    uint8_t placeholder{0};
};

// Moved from local scope so
// BuildTransferDg can reference it via void* parameter. No semantic change.
struct TransferDstImage {
    MlnResource resource{MLN_NULL_HANDLE};
    MlnResourceView resourceView{MLN_NULL_HANDLE};
};

// PrimaryWalkerState — bundles walker-local state for Handle* functions.
struct PrimaryWalkerState {
    MlnRenderState& state;
    RenderPassSegment*& currentRP;
    uint32_t& currentSubpassIndex;
    OpenSegment& openSeg;
    RenderPassSegment& curRP;
    vector<MlnObjectGroup>& curStreamOGs;
    uint32_t& curStreamSubpass;
    vector<TransferOp>& curTransferOps;
    vector<TransferDstImage>& curTransferDsts;
    vector<MlnResource>& curTransferDstBuffers; // dst buffers for current transfer batch
    uint32_t& streamingRpSegIdx;
    void* pendingFrame{nullptr};     // PendingDestroyFrame*
    void* outDataGraphs{nullptr};    // vector<MlnDataGraph>*
    void* outDgResources{nullptr};   // vector<DataGraphResourceInfo>*
    void* additionalRpSegs{nullptr}; // vector<RenderPassSegment>*
    void* psoMgr{nullptr};           // NodeContextPsoManager*
    // Default-init to nullptr so a future init-list bug (e.g. missing element) still
    // leaves backendNode safe to test with `if (w.backendNode)` rather than a stack
    // garbage pointer that crashes inside HandleExecuteBackendFrame.
    void* backendNode{nullptr}; // IRenderBackendNode* (for EXECUTE_BACKEND_FRAME_POSITION legacy path)
};

// ResolveGraphicsPsoForCurrentRp — deferred PSO lookup at RP time.
// Shared by HandleStateCommand (BIND_PIPELINE) and both walkers (DRAW case).
inline bool ResolveGraphicsPsoForCurrentRp(const RenderHandle psoHandle, RenderPassSegment* rp,
    uint32_t requestedSubpassIndex, MlnRenderState& rs, NodeContextPsoManager& psoMgr)
{
    if (!rp || !rp->beginCmd) {
        return false;
    }
    uint32_t subpassIndex = requestedSubpassIndex;
    if (rp->beginCmd->subpasses.empty()) {
        subpassIndex = 0;
    } else if (subpassIndex >= rp->beginCmd->subpasses.size()) {
        subpassIndex = static_cast<uint32_t>(rp->beginCmd->subpasses.size() - 1);
    }
    const auto& rpDesc = rp->beginCmd->renderPassDesc;
    const auto* pso = psoMgr.GetGraphicsPso(
        psoHandle, rpDesc, rp->beginCmd->subpasses, subpassIndex, 0, &rp->renderPassData, nullptr);
    if (pso) {
        rs.graphicsPso = static_cast<const GraphicsPipelineStateObjectMln*>(pso);
        return true;
    }
    MLN_LOG_ERR("deferred graphics PSO lookup FAILED (subpass=%u, psoHandle idx=%u, gen=%u)", subpassIndex,
        RenderHandleUtil::GetIndexPart(psoHandle), RenderHandleUtil::GetGenerationIndexPart(psoHandle));
    return false;
}

// MergeStitchedRpSegs — merges stitched render pass segments from secondary ctx.
// Phase 1.5 merge block (originally cpp:4830-4854).
// Appends additionalRpSegs (from secondary ctxs) into renderPassSegments, then merges
// consecutive rpSegs sharing the same subpassStartIndex (stitched subpasses produced when
// render graph subpass merge yields subpassCount=1 but the cmdList walker still produces
// multiple rpSegs).

// Free function in anonymous namespace because it only operates on local vars
// (no class member access needed). Both args are anon-ns types so can't go
// through the .h interface anyway.
inline void MergeStitchedRpSegs(
    vector<RenderPassSegment>& renderPassSegments, vector<RenderPassSegment>* additionalRpSegs)
{
    // Append rpSegs from other ctx's in the same multi-render-command-list group.
    if (additionalRpSegs && !additionalRpSegs->empty()) {
        for (auto& seg : *additionalRpSegs) {
            renderPassSegments.push_back(BASE_NS::move(seg));
        }
        additionalRpSegs->clear();
        MLN_LOG_GRAPH("Phase1.5: appended additional rpSegs, total=%zu", renderPassSegments.size());
    }
    for (size_t i = 1; i < renderPassSegments.size();) {
        auto& prev = renderPassSegments[i - 1];
        auto& curr = renderPassSegments[i];
        if (curr.beginCmd && curr.beginCmd->beginType == RenderPassBeginType::RENDER_PASS_SUBPASS_BEGIN &&
            curr.subpassStartIndex == prev.subpassStartIndex) {
            // Merge both fallback drawGroups and optimized secondaryOGHandles.
            for (auto& dg : curr.drawGroups) {
                prev.drawGroups.push_back(BASE_NS::move(dg));
            }
            for (auto& og : curr.secondaryOGHandles) {
                prev.secondaryOGHandles.push_back(og);
            }
            if (g_mlnLog.graph) {
                MLN_LOG_GRAPH(
                    "Phase1.5: MERGED stitched rpSeg[%zu] (draws=%zu OGs=%zu) into rpSeg[%zu] (now draws=%zu OGs=%zu)",
                    i, curr.drawGroups.size(), curr.secondaryOGHandles.size(), i - 1, prev.drawGroups.size(),
                    prev.secondaryOGHandles.size());
            }
            renderPassSegments.erase(renderPassSegments.begin() + static_cast<ptrdiff_t>(i));
        } else {
            ++i;
        }
    }
}

// Inputs for BuildOGFromState packed into one struct.
// Defined in anonymous namespace because it references DrawCallGroup (anon-ns
// type). Caller fills this struct on the stack before each call. The function
// is passed `const BuildOGInputs*` opaquely as `const void*` to keep the .h
// clean of anon-ns types. Same opaque pattern as RenderSingleCommandList /
// BuildTransferDg / BuildComputeDg.
struct BuildOGInputs {
    const DrawCallGroup* dg{nullptr};
    const MlnResourceSetDescriptor* resourceSetPtr{nullptr};
    const MlnStateSetDescriptor* stateSet{nullptr};
    const MlnGraphicsCommandDescriptor* graphicsCmd{nullptr};
    // For Mode 2 in-place update path (UpdateCachedOG):
    const MlnResourceVertexBuffers* resVB{nullptr};
    const MlnResourceIndexBuffer* resIB{nullptr};
    const MlnResourceBindingSets* resBS{nullptr};
    const MlnResourceProgramConstants* resPC{nullptr};
    const MlnViewportDynamicState* vpState{nullptr};
    const MlnScissorDynamicState* scState{nullptr};
    const MlnDepthBiasDynamicState* depthBiasState{nullptr};
    const MlnDepthBoundsDynamicState* depthBoundsState{nullptr};
    const MlnStencilCompareMaskDynamicState* stencilCompareMask{nullptr};
    const MlnStencilWriteMaskDynamicState* stencilWriteMask{nullptr};
    const MlnStencilReferenceDynamicState* stencilReference{nullptr};
};

void SnapshotStateToDrawGroup(DrawCallGroup& group, const MlnRenderState& state)
{
    group.vertexBufferCount = state.vertexBufferCount;
    for (uint32_t i = 0; i < state.vertexBufferCount; ++i) {
        group.vertexBuffers[i] = state.vertexBuffers[i];
        group.vertexBufferOffsets[i] = state.vertexBufferOffsets[i];
        group.vertexBufferSizes[i] = state.vertexBufferSizes[i];
    }
    group.indexBuffer = state.indexBuffer;
    group.indexBufferOffset = state.indexBufferOffset;
    group.indexType = state.indexType;

    group.firstSet = state.firstSet;
    group.bindingSetCount = (state.endSet > state.firstSet) ? (state.endSet - state.firstSet) : 0;
    for (uint32_t i = 0; i < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++i) {
        group.bindingSets[i] = state.bindingSets[i];
    }
    // Flatten per-set dynamic offsets in set order for the ObjectGroup
    group.bindingSetDynamicOffsetCount = 0;
    for (uint32_t s = state.firstSet; s < state.endSet; ++s) {
        const auto& setDyn = state.perSetDynOffsets[s];
        if (g_mlnLog.graph && setDyn.count > 0) {
            MLN_LOG_GRAPH("Snapshot set=%u count=%u offsets=[%u,%u,%u,%u]", s, setDyn.count,
                (setDyn.count > 0) ? setDyn.offsets[0] : 0, (setDyn.count > 1) ? setDyn.offsets[1] : 0,
                (setDyn.count > 2) ? setDyn.offsets[2] : 0, (setDyn.count > 3) ? setDyn.offsets[3] : 0);
        }
        for (uint32_t d = 0; d < setDyn.count; ++d) {
            if (group.bindingSetDynamicOffsetCount < MAX_TOTAL_DYNAMIC_OFFSETS) {
                group.bindingSetDynamicOffsets[group.bindingSetDynamicOffsetCount++] = setDyn.offsets[d];
            }
        }
    }

    group.pushConstantOffset = state.pushConstantOffset;
    group.pushConstantSize = state.pushConstantSize;
    if (state.pushConstantSize > 0) {
        if (memcpy_s(group.pushConstantData, sizeof(group.pushConstantData), state.pushConstantData,
                state.pushConstantSize) != 0) {
            group.pushConstantSize = 0;
        }
    }

    group.hasViewport = state.hasViewport;
    group.viewport = state.viewport;
    group.hasScissor = state.hasScissor;
    group.scissor = state.scissor;

    group.hasLineWidth = state.hasLineWidth;
    group.lineWidth = state.lineWidth;
    group.hasDepthBias = state.hasDepthBias;
    group.depthBiasConstantFactor = state.depthBiasConstantFactor;
    group.depthBiasClamp = state.depthBiasClamp;
    group.depthBiasSlopeFactor = state.depthBiasSlopeFactor;
    group.hasBlendConstants = state.hasBlendConstants;
    for (uint32_t i = 0; i < 4u; ++i) {
        group.blendConstants[i] = state.blendConstants[i];
    }
    group.hasDepthBounds = state.hasDepthBounds;
    group.minDepthBounds = state.minDepthBounds;
    group.maxDepthBounds = state.maxDepthBounds;
    group.hasStencilState = state.hasStencilState;
    group.stencilFrontCompareMask = state.stencilFrontCompareMask;
    group.stencilFrontWriteMask = state.stencilFrontWriteMask;
    group.stencilFrontReference = state.stencilFrontReference;
    group.stencilBackCompareMask = state.stencilBackCompareMask;
    group.stencilBackWriteMask = state.stencilBackWriteMask;
    group.stencilBackReference = state.stencilBackReference;
}

void SnapshotStateToComputeGroup(ComputeDispatchGroup& group, const MlnRenderState& state)
{
    group.firstSet = state.firstSet;
    group.bindingSetCount = (state.endSet > state.firstSet) ? (state.endSet - state.firstSet) : 0;
    for (uint32_t i = 0; i < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++i) {
        group.bindingSets[i] = state.bindingSets[i];
    }
    // Flatten per-set dynamic offsets in set order for the ObjectGroup
    group.bindingSetDynamicOffsetCount = 0;
    for (uint32_t s = state.firstSet; s < state.endSet; ++s) {
        const auto& setDyn = state.perSetDynOffsets[s];
        for (uint32_t d = 0; d < setDyn.count; ++d) {
            if (group.bindingSetDynamicOffsetCount < MAX_TOTAL_DYNAMIC_OFFSETS) {
                group.bindingSetDynamicOffsets[group.bindingSetDynamicOffsetCount++] = setDyn.offsets[d];
            }
        }
    }

    group.pushConstantOffset = state.pushConstantOffset;
    group.pushConstantSize = state.pushConstantSize;
    if (state.pushConstantSize > 0) {
        if (memcpy_s(group.pushConstantData, sizeof(group.pushConstantData), state.pushConstantData,
                state.pushConstantSize) != 0) {
            group.pushConstantSize = 0;
        }
    }

    // Flatten storage resources from currently-bound sets. Used as compute DG outputs
    // so SG can declare proper memory dependencies for downstream consumers.
    group.storageOutputs.clear();
    for (uint32_t s = state.firstSet; s < state.endSet; ++s) {
        const auto& storage = state.perSetStorage[s];
        for (const auto& e : storage.entries) {
            group.storageOutputs.push_back(e);
        }
    }
}

// Hash a MlnRenderTarget configuration for cache lookup (mirrors Vulkan HashFramebuffer).
// Includes attachment views, load/store ops, clear values, and render area.
uint64_t HashRenderTargetConfig(const BASE_NS::vector<MlnAttachmentDescriptor>& colors, bool hasDepth,
    const MlnAttachmentDescriptor& depthAtt, bool hasStencil, const MlnAttachmentDescriptor& stencilAtt, uint32_t areaW,
    uint32_t areaH)
{
    uint64_t h = 0;
    BASE_NS::HashCombine(h, static_cast<uint64_t>(areaW), static_cast<uint64_t>(areaH));
    BASE_NS::HashCombine(h, static_cast<uint64_t>(colors.size()));
    for (const auto& c : colors) {
        BASE_NS::HashCombine(h, reinterpret_cast<uint64_t>(c.imageResourceView),
            reinterpret_cast<uint64_t>(c.resolveResourceView), static_cast<uint64_t>(c.loadOp),
            static_cast<uint64_t>(c.storeOp));
        // OPT #9: include clearValue in hash to avoid stale clear color on cache hit
        if (c.loadOp == MLN_ATTACHMENT_LOAD_OP_CLEAR) {
            BASE_NS::HashCombine(h, static_cast<uint64_t>(c.clearValue.color.u[0]),
                static_cast<uint64_t>(c.clearValue.color.u[1]), static_cast<uint64_t>(c.clearValue.color.u[2]),
                static_cast<uint64_t>(c.clearValue.color.u[3]));
        }
    }
    if (hasDepth) {
        BASE_NS::HashCombine(h, reinterpret_cast<uint64_t>(depthAtt.imageResourceView),
            reinterpret_cast<uint64_t>(depthAtt.resolveResourceView), static_cast<uint64_t>(depthAtt.loadOp),
            static_cast<uint64_t>(depthAtt.storeOp));
        if (depthAtt.loadOp == MLN_ATTACHMENT_LOAD_OP_CLEAR) {
            uint64_t cv = 0;
            if (memcpy_s(&cv, sizeof(cv), &depthAtt.clearValue.depthStencil, sizeof(cv)) != EOK) {
                cv = 0;
            }
            BASE_NS::HashCombine(h, cv);
        }
    }
    if (hasStencil) {
        BASE_NS::HashCombine(h, reinterpret_cast<uint64_t>(stencilAtt.imageResourceView),
            static_cast<uint64_t>(stencilAtt.loadOp), static_cast<uint64_t>(stencilAtt.storeOp));
        if (stencilAtt.loadOp == MLN_ATTACHMENT_LOAD_OP_CLEAR) {
            uint64_t cv = 0;
            if (memcpy_s(&cv, sizeof(cv), &stencilAtt.clearValue.depthStencil, sizeof(cv)) != EOK) {
                cv = 0;
            }
            BASE_NS::HashCombine(h, cv);
        }
    }
    return h;
}

// ---- OG Update binary-search debug: bitmask controls which properties go through Update ----
// bit=1 → dynamic (updated via MlnUpdateGraphicsObjectGroup on cache hit)
// Hash the static (frame-invariant) part of a PSO for OG cache key.
uint64_t HashPsoStaticState(const PipelineStateObjectPlatformDataMln& pso)
{
    uint64_t h = 0;
    BASE_NS::HashCombine(h, static_cast<uint64_t>(pso.topology), static_cast<uint64_t>(pso.cullMode),
        static_cast<uint64_t>(pso.frontFace), static_cast<uint64_t>(pso.depthTestEnable),
        static_cast<uint64_t>(pso.depthWriteEnable), static_cast<uint64_t>(pso.depthCompareOp),
        static_cast<uint64_t>(pso.stencilTestEnable), static_cast<uint64_t>(pso.rasterizerDiscardEnable));
    return h;
}

// Hash the OG identity for cache lookup.
// Includes: program, PSO static state, draw type, VB count.
// WORKAROUND: Vertex buffer handles+offsets are also hashed (treated as static identity) because
// MlnUpdateGraphicsObjectGroup does not correctly apply VERTEX_BUFFERS modifier updates.
// This was confirmed via binary-search debugging (OG_UPDATE_BITS): only the VB modifier causes
// black-screen when updated dynamically; all other 11 modifier types work correctly.
// Impact: VB changes cause hash miss → fresh Create, but VB is typically frame-invariant for
// static meshes, so the extra miss rate is negligible in practice.
uint64_t HashOGIdentity(const DrawCallGroup& dg)
{
    uint64_t h = 0;
    // [SDK] programInterface removed from OG descriptor — hash only by program.
    BASE_NS::HashCombine(h, reinterpret_cast<uint64_t>(dg.program));
    BASE_NS::HashCombine(h, HashPsoStaticState(dg.psoPlat));
    BASE_NS::HashCombine(h, static_cast<uint64_t>(dg.isIndexed), static_cast<uint64_t>(dg.isIndirect),
        static_cast<uint64_t>(dg.vertexBufferCount));

    // VB hashed as static identity (driver workaround — Update VB modifier is broken)
    for (uint32_t i = 0; i < dg.vertexBufferCount; ++i) {
        BASE_NS::HashCombine(
            h, reinterpret_cast<uint64_t>(dg.vertexBuffers[i]), static_cast<uint64_t>(dg.vertexBufferOffsets[i]));
    }

    return h;
}

// Build MlnGraphicsObjectGroupUpdateDescriptor from a DrawCallGroup to update a cached OG.
// Updates all dynamic state EXCEPT vertex buffers.
// WORKAROUND: VERTEX_BUFFERS modifier is excluded because MlnUpdateGraphicsObjectGroup does not
// correctly apply VB updates (confirmed via binary-search: only VB modifier causes black-screen).
// VB identity is hashed in HashOGIdentity instead, so VB changes trigger cache miss → fresh Create.
// Returns true on success.
bool UpdateCachedOG(MlnDevice mlnDevice, MlnObjectGroup cachedOG, const DrawCallGroup& dg,
    const MlnResourceVertexBuffers& resVB, const MlnResourceIndexBuffer& resIB, const MlnResourceBindingSets& resBS,
    const MlnResourceProgramConstants& resPC, const MlnViewportDynamicState& vpState,
    const MlnScissorDynamicState& scState, const MlnDepthBiasDynamicState& depthBiasState,
    const MlnDepthBoundsDynamicState& depthBoundsState, const MlnStencilCompareMaskDynamicState& stencilCompareMask,
    const MlnStencilWriteMaskDynamicState& stencilWriteMask, const MlnStencilReferenceDynamicState& stencilReference)
{
    // Max modifiers: bindings(1) + dynOffsets(1) + pushConst(1) + IB(1) + command(1)
    //   + viewport(1) + scissor(1) + depthBias(1) + blendConst(1) + depthBounds(1)
    //   + stencilCmp(1) + stencilWrite(1) + stencilRef(1) = 13
    // NOTE: VB modifier intentionally excluded (driver bug workaround)
    constexpr uint32_t MAX_MODIFIER_CONTENTS = 13;
    MlnGraphicsObjectGroupModifierContent contents[MAX_MODIFIER_CONTENTS]{};
    uint32_t contentCount = 0;

    // Binding sets
    if (resBS.bindingSetCount > 0) {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_BINDING_SETS;
        c.data.bindingSet = &resBS;
    }

    // Dynamic offsets
    MlnBindingSetsDynamicOffsets dynOffsetsDesc{};
    if (dg.bindingSetDynamicOffsetCount > 0) {
        dynOffsetsDesc.firstSet = dg.firstSet;
        dynOffsetsDesc.dynamicOffsetCount = dg.bindingSetDynamicOffsetCount;
        dynOffsetsDesc.dynamicOffsets = dg.bindingSetDynamicOffsets;
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_BINDING_SETS_DYNAMIC_OFFSETS;
        c.data.dynamicOffsets = &dynOffsetsDesc;
    }

    // Push constants
    if (dg.pushConstantSize > 0) {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_PROGRAM_CONSTANTS;
        c.data.programConstant = &resPC;
    }

    // NOTE: VERTEX_BUFFERS modifier intentionally skipped here.
    // MlnUpdateGraphicsObjectGroup does not correctly apply VB modifier updates, causing
    // black-screen rendering. VB is instead part of hash identity (see HashOGIdentity),
    // so any VB change triggers cache miss → fresh MlnCreateGraphicsObjectGroup.

    // Index buffer
    if (dg.indexBuffer) {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_INDEX_BUFFER;
        c.data.indexBuffer = &resIB;
    }

    // Draw command
    {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_COMMAND;
        if (dg.isIndirect) {
            c.data.drawIndirect = &dg.drawIndirectCmd;
        } else if (dg.isIndexed) {
            c.data.drawIndexed = &dg.drawIndexedCmd;
        } else {
            c.data.draw = &dg.drawCmd;
        }
    }

    // Viewport
    {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_VIEWPORT;
        c.data.viewport = &vpState;
    }

    // Scissor
    {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_SCISSOR;
        c.data.scissor = &scState;
    }

    // Depth bias
    if (dg.hasDepthBias) {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_DEPTH_BIAS;
        c.data.depthBias = &depthBiasState;
    }

    // Blend constants
    if (dg.hasBlendConstants) {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_BLEND_CONSTANTS;
        c.data.blendConstants[0] = dg.blendConstants[0];
        c.data.blendConstants[1] = dg.blendConstants[1];
        c.data.blendConstants[2] = dg.blendConstants[2];
        c.data.blendConstants[3] = dg.blendConstants[3];
    }

    // Depth bounds
    if (dg.hasDepthBounds) {
        auto& c = contents[contentCount++];
        c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_DEPTH_BOUNDS;
        c.data.depthBound = &depthBoundsState;
    }

    // Stencil
    if (dg.hasStencilState) {
        {
            auto& c = contents[contentCount++];
            c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_STENCIL_COMPARE_MASK;
            c.data.stencilCompareMask = &stencilCompareMask;
        }
        {
            auto& c = contents[contentCount++];
            c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_STENCIL_WRITE_MASK;
            c.data.stencilWriteMask = &stencilWriteMask;
        }
        {
            auto& c = contents[contentCount++];
            c.type = MLN_GRAPHICS_OBJECT_GROUP_MODIFIER_TYPE_STENCIL_REFERENCE;
            c.data.stencilReference = &stencilReference;
        }
    }

    if (contentCount == 0) {
        return true; // nothing to update
    }

    // Each content needs its own MlnGraphicsObjectGroupModifier entry.
    // Driver expects modifierCount == number of individual modifiers in the array.
    MlnGraphicsObjectGroupModifier modifiers[MAX_MODIFIER_CONTENTS]{};
    for (uint32_t i = 0; i < contentCount; ++i) {
        modifiers[i].extensionCount = 0;
        modifiers[i].extensions = nullptr;
        modifiers[i].commandIndex = 0; // single-command OG
        modifiers[i].contentCount = 1;
        modifiers[i].contents = &contents[i];
    }

    MlnGraphicsObjectGroupUpdateDescriptor updateDesc{};
    updateDesc.extensionCount = 0;
    updateDesc.extensions = nullptr;
    updateDesc.flags = static_cast<MlnGraphicsObjectGroupUpdateDescriptorFlags>(0);
    updateDesc.modifierCount = contentCount;
    updateDesc.modifiers = modifiers;

    MlnStatus st = MlnUpdateGraphicsObjectGroup(mlnDevice, cachedOG, &updateDesc);
    return (st == MLN_STATUS_SUCCESS);
}

} // anonymous namespace

// Helper class for running std::function as a ThreadPool task (same pattern as Vulkan backend).
class MlnFunctionTask final : public CORE_NS::IThreadPool::ITask {
public:
    static Ptr Create(std::function<void()> func)
    {
        return Ptr{new MlnFunctionTask(BASE_NS::move(func))};
    }

    explicit MlnFunctionTask(std::function<void()> func) : func_(BASE_NS::move(func)) {}

    void operator()() override
    {
        func_();
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    std::function<void()> func_;
};

// ============================================================================
// RenderBackendMln
// ============================================================================

RenderBackendMln::RenderBackendMln(Device& device, GpuResourceManager& gpuResourceMgr, CORE_NS::ITaskQueue* queue)
    : device_(device), deviceMln_(static_cast<DeviceMln&>(device)), gpuResourceMgr_(gpuResourceMgr), queue_(queue)
{
    MlnLogRefreshFlags();
    MLN_LOG_INIT("RenderBackendMln: created");

    MlnTimelineDescriptor tlDesc{};
    tlDesc.extensionCount = 0;
    tlDesc.extensions = nullptr;
    tlDesc.flags = 0;
    tlDesc.type = MLN_TIMELINE_TYPE_COUNTER;
    tlDesc.initialValue = 0;
    submitTimeline_ = MlnCreateTimeline(deviceMln_.GetMlnDevice(), &tlDesc);
    if (!submitTimeline_) {
        MLN_LOG_ERR("RenderBackendMln: MlnCreateTimeline(submit) FAILED");
    }

    // Acquire timeline for swapchain synchronization.
    // Per driver requirement: swapchain-related timelines MUST use
    // MLN_TIMELINE_DESCRIPTOR_EXTERNAL_SYNC_FD_BIT flag. Driver signals this
    // timeline directly when the image is released by compositor.
    // timelineValue is ignored; signal goes directly to the timeline object.
    MlnTimelineDescriptor acqTlDesc{};
    acqTlDesc.extensionCount = 0;
    acqTlDesc.extensions = nullptr;
    acqTlDesc.flags = MLN_TIMELINE_DESCRIPTOR_EXTERNAL_SYNC_FD_BIT;
    acqTlDesc.type = MLN_TIMELINE_TYPE_BINARY;
    acqTlDesc.initialValue = 0;
    acquireTimeline_ = MlnCreateTimeline(deviceMln_.GetMlnDevice(), &acqTlDesc);
    if (!acquireTimeline_) {
        MLN_LOG_ERR("RenderBackendMln: MlnCreateTimeline(acquire) FAILED");
    }
}

RenderBackendMln::~RenderBackendMln()
{
    if (submitTimeline_) {
        // Wait for all pending GPU work
        if (submitTimelineValue_ > 0) {
            MlnTimelineWaitDescriptor waitDesc{};
            waitDesc.extensionCount = 0;
            waitDesc.extensions = nullptr;
            waitDesc.flags = 0;
            waitDesc.timelineCount = 1;
            waitDesc.timelines = &submitTimeline_;
            waitDesc.values = &submitTimelineValue_;
            MlnWaitForTimelines(deviceMln_.GetMlnDevice(), &waitDesc, UINT64_MAX);
        }
        // Destroy all pending frames
        MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
        for (auto& frame : pendingDestroyFrames_) {
            for (auto& sg : frame.schedulingGraphs) {
                MlnDestroySchedulingGraph(mlnDevice, sg);
            }
            for (auto& dg : frame.dataGraphs) {
                MlnDestroyDataGraph(mlnDevice, dg);
            }
            for (auto& og : frame.objectGroups) {
                MlnDestroyObjectGroup(mlnDevice, og);
            }
            for (auto& rt : frame.renderTargets) {
                MlnDestroyRenderTarget(mlnDevice, rt);
            }
        }
        pendingDestroyFrames_.clear();

        MlnDestroyTimeline(deviceMln_.GetMlnDevice(), submitTimeline_);
    }
    if (acquireTimeline_) {
        MlnDestroyTimeline(deviceMln_.GetMlnDevice(), acquireTimeline_);
    }
    // Destroy all cached render targets
    for (auto& entry : renderTargetCache_) {
        if (entry.second.renderTarget) {
            MlnDestroyRenderTarget(deviceMln_.GetMlnDevice(), entry.second.renderTarget);
        }
    }
    renderTargetCache_.clear();

    // Destroy all cached ObjectGroups (pool-based)
    for (auto& kv : ogPools_) {
        for (auto& entry : kv.second.entries) {
            if (entry.objectGroup) {
                MlnDestroyObjectGroup(deviceMln_.GetMlnDevice(), entry.objectGroup);
            }
        }
    }
    ogPools_.clear();
    ogTotalCount_ = 0;

    // Destroy empty binding set used for DG-Error fix
    if (emptyBindingSet_) {
        MlnDestroyBindingSet(deviceMln_.GetMlnDevice(), emptyBindingSet_);
    }
    if (emptyBindingLayout_) {
        MlnDestroyBindingLayout(deviceMln_.GetMlnDevice(), emptyBindingLayout_);
    }

    MLN_LOG_INIT("RenderBackendMln: destroyed");
}

void RenderBackendMln::EvictStaleRenderTargets()
{
    // Evict RTs not used for bufferingCount+2 frames (same policy as Vulkan)
    constexpr uint64_t ADDITIONAL_FRAMES = 2u;
    const uint64_t frameCount = device_.GetFrameCount();
    const auto minAge = device_.GetCommandBufferingCount() + ADDITIONAL_FRAMES;
    const auto ageLimit = (frameCount < minAge) ? 0u : (frameCount - minAge);

    MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
    for (auto it = renderTargetCache_.begin(); it != renderTargetCache_.end();) {
        if (it->second.frameUseIndex < ageLimit) {
            if (it->second.renderTarget) {
                MlnDestroyRenderTarget(mlnDevice, it->second.renderTarget);
            }
            it = renderTargetCache_.erase(it);
        } else {
            ++it;
        }
    }
}

void RenderBackendMln::ResetOGPoolCounters()
{
    const auto lock = std::lock_guard<std::mutex>(ogPoolsMutex_);
    for (auto& kv : ogPools_) {
        if (g_mlnLog.ogPendingStage) {
            // Merge pendingEntries (OGs created last frame, now submitted) into entries.
            for (auto& pe : kv.second.pendingEntries) {
                kv.second.entries.push_back(pe);
            }
            kv.second.pendingEntries.clear();
        }
        kv.second.frameAllocIndex = 0;
    }
}

void RenderBackendMln::EvictStaleOGs()
{
    // Evict individual OG entries not used for bufferingCount+2 frames.
    // Also remove empty pools and trim pool tails that exceed this frame's usage.
    constexpr uint64_t ADDITIONAL_FRAMES = 2u;
    const uint64_t frameCount = device_.GetFrameCount();
    const auto minAge = device_.GetCommandBufferingCount() + ADDITIONAL_FRAMES;
    const auto ageLimit = (frameCount < minAge) ? 0u : (frameCount - minAge);

    MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
    const auto lock = std::lock_guard<std::mutex>(ogPoolsMutex_);

    // Helper: drain a pool's pendingEntries before erasing the pool itself.
    // Under the normal call order (ResetOGPoolCounters runs BEFORE EvictStaleOGs),
    // every pool entering this function already has pendingEntries empty — merged
    // into entries by ResetOGPoolCounters. But some worker/secondary paths may
    // push into pendingEntries concurrently, and the order guarantee could be
    // broken by future refactors; this helper ensures a clean driver-side tear-
    // down whenever a pool with non-empty pendingEntries really does get erased.
    auto drainPending = [this, mlnDevice](OGPool& pool) {
        for (auto& pe : pool.pendingEntries) {
            if (pe.objectGroup) {
                MlnDestroyObjectGroup(mlnDevice, pe.objectGroup);
                ogTotalCount_--;
            }
        }
        pool.pendingEntries.clear();
    };

    for (auto poolIt = ogPools_.begin(); poolIt != ogPools_.end();) {
        auto& entries = poolIt->second.entries;
        auto& pending = poolIt->second.pendingEntries;
        // Trim stale entries from the tail (entries beyond this frame's usage)
        while (!entries.empty() && entries.back().lastFrameUsed < ageLimit) {
            if (entries.back().objectGroup) {
                MlnDestroyObjectGroup(mlnDevice, entries.back().objectGroup);
                ogTotalCount_--;
            }
            entries.pop_back();
        }
        // Only erase a pool when BOTH entries AND pendingEntries are empty.
        // In the normal path pending is already empty here (merged by
        // ResetOGPoolCounters just before us), so this degenerates to the
        // historical `entries.empty()` check. The extra `pending.empty()` guard
        // is a defensive invariant — if a worker produced OGs after the merge
        // or the call order regresses, we still won't orphan staged OGs.
        if (entries.empty() && pending.empty()) {
            poolIt = ogPools_.erase(poolIt);
        } else {
            ++poolIt;
        }
    }

    // Hard cap: if total OG count exceeds limit, evict oldest entries globally
    while (ogTotalCount_ > OG_CACHE_MAX_TOTAL) {
        // Find the pool with the oldest tail entry
        uint64_t oldestFrame = UINT64_MAX;
        decltype(ogPools_)::iterator oldestPool = ogPools_.end();
        for (auto it = ogPools_.begin(); it != ogPools_.end(); ++it) {
            if (!it->second.entries.empty()) {
                uint64_t tailFrame = it->second.entries.back().lastFrameUsed;
                if (tailFrame < oldestFrame) {
                    oldestFrame = tailFrame;
                    oldestPool = it;
                }
            }
        }
        if (oldestPool == ogPools_.end()) {
            break;
        }
        auto& entries = oldestPool->second.entries;
        if (entries.back().objectGroup) {
            MlnDestroyObjectGroup(mlnDevice, entries.back().objectGroup);
        }
        entries.pop_back();
        ogTotalCount_--;
        // Same invariant as above: only erase a pool when BOTH vectors are empty.
        // If hard-cap trimmed entries to empty but pendingEntries still holds
        // staged OGs, drain them properly before erase to avoid leak.
        if (entries.empty() && oldestPool->second.pendingEntries.empty()) {
            ogPools_.erase(oldestPool);
        } else if (entries.empty()) {
            drainPending(oldestPool->second);
            ogPools_.erase(oldestPool);
        }
    }
}

void RenderBackendMln::ReclaimCompletedFrames()
{
    if (pendingDestroyFrames_.empty()) {
        return;
    }

    MlnDevice mlnDevice = deviceMln_.GetMlnDevice();

    // Query current timeline value
    uint64_t completedValue = 0;
    MlnGetTimelineValue(mlnDevice, submitTimeline_, &completedValue);

    // Destroy all frames whose timeline has been reached
    auto it = pendingDestroyFrames_.begin();
    while (it != pendingDestroyFrames_.end()) {
        if (it->timelineValue <= completedValue) {
            for (auto& sg : it->schedulingGraphs) {
                MlnDestroySchedulingGraph(mlnDevice, sg);
            }
            for (auto& dg : it->dataGraphs) {
                MlnDestroyDataGraph(mlnDevice, dg);
            }
            for (auto& og : it->objectGroups) {
                MlnDestroyObjectGroup(mlnDevice, og);
            }
            for (auto& rt : it->renderTargets) {
                MlnDestroyRenderTarget(mlnDevice, rt);
            }
            it = pendingDestroyFrames_.erase(it);
        } else {
            ++it;
        }
    }

    // If too many pending frames have accumulated, force-wait for the oldest
    constexpr size_t MAX_PENDING_FRAMES = 3;
    while (pendingDestroyFrames_.size() > MAX_PENDING_FRAMES) {
        auto& oldest = pendingDestroyFrames_.front();
        MlnTimelineWaitDescriptor waitDesc{};
        waitDesc.extensionCount = 0;
        waitDesc.extensions = nullptr;
        waitDesc.flags = 0;
        waitDesc.timelineCount = 1;
        waitDesc.timelines = &submitTimeline_;
        waitDesc.values = &oldest.timelineValue;
        constexpr uint64_t RECLAIM_WAIT_TIMEOUT_NS = 5000000000ULL; // 5 seconds
        MlnStatus waitResult = MlnWaitForTimelines(mlnDevice, &waitDesc, RECLAIM_WAIT_TIMEOUT_NS);
        if (waitResult == MLN_STATUS_TIMEOUT) {
            MLN_LOG_ERR(
                "ReclaimCompletedFrames: TIMEOUT 5s waiting for frame (timelineValue=%llu) — "
                "force-destroying %zu SGs, %zu DGs, %zu OGs, %zu RTs",
                static_cast<unsigned long long>(oldest.timelineValue), oldest.schedulingGraphs.size(),
                oldest.dataGraphs.size(), oldest.objectGroups.size(), oldest.renderTargets.size());
        } else if (waitResult != MLN_STATUS_SUCCESS) {
            MLN_LOG_ERR("ReclaimCompletedFrames: wait FAILED (status=%d, timelineValue=%llu)",
                static_cast<int>(waitResult), static_cast<unsigned long long>(oldest.timelineValue));
        }
        for (auto& sg : oldest.schedulingGraphs) {
            MlnDestroySchedulingGraph(mlnDevice, sg);
        }
        for (auto& dg : oldest.dataGraphs) {
            MlnDestroyDataGraph(mlnDevice, dg);
        }
        for (auto& og : oldest.objectGroups) {
            MlnDestroyObjectGroup(mlnDevice, og);
        }
        for (auto& rt : oldest.renderTargets) {
            MlnDestroyRenderTarget(mlnDevice, rt);
        }
        pendingDestroyFrames_.erase(pendingDestroyFrames_.begin());
    }
}

void RenderBackendMln::Render(
    RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    MLN_LOG_FRAME("========== FRAME-BEGIN frame=%u ==========", g_debugFrameCount);
    presentationData_.present = false;
    presentationData_.infos.clear();

    // Reclaim GPU objects from completed previous frames + evict stale caches.
    // ORDER MATTERS: ResetOGPoolCounters must run BEFORE EvictStaleOGs.
    //   - ResetOGPoolCounters merges last frame's pendingEntries into entries,
    //     populating the pools with OGs eligible for reuse this frame.
    //   - EvictStaleOGs then sees the fully populated entries and can correctly
    //     decide staleness + empty-pool removal.
    // If reversed, EvictStaleOGs would see pools with empty `entries` (pending
    // not yet merged) and erase them, taking the staged OGs with them.
    ReclaimCompletedFrames();
    EvictStaleRenderTargets();
    ResetOGPoolCounters();
    EvictStaleOGs();

    // Global descriptor sets are managed outside NodeContextDescriptorSetManager.
    auto& globalDescriptorSetMgr = static_cast<DescriptorSetManagerMln&>(device_.GetDescriptorSetManager());
    globalDescriptorSetMgr.BeginBackendFrame();
    UpdateGlobalDescriptorSets();

    const uint64_t timelineBefore = submitTimelineValue_;

    AcquirePresentationInfo(renderCommandFrameData, backBufferConfig);
    RenderAllCommandLists(renderCommandFrameData);

    // Track whether this frame actually did a QueueSubmit.
    // Used by Present to decide whether to wait on submitTimeline_.
    frameDidSubmit_ = (submitTimelineValue_ != timelineBefore);

    SubmitFrame(renderCommandFrameData);
}

void RenderBackendMln::Present(const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    if (g_mlnLog.frame) {
        MLN_LOG_FRAME("RenderBackendMln::Present (presentData=%d, infos=%zu, timelineVal=%llu)",
            presentationData_.present ? 1 : 0, presentationData_.infos.size(),
            static_cast<unsigned long long>(submitTimelineValue_));
    }
    if (!presentationData_.present) {
        MLN_LOG_FRAME("Present: presentationData_.present is false -- no swapchain present!");
        return;
    }

    for (size_t idx = 0; idx < presentationData_.infos.size(); ++idx) {
        const auto& info = presentationData_.infos[idx];
        if (!info.useSwapchain || !info.validAcquire) {
            continue;
        }

        const RenderHandle scHandle =
            (idx < backBufferConfig.swapchainData.size()) ? backBufferConfig.swapchainData[idx].handle : RenderHandle{};
        const auto* swapchain = static_cast<const SwapchainMln*>(device_.GetSwapchain(scHandle));
        if (!swapchain) {
            continue;
        }

        SwapchainMln& swapchainMln = const_cast<SwapchainMln&>(*swapchain);
        // If no QueueSubmit this frame, present without waiting (mirrors Vulkan:
        // presentationWaitSemaphore=NULL when no command buffers submitted).
        // Image was acquired but not rendered to → compositor shows undefined/black content.
        const MlnTimeline presentWaitTimeline = frameDidSubmit_ ? submitTimeline_ : MLN_NULL_HANDLE;
        const uint64_t presentWaitValue = frameDidSubmit_ ? submitTimelineValue_ : 0;
        MlnStatus result = swapchainMln.Present(info.swapchainImageIndex, presentWaitTimeline, presentWaitValue);
        if (result == MLN_STATUS_SUCCESS) {
            MLN_LOG_FRAME("Present OK (imgIdx=%u, waitTimeline=%llu)", info.swapchainImageIndex,
                static_cast<unsigned long long>(submitTimelineValue_));
        } else {
            MLN_LOG_ERR("Present FAILED (imgIdx=%u, status=%d)", info.swapchainImageIndex, static_cast<int>(result));
        }
    }
}

void RenderBackendMln::AcquirePresentationInfo(
    RenderCommandFrameData& renderCommandFrameData, const RenderBackendBackBufferConfiguration& backBufferConfig)
{
    if (!device_.HasSwapchain() || backBufferConfig.swapchainData.empty()) {
        return;
    }

    presentationData_.present = true;
    presentationData_.infos.resize(backBufferConfig.swapchainData.size());

    for (size_t swapIdx = 0; swapIdx < backBufferConfig.swapchainData.size(); ++swapIdx) {
        const auto& swapData = backBufferConfig.swapchainData[swapIdx];
        PresentationInfo pi;

        const auto* swapchain = static_cast<const SwapchainMln*>(device_.GetSwapchain(swapData.handle));
        if (!swapchain) {
            presentationData_.infos[swapIdx] = pi;
            continue;
        }

        pi.useSwapchain = true;

        // Avoid re-acquiring if the same swapchain was already acquired
        for (const auto& piRef : presentationData_.infos) {
            if (piRef.useSwapchain && piRef.validAcquire) {
                pi.useSwapchain = false;
                break;
            }
        }

        if (pi.useSwapchain) {
            SwapchainMln& swapchainMln = const_cast<SwapchainMln&>(*swapchain);
            uint32_t imageIndex = 0;
            // Signal acquireTimeline_ (binary) when image becomes available from compositor.
            // Per Maleoon spec: binary timeline ignores timelineValue, pass 0.
            MlnStatus result = swapchainMln.AcquireNextImage(imageIndex, acquireTimeline_, 0);
            if (result == MLN_STATUS_SUCCESS || result == MLN_STATUS_SUBOPTIMAL) {
                pi.validAcquire = true;
                pi.swapchainImageIndex = imageIndex;

                // Store acquired MlnResource for acquire wait targeting in QueueSubmit
                if (imageIndex < swapchainMln.GetPlatformData().images.size()) {
                    pi.acquiredImageResource = swapchainMln.GetPlatformData().images[imageIndex];
                }

                const Device::SwapchainData swapchainData = device_.GetSwapchainData(swapData.handle);
                const RenderHandle handle = swapchainData.remappableSwapchainImage;
                if (imageIndex < swapchainData.imageViewCount) {
                    const RenderHandle currentSwapchainHandle = swapchainData.imageViews[imageIndex];
                    gpuResourceMgr_.RenderBackendImmediateRemapGpuImageHandle(handle, currentSwapchainHandle);
                }

                if (g_mlnLog.frame) {
                    MLN_LOG_FRAME("AcquirePresentationInfo: acquired swapchain image %u", imageIndex);
                }
            } else {
                MLN_LOG_ERR("RenderBackendMln: AcquireNextImage failed (status=%d)", static_cast<int>(result));
                pi.validAcquire = false;
            }
        }

        presentationData_.infos[swapIdx] = pi;
    }
}

void RenderBackendMln::RenderAllCommandLists(RenderCommandFrameData& renderCommandFrameData)
{
    const auto& contexts = renderCommandFrameData.renderCommandContexts;
    MLN_LOG_FRAME("RenderAllCommandLists: frame=%u %zu command contexts total", g_debugFrameCount, contexts.size());

    // Clear binding set debug cache (only needed when graph logging enabled)
    if (g_mlnLog.graph) {
        bsDebugCache_.clear();
    }

    // Ensure fallback handles are ready before building SchedulingGraph resources.
    EnsureDefaultImageResourceView();

    const uint32_t ctxCount = static_cast<uint32_t>(contexts.size());
    MlnDevice mlnDevice = deviceMln_.GetMlnDevice();

    // =========================================================================
    // Parallel phase: each command list builds Maleoon objects independently.
    // Each task writes to its own PerCommandListResult (no shared mutable state).
    // =========================================================================
    vector<PerCommandListResult> perCmdResults(ctxCount);

    // Multi-threading controlled by MLN_ENABLE_MULTITHREADING in render_backend_mln.h
#if MLN_ENABLE_MULTITHREADING
    if (queue_) {
        // Parallel processing with Vulkan-style subpassCount grouping.
        // Groups of ctx's (subpassCount > 1) are processed in a single task (serial within group).
        // Independent ctx's (subpassCount == 1) each get their own parallel task.
        uint64_t taskId = 0;
        for (uint32_t i = 0; i < ctxCount;) {
            auto& firstCtx = renderCommandFrameData.renderCommandContexts[i];
            if (!firstCtx.renderCommandList || !firstCtx.renderCommandList->HasValidRenderCommands()) {
                ++i;
                continue;
            }
            const auto mrpData = firstCtx.renderCommandList->GetMultiRenderCommandListData();
            const uint32_t rcCount = (mrpData.subpassCount > 0) ? mrpData.subpassCount : 1;
            const uint32_t groupEnd = (i + rcCount <= ctxCount) ? (i + rcCount) : ctxCount;

            if (rcCount <= 1) {
                // Single ctx → independent parallel task
                queue_->Submit(taskId++, MlnFunctionTask::Create([this, &renderCommandFrameData, &perCmdResults, i]() {
                    auto& ctx = renderCommandFrameData.renderCommandContexts[i];
                    MLN_LOG_FRAME("TASK-START frame=%u ctx[%u] (parallel, single)", g_debugFrameCount, i);
                    PendingDestroyFrame localFrame{};
                    RenderSingleCommandList(ctx, localFrame, perCmdResults[i].dataGraphs, perCmdResults[i].dgResources);
                    perCmdResults[i].objectGroups = BASE_NS::move(localFrame.objectGroups);
                    perCmdResults[i].renderTargets = BASE_NS::move(localFrame.renderTargets);
                    MLN_LOG_FRAME("TASK-END frame=%u ctx[%u] DGs=%zu OGs=%zu", g_debugFrameCount, i,
                        perCmdResults[i].dataGraphs.size(), perCmdResults[i].objectGroups.size());
                }));
                ++i;
            } else {
                // Multi-ctx group → single task, serial within (stitched ctx Phase1-only)
                queue_->Submit(
                    taskId++, MlnFunctionTask::Create([this, &renderCommandFrameData, &perCmdResults, i, groupEnd]() {
                        MLN_LOG_FRAME("CTX-GROUP frame=%u ctx[%u..%u] (parallel)", g_debugFrameCount, i, groupEnd - 1);

                        // [OPT] Share ONE groupFrame across secondaries + primary so OGs
                        // created by secondaries (via ogDirectBuild) are tracked for deferred
                        // destruction. Previously used a discarded dummyFrame → OG leak.
                        PendingDestroyFrame groupFrame{};

                        // Step 1: Collect rpSegs from stitched ctx's (Phase 1 only)
                        vector<RenderPassSegment> collectedRpSegs;
                        for (uint32_t j = i + 1; j < groupEnd; ++j) {
                            auto& ctx = renderCommandFrameData.renderCommandContexts[j];
                            if (!ctx.renderCommandList || !ctx.renderCommandList->HasValidRenderCommands()) {
                                continue;
                            }
                            MLN_LOG_FRAME(
                                "TASK-START frame=%u ctx[%u] (Phase1-only, parallel group)", g_debugFrameCount, j);
                            vector<RenderPassSegment> rpSegs;
                            vector<MlnDataGraph> dummyDGs;
                            vector<DataGraphResourceInfo> dummyRes;
                            RenderSingleCommandList(ctx, groupFrame, dummyDGs, dummyRes, nullptr, &rpSegs);
                            for (auto& seg : rpSegs) {
                                collectedRpSegs.push_back(BASE_NS::move(seg));
                            }
                        }

                        // Step 2: Primary ctx Phase1+2 with merged rpSegs (reuse groupFrame)
                        auto& primaryCtx = renderCommandFrameData.renderCommandContexts[i];
                        MLN_LOG_FRAME("TASK-START frame=%u ctx[%u] (primary, +%zu rpSegs, parallel)", g_debugFrameCount,
                            i, collectedRpSegs.size());
                        RenderSingleCommandList(primaryCtx, groupFrame, perCmdResults[i].dataGraphs,
                            perCmdResults[i].dgResources, collectedRpSegs.empty() ? nullptr : &collectedRpSegs,
                            nullptr);
                        perCmdResults[i].objectGroups = BASE_NS::move(groupFrame.objectGroups);
                        perCmdResults[i].renderTargets = BASE_NS::move(groupFrame.renderTargets);
                        MLN_LOG_FRAME("TASK-END frame=%u ctx[%u] DGs=%zu OGs=%zu", g_debugFrameCount, i,
                            perCmdResults[i].dataGraphs.size(), perCmdResults[i].objectGroups.size());
                    }));
                i = groupEnd;
            }
        }
        MLN_LOG_FRAME("MILESTONE-5.5 frame=%u PRE-Execute: %llu tasks submitted", g_debugFrameCount,
            static_cast<unsigned long long>(taskId));
        queue_->Execute();
        queue_->Clear();
        MLN_LOG_FRAME("MILESTONE-5.6 frame=%u POST-Execute: all tasks done", g_debugFrameCount);
    } else
#endif
    {
        // Sequential processing with Vulkan-style subpassCount grouping.
        // When ctx[i].subpassCount > 1, ctx[i..i+subpassCount-1] form a group
        // that shares one render pass (= one DG in Maleoon).
        // Mirrors Vulkan: render_backend_vk.cpp line 735-784.
        for (uint32_t i = 0; i < ctxCount;) {
            auto& firstCtx = renderCommandFrameData.renderCommandContexts[i];
            if (!firstCtx.renderCommandList || !firstCtx.renderCommandList->HasValidRenderCommands()) {
                ++i;
                continue;
            }
            const auto mrpData = firstCtx.renderCommandList->GetMultiRenderCommandListData();
            const uint32_t rcCount = (mrpData.subpassCount > 0) ? mrpData.subpassCount : 1;
            const uint32_t groupEnd = (i + rcCount <= ctxCount) ? (i + rcCount) : ctxCount;

            if (rcCount <= 1) {
                // Single ctx — process normally (Phase 1 + Phase 2)
                MLN_LOG_FRAME("TASK-START frame=%u ctx[%u] (single)", g_debugFrameCount, i);
                PendingDestroyFrame localFrame{};
                RenderSingleCommandList(
                    firstCtx, localFrame, perCmdResults[i].dataGraphs, perCmdResults[i].dgResources);
                perCmdResults[i].objectGroups = BASE_NS::move(localFrame.objectGroups);
                perCmdResults[i].renderTargets = BASE_NS::move(localFrame.renderTargets);
                MLN_LOG_FRAME("TASK-END frame=%u ctx[%u] DGs=%zu OGs=%zu", g_debugFrameCount, i,
                    perCmdResults[i].dataGraphs.size(), perCmdResults[i].objectGroups.size());
                ++i;
            } else {
                // Multi-ctx group (rcCount > 1): all ctx's share one logical render pass.
                // Vulkan records them into ONE VkCommandBuffer (shared cmdBuf, single
                // vkBeginRenderPass..vkEndRenderPass). Maleoon equivalent: secondary ctx's
                // run Phase 1 only to collect drawGroups, primary ctx merges and creates
                // one DG. secondaryCmdLists flag differentiates VK secondary cmdBuf path
                // vs shared primary cmdBuf path, but both produce one RP in Vulkan.
                MLN_LOG_FRAME("CTX-GROUP frame=%u ctx[%u..%u] subpassCount=%u secondaryCmdLists=%d", g_debugFrameCount,
                    i, groupEnd - 1, rcCount, mrpData.secondaryCmdLists ? 1 : 0);

                // [OPT] Share ONE groupFrame across secondaries + primary so OGs
                // created by secondaries (via ogDirectBuild) are tracked for deferred
                // destruction. Previously used a discarded dummyFrame → OG leak.
                PendingDestroyFrame groupFrame{};

                // Step 1: Collect rpSegs from stitched ctx's (Phase 1 only)
                vector<RenderPassSegment> collectedRpSegs;
                for (uint32_t j = i + 1; j < groupEnd; ++j) {
                    auto& ctx = renderCommandFrameData.renderCommandContexts[j];
                    if (!ctx.renderCommandList || !ctx.renderCommandList->HasValidRenderCommands()) {
                        continue;
                    }
                    MLN_LOG_FRAME("TASK-START frame=%u ctx[%u] (Phase1-only, group member)", g_debugFrameCount, j);
                    vector<RenderPassSegment> rpSegs;
                    vector<MlnDataGraph> dummyDGs;
                    vector<DataGraphResourceInfo> dummyRes;
                    RenderSingleCommandList(ctx, groupFrame, dummyDGs, dummyRes, nullptr, &rpSegs); // Phase 1 only
                    for (auto& seg : rpSegs) {
                        collectedRpSegs.push_back(BASE_NS::move(seg));
                    }
                    MLN_LOG_FRAME(
                        "TASK-END frame=%u ctx[%u] collected %zu rpSegs", g_debugFrameCount, j, rpSegs.size());
                }

                // Step 2: First ctx runs full Phase 1 + Phase 2 with merged rpSegs (reuse groupFrame)
                MLN_LOG_FRAME("TASK-START frame=%u ctx[%u] (primary, +%zu additional rpSegs)", g_debugFrameCount, i,
                    collectedRpSegs.size());
                RenderSingleCommandList(firstCtx, groupFrame, perCmdResults[i].dataGraphs, perCmdResults[i].dgResources,
                    collectedRpSegs.empty() ? nullptr : &collectedRpSegs, nullptr);
                perCmdResults[i].objectGroups = BASE_NS::move(groupFrame.objectGroups);
                perCmdResults[i].renderTargets = BASE_NS::move(groupFrame.renderTargets);
                MLN_LOG_FRAME("TASK-END frame=%u ctx[%u] DGs=%zu OGs=%zu", g_debugFrameCount, i,
                    perCmdResults[i].dataGraphs.size(), perCmdResults[i].objectGroups.size());

                i = groupEnd;
            }
        }
    }

    // =========================================================================
    // Serial merge: combine per-command-list results into one PendingDestroyFrame.
    // =========================================================================
    PendingDestroyFrame pendingFrame{};
    vector<MlnDataGraph> allDataGraphs;
    vector<DataGraphResourceInfo> allDgResources;

    for (uint32_t ci = 0; ci < ctxCount; ++ci) {
        auto& r = perCmdResults[ci];
        allDataGraphs.insert(allDataGraphs.end(), r.dataGraphs.begin(), r.dataGraphs.end());
        allDgResources.insert(allDgResources.end(), r.dgResources.begin(), r.dgResources.end());
        pendingFrame.objectGroups.insert(pendingFrame.objectGroups.end(), r.objectGroups.begin(), r.objectGroups.end());
        pendingFrame.renderTargets.insert(
            pendingFrame.renderTargets.end(), r.renderTargets.begin(), r.renderTargets.end());
    }

    // [MILESTONE-6] Phase 3 entry — all RenderSingleCommandList done, results merged
    MLN_LOG_FRAME("MILESTONE-6 frame=%u PHASE3-ENTRY: DGs=%zu OGs=%zu RTs=%zu pendingFrames=%zu rtCache=%zu",
        g_debugFrameCount, allDataGraphs.size(), pendingFrame.objectGroups.size(), pendingFrame.renderTargets.size(),
        pendingDestroyFrames_.size(), renderTargetCache_.size());

    // Nothing to submit — clean up any OGs/RTs that were created but produced no DGs
    if (allDataGraphs.empty()) {
        if (!pendingFrame.objectGroups.empty() || !pendingFrame.renderTargets.empty()) {
            MLN_LOG_ERR("RenderAllCommandLists: no DataGraphs but %zu OGs + %zu RTs leaked — destroying",
                pendingFrame.objectGroups.size(), pendingFrame.renderTargets.size());
            for (auto& og : pendingFrame.objectGroups) {
                MlnDestroyObjectGroup(mlnDevice, og);
            }
            for (auto& rt : pendingFrame.renderTargets) {
                MlnDestroyRenderTarget(mlnDevice, rt);
            }
        }
        return;
    }

    // Store all data graphs for deferred destruction
    pendingFrame.dataGraphs = allDataGraphs;

    // =========================================================================
    // Phase 3: Single SchedulingGraph submit with proper dependencies.
    // All DataGraphs are mapped to passNodes within one SG. Each passNode
    // declares its output resources (from Phase 2) and depends on the
    // previous passNode for correct cache flush/invalidate between passes.
    // =========================================================================

    // Guard: skip submit if timeline is NULL
    if (!submitTimeline_) {
        MLN_LOG_ERR("Phase3 frame=%u: SKIPPING submit — submitTimeline_ is NULL", g_debugFrameCount);
        for (auto& dg : pendingFrame.dataGraphs) {
            MlnDestroyDataGraph(mlnDevice, dg);
        }
        for (auto& og : pendingFrame.objectGroups) {
            MlnDestroyObjectGroup(mlnDevice, og);
        }
        for (auto& rt : pendingFrame.renderTargets) {
            MlnDestroyRenderTarget(mlnDevice, rt);
        }
        return;
    }

    const uint32_t dgCount = static_cast<uint32_t>(allDataGraphs.size());

    // Build passNode descriptors with output resources and sequential dependencies
    vector<uint64_t> passIds(dgCount);
    vector<MlnPassNodeDependencyDescriptor> depDescs(dgCount); // [i] = dependency for passNode[i]
    vector<MlnPassNodeDescriptor> passNodes(dgCount);

    for (uint32_t i = 0; i < dgCount; ++i) {
        passIds[i] = static_cast<uint64_t>(i + 1); // passIds: 1, 2, 3, ...

        MlnPassNodeDescriptor& pn = passNodes[i];
        pn.extensionCount = 0;
        pn.extensions = nullptr;
        pn.flags = static_cast<MlnPassNodeDescriptorFlags>(0);
        pn.passId = passIds[i];
        pn.workloads = nullptr;

        // Output resources from Phase 2
        if (i < allDgResources.size() && allDgResources[i].outputCount > 0) {
            pn.outputCount = allDgResources[i].outputCount;
            pn.outputResources = allDgResources[i].outputs;
            pn.storeop = allDgResources[i].storeOps;
        } else {
            pn.outputCount = 0;
            pn.outputResources = nullptr;
            pn.storeop = nullptr;
        }

        // Sequential dependency: passNode[i] depends on passNode[i-1]
        // [REFAC Step 8a §8.7] Stage mask precision: pull producer's srcStage
        // and consumer's srcStage (used as upper-bound dstStage approximation)
        // from allDgResources, instead of conservative ALL_COMMANDS_BIT.
        // Default of allDgResources[i].srcStage is ALL_COMMANDS_BIT (set in Step 1),
        // so any DG that doesn't override it (none currently) gets the old behavior.
        if (i > 0 && (i - 1) < allDgResources.size() && allDgResources[i - 1].outputCount > 0) {
            MlnPassNodeDependencyDescriptor& dep = depDescs[i];
            dep.extensionCount = 0;
            dep.extensions = nullptr;
            dep.passId = passIds[i - 1]; // depend on previous pass
            dep.depResourceCount = allDgResources[i - 1].outputCount;
            dep.depResources = allDgResources[i - 1].outputs;
            // [REFAC Step 8a] producer's output stage / consumer's input stage approx
            dep.srcStage = allDgResources[i - 1].srcStage;
            dep.dstStage =
                (i < allDgResources.size()) ? allDgResources[i].srcStage : MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;
            dep.filterMargin = 0;
            pn.depCount = 1;
            pn.depPasses = &depDescs[i];
        } else if (i > 0) {
            // Previous passNode has no declared output resources, but still establish
            // execution ordering to ensure correct sequencing (e.g., transfer before render).
            MlnPassNodeDependencyDescriptor& dep = depDescs[i];
            dep.extensionCount = 0;
            dep.extensions = nullptr;
            dep.passId = passIds[i - 1];
            dep.depResourceCount = 0;
            dep.depResources = nullptr;
            // [REFAC Step 8a] precise stage masks (same as above branch)
            dep.srcStage =
                ((i - 1) < allDgResources.size()) ? allDgResources[i - 1].srcStage : MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;
            dep.dstStage =
                (i < allDgResources.size()) ? allDgResources[i].srcStage : MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;
            dep.filterMargin = 0;
            pn.depCount = 1;
            pn.depPasses = &depDescs[i];
        } else {
            pn.depCount = 0;
            pn.depPasses = nullptr;
        }
    }

    // [MILESTONE-7] Pre-SchedulingGraph — passNodes built, about to create SG
    MLN_LOG_FRAME("MILESTONE-7 frame=%u PRE-CreateSG: passNodes=%u", g_debugFrameCount, dgCount);
    for (uint32_t i = 0; i < dgCount; ++i) {
        const auto& pn = passNodes[i];
        MLN_LOG_GRAPH("  passNode[%u]: frame=%u passId=%llu outputCount=%u depCount=%u DG=%p", i, g_debugFrameCount,
            static_cast<unsigned long long>(pn.passId), pn.outputCount, pn.depCount,
            reinterpret_cast<void*>(allDataGraphs[i]));
    }

    // Create single SchedulingGraph
    MlnSchedulingGraphDescriptor sgDesc{};
    sgDesc.extensionCount = 0;
    sgDesc.extensions = nullptr;
    sgDesc.flags = static_cast<MlnSchedulingGraphDescriptorFlags>(0);
    sgDesc.passNodeCount = dgCount;
    sgDesc.passNodes = passNodes.data();

    MlnSchedulingGraph sg = MlnCreateSchedulingGraph(mlnDevice, &sgDesc);

    if (!sg) {
        MLN_LOG_ERR("Phase3 frame=%u: MlnCreateSchedulingGraph FAILED (passNodes=%u)", g_debugFrameCount, dgCount);
        for (auto& dg : pendingFrame.dataGraphs) {
            MlnDestroyDataGraph(mlnDevice, dg);
        }
        for (auto& og : pendingFrame.objectGroups) {
            MlnDestroyObjectGroup(mlnDevice, og);
        }
        for (auto& rt : pendingFrame.renderTargets) {
            MlnDestroyRenderTarget(mlnDevice, rt);
        }
        return;
    }
    pendingFrame.schedulingGraphs.push_back(sg);

    // =========================================================================
    // [CREATE-SG] Comprehensive SchedulingGraph dump — prints full SG topology
    // including DG count, per-DG type/OG count/outputs/dependencies/stage masks.
    // Entire block guarded by g_mlnLog.frame — zero overhead when disabled.
    // =========================================================================
    if (g_mlnLog.frame) {
        auto dgTypeName = [](MlnProgramStageFlags srcStage) -> const char* {
            if (srcStage & MLN_PROGRAM_STAGE_ALL_TRANSFER_BIT) {
                return "TRANSFER";
            }
            if (srcStage & (MLN_PROGRAM_STAGE_COPY_BIT | MLN_PROGRAM_STAGE_BLIT_BIT)) {
                return "TRANSFER";
            }
            if (srcStage & MLN_PROGRAM_STAGE_COMPUTE_SHADER_BIT) {
                return "COMPUTE";
            }
            if (srcStage & (MLN_PROGRAM_STAGE_ALL_GRAPHICS_BIT | MLN_PROGRAM_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
                               MLN_PROGRAM_STAGE_LATE_FRAGMENT_TESTS_BIT)) {
                return "GRAPHICS";
            }
            return "UNKNOWN";
        };
        auto stageFlagsStr = [](MlnProgramStageFlags f, char* buf, size_t bufLen) {
            int pos = 0;
            if (f & MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "ALL_CMD|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_ALL_GRAPHICS_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "ALL_GFX|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "COLOR_OUT|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_LATE_FRAGMENT_TESTS_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "LATE_FRAG|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_EARLY_FRAGMENT_TESTS_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "EARLY_FRAG|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_FRAGMENT_SHADER_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "FRAG|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_VERTEX_SHADER_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "VERT|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_COMPUTE_SHADER_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "COMPUTE|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_ALL_TRANSFER_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "ALL_XFER|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_COPY_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "COPY|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_BLIT_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "BLIT|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (f & MLN_PROGRAM_STAGE_RESOLVE_BIT) {
                const int n = snprintf_s(buf + pos, bufLen - pos, bufLen - pos - 1, "RESOLVE|");
                if (n > 0) {
                    pos += n;
                }
            }
            if (pos > 0 && buf[pos - 1] == '|') {
                buf[pos - 1] = '\0'; // trim trailing '|'
            } else if (pos == 0) {
                (void)snprintf_s(buf, bufLen, bufLen - 1, "0x%x", static_cast<uint32_t>(f));
            }
        };

        // Count total OGs per DG type
        uint32_t totalTransferDGs = 0;
        uint32_t totalComputeDGs = 0;
        uint32_t totalGraphicsDGs = 0;
        uint32_t totalOGs = static_cast<uint32_t>(pendingFrame.objectGroups.size());
        uint32_t totalRTs = static_cast<uint32_t>(pendingFrame.renderTargets.size());
        for (uint32_t i = 0; i < dgCount && i < allDgResources.size(); ++i) {
            const char* t = dgTypeName(allDgResources[i].srcStage);
            if (t[0] == 'T') {
                totalTransferDGs++;
            } else if (t[0] == 'C') {
                totalComputeDGs++;
            } else if (t[0] == 'G') {
                totalGraphicsDGs++;
            }
        }

        MLN_LOG_FRAME(
            "[CREATE-SG] frame=%u SG=%p | DGs=%u (transfer=%u compute=%u graphics=%u) | OGs=%u | RTs=%u | "
            "timeline=%llu",
            g_debugFrameCount, reinterpret_cast<void*>(sg), dgCount, totalTransferDGs, totalComputeDGs,
            totalGraphicsDGs, totalOGs, totalRTs, static_cast<unsigned long long>(submitTimelineValue_));

        for (uint32_t i = 0; i < dgCount; ++i) {
            const auto& pn = passNodes[i];
            const char* type = "UNKNOWN";
            uint32_t ogCount = 0;
            MlnRenderTarget rt = MLN_NULL_HANDLE;
            char srcBuf[128] = {};
            char dstBuf[128] = {};

            if (i < allDgResources.size()) {
                type = dgTypeName(allDgResources[i].srcStage);
                rt = allDgResources[i].renderTarget;
                stageFlagsStr(allDgResources[i].srcStage, srcBuf, sizeof(srcBuf));
            }

            // Dependency info
            uint64_t depPassId = 0;
            uint32_t depResCount = 0;
            if (pn.depCount > 0 && pn.depPasses) {
                depPassId = pn.depPasses[0].passId;
                depResCount = pn.depPasses[0].depResourceCount;
                stageFlagsStr(pn.depPasses[0].srcStage, srcBuf, sizeof(srcBuf));
                stageFlagsStr(pn.depPasses[0].dstStage, dstBuf, sizeof(dstBuf));
            }

            MLN_LOG_FRAME(
                "[CREATE-SG]   DG[%u] passId=%llu type=%-8s DG=%p RT=%p | outputs=%u | dep={passId=%llu resCount=%u "
                "src=[%s] dst=[%s]}",
                i, static_cast<unsigned long long>(pn.passId), type, reinterpret_cast<void*>(allDataGraphs[i]),
                reinterpret_cast<void*>(rt), pn.outputCount, static_cast<unsigned long long>(depPassId), depResCount,
                srcBuf, dstBuf);

            // Output resources detail
            for (uint32_t oi = 0; oi < pn.outputCount && oi < MAX_DG_OUTPUT_RESOURCES; ++oi) {
                const auto& res = allDgResources[i].outputs[oi];
                MLN_LOG_FRAME("[CREATE-SG]     output[%u]: type=%u resource=%p view=%p storeOp=%u", oi,
                    static_cast<uint32_t>(res.type), reinterpret_cast<void*>(res.bufferResource),
                    reinterpret_cast<void*>(res.imageResourceView),
                    static_cast<uint32_t>(allDgResources[i].storeOps[oi]));
            }
        }

        // Dependency chain summary (visual)
        {
            char chainBuf[512] = {};
            int cpos = 0;
            for (uint32_t i = 0; i < dgCount && i < allDgResources.size(); ++i) {
                const char* t = dgTypeName(allDgResources[i].srcStage);
                char shortType = t[0]; // T/C/G/U
                if (i > 0) {
                    const int n =
                        snprintf_s(chainBuf + cpos, sizeof(chainBuf) - cpos, sizeof(chainBuf) - cpos - 1, " -> ");
                    if (n > 0) {
                        cpos += n;
                    }
                }
                {
                    const int n = snprintf_s(
                        chainBuf + cpos, sizeof(chainBuf) - cpos, sizeof(chainBuf) - cpos - 1, "%c[%u]", shortType, i);
                    if (n > 0) {
                        cpos += n;
                    }
                }
                if (cpos >= 500) {
                    (void)snprintf_s(chainBuf + cpos, sizeof(chainBuf) - cpos, sizeof(chainBuf) - cpos - 1, "...");
                    break;
                }
            }
            MLN_LOG_FRAME("[CREATE-SG]   chain: %s", chainBuf);
        }
    } // if (g_mlnLog.frame) — CREATE-SG dump

    // Single submit with all DataGraphs
    MlnSchedulingGraphSubmitDescriptor sgSubmitDesc{};
    sgSubmitDesc.schedulingGraph = sg;
    sgSubmitDesc.dataGraphCount = dgCount;
    sgSubmitDesc.dataGraphs = allDataGraphs.data();
    sgSubmitDesc.passIds = passIds.data();

    submitTimelineValue_++;

    // Signal timeline per-pass (all passes in one signal descriptor)
    vector<MlnProgramStageFlags> stageMasks(dgCount, MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT);

    // Build signal descriptors: submitTimeline_ (always) + frameSyncTimeline (if available)
    MlnTimelineSubmitDescriptor signalDescs[2]{};
    uint32_t signalCount = 1;

    // Signal timelines after the LAST pass completes.
    // Only need to signal once (last pass) — Present and WaitForFrameFence
    // just need to know "all rendering is done", not per-pass completion.
    const uint64_t lastPassId = passIds[dgCount - 1];
    // [REFAC Step 8a §8.7] Use last pass's actual srcStage (set by Build* helpers)
    // instead of conservative ALL_COMMANDS_BIT. Default is ALL_COMMANDS_BIT, so
    // unset DGs still get the old behavior.
    MlnProgramStageFlags lastPassStage = ((dgCount - 1) < allDgResources.size()) ? allDgResources[dgCount - 1].srcStage
                                                                                 : MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;

    // Signal 1: submitTimeline_ — for Present wait + deferred object destruction
    signalDescs[0].timeline = submitTimeline_;
    signalDescs[0].value = submitTimelineValue_;
    signalDescs[0].passCount = 1;
    signalDescs[0].passIds = &lastPassId;
    signalDescs[0].stageMasks = &lastPassStage;

    // Signal 2: frameSyncTimeline — for engine frame synchronization (WaitForFrameFence)
    auto* frameSync = static_cast<RenderFrameSyncMln*>(renderCommandFrameData.renderFrameSync);
    MlnTimeline frameSyncTimeline = frameSync ? frameSync->GetCurrentTimeline() : MLN_NULL_HANDLE;
    if (frameSyncTimeline) {
        signalDescs[1].timeline = frameSyncTimeline;
        signalDescs[1].value = submitTimelineValue_;
        signalDescs[1].passCount = 1;
        signalDescs[1].passIds = &lastPassId;
        signalDescs[1].stageMasks = &lastPassStage;
        signalCount = 2;
    }

    // Wait for acquire binary timeline before rendering to swapchain image.
    // ACQUIRE_WAIT_MODE controls which passes wait for acquire:
    //   1 = Partial wait: only passes writing swapchain image (causes artifacts, pending driver fix)
    //   2 = Full wait: all passes wait (correct but conservative)
    //   3 = First pass only: minimal wait, SG dependency chain ensures safety (default)
#define ACQUIRE_WAIT_MODE 3

    MlnTimelineSubmitDescriptor acquireWaitDesc{};
    uint32_t waitCount = 0;
    if (acquireTimeline_ && presentationData_.present && dgCount > 0) {
        acquireWaitDesc.timeline = acquireTimeline_;
        acquireWaitDesc.value = 0; // binary timeline: value ignored by driver

#if ACQUIRE_WAIT_MODE == 1
        // Mode 1: Partial wait — only the FIRST pass that writes swapchain image waits.
        // Causes block artifacts under GPU load. Pending driver investigation.
        vector<MlnResource> acquiredSwapImages;
        for (const auto& info : presentationData_.infos) {
            if (info.useSwapchain && info.validAcquire && info.acquiredImageResource) {
                acquiredSwapImages.push_back(info.acquiredImageResource);
            }
        }
        uint64_t firstSwapPassId = 0;
        MlnProgramStageFlags firstSwapStageMask = MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;
        bool foundSwapPass = false;
        for (uint32_t di = 0; di < dgCount && !foundSwapPass; ++di) {
            if (di < allDgResources.size()) {
                for (uint32_t oi = 0; oi < allDgResources[di].outputCount; ++oi) {
                    const MlnResource outRes = allDgResources[di].outputs[oi].bufferResource;
                    for (const auto& swapRes : acquiredSwapImages) {
                        if (outRes == swapRes) {
                            firstSwapPassId = passIds[di];
                            firstSwapStageMask = stageMasks[di];
                            foundSwapPass = true;
                            break;
                        }
                    }
                    if (foundSwapPass)
                        break;
                }
            }
        }
        if (foundSwapPass) {
            acquireWaitDesc.passCount = 1;
            acquireWaitDesc.passIds = &firstSwapPassId;
            acquireWaitDesc.stageMasks = &firstSwapStageMask;
        } else {
            // Fallback: no DG explicitly outputs to swapchain — wait all passes
            acquireWaitDesc.passCount = dgCount;
            acquireWaitDesc.passIds = passIds.data();
            acquireWaitDesc.stageMasks = stageMasks.data();
        }
        MLN_LOG_ERR("[TIMELINE] PARTIAL-WAIT: frame=%u firstSwapPass=%llu found=%d dgCount=%u", g_debugFrameCount,
            static_cast<unsigned long long>(firstSwapPassId), foundSwapPass ? 1 : 0, dgCount);

#elif ACQUIRE_WAIT_MODE == 2
        // Mode 2: Full wait — all passes wait for acquire.
        // Correct but conservative; non-swapchain passes cannot overlap with acquire.
        acquireWaitDesc.passCount = dgCount;
        acquireWaitDesc.passIds = passIds.data();
        acquireWaitDesc.stageMasks = stageMasks.data();

#elif ACQUIRE_WAIT_MODE == 3
        // Mode 3: Wait first pass only.
        // SG linear dependency chain guarantees all subsequent passes execute after
        // the first. Minimal wait, no tearing, verified on device.
        acquireWaitDesc.passCount = 1;
        acquireWaitDesc.passIds = &passIds[0];
        acquireWaitDesc.stageMasks = &stageMasks[0];
#endif

        waitCount = 1;
    }

    MlnSubmitDescriptor submitDesc{};
    submitDesc.extensionCount = 0;
    submitDesc.extensions = nullptr;
    submitDesc.flags = 0;
    submitDesc.schedulingGraphCount = 1;
    submitDesc.schedulingGraphs = &sgSubmitDesc;
    submitDesc.waitTimelineCount = waitCount;
    submitDesc.waitTimelines = (waitCount > 0) ? &acquireWaitDesc : nullptr;
    submitDesc.signalTimelineCount = signalCount;
    submitDesc.signalTimelines = signalDescs;

    // [MILESTONE-8] Pre-QueueSubmit — SG created, about to submit
    MLN_LOG_FRAME("MILESTONE-8 frame=%u PRE-QueueSubmit: SG=%p DGs=%u timeline=%llu signalCount=%u", g_debugFrameCount,
        reinterpret_cast<void*>(sg), dgCount, static_cast<unsigned long long>(submitTimelineValue_), signalCount);

    MlnStatus result = MlnQueueSubmit(deviceMln_.GetMlnQueue(), &submitDesc);
    if (result != MLN_STATUS_SUCCESS) {
        MLN_LOG_ERR("Phase3 frame=%u: MlnQueueSubmit FAILED (status=%d, DGs=%u) — destroying objects immediately",
            g_debugFrameCount, static_cast<int>(result), dgCount);
        // Submit failed — timeline will never signal this value.
        // Destroy objects immediately to avoid deadlock in ReclaimCompletedFrames.
        for (auto& sg2 : pendingFrame.schedulingGraphs) {
            MlnDestroySchedulingGraph(mlnDevice, sg2);
        }
        for (auto& dg2 : pendingFrame.dataGraphs) {
            MlnDestroyDataGraph(mlnDevice, dg2);
        }
        for (auto& og2 : pendingFrame.objectGroups) {
            MlnDestroyObjectGroup(mlnDevice, og2);
        }
        for (auto& rt2 : pendingFrame.renderTargets) {
            MlnDestroyRenderTarget(mlnDevice, rt2);
        }
        submitTimelineValue_--; // revert since signal never happened
        // MarkFrameSubmitted is only called on successful submit (above), so no deadlock risk.
        return;
    }

    // [MILESTONE-9] Post-QueueSubmit — survived submit
    MLN_LOG_FRAME("MILESTONE-9 frame=%u POST-QueueSubmit: status=%d timeline=%llu", g_debugFrameCount,
        static_cast<int>(result), static_cast<unsigned long long>(submitTimelineValue_));

    // Mark frameSync fence at the exact point of successful submit — mirrors Vulkan's
    // FrameFenceIsSignalled() inside RenderProcessSubmitCommandLists.
    // Only called here (not in SubmitFrame), so frames without QueueSubmit won't deadlock.
    if (auto* frameSync = static_cast<RenderFrameSyncMln*>(renderCommandFrameData.renderFrameSync)) {
        frameSync->MarkFrameSubmitted(submitTimelineValue_);
    }

    // Defer destruction of GPU objects until the timeline value is reached
    pendingFrame.timelineValue = submitTimelineValue_;
    pendingDestroyFrames_.push_back(BASE_NS::move(pendingFrame));

    g_debugFrameCount++;
}

void RenderBackendMln::EnsureDefaultImageResourceView()
{
    if (defaultImageResourceView_ && defaultBufferResource_) {
        return;
    }

    // Recover missing default buffer handle if image view was already cached.
    if (defaultImageResourceView_ && !defaultBufferResource_) {
        const RenderHandleReference defaultBufHandle =
            gpuResourceMgr_.GetBufferHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_BUFFER);
        const GpuBufferMln* defaultBuf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(defaultBufHandle.GetHandle());
        if (defaultBuf && defaultBuf->GetPlatformData().resource) {
            defaultBufferResource_ = defaultBuf->GetPlatformData().resource;
            MLN_LOG_GRAPH("Recovered missing default buffer resource from CORE_DEFAULT_GPU_BUFFER: %p",
                reinterpret_cast<void*>(defaultBufferResource_));
            return;
        }
    }

    const RenderHandleReference defaultImgHandle =
        gpuResourceMgr_.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
    const GpuImageMln* defaultImg = gpuResourceMgr_.GetImage<GpuImageMln>(defaultImgHandle.GetHandle());
    if (defaultImg && defaultImg->GetPlatformData().resourceView && defaultImg->GetPlatformData().resource) {
        defaultImageResourceView_ = defaultImg->GetPlatformData().resourceView;
        defaultBufferResource_ = defaultImg->GetPlatformData().resource;
        MLN_LOG_GRAPH("Default fallback handles cached from CORE_DEFAULT_GPU_IMAGE: imgView=%p bufRes=%p",
            reinterpret_cast<void*>(defaultImageResourceView_), reinterpret_cast<void*>(defaultBufferResource_));
        return;
    }

    // Fallback path: image view from default image + buffer from default buffer.
    if (defaultImg && defaultImg->GetPlatformData().resourceView) {
        const RenderHandleReference defaultBufHandle =
            gpuResourceMgr_.GetBufferHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_BUFFER);
        const GpuBufferMln* defaultBuf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(defaultBufHandle.GetHandle());
        if (defaultBuf && defaultBuf->GetPlatformData().resource) {
            defaultImageResourceView_ = defaultImg->GetPlatformData().resourceView;
            defaultBufferResource_ = defaultBuf->GetPlatformData().resource;
            MLN_LOG_GRAPH("Default fallback handles cached from image+buffer: imgView=%p bufRes=%p",
                reinterpret_cast<void*>(defaultImageResourceView_), reinterpret_cast<void*>(defaultBufferResource_));
            return;
        }
    }

    MLN_LOG_GRAPH("Default fallback handles unavailable: imgView=%p bufRes=%p",
        reinterpret_cast<void*>(defaultImageResourceView_), reinterpret_cast<void*>(defaultBufferResource_));
}

void RenderBackendMln::EnsureEmptyBindingSet()
{
    if (emptyBindingSet_) {
        return;
    }
    MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
    MlnBindingLayoutDescriptor emptyLayoutDesc{};
    emptyLayoutDesc.flags = 0;
    emptyLayoutDesc.bindingCount = 0;
    emptyLayoutDesc.bindings = nullptr;
    emptyLayoutDesc.bindingFlags = nullptr;
    emptyBindingLayout_ = MlnCreateBindingLayout(mlnDevice, &emptyLayoutDesc);
    if (emptyBindingLayout_) {
        emptyBindingSet_ = MlnCreateBindingSet(mlnDevice, emptyBindingLayout_, 0);
    }
    if (emptyBindingSet_) {
        MLN_LOG_INIT("Empty binding set created for DG-Error fix: layout=%p, set=%p",
            reinterpret_cast<void*>(emptyBindingLayout_), reinterpret_cast<void*>(emptyBindingSet_));
    } else {
        MLN_LOG_GRAPH("Failed to create empty binding set (layout=%p)", reinterpret_cast<void*>(emptyBindingLayout_));
    }
}

void RenderBackendMln::WriteDescriptorSetBindings(
    MlnBindingSet bindingSet, const DescriptorSetLayoutBindingResourcesHandler& bindingResources)
{
    if (!bindingSet) {
        return;
    }

    const MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
    const uint32_t bufferResCount = static_cast<uint32_t>(bindingResources.buffers.size());
    const uint32_t imageResCount = static_cast<uint32_t>(bindingResources.images.size());
    const uint32_t samplerResCount = static_cast<uint32_t>(bindingResources.samplers.size());

    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH("WriteDS-BEGIN: frame=%u bindingSet=%p bufs=%u imgs=%u samps=%u", g_debugFrameCount,
            reinterpret_cast<void*>(bindingSet), bufferResCount, imageResCount, samplerResCount);
    }

    // OPT #3: Batch all descriptor writes and submit in a single MlnUpdateBindingSets call.
    // Previously each binding was a separate API call (~100-400/frame), now batched (~30-80/frame).
    constexpr uint32_t BATCH_CAPACITY = 64;
    MlnWriteBindingSet batchWrites[BATCH_CAPACITY];
    MlnBindingBufferDescriptor batchBufDescs[BATCH_CAPACITY];
    MlnBindingImageDescriptor batchImgDescs[BATCH_CAPACITY];
    uint32_t batchCount = 0;

    // Write buffer bindings (including acceleration structure bindings, which come through
    // the buffer path since AS resources are stored as GpuBuffer — mirrors Vulkan backend).
    for (const auto& refBuf : bindingResources.buffers) {
        const auto& ref = refBuf.desc;
        const uint32_t descriptorCount = ref.binding.descriptorCount;
        if (descriptorCount == 0) {
            continue;
        }
        const uint32_t arrayOffset = ref.arrayOffset;
        const uint64_t requiredEntries = static_cast<uint64_t>(arrayOffset) + descriptorCount - 1u;
        if ((descriptorCount > 1u) && (requiredEntries > bufferResCount)) {
            MLN_LOG_GRAPH("UpdateDescriptorSets: buffer binding %u array out of range (offset=%u, count=%u, size=%u)",
                ref.binding.binding, arrayOffset, descriptorCount, bufferResCount);
            continue;
        }

        // Acceleration structure binding: different write path than regular buffer.
        // CORE uses 1000150000, MLN uses 12 — cannot static_cast directly.
        if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE) {
            for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
                const BindableBuffer& bRes =
                    (idx == 0) ? ref.resource : bindingResources.buffers[arrayOffset + idx - 1].desc.resource;
                const GpuBufferMln* bufPtr = gpuResourceMgr_.GetBuffer<GpuBufferMln>(bRes.handle);
                if (!bufPtr || !bufPtr->IsAccelerationStructure()) {
                    continue;
                }
                const auto& platAccel = bufPtr->GetPlatformDataAccelerationStructure();
                if (!platAccel.accelerationStructure) {
                    MLN_LOG_ERR(
                        "UpdateDescriptorSets: AS binding %u has null MlnAccelerationStructure", ref.binding.binding);
                    continue;
                }

                MlnWriteBindingSet write{};
                write.dstSet = bindingSet;
                write.dstBinding = ref.binding.binding;
                write.dstArrayElement = idx;
                write.bindingCount = 1;
                write.bindingType = MLN_BINDING_TYPE_ACCELERATION_STRUCTURE;
                write.imageDescriptor = nullptr;
                write.bufferDescriptor = nullptr;
                write.texelBufferResourceView = nullptr;
                write.inlineUniformDescriptor = nullptr;
                write.accelerationStructure = &platAccel.accelerationStructure;

                batchWrites[batchCount] = write;
                if (++batchCount >= BATCH_CAPACITY) {
                    MlnUpdateBindingSets(mlnDevice, batchCount, batchWrites, 0, nullptr);
                    batchCount = 0;
                }
            }
            continue; // skip the normal buffer path below
        }

        for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
            const BindableBuffer& bRes =
                (idx == 0) ? ref.resource : bindingResources.buffers[arrayOffset + idx - 1].desc.resource;
            const GpuBufferMln* bufPtr = gpuResourceMgr_.GetBuffer<GpuBufferMln>(bRes.handle);
            if (!bufPtr || !bufPtr->GetPlatformData().resource) {
                continue;
            }
            const auto& platBuffer = bufPtr->GetPlatformData();
            const auto optionalByteOffset = static_cast<MlnDeviceSize>(bRes.byteOffset);
            const MlnDeviceSize byteOffset =
                static_cast<MlnDeviceSize>(platBuffer.currentByteOffset) + optionalByteOffset;
            // BUG-14 FIX: Clamp range like VK backend does. bRes.byteSize defaults to
            // GPU_BUFFER_WHOLE_SIZE (0xFFFFFFFF) which is NOT MLN_WHOLE_SIZE (0xFFFFFFFFFFFFFFFF).
            // Passing 0xFFFFFFFF as range causes offset+range > bufferSize -> invalid descriptor.
            const MlnDeviceSize remainingSize =
                static_cast<MlnDeviceSize>(platBuffer.bindMemoryByteSize) - optionalByteOffset;
            const MlnDeviceSize requestedSize = static_cast<MlnDeviceSize>(bRes.byteSize);
            const MlnDeviceSize bufferRange = (requestedSize < remainingSize) ? requestedSize : remainingSize;

            MlnBindingBufferDescriptor bufDesc{};
            bufDesc.bufferResource = platBuffer.resource;
            bufDesc.offset = byteOffset;
            bufDesc.range = bufferRange;

            MlnWriteBindingSet write{};
            write.dstSet = bindingSet;
            write.dstBinding = ref.binding.binding;
            write.dstArrayElement = idx;
            write.bindingCount = 1;
            write.bindingType = static_cast<MlnBindingType>(ref.binding.descriptorType);
            write.bufferDescriptor = &bufDesc;
            write.imageDescriptor = nullptr;
            write.texelBufferResourceView = nullptr;
            write.inlineUniformDescriptor = nullptr;

            if (g_mlnLog.graph) {
                MLN_LOG_GRAPH(
                    "WriteDS: frame=%u dstBinding=%u type=%u buf=%p offset=%llu range=%llu "
                    "(mapOff=%u, appOff=%u, rawByteSize=%u, bindMemSz=%u)",
                    g_debugFrameCount, ref.binding.binding, static_cast<uint32_t>(ref.binding.descriptorType),
                    reinterpret_cast<void*>(platBuffer.resource), static_cast<unsigned long long>(byteOffset),
                    static_cast<unsigned long long>(bufferRange), platBuffer.currentByteOffset, bRes.byteOffset,
                    bRes.byteSize, platBuffer.bindMemoryByteSize);
                // Comprehensive buffer content dump for black-screen diagnosis
                if (platBuffer.mappedData && bufferRange >= 16) {
                    const uint8_t* base = static_cast<const uint8_t*>(platBuffer.mappedData) + byteOffset;
                    const float* f = reinterpret_cast<const float*>(base);
                    const uint32_t* u = reinterpret_cast<const uint32_t*>(base);
                    const uint32_t binding = ref.binding.binding;

                    if (binding == 3u) {
                        // DefaultMaterialFogStruct (112 bytes):
                        // uvec4 indices (0), vec4 firstLayer (16), vec4 secondLayer (32),
                        // vec4 baseFactors (48), vec4 inscatteringColor (64), vec4 envMapFactor (80),
                        // vec4 additionalFactor (96)
                        MLN_LOG_GRAPH(
                            "WriteDS-FOG bind=3: indices=[0x%08x,0x%08x,0x%08x,0x%08x] "
                            "firstLayer=[%.4f,%.4f,%.4f,%.4f]",
                            u[0], u[1], u[2], u[3], f[4], f[5], f[6], f[7]);
                        if (bufferRange >= 64) {
                            MLN_LOG_GRAPH(
                                "WriteDS-FOG bind=3: secondLayer=[%.4f,%.4f,%.4f,%.4f] "
                                "baseFactors=[%.4f,%.4f,%.4f,%.4f]",
                                f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15]);
                        }
                        if (bufferRange >= 112) {
                            MLN_LOG_GRAPH(
                                "WriteDS-FOG bind=3: inscatterColor=[%.4f,%.4f,%.4f,%.4f] "
                                "envMapFac=[%.4f,%.4f,%.4f,%.4f] addlFac=[%.4f,%.4f,%.4f,%.4f]",
                                f[16], f[17], f[18], f[19], f[20], f[21], f[22], f[23], f[24], f[25], f[26], f[27]);
                        }
                        // Check for NaN in any of the 28 floats
                        bool hasNaN = false;
                        for (uint32_t fi = 0; fi < 28 && fi * 4 < bufferRange; ++fi) {
                            if (f[fi] != f[fi]) {
                                hasNaN = true;
                                break;
                            } // NaN != NaN
                        }
                        if (hasNaN) {
                            MLN_LOG_GRAPH("WriteDS-FOG bind=3: *** NaN DETECTED in fog UBO ***");
                        }
                    } else if (binding == 4u) {
                        // DefaultMaterialLightStruct: dirBeginIdx, dirCount, ptBeginIdx, ptCount, ...
                        MLN_LOG_GRAPH(
                            "WriteDS-LIGHT bind=4: dirBeginIdx=%u dirCount=%u ptBeginIdx=%u ptCount=%u "
                            "spotBeginIdx=%u spotCount=%u range=%llu",
                            u[0], u[1], u[2], u[3], u[4], u[5], static_cast<unsigned long long>(bufferRange));
                        const uint32_t totalLights = u[1] + u[3] + u[5];
                        if (totalLights > 0 && bufferRange >= 224) {
                            const float* lc = f + 24; // light[0] at offset 96 bytes
                            MLN_LOG_GRAPH(
                                "WriteDS-LIGHT light[0] pos=[%.2f,%.2f,%.2f,%.2f] "
                                "dir=[%.2f,%.2f,%.2f,%.2f] color=[%.2f,%.2f,%.2f,%.2f]",
                                lc[0], lc[1], lc[2], lc[3], lc[4], lc[5], lc[6], lc[7], lc[8], lc[9], lc[10], lc[11]);
                        }
                    } else if (binding == 2u) {
                        MLN_LOG_GRAPH(
                            "WriteDS-ENV bind=2: indirSpec=[%.4f,%.4f,%.4f,%.4f] "
                            "indirDiff=[%.4f,%.4f,%.4f,%.4f] envMap=[%.4f,%.4f,%.4f,%.4f]",
                            f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8], f[9], f[10], f[11]);
                    } else if (binding == 5u) {
                        // GlobalPostProcessStruct (512 bytes):
                        // uvec4 flags (0), vec4 renderTimings (16), vec4 factors[14] (32..255), vec4 userFactors[16]
                        // (256..511)
                        MLN_LOG_GRAPH(
                            "WriteDS-POSTPROC bind=5: flags=[0x%08x,0x%08x,0x%08x,0x%08x] "
                            "renderTimings=[%.4f,%.4f,%.4f,%.4f]",
                            u[0], u[1], u[2], u[3], f[4], f[5], f[6], f[7]);
                        if (bufferRange >= 80) {
                            MLN_LOG_GRAPH(
                                "WriteDS-POSTPROC bind=5: factors[0]=[%.4f,%.4f,%.4f,%.4f] "
                                "factors[1]=[%.4f,%.4f,%.4f,%.4f] factors[2]=[%.4f,%.4f,%.4f,%.4f]",
                                f[8], f[9], f[10], f[11], f[12], f[13], f[14], f[15], f[16], f[17], f[18], f[19]);
                        }
                        // Check if entire buffer is zero
                        bool allZero = true;
                        for (uint32_t bi = 0; bi < bufferRange / 4 && bi < 128; ++bi) {
                            if (u[bi] != 0) {
                                allZero = false;
                                break;
                            }
                        }
                        if (allZero) {
                            MLN_LOG_GRAPH("WriteDS-POSTPROC bind=5: *** ALL ZEROS (first %u bytes) ***",
                                static_cast<uint32_t>(bufferRange < 512 ? bufferRange : 512));
                        }
                    } else if (binding == 6u) {
                        // DefaultMaterialLightClusterData[3456] SSBO (221KB)
                        // Each cluster: uint count + uint[15] lightIndices = 64 bytes
                        // Check first 16 clusters for any non-zero count
                        uint32_t nonZeroClusters = 0;
                        uint32_t maxCount = 0;
                        const uint32_t checkClusters =
                            (bufferRange >= 1024) ? 16u : static_cast<uint32_t>(bufferRange / 64);
                        for (uint32_t ci = 0; ci < checkClusters; ++ci) {
                            uint32_t clusterCount = u[ci * 16]; // 64 bytes / 4 = 16 uint32s per cluster
                            if (clusterCount > 0) {
                                ++nonZeroClusters;
                            }
                            if (clusterCount > maxCount) {
                                maxCount = clusterCount;
                            }
                        }
                        MLN_LOG_GRAPH(
                            "WriteDS-CLUSTER bind=6: first %u clusters: %u non-zero, maxCount=%u, "
                            "totalRange=%llu",
                            checkClusters, nonZeroClusters, maxCount, static_cast<unsigned long long>(bufferRange));
                        // Dump raw first 64 bytes (cluster[0])
                        MLN_LOG_GRAPH(
                            "WriteDS-CLUSTER bind=6: cluster[0] raw: [%u, %u, %u, %u, %u, %u, %u, %u, "
                            "%u, %u, %u, %u, %u, %u, %u, %u]",
                            u[0], u[1], u[2], u[3], u[4], u[5], u[6], u[7], u[8], u[9], u[10], u[11], u[12], u[13],
                            u[14], u[15]);
                        // Also check if ENTIRE buffer is zero (sample every 4KB)
                        bool allZero = true;
                        for (uint32_t bi = 0; bi < bufferRange / 4; bi += 1024) {
                            if (u[bi] != 0) {
                                allZero = false;
                                break;
                            }
                        }
                        if (allZero) {
                            MLN_LOG_GRAPH("WriteDS-CLUSTER bind=6: *** ENTIRE SSBO APPEARS ZERO (sampled) ***");
                        }
                    } else {
                        // Generic dump for other bindings (0=camera, 1=viewport, etc.)
                        MLN_LOG_GRAPH("WriteDS-DUMP bind=%u: [%.4f, %.4f, %.4f, %.4f] [%.4f, %.4f, %.4f, %.4f]",
                            binding, f[0], f[1], f[2], f[3], (bufferRange >= 32) ? f[4] : 0.f,
                            (bufferRange >= 32) ? f[5] : 0.f, (bufferRange >= 32) ? f[6] : 0.f,
                            (bufferRange >= 32) ? f[7] : 0.f);
                    }
                }
            }
            // Cache for OG dump (only when graph logging enabled)
            if (g_mlnLog.graph) {
                BindingDebugEntry dbgEntry{};
                dbgEntry.binding = ref.binding.binding;
                dbgEntry.bindingType = static_cast<uint32_t>(ref.binding.descriptorType);
                dbgEntry.bufResource = platBuffer.resource;
                dbgEntry.bufOffset = byteOffset;
                dbgEntry.bufRange = bufferRange;
                bsDebugCache_[reinterpret_cast<uintptr_t>(bindingSet)].push_back(dbgEntry);
            }
            batchBufDescs[batchCount] = bufDesc;
            batchWrites[batchCount] = write;
            batchWrites[batchCount].bufferDescriptor = &batchBufDescs[batchCount];
            if (++batchCount >= BATCH_CAPACITY) {
                MlnUpdateBindingSets(mlnDevice, batchCount, batchWrites, 0, nullptr);
                batchCount = 0;
            }
        }
    }

    // Write image bindings
    for (const auto& refImg : bindingResources.images) {
        const auto& ref = refImg.desc;
        const uint32_t descriptorCount = ref.binding.descriptorCount;
        if (descriptorCount == 0) {
            continue;
        }
        const uint32_t arrayOffset = ref.arrayOffset;
        const uint64_t requiredEntries = static_cast<uint64_t>(arrayOffset) + descriptorCount - 1u;
        if ((descriptorCount > 1u) && (requiredEntries > imageResCount)) {
            MLN_LOG_GRAPH("UpdateDescriptorSets: image binding %u array out of range (offset=%u, count=%u, size=%u)",
                ref.binding.binding, arrayOffset, descriptorCount, imageResCount);
            continue;
        }
        for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
            const BindableImage& iRes =
                (idx == 0) ? ref.resource : bindingResources.images[arrayOffset + idx - 1].desc.resource;

            MlnBindingImageDescriptor imgDesc{};
            // BUG-30 FIX: MaleoonPBR always uses SHADER_READ_ONLY_OPTIMAL for sampled image
            // bindings. AGP BindableImage defaults imageLayout to UNDEFINED (0), which causes
            // native Maleoon driver to read zeros from the texture. Override UNDEFINED to
            // SHADER_READ_ONLY_OPTIMAL to match MaleoonPBR behavior.
            if (iRes.imageLayout == ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED) {
                imgDesc.imageLayout = MLN_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            } else {
                imgDesc.imageLayout = static_cast<MlnImageLayout>(iRes.imageLayout);
            }

            const GpuImageMln* imgPtr = gpuResourceMgr_.GetImage<GpuImageMln>(iRes.handle);
            if (imgPtr && imgPtr->GetPlatformData().resourceView) {
                // Default: full mip/layer view
                MlnResourceView imageView = imgPtr->GetPlatformData().resourceView;
                // Select per-layer or per-mip view when requested (matching VK backend logic)
                const auto& platViews = imgPtr->GetPlatformDataViews();
                if ((iRes.layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS) &&
                    (iRes.layer < platViews.layerImageViews.size())) {
                    imageView = platViews.layerImageViews[iRes.layer];
                } else if (iRes.mip != PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS) {
                    if ((iRes.layer == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS) &&
                        (iRes.mip < platViews.mipImageAllLayerViews.size())) {
                        imageView = platViews.mipImageAllLayerViews[iRes.mip];
                    } else if (iRes.mip < platViews.mipImageViews.size()) {
                        imageView = platViews.mipImageViews[iRes.mip];
                    }
                }
                imgDesc.imageResourceView = imageView;
            } else if (defaultImageResourceView_) {
                MLN_LOG_ERR(
                    "UpdateDescriptorSets: null image at binding %u[%u], using default", ref.binding.binding, idx);
                imgDesc.imageResourceView = defaultImageResourceView_;
            } else {
                MLN_LOG_ERR("UpdateDescriptorSets: null image at binding %u[%u], no default, skipping",
                    ref.binding.binding, idx);
                continue;
            }

            if (RenderHandleUtil::IsValid(iRes.samplerHandle)) {
                const GpuSamplerMln* sampPtr = gpuResourceMgr_.GetSampler<GpuSamplerMln>(iRes.samplerHandle);
                if (sampPtr) {
                    imgDesc.sampler = sampPtr->GetPlatformData().sampler;
                }
            }

            MlnWriteBindingSet write{};
            write.dstSet = bindingSet;
            write.dstBinding = ref.binding.binding;
            write.dstArrayElement = idx;
            write.bindingCount = 1;
            write.bindingType = static_cast<MlnBindingType>(ref.binding.descriptorType);
            write.imageDescriptor = &imgDesc;
            write.bufferDescriptor = nullptr;
            write.texelBufferResourceView = nullptr;
            write.inlineUniformDescriptor = nullptr;

            // Fetch image info for debug cache + logging
            const GpuImageDesc imgDescInfo = imgPtr ? gpuResourceMgr_.GetImageDescriptor(iRes.handle) : GpuImageDesc{};
            if (g_mlnLog.graph) {
                const auto imgName = gpuResourceMgr_.GetName(iRes.handle);
                const MlnResource imgResource = imgPtr ? imgPtr->GetPlatformData().resource : MLN_NULL_HANDLE;
                MLN_LOG_GRAPH(
                    "WriteDS-IMG: dstBinding=%u type=%u imgResource=%p imgView=%p sampler=%p "
                    "imgLayout=%u imgSize=%ux%u imgFmt=%u isDefault=%d name='%s'",
                    ref.binding.binding, static_cast<uint32_t>(ref.binding.descriptorType),
                    reinterpret_cast<void*>(imgResource), reinterpret_cast<void*>(imgDesc.imageResourceView),
                    reinterpret_cast<void*>(imgDesc.sampler), static_cast<uint32_t>(imgDesc.imageLayout),
                    imgDescInfo.width, imgDescInfo.height, static_cast<uint32_t>(imgDescInfo.format),
                    (imgDesc.imageResourceView == defaultImageResourceView_) ? 1 : 0, imgName.c_str());
            }
            // Cache for OG dump (only when graph logging enabled)
            if (g_mlnLog.graph) {
                BindingDebugEntry dbgEntry{};
                dbgEntry.binding = ref.binding.binding;
                dbgEntry.bindingType = static_cast<uint32_t>(ref.binding.descriptorType);
                dbgEntry.imgView = imgDesc.imageResourceView;
                dbgEntry.imgSampler = imgDesc.sampler;
                dbgEntry.imgLayout = static_cast<uint32_t>(imgDesc.imageLayout);
                dbgEntry.imgWidth = imgDescInfo.width;
                dbgEntry.imgHeight = imgDescInfo.height;
                dbgEntry.imgFormat = static_cast<uint32_t>(imgDescInfo.format);
                bsDebugCache_[reinterpret_cast<uintptr_t>(bindingSet)].push_back(dbgEntry);
            }
            batchImgDescs[batchCount] = imgDesc;
            batchWrites[batchCount] = write;
            batchWrites[batchCount].imageDescriptor = &batchImgDescs[batchCount];
            if (++batchCount >= BATCH_CAPACITY) {
                MlnUpdateBindingSets(mlnDevice, batchCount, batchWrites, 0, nullptr);
                batchCount = 0;
            }
        }
    }

    // Write sampler-only bindings
    for (const auto& refSamp : bindingResources.samplers) {
        const auto& ref = refSamp.desc;
        const uint32_t descriptorCount = ref.binding.descriptorCount;
        if (descriptorCount == 0) {
            continue;
        }
        const uint32_t arrayOffset = ref.arrayOffset;
        const uint64_t requiredEntries = static_cast<uint64_t>(arrayOffset) + descriptorCount - 1u;
        if ((descriptorCount > 1u) && (requiredEntries > samplerResCount)) {
            MLN_LOG_GRAPH("UpdateDescriptorSets: sampler binding %u array out of range (offset=%u, count=%u, size=%u)",
                ref.binding.binding, arrayOffset, descriptorCount, samplerResCount);
            continue;
        }
        if (!defaultImageResourceView_) {
            MLN_LOG_GRAPH(
                "UpdateDescriptorSets: sampler binding %u has no fallback image view, skipping", ref.binding.binding);
            continue;
        }

        for (uint32_t idx = 0; idx < descriptorCount; ++idx) {
            const BindableSampler& sRes =
                (idx == 0) ? ref.resource : bindingResources.samplers[arrayOffset + idx - 1].desc.resource;
            const GpuSamplerMln* sampPtr = gpuResourceMgr_.GetSampler<GpuSamplerMln>(sRes.handle);
            if (!sampPtr || !sampPtr->GetPlatformData().sampler) {
                continue;
            }

            MlnBindingImageDescriptor imgDesc{};
            imgDesc.sampler = sampPtr->GetPlatformData().sampler;
            imgDesc.imageResourceView = defaultImageResourceView_;
            imgDesc.imageLayout = MLN_IMAGE_LAYOUT_UNDEFINED;

            MlnWriteBindingSet write{};
            write.dstSet = bindingSet;
            write.dstBinding = ref.binding.binding;
            write.dstArrayElement = idx;
            write.bindingCount = 1;
            write.bindingType = MLN_BINDING_TYPE_SAMPLER;
            write.imageDescriptor = &imgDesc;
            write.bufferDescriptor = nullptr;
            write.texelBufferResourceView = nullptr;
            write.inlineUniformDescriptor = nullptr;

            if (g_mlnLog.graph) {
                MLN_LOG_GRAPH("WriteDS-SAMP: dstBinding=%u sampler=%p", ref.binding.binding,
                    reinterpret_cast<void*>(imgDesc.sampler));
            }
            // Cache for OG dump (only when graph logging enabled)
            if (g_mlnLog.graph) {
                BindingDebugEntry dbgEntry{};
                dbgEntry.binding = ref.binding.binding;
                dbgEntry.bindingType = static_cast<uint32_t>(MLN_BINDING_TYPE_SAMPLER);
                dbgEntry.sampler = imgDesc.sampler;
                bsDebugCache_[reinterpret_cast<uintptr_t>(bindingSet)].push_back(dbgEntry);
            }
            batchImgDescs[batchCount] = imgDesc;
            batchWrites[batchCount] = write;
            batchWrites[batchCount].imageDescriptor = &batchImgDescs[batchCount];
            if (++batchCount >= BATCH_CAPACITY) {
                MlnUpdateBindingSets(mlnDevice, batchCount, batchWrites, 0, nullptr);
                batchCount = 0;
            }
        }
    }

    // Flush remaining batched descriptor writes
    if (batchCount > 0) {
        MlnUpdateBindingSets(mlnDevice, batchCount, batchWrites, 0, nullptr);
    }
}

void RenderBackendMln::UpdateGlobalDescriptorSets()
{
    auto& globalDsMgr = static_cast<DescriptorSetManagerMln&>(device_.GetDescriptorSetManager());
    const auto& allDescSets = globalDsMgr.GetUpdateDescriptorSetHandles();
    const uint32_t upDescriptorSetCount = static_cast<uint32_t>(allDescSets.size());
    if (upDescriptorSetCount == 0) {
        return;
    }

    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH("UpdateGlobalDescriptorSets: frame=%u total=%u", g_debugFrameCount, upDescriptorSetCount);
    }
    EnsureDefaultImageResourceView();
    for (uint32_t descIdx = 0U; descIdx < upDescriptorSetCount; ++descIdx) {
        const RenderHandle descHandle = allDescSets[descIdx];
        if (RenderHandleUtil::GetHandleType(descHandle) != RenderHandleType::DESCRIPTOR_SET) {
            continue;
        }
        if (!globalDsMgr.UpdateDescriptorSetGpuHandle(descHandle)) {
            continue;
        }
        const MlnBindingSet bindingSet = globalDsMgr.GetMlnBindingSet(descHandle);
        if (!bindingSet) {
            continue;
        }
        const DescriptorSetLayoutBindingResourcesHandler bindingResources =
            globalDsMgr.GetCpuDescriptorSetData(descHandle);
        WriteDescriptorSetBindings(bindingSet, bindingResources);
    }
}

void RenderBackendMln::UpdateCommandListDescriptorSets(
    const RenderCommandList& renderCommandList, NodeContextDescriptorSetManager& ncdsm)
{
    auto& dsMgr = static_cast<NodeContextDescriptorSetManagerMln&>(ncdsm);
    auto& globalDsMgr = static_cast<DescriptorSetManagerMln&>(device_.GetDescriptorSetManager());
    const auto& allDescSets = renderCommandList.GetUpdateDescriptorSetHandles();
    const auto upDescriptorSetCount = static_cast<uint32_t>(allDescSets.size());

    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH("UpdateDescriptorSets: frame=%u total=%u", g_debugFrameCount, upDescriptorSetCount);
    }
    EnsureDefaultImageResourceView();

    for (uint32_t descIdx = 0U; descIdx < upDescriptorSetCount; ++descIdx) {
        const RenderHandle descHandle = allDescSets[descIdx];
        if (RenderHandleUtil::GetHandleType(descHandle) != RenderHandleType::DESCRIPTOR_SET) {
            continue;
        }

        const bool isGlobal = (RenderHandleUtil::GetAdditionalData(descHandle) &
                                  NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT) != 0u;
        const bool needsUpdate = isGlobal ? globalDsMgr.UpdateDescriptorSetGpuHandle(descHandle)
                                          : dsMgr.UpdateDescriptorSetGpuHandle(descHandle);
        if (!needsUpdate) {
            continue;
        }

        const MlnBindingSet bindingSet =
            isGlobal ? globalDsMgr.GetMlnBindingSet(descHandle) : dsMgr.GetMlnBindingSet(descHandle);
        if (!bindingSet) {
            continue;
        }

        const DescriptorSetLayoutBindingResourcesHandler bindingResources =
            isGlobal ? globalDsMgr.GetCpuDescriptorSetData(descHandle) : dsMgr.GetCpuDescriptorSetData(descHandle);
        WriteDescriptorSetBindings(bindingSet, bindingResources);
    }
}

// WalkSecondaryCtx — independent secondary ctx walker. Collects RenderPassSegments
// with DrawCallGroup snapshots for primary ctx to lazy-merge. No DG/OG building.
void RenderBackendMln::WalkSecondaryCtx(
    RenderCommandContext& renderCommandCtx, void* outRpSegsOnlyVoid, void* pendingFrameVoid)
{
    auto* outRpSegsOnly = static_cast<vector<RenderPassSegment>*>(outRpSegsOnlyVoid);
    // [OPT] pendingFrame tracks OGs created by secondary ctx (when ogDirectBuild=true).
    // Caller guarantees non-null when direct-build is enabled. nullptr is OK only in
    // legacy fallback path (ogDirectBuild=false, no OGs created here).
    auto* pendingFrame = static_cast<PendingDestroyFrame*>(pendingFrameVoid);
    const RenderCommandList& cmdList = *renderCommandCtx.renderCommandList;
    NodeContextPsoManager& psoMgr = *renderCommandCtx.nodeContextPsoMgr;
    NodeContextDescriptorSetManager& descriptorSetMgr = *renderCommandCtx.nodeContextDescriptorSetMgr;
    const auto renderCommands = cmdList.GetRenderCommands();
    if (renderCommands.empty()) {
        return;
    }

    static_cast<NodeContextDescriptorSetManagerMln&>(descriptorSetMgr).BeginBackendFrame();
    psoMgr.BeginBackendFrame();
    UpdateCommandListDescriptorSets(cmdList, descriptorSetMgr);

    MlnRenderState state{};
    vector<RenderPassSegment> renderPassSegments;
    RenderPassSegment* currentRP = nullptr;
    uint32_t currentSubpassIndex = 0;

    for (const auto& ref : renderCommands) {
        if (!ref.rc)
            continue;
        if (HandleStateCommand(&ref, &state, currentRP, currentSubpassIndex, psoMgr, descriptorSetMgr)) {
            continue;
        }
        switch (ref.type) {
            case RenderCommandType::BEGIN_RENDER_PASS: {
                renderPassSegments.push_back({});
                currentRP = &renderPassSegments.back();
                currentRP->beginCmd = static_cast<const RenderCommandBeginRenderPass*>(ref.rc);
                currentRP->subpassStartIndex = currentRP->beginCmd->subpassStartIndex;
                currentSubpassIndex = currentRP->subpassStartIndex;
                {
                    const auto& rpDesc = currentRP->beginCmd->renderPassDesc;
                    const uint32_t maxAtt = PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT;
                    const uint32_t attCount = (rpDesc.attachmentCount > maxAtt) ? maxAtt : rpDesc.attachmentCount;
                    currentRP->renderPassData.attachmentCount = attCount;
                    for (uint32_t ai = 0; ai < attCount; ++ai) {
                        const GpuImageDesc imgDesc = gpuResourceMgr_.GetImageDescriptor(rpDesc.attachmentHandles[ai]);
                        currentRP->renderPassData.attachmentFormats[ai] = ToMlnFormat(imgDesc.format);
                        currentRP->renderPassData.attachmentSampleCounts[ai] =
                            ToMlnSampleCountFlags(imgDesc.sampleCountFlags);
                    }
                }
                state.graphicsPso = nullptr;
                state.hasGraphicsPsoHandle = false;
                state.graphicsPsoHandle = {};
                state.firstSet = 0;
                state.endSet = 0;
                for (auto& bs : state.bindingSets) {
                    bs = MLN_NULL_HANDLE;
                }
                for (auto& psd : state.perSetDynOffsets) {
                    psd.count = 0;
                }
                state.vertexBufferCount = 0;
                state.indexBuffer = MLN_NULL_HANDLE;
                state.pushConstantSize = 0;
                state.hasViewport = false;
                state.hasScissor = false;
                break;
            }
            case RenderCommandType::END_RENDER_PASS: {
                currentRP = nullptr;
                break;
            }
            case RenderCommandType::DRAW: {
                if (currentRP && !state.graphicsPso && state.hasGraphicsPsoHandle) {
                    ResolveGraphicsPsoForCurrentRp(
                        state.graphicsPsoHandle, currentRP, currentSubpassIndex, state, psoMgr);
                }
                if (!currentRP || !state.graphicsPso)
                    break;
                const auto& cmd = *static_cast<const RenderCommandDraw*>(ref.rc);
                // [OPT] Direct-build path: stack-only DrawCallGroup, no vector heap alloc.
                // Fallback path: full DrawCallGroup snapshot to drawGroups (legacy 3-phase).
                if (g_mlnLog.ogDirectBuild && pendingFrame) {
                    MlnObjectGroup og =
                        BuildOGFromDrawCommand(&state, &cmd, currentRP, currentSubpassIndex, *pendingFrame);
                    if (og) {
                        currentRP->secondaryOGHandles.push_back(og);
                    }
                } else {
                    DrawCallGroup group{};
                    const auto& psoPlat = state.graphicsPso->GetPlatformData();
                    group.psoPlat = psoPlat;
                    group.program = psoPlat.program;
                    group.subpassIndex = currentSubpassIndex;
                    SnapshotStateToDrawGroup(group, state);
                    if (cmd.drawType == DrawType::DRAW_INDEXED) {
                        group.isIndexed = true;
                        group.drawIndexedCmd.indexCount = cmd.indexCount;
                        group.drawIndexedCmd.instanceCount = cmd.instanceCount;
                        group.drawIndexedCmd.firstIndex = cmd.firstIndex;
                        group.drawIndexedCmd.vertexOffset = cmd.vertexOffset;
                        group.drawIndexedCmd.firstInstance = cmd.firstInstance;
                    } else {
                        group.isIndexed = false;
                        group.drawCmd.vertexCount = cmd.vertexCount;
                        group.drawCmd.instanceCount = cmd.instanceCount;
                        group.drawCmd.firstVertex = cmd.firstVertex;
                        group.drawCmd.firstInstance = cmd.firstInstance;
                    }
                    currentRP->drawGroups.push_back(BASE_NS::move(group));
                }
                break;
            }
            case RenderCommandType::DRAW_INDIRECT: {
                if (currentRP && !state.graphicsPso && state.hasGraphicsPsoHandle) {
                    ResolveGraphicsPsoForCurrentRp(
                        state.graphicsPsoHandle, currentRP, currentSubpassIndex, state, psoMgr);
                }
                if (!currentRP || !state.graphicsPso)
                    break;
                const auto& cmd = *static_cast<const RenderCommandDrawIndirect*>(ref.rc);
                // [OPT] Direct-build path.
                if (g_mlnLog.ogDirectBuild && pendingFrame) {
                    MlnObjectGroup og =
                        BuildOGFromDrawIndirectCommand(&state, &cmd, currentRP, currentSubpassIndex, *pendingFrame);
                    if (og) {
                        currentRP->secondaryOGHandles.push_back(og);
                    }
                } else {
                    const auto* buf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.argsHandle);
                    if (!buf)
                        break;
                    DrawCallGroup group{};
                    const auto& psoPlat = state.graphicsPso->GetPlatformData();
                    group.psoPlat = psoPlat;
                    group.program = psoPlat.program;
                    group.subpassIndex = currentSubpassIndex;
                    SnapshotStateToDrawGroup(group, state);
                    group.isIndirect = true;
                    group.drawIndirectCmd.bufferResource = buf->GetPlatformData().resource;
                    group.drawIndirectCmd.offset =
                        static_cast<MlnDeviceSize>(cmd.offset) + buf->GetPlatformData().currentByteOffset;
                    group.drawIndirectCmd.drawCount = cmd.drawCount;
                    group.drawIndirectCmd.stride = cmd.stride;
                    currentRP->drawGroups.push_back(BASE_NS::move(group));
                }
                break;
            }
            case RenderCommandType::NEXT_SUBPASS: {
                ++currentSubpassIndex;
                if (currentRP && currentRP->beginCmd && currentSubpassIndex < currentRP->beginCmd->subpasses.size()) {
                    if (state.hasGraphicsPsoHandle) {
                        ResolveGraphicsPsoForCurrentRp(
                            state.graphicsPsoHandle, currentRP, currentSubpassIndex, state, psoMgr);
                    }
                }
                break;
            }
            case RenderCommandType::DISPATCH:
            case RenderCommandType::DISPATCH_INDIRECT:
                MLN_LOG_ERR("Secondary ctx contains illegal cmd type: DISPATCH — skipping");
                break;
            case RenderCommandType::COPY_BUFFER:
            case RenderCommandType::COPY_BUFFER_IMAGE:
            case RenderCommandType::COPY_IMAGE:
            case RenderCommandType::BLIT_IMAGE:
            case RenderCommandType::CLEAR_COLOR_IMAGE:
                MLN_LOG_ERR("Secondary ctx contains illegal cmd type: transfer — skipping");
                break;
            case RenderCommandType::BARRIER_POINT:
                break;
            case RenderCommandType::EXECUTE_BACKEND_FRAME_POSITION:
                MLN_LOG_ERR("Secondary ctx contains illegal cmd type: EXECUTE_BACKEND_FRAME_POSITION — skipping");
                break;
            default:
                break;
        }
    }
    MergeStitchedRpSegs(renderPassSegments, nullptr);
    if (outRpSegsOnly) {
        *outRpSegsOnly = BASE_NS::move(renderPassSegments);
    }
}

// BuildTransferDg — builds transfer data graphs from a batch of transfer ops.
// Called by streaming walker (flushTransfer lambda) per transfer segment.
void RenderBackendMln::BuildTransferDg(void* transferOpsVec, void* transferDstImagesVec, void* transferDstBuffersVec,
    PendingDestroyFrame& pendingFrame, vector<MlnDataGraph>& outDataGraphs,
    vector<DataGraphResourceInfo>& outDgResources)
{
    auto& transferOps = *static_cast<vector<TransferOp>*>(transferOpsVec);
    const auto& transferDstImages = *static_cast<const vector<TransferDstImage>*>(transferDstImagesVec);
    static const vector<MlnResource> kEmptyBufferList;
    const auto& transferDstBuffers =
        transferDstBuffersVec ? *static_cast<const vector<MlnResource>*>(transferDstBuffersVec) : kEmptyBufferList;

    MLN_LOG_TRANS("BuildTransferDg entry: ops=%zu dstImgs=%zu dstBufs=%zu", transferOps.size(),
        transferDstImages.size(), transferDstBuffers.size());

    if (transferOps.empty()) {
        return;
    }

    const MlnDevice mlnDevice = deviceMln_.GetMlnDevice();

    // Fix up internal pointers (regions point to stack data that was moved)
    for (auto& op : transferOps) {
        switch (op.type) {
            case MLN_TRANSFER_TYPE_COPY_BUFFER:
                op.copyBuffer.regions = &op.copyBufferRegion;
                break;
            case MLN_TRANSFER_TYPE_COPY_BUFFER_TO_IMAGE:
                op.copyBufferToImage.regions = &op.bufferImageRegion;
                break;
            case MLN_TRANSFER_TYPE_COPY_IMAGE_TO_BUFFER:
                op.copyImageToBuffer.regions = &op.bufferImageRegion;
                break;
            case MLN_TRANSFER_TYPE_COPY_IMAGE:
                op.copyImage.regions = &op.imageCopyRegion;
                break;
            case MLN_TRANSFER_TYPE_BLIT:
                op.blitImage.regions = &op.blitRegion;
                break;
            case MLN_TRANSFER_TYPE_CLEAR_IMAGE:
                op.clearImage.regions = &op.clearImageRegion;
                break;
            default:
                break;
        }
    }

    vector<MlnTransferType> transferTypes(transferOps.size());
    vector<MlnTransferDescriptor> transferDescs(transferOps.size());
    for (size_t i = 0; i < transferOps.size(); ++i) {
        transferTypes[i] = transferOps[i].type;
        switch (transferOps[i].type) {
            case MLN_TRANSFER_TYPE_COPY_BUFFER:
                transferDescs[i].copyBufferDesc = &transferOps[i].copyBuffer;
                break;
            case MLN_TRANSFER_TYPE_COPY_BUFFER_TO_IMAGE:
                transferDescs[i].copyBufferToImageDesc = &transferOps[i].copyBufferToImage;
                break;
            case MLN_TRANSFER_TYPE_COPY_IMAGE_TO_BUFFER:
                transferDescs[i].copyImageToBufferDesc = &transferOps[i].copyImageToBuffer;
                break;
            case MLN_TRANSFER_TYPE_COPY_IMAGE:
                transferDescs[i].copyImageDesc = &transferOps[i].copyImage;
                break;
            case MLN_TRANSFER_TYPE_BLIT:
                transferDescs[i].blitImageDesc = &transferOps[i].blitImage;
                break;
            case MLN_TRANSFER_TYPE_CLEAR_IMAGE:
                transferDescs[i].clearColorImageDesc = &transferOps[i].clearImage;
                break;
            default:
                break;
        }
    }

    MlnTransferObjectGroupDescriptor togDesc{};
    togDesc.extensionCount = 0;
    togDesc.extensions = nullptr;
    togDesc.flags = 0;
    togDesc.count = static_cast<uint32_t>(transferOps.size());
    togDesc.transferType = transferTypes.data();
    togDesc.transferDescriptor = transferDescs.data();

    MlnObjectGroup transferOG = MlnCreateTransferObjectGroup(mlnDevice, &togDesc);

    if (g_mlnLog.trans) {
        MLN_LOG_TRANS("Phase2 frame=%u: MlnCreateTransferObjectGroup (ops=%u) -> %s", g_debugFrameCount,
            static_cast<uint32_t>(transferOps.size()), transferOG ? "OK" : "FAIL");
    }
    if (!transferOG) {
        return;
    }
    pendingFrame.objectGroups.push_back(transferOG);

    MlnTransferDataGraphDescriptor tdgDesc{};
    tdgDesc.extensionCount = 0;
    tdgDesc.extensions = nullptr;
    tdgDesc.flags = 0;
    tdgDesc.objectGroupCount = 1;
    tdgDesc.objectGroups = &transferOG;

    MlnDataGraph transferDG = MlnCreateTransferDataGraph(mlnDevice, &tdgDesc);

    if (g_mlnLog.trans) {
        MLN_LOG_TRANS("Phase2 frame=%u: MlnCreateTransferDataGraph OG=%p -> %s", g_debugFrameCount,
            reinterpret_cast<void*>(transferOG), transferDG ? "OK" : "FAIL");
    }
    if (!transferDG) {
        return;
    }
    outDataGraphs.push_back(transferDG);

    // Populate output resources from collected transfer destination images + buffers.
    // This enables SG to insert proper dependencies (TRANSFER_WRITE → consumer reads)
    // between transfer DG and downstream compute/graphics DGs. Without buffer outputs,
    // staging-written indirect args / SDF buffers race with compute dispatch on first
    // frame (see splash-screen analysis).
    DataGraphResourceInfo dgResInfo{};
    for (size_t ti = 0; ti < transferDstImages.size() && dgResInfo.outputCount < MAX_DG_OUTPUT_RESOURCES; ++ti) {
        const uint32_t idx = dgResInfo.outputCount++;
        auto& res = dgResInfo.outputs[idx];
        res.extensionCount = 0;
        res.extensions = nullptr;
        res.type = MLN_PASS_NODE_RESOURCE_TYPE_IMAGE;
        res.imageResourceView = transferDstImages[ti].resourceView;
        res.bufferResource = transferDstImages[ti].resource;
        dgResInfo.storeOps[idx] = MLN_ATTACHMENT_STORE_OP_STORE;
    }
    for (size_t bi = 0; bi < transferDstBuffers.size() && dgResInfo.outputCount < MAX_DG_OUTPUT_RESOURCES; ++bi) {
        const uint32_t idx = dgResInfo.outputCount++;
        auto& res = dgResInfo.outputs[idx];
        res.extensionCount = 0;
        res.extensions = nullptr;
        res.type = MLN_PASS_NODE_RESOURCE_TYPE_BUFFER;
        res.imageResourceView = MLN_NULL_HANDLE;
        res.bufferResource = transferDstBuffers[bi];
        dgResInfo.storeOps[idx] = MLN_ATTACHMENT_STORE_OP_STORE;
    }
    if (g_mlnLog.trans) {
        MLN_LOG_TRANS("Phase2 frame=%u: TransferDG output resources: %u total (%zu images, %zu buffers)",
            g_debugFrameCount, dgResInfo.outputCount, transferDstImages.size(), transferDstBuffers.size());
        for (uint32_t di = 0; di < dgResInfo.outputCount; ++di) {
            MLN_LOG_TRANS("  transferDst[%u]: type=%u resource=%p view=%p", di,
                static_cast<uint32_t>(dgResInfo.outputs[di].type),
                reinterpret_cast<void*>(dgResInfo.outputs[di].bufferResource),
                reinterpret_cast<void*>(dgResInfo.outputs[di].imageResourceView));
        }
    }

    // [REFAC §8.7] Stage mask precision: transfer DG writes at ALL_TRANSFER
    // stage. dstStage stays ALL_COMMANDS (conservative); Phase 3 will use
    // the consumer's srcStage to tighten passNode dependencies in Step 8.
    dgResInfo.srcStage = MLN_PROGRAM_STAGE_ALL_TRANSFER_BIT;
    dgResInfo.dstStage = MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;

    outDgResources.push_back(dgResInfo);
}

// ============================================================================
// BuildComputeDg — builds compute data graph from accumulated compute state.
// Phase 2 "Build compute data graphs" (per-iteration body, original at
// cpp:3473-3590). Each call processes one ComputeDispatchGroup, builds one
// ComputeOG and one ComputeDG. Behavior goal: byte-for-byte equivalent to
// the old inline code, plus §8.7 stage mask precision (COMPUTE_SHADER_BIT).
// ============================================================================
void RenderBackendMln::BuildComputeDg(const void* computeDispatchGroupPtr, PendingDestroyFrame& pendingFrame,
    vector<MlnDataGraph>& outDataGraphs, vector<DataGraphResourceInfo>& outDgResources)
{
    const auto& cg = *static_cast<const ComputeDispatchGroup*>(computeDispatchGroupPtr);

    if (!cg.program) {
        return;
    }

    const MlnDevice mlnDevice = deviceMln_.GetMlnDevice();

    // Validate binding sets - null handles crash Maleoon runtime
    const uint32_t maxSetCount = PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT;
    const uint32_t computeSetStart = cg.firstSet;
    const uint32_t requestedComputeSetEnd = computeSetStart + cg.bindingSetCount;
    const uint32_t computeSetEnd = (requestedComputeSetEnd > maxSetCount) ? maxSetCount : requestedComputeSetEnd;
    bool hasInvalidComputeBS = (requestedComputeSetEnd > maxSetCount) || (computeSetStart >= maxSetCount);
    for (uint32_t bsi = computeSetStart; bsi < computeSetEnd; ++bsi) {
        if (!cg.bindingSets[bsi]) {
            MLN_LOG_ERR("ComputeDispatch has null MlnBindingSet at index %u, skipping binding sets", bsi);
            hasInvalidComputeBS = true;
            break;
        }
    }

    // DG-Error fix: same padding as graphics path — fill sets 0..firstSet-1 with dummy.
    MlnBindingSet paddedComputeBS[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};
    bool usePaddedComputeBS = false;
    if (computeSetStart > 0 && !hasInvalidComputeBS && computeSetEnd > computeSetStart) {
        EnsureEmptyBindingSet();
        if (emptyBindingSet_) {
            for (uint32_t i = 0; i < computeSetStart; ++i) {
                paddedComputeBS[i] = emptyBindingSet_;
            }
            for (uint32_t i = computeSetStart; i < computeSetEnd; ++i) {
                paddedComputeBS[i] = cg.bindingSets[i];
            }
            usePaddedComputeBS = true;
        }
    }

    MlnResourceBindingSets resBS{};
    if (usePaddedComputeBS) {
        resBS.firstSet = 0;
        resBS.bindingSetCount = computeSetEnd;
        resBS.bindingSets = paddedComputeBS;
    } else {
        resBS.firstSet = computeSetStart;
        resBS.bindingSetCount = hasInvalidComputeBS ? 0 : (computeSetEnd - computeSetStart);
        resBS.bindingSets = (resBS.bindingSetCount > 0) ? &cg.bindingSets[computeSetStart] : nullptr;
    }
    resBS.dynamicOffsetCount = (resBS.bindingSetCount > 0) ? cg.bindingSetDynamicOffsetCount : 0;
    resBS.dynamicOffsets = (resBS.dynamicOffsetCount > 0) ? cg.bindingSetDynamicOffsets : nullptr;

    MlnProgramConstants pc{};
    pc.offset = 0;
    pc.size = cg.pushConstantSize;
    pc.values = cg.pushConstantData;

    MlnResourceProgramConstants resPC{};
    resPC.programConstantCount = (cg.pushConstantSize > 0) ? 1 : 0;
    resPC.programConstants = (cg.pushConstantSize > 0) ? &pc : nullptr;

    MlnComputeGroupCount groupCount{};
    MlnComputeGroupCountIndirect groupCountIndirect{};
    MlnComputeCommand actionCmd{};
    actionCmd.baseGroup = nullptr;

    if (cg.isIndirect) {
        groupCountIndirect.bufferResource = cg.indirectBuffer;
        groupCountIndirect.offset = cg.indirectOffset;
        actionCmd.type = MLN_COMPUTE_COMMAND_TYPE_GROUP_COUNT_INDIRECT;
        actionCmd.data.groupCountIndirect = &groupCountIndirect;
    } else {
        groupCount.groupCountX = cg.groupCountX;
        groupCount.groupCountY = cg.groupCountY;
        groupCount.groupCountZ = cg.groupCountZ;
        actionCmd.type = MLN_COMPUTE_COMMAND_TYPE_GROUP_COUNT;
        actionCmd.data.groupCount = &groupCount;
    }

    MlnComputeCommandDescriptor computeCmd{};
    computeCmd.extensionCount = 0;
    computeCmd.extensions = nullptr;
    computeCmd.flags = 0;
    computeCmd.command = &actionCmd;
    computeCmd.bindingSet = nullptr;
    computeCmd.programConstant = nullptr;

    MlnComputeObjectGroupDescriptor cogDesc{};
    cogDesc.extensionCount = 0;
    cogDesc.extensions = nullptr;
    cogDesc.flags = 0;
    cogDesc.program = cg.program;
    cogDesc.bindingSet = (resBS.bindingSetCount > 0) ? &resBS : nullptr;
    cogDesc.programConstant = (cg.pushConstantSize > 0) ? &resPC : nullptr;
    cogDesc.commandCount = 1;
    cogDesc.commands = &computeCmd;

    MlnObjectGroup computeOG = MLN_NULL_HANDLE;
    computeOG = MlnCreateComputeObjectGroup(mlnDevice, &cogDesc);

    if (g_mlnLog.comp) {
        if (cg.isIndirect) {
            MLN_LOG_COMP(
                "Phase2 frame=%u: MlnCreateComputeObjectGroup INDIRECT (buf=%p offset=%llu, bindings=%u, dyn=%u, "
                "pc=%u) -> %s",
                g_debugFrameCount, reinterpret_cast<void*>(cg.indirectBuffer),
                static_cast<unsigned long long>(cg.indirectOffset), cg.bindingSetCount, cg.bindingSetDynamicOffsetCount,
                cg.pushConstantSize, computeOG ? "OK" : "FAIL");
        } else {
            MLN_LOG_COMP(
                "Phase2 frame=%u: MlnCreateComputeObjectGroup (dispatch=%ux%ux%u, bindings=%u, dyn=%u, pc=%u) -> %s",
                g_debugFrameCount, cg.groupCountX, cg.groupCountY, cg.groupCountZ, cg.bindingSetCount,
                cg.bindingSetDynamicOffsetCount, cg.pushConstantSize, computeOG ? "OK" : "FAIL");
        }
    }
    if (!computeOG) {
        return;
    }
    pendingFrame.objectGroups.push_back(computeOG);

    MlnComputeDataGraphDescriptor cdgDesc{};
    cdgDesc.extensionCount = 0;
    cdgDesc.extensions = nullptr;
    cdgDesc.flags = 0;
    cdgDesc.objectGroupCount = 1;
    cdgDesc.objectGroups = &computeOG;

    MlnDataGraph computeDG = MlnCreateComputeDataGraph(mlnDevice, &cdgDesc);

    if (g_mlnLog.comp) {
        MLN_LOG_COMP("Phase2 frame=%u: MlnCreateComputeDataGraph -> %s", g_debugFrameCount, computeDG ? "OK" : "FAIL");
    }
    if (!computeDG) {
        return;
    }
    outDataGraphs.push_back(computeDG);

    // Compute DGs output via storage images/buffers; no RT attachments.
    // [REFAC §8.7] Stage mask: compute DG writes at COMPUTE_SHADER stage.

    // Storage resources bound to this dispatch are declared as DG outputs so SG
    // passNode dependencies can carry compute-write → consumer-read memory
    // visibility (matches the transfer DG output declaration pattern). Without
    // this, downstream readers of compute-written images/buffers may observe
    // stale data because no cache flush/invalidate edge is established.
    DataGraphResourceInfo dgResInfo{};
    for (const auto& res : cg.storageOutputs) {
        if (dgResInfo.outputCount >= MAX_DG_OUTPUT_RESOURCES)
            break;
        // De-dup against earlier entries (same resource may appear in multiple sets)
        bool dup = false;
        for (uint32_t i = 0; i < dgResInfo.outputCount; ++i) {
            if (dgResInfo.outputs[i].bufferResource == res.bufferResource && dgResInfo.outputs[i].type == res.type) {
                dup = true;
                break;
            }
        }
        if (dup)
            continue;
        const uint32_t idx = dgResInfo.outputCount++;
        dgResInfo.outputs[idx] = res;
        dgResInfo.storeOps[idx] = MLN_ATTACHMENT_STORE_OP_STORE;
    }
    dgResInfo.srcStage = MLN_PROGRAM_STAGE_COMPUTE_SHADER_BIT;
    dgResInfo.dstStage = MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;
    if (g_mlnLog.comp) {
        MLN_LOG_COMP("Phase2 frame=%u: ComputeDG outputs=%u (from %zu storage bindings)", g_debugFrameCount,
            dgResInfo.outputCount, cg.storageOutputs.size());
    }
    outDgResources.push_back(dgResInfo);
}

// ============================================================================
// BuildAccelStructDg — translates a CORE AS build command into Maleoon OG/DG.

// Mirrors the Vulkan backend's RenderCommand(RenderCommandBuildAccelerationStructure)
// but routes through OG/DG framework instead of recording to a command buffer.
//   CORE geometry data → MlnAccelerationStructureGeometryDescriptor[]
//   + MlnAccelerationStructureBuildRangeDescriptor[]
//   → MlnAccelerationStructureBuildCommand
//   → MlnCreateAccelerationStructureObjectGroup → OG
//   → MlnCreateAccelerationStructureDataGraph   → DG → push
// ============================================================================
void RenderBackendMln::BuildAccelStructDg(const RenderCommandBuildAccelerationStructure& renderCmd,
    PendingDestroyFrame& pendingFrame, BASE_NS::vector<MlnDataGraph>& outDataGraphs,
    BASE_NS::vector<DataGraphResourceInfo>& outDgResources)
{
    const MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
    const AsBuildGeometryData& geometry = renderCmd.geometry;

    // Resolve dst AS
    const GpuBufferMln* dstBuf = gpuResourceMgr_.GetBuffer<const GpuBufferMln>(geometry.dstAccelerationStructure);
    const GpuBufferMln* scratchBuf = gpuResourceMgr_.GetBuffer<const GpuBufferMln>(geometry.scratchBuffer.handle);
    if (!dstBuf || !dstBuf->IsAccelerationStructure() || !scratchBuf) {
        MLN_LOG_ERR("BuildAccelStructDg: null dst or scratch buffer");
        return;
    }
    const auto& dstAccel = dstBuf->GetPlatformDataAccelerationStructure();
    if (!dstAccel.accelerationStructure) {
        MLN_LOG_ERR("BuildAccelStructDg: dst MlnAccelerationStructure is null");
        return;
    }

    // Gather geometry descriptors + build range infos (mirrors Vulkan's loop)
    const size_t totalGeomCount =
        renderCmd.trianglesView.size() + renderCmd.aabbsView.size() + renderCmd.instancesView.size();
    BASE_NS::vector<MlnAccelerationStructureGeometryDescriptor> geometries(totalGeomCount);
    BASE_NS::vector<MlnAccelerationStructureBuildRangeDescriptor> ranges(totalGeomCount);
    uint32_t arrayIdx = 0;

    // Triangles
    for (const auto& tri : renderCmd.trianglesView) {
        auto& g = geometries[arrayIdx];
        g.extensionCount = 0;
        g.extensions = nullptr;
        g.geometryType = MLN_GEOMETRY_TYPE_TRIANGLES;
        g.flags = static_cast<MlnGeometryFlags>(tri.info.geometryFlags);

        auto& td = g.geometry.triangles;
        td.extensionCount = 0;
        td.extensions = nullptr;
        td.vertexFormat = static_cast<MlnFormat>(tri.info.vertexFormat);
        td.vertexStride = tri.info.vertexStride;
        td.maxVertex = tri.info.maxVertex;
        td.indexType = static_cast<MlnIndexType>(tri.info.indexType);
        td.vertexData = 0;
        td.indexData = 0;
        td.transformData = 0;

        if (const GpuBufferMln* vb = gpuResourceMgr_.GetBuffer<const GpuBufferMln>(tri.vertexData.handle); vb) {
            td.vertexData = vb->GetPlatformData().deviceAddress + tri.vertexData.offset;
        }
        if (const GpuBufferMln* ib = gpuResourceMgr_.GetBuffer<const GpuBufferMln>(tri.indexData.handle); ib) {
            td.indexData = ib->GetPlatformData().deviceAddress + tri.indexData.offset;
        }
        if (RenderHandleUtil::IsValid(tri.transformData.handle)) {
            if (const GpuBufferMln* tb = gpuResourceMgr_.GetBuffer<const GpuBufferMln>(tri.transformData.handle); tb) {
                td.transformData = tb->GetPlatformData().deviceAddress + tri.transformData.offset;
            }
        }
        ranges[arrayIdx] = {tri.info.indexCount / 3u, 0u, 0u, 0u};
        arrayIdx++;
    }
    // AABBs
    for (const auto& aabb : renderCmd.aabbsView) {
        auto& g = geometries[arrayIdx];
        g.extensionCount = 0;
        g.extensions = nullptr;
        g.geometryType = MLN_GEOMETRY_TYPE_AABBS;
        g.flags = static_cast<MlnGeometryFlags>(aabb.info.geometryFlags);
        auto& ad = g.geometry.aabbs;
        ad.extensionCount = 0;
        ad.extensions = nullptr;
        ad.stride = aabb.info.stride;
        ad.data = 0;
        if (const GpuBufferMln* buf = gpuResourceMgr_.GetBuffer<const GpuBufferMln>(aabb.data.handle); buf) {
            ad.data = buf->GetPlatformData().deviceAddress + aabb.data.offset;
        }
        ranges[arrayIdx] = {1u, 0u, 0u, 0u};
        arrayIdx++;
    }
    // Instances
    for (const auto& inst : renderCmd.instancesView) {
        auto& g = geometries[arrayIdx];
        g.extensionCount = 0;
        g.extensions = nullptr;
        g.geometryType = MLN_GEOMETRY_TYPE_INSTANCES;
        g.flags = static_cast<MlnGeometryFlags>(inst.info.geometryFlags);
        auto& id = g.geometry.instances;
        id.extensionCount = 0;
        id.extensions = nullptr;
        id.arrayOfPointers = inst.info.arrayOfPointers ? 1u : 0u;
        id.data = 0;
        if (const GpuBufferMln* buf = gpuResourceMgr_.GetBuffer<const GpuBufferMln>(inst.data.handle); buf) {
            id.data = buf->GetPlatformData().deviceAddress + inst.data.offset;
        }
        ranges[arrayIdx] = {inst.info.primitiveCount, 0u, 0u, 0u};
        arrayIdx++;
    }

    // Build geometry descriptor
    const bool isTopLevel =
        (geometry.info.type == AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL);
    MlnAccelerationStructureBuildGeometryDescriptor buildGeom{};
    buildGeom.extensionCount = 0;
    buildGeom.extensions = nullptr;
    buildGeom.type =
        isTopLevel ? MLN_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL : MLN_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    buildGeom.flags = static_cast<MlnBuildAccelerationStructureFlags>(geometry.info.flags);
    buildGeom.mode =
        (geometry.info.mode == BuildAccelerationStructureMode::CORE_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD)
            ? MLN_ACCELERATION_STRUCTURE_BUILD_MODE_BUILD
            : MLN_ACCELERATION_STRUCTURE_BUILD_MODE_UPDATE;
    buildGeom.srcAccelerationStructure = MLN_NULL_HANDLE;
    buildGeom.dstAccelerationStructure = dstAccel.accelerationStructure;
    buildGeom.geometryCount = isTopLevel ? 1u : arrayIdx;
    buildGeom.geometries = geometries.data();
    buildGeom.geometriesPointers = nullptr;
    buildGeom.scratchData = scratchBuf->GetPlatformData().deviceAddress + geometry.scratchBuffer.offset;

    // Build command
    const MlnAccelerationStructureBuildRangeDescriptor* rangesPtr = ranges.data();
    MlnAccelerationStructureBuildCommand buildCmd{};
    buildCmd.extensionCount = 0;
    buildCmd.extensions = nullptr;
    buildCmd.infoCount = 1;
    buildCmd.buildGeometry = &buildGeom;
    buildCmd.buildRanges = &rangesPtr;

    MlnAccelerationStructureCommandDescriptor asCommandDesc{};
    asCommandDesc.type = MLN_ACCELERATION_STRUCTURE_COMMAND_TYPE_BUILD;
    asCommandDesc.data.build = &buildCmd;

    // Create OG
    MlnAccelerationStructureObjectGroupDescriptor ogDesc{};
    ogDesc.extensionCount = 0;
    ogDesc.extensions = nullptr;
    ogDesc.flags = 0;
    ogDesc.commandCount = 1;
    ogDesc.commands = &asCommandDesc;

    MlnObjectGroup asOG = MlnCreateAccelerationStructureObjectGroup(mlnDevice, &ogDesc);
    if (!asOG) {
        MLN_LOG_ERR("BuildAccelStructDg: MlnCreateAccelerationStructureObjectGroup failed");
        return;
    }
    pendingFrame.objectGroups.push_back(asOG);

    // Create DG
    MlnAccelerationStructureDataGraphDescriptor dgDesc{};
    dgDesc.extensionCount = 0;
    dgDesc.extensions = nullptr;
    dgDesc.flags = 0;
    dgDesc.objectGroupCount = 1;
    dgDesc.objectGroups = &asOG;

    MlnDataGraph asDG = MlnCreateAccelerationStructureDataGraph(mlnDevice, &dgDesc);
    if (!asDG) {
        MLN_LOG_ERR("BuildAccelStructDg: MlnCreateAccelerationStructureDataGraph failed");
        return;
    }
    pendingFrame.dataGraphs.push_back(asDG);
    outDataGraphs.push_back(asDG);

    DataGraphResourceInfo dgResInfo{};
    dgResInfo.srcStage = MLN_PROGRAM_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT;
    dgResInfo.dstStage = MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;
    outDgResources.push_back(dgResInfo);
}

// ============================================================================
// BuildRenderTarget — creates or retrieves cached MlnRenderTarget for a render pass.
// Phase 2 graphics RT creation/cache block (original at cpp:3889-3987 of the
// pre-Step-3-baseline). RT cache mirrors Vulkan FramebufferCache: hash by
// attachment views + load/store ops + render area, lookup with mutex, double-
// check pattern for race when MlnCreateRenderTarget runs outside the lock.

// On success, RT is owned by renderTargetCache_ (NOT by pendingFrame).
// Returns MLN_NULL_HANDLE on failure; caller should skip the rpSeg.
// ============================================================================
MlnRenderTarget RenderBackendMln::BuildRenderTarget(const vector<MlnAttachmentDescriptor>& colorAttachments,
    const MlnAttachmentDescriptor* depthAttachment, const MlnAttachmentDescriptor* stencilAttachment,
    uint32_t areaWidth, uint32_t areaHeight, uint32_t diagSubpassIndex, uint32_t diagBeginType, size_t diagDrawCount)
{
    const MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
    const bool hasDepth = (depthAttachment != nullptr);
    const bool hasStencil = (stencilAttachment != nullptr);
    const uint32_t colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());

    // Create or reuse MlnRenderTarget via cache (mirrors Vulkan FramebufferCache)
    const uint64_t rtHash =
        HashRenderTargetConfig(colorAttachments, hasDepth, hasDepth ? *depthAttachment : MlnAttachmentDescriptor{},
            hasStencil, hasStencil ? *stencilAttachment : MlnAttachmentDescriptor{}, areaWidth, areaHeight);

    MlnRenderTarget renderTarget = MLN_NULL_HANDLE;
    const uint64_t frameCount = device_.GetFrameCount();
    {
        std::lock_guard<std::mutex> lock(renderTargetCacheMutex_);
        if (auto cacheIt = renderTargetCache_.find(rtHash); cacheIt != renderTargetCache_.end()) {
            // Cache hit — reuse existing RT
            cacheIt->second.frameUseIndex = frameCount;
            renderTarget = cacheIt->second.renderTarget;
        }
    }
    if (!renderTarget) {
        // Cache miss — create new RT outside the lock (MlnCreateRenderTarget may be slow)
        MlnRenderTargetDescriptor rtDesc{};
        rtDesc.extensionCount = 0;
        rtDesc.extensions = nullptr;
        rtDesc.flags = 0;
        rtDesc.renderArea.origin = {0, 0};
        rtDesc.renderArea.size = {areaWidth, areaHeight};
        rtDesc.viewMask = 0;
        rtDesc.layerCount = 0;
        rtDesc.colorCount = colorAttachmentCount;
        rtDesc.colors = colorAttachments.empty() ? nullptr : colorAttachments.data();
        rtDesc.depth = depthAttachment;
        rtDesc.stencil = stencilAttachment;

        // [MILESTONE-2] Pre-RT creation — if crash after this but before MILESTONE-3, it's in MlnCreateRenderTarget
        MLN_LOG_FRAME(
            "MILESTONE-2 frame=%u PRE-CreateRT: area=%ux%u colors=%u depth=%d stencil=%d "
            "layerCount=%u viewMask=%u rtHash=0x%llx",
            g_debugFrameCount, rtDesc.renderArea.size.width, rtDesc.renderArea.size.height, rtDesc.colorCount,
            hasDepth ? 1 : 0, hasStencil ? 1 : 0, rtDesc.layerCount, rtDesc.viewMask,
            static_cast<unsigned long long>(rtHash));
        for (uint32_t ci = 0; ci < colorAttachmentCount && ci < colorAttachments.size(); ++ci) {
            MLN_LOG_GRAPH("  RT-color[%u]: frame=%u view=%p loadOp=%u storeOp=%u", ci, g_debugFrameCount,
                reinterpret_cast<void*>(colorAttachments[ci].imageResourceView),
                static_cast<uint32_t>(colorAttachments[ci].loadOp),
                static_cast<uint32_t>(colorAttachments[ci].storeOp));
        }
        if (hasDepth) {
            MLN_LOG_GRAPH("  RT-depth: frame=%u view=%p loadOp=%u storeOp=%u", g_debugFrameCount,
                reinterpret_cast<void*>(depthAttachment->imageResourceView),
                static_cast<uint32_t>(depthAttachment->loadOp), static_cast<uint32_t>(depthAttachment->storeOp));
        }

        renderTarget = MlnCreateRenderTarget(mlnDevice, &rtDesc);
        if (renderTarget) {
            std::lock_guard<std::mutex> lock(renderTargetCacheMutex_);
            // Double-check: another thread may have inserted the same hash
            auto existIt = renderTargetCache_.find(rtHash);
            if (existIt != renderTargetCache_.end()) {
                // Another thread won the race — destroy our duplicate, use theirs
                MlnDestroyRenderTarget(mlnDevice, renderTarget);
                renderTarget = existIt->second.renderTarget;
                existIt->second.frameUseIndex = frameCount;
            } else {
                renderTargetCache_[rtHash] = {frameCount, renderTarget};
            }
        }
    }
    if (!renderTarget) {
        MLN_LOG_GRAPH(
            "Phase2 frame=%u: MlnCreateRenderTarget FAILED (subpass=%u, area=%ux%u, colors=%u, depth=%d, stencil=%d)",
            g_debugFrameCount, diagSubpassIndex, areaWidth, areaHeight, colorAttachmentCount, hasDepth ? 1 : 0,
            hasStencil ? 1 : 0);
        return MLN_NULL_HANDLE;
    }
    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH(
            "Phase2 frame=%u: MlnCreateRenderTarget OK (subpass=%u, beginType=%u, area=%ux%u, colors=%u, depth=%d, "
            "stencil=%d, draws=%zu)",
            g_debugFrameCount, diagSubpassIndex, diagBeginType, areaWidth, areaHeight, colorAttachmentCount,
            hasDepth ? 1 : 0, hasStencil ? 1 : 0, diagDrawCount);
        // Dump color attachment details (loadOp, storeOp, clearColor)
        for (uint32_t ci = 0; ci < colorAttachmentCount && ci < colorAttachments.size(); ++ci) {
            const auto& ca = colorAttachments[ci];
            MLN_LOG_GRAPH(
                "Phase2 frame=%u: RT color[%u] view=%p loadOp=%u storeOp=%u "
                "clear=(%.3f,%.3f,%.3f,%.3f)",
                g_debugFrameCount, ci, reinterpret_cast<void*>(ca.imageResourceView), static_cast<uint32_t>(ca.loadOp),
                static_cast<uint32_t>(ca.storeOp), ca.clearValue.color.f[0], ca.clearValue.color.f[1],
                ca.clearValue.color.f[2], ca.clearValue.color.f[3]);
        }
        if (hasDepth) {
            MLN_LOG_GRAPH("Phase2 frame=%u: RT depth view=%p loadOp=%u storeOp=%u clearDepth=%.3f", g_debugFrameCount,
                reinterpret_cast<void*>(depthAttachment->imageResourceView),
                static_cast<uint32_t>(depthAttachment->loadOp), static_cast<uint32_t>(depthAttachment->storeOp),
                depthAttachment->clearValue.depthStencil.depth);
        }
    }
    return renderTarget;
}

// ============================================================================
// BuildOGFromState — creates MlnObjectGroup from accumulated render state (OG_CACHE_MODE 0/1/2).
// Phase 2 graphics OG creation block. Original at cpp:4333-4691 (pre-Step-5b).

// 🚨 CRITICAL: Contains OG_CACHE_MODE 0/1/2 three-way #if block. Per
// feedback_large_refactor_discipline (T_3807609a_b6c005a5: OG_CACHE_MODE 1/2
// were once accidentally deleted hidden in 200+ lines of cleanup), the entire
// #if/#elif/#endif structure is moved verbatim. DO NOT split, simplify, or
// "optimize" any branch.

// Must be a private method (not free function) because it accesses class
// members: device_, ogPools_, ogPoolsMutex_, ogTotalCount_, bsDebugCache_.
// ============================================================================
void RenderBackendMln::BuildOGFromState(
    const void* inputsPtr, PendingDestroyFrame& pendingFrame, vector<MlnObjectGroup>& objectGroups)
{
    const auto& in = *static_cast<const BuildOGInputs*>(inputsPtr);
    const DrawCallGroup& dg = *in.dg;
    const MlnResourceSetDescriptor* resourceSetPtr = in.resourceSetPtr;
    const MlnStateSetDescriptor& stateSet = *in.stateSet;
    const MlnGraphicsCommandDescriptor& graphicsCmd = *in.graphicsCmd;

    const MlnDevice mlnDevice = deviceMln_.GetMlnDevice();

    // OG Pool Cache: lookup by static identity hash, allocate by frame-order index.
    // OG_CACHE_MODE configured in render_backend_mln.h
    const uint64_t ogHash = HashOGIdentity(dg);
    const uint64_t frameCount = device_.GetFrameCount();
    MlnObjectGroup objGroup = MLN_NULL_HANDLE;

#if OG_CACHE_MODE == 0
    // Mode 0: No cache — create every frame, destroy via pendingFrame (original behavior)
    MlnGraphicsObjectGroupDescriptor ogDesc{};
    ogDesc.extensionCount = 0;
    ogDesc.extensions = nullptr;
    ogDesc.flags = 0;
    ogDesc.program = dg.program;
    ogDesc.resourceSet = resourceSetPtr;
    ogDesc.stateSet = &stateSet;
    ogDesc.commandCount = 1;
    ogDesc.commands = &graphicsCmd;

    // ======== FULL OG PARAM DUMP (only when graph logging enabled) ========
    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH(
            "====== MlnCreateGraphicsObjectGroup (frame=%llu) ======", static_cast<unsigned long long>(frameCount));
        MLN_LOG_GRAPH("  ogDesc: extCount=%u flags=0x%x program=%p cmdCount=%u inheritance=%p", ogDesc.extensionCount,
            ogDesc.flags, reinterpret_cast<void*>(ogDesc.program), ogDesc.commandCount,
            reinterpret_cast<void*>(ogDesc.inheritance));

        // -- ResourceSet --
        if (ogDesc.resourceSet) {
            const auto* rs = ogDesc.resourceSet;
            MLN_LOG_GRAPH("    resourceSet: flags=0x%x dynResCount=%u", rs->flags, rs->dynamicResourceCount);
            for (uint32_t dri = 0; dri < rs->dynamicResourceCount; ++dri) {
                MLN_LOG_GRAPH("      dynResource[%u]=%u", dri, static_cast<uint32_t>(rs->dynamicResource[dri]));
            }
            // VertexBuffers
            if (rs->resourceVertexBuffers) {
                const auto* vb = rs->resourceVertexBuffers;
                MLN_LOG_GRAPH("      VB: firstBinding=%u bindingCount=%u", vb->firstBinding, vb->bindingCount);
                for (uint32_t vi = 0; vi < vb->bindingCount; ++vi) {
                    MLN_LOG_GRAPH("        VB[%u]: res=%p offset=%llu size=%llu stride=%llu", vi,
                        vb->bufferResources ? reinterpret_cast<void*>(vb->bufferResources[vi]) : nullptr,
                        vb->offsets ? static_cast<unsigned long long>(vb->offsets[vi]) : 0ULL,
                        vb->sizes ? static_cast<unsigned long long>(vb->sizes[vi]) : 0ULL,
                        vb->strides ? static_cast<unsigned long long>(vb->strides[vi]) : 0ULL);
                }
            } else {
                MLN_LOG_GRAPH("      VB: nullptr");
            }
            // IndexBuffer
            if (rs->resourceIndexBuffer) {
                const auto* ib = rs->resourceIndexBuffer;
                MLN_LOG_GRAPH("      IB: res=%p offset=%llu size=%llu indexType=%u",
                    reinterpret_cast<void*>(ib->bufferResource), static_cast<unsigned long long>(ib->offset),
                    static_cast<unsigned long long>(ib->size), static_cast<uint32_t>(ib->indexType));
            } else {
                MLN_LOG_GRAPH("      IB: nullptr");
            }
            // BindingSets
            if (rs->resourceBindingSets) {
                const auto* bs = rs->resourceBindingSets;
                MLN_LOG_GRAPH("      BS: firstSet=%u setCount=%u dynOffsetCount=%u", bs->firstSet, bs->bindingSetCount,
                    bs->dynamicOffsetCount);
                for (uint32_t bsi = 0; bsi < bs->bindingSetCount; ++bsi) {
                    MlnBindingSet bsHandle = bs->bindingSets ? bs->bindingSets[bsi] : MLN_NULL_HANDLE;
                    MLN_LOG_GRAPH("        BS[%u]=%p", bsi, reinterpret_cast<void*>(bsHandle));
                    // Look up cached binding resource details
                    auto cacheIt = bsDebugCache_.find(reinterpret_cast<uintptr_t>(bsHandle));
                    if (cacheIt != bsDebugCache_.end()) {
                        for (const auto& e : cacheIt->second) {
                            const char* typeName = "UNKNOWN";
                            switch (e.bindingType) {
                                case 0:
                                    typeName = "SAMPLER";
                                    break;
                                case 1:
                                    typeName = "COMBINED_IMG_SAMPLER";
                                    break;
                                case 2:
                                    typeName = "SAMPLED_IMAGE";
                                    break;
                                case 3:
                                    typeName = "STORAGE_IMAGE";
                                    break;
                                case 6:
                                    typeName = "UNIFORM_BUFFER";
                                    break;
                                case 7:
                                    typeName = "STORAGE_BUFFER";
                                    break;
                                case 8:
                                    typeName = "UNIFORM_BUFFER_DYN";
                                    break;
                                case 9:
                                    typeName = "STORAGE_BUFFER_DYN";
                                    break;
                                case 10:
                                    typeName = "INPUT_ATTACHMENT";
                                    break;
                                default:
                                    break;
                            }
                            if (e.bindingType >= 6 && e.bindingType <= 9) {
                                MLN_LOG_GRAPH("          b[%u][%u] %s buf=%p off=%llu range=%llu", e.binding,
                                    e.arrayIndex, typeName, reinterpret_cast<void*>(e.bufResource),
                                    static_cast<unsigned long long>(e.bufOffset),
                                    static_cast<unsigned long long>(e.bufRange));
                            } else if ((e.bindingType >= 1 && e.bindingType <= 3) || e.bindingType == 10) {
                                MLN_LOG_GRAPH("          b[%u][%u] %s view=%p samp=%p layout=%u %ux%u fmt=%u",
                                    e.binding, e.arrayIndex, typeName, reinterpret_cast<void*>(e.imgView),
                                    reinterpret_cast<void*>(e.imgSampler), e.imgLayout, e.imgWidth, e.imgHeight,
                                    e.imgFormat);
                            } else if (e.bindingType == 0) {
                                MLN_LOG_GRAPH("          b[%u][%u] SAMPLER samp=%p", e.binding, e.arrayIndex,
                                    reinterpret_cast<void*>(e.sampler));
                            }
                        }
                    } else {
                        MLN_LOG_GRAPH("          (no cached details for this BS)");
                    }
                }
                for (uint32_t doi = 0; doi < bs->dynamicOffsetCount; ++doi) {
                    MLN_LOG_GRAPH("        dynOffset[%u]=%u", doi, bs->dynamicOffsets ? bs->dynamicOffsets[doi] : 0);
                }
            } else {
                MLN_LOG_GRAPH("      BS: nullptr");
            }
            // ProgramConstants
            if (rs->resourceProgramConstants) {
                const auto* rpc = rs->resourceProgramConstants;
                MLN_LOG_GRAPH("      PC: count=%u", rpc->programConstantCount);
                for (uint32_t pci = 0; pci < rpc->programConstantCount; ++pci) {
                    const auto& pcEntry = rpc->programConstants[pci];
                    MLN_LOG_GRAPH("        PC[%u]: offset=%u size=%u values=%p", pci, pcEntry.offset, pcEntry.size,
                        pcEntry.values);
                    if (pcEntry.values && pcEntry.size > 0) {
                        const uint8_t* data = static_cast<const uint8_t*>(pcEntry.values);
                        char hexBuf[200] = {};
                        int hpos = 0;
                        const uint32_t dumpLen = (pcEntry.size < 64) ? pcEntry.size : 64;
                        for (uint32_t bi = 0; bi < dumpLen && hpos < 190; ++bi) {
                            const int n = snprintf_s(
                                hexBuf + hpos, sizeof(hexBuf) - hpos, sizeof(hexBuf) - hpos - 1, "%02x ", data[bi]);
                            if (n > 0) {
                                hpos += n;
                            }
                        }
                        MLN_LOG_GRAPH("          hex(%u bytes): %s", dumpLen, hexBuf);
                    }
                }
            } else {
                MLN_LOG_GRAPH("      PC: nullptr");
            }
        } else {
            MLN_LOG_GRAPH("    resourceSet: nullptr");
        }

        // -- StateSet --
        if (ogDesc.stateSet) {
            const auto* ss = ogDesc.stateSet;
            MLN_LOG_GRAPH(
                "    stateSet: flags=0x%x lineWidth=%.3f blend=(%.3f,%.3f,%.3f,%.3f) "
                "topology=%u primRestart=%u",
                ss->flags, ss->lineWidth, ss->blendConstants[0], ss->blendConstants[1], ss->blendConstants[2],
                ss->blendConstants[3], static_cast<uint32_t>(ss->topology), ss->primitiveRestartEnable);
            // DynamicState
            if (ss->dynamicState) {
                const auto* ds = ss->dynamicState;
                MLN_LOG_GRAPH("      dynState: count=%u", ds->dynamicStateCount);
                for (uint32_t dsi = 0; dsi < ds->dynamicStateCount; ++dsi) {
                    MLN_LOG_GRAPH(
                        "        dynStateType[%u]=%u", dsi, static_cast<uint32_t>(ds->dynamicStateTypes[dsi]));
                }
            } else {
                MLN_LOG_GRAPH("      dynState: nullptr");
            }
            // Viewport
            if (ss->viewport) {
                const auto* vp = ss->viewport;
                MLN_LOG_GRAPH("      viewport: first=%u count=%u", vp->firstViewport, vp->viewportCount);
                for (uint32_t vpi = 0; vpi < vp->viewportCount; ++vpi) {
                    const auto& v = vp->viewports[vpi];
                    MLN_LOG_GRAPH("        vp[%u]: x=%.1f y=%.1f w=%.1f h=%.1f minD=%.3f maxD=%.3f", vpi, v.x, v.y,
                        v.width, v.height, v.minDepth, v.maxDepth);
                }
            } else {
                MLN_LOG_GRAPH("      viewport: nullptr");
            }
            // Scissor
            if (ss->scissor) {
                const auto* sc = ss->scissor;
                MLN_LOG_GRAPH("      scissor: first=%u count=%u", sc->firstScissor, sc->scissorCount);
                for (uint32_t sci = 0; sci < sc->scissorCount; ++sci) {
                    const auto& s = sc->scissors[sci];
                    MLN_LOG_GRAPH("        sc[%u]: x=%d y=%d w=%u h=%u", sci, s.origin.x, s.origin.y, s.size.width,
                        s.size.height);
                }
            } else {
                MLN_LOG_GRAPH("      scissor: nullptr");
            }
            // DepthBias
            if (ss->depthBias) {
                MLN_LOG_GRAPH("      depthBias: constFactor=%.6f clamp=%.6f slopeFactor=%.6f",
                    ss->depthBias->depthBiasConstantFactor, ss->depthBias->depthBiasClamp,
                    ss->depthBias->depthBiasSlopeFactor);
            }
            // DepthBounds
            if (ss->depthBounds) {
                MLN_LOG_GRAPH("      depthBounds: min=%.6f max=%.6f", ss->depthBounds->minDepthBounds,
                    ss->depthBounds->maxDepthBounds);
            }
            // Stencil
            if (ss->compareMask) {
                MLN_LOG_GRAPH("      stencilCompareMask: face=0x%x mask=0x%x", ss->compareMask->faceMask,
                    ss->compareMask->compareMask);
            }
            if (ss->writeMask) {
                MLN_LOG_GRAPH(
                    "      stencilWriteMask: face=0x%x mask=0x%x", ss->writeMask->faceMask, ss->writeMask->writeMask);
            }
            if (ss->reference) {
                MLN_LOG_GRAPH(
                    "      stencilRef: face=0x%x ref=0x%x", ss->reference->faceMask, ss->reference->reference);
            }
        } else {
            MLN_LOG_GRAPH("    stateSet: nullptr");
        }

        // -- Commands --
        if (ogDesc.commands) {
            const auto* cmd = &ogDesc.commands[0];
            MLN_LOG_GRAPH("    cmd[0]: extCount=%u flags=0x%x resourceSet=%p stateSet=%p", cmd->extensionCount,
                cmd->flags, reinterpret_cast<void*>(cmd->resourceSet), reinterpret_cast<void*>(cmd->stateSet));
            if (cmd->command) {
                const auto* gc = cmd->command;
                if (gc->type == MLN_GRAPHICS_COMMAND_TYPE_DRAW && gc->data.draw) {
                    MLN_LOG_GRAPH("      DRAW: vtxCount=%u instCount=%u firstVtx=%u firstInst=%u",
                        gc->data.draw->vertexCount, gc->data.draw->instanceCount, gc->data.draw->firstVertex,
                        gc->data.draw->firstInstance);
                } else if (gc->type == MLN_GRAPHICS_COMMAND_TYPE_DRAW_INDEXED && gc->data.drawIndexed) {
                    MLN_LOG_GRAPH(
                        "      DRAW_INDEXED: idxCount=%u instCount=%u firstIdx=%u "
                        "vtxOffset=%u firstInst=%u",
                        gc->data.drawIndexed->indexCount, gc->data.drawIndexed->instanceCount,
                        gc->data.drawIndexed->firstIndex, gc->data.drawIndexed->vertexOffset,
                        gc->data.drawIndexed->firstInstance);
                } else if (gc->type == MLN_GRAPHICS_COMMAND_TYPE_DRAW_INDIRECT && gc->data.drawIndirect) {
                    MLN_LOG_GRAPH("      DRAW_INDIRECT: buf=%p offset=%llu drawCount=%u stride=%u",
                        reinterpret_cast<void*>(gc->data.drawIndirect->bufferResource),
                        static_cast<unsigned long long>(gc->data.drawIndirect->offset),
                        gc->data.drawIndirect->drawCount, gc->data.drawIndirect->stride);
                } else {
                    MLN_LOG_ERR("      command.type=%u (unknown or null data)", static_cast<uint32_t>(gc->type));
                }
            }
        }
        MLN_LOG_GRAPH("====== END ======");
    } // end if (g_mlnLog.graph) OG PARAM DUMP

    objGroup = MlnCreateGraphicsObjectGroup(mlnDevice, &ogDesc);
    if (objGroup) {
        objectGroups.push_back(objGroup);
        pendingFrame.objectGroups.push_back(objGroup);
        MLN_LOG_GRAPH("RESULT: objGroup=%p OK", reinterpret_cast<void*>(objGroup));
    } else {
        MLN_LOG_GRAPH("RESULT: SKIP/FAIL (program=%p)", reinterpret_cast<void*>(dg.program));
    }

#elif OG_CACHE_MODE == 1
    // Mode 1: Pool cache with destroy+recreate (avoids MlnUpdateGraphicsObjectGroup)
    // On cache hit: destroy the old OG, create a new one in its place.
    // Still benefits from stable pool sizing (no vector realloc after warm-up).
    bool cacheHit = false;
    {
        const auto lock = std::lock_guard<std::mutex>(ogPoolsMutex_);
        auto& pool = ogPools_[ogHash];
        const uint32_t idx = pool.frameAllocIndex++;

        if (idx < static_cast<uint32_t>(pool.entries.size())) {
            // Destroy old OG and replace with a fresh one
            if (pool.entries[idx].objectGroup) {
                MlnDestroyObjectGroup(mlnDevice, pool.entries[idx].objectGroup);
            }
            pool.entries[idx].objectGroup = MLN_NULL_HANDLE;
            pool.entries[idx].lastFrameUsed = frameCount;
            cacheHit = true; // slot exists, just needs a fresh OG
        }
    }

    // Create fresh OG (both cache hit with destroyed slot and cache miss)
    MlnGraphicsObjectGroupDescriptor ogDesc{};
    ogDesc.extensionCount = 0;
    ogDesc.extensions = nullptr;
    ogDesc.flags = 0;
    ogDesc.program = dg.program;
    // [SDK] MlnGraphicsObjectGroupDescriptor removed `interface` field;
    // programInterface is now implicit from program binding.
    ogDesc.resourceSet = resourceSetPtr;
    ogDesc.stateSet = &stateSet;
    ogDesc.commandCount = 1;
    ogDesc.commands = &graphicsCmd;

    objGroup = MlnCreateGraphicsObjectGroup(mlnDevice, &ogDesc);
    if (objGroup) {
        objectGroups.push_back(objGroup);
        const auto lock = std::lock_guard<std::mutex>(ogPoolsMutex_);
        if (cacheHit) {
            // Replace the destroyed slot's handle
            auto& pool = ogPools_[ogHash];
            // Find the slot we just cleared (frameAllocIndex was already incremented)
            const uint32_t idx = pool.frameAllocIndex - 1;
            if (idx < static_cast<uint32_t>(pool.entries.size())) {
                pool.entries[idx].objectGroup = objGroup;
            }
        } else {
            // Cache miss: stage to pendingEntries (when enabled) to ensure OG is
            // submitted before reuse; or directly to entries (when disabled).
            if (g_mlnLog.ogPendingStage) {
                ogPools_[ogHash].pendingEntries.push_back({frameCount, objGroup});
            } else {
                ogPools_[ogHash].entries.push_back({frameCount, objGroup});
            }
            ogTotalCount_++;
        }
    } else {

        MLN_LOG_ERR("Phase2 frame=%u: MlnCreateGraphicsObjectGroup FAILED", g_debugFrameCount);
    }

#else
    // Mode 2: Pool cache with MlnUpdateGraphicsObjectGroup (full optimization)
    bool cacheHit = false;
    {
        const auto lock = std::lock_guard<std::mutex>(ogPoolsMutex_);
        auto& pool = ogPools_[ogHash];
        const uint32_t idx = pool.frameAllocIndex++;

        if (idx < static_cast<uint32_t>(pool.entries.size())) {
            pool.entries[idx].lastFrameUsed = frameCount;
            objGroup = pool.entries[idx].objectGroup;
            cacheHit = true;
        }
    }

    if (cacheHit) {
        bool updateOk = UpdateCachedOG(mlnDevice, objGroup, dg, *in.resVB, *in.resIB, *in.resBS, *in.resPC, *in.vpState,
            *in.scState, *in.depthBiasState, *in.depthBoundsState, *in.stencilCompareMask, *in.stencilWriteMask,
            *in.stencilReference);
        if (updateOk) {
            objectGroups.push_back(objGroup);
        } else {
            MLN_LOG_ERR("Phase2: OG pool update FAILED, falling back to create");
            cacheHit = false;
        }
    }

    if (!cacheHit) {
        MlnGraphicsObjectGroupDescriptor ogDesc{};
        ogDesc.extensionCount = 0;
        ogDesc.extensions = nullptr;
        // PERSISTENT_BIT: tells driver this OG will be reused via
        // MlnUpdateGraphicsObjectGroup. Without it, driver may treat OG
        // as one-shot and release internal mutable state after DG consumption,
        // making subsequent Update calls silently no-op.
        ogDesc.flags = MLN_GRAPHICS_OBJECT_GROUP_DESCRIPTOR_PERSISTENT_BIT;
        ogDesc.program = dg.program;
        // [SDK] interface field removed (same as Mode 0 above).
        ogDesc.resourceSet = resourceSetPtr;
        ogDesc.stateSet = &stateSet;
        ogDesc.commandCount = 1;
        ogDesc.commands = &graphicsCmd;

        objGroup = MlnCreateGraphicsObjectGroup(mlnDevice, &ogDesc);
        if (objGroup) {
            objectGroups.push_back(objGroup);
            const auto lock = std::lock_guard<std::mutex>(ogPoolsMutex_);
            // Cache miss: stage to pendingEntries (when enabled) to ensure OG is
            // submitted before reuse; or directly to entries (when disabled).
            if (g_mlnLog.ogPendingStage) {
                ogPools_[ogHash].pendingEntries.push_back({frameCount, objGroup});
            } else {
                ogPools_[ogHash].entries.push_back({frameCount, objGroup});
            }
            ogTotalCount_++;
        } else {
            MLN_LOG_ERR("Phase2 frame=%u: MlnCreateGraphicsObjectGroup FAILED", g_debugFrameCount);
        }
    }
#endif // OG_CACHE_MODE
}

// Walker helpers — converted from lambdas so Handle* can call them.
void RenderBackendMln::FlushTransferBatch(void* wctxPtr)
{
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);
    auto& pf = *static_cast<PendingDestroyFrame*>(w.pendingFrame);
    auto& dgs = *static_cast<BASE_NS::vector<MlnDataGraph>*>(w.outDataGraphs);
    auto& dgr = *static_cast<BASE_NS::vector<DataGraphResourceInfo>*>(w.outDgResources);
    if (!w.curTransferOps.empty()) {
        BuildTransferDg(&w.curTransferOps, &w.curTransferDsts, &w.curTransferDstBuffers, pf, dgs, dgr);
    }
    // Always clear ALL transfer-batch state on segment close — even when ops is empty.
    // Defensive against any orphan dst entries (e.g. an outside-of-transfer BARRIER_POINT
    // that bypassed the openSeg gate due to a future regression). Prevents stale dsts
    // from leaking into the next transfer batch's outputs.
    w.curTransferOps.clear();
    w.curTransferDsts.clear();
    w.curTransferDstBuffers.clear();
    if (w.openSeg == OpenSegment::TRANSFER) {
        w.openSeg = OpenSegment::NONE;
    }
}

void RenderBackendMln::FlushGraphicsBatch(void* wctxPtr)
{
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);
    auto& pf = *static_cast<PendingDestroyFrame*>(w.pendingFrame);
    auto& dgs = *static_cast<BASE_NS::vector<MlnDataGraph>*>(w.outDataGraphs);
    auto& dgr = *static_cast<BASE_NS::vector<DataGraphResourceInfo>*>(w.outDgResources);
    if (w.curRP.beginCmd != nullptr) {
        // Handle merged secondary ctx contributions (from additionalRpSegs lazy-merge):
        //   - drawGroups (fallback path, ogDirectBuild=false): convert to OGs now.
        //   - secondaryOGHandles (optimized path, ogDirectBuild=true): already built,
        //     just copy the handles. The underlying OGs are already tracked in their
        //     group's pendingFrame (see RenderAllCommandLists groupFrame fix).
        if (!w.curRP.drawGroups.empty()) {
            BuildOGsFromDrawGroups(&w.curRP, w.curStreamSubpass, pf, w.curStreamOGs);
            w.curRP.drawGroups.clear();
        }
        if (!w.curRP.secondaryOGHandles.empty()) {
            w.curStreamOGs.insert(
                w.curStreamOGs.end(), w.curRP.secondaryOGHandles.begin(), w.curRP.secondaryOGHandles.end());
            w.curRP.secondaryOGHandles.clear();
        }
        BuildGraphicsDg(w.curRP.beginCmd, w.curStreamSubpass, w.curStreamOGs, w.streamingRpSegIdx, pf, dgs, dgr);
        w.curStreamOGs.clear();
        ++w.streamingRpSegIdx;
        w.curRP = {};
    }
    if (w.openSeg == OpenSegment::GRAPHICS) {
        w.openSeg = OpenSegment::NONE;
    }
}

void RenderBackendMln::HandleBeginRenderPass(const void* refPtr, void* wctxPtr)
{
    const auto& ref = *static_cast<const RenderCommandWithType*>(refPtr);
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);
    auto& psoMgr = *static_cast<NodeContextPsoManager*>(w.psoMgr);

    FlushTransferBatch(wctxPtr);
    FlushGraphicsBatch(wctxPtr);
    w.curRP = {};
    w.curStreamOGs.clear();
    w.curStreamSubpass = 0;
    w.currentRP = &w.curRP;
    w.openSeg = OpenSegment::GRAPHICS;
    w.currentRP->beginCmd = static_cast<const RenderCommandBeginRenderPass*>(ref.rc);
    w.currentRP->subpassStartIndex = w.currentRP->beginCmd->subpassStartIndex;
    w.currentSubpassIndex = w.currentRP->subpassStartIndex;
    // Collect attachment formats
    {
        const auto& rpDesc = w.currentRP->beginCmd->renderPassDesc;
        const uint32_t maxAtt = PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT;
        const uint32_t attCount = (rpDesc.attachmentCount > maxAtt) ? maxAtt : rpDesc.attachmentCount;
        w.currentRP->renderPassData.attachmentCount = attCount;
        for (uint32_t ai = 0; ai < attCount; ++ai) {
            const GpuImageDesc imgDesc = gpuResourceMgr_.GetImageDescriptor(rpDesc.attachmentHandles[ai]);
            w.currentRP->renderPassData.attachmentFormats[ai] = ToMlnFormat(imgDesc.format);
            w.currentRP->renderPassData.attachmentSampleCounts[ai] = ToMlnSampleCountFlags(imgDesc.sampleCountFlags);
        }
    }
    // Reset state for new render pass
    w.state.graphicsPso = nullptr;
    w.state.hasGraphicsPsoHandle = false;
    w.state.graphicsPsoHandle = {};
    w.state.firstSet = 0;
    w.state.endSet = 0;
    for (auto& bs : w.state.bindingSets) {
        bs = MLN_NULL_HANDLE;
    }
    for (auto& psd : w.state.perSetDynOffsets) {
        psd.count = 0;
    }
    w.state.vertexBufferCount = 0;
    w.state.indexBuffer = MLN_NULL_HANDLE;
    w.state.indexBufferOffset = 0;
    w.state.pushConstantSize = 0;
    w.state.hasViewport = false;
    w.state.hasScissor = false;
    w.state.hasLineWidth = false;
    w.state.hasDepthBias = false;
    w.state.hasBlendConstants = false;
    w.state.hasDepthBounds = false;
    w.state.hasStencilState = false;
    if (w.state.hasGraphicsPsoHandle) {
        ResolveGraphicsPsoForCurrentRp(w.state.graphicsPsoHandle, w.currentRP, w.currentSubpassIndex, w.state, psoMgr);
    }
}

void RenderBackendMln::HandleDraw(const void* refPtr, void* wctxPtr)
{
    const auto& ref = *static_cast<const RenderCommandWithType*>(refPtr);
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);
    auto& psoMgr = *static_cast<NodeContextPsoManager*>(w.psoMgr);
    auto& pendingFrame = *static_cast<PendingDestroyFrame*>(w.pendingFrame);

    if (w.currentRP && !w.state.graphicsPso && w.state.hasGraphicsPsoHandle) {
        ResolveGraphicsPsoForCurrentRp(w.state.graphicsPsoHandle, w.currentRP, w.currentSubpassIndex, w.state, psoMgr);
    }
    if (!w.currentRP || !w.state.graphicsPso) {
        return;
    }

    // [OPT] Direct-build path: stack DrawCallGroup, zero vector heap alloc.
    // Fallback path: tmp RenderPassSegment + drawGroups vector push (legacy, one alloc).
    if (g_mlnLog.ogDirectBuild) {
        MlnObjectGroup og = MLN_NULL_HANDLE;
        if (ref.type == RenderCommandType::DRAW) {
            const auto& cmd = *static_cast<const RenderCommandDraw*>(ref.rc);
            og = BuildOGFromDrawCommand(&w.state, &cmd, w.currentRP, w.currentSubpassIndex, pendingFrame);
        } else { // DRAW_INDIRECT
            const auto& cmd = *static_cast<const RenderCommandDrawIndirect*>(ref.rc);
            og = BuildOGFromDrawIndirectCommand(&w.state, &cmd, w.currentRP, w.currentSubpassIndex, pendingFrame);
        }
        if (og) {
            w.curStreamOGs.push_back(og);
        }
        w.curStreamSubpass = w.currentSubpassIndex;
        return;
    }

    DrawCallGroup group{};
    const auto& psoPlat = w.state.graphicsPso->GetPlatformData();
    group.psoPlat = psoPlat;
    group.program = psoPlat.program;
    group.subpassIndex = w.currentSubpassIndex;
    SnapshotStateToDrawGroup(group, w.state);

    if (ref.type == RenderCommandType::DRAW) {
        const auto& cmd = *static_cast<const RenderCommandDraw*>(ref.rc);
        if (cmd.drawType == DrawType::DRAW_INDEXED) {
            group.isIndexed = true;
            group.drawIndexedCmd.indexCount = cmd.indexCount;
            group.drawIndexedCmd.instanceCount = cmd.instanceCount;
            group.drawIndexedCmd.firstIndex = cmd.firstIndex;
            group.drawIndexedCmd.vertexOffset = cmd.vertexOffset;
            group.drawIndexedCmd.firstInstance = cmd.firstInstance;
        } else {
            group.isIndexed = false;
            group.drawCmd.vertexCount = cmd.vertexCount;
            group.drawCmd.instanceCount = cmd.instanceCount;
            group.drawCmd.firstVertex = cmd.firstVertex;
            group.drawCmd.firstInstance = cmd.firstInstance;
        }
    } else { // DRAW_INDIRECT
        const auto& cmd = *static_cast<const RenderCommandDrawIndirect*>(ref.rc);
        const auto* buf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.argsHandle);
        if (!buf) {
            return;
        }
        group.isIndirect = true;
        group.drawIndirectCmd.bufferResource = buf->GetPlatformData().resource;
        group.drawIndirectCmd.offset =
            static_cast<MlnDeviceSize>(cmd.offset) + buf->GetPlatformData().currentByteOffset;
        group.drawIndirectCmd.drawCount = cmd.drawCount;
        group.drawIndirectCmd.stride = cmd.stride;
    }

    RenderPassSegment tmpSeg{};
    tmpSeg.beginCmd = w.curRP.beginCmd;
    tmpSeg.drawGroups.push_back(BASE_NS::move(group));
    BuildOGsFromDrawGroups(&tmpSeg, w.currentSubpassIndex, pendingFrame, w.curStreamOGs);
    w.curStreamSubpass = w.currentSubpassIndex;
}

void RenderBackendMln::HandleTransferOp(const void* refPtr, void* wctxPtr)
{
    const auto& ref = *static_cast<const RenderCommandWithType*>(refPtr);
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);

    switch (ref.type) {
        case RenderCommandType::COPY_BUFFER: {
            const auto& cmd = *static_cast<const RenderCommandCopyBuffer*>(ref.rc);
            const auto* srcBuf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.srcHandle);
            const auto* dstBuf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.dstHandle);
            if (g_mlnLog.trans) {
                MLN_LOG_TRANS("Phase1 frame=%u: COPY_BUFFER src=%p dst=%p size=%u srcOff=%u dstOff=%u",
                    g_debugFrameCount, srcBuf ? reinterpret_cast<void*>(srcBuf->GetPlatformData().resource) : nullptr,
                    dstBuf ? reinterpret_cast<void*>(dstBuf->GetPlatformData().resource) : nullptr, cmd.bufferCopy.size,
                    cmd.bufferCopy.srcOffset, cmd.bufferCopy.dstOffset);
            }
            if (srcBuf && dstBuf && cmd.bufferCopy.size > 0) {
                TransferOp op{};
                op.type = MLN_TRANSFER_TYPE_COPY_BUFFER;
                op.copyBufferRegion.extensionCount = 0;
                op.copyBufferRegion.extensions = nullptr;
                op.copyBufferRegion.srcOrigin = static_cast<MlnDeviceSize>(cmd.bufferCopy.srcOffset);
                op.copyBufferRegion.dstOrigin = static_cast<MlnDeviceSize>(cmd.bufferCopy.dstOffset);
                op.copyBufferRegion.size = static_cast<MlnDeviceSize>(cmd.bufferCopy.size);

                op.copyBuffer.extensionCount = 0;
                op.copyBuffer.extensions = nullptr;
                op.copyBuffer.srcBufferResource = srcBuf->GetPlatformData().resource;
                op.copyBuffer.dstBufferResource = dstBuf->GetPlatformData().resource;
                op.copyBuffer.regionCount = 1;
                op.copyBuffer.regions = &op.copyBufferRegion;
                // Streaming: accumulate to current transfer batch.
                if (w.openSeg != OpenSegment::TRANSFER) {
                    w.openSeg = OpenSegment::TRANSFER;
                }
                w.curTransferOps.push_back(BASE_NS::move(op));
                // Track dst buffer so SG can insert transfer→consumer dependency edge.
                AddTransferDstBuffer(wctxPtr, dstBuf->GetPlatformData().resource);
            }
            break;
        }
        case RenderCommandType::COPY_BUFFER_IMAGE: {
            const auto& cmd = *static_cast<const RenderCommandCopyBufferImage*>(ref.rc);
            if (cmd.copyType == RenderCommandCopyBufferImage::CopyType::UNDEFINED) {
                break;
            }

            const GpuBufferMln* gpuBuffer = nullptr;
            const GpuImageMln* gpuImage = nullptr;
            if (cmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) {
                gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.srcHandle);
                gpuImage = gpuResourceMgr_.GetImage<GpuImageMln>(cmd.dstHandle);
            } else {
                gpuImage = gpuResourceMgr_.GetImage<GpuImageMln>(cmd.srcHandle);
                gpuBuffer = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.dstHandle);
            }

            if (g_mlnLog.trans) {
                const RenderHandle imgHandle = (cmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE)
                                                   ? cmd.dstHandle
                                                   : cmd.srcHandle;
                const GpuImageDesc imgDesc = gpuImage ? gpuResourceMgr_.GetImageDescriptor(imgHandle) : GpuImageDesc{};
                const auto imgName = gpuResourceMgr_.GetName(imgHandle);
                const auto bufName = gpuResourceMgr_.GetName(
                    (cmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) ? cmd.srcHandle
                                                                                              : cmd.dstHandle);
                MLN_LOG_TRANS(
                    "Phase1 frame=%u: COPY_BUFFER_IMAGE type=%s buf=%p img=%p imgSize=%ux%u imgFmt=%u "
                    "bufOff=%u extent=%ux%u mip=%u layer=%u layerCount=%u aspect=0x%x "
                    "imgName='%s' bufName='%s'",
                    g_debugFrameCount,
                    (cmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) ? "B2I" : "I2B",
                    gpuBuffer ? reinterpret_cast<void*>(gpuBuffer->GetPlatformData().resource) : nullptr,
                    gpuImage ? reinterpret_cast<void*>(gpuImage->GetPlatformData().resource) : nullptr, imgDesc.width,
                    imgDesc.height, static_cast<uint32_t>(imgDesc.format), cmd.bufferImageCopy.bufferOffset,
                    cmd.bufferImageCopy.imageExtent.width, cmd.bufferImageCopy.imageExtent.height,
                    cmd.bufferImageCopy.imageSubresource.mipLevel, cmd.bufferImageCopy.imageSubresource.baseArrayLayer,
                    cmd.bufferImageCopy.imageSubresource.layerCount,
                    cmd.bufferImageCopy.imageSubresource.imageAspectFlags, imgName.c_str(), bufName.c_str());
                // Dump staging buffer content for B2I to verify texture data is non-zero
                if (cmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE && gpuBuffer &&
                    gpuBuffer->GetPlatformData().mappedData) {
                    const uint8_t* stagingData = static_cast<const uint8_t*>(gpuBuffer->GetPlatformData().mappedData) +
                                                 cmd.bufferImageCopy.bufferOffset;
                    const uint32_t texelBytes = cmd.bufferImageCopy.imageExtent.width *
                                                cmd.bufferImageCopy.imageExtent.height * 4; // assume RGBA8
                    // Check if staging data is all zeros
                    bool allZero = true;
                    const uint32_t checkBytes = (texelBytes < 64) ? texelBytes : 64;
                    for (uint32_t bi = 0; bi < checkBytes; ++bi) {
                        if (stagingData[bi] != 0) {
                            allZero = false;
                            break;
                        }
                    }
                    // Dump first 16 bytes as RGBA pixels
                    MLN_LOG_TRANS(
                        "Phase1 frame=%u: B2I-DATA: first 16 bytes=[%02x %02x %02x %02x  %02x %02x %02x %02x  "
                        "%02x %02x %02x %02x  %02x %02x %02x %02x] allZero=%d texelBytes=%u",
                        g_debugFrameCount, stagingData[0], stagingData[1], stagingData[2], stagingData[3],
                        stagingData[4], stagingData[5], stagingData[6], stagingData[7], stagingData[8], stagingData[9],
                        stagingData[10], stagingData[11], stagingData[12], stagingData[13], stagingData[14],
                        stagingData[15], allZero ? 1 : 0, texelBytes);
                }
            }
            if (gpuBuffer && gpuImage) {
                TransferOp op{};
                const auto& bic = cmd.bufferImageCopy;

                op.bufferImageRegion.extensionCount = 0;
                op.bufferImageRegion.extensions = nullptr;
                op.bufferImageRegion.bufferOrigin = static_cast<MlnDeviceSize>(bic.bufferOffset);
                op.bufferImageRegion.bufferRowLength = bic.bufferRowLength;
                op.bufferImageRegion.bufferImageHeight = bic.bufferImageHeight;
                op.bufferImageRegion.imageSubresource.aspectMask =
                    static_cast<MlnImageAspectFlags>(bic.imageSubresource.imageAspectFlags);
                op.bufferImageRegion.imageSubresource.mipLevel = bic.imageSubresource.mipLevel;
                op.bufferImageRegion.imageSubresource.baseArrayLayer = bic.imageSubresource.baseArrayLayer;
                // Expand GPU_IMAGE_ALL_LAYERS sentinel to actual layer count
                // (matches Vulkan: gpu_image_vk.cpp CopyBufferImage handling)
                op.bufferImageRegion.imageSubresource.layerCount =
                    (bic.imageSubresource.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                        ? gpuImage->GetDesc().layerCount
                        : bic.imageSubresource.layerCount;
                op.bufferImageRegion.imageOrigin = {static_cast<int32_t>(bic.imageOffset.width),
                    static_cast<int32_t>(bic.imageOffset.height), static_cast<int32_t>(bic.imageOffset.depth)};
                op.bufferImageRegion.imageSize = {bic.imageExtent.width, bic.imageExtent.height, bic.imageExtent.depth};

                if (cmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) {
                    op.type = MLN_TRANSFER_TYPE_COPY_BUFFER_TO_IMAGE;
                    op.copyBufferToImage.extensionCount = 0;
                    op.copyBufferToImage.extensions = nullptr;
                    op.copyBufferToImage.srcBufferResource = gpuBuffer->GetPlatformData().resource;
                    op.copyBufferToImage.dstImageResource = gpuImage->GetPlatformData().resource;
                    op.copyBufferToImage.regionCount = 1;
                    op.copyBufferToImage.regions = &op.bufferImageRegion;
                } else {
                    op.type = MLN_TRANSFER_TYPE_COPY_IMAGE_TO_BUFFER;
                    op.copyImageToBuffer.extensionCount = 0;
                    op.copyImageToBuffer.extensions = nullptr;
                    op.copyImageToBuffer.srcImageResource = gpuImage->GetPlatformData().resource;
                    op.copyImageToBuffer.dstBufferResource = gpuBuffer->GetPlatformData().resource;
                    op.copyImageToBuffer.regionCount = 1;
                    op.copyImageToBuffer.regions = &op.bufferImageRegion;
                }
                // Track destination resource for SG output declaration. Direction matters:
                //   B2I → dst is image → AddTransferDstImage
                //   I2B → dst is buffer → AddTransferDstBuffer (e.g. GPU readback paths)
                // Without per-direction tracking, downstream consumers of the dst resource
                // (compute reading the readback buffer, shader sampling the uploaded image)
                // miss the SG memory dependency edge.
                const auto& imgPlat = gpuImage->GetPlatformData();
                const auto& bufPlat = gpuBuffer->GetPlatformData();
                // Streaming: accumulate to current transfer batch.
                if (w.openSeg != OpenSegment::TRANSFER) {
                    w.openSeg = OpenSegment::TRANSFER;
                }
                if (cmd.copyType == RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE) {
                    AddTransferDstImage(wctxPtr, imgPlat.resource, imgPlat.resourceView);
                } else {
                    AddTransferDstBuffer(wctxPtr, bufPlat.resource);
                }
                w.curTransferOps.push_back(BASE_NS::move(op));
            }
            break;
        }
        case RenderCommandType::COPY_IMAGE: {
            const auto& cmd = *static_cast<const RenderCommandCopyImage*>(ref.rc);
            const auto* srcImg = gpuResourceMgr_.GetImage<GpuImageMln>(cmd.srcHandle);
            const auto* dstImg = gpuResourceMgr_.GetImage<GpuImageMln>(cmd.dstHandle);
            {
                const GpuImageDesc srcDesc = gpuResourceMgr_.GetImageDescriptor(cmd.srcHandle);
                const GpuImageDesc dstDesc = gpuResourceMgr_.GetImageDescriptor(cmd.dstHandle);
                if (g_mlnLog.graph) {
                    MLN_LOG_GRAPH("Phase1 frame=%u: COPY_IMAGE src(%ux%u,fmt=%u) -> dst(%ux%u,fmt=%u)",
                        g_debugFrameCount, srcDesc.width, srcDesc.height, static_cast<uint32_t>(srcDesc.format),
                        dstDesc.width, dstDesc.height, static_cast<uint32_t>(dstDesc.format));
                }
            }
            if (srcImg && dstImg) {
                TransferOp op{};
                op.type = MLN_TRANSFER_TYPE_COPY_IMAGE;

                const auto& ic = cmd.imageCopy;
                op.imageCopyRegion.extensionCount = 0;
                op.imageCopyRegion.extensions = nullptr;
                op.imageCopyRegion.srcSubresource.aspectMask =
                    static_cast<MlnImageAspectFlags>(ic.srcSubresource.imageAspectFlags);
                op.imageCopyRegion.srcSubresource.mipLevel = ic.srcSubresource.mipLevel;
                op.imageCopyRegion.srcSubresource.baseArrayLayer = ic.srcSubresource.baseArrayLayer;
                op.imageCopyRegion.srcSubresource.layerCount =
                    (ic.srcSubresource.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                        ? srcImg->GetDesc().layerCount
                        : ((ic.srcSubresource.layerCount == 0) ? 1 : ic.srcSubresource.layerCount);
                op.imageCopyRegion.srcOrigin = {ic.srcOffset.x, ic.srcOffset.y, ic.srcOffset.z};
                op.imageCopyRegion.dstSubresource.aspectMask =
                    static_cast<MlnImageAspectFlags>(ic.dstSubresource.imageAspectFlags);
                op.imageCopyRegion.dstSubresource.mipLevel = ic.dstSubresource.mipLevel;
                op.imageCopyRegion.dstSubresource.baseArrayLayer = ic.dstSubresource.baseArrayLayer;
                op.imageCopyRegion.dstSubresource.layerCount =
                    (ic.dstSubresource.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                        ? dstImg->GetDesc().layerCount
                        : ((ic.dstSubresource.layerCount == 0) ? 1 : ic.dstSubresource.layerCount);
                op.imageCopyRegion.dstOrigin = {ic.dstOffset.x, ic.dstOffset.y, ic.dstOffset.z};
                op.imageCopyRegion.imageSize = {ic.extent.width, ic.extent.height, ic.extent.depth};

                op.copyImage.extensionCount = 0;
                op.copyImage.extensions = nullptr;
                op.copyImage.srcImageResource = srcImg->GetPlatformData().resource;
                op.copyImage.dstImageResource = dstImg->GetPlatformData().resource;
                op.copyImage.regionCount = 1;
                op.copyImage.regions = &op.imageCopyRegion;

                // Track destination image for SG output resource declaration
                const auto& dstPlat = dstImg->GetPlatformData();
                // Streaming: accumulate to current transfer batch.
                if (w.openSeg != OpenSegment::TRANSFER) {
                    w.openSeg = OpenSegment::TRANSFER;
                }
                AddTransferDstImage(wctxPtr, dstPlat.resource, dstPlat.resourceView);
                w.curTransferOps.push_back(BASE_NS::move(op));
            }
            break;
        }
        case RenderCommandType::BLIT_IMAGE: {
            const auto& cmd = *static_cast<const RenderCommandBlitImage*>(ref.rc);
            const auto* srcImg = gpuResourceMgr_.GetImage<GpuImageMln>(cmd.srcHandle);
            const auto* dstImg = gpuResourceMgr_.GetImage<GpuImageMln>(cmd.dstHandle);
            {
                const GpuImageDesc srcDesc = gpuResourceMgr_.GetImageDescriptor(cmd.srcHandle);
                const GpuImageDesc dstDesc = gpuResourceMgr_.GetImageDescriptor(cmd.dstHandle);
                if (g_mlnLog.trans) {
                    MLN_LOG_TRANS(
                        "Phase1 frame=%u: BLIT_IMAGE src(%ux%u,fmt=%u) -> dst(%ux%u,fmt=%u), srcMln=%p, dstMln=%p",
                        g_debugFrameCount, srcDesc.width, srcDesc.height, static_cast<uint32_t>(srcDesc.format),
                        dstDesc.width, dstDesc.height, static_cast<uint32_t>(dstDesc.format),
                        srcImg ? reinterpret_cast<void*>(srcImg->GetPlatformData().resource) : nullptr,
                        dstImg ? reinterpret_cast<void*>(dstImg->GetPlatformData().resource) : nullptr);
                }
            }
            if (srcImg && dstImg) {
                TransferOp op{};
                op.type = MLN_TRANSFER_TYPE_BLIT;

                const auto& ib = cmd.imageBlit;
                op.blitRegion.extensionCount = 0;
                op.blitRegion.extensions = nullptr;
                op.blitRegion.srcSubresource.aspectMask =
                    static_cast<MlnImageAspectFlags>(ib.srcSubresource.imageAspectFlags);
                op.blitRegion.srcSubresource.mipLevel = ib.srcSubresource.mipLevel;
                op.blitRegion.srcSubresource.baseArrayLayer = ib.srcSubresource.baseArrayLayer;
                op.blitRegion.srcSubresource.layerCount =
                    (ib.srcSubresource.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                        ? srcImg->GetDesc().layerCount
                        : ((ib.srcSubresource.layerCount == 0) ? 1 : ib.srcSubresource.layerCount);
                op.blitRegion.srcOrigins[0] = {static_cast<int32_t>(ib.srcOffsets[0].width),
                    static_cast<int32_t>(ib.srcOffsets[0].height), static_cast<int32_t>(ib.srcOffsets[0].depth)};
                op.blitRegion.srcOrigins[1] = {static_cast<int32_t>(ib.srcOffsets[1].width),
                    static_cast<int32_t>(ib.srcOffsets[1].height), static_cast<int32_t>(ib.srcOffsets[1].depth)};
                op.blitRegion.dstSubresource.aspectMask =
                    static_cast<MlnImageAspectFlags>(ib.dstSubresource.imageAspectFlags);
                op.blitRegion.dstSubresource.mipLevel = ib.dstSubresource.mipLevel;
                op.blitRegion.dstSubresource.baseArrayLayer = ib.dstSubresource.baseArrayLayer;
                op.blitRegion.dstSubresource.layerCount =
                    (ib.dstSubresource.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                        ? dstImg->GetDesc().layerCount
                        : ((ib.dstSubresource.layerCount == 0) ? 1 : ib.dstSubresource.layerCount);
                op.blitRegion.dstOrigins[0] = {static_cast<int32_t>(ib.dstOffsets[0].width),
                    static_cast<int32_t>(ib.dstOffsets[0].height), static_cast<int32_t>(ib.dstOffsets[0].depth)};
                op.blitRegion.dstOrigins[1] = {static_cast<int32_t>(ib.dstOffsets[1].width),
                    static_cast<int32_t>(ib.dstOffsets[1].height), static_cast<int32_t>(ib.dstOffsets[1].depth)};

                op.blitImage.extensionCount = 0;
                op.blitImage.extensions = nullptr;
                op.blitImage.srcImageResource = srcImg->GetPlatformData().resource;
                op.blitImage.dstImageResource = dstImg->GetPlatformData().resource;
                op.blitImage.regionCount = 1;
                op.blitImage.regions = &op.blitRegion;
                op.blitImage.filter = ToMlnFilter(cmd.filter);

                // Track destination image for SG output resource declaration
                const auto& blitDstPlat = dstImg->GetPlatformData();
                // Streaming: accumulate to current transfer batch.
                if (w.openSeg != OpenSegment::TRANSFER) {
                    w.openSeg = OpenSegment::TRANSFER;
                }
                AddTransferDstImage(wctxPtr, blitDstPlat.resource, blitDstPlat.resourceView);
                w.curTransferOps.push_back(BASE_NS::move(op));
            }
            break;
        }
        case RenderCommandType::CLEAR_COLOR_IMAGE: {
            // CLEAR_COLOR_IMAGE: maps to MLN_TRANSFER_TYPE_CLEAR_IMAGE.
            // Each ImageSubresourceRange becomes one TransferOp (matches the
            // single-region-per-op pattern used by COPY/BLIT above).
            // Without this case, all CLEAR_IMG commands were silently dropped,
            // leaving cloud volume / SDF voxel grid textures uninitialized →
            // probabilistic first-frame splash on first compute read.
            const auto& cmd = *static_cast<const RenderCommandClearColorImage*>(ref.rc);
            const auto* dstImg = gpuResourceMgr_.GetImage<GpuImageMln>(cmd.handle);
            if (!dstImg || !dstImg->GetPlatformData().resource) {
                MLN_LOG_ERR("CLEAR_COLOR_IMAGE: image lookup FAILED handle=0x%llx",
                    static_cast<unsigned long long>(cmd.handle.id));
                break;
            }
            const auto& dstPlat = dstImg->GetPlatformData();
            const uint32_t imgLayerCount = dstImg->GetDesc().layerCount;
            for (const auto& range : cmd.ranges) {
                TransferOp op{};
                op.type = MLN_TRANSFER_TYPE_CLEAR_IMAGE;

                op.clearImageRegion.extensionCount = 0;
                op.clearImageRegion.extensions = nullptr;
                op.clearImageRegion.aspectMask = static_cast<MlnImageAspectFlags>(range.imageAspectFlags);
                op.clearImageRegion.baseMipLevel = range.baseMipLevel;
                op.clearImageRegion.levelCount = (range.levelCount == PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS)
                                                     ? dstImg->GetDesc().mipCount
                                                     : range.levelCount;
                op.clearImageRegion.baseArrayLayer = range.baseArrayLayer;
                op.clearImageRegion.layerCount = (range.layerCount == PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)
                                                     ? imgLayerCount
                                                     : range.layerCount;

                op.clearImage.extensionCount = 0;
                op.clearImage.extensions = nullptr;
                op.clearImage.imageResource = dstPlat.resource;
                // ClearColorValue is a union of float/int/uint arrays; copy the float repr,
                // Maleoon driver interprets it according to image format.
                op.clearImage.clearValue.color.f[0] = cmd.color.float32[0];
                op.clearImage.clearValue.color.f[1] = cmd.color.float32[1];
                op.clearImage.clearValue.color.f[2] = cmd.color.float32[2];
                op.clearImage.clearValue.color.f[3] = cmd.color.float32[3];
                op.clearImage.regionCount = 1;
                op.clearImage.regions = &op.clearImageRegion;

                if (w.openSeg != OpenSegment::TRANSFER)
                    w.openSeg = OpenSegment::TRANSFER;
                AddTransferDstImage(wctxPtr, dstPlat.resource, dstPlat.resourceView);
                w.curTransferOps.push_back(BASE_NS::move(op));
            }
            if (g_mlnLog.trans) {
                MLN_LOG_TRANS("Phase1 frame=%u: CLEAR_COLOR_IMAGE img=%p ranges=%zu color=(%g,%g,%g,%g)",
                    g_debugFrameCount, reinterpret_cast<void*>(dstPlat.resource), cmd.ranges.size(),
                    cmd.color.float32[0], cmd.color.float32[1], cmd.color.float32[2], cmd.color.float32[3]);
            }
            break;
        }
        default:
            break;
    }
}

// HandleExecuteBackendFrame — dispatches EXECUTE_BACKEND_FRAME_POSITION to backend command/node.

// The engine emits this command from two RenderCommandList APIs:
//   - SetExecuteBackendCommand(IRenderBackendCommand::Ptr)
//        cmd.command != nullptr → invoke the command callback object
//   - SetExecuteBackendFramePosition()  [legacy IRenderBackendNode pattern]
//        cmd.command == nullptr  → invoke the render node itself, where
//                                  wctx.backendNode = renderCommandCtx.renderBackendNode
//                                  (non-null only for CLASS_TYPE_BACKEND_NODE RNs)

// Both target methods have a const-state version (legacy, used by Vulkan backend) and
// a mutable-state overload (used by Maleoon for declarative DG push). C++ overload
// resolution dispatches based on argument const-ness; we pass a non-const recordingState
// so the mutable overload is selected.

// We invoke the mutable overload directly. The C++ default empty {} body in the base
// interface is a safe no-op for subclasses that did not override it — assuming all
// related subclass binaries are rebuilt against the current base interface (no ABI
// skew). Post-call we check whether any MlnDataGraph was pushed; if not, log an ERROR
// indicating the dispatch type so subclass authors can see they need to override the
// mutable overload to support Maleoon.
void RenderBackendMln::HandleExecuteBackendFrame(const void* refPtr, void* wctxPtr)
{
    const auto& ref = *static_cast<const RenderCommandWithType*>(refPtr);
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);
    const auto& cmd = *static_cast<const RenderCommandExecuteBackendFramePosition*>(ref.rc);

    // Preserve command-order correctness: pending transfer/graphics batches must
    // be flushed BEFORE the backend pushes its DGs.
    FlushTransferBatch(wctxPtr);
    FlushGraphicsBatch(wctxPtr);

    auto& outDGs = *static_cast<BASE_NS::vector<MlnDataGraph>*>(w.outDataGraphs);
    auto& outRes = *static_cast<BASE_NS::vector<DataGraphResourceInfo>*>(w.outDgResources);
    const size_t dgCountBefore = outDGs.size();

    // Build recording state. Typed pointers let the backend command/node
    // directly push to outDataGraphs / outDgResources.
    RenderBackendRecordingStateMln recordingState{};
    recordingState.mlnDevice = deviceMln_.GetMlnDevice();
    recordingState.outDataGraphs = &outDGs;
    recordingState.outDgResources = &outRes;

    ILowLevelDeviceMln& lowLevelDevice = static_cast<ILowLevelDeviceMln&>(device_.GetLowLevelDevice());

    // Tag the dispatch type so the post-call diagnostic can name the right thing.
    enum class DispatchType { Command, BackendNode, None };
    DispatchType dispatched = DispatchType::None;

    if (cmd.command) {
        MLN_LOG_FRAME(
            "EXECUTE_BACKEND_FRAME: invoking command callback (cmd.command=%p)", static_cast<const void*>(cmd.command));
        // Non-const arguments → overload resolution selects the mutable overload.
        cmd.command->ExecuteBackendCommand(lowLevelDevice, recordingState);
        dispatched = DispatchType::Command;
    } else if (w.backendNode) {
        MLN_LOG_FRAME("EXECUTE_BACKEND_FRAME: invoking backend node (backendNode=%p)", w.backendNode);
        auto* backendNode = static_cast<IRenderBackendNode*>(w.backendNode);
        backendNode->ExecuteBackendFrame(lowLevelDevice, recordingState);
        dispatched = DispatchType::BackendNode;
    } else {
        MLN_LOG_FRAME("EXECUTE_BACKEND_FRAME: cmd.command and backendNode both null — no-op");
        return;
    }

    // Post-call diagnostic: a properly adapted subclass should have pushed at least
    // one MlnDataGraph. If the count didn't change, the subclass either didn't
    // override the mutable overload (calling base default {}) or its override is a
    // no-op for Maleoon. Tell the user which dispatch type missed.
    const size_t dgCountAfter = outDGs.size();
    if (dgCountAfter == dgCountBefore) {
        if (dispatched == DispatchType::Command) {
            MLN_LOG_ERR(
                "EXECUTE_BACKEND_FRAME: IRenderBackendCommand subclass "
                "(cmd.command=%p) did not push any MlnDataGraph — likely missing "
                "an override of the mutable ExecuteBackendCommand overload "
                "(ILowLevelDevice&, RenderBackendRecordingState&). To support "
                "Maleoon, override the mutable overload, cast the args to the "
                "Maleoon-typed variants, and push DGs into "
                "recordingState.outDataGraphs.",
                static_cast<const void*>(cmd.command));
        } else if (dispatched == DispatchType::BackendNode) {
            MLN_LOG_ERR(
                "EXECUTE_BACKEND_FRAME: IRenderBackendNode subclass "
                "(backendNode=%p) did not push any MlnDataGraph — likely missing "
                "an override of the mutable ExecuteBackendFrame overload "
                "(const ILowLevelDevice&, RenderBackendRecordingState&). To support "
                "Maleoon, override the mutable overload, cast the args to the "
                "Maleoon-typed variants, and push DGs into "
                "recordingState.outDataGraphs.",
                w.backendNode);
        }
        return;
    }

    // Override produced DGs. Auto-sync outDgResources with backend-pushed DGs:
    // the backend may have pushed only to outDataGraphs (common case) or to both
    // (advanced). Pad outRes with defaults so Phase 3 SG build sees aligned sizes.
    if (outRes.size() < dgCountAfter) {
        const size_t missing = dgCountAfter - outRes.size();
        for (size_t i = 0; i < missing; ++i) {
            DataGraphResourceInfo info{}; // outputCount=0, srcStage=ALL_COMMANDS_BIT
            outRes.push_back(info);
        }
        MLN_LOG_GRAPH(
            "EXECUTE_BACKEND: auto-synced %zu DataGraphResourceInfo "
            "defaults for %zu backend-added DGs",
            missing, dgCountAfter - dgCountBefore);
    } else {
        MLN_LOG_FRAME(
            "EXECUTE_BACKEND: produced %zu DG(s); outDgResources already "
            "populated by backend",
            dgCountAfter - dgCountBefore);
    }
}

void RenderBackendMln::AddTransferDstImage(void* wctxPtr, MlnResource res, MlnResourceView view)
{
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);
    if (!res || w.curTransferDsts.size() >= MAX_DG_OUTPUT_RESOURCES) {
        return;
    }
    for (const auto& e : w.curTransferDsts) {
        if (e.resource == res)
            return;
    }
    w.curTransferDsts.push_back({res, view});
}

void RenderBackendMln::AddTransferDstBuffer(void* wctxPtr, MlnResource res)
{
    auto& w = *static_cast<PrimaryWalkerState*>(wctxPtr);
    if (!res) {
        return;
    }
    if (w.curTransferDstBuffers.size() >= MAX_DG_OUTPUT_RESOURCES) {
        MLN_LOG_TRANS("AddTransferDstBuffer: capacity full (%zu)", w.curTransferDstBuffers.size());
        return;
    }
    for (const auto& e : w.curTransferDstBuffers) {
        if (e == res)
            return;
    }
    w.curTransferDstBuffers.push_back(res);
}

// HandleStateCommand — processes BIND_*/PUSH_CONSTANT/DYNAMIC_STATE_* commands.
// Returns true if handled. Shared by WalkPrimaryCtx and WalkSecondaryCtx.
bool RenderBackendMln::HandleStateCommand(const void* refPtr, void* statePtr, void* currentRPPtr,
    uint32_t currentSubpassIndex, NodeContextPsoManager& psoMgr, NodeContextDescriptorSetManager& descriptorSetMgr)
{
    const auto& ref = *static_cast<const RenderCommandWithType*>(refPtr);
    auto& state = *static_cast<MlnRenderState*>(statePtr);
    auto* currentRP = static_cast<RenderPassSegment*>(currentRPPtr);

    switch (ref.type) {
        case RenderCommandType::BIND_PIPELINE: {
            const auto& cmd = *static_cast<const RenderCommandBindPipeline*>(ref.rc);
            state.graphicsPso = nullptr;
            state.computePso = nullptr;
            if (cmd.pipelineBindPoint == PipelineBindPoint::CORE_PIPELINE_BIND_POINT_GRAPHICS) {
                state.graphicsPsoHandle = cmd.psoHandle;
                state.hasGraphicsPsoHandle = true;
                if (currentRP && currentRP->beginCmd) {
                    if (!ResolveGraphicsPsoForCurrentRp(cmd.psoHandle, currentRP, currentSubpassIndex, state, psoMgr)) {

                        MLN_LOG_ERR(
                            "Phase1 frame=%u: BIND_PIPELINE graphics PSO lookup FAILED "
                            "(subpass=%u, psoHandle idx=%u, gen=%u)",
                            g_debugFrameCount, currentSubpassIndex, RenderHandleUtil::GetIndexPart(cmd.psoHandle),
                            RenderHandleUtil::GetGenerationIndexPart(cmd.psoHandle));
                    }
                } else {
                    if (g_mlnLog.graph) {
                        MLN_LOG_GRAPH(
                            "Phase1 frame=%u: BIND_PIPELINE graphics seen before BEGIN_RENDER_PASS, deferring PSO "
                            "lookup",
                            g_debugFrameCount);
                    }
                }
            } else {
                const auto* pso = psoMgr.GetComputePso(cmd.psoHandle, nullptr);
                if (pso) {
                    state.computePso = static_cast<const ComputePipelineStateObjectMln*>(pso);
                }
            }
            if (g_mlnLog.graph) {
                MLN_LOG_GRAPH("Phase1 frame=%u: BIND_PIPELINE type=%s program=%p psoHandle=(%u,%u)", g_debugFrameCount,
                    (cmd.pipelineBindPoint == PipelineBindPoint::CORE_PIPELINE_BIND_POINT_GRAPHICS) ? "GFX" : "COMPUTE",
                    (cmd.pipelineBindPoint == PipelineBindPoint::CORE_PIPELINE_BIND_POINT_GRAPHICS)
                        ? (state.graphicsPso ? reinterpret_cast<void*>(state.graphicsPso->GetPlatformData().program)
                                             : nullptr)
                        : (state.computePso ? reinterpret_cast<void*>(state.computePso->GetPlatformData().program)
                                            : nullptr),
                    RenderHandleUtil::GetIndexPart(cmd.psoHandle),
                    RenderHandleUtil::GetGenerationIndexPart(cmd.psoHandle));
            }
            break;
        }
        case RenderCommandType::BIND_VERTEX_BUFFERS: {
            const auto& cmd = *static_cast<const RenderCommandBindVertexBuffers*>(ref.rc);
            const uint32_t maxVertexBuffers = PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT;
            state.vertexBufferCount =
                (cmd.vertexBufferCount > maxVertexBuffers) ? maxVertexBuffers : cmd.vertexBufferCount;
            if (cmd.vertexBufferCount > maxVertexBuffers) {
                MLN_LOG_GRAPH(
                    "BIND_VERTEX_BUFFERS truncated: requested=%u, max=%u", cmd.vertexBufferCount, maxVertexBuffers);
            }
            for (uint32_t i = 0; i < state.vertexBufferCount; ++i) {
                const auto* buf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.vertexBuffers[i].bufferHandle);
                if (buf) {
                    const auto& plat = buf->GetPlatformData();
                    state.vertexBuffers[i] = plat.resource;
                    state.vertexBufferOffsets[i] = static_cast<MlnDeviceSize>(cmd.vertexBuffers[i].bufferOffset);
                    // BUG-22 FIX: native Maleoon driver may dereference sizes array.
                    // Size = full buffer capacity (bindMemoryByteSize), not remaining-from-offset.
                    // Offset is provided separately in resVB.offsets.
                    state.vertexBufferSizes[i] = static_cast<MlnDeviceSize>(plat.bindMemoryByteSize);
                } else {
                    MLN_LOG_GRAPH("RenderBackendMln: invalid vertex buffer handle at slot %u", i);
                    state.vertexBuffers[i] = MLN_NULL_HANDLE;
                    state.vertexBufferOffsets[i] = 0;
                    state.vertexBufferSizes[i] = 0;
                }
            }
            if (g_mlnLog.graph) {
                MLN_LOG_GRAPH(
                    "Phase1 frame=%u: BIND_VERTEX_BUFFERS count=%u", g_debugFrameCount, state.vertexBufferCount);
                for (uint32_t vi = 0; vi < state.vertexBufferCount; ++vi) {
                    MLN_LOG_GRAPH("  VB[%u]: resource=%p offset=%llu size=%llu", vi,
                        reinterpret_cast<void*>(state.vertexBuffers[vi]),
                        static_cast<unsigned long long>(state.vertexBufferOffsets[vi]),
                        static_cast<unsigned long long>(state.vertexBufferSizes[vi]));
                }
            }
            break;
        }
        case RenderCommandType::BIND_INDEX_BUFFER: {
            const auto& cmd = *static_cast<const RenderCommandBindIndexBuffer*>(ref.rc);
            const auto* buf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.indexBuffer.bufferHandle);
            if (buf) {
                state.indexBuffer = buf->GetPlatformData().resource;
                state.indexBufferOffset = static_cast<MlnDeviceSize>(cmd.indexBuffer.bufferOffset);
                state.indexType = (cmd.indexBuffer.indexType == IndexType::CORE_INDEX_TYPE_UINT16)
                                      ? MLN_INDEX_TYPE_UINT16
                                      : MLN_INDEX_TYPE_UINT32;
            }
            if (g_mlnLog.graph) {
                MLN_LOG_GRAPH("Phase1 frame=%u: BIND_INDEX_BUFFER buf=%p offset=%llu type=%u", g_debugFrameCount,
                    reinterpret_cast<void*>(state.indexBuffer),
                    static_cast<unsigned long long>(state.indexBufferOffset), static_cast<uint32_t>(state.indexType));
            }
            break;
        }
        case RenderCommandType::BIND_DESCRIPTOR_SETS: {
            const auto& cmd = *static_cast<const RenderCommandBindDescriptorSets*>(ref.rc);
            const uint32_t maxSetCount = PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT;
            const uint32_t requestedEndSet = cmd.firstSet + cmd.setCount;
            const uint32_t cmdEndSet = (requestedEndSet > maxSetCount) ? maxSetCount : requestedEndSet;
            if (cmd.firstSet >= maxSetCount) {
                if (cmd.setCount > 0) {
                    MLN_LOG_GRAPH("BIND_DESCRIPTOR_SETS ignored: firstSet=%u out of range", cmd.firstSet);
                }
                break;
            }
            // Extend tracked range to union of previous and current.
            // Multiple BIND_DESCRIPTOR_SETS calls accumulate (e.g. set 0 then sets 1-2).
            if (state.endSet == 0 && state.firstSet == 0) {
                // First bind call in this command list
                state.firstSet = cmd.firstSet;
                state.endSet = cmdEndSet;
            } else {
                if (cmd.firstSet < state.firstSet) {
                    state.firstSet = cmd.firstSet;
                }
                if (cmdEndSet > state.endSet) {
                    state.endSet = cmdEndSet;
                }
            }
            if (g_mlnLog.graph) {
                MLN_LOG_GRAPH(
                    "Phase1 frame=%u: BUG12FIX BIND_DS: cmd.firstSet=%u cmd.setCount=%u -> accumulated range [%u, %u)",
                    g_debugFrameCount, cmd.firstSet, cmd.setCount, state.firstSet, state.endSet);
            }
            // Resolve descriptor set handles to MlnBindingSet objects
            {
                auto& dsMgr = static_cast<NodeContextDescriptorSetManagerMln&>(descriptorSetMgr);
                for (uint32_t idx = cmd.firstSet; idx < cmdEndSet; ++idx) {
                    const RenderHandle dsHandle = cmd.descriptorSetHandles[idx];
                    if (RenderHandleUtil::GetHandleType(dsHandle) == RenderHandleType::DESCRIPTOR_SET) {
                        state.bindingSets[idx] = dsMgr.GetMlnBindingSet(dsHandle);
                        // If MlnBindingSet not yet created (lazy creation missed),
                        // force UpdateDescriptorSetGpuHandle to trigger creation + write.
                        if (!state.bindingSets[idx]) {
                            const bool isGlobalDS = (RenderHandleUtil::GetAdditionalData(dsHandle) &
                                                        NodeContextDescriptorSetManager::GLOBAL_DESCRIPTOR_BIT) != 0u;
                            if (isGlobalDS) {
                                static_cast<DescriptorSetManagerMln&>(device_.GetDescriptorSetManager())
                                    .UpdateDescriptorSetGpuHandle(dsHandle);
                            } else {
                                dsMgr.UpdateDescriptorSetGpuHandle(dsHandle);
                            }
                            state.bindingSets[idx] = dsMgr.GetMlnBindingSet(dsHandle);
                            if (state.bindingSets[idx]) {
                                const auto bindRes = isGlobalDS ? static_cast<DescriptorSetManagerMln&>(
                                                                      device_.GetDescriptorSetManager())
                                                                      .GetCpuDescriptorSetData(dsHandle)
                                                                : dsMgr.GetCpuDescriptorSetData(dsHandle);
                                WriteDescriptorSetBindings(state.bindingSets[idx], bindRes);
                            }
                        }

                        // [STORAGE-OUTPUTS] Scan this set for STORAGE_IMAGE / STORAGE_BUFFER
                        // bindings and stash them in state.perSetStorage[idx]. Used by
                        // BuildComputeDg to populate compute DG outputs so SG can declare
                        // proper compute → consumer memory dependencies.
                        // Conservative: any STORAGE_* binding is treated as a potential
                        // output (shader access flags are not visible at this layer).
                        {
                            auto& setStorage = state.perSetStorage[idx];
                            setStorage.entries.clear();
                            if (state.bindingSets[idx]) {
                                const auto bindRes = dsMgr.GetCpuDescriptorSetData(dsHandle);
                                for (const auto& rb : bindRes.buffers) {
                                    const auto& bRef = rb.desc;
                                    const auto dt = bRef.binding.descriptorType;
                                    const bool isStorageBuf =
                                        (dt == DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER) ||
                                        (dt == DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) ||
                                        (dt == DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC);
                                    if (!isStorageBuf || bRef.binding.descriptorCount == 0)
                                        continue;
                                    const uint32_t arrOff = bRef.arrayOffset;
                                    for (uint32_t ai = 0; ai < bRef.binding.descriptorCount; ++ai) {
                                        const auto& bRes =
                                            (ai == 0) ? bRef.resource : bindRes.buffers[arrOff + ai - 1].desc.resource;
                                        const auto* bufPtr = gpuResourceMgr_.GetBuffer<GpuBufferMln>(bRes.handle);
                                        if (!bufPtr || !bufPtr->GetPlatformData().resource)
                                            continue;
                                        MlnPassNodeResourceDescriptor rd{};
                                        rd.extensionCount = 0;
                                        rd.extensions = nullptr;
                                        rd.type = MLN_PASS_NODE_RESOURCE_TYPE_BUFFER;
                                        rd.imageResourceView = MLN_NULL_HANDLE;
                                        rd.bufferResource = bufPtr->GetPlatformData().resource;
                                        setStorage.entries.push_back(rd);
                                    }
                                }
                                for (const auto& ri : bindRes.images) {
                                    const auto& iRef = ri.desc;
                                    if (iRef.binding.descriptorType !=
                                        DescriptorType::CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE)
                                        continue;
                                    if (iRef.binding.descriptorCount == 0)
                                        continue;
                                    const uint32_t arrOff = iRef.arrayOffset;
                                    for (uint32_t ai = 0; ai < iRef.binding.descriptorCount; ++ai) {
                                        const auto& iRes =
                                            (ai == 0) ? iRef.resource : bindRes.images[arrOff + ai - 1].desc.resource;
                                        const auto* imgPtr = gpuResourceMgr_.GetImage<GpuImageMln>(iRes.handle);
                                        if (!imgPtr || !imgPtr->GetPlatformData().resource)
                                            continue;
                                        const auto& imgPlat = imgPtr->GetPlatformData();
                                        MlnPassNodeResourceDescriptor rd{};
                                        rd.extensionCount = 0;
                                        rd.extensions = nullptr;
                                        rd.type = MLN_PASS_NODE_RESOURCE_TYPE_IMAGE;
                                        rd.imageResourceView = imgPlat.resourceView;
                                        rd.bufferResource = imgPlat.resource;
                                        setStorage.entries.push_back(rd);
                                    }
                                }
                            }
                        }

                        // Fill BS debug cache from CPU descriptor data
                        // (only when graph logging enabled — zero overhead when off)
                        if (g_mlnLog.graph && state.bindingSets[idx]) {
                            const uintptr_t bsKey = reinterpret_cast<uintptr_t>(state.bindingSets[idx]);
                            if (bsDebugCache_.find(bsKey) == bsDebugCache_.end()) {
                                const auto bindRes = dsMgr.GetCpuDescriptorSetData(dsHandle);
                                BASE_NS::vector<BindingDebugEntry> entries;
                                // Buffer bindings (with array support)
                                for (const auto& rb : bindRes.buffers) {
                                    const auto& ref = rb.desc;
                                    if (ref.binding.descriptorCount == 0)
                                        continue;
                                    const uint32_t arrOff = ref.arrayOffset;
                                    for (uint32_t ai = 0; ai < ref.binding.descriptorCount; ++ai) {
                                        const auto& bRes =
                                            (ai == 0) ? ref.resource : bindRes.buffers[arrOff + ai - 1].desc.resource;
                                        const auto* bufPtr = gpuResourceMgr_.GetBuffer<GpuBufferMln>(bRes.handle);
                                        BindingDebugEntry e{};
                                        e.binding = ref.binding.binding;
                                        e.arrayIndex = ai;
                                        e.bindingType = static_cast<uint32_t>(ref.binding.descriptorType);
                                        if (bufPtr) {
                                            const auto& plat = bufPtr->GetPlatformData();
                                            const uint64_t optOff = static_cast<uint64_t>(bRes.byteOffset);
                                            e.bufResource = plat.resource;
                                            e.bufOffset = static_cast<uint64_t>(plat.currentByteOffset) + optOff;
                                            const uint64_t remaining = (plat.bindMemoryByteSize > optOff)
                                                                           ? plat.bindMemoryByteSize - optOff
                                                                           : 0;
                                            e.bufRange = (static_cast<uint64_t>(bRes.byteSize) < remaining)
                                                             ? bRes.byteSize
                                                             : remaining;
                                        }
                                        entries.push_back(e);
                                    }
                                }
                                // Image bindings (with array support)
                                for (const auto& ri : bindRes.images) {
                                    const auto& ref = ri.desc;
                                    if (ref.binding.descriptorCount == 0)
                                        continue;
                                    const uint32_t arrOff = ref.arrayOffset;
                                    for (uint32_t ai = 0; ai < ref.binding.descriptorCount; ++ai) {
                                        const auto& iRes =
                                            (ai == 0) ? ref.resource : bindRes.images[arrOff + ai - 1].desc.resource;
                                        const auto* imgPtr = gpuResourceMgr_.GetImage<GpuImageMln>(iRes.handle);
                                        BindingDebugEntry e{};
                                        e.binding = ref.binding.binding;
                                        e.arrayIndex = ai;
                                        e.bindingType = static_cast<uint32_t>(ref.binding.descriptorType);
                                        if (imgPtr) {
                                            e.imgView = imgPtr->GetPlatformData().resourceView;
                                            const auto imgD = gpuResourceMgr_.GetImageDescriptor(iRes.handle);
                                            e.imgWidth = imgD.width;
                                            e.imgHeight = imgD.height;
                                            e.imgFormat = static_cast<uint32_t>(imgD.format);
                                        }
                                        if (RenderHandleUtil::IsValid(iRes.samplerHandle)) {
                                            const auto* sp =
                                                gpuResourceMgr_.GetSampler<GpuSamplerMln>(iRes.samplerHandle);
                                            if (sp)
                                                e.imgSampler = sp->GetPlatformData().sampler;
                                        }
                                        entries.push_back(e);
                                    }
                                }
                                // Sampler bindings (with array support)
                                for (const auto& rs : bindRes.samplers) {
                                    const auto& ref = rs.desc;
                                    if (ref.binding.descriptorCount == 0)
                                        continue;
                                    const uint32_t arrOff = ref.arrayOffset;
                                    for (uint32_t ai = 0; ai < ref.binding.descriptorCount; ++ai) {
                                        const auto& sRes =
                                            (ai == 0) ? ref.resource : bindRes.samplers[arrOff + ai - 1].desc.resource;
                                        const auto* sp = gpuResourceMgr_.GetSampler<GpuSamplerMln>(sRes.handle);
                                        BindingDebugEntry e{};
                                        e.binding = ref.binding.binding;
                                        e.arrayIndex = ai;
                                        e.bindingType = static_cast<uint32_t>(MLN_BINDING_TYPE_SAMPLER);
                                        if (sp)
                                            e.sampler = sp->GetPlatformData().sampler;
                                        entries.push_back(e);
                                    }
                                }
                                bsDebugCache_[bsKey] = BASE_NS::move(entries);
                            }
                        }
                    } else {
                        state.bindingSets[idx] = MLN_NULL_HANDLE;
                    }

                    // Per-set dynamic offsets: iterate dynamic descriptor resources
                    // (matching Vulkan which uses GetDynamicOffsetDescriptors to validate
                    // each offset against the buffer's fullByteSize + currentByteOffset).
                    auto& setDyn = state.perSetDynOffsets[idx];
                    setDyn.count = 0;
                    const RenderHandle dsHandleForDyn = cmd.descriptorSetHandles[idx];
                    if (RenderHandleUtil::GetHandleType(dsHandleForDyn) == RenderHandleType::DESCRIPTOR_SET) {
                        const DynamicOffsetDescriptors dod = dsMgr.GetDynamicOffsetDescriptors(dsHandleForDyn);
                        const auto dodResCount = static_cast<uint32_t>(dod.resources.size());
                        const auto& dynRef = cmd.descriptorSetDynamicOffsets[idx];
                        if (dodResCount > 0) {
                            const uint32_t maxPerSet = PipelineLayoutConstants::MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT;
                            const uint32_t fillCount = (dodResCount > maxPerSet) ? maxPerSet : dodResCount;
                            for (uint32_t di = 0; di < fillCount; ++di) {
                                uint32_t byteOffset = 0u;
                                if (dynRef.dynamicOffsets && di < dynRef.dynamicOffsetCount) {
                                    byteOffset = dynRef.dynamicOffsets[di];
                                }
                                // Vulkan-style overflow validation: if dynamic offset + ring buffer
                                // frame offset exceeds total buffer size, clamp to 0.
                                const GpuBufferMln* dynBuf =
                                    gpuResourceMgr_.GetBuffer<const GpuBufferMln>(dod.resources[di]);
                                if (dynBuf) {
                                    const auto& bufData = dynBuf->GetPlatformData();
                                    if (bufData.fullByteSize < byteOffset + bufData.currentByteOffset) {
                                        if (g_mlnLog.graph) {
                                            MLN_LOG_GRAPH(
                                                "BIND_DS dynOffset overflow: set=%u di=%u "
                                                "byteOffset=%u + currentByteOffset=%u > fullByteSize=%u, "
                                                "clamping to 0",
                                                idx, di, byteOffset, bufData.currentByteOffset, bufData.fullByteSize);
                                        }
                                        byteOffset = 0u;
                                    }
                                }
                                setDyn.offsets[di] = byteOffset;
                            }
                            setDyn.count = fillCount;
                            if (dynRef.dynamicOffsetCount != dodResCount) {
                                MLN_LOG_GRAPH(
                                    "BIND_DS dynOffset count mismatch set=%u: "
                                    "layout expects %u, cmd provides %u (padded with 0)",
                                    idx, dodResCount, dynRef.dynamicOffsetCount);
                            }
                        }
                    }
                }
            }
            break;
        }
        case RenderCommandType::PUSH_CONSTANT: {
            const auto& cmd = *static_cast<const RenderCommandPushConstant*>(ref.rc);
            if (cmd.data && cmd.pushConstant.byteSize > 0) {
                const uint32_t offset = 0;
                const uint32_t size = cmd.pushConstant.byteSize;
                if (offset + size <= sizeof(state.pushConstantData)) {
                    if (memcpy_s(state.pushConstantData + offset, sizeof(state.pushConstantData) - offset, cmd.data,
                            size) == 0) {
                        state.pushConstantOffset = offset;
                        if (offset + size > state.pushConstantSize) {
                            state.pushConstantSize = offset + size;
                        }
                    }
                }
                if (g_mlnLog.graph) {
                    const float* pcf = reinterpret_cast<const float*>(cmd.data);
                    const uint32_t* pcu = reinterpret_cast<const uint32_t*>(cmd.data);
                    const uint32_t dwords = size / 4;
                    if (dwords >= 8) {
                        // PostProcessTonemapStruct or similar: dump as float + hex
                        MLN_LOG_GRAPH(
                            "Phase1 frame=%u: PUSH_CONSTANT: size=%u "
                            "f=[%.4f,%.4f,%.4f,%.4f, %.4f,%.4f,%.4f,%.4f] "
                            "u=[0x%08x,0x%08x,0x%08x,0x%08x, 0x%08x,0x%08x,0x%08x,0x%08x]",
                            g_debugFrameCount, size, pcf[0], pcf[1], pcf[2], pcf[3], pcf[4], pcf[5], pcf[6], pcf[7],
                            pcu[0], pcu[1], pcu[2], pcu[3], pcu[4], pcu[5], pcu[6], pcu[7]);
                    } else if (dwords >= 4) {
                        MLN_LOG_GRAPH(
                            "Phase1 frame=%u: PUSH_CONSTANT: size=%u f=[%.4f,%.4f,%.4f,%.4f] "
                            "u=[0x%08x,0x%08x,0x%08x,0x%08x]",
                            g_debugFrameCount, size, pcf[0], pcf[1], pcf[2], pcf[3], pcu[0], pcu[1], pcu[2], pcu[3]);
                    } else {
                        MLN_LOG_GRAPH(
                            "Phase1 frame=%u: PUSH_CONSTANT: size=%u u[0]=0x%08x", g_debugFrameCount, size, pcu[0]);
                    }
                }
            }
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_VIEWPORT: {
            const auto& cmd = *static_cast<const RenderCommandDynamicStateViewport*>(ref.rc);
            state.hasViewport = true;
            state.viewport.x = cmd.viewportDesc.x;
            state.viewport.y = cmd.viewportDesc.y;
            state.viewport.width = cmd.viewportDesc.width;
            state.viewport.height = cmd.viewportDesc.height;
            state.viewport.minDepth = cmd.viewportDesc.minDepth;
            state.viewport.maxDepth = cmd.viewportDesc.maxDepth;
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_SCISSOR: {
            const auto& cmd = *static_cast<const RenderCommandDynamicStateScissor*>(ref.rc);
            state.hasScissor = true;
            state.scissor.origin.x = static_cast<int32_t>(cmd.scissorDesc.offsetX);
            state.scissor.origin.y = static_cast<int32_t>(cmd.scissorDesc.offsetY);
            state.scissor.size.width = cmd.scissorDesc.extentWidth;
            state.scissor.size.height = cmd.scissorDesc.extentHeight;
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_LINE_WIDTH: {
            const auto& cmd = *static_cast<const RenderCommandDynamicStateLineWidth*>(ref.rc);
            state.hasLineWidth = true;
            state.lineWidth = cmd.lineWidth;
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_DEPTH_BIAS: {
            const auto& cmd = *static_cast<const RenderCommandDynamicStateDepthBias*>(ref.rc);
            state.hasDepthBias = true;
            state.depthBiasConstantFactor = cmd.depthBiasConstantFactor;
            state.depthBiasClamp = cmd.depthBiasClamp;
            state.depthBiasSlopeFactor = cmd.depthBiasSlopeFactor;
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_BLEND_CONSTANTS: {
            const auto& cmd = *static_cast<const RenderCommandDynamicStateBlendConstants*>(ref.rc);
            state.hasBlendConstants = true;
            state.blendConstants[0u] = cmd.blendConstants[0u];
            state.blendConstants[1u] = cmd.blendConstants[1u];
            state.blendConstants[2u] = cmd.blendConstants[2u];
            state.blendConstants[3u] = cmd.blendConstants[3u];
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_DEPTH_BOUNDS: {
            const auto& cmd = *static_cast<const RenderCommandDynamicStateDepthBounds*>(ref.rc);
            state.hasDepthBounds = true;
            state.minDepthBounds = cmd.minDepthBounds;
            state.maxDepthBounds = cmd.maxDepthBounds;
            break;
        }
        case RenderCommandType::DYNAMIC_STATE_STENCIL: {
            const auto& cmd = *static_cast<const RenderCommandDynamicStateStencil*>(ref.rc);
            state.hasStencilState = true;
            const bool front = (cmd.faceMask & CORE_STENCIL_FACE_FRONT_BIT) != 0;
            const bool back = (cmd.faceMask & CORE_STENCIL_FACE_BACK_BIT) != 0;
            switch (cmd.dynamicState) {
                case StencilDynamicState::COMPARE_MASK:
                    if (front) {
                        state.stencilFrontCompareMask = cmd.mask;
                    }
                    if (back) {
                        state.stencilBackCompareMask = cmd.mask;
                    }
                    break;
                case StencilDynamicState::WRITE_MASK:
                    if (front) {
                        state.stencilFrontWriteMask = cmd.mask;
                    }
                    if (back) {
                        state.stencilBackWriteMask = cmd.mask;
                    }
                    break;
                case StencilDynamicState::REFERENCE:
                    if (front) {
                        state.stencilFrontReference = cmd.mask;
                    }
                    if (back) {
                        state.stencilBackReference = cmd.mask;
                    }
                    break;
                default:
                    break;
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

// BuildOGsFromDrawGroups — marshals DrawCallGroup → Mln*Descriptors → BuildOGFromState.
// Used by 3-phase path (Phase 2) and streaming lazy-merge of secondary ctx drawGroups.
void RenderBackendMln::BuildOGsFromDrawGroups(const void* rpSegPtr, uint32_t activeSubpassIndex,
    PendingDestroyFrame& pendingFrame, vector<MlnObjectGroup>& outObjectGroups)
{
    auto& rpSeg = *static_cast<const RenderPassSegment*>(rpSegPtr);
    if (!rpSeg.beginCmd) {
        return;
    }
    const auto& rpDesc = rpSeg.beginCmd->renderPassDesc;

    for (auto& dg : rpSeg.drawGroups) {
        if (dg.subpassIndex != activeSubpassIndex) {
            MLN_LOG_ERR("Skipping draw from subpass %u while building subpass %u", dg.subpassIndex, activeSubpassIndex);
            continue;
        }
        BuildSingleOGFromDrawGroup(&dg, &rpDesc, pendingFrame, outObjectGroups);
    }
}

// Extracted per-DG body for reuse by both BuildOGsFromDrawGroups (batch path)
// and BuildOGFromDraw*Command (streaming/secondary direct-build path, stack-only).
void RenderBackendMln::BuildSingleOGFromDrawGroup(const void* dgPtr, const void* rpDescPtr,
    PendingDestroyFrame& pendingFrame, vector<MlnObjectGroup>& outObjectGroups)
{
    const auto& dg = *static_cast<const DrawCallGroup*>(dgPtr);
    const auto& rpDesc = *static_cast<const RenderPassDesc*>(rpDescPtr);
    {
        if (!dg.program) {
            return;
        }
        bool hasNullVB = false;
        for (uint32_t vi = 0; vi < dg.vertexBufferCount; ++vi) {
            if (!dg.vertexBuffers[vi]) {
                MLN_LOG_ERR("DrawGroup has null vertex buffer at slot %u - skipping draw", vi);
                hasNullVB = true;
                break;
            }
        }
        if (hasNullVB) {
            return;
        }

        MlnDeviceSize vbStrides[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT]{};
        MlnResourceVertexBuffers resVB{};
        resVB.firstBinding = 0;
        resVB.bindingCount = dg.vertexBufferCount;
        resVB.bufferResources = dg.vertexBuffers;
        resVB.offsets = dg.vertexBufferOffsets;
        resVB.sizes = dg.vertexBufferSizes;
        resVB.strides = vbStrides;

        MlnResourceIndexBuffer resIB{};
        resIB.bufferResource = dg.indexBuffer;
        resIB.offset = dg.indexBufferOffset;
        resIB.size = 0;
        resIB.indexType = dg.indexType;

        const uint32_t maxSetCount = PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT;
        const uint32_t drawSetStart = dg.firstSet;
        const uint32_t requestedDrawSetEnd = drawSetStart + dg.bindingSetCount;
        const uint32_t drawSetEnd = (requestedDrawSetEnd > maxSetCount) ? maxSetCount : requestedDrawSetEnd;
        bool hasInvalidBindingSet = (requestedDrawSetEnd > maxSetCount) || (drawSetStart >= maxSetCount);

        MlnBindingSet paddedBindingSets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT]{};
        bool usePaddedBS = false;
        if (!hasInvalidBindingSet && drawSetEnd > drawSetStart) {
            EnsureEmptyBindingSet();
            for (uint32_t i = 0; i < drawSetStart; ++i) {
                paddedBindingSets[i] = emptyBindingSet_;
                usePaddedBS = true;
            }
            for (uint32_t i = drawSetStart; i < drawSetEnd; ++i) {
                if (dg.bindingSets[i]) {
                    paddedBindingSets[i] = dg.bindingSets[i];
                } else if (emptyBindingSet_) {
                    paddedBindingSets[i] = emptyBindingSet_;
                    usePaddedBS = true;
                } else {
                    hasInvalidBindingSet = true;
                    break;
                }
            }
        }
        MlnResourceBindingSets resBS{};
        if (!hasInvalidBindingSet && usePaddedBS) {
            resBS.firstSet = 0;
            resBS.bindingSetCount = drawSetEnd;
            resBS.bindingSets = paddedBindingSets;
        } else if (!hasInvalidBindingSet && drawSetEnd > drawSetStart) {
            resBS.firstSet = drawSetStart;
            resBS.bindingSetCount = drawSetEnd - drawSetStart;
            resBS.bindingSets = &dg.bindingSets[drawSetStart];
        } else {
            resBS.firstSet = drawSetStart;
            resBS.bindingSetCount = 0;
            resBS.bindingSets = nullptr;
        }
        resBS.dynamicOffsetCount = (resBS.bindingSetCount > 0) ? dg.bindingSetDynamicOffsetCount : 0;
        resBS.dynamicOffsets = (resBS.dynamicOffsetCount > 0) ? dg.bindingSetDynamicOffsets : nullptr;

        MlnProgramConstants pc{};
        pc.offset = 0;
        pc.size = dg.pushConstantSize;
        pc.values = dg.pushConstantData;
        MlnResourceProgramConstants resPC{};
        resPC.programConstantCount = (dg.pushConstantSize > 0) ? 1 : 0;
        resPC.programConstants = (dg.pushConstantSize > 0) ? &pc : nullptr;

        MlnDynamicResourceType dynResTypes[4];
        uint32_t dynResCount = 0;
        if (dg.vertexBufferCount > 0)
            dynResTypes[dynResCount++] = MLN_DYNAMIC_RESOURCE_TYPE_VERTEX_BUFFERS;
        if (dg.indexBuffer)
            dynResTypes[dynResCount++] = MLN_DYNAMIC_RESOURCE_TYPE_INDEX_BUFFERS;
        if (resBS.bindingSetCount > 0)
            dynResTypes[dynResCount++] = MLN_DYNAMIC_RESOURCE_TYPE_BINDING_SETS;
        if (dg.pushConstantSize > 0)
            dynResTypes[dynResCount++] = MLN_DYNAMIC_RESOURCE_TYPE_PROGRAM_CONSTANTS;

        MlnResourceSetDescriptor resourceSet{};
        resourceSet.dynamicResourceCount = dynResCount;
        resourceSet.dynamicResource = (dynResCount > 0) ? dynResTypes : nullptr;
        resourceSet.resourceVertexBuffers = (dg.vertexBufferCount > 0) ? &resVB : nullptr;
        resourceSet.resourceIndexBuffer = dg.indexBuffer ? &resIB : nullptr;
        resourceSet.resourceBindingSets = (resBS.bindingSetCount > 0) ? &resBS : nullptr;
        resourceSet.resourceProgramConstants = (dg.pushConstantSize > 0) ? &resPC : nullptr;
        MlnResourceSetDescriptor* resourceSetPtr = &resourceSet;

        MlnGraphicsCommand actionCmd{};
        if (dg.isIndirect) {
            actionCmd.type = MLN_GRAPHICS_COMMAND_TYPE_DRAW_INDIRECT;
            actionCmd.data.drawIndirect = &dg.drawIndirectCmd;
        } else if (dg.isIndexed) {
            actionCmd.type = MLN_GRAPHICS_COMMAND_TYPE_DRAW_INDEXED;
            actionCmd.data.drawIndexed = &dg.drawIndexedCmd;
        } else {
            actionCmd.type = MLN_GRAPHICS_COMMAND_TYPE_DRAW;
            actionCmd.data.draw = &dg.drawCmd;
        }

        MlnGraphicsCommandDescriptor graphicsCmd{};
        graphicsCmd.extensionCount = 0;
        graphicsCmd.extensions = nullptr;
        graphicsCmd.flags = 0;
        graphicsCmd.command = &actionCmd;
        graphicsCmd.resourceSet = nullptr;
        graphicsCmd.stateSet = nullptr;

        MlnViewportDynamicState vpState{};
        MlnScissorDynamicState scState{};
        MlnDepthBiasDynamicState depthBiasState{};
        MlnDepthBoundsDynamicState depthBoundsState{};
        MlnStencilCompareMaskDynamicState stencilCompareMask{};
        MlnStencilWriteMaskDynamicState stencilWriteMask{};
        MlnStencilReferenceDynamicState stencilReference{};
        MlnViewport fallbackViewport{};
        MlnRect2D fallbackScissor{};
        MlnStateSetDescriptor stateSet{};
        MlnDynamicState dynState{};
        dynState.dynamicStateCount = dg.psoPlat.dynamicStateCount;
        dynState.dynamicStateTypes = dg.psoPlat.dynamicStateTypes;
        stateSet.dynamicState = &dynState;

        vpState.firstViewport = 0;
        vpState.viewportCount = 1;
        if (dg.hasViewport) {
            vpState.viewports = &dg.viewport;
        } else {
            fallbackViewport = {0.0f, 0.0f, static_cast<float>(rpDesc.renderArea.extentWidth),
                static_cast<float>(rpDesc.renderArea.extentHeight), 0.0f, 1.0f};
            vpState.viewports = &fallbackViewport;
        }
        stateSet.viewport = &vpState;

        scState.firstScissor = 0;
        scState.scissorCount = 1;
        if (dg.hasScissor) {
            scState.scissors = &dg.scissor;
        } else {
            fallbackScissor.origin = {0, 0};
            fallbackScissor.size = {rpDesc.renderArea.extentWidth, rpDesc.renderArea.extentHeight};
            scState.scissors = &fallbackScissor;
        }
        stateSet.scissor = &scState;

        if (dg.hasDepthBias) {
            depthBiasState.depthBiasConstantFactor = dg.depthBiasConstantFactor;
            depthBiasState.depthBiasClamp = dg.depthBiasClamp;
            depthBiasState.depthBiasSlopeFactor = dg.depthBiasSlopeFactor;
            stateSet.depthBias = &depthBiasState;
        }
        if (dg.hasBlendConstants) {
            stateSet.blendConstants[0] = dg.blendConstants[0];
            stateSet.blendConstants[1] = dg.blendConstants[1];
            stateSet.blendConstants[2] = dg.blendConstants[2];
            stateSet.blendConstants[3] = dg.blendConstants[3];
        }
        if (dg.hasDepthBounds) {
            depthBoundsState.minDepthBounds = dg.minDepthBounds;
            depthBoundsState.maxDepthBounds = dg.maxDepthBounds;
            stateSet.depthBounds = &depthBoundsState;
        }
        if (dg.hasStencilState) {
            stencilCompareMask.faceMask = static_cast<MlnStencilFaceFlags>(CORE_STENCIL_FRONT_AND_BACK);
            stencilCompareMask.compareMask = dg.stencilFrontCompareMask;
            stateSet.compareMask = &stencilCompareMask;
            stencilWriteMask.faceMask = static_cast<MlnStencilFaceFlags>(CORE_STENCIL_FRONT_AND_BACK);
            stencilWriteMask.writeMask = dg.stencilFrontWriteMask;
            stateSet.writeMask = &stencilWriteMask;
            stencilReference.faceMask = static_cast<MlnStencilFaceFlags>(CORE_STENCIL_FRONT_AND_BACK);
            stencilReference.reference = dg.stencilFrontReference;
            stateSet.reference = &stencilReference;
        }
        graphicsCmd.resourceSet = resourceSetPtr;
        graphicsCmd.stateSet = &stateSet;

        BuildOGInputs ogInputs{};
        ogInputs.dg = &dg;
        ogInputs.resourceSetPtr = resourceSetPtr;
        ogInputs.stateSet = &stateSet;
        ogInputs.graphicsCmd = &graphicsCmd;
        ogInputs.resVB = &resVB;
        ogInputs.resIB = &resIB;
        ogInputs.resBS = &resBS;
        ogInputs.resPC = &resPC;
        ogInputs.vpState = &vpState;
        ogInputs.scState = &scState;
        ogInputs.depthBiasState = &depthBiasState;
        ogInputs.depthBoundsState = &depthBoundsState;
        ogInputs.stencilCompareMask = &stencilCompareMask;
        ogInputs.stencilWriteMask = &stencilWriteMask;
        ogInputs.stencilReference = &stencilReference;
        BuildOGFromState(&ogInputs, pendingFrame, outObjectGroups);
    }
}

// [OPT] Build single OG directly from state + DRAW command (stack-only DrawCallGroup,
// no vector heap alloc). Called by WalkSecondaryCtx and HandleDraw when
// g_mlnLog.ogDirectBuild == true. Returns MLN_NULL_HANDLE on failure.
// Opaque void* signature because MlnRenderState / RenderPassSegment are anonymous-
// namespace types in this .cpp (same pattern as RenderSingleCommandList).
MlnObjectGroup RenderBackendMln::BuildOGFromDrawCommand(const void* statePtr, const void* cmdPtr, const void* rpSegPtr,
    uint32_t subpassIndex, PendingDestroyFrame& pendingFrame)
{
    const auto& state = *static_cast<const MlnRenderState*>(statePtr);
    const auto& cmd = *static_cast<const RenderCommandDraw*>(cmdPtr);
    const auto& rpSeg = *static_cast<const RenderPassSegment*>(rpSegPtr);

    if (!state.graphicsPso || !rpSeg.beginCmd)
        return MLN_NULL_HANDLE;
    const auto& psoPlat = state.graphicsPso->GetPlatformData();
    if (!psoPlat.program)
        return MLN_NULL_HANDLE;

    // Stack-only DrawCallGroup — zero heap alloc per draw.
    DrawCallGroup dg{};
    dg.psoPlat = psoPlat;
    dg.program = psoPlat.program;
    dg.subpassIndex = subpassIndex;
    SnapshotStateToDrawGroup(dg, state);
    if (cmd.drawType == DrawType::DRAW_INDEXED) {
        dg.isIndexed = true;
        dg.drawIndexedCmd.indexCount = cmd.indexCount;
        dg.drawIndexedCmd.instanceCount = cmd.instanceCount;
        dg.drawIndexedCmd.firstIndex = cmd.firstIndex;
        dg.drawIndexedCmd.vertexOffset = cmd.vertexOffset;
        dg.drawIndexedCmd.firstInstance = cmd.firstInstance;
    } else {
        dg.isIndexed = false;
        dg.drawCmd.vertexCount = cmd.vertexCount;
        dg.drawCmd.instanceCount = cmd.instanceCount;
        dg.drawCmd.firstVertex = cmd.firstVertex;
        dg.drawCmd.firstInstance = cmd.firstInstance;
    }

    vector<MlnObjectGroup> outOGs;
    outOGs.reserve(1);
    BuildSingleOGFromDrawGroup(&dg, &rpSeg.beginCmd->renderPassDesc, pendingFrame, outOGs);
    return outOGs.empty() ? MLN_NULL_HANDLE : outOGs[0];
}

MlnObjectGroup RenderBackendMln::BuildOGFromDrawIndirectCommand(const void* statePtr, const void* cmdPtr,
    const void* rpSegPtr, uint32_t subpassIndex, PendingDestroyFrame& pendingFrame)
{
    const auto& state = *static_cast<const MlnRenderState*>(statePtr);
    const auto& cmd = *static_cast<const RenderCommandDrawIndirect*>(cmdPtr);
    const auto& rpSeg = *static_cast<const RenderPassSegment*>(rpSegPtr);

    if (!state.graphicsPso || !rpSeg.beginCmd)
        return MLN_NULL_HANDLE;
    const auto* buf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.argsHandle);
    if (!buf)
        return MLN_NULL_HANDLE;
    const auto& psoPlat = state.graphicsPso->GetPlatformData();
    if (!psoPlat.program)
        return MLN_NULL_HANDLE;

    DrawCallGroup dg{};
    dg.psoPlat = psoPlat;
    dg.program = psoPlat.program;
    dg.subpassIndex = subpassIndex;
    SnapshotStateToDrawGroup(dg, state);
    dg.isIndirect = true;
    dg.drawIndirectCmd.bufferResource = buf->GetPlatformData().resource;
    dg.drawIndirectCmd.offset = static_cast<MlnDeviceSize>(cmd.offset) + buf->GetPlatformData().currentByteOffset;
    dg.drawIndirectCmd.drawCount = cmd.drawCount;
    dg.drawIndirectCmd.stride = cmd.stride;

    vector<MlnObjectGroup> outOGs;
    outOGs.reserve(1);
    BuildSingleOGFromDrawGroup(&dg, &rpSeg.beginCmd->renderPassDesc, pendingFrame, outOGs);
    return outOGs.empty() ? MLN_NULL_HANDLE : outOGs[0];
}

// BuildGraphicsDg — builds one graphics DataGraph from pre-built OGs + render pass descriptor.
void RenderBackendMln::BuildGraphicsDg(const void* beginCmdPtr, uint32_t activeSubpassIndex,
    const vector<MlnObjectGroup>& objectGroups, uint32_t rpSegIdx, PendingDestroyFrame& pendingFrame,
    vector<MlnDataGraph>& dataGraphs, vector<DataGraphResourceInfo>& dgResources)
{
    const auto* beginCmd = static_cast<const RenderCommandBeginRenderPass*>(beginCmdPtr);
    if (!beginCmd) {
        return;
    }

    const MlnDevice mlnDevice = deviceMln_.GetMlnDevice();
    const auto& rpDesc = beginCmd->renderPassDesc;
    const auto& subpasses = beginCmd->subpasses;
    MLN_LOG_GRAPH("Phase2 frame=%u: rpSeg[%u] area=%ux%u OGs=%zu subpasses=%zu attCnt=%u", g_debugFrameCount, rpSegIdx,
        rpDesc.renderArea.extentWidth, rpDesc.renderArea.extentHeight, objectGroups.size(), subpasses.size(),
        rpDesc.attachmentCount);
    if (subpasses.empty()) {
        return;
    }
    const bool isSubpassBegin = (beginCmd->beginType == RenderPassBeginType::RENDER_PASS_SUBPASS_BEGIN);

    if (activeSubpassIndex >= subpasses.size()) {
        activeSubpassIndex = 0;
    }

    const auto& sp = subpasses[activeSubpassIndex];

    // [MILESTONE-1] rpSeg entry — if crash after this but before MILESTONE-2, it's in attachment setup
    MLN_LOG_FRAME(
        "MILESTONE-1 frame=%u rpSeg[%u]: subpass=%u colorAtt=%u depthAttIdx=%u "
        "OGs=%zu area=%ux%u attCnt=%u isSubpassBegin=%d",
        g_debugFrameCount, rpSegIdx, activeSubpassIndex, sp.colorAttachmentCount, sp.depthAttachmentIndex,
        objectGroups.size(), rpDesc.renderArea.extentWidth, rpDesc.renderArea.extentHeight, rpDesc.attachmentCount,
        isSubpassBegin ? 1 : 0);

    // Log subpass structure for debugging
    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH(
            "Phase2 frame=%u: subpass[%u]: colorAttCount=%u, depthAttIdx=%u, depthAttCount=%u, inputAttCount=%u",
            g_debugFrameCount, activeSubpassIndex, sp.colorAttachmentCount, sp.depthAttachmentIndex,
            sp.depthAttachmentCount, sp.inputAttachmentCount);
        for (uint32_t ci = 0; ci < sp.colorAttachmentCount; ++ci) {
            const uint32_t cAttIdx = sp.colorAttachmentIndices[ci];
            if (cAttIdx < rpDesc.attachmentCount) {
                const GpuImageDesc cImgDesc = gpuResourceMgr_.GetImageDescriptor(rpDesc.attachmentHandles[cAttIdx]);
                MLN_LOG_GRAPH("Phase2 frame=%u:   color[%u]: attachIdx=%u, fmt=%u, %ux%u", g_debugFrameCount, ci,
                    cAttIdx, static_cast<uint32_t>(cImgDesc.format), cImgDesc.width, cImgDesc.height);
            }
        }
    }

    // Build color attachment descriptors
    vector<MlnAttachmentDescriptor> colorAttachments(sp.colorAttachmentCount);
    bool hasNullColorAttachment = false;
    for (uint32_t i = 0; i < sp.colorAttachmentCount; ++i) {
        auto& att = colorAttachments[i];
        att.extensionCount = 0;
        att.extensions = nullptr;
        att.resolveMode = static_cast<MlnResolveModeFlagBits>(0);
        att.resolveResourceView = MLN_NULL_HANDLE;
        att.resolveImageLayout = MLN_IMAGE_LAYOUT_UNDEFINED;

        const uint32_t attIdx = sp.colorAttachmentIndices[i];
        if (attIdx < rpDesc.attachmentCount) {
            const auto* img = gpuResourceMgr_.GetImage<GpuImageMln>(rpDesc.attachmentHandles[attIdx]);
            if (img) {
                const auto& attDesc = rpDesc.attachments[attIdx];
                const auto& platViews = img->GetPlatformDataViews();
                // Select view matching VK backend (node_context_pool_manager_vk.cpp:176-195):
                // default=imageViewBase, multiview→allLayerViews, mip≥1→mipViews, layer≥1→layerViews
                MlnResourceView selectedView = img->GetPlatformData().resourceViewBase
                                                   ? img->GetPlatformData().resourceViewBase
                                                   : img->GetPlatformData().resourceView;
                // TODO: multiview path (viewMask > 1) → mipImageAllLayerViews[mipLevel]
                if ((attDesc.mipLevel >= 1) &&
                    (attDesc.mipLevel < static_cast<uint32_t>(platViews.mipImageViews.size()))) {
                    selectedView = platViews.mipImageViews[attDesc.mipLevel];
                } else if ((attDesc.layer >= 1) &&
                           (attDesc.layer < static_cast<uint32_t>(platViews.layerImageViews.size()))) {
                    selectedView = platViews.layerImageViews[attDesc.layer];
                }
                att.imageResourceView = selectedView;
                if (!att.imageResourceView) {
                    MLN_LOG_ERR("Color attachment %u has null imageResourceView", i);
                    hasNullColorAttachment = true;
                }
            } else {
                MLN_LOG_GRAPH("Color attachment %u: GpuImageMln not found", i);
                att.imageResourceView = MLN_NULL_HANDLE;
                hasNullColorAttachment = true;
            }
            att.imageLayout = MLN_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            // For stitched subpasses (render graph split into multiple command lists),
            // preserve previous subpass contents instead of replaying original loadOp.
            att.loadOp = isSubpassBegin ? MLN_ATTACHMENT_LOAD_OP_LOAD : ToMlnLoadOp(rpDesc.attachments[attIdx].loadOp);
            att.storeOp = ToMlnStoreOp(rpDesc.attachments[attIdx].storeOp);
            const auto& cc = rpDesc.attachments[attIdx].clearValue.color;
            att.clearValue.color.f[0] = cc.float32[0];
            att.clearValue.color.f[1] = cc.float32[1];
            att.clearValue.color.f[2] = cc.float32[2];
            att.clearValue.color.f[3] = cc.float32[3];

            // Wire resolve attachment if present (MSAA resolve)
            if (i < sp.resolveAttachmentCount) {
                const uint32_t resolveIdx = sp.resolveAttachmentIndices[i];
                if (resolveIdx < rpDesc.attachmentCount && resolveIdx != ~0u) {
                    const auto* resolveImg =
                        gpuResourceMgr_.GetImage<GpuImageMln>(rpDesc.attachmentHandles[resolveIdx]);
                    if (resolveImg) {
                        att.resolveMode = MLN_RESOLVE_MODE_AVERAGE_BIT;
                        att.resolveResourceView = resolveImg->GetPlatformData().resourceViewBase
                                                      ? resolveImg->GetPlatformData().resourceViewBase
                                                      : resolveImg->GetPlatformData().resourceView;
                        att.resolveImageLayout = MLN_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                    }
                }
            }

            if (g_mlnLog.graph) {
                const GpuImageDesc cImgDesc2 = gpuResourceMgr_.GetImageDescriptor(rpDesc.attachmentHandles[attIdx]);
                MLN_LOG_GRAPH(
                    "Phase2 frame=%u: color[%u] rpAttIdx=%u fmt=%u %ux%u clear=(%.3f,%.3f,%.3f,%.3f) loadOp=%u "
                    "storeOp=%u imgRes=%p resolve=%p",
                    g_debugFrameCount, i, attIdx, static_cast<uint32_t>(cImgDesc2.format), cImgDesc2.width,
                    cImgDesc2.height, att.clearValue.color.f[0], att.clearValue.color.f[1], att.clearValue.color.f[2],
                    att.clearValue.color.f[3], static_cast<uint32_t>(att.loadOp), static_cast<uint32_t>(att.storeOp),
                    reinterpret_cast<void*>(att.imageResourceView), reinterpret_cast<void*>(att.resolveResourceView));
            }
        }
    }

    if (hasNullColorAttachment) {
        MLN_LOG_ERR("Skipping render pass with null color attachment(s)");
        return;
    }

    // Build depth attachment
    MlnAttachmentDescriptor depthAttachment{};
    depthAttachment.extensionCount = 0;
    depthAttachment.extensions = nullptr;
    depthAttachment.resolveMode = static_cast<MlnResolveModeFlagBits>(0);
    depthAttachment.resolveResourceView = MLN_NULL_HANDLE;
    depthAttachment.resolveImageLayout = MLN_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.imageResourceView = MLN_NULL_HANDLE;
    depthAttachment.imageLayout = MLN_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Build stencil attachment (separate from depth when format has stencil)
    MlnAttachmentDescriptor stencilAttachment{};
    stencilAttachment.extensionCount = 0;
    stencilAttachment.extensions = nullptr;
    stencilAttachment.resolveMode = static_cast<MlnResolveModeFlagBits>(0);
    stencilAttachment.resolveResourceView = MLN_NULL_HANDLE;
    stencilAttachment.resolveImageLayout = MLN_IMAGE_LAYOUT_UNDEFINED;
    stencilAttachment.imageResourceView = MLN_NULL_HANDLE;
    stencilAttachment.imageLayout = MLN_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    bool hasDepth = false;
    bool hasStencil = false;
    if (sp.depthAttachmentIndex < rpDesc.attachmentCount) {
        const auto* img = gpuResourceMgr_.GetImage<GpuImageMln>(rpDesc.attachmentHandles[sp.depthAttachmentIndex]);
        if (img) {
            const auto& depthAttDesc = rpDesc.attachments[sp.depthAttachmentIndex];
            const auto& depthPlatViews = img->GetPlatformDataViews();
            // Same VK-aligned selection as color attachments:
            // default=imageViewBase, mip≥1→mipViews, layer≥1→layerViews
            MlnResourceView view = img->GetPlatformData().resourceViewBase ? img->GetPlatformData().resourceViewBase
                                                                           : img->GetPlatformData().resourceView;
            if ((depthAttDesc.mipLevel >= 1) &&
                (depthAttDesc.mipLevel < static_cast<uint32_t>(depthPlatViews.mipImageViews.size()))) {
                view = depthPlatViews.mipImageViews[depthAttDesc.mipLevel];
            } else if ((depthAttDesc.layer >= 1) &&
                       (depthAttDesc.layer < static_cast<uint32_t>(depthPlatViews.layerImageViews.size()))) {
                view = depthPlatViews.layerImageViews[depthAttDesc.layer];
            }

            if (!view) {
                MLN_LOG_ERR("Depth attachment has null imageResourceView, skipping depth");
            } else {
                depthAttachment.imageResourceView = view;
                hasDepth = true;

                depthAttachment.loadOp = isSubpassBegin
                                             ? MLN_ATTACHMENT_LOAD_OP_LOAD
                                             : ToMlnLoadOp(rpDesc.attachments[sp.depthAttachmentIndex].loadOp);
                depthAttachment.storeOp = ToMlnStoreOp(rpDesc.attachments[sp.depthAttachmentIndex].storeOp);
                depthAttachment.clearValue.depthStencil.depth =
                    rpDesc.attachments[sp.depthAttachmentIndex].clearValue.depthStencil.depth;
                depthAttachment.clearValue.depthStencil.stencil =
                    rpDesc.attachments[sp.depthAttachmentIndex].clearValue.depthStencil.stencil;

                if (g_mlnLog.graph) {
                    MLN_LOG_GRAPH("Phase2 frame=%u: depth clear=%.4f, stencilClear=%u, loadOp=%u, storeOp=%u, fmt=%u",
                        g_debugFrameCount, depthAttachment.clearValue.depthStencil.depth,
                        depthAttachment.clearValue.depthStencil.stencil, static_cast<uint32_t>(depthAttachment.loadOp),
                        static_cast<uint32_t>(depthAttachment.storeOp),
                        static_cast<uint32_t>(
                            gpuResourceMgr_.GetImageDescriptor(rpDesc.attachmentHandles[sp.depthAttachmentIndex])
                                .format));
                }

                // Check if format has stencil component
                MlnFormat depthFmt = static_cast<MlnFormat>(
                    gpuResourceMgr_.GetImageDescriptor(rpDesc.attachmentHandles[sp.depthAttachmentIndex]).format);
                if (HasStencilComponent(depthFmt)) {
                    stencilAttachment.imageResourceView = view;
                    hasStencil = true;
                    stencilAttachment.loadOp =
                        isSubpassBegin ? MLN_ATTACHMENT_LOAD_OP_LOAD
                                       : ToMlnLoadOp(rpDesc.attachments[sp.depthAttachmentIndex].stencilLoadOp);
                    stencilAttachment.storeOp =
                        ToMlnStoreOp(rpDesc.attachments[sp.depthAttachmentIndex].stencilStoreOp);
                    stencilAttachment.clearValue.depthStencil.stencil =
                        rpDesc.attachments[sp.depthAttachmentIndex].clearValue.depthStencil.stencil;
                }

                // Wire depth resolve attachment if present (MSAA depth resolve)
                if (sp.depthResolveAttachmentCount > 0 && sp.depthResolveAttachmentIndex < rpDesc.attachmentCount) {
                    const auto* resolveImg =
                        gpuResourceMgr_.GetImage<GpuImageMln>(rpDesc.attachmentHandles[sp.depthResolveAttachmentIndex]);
                    if (resolveImg) {
                        depthAttachment.resolveMode = ToMlnResolveMode(sp.depthResolveModeFlags);
                        depthAttachment.resolveResourceView = resolveImg->GetPlatformData().resourceViewBase
                                                                  ? resolveImg->GetPlatformData().resourceViewBase
                                                                  : resolveImg->GetPlatformData().resourceView;
                        depthAttachment.resolveImageLayout = MLN_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        if (hasStencil) {
                            stencilAttachment.resolveMode = ToMlnResolveMode(sp.stencilResolveModeFlags);
                            stencilAttachment.resolveResourceView = depthAttachment.resolveResourceView;
                            stencilAttachment.resolveImageLayout = MLN_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                        }
                    }
                }
            }
        }
    }

    // [REFAC Step 5a] RT creation/cache moved to BuildRenderTarget helper.
    MlnRenderTarget renderTarget = BuildRenderTarget(colorAttachments, hasDepth ? &depthAttachment : nullptr,
        hasStencil ? &stencilAttachment : nullptr, rpDesc.renderArea.extentWidth, rpDesc.renderArea.extentHeight,
        activeSubpassIndex, static_cast<uint32_t>(beginCmd->beginType), objectGroups.size());
    if (!renderTarget) {
        return;
    }
    // RT is owned by renderTargetCache_, NOT by pendingFrame.
    // (pendingFrame.renderTargets is now only used for non-cached RTs, if any.)

    MLN_LOG_FRAME("MILESTONE-3 frame=%u POST-CreateRT: RT=%p OGs=%zu", g_debugFrameCount,
        reinterpret_cast<void*>(renderTarget), objectGroups.size());
    // OG creation loop moved to BuildOGsFromDrawGroups; objectGroups passed in pre-built.

    // [MILESTONE-4] Pre-DG creation
    MLN_LOG_FRAME("MILESTONE-4 frame=%u PRE-CreateDG: OGs=%zu RT=%p", g_debugFrameCount, objectGroups.size(),
        reinterpret_cast<void*>(renderTarget));

    // Create DataGraph - even with 0 OGs, the render target's loadOp=CLEAR
    // still executes, which is needed for clear-only render passes.
    MlnGraphicsDataGraphDescriptor dgDesc{};
    dgDesc.extensionCount = 0;
    dgDesc.extensions = nullptr;
    dgDesc.flags = 0;
    dgDesc.objectGroupCount = static_cast<uint32_t>(objectGroups.size());
    dgDesc.objectGroups = objectGroups.empty() ? nullptr : objectGroups.data();
    dgDesc.renderTarget = renderTarget;

    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH("Phase2 frame=%u: PRE-MlnCreateGraphicsDataGraph (subpass=%u, OGs=%u, RT=%p)", g_debugFrameCount,
            activeSubpassIndex, static_cast<uint32_t>(objectGroups.size()), reinterpret_cast<void*>(renderTarget));
        for (uint32_t ogi = 0; ogi < objectGroups.size(); ++ogi) {
            MLN_LOG_GRAPH("Phase2:   OG[%u]=%p", ogi, reinterpret_cast<void*>(objectGroups[ogi]));
        }
    }
    MlnDataGraph dataGraph = MlnCreateGraphicsDataGraph(mlnDevice, &dgDesc);

    // [MILESTONE-5] Post-DG creation — survived this rpSeg
    MLN_LOG_FRAME("MILESTONE-5 frame=%u POST-CreateDG: DG=%p OGs=%zu RT=%p", g_debugFrameCount,
        reinterpret_cast<void*>(dataGraph), objectGroups.size(), reinterpret_cast<void*>(renderTarget));

    if (dataGraph) {
        if (g_mlnLog.graph) {
            MLN_LOG_GRAPH("Phase2 frame=%u: MlnCreateGraphicsDataGraph OK (subpass=%u, OGs=%u)", g_debugFrameCount,
                activeSubpassIndex, static_cast<uint32_t>(objectGroups.size()));
        }
        // BLOOM_EXEC: Log ALL DataGraphs to diagnose bloom.
        {
            uint32_t colorFmt = 0;
            if (sp.colorAttachmentCount > 0) {
                const uint32_t cAttIdx = sp.colorAttachmentIndices[0];
                if (cAttIdx < rpDesc.attachmentCount) {
                    const GpuImageDesc cDesc = gpuResourceMgr_.GetImageDescriptor(rpDesc.attachmentHandles[cAttIdx]);
                    colorFmt = static_cast<uint32_t>(cDesc.format);
                }
            }
            MLN_LOG_GRAPH("Phase2 frame=%u: DG area=%ux%u fmt=%u colors=%u depthIdx=%u attCnt=%u OGs=%zu",
                g_debugFrameCount, rpDesc.renderArea.extentWidth, rpDesc.renderArea.extentHeight, colorFmt,
                sp.colorAttachmentCount, sp.depthAttachmentIndex, rpDesc.attachmentCount, objectGroups.size());
        }
        dataGraphs.push_back(dataGraph);

        // Track output resources for SchedulingGraph dependency declarations.
        // Both imageResourceView and bufferResource must be the REAL handles
        // of the actual render target attachment (not dummy/placeholder).
        DataGraphResourceInfo dgResInfo{};
        for (uint32_t ci = 0; ci < sp.colorAttachmentCount && dgResInfo.outputCount < MAX_DG_OUTPUT_RESOURCES; ++ci) {
            const uint32_t attIdx = sp.colorAttachmentIndices[ci];
            if (attIdx >= rpDesc.attachmentCount || !colorAttachments[ci].imageResourceView) {
                continue;
            }
            const auto* img = gpuResourceMgr_.GetImage<GpuImageMln>(rpDesc.attachmentHandles[attIdx]);
            if (!img) {
                continue;
            }
            const uint32_t idx = dgResInfo.outputCount++;
            auto& res = dgResInfo.outputs[idx];
            res.type = MLN_PASS_NODE_RESOURCE_TYPE_IMAGE;
            res.imageResourceView = colorAttachments[ci].imageResourceView;
            res.bufferResource = img->GetPlatformData().resource; // real image MlnResource
            dgResInfo.storeOps[idx] = colorAttachments[ci].storeOp;
        }
        if (hasDepth && depthAttachment.imageResourceView && sp.depthAttachmentIndex < rpDesc.attachmentCount &&
            dgResInfo.outputCount < MAX_DG_OUTPUT_RESOURCES) {
            const auto* depthImg =
                gpuResourceMgr_.GetImage<GpuImageMln>(rpDesc.attachmentHandles[sp.depthAttachmentIndex]);
            if (depthImg) {
                const uint32_t idx = dgResInfo.outputCount++;
                auto& res = dgResInfo.outputs[idx];
                res.type = MLN_PASS_NODE_RESOURCE_TYPE_IMAGE;
                res.imageResourceView = depthAttachment.imageResourceView;
                res.bufferResource = depthImg->GetPlatformData().resource; // real depth MlnResource
                dgResInfo.storeOps[idx] = depthAttachment.storeOp;
            }
        }
        dgResInfo.renderTarget = renderTarget;
        // [REFAC §8.7] Stage mask: graphics DG writes at ALL_GRAPHICS stage.
        dgResInfo.srcStage = MLN_PROGRAM_STAGE_ALL_GRAPHICS_BIT;
        dgResInfo.dstStage = MLN_PROGRAM_STAGE_ALL_COMMANDS_BIT;
        dgResources.push_back(dgResInfo);
    } else {
        MLN_LOG_ERR("Phase2 frame=%u: MlnCreateGraphicsDataGraph FAILED (subpass=%u, OGs=%u)", g_debugFrameCount,
            activeSubpassIndex, static_cast<uint32_t>(objectGroups.size()));
    }
}

// RenderSingleCommandList — thin dispatcher that routes to the appropriate walker.
// Primary ctx (outRpSegsOnly==null) → WalkPrimaryCtx (streaming, builds DGs inline)
// Secondary ctx (outRpSegsOnly!=null) → WalkSecondaryCtx (collects rpSegs only)
void RenderBackendMln::RenderSingleCommandList(RenderCommandContext& renderCommandCtx,
    PendingDestroyFrame& pendingFrame, BASE_NS::vector<MlnDataGraph>& outDataGraphs,
    BASE_NS::vector<DataGraphResourceInfo>& outDgResources, void* additionalRpSegsVoid, void* outRpSegsOnlyVoid)
{
    if (outRpSegsOnlyVoid) {
        // [OPT] Pass pendingFrame so secondary ctx can track OGs it creates
        // via direct-build path (ogDirectBuild=true). Caller (RenderAllCommandLists)
        // must share this frame across the whole multi-ctx group so OGs get tracked
        // for deferred destruction and don't leak.
        WalkSecondaryCtx(renderCommandCtx, outRpSegsOnlyVoid, &pendingFrame);
    } else {
        WalkPrimaryCtx(renderCommandCtx, pendingFrame, outDataGraphs, outDgResources, additionalRpSegsVoid);
    }
}

// WalkPrimaryCtx — primary command list walker.
// WalkPrimaryCtx — streaming-only primary ctx walker. Builds DGs inline in command order.
void RenderBackendMln::WalkPrimaryCtx(RenderCommandContext& renderCommandCtx, PendingDestroyFrame& pendingFrame,
    BASE_NS::vector<MlnDataGraph>& outDataGraphs, BASE_NS::vector<DataGraphResourceInfo>& outDgResources,
    void* additionalRpSegsVoid)
{
    auto* additionalRpSegs = static_cast<vector<RenderPassSegment>*>(additionalRpSegsVoid);
    const RenderCommandList& cmdList = *renderCommandCtx.renderCommandList;
    NodeContextPsoManager& psoMgr = *renderCommandCtx.nodeContextPsoMgr;
    NodeContextDescriptorSetManager& descriptorSetMgr = *renderCommandCtx.nodeContextDescriptorSetMgr;
    const auto renderCommands = cmdList.GetRenderCommands();

    if (renderCommands.empty()) {
        return;
    }

    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH("RenderBackendMln::RenderSingleCommandList frame=%u (cmdCount=%zu)", g_debugFrameCount,
            renderCommands.size());
        const auto mrp = cmdList.GetMultiRenderCommandListData();
        MLN_LOG_GRAPH(
            "RenderSingleCommandList frame=%u meta: subpassCount=%u, rpBeginIdx=%u, rpEndIdx=%u, secondaryCmdLists=%d",
            g_debugFrameCount, mrp.subpassCount, mrp.rpBeginCmdIndex, mrp.rpEndCmdIndex, mrp.secondaryCmdLists ? 1 : 0);
    }

    static_cast<NodeContextDescriptorSetManagerMln&>(descriptorSetMgr).BeginBackendFrame();
    psoMgr.BeginBackendFrame();

    // Update descriptor sets (resolve resources, write MlnBindingSets)
    UpdateCommandListDescriptorSets(cmdList, descriptorSetMgr);

    // =========================================================================
    // Phase 1: Scan command list -> collect render pass segments, compute
    //          dispatches, and transfer operations
    // =========================================================================
    MlnRenderState state{};

    RenderPassSegment* currentRP = nullptr;
    uint32_t currentSubpassIndex = 0;
    uint32_t beginRpCmdCount = 0;
    uint32_t beginRpSubpassCmdCount = 0;
    uint32_t endRpCmdCount = 0;
    uint32_t endSubpassCmdCount = 0;
    uint32_t nextSubpassCmdCount = 0;
    uint32_t bindGfxPipelineCmdCount = 0;
    uint32_t bindComputePipelineCmdCount = 0;
    uint32_t drawCmdCount = 0;
    uint32_t drawIndirectCmdCount = 0;

    // Streaming walker state — builds DGs inline in command order (§2.1 fix).
    OpenSegment openSeg = OpenSegment::NONE;
    RenderPassSegment curRP{};           // current open graphics segment (still needed for currentRP pointer + 3-phase)
    vector<MlnObjectGroup> curStreamOGs; // streaming: OGs built at DRAW time (§4.2 DrawCallGroup elimination)
    uint32_t curStreamSubpass = 0;       // streaming: active subpass for current RP
    vector<TransferOp> curTransferOps;   // current open transfer batch
    vector<TransferDstImage> curTransferDsts;  // dst images for current batch
    vector<MlnResource> curTransferDstBuffers; // dst buffers for current batch
    uint32_t streamingRpSegIdx = 0;            // diagnostic counter for BuildGraphicsDg

    // Walker state bundle for Handle* member functions
    PrimaryWalkerState wctx{state, currentRP, currentSubpassIndex, openSeg, curRP, curStreamOGs, curStreamSubpass,
        curTransferOps, curTransferDsts, curTransferDstBuffers, streamingRpSegIdx, &pendingFrame, &outDataGraphs,
        &outDgResources, additionalRpSegs, &psoMgr, renderCommandCtx.renderBackendNode};

    // resolveGraphicsPsoForCurrentRp: moved to anonymous-namespace static helper
    // (ResolveGraphicsPsoForCurrentRp) so HandleStateCommand and WalkSecondaryCtx can share it.

    for (const auto& ref : renderCommands) {
        if (!ref.rc) {
            continue;
        }

        if (HandleStateCommand(&ref, &state, currentRP, currentSubpassIndex, psoMgr, descriptorSetMgr)) {
            continue;
        }

        switch (ref.type) {
            case RenderCommandType::BEGIN_RENDER_PASS: {
                HandleBeginRenderPass(&ref, &wctx);
                break;
            }
            case RenderCommandType::END_RENDER_PASS: {
                ++endRpCmdCount;
                const auto* endCmd = static_cast<const RenderCommandEndRenderPass*>(ref.rc);
                if (endCmd && (endCmd->endType == RenderPassEndType::END_SUBPASS)) {
                    ++endSubpassCmdCount;
                }
                // [REFAC Step 8b] Streaming path: lazy-merge any matching secondary
                // rpSegs into curRP, then immediately build the graphics DG.
                if (curRP.beginCmd) {
                    if (additionalRpSegs && !additionalRpSegs->empty()) {
                        for (auto it = additionalRpSegs->begin(); it != additionalRpSegs->end();) {
                            if (it->beginCmd &&
                                it->beginCmd->beginType == RenderPassBeginType::RENDER_PASS_SUBPASS_BEGIN &&
                                it->subpassStartIndex == curRP.subpassStartIndex) {
                                // Merge both fallback and optimized secondary contributions.
                                for (auto& dg : it->drawGroups) {
                                    curRP.drawGroups.push_back(BASE_NS::move(dg));
                                }
                                for (auto& og : it->secondaryOGHandles) {
                                    curRP.secondaryOGHandles.push_back(og);
                                }
                                it = additionalRpSegs->erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                    FlushGraphicsBatch(&wctx);
                }
                currentRP = nullptr;
                break;
            }
            case RenderCommandType::DRAW:
            case RenderCommandType::DRAW_INDIRECT: {
                HandleDraw(&ref, &wctx);
                break;
            }
            case RenderCommandType::DISPATCH: {
                if (!state.computePso) {
                    break;
                }
                const auto& cmd = *static_cast<const RenderCommandDispatch*>(ref.rc);
                const auto& psoPlat = state.computePso->GetPlatformData();

                ComputeDispatchGroup cg{};
                cg.program = psoPlat.program;
                SnapshotStateToComputeGroup(cg, state);

                cg.groupCountX = cmd.groupCountX;
                cg.groupCountY = cmd.groupCountY;
                cg.groupCountZ = cmd.groupCountZ;

                if (g_mlnLog.comp) {
                    MLN_LOG_COMP("Phase1 frame=%u: DISPATCH program=%p groupCount=(%u,%u,%u)", g_debugFrameCount,
                        reinterpret_cast<void*>(cg.program), cmd.groupCountX, cmd.groupCountY, cmd.groupCountZ);
                }

                // Streaming: build compute DG immediately (in command order).
                FlushTransferBatch(&wctx);
                BuildComputeDg(&cg, pendingFrame, outDataGraphs, outDgResources);
                break;
            }
            case RenderCommandType::DISPATCH_INDIRECT: {
                if (!state.computePso) {
                    break;
                }
                const auto& cmd = *static_cast<const RenderCommandDispatchIndirect*>(ref.rc);
                const auto* buf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cmd.argsHandle);
                if (!buf) {
                    break;
                }
                const auto& psoPlat = state.computePso->GetPlatformData();

                ComputeDispatchGroup cg{};
                cg.program = psoPlat.program;
                SnapshotStateToComputeGroup(cg, state);

                cg.isIndirect = true;
                cg.indirectBuffer = buf->GetPlatformData().resource;
                cg.indirectOffset = static_cast<MlnDeviceSize>(cmd.offset) + buf->GetPlatformData().currentByteOffset;

                // Streaming: build compute DG immediately.
                FlushTransferBatch(&wctx);
                BuildComputeDg(&cg, pendingFrame, outDataGraphs, outDgResources);
                break;
            }
            case RenderCommandType::COPY_BUFFER:
            case RenderCommandType::COPY_BUFFER_IMAGE:
            case RenderCommandType::COPY_IMAGE:
            case RenderCommandType::BLIT_IMAGE:
            case RenderCommandType::CLEAR_COLOR_IMAGE: {
                HandleTransferOp(&ref, &wctx);
                break;
            }
            case RenderCommandType::BARRIER_POINT: {
                // In Vulkan, BARRIER_POINT inserts vkCmdPipelineBarrier for image layout transitions.
                // In Maleoon, we collect barrier images/buffers and declare them as Transfer DG output
                // resources so the SchedulingGraph handles layout transitions implicitly.

                // [SPLIT-ON-TRANSFER-DEP] Per Maleoon spec (MlnTransferObjectGroup §3.4.4):
                //   "If two copies are not depending on each other, they can be packed into a
                //    single object group."
                // → A transfer↔transfer dependency (BARRIER with srcAccess=TRANSFER_WRITE and
                //   dstAccess=TRANSFER_READ/WRITE) means the next COPY depends on a previous
                //   COPY in this batch. They MUST be in different OG/DG so SG can declare the
                //   dependency via passNode depResources. We detect such barriers here and
                //   flush the current transfer batch before continuing.
                const auto& bp = *static_cast<const RenderCommandBarrierPoint*>(ref.rc);
                const RenderBarrierList* rbl = renderCommandCtx.renderBarrierList;
                bool transferToTransferDep = false;
                if (rbl && rbl->HasBarriers(bp.barrierPointIndex)) {
                    const auto* bpBarriers = rbl->GetBarrierPointBarriers(bp.barrierPointIndex);
                    if (bpBarriers) {
                        const auto* bList = bpBarriers->firstBarrierList;
                        for (uint32_t bli = 0; bli < bpBarriers->barrierListCount && bList; ++bli) {
                            for (uint32_t bi = 0; bi < bList->count; ++bi) {
                                const CommandBarrier& cb = bList->commandBarriers[bi];
                                const RenderHandleType handleType = RenderHandleUtil::GetHandleType(cb.resourceHandle);

                                // [SPLIT-ON-TRANSFER-DEP] Detect transfer↔transfer dep on any
                                // resource type. srcAccess having TRANSFER_WRITE_BIT means a
                                // previous COPY wrote this resource; dstAccess having
                                // TRANSFER_READ/WRITE means the next COPY reads/writes it.
                                {
                                    constexpr uint32_t kTrW = AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT;
                                    constexpr uint32_t kTrR = AccessFlagBits::CORE_ACCESS_TRANSFER_READ_BIT;
                                    constexpr uint32_t kTrMask = kTrW | kTrR;
                                    const uint32_t srcA = cb.src.accessFlags;
                                    const uint32_t dstA = cb.dst.accessFlags;
                                    if ((srcA & kTrW) && (dstA & kTrMask)) {
                                        transferToTransferDep = true;
                                    }
                                }

                                // [BARRIER-OPENSEG-GATE] Only attach the resource to the
                                // CURRENT transfer batch's outputs when we are actually inside
                                // an open transfer segment. Outside transfer (e.g. compute→shader
                                // hand-off, graphics→shader hand-off) the resource was NOT
                                // written by transfer, so adding it to curTransferDsts/Buffers
                                // would leak into the next transfer DG's outputs and falsely
                                // declare transfer authorship of an image/buffer that compute
                                // or graphics actually wrote. Compute storage outputs are now
                                // tracked separately (BIND_DS → state.perSetStorage); graphics
                                // outputs go via BuildGraphicsDg attachments — both don't need
                                // BARRIER_POINT to forward them.
                                const bool inTransferSeg = (wctx.openSeg == OpenSegment::TRANSFER);
                                if (handleType == RenderHandleType::GPU_IMAGE) {
                                    const auto srcLayout = cb.src.optionalImageLayout;
                                    const auto dstLayout = cb.dst.optionalImageLayout;
                                    // Skip:
                                    //   - dst attachment layouts (2/3/4): owned by graphics DG render pass
                                    //   - dst TRANSFER_SRC/DST (6/7): pure staging-internal transitions
                                    //   - no-op transitions (src==dst)
                                    // Include the rest (GENERAL=1, SHADER_READ_ONLY=5, PRESENT_SRC, etc.)
                                    const bool isAttachmentLayout =
                                        (dstLayout == ImageLayout::CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) ||
                                        (dstLayout ==
                                            ImageLayout::CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) ||
                                        (dstLayout == ImageLayout::CORE_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
                                    const bool isTransferLayout =
                                        (dstLayout == ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL) ||
                                        (dstLayout == ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
                                    if (inTransferSeg && srcLayout != dstLayout && !isAttachmentLayout &&
                                        !isTransferLayout) {
                                        const auto* img = gpuResourceMgr_.GetImage<GpuImageMln>(cb.resourceHandle);
                                        if (img && img->GetPlatformData().resource) {
                                            const auto& plat = img->GetPlatformData();
                                            AddTransferDstImage(&wctx, plat.resource, plat.resourceView);
                                        }
                                    } else if (g_mlnLog.graph && srcLayout != dstLayout) {
                                        MLN_LOG_GRAPH(
                                            "BARRIER_POINT: skipping image transition %u->%u "
                                            "(openSeg=%u, attachment/transfer-internal/non-transfer)",
                                            (uint32_t)srcLayout, (uint32_t)dstLayout, (uint32_t)wctx.openSeg);
                                    }
                                } else if (handleType == RenderHandleType::GPU_BUFFER) {
                                    const auto srcAccess = cb.src.accessFlags;
                                    const auto dstAccess = cb.dst.accessFlags;
                                    if (g_mlnLog.trans) {
                                        MLN_LOG_TRANS("BARRIER buf: srcAccess=0x%x dstAccess=0x%x handle=0x%llx",
                                            (uint32_t)srcAccess, (uint32_t)dstAccess,
                                            static_cast<unsigned long long>(cb.resourceHandle.id));
                                    }
                                    constexpr uint32_t kTransferWriteBit =
                                        AccessFlagBits::CORE_ACCESS_TRANSFER_WRITE_BIT;
                                    if (inTransferSeg && (srcAccess & kTransferWriteBit)) {
                                        const auto* buf = gpuResourceMgr_.GetBuffer<GpuBufferMln>(cb.resourceHandle);
                                        if (buf && buf->GetPlatformData().resource) {
                                            AddTransferDstBuffer(&wctx, buf->GetPlatformData().resource);
                                        } else {
                                            MLN_LOG_ERR("BARRIER buf: lookup FAILED handle=0x%llx",
                                                static_cast<unsigned long long>(cb.resourceHandle.id));
                                        }
                                    }
                                } else {
                                    // Memory barrier (CustomMemoryBarrier) — emitted by upper
                                    // layer with handleType=UNDEFINED to denote a global
                                    // pipeline barrier that is not attached to a specific
                                    // resource. Maleoon's SG model expresses memory visibility
                                    // through per-resource depResources; the linear passNode
                                    // chain already enforces ordering between adjacent DGs, so
                                    // a memory-only barrier is implicitly satisfied as long as
                                    // the relevant resources are correctly declared as outputs
                                    // by their writer DG (transfer/compute/graphics). Log when
                                    // observed so future scenarios that rely on stronger
                                    // memory barriers can be diagnosed.
                                    if (g_mlnLog.graph) {
                                        MLN_LOG_GRAPH(
                                            "BARRIER memory/unknown: handleType=%u "
                                            "srcAccess=0x%x dstAccess=0x%x — relying on SG passNode chain",
                                            (uint32_t)handleType, (uint32_t)cb.src.accessFlags,
                                            (uint32_t)cb.dst.accessFlags);
                                    }
                                }
                            }
                            bList = bList->nextBarrierPointBarrierList;
                        }
                    }
                }
                // [SPLIT-ON-TRANSFER-DEP] Flush after all dst tracking is done so the resources
                // transitioned in this BARRIER_POINT (e.g. transfer→shader_read on imgA) are
                // included in the OUTGOING batch's outputs, while the new batch starts empty
                // and accumulates the next dependent COPY.
                if (transferToTransferDep && wctx.openSeg == OpenSegment::TRANSFER) {
                    MLN_LOG_TRANS("BARRIER_POINT split: transfer<->transfer dep, flushing batch");
                    FlushTransferBatch(&wctx);
                }
                break;
            }
            case RenderCommandType::BUILD_ACCELERATION_STRUCTURE: {
                FlushTransferBatch(&wctx);
                const auto& asCmd = *static_cast<const RenderCommandBuildAccelerationStructure*>(ref.rc);
                BuildAccelStructDg(asCmd, pendingFrame, outDataGraphs, outDgResources);
                break;
            }
            case RenderCommandType::COPY_ACCELERATION_STRUCTURE_INSTANCES: {
                // CPU-side copy of instance data into a mapped buffer (mirrors Vulkan backend).
                const auto& copyCmd = *static_cast<const RenderCommandCopyAccelerationStructureInstances*>(ref.rc);
                const RenderHandle dstHandle = copyCmd.destination.handle;
                if (uint8_t* dstDataBegin = static_cast<uint8_t*>(gpuResourceMgr_.MapBuffer(dstHandle)); dstDataBegin) {
                    const GpuBufferDesc dstBufDesc = gpuResourceMgr_.GetBufferDescriptor(dstHandle);
                    const uint8_t* dstDataEnd = dstDataBegin + dstBufDesc.byteSize;
                    dstDataBegin += size_t(copyCmd.destination.offset);

                    for (uint32_t instIdx = 0; instIdx < copyCmd.instancesView.size(); ++instIdx) {
                        const auto& inst = copyCmd.instancesView[instIdx];
                        uint64_t accelDeviceAddress = 0;
                        if (const GpuBufferMln* asPtr =
                                gpuResourceMgr_.GetBuffer<GpuBufferMln>(inst.accelerationStructure);
                            asPtr && asPtr->IsAccelerationStructure()) {
                            accelDeviceAddress = asPtr->GetPlatformDataAccelerationStructure().deviceAddress;
                        }
                        const auto& tr = inst.transform;
                        // Convert 4x3 column-major to 3x4 row-major (Maleoon layout matches Vulkan)
                        MlnAccelerationStructureInstance mlnInst{};
                        mlnInst.transform.matrix[0][0] = tr[0].x;
                        mlnInst.transform.matrix[0][1] = tr[1].x;
                        mlnInst.transform.matrix[0][2] = tr[2].x;
                        mlnInst.transform.matrix[0][3] = tr[3].x;
                        mlnInst.transform.matrix[1][0] = tr[0].y;
                        mlnInst.transform.matrix[1][1] = tr[1].y;
                        mlnInst.transform.matrix[1][2] = tr[2].y;
                        mlnInst.transform.matrix[1][3] = tr[3].y;
                        mlnInst.transform.matrix[2][0] = tr[0].z;
                        mlnInst.transform.matrix[2][1] = tr[1].z;
                        mlnInst.transform.matrix[2][2] = tr[2].z;
                        mlnInst.transform.matrix[2][3] = tr[3].z;
                        mlnInst.instanceCustomIndex = inst.instanceCustomIndex;
                        mlnInst.mask = inst.mask;
                        mlnInst.instanceShaderBindingTableRecordOffset = 0U;
                        mlnInst.flags = static_cast<MlnAccelerationStructureInstanceFlags>(inst.flags);
                        mlnInst.accelerationStructureReference = accelDeviceAddress;

                        constexpr size_t byteSize = sizeof(MlnAccelerationStructureInstance);
                        uint8_t* dstData = dstDataBegin + byteSize * instIdx;
                        if (dstData + byteSize <= dstDataEnd) {
                            if (memcpy_s(dstData, byteSize, &mlnInst, byteSize) != 0) {
                                MLN_LOG_ERR("RenderBackendMln: memcpy_s failed for AS instance data");
                            }
                        }
                    }
                    gpuResourceMgr_.UnmapBuffer(dstHandle);
                }
                break;
            }
            case RenderCommandType::NEXT_SUBPASS:
                ++nextSubpassCmdCount;
                if (!currentRP || !currentRP->beginCmd) {
                    MLN_LOG_GRAPH("Phase1: NEXT_SUBPASS without active render pass");
                    break;
                }
                if (currentRP->beginCmd->subpasses.empty()) {
                    MLN_LOG_GRAPH("Phase1: NEXT_SUBPASS ignored; no subpasses in begin command");
                    break;
                }
                if ((currentSubpassIndex + 1u) < currentRP->beginCmd->subpasses.size()) {
                    ++currentSubpassIndex;
                    state.graphicsPso = nullptr;
                    if (state.hasGraphicsPsoHandle) {
                        ResolveGraphicsPsoForCurrentRp(
                            state.graphicsPsoHandle, currentRP, currentSubpassIndex, state, psoMgr);
                    }
                } else {
                    MLN_LOG_GRAPH("Phase1: NEXT_SUBPASS out of range (current=%u, subpassCount=%u)",
                        currentSubpassIndex, static_cast<uint32_t>(currentRP->beginCmd->subpasses.size()));
                }
                break;
            case RenderCommandType::EXECUTE_BACKEND_FRAME_POSITION: {
                // Dispatch to handler. Path A vs Path B distinction + adaptation
                // detection live inside HandleExecuteBackendFrame for one place.
                HandleExecuteBackendFrame(&ref, &wctx);
                break;
            }
            // ---- Other commands that produce real GPU side effects but are not yet
            // implemented in the Maleoon backend. Log explicitly so drops are visible
            // rather than silently falling through `default: break`.
            case RenderCommandType::WRITE_TIMESTAMP:
                // Query-pool timestamp write. Maleoon timestamp/query API not wired up.
                MLN_LOG_ERR("Phase1: WRITE_TIMESTAMP dropped (Maleoon query API not implemented)");
                break;
            case RenderCommandType::GPU_QUEUE_TRANSFER_RELEASE:
            case RenderCommandType::GPU_QUEUE_TRANSFER_ACQUIRE:
                // Cross-queue ownership transfer. Single-queue scenarios don't need it;
                // multi-queue scenarios will need a Maleoon-equivalent ownership barrier.
                MLN_LOG_ERR("Phase1: GPU_QUEUE_TRANSFER_RELEASE/ACQUIRE dropped (multi-queue not supported)");
                break;
            case RenderCommandType::DYNAMIC_STATE_FRAGMENT_SHADING_RATE:
                // VRS (Variable Rate Shading) dynamic state. Not yet wired to Maleoon; PSOs
                // using VRS will fall back to default 1x1 shading rate. Performance loss
                // only — no correctness impact for non-VRS render paths.
                MLN_LOG_ERR("Phase1: DYNAMIC_STATE_FRAGMENT_SHADING_RATE dropped (VRS not implemented)");
                break;
            case RenderCommandType::BEGIN_DEBUG_MARKER:
            case RenderCommandType::END_DEBUG_MARKER:
                // Debug markers (RenderDoc/PIX). Cosmetic, no GPU side effect — safe to skip.
                // Explicit case so this drop is intentional rather than `default: break`.
                break;
            default:
                MLN_LOG_ERR("Phase1: unknown RenderCommandType=%u dropped", (uint32_t)ref.type);
                break;
        }
    }

    // Streaming walker complete — all DGs built inline during command traversal.
    // Final flushes — streaming walker complete
    FlushTransferBatch(&wctx);
    FlushGraphicsBatch(&wctx);
    if (g_mlnLog.graph) {
        MLN_LOG_GRAPH("Phase2 frame=%u: streaming walker emitted %zu DGs (rpSegIdx=%u)", g_debugFrameCount,
            outDataGraphs.size(), streamingRpSegIdx);
    }
}

// MLN_ENABLE_ASYNC_SUBMIT configured in render_backend_mln.h

void RenderBackendMln::SubmitFrame(RenderCommandFrameData& renderCommandFrameData)
{
    // Note: MarkFrameSubmitted is now called inside RenderAllCommandLists at the exact point
    // of successful QueueSubmit (mirrors Vulkan's FrameFenceIsSignalled pattern).
    // This ensures frames without QueueSubmit don't mark the fence → no deadlock.

#if !MLN_ENABLE_ASYNC_SUBMIT
    {
        MlnStatus waitResult = MlnQueueWaitIdle(deviceMln_.GetMlnQueue());
        MLN_LOG_FRAME("WAIT-IDLE frame=%u status=%d", g_debugFrameCount, static_cast<int>(waitResult));
    }
#endif
    MLN_LOG_FRAME("========== FRAME-END frame=%u ==========", g_debugFrameCount);
}

RENDER_END_NAMESPACE()
