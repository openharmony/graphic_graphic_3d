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

#ifndef MALEOON_RENDER_BACKEND_MLN_H
#define MALEOON_RENDER_BACKEND_MLN_H

// ============================================================
// Maleoon Backend Configuration Switches
// ============================================================
#define MLN_ENABLE_MULTITHREADING       1              // 0=sequential, 1=parallel (frontend+backend)
#define OG_CACHE_MODE                   2              // 0=no cache, 1=pool+recreate, 2=pool+update
#define MLN_ENABLE_ASYNC_SUBMIT         1              // 0=WaitIdle every frame, 1=Timeline pipelining
// ============================================================

#include <mutex>

#include <base/containers/vector.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <core/threading/intf_thread_pool.h>
#include <render/namespace.h>

#include "render_backend.h"
#include "maleoon/device_mln.h"

#include <maleoon/maleoon.h>

RENDER_BEGIN_NAMESPACE()

class Device;
class DeviceMln;
struct RenderCommandBuildAccelerationStructure;
class GpuResourceManager;
class NodeContextPsoManager;
class NodeContextPoolManager;
class NodeContextDescriptorSetManager;
struct DescriptorSetLayoutBindingResourcesHandler;
class RenderBarrierList;
class RenderCommandList;
struct RenderCommandContext;

class RenderBackendMln final : public RenderBackend {
public:
    RenderBackendMln(Device& device, GpuResourceManager& gpuResourceMgr, CORE_NS::ITaskQueue* queue);
    ~RenderBackendMln() override;

    void Render(RenderCommandFrameData& renderCommandFrameData,
        const RenderBackendBackBufferConfiguration& backBufferConfig) override;
    void Present(const RenderBackendBackBufferConfiguration& backBufferConfig) override;

private:
    struct PresentationInfo {
        bool useSwapchain { false };
        bool validAcquire { false };
        uint32_t swapchainImageIndex { ~0u };
        uint32_t renderNodeCommandListIndex { ~0u };
        MlnResource acquiredImageResource { MLN_NULL_HANDLE }; // for acquire wait targeting
    };
    struct PresentationData {
        bool present { true };
        BASE_NS::vector<PresentationInfo> infos;
    };

    // Deferred-destroy container: objects waiting for GPU completion
    struct PendingDestroyFrame {
        uint64_t timelineValue {0};
        BASE_NS::vector<MlnSchedulingGraph> schedulingGraphs;
        BASE_NS::vector<MlnDataGraph> dataGraphs;
        BASE_NS::vector<MlnObjectGroup> objectGroups;
        BASE_NS::vector<MlnRenderTarget> renderTargets;
    };

 
    struct RenderBackendRecordingStateMln final : public RenderBackendRecordingState {
        MlnDevice mlnDevice { MLN_NULL_HANDLE };
 
        /** Backend node pushes created DGs here. Writable through pointer indirection
        *  even when accessed via const& from the base interface. */
        BASE_NS::vector<MlnDataGraph>* outDataGraphs {nullptr};
        /** Optional per-DG resource tracking. Writable through pointer indirection.
        *  Framework auto-syncs defaults if plugin does not populate. */
        BASE_NS::vector<DataGraphResourceInfo>* outDgResources {nullptr};
    };

    // [DGRESINFO] Single source of truth: Render::DataGraphResourceInfo + MAX_DG_OUTPUT_RESOURCES
    // are now declared in <render/maleoon/intf_device_mln.h> (transitively included via
    // device_mln.h). The public type is shared with GS plugins via RenderBackendRecordingStateMln,
    // so a private duplicate would require const-cast / type-pun tricks. Keep the public type
    // as canonical; extend it there if more fields are needed.

    void AcquirePresentationInfo(
        RenderCommandFrameData& renderCommandFrameData,
        const RenderBackendBackBufferConfiguration& backBufferConfig);

    void RenderAllCommandLists(RenderCommandFrameData& renderCommandFrameData);
    // Process a single command list. Internal RenderPassSegment vectors are
    // passed as opaque void* to avoid exposing the .cpp-local struct in the header.
    // - additionalRpSegs: void* to vector<RenderPassSegment>*, appended before Phase 2
    // - outRpSegsOnly: void* to vector<RenderPassSegment>*, Phase 1 only mode
    void RenderSingleCommandList(
        RenderCommandContext& renderCommandCtx,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnDataGraph>& outDataGraphs,
        BASE_NS::vector<DataGraphResourceInfo>& outDgResources,
        void* additionalRpSegs = nullptr,
        void* outRpSegsOnly = nullptr);
    void FlushTransferBatch(void* wctx);
    void FlushGraphicsBatch(void* wctx);
    void AddTransferDstImage(void* wctx, MlnResource res, MlnResourceView view);
    void AddTransferDstBuffer(void* wctx, MlnResource res);
    void HandleBeginRenderPass(const void* refPtr, void* wctx);
    void HandleDraw(const void* refPtr, void* wctx);
    void HandleTransferOp(const void* refPtr, void* wctx);
    void HandleExecuteBackendFrame(const void* refPtr, void* wctx);
    bool HandleStateCommand(const void* refPtr, void* statePtr, void* currentRPPtr,
        uint32_t currentSubpassIndex, NodeContextPsoManager& psoMgr,
        NodeContextDescriptorSetManager& descriptorSetMgr);
    // [REFAC Step 2] Secondary ctx deferred walker — fills outRpSegsOnly with
    // rpSegs containing DrawCallGroup snapshots for primary ctx to lazy-merge.
    // Placeholder: body filled in Step 7b when the main loop switches to walker.
    // outRpSegsOnly: void* to BASE_NS::vector<RenderPassSegment>*
    void WalkSecondaryCtx(
        RenderCommandContext& renderCommandCtx,
        void* outRpSegsOnly,
        void* pendingFrame = nullptr);  // [OPT] PendingDestroyFrame* for ogDirectBuild OG tracking
    // [REFAC Step 7b] Primary ctx walker — full per-cmd-list processing.
    // Body filled in Step 7b by physically renaming RenderSingleCommandList →
    // WalkPrimaryCtx (mechanical move, three-phase architecture preserved).
    // Currently still handles BOTH primary and secondary paths via outRpSegsOnly
    // parameter (split happens in Step 8 alongside streaming walker refactor
    // that fixes DG execution order P0 bug, plan §2.1).
    //   additionalRpSegs: void* to BASE_NS::vector<RenderPassSegment>* (or null)
    //   outRpSegsOnly:    void* to BASE_NS::vector<RenderPassSegment>* (or null)
    //                     when non-null, secondary-ctx Phase 1-only mode
    void WalkPrimaryCtx(
        RenderCommandContext& renderCommandCtx,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnDataGraph>& outDataGraphs,
        BASE_NS::vector<DataGraphResourceInfo>& outDgResources,
        void* additionalRpSegs);
    // [REFAC Step 3] Build one transfer DataGraph from a batch of TransferOps.
    // Extracted from RenderSingleCommandList Phase 2 "Build transfer data graphs".
    // transferOpsVec / transferDstImagesVec are void* because TransferOp and
    // TransferDstImage are anonymous-namespace types in the .cpp — same opaque
    // pattern as RenderSingleCommandList's additionalRpSegs.
    //   transferOpsVec:      BASE_NS::vector<TransferOp>* (non-const, in-place fixups)
    //   transferDstImagesVec: const BASE_NS::vector<TransferDstImage>*
    void BuildTransferDg(
        void* transferOpsVec,
        void* transferDstImagesVec,
        void* transferDstBuffersVec,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnDataGraph>& outDataGraphs,
        BASE_NS::vector<DataGraphResourceInfo>& outDgResources);
    // [REFAC Step 4] Build one compute DataGraph from a single dispatch group.
    // Extracted from RenderSingleCommandList Phase 2 "Build compute data graphs"
    // (per-iteration body of the for-loop). Needs class member access:
    // EnsureEmptyBindingSet() / emptyBindingSet_ for set padding.
    //   computeDispatchGroupPtr: const ComputeDispatchGroup* (anon-ns type)
    void BuildComputeDg(
        const void* computeDispatchGroupPtr,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnDataGraph>& outDataGraphs,
        BASE_NS::vector<DataGraphResourceInfo>& outDgResources);
    void BuildAccelStructDg(
        const RenderCommandBuildAccelerationStructure& renderCmd,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnDataGraph>& outDataGraphs,
        BASE_NS::vector<DataGraphResourceInfo>& outDgResources);
    // [REFAC Step 5a] Build (or look up cached) MlnRenderTarget for one rpSeg.
    // Extracted from RenderSingleCommandList Phase 2 graphics RT creation block.
    // Needs class member access: device_, renderTargetCache_, renderTargetCacheMutex_.
    // Returns MLN_NULL_HANDLE on failure (caller should skip the rpSeg).
    //   colorAttachments: built by caller from subpass color attachments
    //   depthAttachment / stencilAttachment: pass nullptr if not present
    //   areaWidth/areaHeight: rpDesc.renderArea extents
    //   diagSubpassIndex/diagBeginType/diagDrawCount: for logging only
    MlnRenderTarget BuildRenderTarget(
        const BASE_NS::vector<MlnAttachmentDescriptor>& colorAttachments,
        const MlnAttachmentDescriptor* depthAttachment,
        const MlnAttachmentDescriptor* stencilAttachment,
        uint32_t areaWidth,
        uint32_t areaHeight,
        uint32_t diagSubpassIndex,
        uint32_t diagBeginType,
        size_t diagDrawCount);
    // [REFAC Step 5b] Build one MlnObjectGroup from a DrawCallGroup state snapshot.
    // Extracted from RenderSingleCommandList Phase 2 graphics OG creation block
    // (originally cpp:4337-4691, contains the OG_CACHE_MODE 0/1/2 three-way
    // #if block — preserved verbatim, MUST NOT be split or simplified).
    // Needs class member access: device_, ogPools_, ogPoolsMutex_, ogTotalCount_,
    // bsDebugCache_ (debug log).
    //   inputsPtr: const BuildOGInputs* (anon-ns struct in .cpp)
    //   outObjectGroups: appended on success
    //   pendingFrame: appended on success in Mode 0
    void BuildOGFromState(
        const void* inputsPtr,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnObjectGroup>& outObjectGroups);
    // Build OGs from a RenderPassSegment's DrawCallGroups (3-phase path / secondary merge).
    // Marshals each DrawCallGroup → Mln*Descriptors → BuildOGFromState.
    //   rpSegPtr: const RenderPassSegment* (anon-ns type)
    //   activeSubpassIndex: which subpass to build OGs for
    void BuildOGsFromDrawGroups(
        const void* rpSegPtr,
        uint32_t activeSubpassIndex,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnObjectGroup>& outObjectGroups);
    // [OPT] Extracted per-DG body — single DrawCallGroup → single OG.
    // Shared by BuildOGsFromDrawGroups (batch path) and BuildOGFromDraw*Command
    // (streaming / secondary direct-build, stack-only).
    //   dgPtr:     const DrawCallGroup* (anon-ns type)
    //   rpDescPtr: const RenderPassDesc* (public API type)
    void BuildSingleOGFromDrawGroup(
        const void* dgPtr,
        const void* rpDescPtr,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnObjectGroup>& outObjectGroups);
    // [OPT] Direct OG creation from DRAW command + state. Stack-only DrawCallGroup.
    // Returns MLN_NULL_HANDLE on failure. Used when g_mlnLog.ogDirectBuild == true.
    //   statePtr: const MlnRenderState* (anon-ns type)
    //   cmdPtr:   const RenderCommandDraw* (public API type)
    //   rpSegPtr: const RenderPassSegment* (anon-ns type)
    MlnObjectGroup BuildOGFromDrawCommand(
        const void* statePtr,
        const void* cmdPtr,
        const void* rpSegPtr,
        uint32_t subpassIndex,
        PendingDestroyFrame& pendingFrame);
    // [OPT] Direct OG creation from DRAW_INDIRECT command + state. Stack-only.
    //   cmdPtr: const RenderCommandDrawIndirect* (public API type)
    MlnObjectGroup BuildOGFromDrawIndirectCommand(
        const void* statePtr,
        const void* cmdPtr,
        const void* rpSegPtr,
        uint32_t subpassIndex,
        PendingDestroyFrame& pendingFrame);
    // Build one graphics DataGraph from pre-built OGs + render pass descriptor.
    // RT creation via BuildRenderTarget, DG assembly, output resource tracking.
    //   beginCmdPtr: const RenderCommandBeginRenderPass* (render pass desc + subpasses)
    //   activeSubpassIndex: caller-computed active subpass index
    //   objectGroups: pre-built OGs (from streaming DRAW or BuildOGsFromDrawGroups)
    //   rpSegIdx: diagnostic counter for logging only
    void BuildGraphicsDg(
        const void* beginCmdPtr,
        uint32_t activeSubpassIndex,
        const BASE_NS::vector<MlnObjectGroup>& objectGroups,
        uint32_t rpSegIdx,
        PendingDestroyFrame& pendingFrame,
        BASE_NS::vector<MlnDataGraph>& outDataGraphs,
        BASE_NS::vector<DataGraphResourceInfo>& outDgResources);
    void UpdateGlobalDescriptorSets();
    void UpdateCommandListDescriptorSets(
        const RenderCommandList& renderCommandList, NodeContextDescriptorSetManager& ncdsm);
    void EnsureDefaultImageResourceView();
    void WriteDescriptorSetBindings(
        MlnBindingSet bindingSet, const DescriptorSetLayoutBindingResourcesHandler& bindingResources);

    void SubmitFrame(RenderCommandFrameData& renderCommandFrameData);

    // Reclaim completed deferred-destroy frames
    void ReclaimCompletedFrames();

    // Per-command-list build result for parallel construction
    struct PerCommandListResult {
        BASE_NS::vector<MlnDataGraph> dataGraphs;
        BASE_NS::vector<DataGraphResourceInfo> dgResources;
        BASE_NS::vector<MlnObjectGroup> objectGroups;
        BASE_NS::vector<MlnRenderTarget> renderTargets;
        bool isStitchedSubpass { false }; // true if ctx began with RENDER_PASS_SUBPASS_BEGIN
        MlnRenderTarget firstRenderTarget { MLN_NULL_HANDLE }; // RT for DG merge
    };

    Device& device_;
    DeviceMln& deviceMln_;
    GpuResourceManager& gpuResourceMgr_;
    CORE_NS::ITaskQueue* queue_ { nullptr };

    PresentationData presentationData_;

    // Timeline for frame submission synchronization
    MlnTimeline submitTimeline_ { MLN_NULL_HANDLE };
    uint64_t submitTimelineValue_ { 0 };

    // Binary timeline for swapchain acquire synchronization (mirrors Vulkan acquire semaphore).
    // MlnAcquireNextImage signals this timeline when the image is available;
    // QueueSubmit waits on it before rendering to the acquired image.
    // Type = MLN_TIMELINE_TYPE_BINARY (per Maleoon spec: timelineValue ignored for binary).
    MlnTimeline acquireTimeline_ { MLN_NULL_HANDLE };

    // True if this frame's RenderAllCommandLists did a successful QueueSubmit.
    // Present uses this to decide wait behavior (mirrors Vulkan presentationWaitSemaphore).
    bool frameDidSubmit_ { false };

    // Double-buffered deferred destruction
    BASE_NS::vector<PendingDestroyFrame> pendingDestroyFrames_;

    // ---- RenderTarget cache (mirrors Vulkan ContextFramebufferCacheVk) ----
    // RT is determined by attachment views + loadOp/storeOp + render area.
    // Cache avoids ~10-15 MlnCreateRenderTarget + MlnDestroyRenderTarget per frame.
    struct RenderTargetCacheEntry {
        uint64_t frameUseIndex { 0 };
        MlnRenderTarget renderTarget { MLN_NULL_HANDLE };
    };
    BASE_NS::unordered_map<uint64_t, RenderTargetCacheEntry> renderTargetCache_;
    std::mutex renderTargetCacheMutex_; // guards renderTargetCache_ for parallel RenderSingleCommandList
    void EvictStaleRenderTargets();

    // ---- ObjectGroup pool cache (Create-once, Update-many) ----
    // Key = hash of static OG identity (program + PSO state + draw type + VB count).
    // Multiple draws per frame can share the same key, so each key maps to a pool
    // of OGs allocated in frame-order. On cache hit, dynamic state is updated via
    // MlnUpdateGraphicsObjectGroup.
    struct OGPoolEntry {
        uint64_t lastFrameUsed { 0 };
        MlnObjectGroup objectGroup { MLN_NULL_HANDLE };
    };
    struct OGPool {
        uint32_t frameAllocIndex { 0 }; // per-frame allocation cursor, reset each frame
        BASE_NS::vector<OGPoolEntry> entries;
        // Newly created OGs are staged here and merged into entries at next frame start.
        // This ensures OGs are submitted to the GPU before they become eligible for reuse.
        BASE_NS::vector<OGPoolEntry> pendingEntries;
    };
    static constexpr uint32_t OG_CACHE_MAX_TOTAL = 1024;
    BASE_NS::unordered_map<uint64_t, OGPool> ogPools_;
    std::mutex ogPoolsMutex_; // protects ogPools_ during parallel RenderSingleCommandList
    uint32_t ogTotalCount_ { 0 }; // total OGs across all pools
    void ResetOGPoolCounters();
    void EvictStaleOGs();

    // Cached default image resource view - used as fallback when image bindings
    // resolve to null, preventing Maleoon runtime SIGSEGV on null MlnResourceView.
    MlnResourceView defaultImageResourceView_ { MLN_NULL_HANDLE };

    // Cached fallback resource handle for MlnPassNodeResourceDescriptor.bufferResource.
    // SG output resources prefer each attachment's own MlnResource; this is used only
    // when the attachment-specific handle cannot be resolved.
    MlnResource defaultBufferResource_ { MLN_NULL_HANDLE };

    // ---- Empty (dummy) binding set for DG-Error fix ----
    // Maleoon declarative model requires OG binding sets to cover all set indices from 0.
    // When the engine only binds sets starting from index N>0 (e.g. bloom binds set 1 only),
    // sets 0..N-1 must be filled with a valid empty binding set to avoid
    // "DG-Error: null set address of cf (set id 0)".
    MlnBindingLayout emptyBindingLayout_ { MLN_NULL_HANDLE };
    MlnBindingSet emptyBindingSet_ { MLN_NULL_HANDLE };
    void EnsureEmptyBindingSet();

    // ---- Debug: BindingSet resource cache for OG dump diagnosis ----
    // MlnBindingSet is opaque; cache WriteDS info so OG dump can print resource details.
    struct BindingDebugEntry {
        uint32_t binding { 0 };      // slot index
        uint32_t arrayIndex { 0 };   // array element index (0 for non-array)
        uint32_t bindingType { 0 };  // MlnBindingType enum value
        // buffer fields (UNIFORM_BUFFER / STORAGE_BUFFER / *_DYNAMIC)
        MlnResource bufResource { MLN_NULL_HANDLE };
        uint64_t bufOffset { 0 };
        uint64_t bufRange { 0 };
        // image fields (SAMPLED_IMAGE / STORAGE_IMAGE / COMBINED_IMAGE_SAMPLER / INPUT_ATTACHMENT)
        MlnResourceView imgView { MLN_NULL_HANDLE };
        MlnSampler imgSampler { MLN_NULL_HANDLE };
        uint32_t imgLayout { 0 };
        uint32_t imgWidth { 0 };
        uint32_t imgHeight { 0 };
        uint32_t imgFormat { 0 };
        // sampler-only fields (SAMPLER)
        MlnSampler sampler { MLN_NULL_HANDLE };
    };
    BASE_NS::unordered_map<uintptr_t, BASE_NS::vector<BindingDebugEntry>> bsDebugCache_;

};

RENDER_END_NAMESPACE()

#endif // MALEOON_RENDER_BACKEND_MLN_H
