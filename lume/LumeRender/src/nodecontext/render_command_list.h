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

#ifndef CORE_RENDER_RENDER_COMMAND_LIST_H
#define CORE_RENDER_RENDER_COMMAND_LIST_H

#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class GpuResourceManager;
class LinearAllocator;
class NodeContextDescriptorSetManager;
class NodeContextPsoManager;
class RenderNodeContextManager;

enum class DrawType : uint32_t {
    UNDEFINED = 0,
    DRAW = 1,
    DRAW_INDEXED = 2,
    DRAW_INDIRECT = 3,
    DRAW_INDEXED_INDIRECT = 4,
};

// BarrierPoint is a special command which will be added before render commands
// that need some kind of syncing.
// RenderGraph is responsible for updating the barriers to be correct.
//
// BarrierPoint is added for:
//
// BeginRenderPass
// Dispatch
// DispatchIndirect
//
// CopyBuffer
// CopyBufferImage
// BlitImage
// ClearColorImage

enum class RenderCommandType : uint32_t {
    UNDEFINED,
    DRAW,
    DRAW_INDIRECT,
    DISPATCH,
    DISPATCH_INDIRECT,
    BIND_PIPELINE,
    BEGIN_RENDER_PASS,
    NEXT_SUBPASS,
    END_RENDER_PASS,
    BIND_VERTEX_BUFFERS,
    BIND_INDEX_BUFFER,
    COPY_BUFFER,
    COPY_BUFFER_IMAGE,
    COPY_IMAGE,
    BLIT_IMAGE,

    BARRIER_POINT,

    BIND_DESCRIPTOR_SETS,

    PUSH_CONSTANT,

    BUILD_ACCELERATION_STRUCTURE,
    COPY_ACCELERATION_STRUCTURE_INSTANCES,

    CLEAR_COLOR_IMAGE,

    DYNAMIC_STATE_VIEWPORT,
    DYNAMIC_STATE_SCISSOR,
    DYNAMIC_STATE_LINE_WIDTH,
    DYNAMIC_STATE_DEPTH_BIAS,
    DYNAMIC_STATE_BLEND_CONSTANTS,
    DYNAMIC_STATE_DEPTH_BOUNDS,
    DYNAMIC_STATE_STENCIL,
    DYNAMIC_STATE_FRAGMENT_SHADING_RATE,

    EXECUTE_BACKEND_FRAME_POSITION,

    WRITE_TIMESTAMP,

    GPU_QUEUE_TRANSFER_RELEASE,
    GPU_QUEUE_TRANSFER_ACQUIRE,

    BEGIN_DEBUG_MARKER,
    END_DEBUG_MARKER,

    COUNT, // used as the number of values and must be last
};

#if RENDER_DEBUG_COMMAND_MARKERS_ENABLED
// These names should match the RenderCommandType enum values.
constexpr const char* COMMAND_NAMES[] = {
    "Undefined",
    "Draw",
    "DrawIndirect",
    "Dispatch",
    "DispatchIndirect",
    "BindPipeline",
    "BeginRenderPass",
    "NextSubpass",
    "EndRenderPass",
    "BindVertexBuffers",
    "BindIndexBuffer",
    "CopyBuffer",
    "CopyBufferImage",
    "CopyImage",
    "BlitImage",

    "BarrierPoint",
    "BindDescriptorSets",

    "PushConstant",

    "BuildAccelerationStructures",
    "CopyAccelerationStructureInstances",

    "ClearColorImage",

    "DynamicStateViewport",
    "DynamicStateScissor",
    "DynamicStateLineWidth",
    "DynamicStateDepthBias",
    "DynamicStateBlendConstants",
    "DynamicStateDepthBounds",
    "DynamicStateStencil",
    "DynamicStateFragmentShadingRate",

    "ExecuteBackendFramePosition",

    "WriteTimestamp",

    "GpuQueueTransferRelease",
    "GpuQueueTransferAcquire",

    "BeginDebugMarker",
    "EndDebugMark",
};
static_assert(BASE_NS::countof(COMMAND_NAMES) == (static_cast<uint32_t>(RenderCommandType::COUNT)));
#endif

// special command which is not tied to a command list
struct ProcessBackendCommand {
    IRenderBackendPositionCommand::Ptr command;
    RenderBackendCommandPosition backendCommandPosition;
};

enum class RenderPassBeginType : uint32_t {
    RENDER_PASS_BEGIN,
    RENDER_PASS_SUBPASS_BEGIN, // multi render commandlist render pass subpass begin
};

enum class RenderPassEndType : uint32_t {
    END_RENDER_PASS,
    END_SUBPASS,
};

struct RenderCommandDraw {
    DrawType drawType { DrawType::UNDEFINED };

    uint32_t vertexCount { 0 };
    uint32_t instanceCount { 0 };
    uint32_t firstVertex { 0 };
    uint32_t firstInstance { 0 };

    uint32_t indexCount { 0 };
    uint32_t firstIndex { 0 };
    int32_t vertexOffset { 0 };
};

struct RenderCommandDrawIndirect {
    DrawType drawType { DrawType::UNDEFINED };

    RenderHandle argsHandle;
    uint32_t offset { 0 };
    uint32_t drawCount { 0 };
    uint32_t stride { 0 };
};

struct RenderCommandDispatch {
    uint32_t groupCountX { 0 };
    uint32_t groupCountY { 0 };
    uint32_t groupCountZ { 0 };
};

struct RenderCommandDispatchIndirect {
    RenderHandle argsHandle;
    uint32_t offset { 0 };
};

struct RenderCommandBindPipeline {
    RenderHandle psoHandle;
    PipelineBindPoint pipelineBindPoint;
};

struct RenderCommandBindDescriptorSets {
    RenderHandle psoHandle; // this is the previously BindPipeline() pso handle

    uint32_t firstSet { 0u };
    uint32_t setCount { 0u };
    RenderHandle descriptorSetHandles[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] { {}, {}, {}, {} };

    struct DescriptorSetDynamicOffsets {
        uint32_t dynamicOffsetCount { 0u };
        uint32_t* dynamicOffsets { nullptr };
    };
    DescriptorSetDynamicOffsets descriptorSetDynamicOffsets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
};

struct RenderPassImageLayouts {
    // counts are the same as in RenderPassDesc
    // these layouts are used for render pass hashing
    // layouts in the beginning of the render pass
    ImageLayout attachmentInitialLayouts[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] {
        ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED
    };

    // layouts after the render pass
    ImageLayout attachmentFinalLayouts[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] {
        ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED
    };
};

struct RenderPassAttachmentResourceStates {
    // the state of resources when used in render pass (subpasses)
    GpuResourceState states[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] {};
    ImageLayout layouts[PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT] { CORE_IMAGE_LAYOUT_UNDEFINED,
        CORE_IMAGE_LAYOUT_UNDEFINED, CORE_IMAGE_LAYOUT_UNDEFINED, CORE_IMAGE_LAYOUT_UNDEFINED,
        CORE_IMAGE_LAYOUT_UNDEFINED, CORE_IMAGE_LAYOUT_UNDEFINED, CORE_IMAGE_LAYOUT_UNDEFINED,
        CORE_IMAGE_LAYOUT_UNDEFINED };
};

struct RenderCommandBeginRenderPass {
    RenderPassBeginType beginType { RenderPassBeginType::RENDER_PASS_BEGIN };
    RenderPassDesc renderPassDesc;
    RenderPassImageLayouts imageLayouts;
    RenderPassAttachmentResourceStates inputResourceStates;
    bool enableAutomaticLayoutChanges { true };

    uint32_t subpassStartIndex { 0 }; // patched multi render command list render passes can have > 0
    BASE_NS::array_view<RenderPassSubpassDesc> subpasses;
    BASE_NS::array_view<RenderPassAttachmentResourceStates> subpassResourceStates;
};

struct RenderCommandNextSubpass {
    SubpassContents subpassContents { SubpassContents::CORE_SUBPASS_CONTENTS_INLINE };
    // index to other render command list if CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS
    uint64_t renderCommandListIndex { 0 };
};

struct RenderCommandEndRenderPass {
    RenderPassEndType endType { RenderPassEndType::END_RENDER_PASS };
    uint32_t subpassStartIndex { 0u };
    uint32_t subpassCount { 0u };
};

struct RenderCommandBlitImage {
    RenderHandle srcHandle;
    RenderHandle dstHandle;

    ImageBlit imageBlit;

    Filter filter { Filter::CORE_FILTER_NEAREST };

    // added already in render command list methods to correct layouts
    ImageLayout srcImageLayout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
    ImageLayout dstImageLayout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
};

struct RenderCommandPushConstant {
    RenderHandle psoHandle; // this is the previously BindPipeline() pso handle

    PushConstant pushConstant;
    uint8_t* data { nullptr };
};

struct ResourceBarrier {
    AccessFlags accessFlags { 0 };
    PipelineStageFlags pipelineStageFlags { 0 };
    ImageLayout optionalImageLayout { ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED };
    uint32_t optionalByteOffset { 0 };
    uint32_t optionalByteSize { PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
    ImageSubresourceRange optionalImageSubresourceRange {};
};

struct CommandBarrier {
    RenderHandle resourceHandle;

    ResourceBarrier src;
    GpuQueue srcGpuQueue;

    ResourceBarrier dst;
    GpuQueue dstGpuQueue;
};

struct RenderCommandBarrierPoint {
    RenderCommandType renderCommandType { RenderCommandType::UNDEFINED };

    uint32_t barrierPointIndex { ~0u };

    uint32_t customBarrierIndexBegin { ~0u };
    uint32_t customBarrierCount { 0u };

    uint32_t vertexIndexBarrierIndexBegin { ~0u };
    uint32_t vertexIndexBarrierCount { 0u };

    uint32_t indirectBufferBarrierIndexBegin { ~0u };
    uint32_t indirectBufferBarrierCount { 0u };

    uint32_t descriptorSetHandleIndexBegin { ~0u };
    uint32_t descriptorSetHandleCount { 0 };
};

struct RenderCommandBindVertexBuffers {
    uint32_t vertexBufferCount { 0 };
    VertexBuffer vertexBuffers[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
};

struct RenderCommandBindIndexBuffer {
    IndexBuffer indexBuffer;
};

struct RenderCommandCopyBuffer {
    RenderHandle srcHandle;
    RenderHandle dstHandle;

    BufferCopy bufferCopy;
};

struct RenderCommandCopyBufferImage {
    enum class CopyType : uint32_t {
        UNDEFINED = 0,
        BUFFER_TO_IMAGE = 1,
        IMAGE_TO_BUFFER = 2,
    };

    CopyType copyType { CopyType::UNDEFINED };
    RenderHandle srcHandle;
    RenderHandle dstHandle;

    BufferImageCopy bufferImageCopy;
};

struct RenderCommandCopyImage {
    RenderHandle srcHandle;
    RenderHandle dstHandle;

    ImageCopy imageCopy;
};

struct RenderCommandBuildAccelerationStructure {
    AsBuildGeometryData geometry;

    AsGeometryTrianglesData* trianglesData { nullptr };
    AsGeometryAabbsData* aabbsData { nullptr };
    AsGeometryInstancesData* instancesData { nullptr };

    BASE_NS::array_view<AsGeometryTrianglesData> trianglesView;
    BASE_NS::array_view<AsGeometryAabbsData> aabbsView;
    BASE_NS::array_view<AsGeometryInstancesData> instancesView;
};

struct RenderCommandCopyAccelerationStructureInstances {
    BufferOffset destination;

    AsInstance* instancesData { nullptr };
    BASE_NS::array_view<AsInstance> instancesView;
};

struct RenderCommandClearColorImage {
    RenderHandle handle;
    ClearColorValue color;
    // added already in render command list methods to correct layout
    ImageLayout imageLayout { CORE_IMAGE_LAYOUT_UNDEFINED };

    BASE_NS::array_view<ImageSubresourceRange> ranges;
};

// dynamic states
struct RenderCommandDynamicStateViewport {
    ViewportDesc viewportDesc;
};

struct RenderCommandDynamicStateScissor {
    ScissorDesc scissorDesc;
};

struct RenderCommandDynamicStateLineWidth {
    float lineWidth { 1.0f };
};

struct RenderCommandDynamicStateDepthBias {
    float depthBiasConstantFactor { 0.0f };
    float depthBiasClamp { 0.0f };
    float depthBiasSlopeFactor { 0.0f };
};

struct RenderCommandDynamicStateBlendConstants {
    float blendConstants[4u] { 0.0f, 0.0f, 0.0f, 0.0f }; // rgba values used in blending
};

struct RenderCommandDynamicStateDepthBounds {
    float minDepthBounds { 0.0f };
    float maxDepthBounds { 1.0f };
};

enum class StencilDynamicState : uint32_t {
    UNDEFINED = 0,
    COMPARE_MASK = 1,
    WRITE_MASK = 2,
    REFERENCE = 3,
};

struct RenderCommandDynamicStateStencil {
    StencilDynamicState dynamicState { StencilDynamicState::UNDEFINED };
    StencilFaceFlags faceMask { 0 };
    uint32_t mask { 0 };
};

struct RenderCommandDynamicStateFragmentShadingRate {
    Size2D fragmentSize { 1u, 1u };
    FragmentShadingRateCombinerOps combinerOps {};
};

struct RenderCommandWriteTimestamp {
    RenderHandle handle;
    uint32_t queryIndex { 0 };
    PipelineStageFlagBits pipelineStageFlagBits { PipelineStageFlagBits::CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
};

struct RenderCommandExecuteBackendFramePosition {
    uint64_t id { 0 };
    IRenderBackendCommand* command { nullptr };
};

#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
struct RenderCommandBeginDebugMarker {
    static constexpr uint32_t SIZE_OF_NAME { 128U };

    BASE_NS::fixed_string<SIZE_OF_NAME> name;
    BASE_NS::Math::Vec4 color { 1.0f, 1.0f, 1.0f, 1.0f };
};

struct RenderCommandEndDebugMarker {
    uint64_t id { 0 };
};
#endif

struct RenderCommandWithType {
    RenderCommandType type { RenderCommandType::UNDEFINED };
    void* rc { nullptr };
};

struct MultiRenderPassCommandListData {
    uint32_t subpassCount { 1u };
    uint32_t rpBeginCmdIndex { ~0u };
    uint32_t rpEndCmdIndex { ~0u };
    uint32_t rpBarrierCmdIndex { ~0u };
    bool secondaryCmdLists { false };
};

// RenderCommandList implementation
// NOTE: Many early optimizations cannot be done in the render command list
// (e.g. render pass and pipeline hashes are fully evaluated in the backend)
class RenderCommandList final : public IRenderCommandList {
public:
    struct LinearAllocatorStruct {
        BASE_NS::vector<BASE_NS::unique_ptr<LinearAllocator>> allocators;
    };

    RenderCommandList(BASE_NS::string_view nodeName, NodeContextDescriptorSetManager& nodeContextDescriptorSetMgr,
        const GpuResourceManager& gpuResourceMgr, const NodeContextPsoManager& nodeContextPsoMgr, const GpuQueue& queue,
        bool enableMultiQueue);
    ~RenderCommandList() = default;

    // called in render node graph if multi-queue gpu release acquire barriers have been patched
    void SetValidGpuQueueReleaseAcquireBarriers();

    BASE_NS::array_view<const RenderCommandWithType> GetRenderCommands() const;
    uint32_t GetRenderCommandCount() const;
    bool HasValidRenderCommands() const;
    GpuQueue GetGpuQueue() const;
    bool HasMultiRenderCommandListSubpasses() const;
    MultiRenderPassCommandListData GetMultiRenderCommandListData() const;
    // bindings are important for global descriptor set dependencies
    bool HasGlobalDescriptorSetBindings() const;

    BASE_NS::array_view<const CommandBarrier> GetCustomBarriers() const;
    BASE_NS::array_view<const VertexBuffer> GetRenderpassVertexInputBufferBarriers() const;
    BASE_NS::array_view<const VertexBuffer> GetRenderpassIndirectBufferBarriers() const;
    // for barriers
    BASE_NS::array_view<const RenderHandle> GetDescriptorSetHandles() const;
    BASE_NS::array_view<const RenderHandle> GetUpdateDescriptorSetHandles() const;

    // reset buffers and data
    void BeginFrame();

    // preparations if needed
    void BeforeRenderNodeExecuteFrame();
    // clean-up if needed
    void AfterRenderNodeExecuteFrame();

    void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) override;
    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t firstIndex, int32_t vertexOffset,
        uint32_t firstInstance) override;
    void DrawIndirect(RenderHandle bufferHandle, uint32_t offset, uint32_t drawCount, uint32_t stride) override;
    void DrawIndexedIndirect(RenderHandle bufferHandle, uint32_t offset, uint32_t drawCount, uint32_t stride) override;

    void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) override;
    void DispatchIndirect(RenderHandle bufferHandle, uint32_t offset) override;

    void BindPipeline(RenderHandle psoHandle) override;

    void PushConstantData(
        const struct RENDER_NS::PushConstant& pushConstant, BASE_NS::array_view<const uint8_t> data) override;
    void PushConstant(const struct RENDER_NS::PushConstant& pushConstant, const uint8_t* data) override;

    void BindVertexBuffers(BASE_NS::array_view<const VertexBuffer> vertexBuffers) override;
    void BindIndexBuffer(const IndexBuffer& indexBuffer) override;

    void BeginRenderPass(
        const RenderPassDesc& renderPassDesc, BASE_NS::array_view<const RenderPassSubpassDesc> subpassDescs) override;
    void BeginRenderPass(const RenderPassDesc& renderPassDesc, uint32_t subpassStartIdx,
        const RenderPassSubpassDesc& subpassDescs) override;
    void EndRenderPass() override;

    void NextSubpass(const SubpassContents& subpassContents) override;

    void BeginDisableAutomaticBarrierPoints() override;
    void EndDisableAutomaticBarrierPoints() override;
    void AddCustomBarrierPoint() override;

    void CustomMemoryBarrier(const GeneralBarrier& source, const GeneralBarrier& destination) override;
    void CustomBufferBarrier(RenderHandle handle, const BufferResourceBarrier& source,
        const BufferResourceBarrier& destination, uint32_t byteOffset, uint32_t byteSize) override;
    void CustomImageBarrier(RenderHandle handle, const ImageResourceBarrier& destination,
        const ImageSubresourceRange& imageSubresourceRange) override;
    void CustomImageBarrier(RenderHandle handle, const ImageResourceBarrier& source,
        const ImageResourceBarrier& destination, const ImageSubresourceRange& imageSubresourceRange) override;

    void CopyBufferToBuffer(
        RenderHandle sourceHandle, RenderHandle destinationHandle, const BufferCopy& bufferCopy) override;
    void CopyBufferToImage(
        RenderHandle sourceHandle, RenderHandle destinationHandle, const BufferImageCopy& bufferImageCopy) override;
    void CopyImageToBuffer(
        RenderHandle sourceHandle, RenderHandle destinationHandle, const BufferImageCopy& bufferImageCopy) override;
    void CopyImageToImage(
        RenderHandle sourceHandle, RenderHandle destinationHandle, const ImageCopy& imageCopy) override;

    void BlitImage(RenderHandle sourceImageHandle, RenderHandle destinationImageHandle, const ImageBlit& imageBlit,
        Filter filter) override;

    void UpdateDescriptorSet(RenderHandle handle, const DescriptorSetLayoutBindingResources& bindingResources) override;
    void UpdateDescriptorSets(BASE_NS::array_view<const RenderHandle> handles,
        BASE_NS::array_view<const DescriptorSetLayoutBindingResources> bindingResources) override;
    void BindDescriptorSet(uint32_t set, RenderHandle handle) override;
    void BindDescriptorSet(
        uint32_t set, RenderHandle handle, BASE_NS::array_view<const uint32_t> dynamicOffsets) override;
    void BindDescriptorSets(uint32_t firstSet, BASE_NS::array_view<const RenderHandle> handles) override;
    void BindDescriptorSet(uint32_t set, const BindDescriptorSetData& desriptorSetData) override;
    void BindDescriptorSets(
        uint32_t firstSet, BASE_NS::array_view<const BindDescriptorSetData> descriptorSetData) override;

    void BuildAccelerationStructures(const AsBuildGeometryData& geometry,
        BASE_NS::array_view<const AsGeometryTrianglesData> triangles,
        BASE_NS::array_view<const AsGeometryAabbsData> aabbs,
        BASE_NS::array_view<const AsGeometryInstancesData> instances) override;
    void CopyAccelerationStructureInstances(
        const BufferOffset& destinationBufferHandle, BASE_NS::array_view<const AsInstance> instances) override;

    void ClearColorImage(
        RenderHandle handle, ClearColorValue color, BASE_NS::array_view<const ImageSubresourceRange> ranges) override;

    // dynamic states
    void SetDynamicStateViewport(const ViewportDesc& viewportDesc) override;
    void SetDynamicStateScissor(const ScissorDesc& scissorDesc) override;
    void SetDynamicStateLineWidth(float lineWidth) override;
    void SetDynamicStateDepthBias(
        float depthBiasConstantFactor, float depthBiasClamp, float depthBiasSlopeFactor) override;
    void SetDynamicStateBlendConstants(BASE_NS::array_view<const float> blendConstants) override;
    void SetDynamicStateDepthBounds(float minDepthBounds, float maxDepthBounds) override;
    void SetDynamicStateStencilCompareMask(StencilFaceFlags faceMask, uint32_t compareMask) override;
    void SetDynamicStateStencilWriteMask(StencilFaceFlags faceMask, uint32_t writeMask) override;
    void SetDynamicStateStencilReference(StencilFaceFlags faceMask, uint32_t reference) override;
    void SetDynamicStateFragmentShadingRate(
        const Size2D& fragmentSize, const FragmentShadingRateCombinerOps& combinerOps) override;

    void SetExecuteBackendFramePosition() override;
    void SetExecuteBackendCommand(IRenderBackendCommand::Ptr backendCommand) override;
    void SetBackendCommand(IRenderBackendPositionCommand::Ptr backendCommand,
        RenderBackendCommandPosition backendCommandPosition) override;
    BASE_NS::array_view<ProcessBackendCommand> GetProcessBackendCommands();

    void BeginDebugMarker(BASE_NS::string_view name) override;
    void BeginDebugMarker(BASE_NS::string_view name, BASE_NS::Math::Vec4 color) override;
    void EndDebugMarker() override;

    // IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
    const BASE_NS::string nodeName_;
#if (RENDER_VALIDATION_ENABLED == 1)
    // used for validation
    const GpuResourceManager& gpuResourceMgr_;
    const NodeContextPsoManager& psoMgr_;
#endif
    NodeContextDescriptorSetManager& nodeContextDescriptorSetManager_;

    // add barrier/synchronization point where descriptor resources need to be synchronized
    // on gfx this happens before BeginRenderPass()
    // on compute this happens before Dispatch -methods
    void AddBarrierPoint(RenderCommandType renderCommandType);

    bool ProcessInputAttachments(const RenderPassDesc& renderPassDsc, const RenderPassSubpassDesc& subpassRef,
        RenderPassAttachmentResourceStates& subpassResourceStates);
    bool ProcessColorAttachments(const RenderPassDesc& renderPassDsc, const RenderPassSubpassDesc& subpassRef,
        RenderPassAttachmentResourceStates& subpassResourceStates);
    bool ProcessResolveAttachments(const RenderPassDesc& renderPassDsc, const RenderPassSubpassDesc& subpassRef,
        RenderPassAttachmentResourceStates& subpassResourceStates);
    bool ProcessDepthAttachments(const RenderPassDesc& renderPassDsc, const RenderPassSubpassDesc& subpassRef,
        RenderPassAttachmentResourceStates& subpassResourceStates);
    bool ProcessFragmentShadingRateAttachments(const RenderPassDesc& renderPassDsc,
        const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates);

    void ValidateRenderPass(const RenderPassDesc& renderPassDesc);
    void ValidatePipeline();
    void ValidatePipelineLayout();

    struct DescriptorSetBind {
        RenderHandle descriptorSetHandle;
        bool hasDynamicBarrierResources { false };
    };
    struct CustomBarrierIndices {
        int32_t prevSize { 0 };
        bool dirtyCustomBarriers { false };
    };

    // state data is mostly for validation
    struct StateData {
        bool validCommandList { true };
        bool validPso { false };
        bool renderPassHasBegun { false };
        bool automaticBarriersEnabled { true };
        bool executeBackendFrameSet { false };

        bool dirtyDescriptorSetsForBarriers { false };

        uint32_t renderPassStartIndex { 0u };
        uint32_t renderPassSubpassCount { 0u };

        uint32_t currentBarrierPointIndex { 0u };

        // handle for validation
        RenderHandle currentPsoHandle;
        bool checkBindPipelineLayout { false };
        PipelineBindPoint currentPsoBindPoint { PipelineBindPoint::CORE_PIPELINE_BIND_POINT_GRAPHICS };

        uint32_t currentBoundSetsMask { 0u }; // bitmask for all descriptor sets
        DescriptorSetBind currentBoundSets[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
        CustomBarrierIndices currentCustomBarrierIndices;

        RenderCommandBarrierPoint* currentBarrierPoint { nullptr };
    };
    StateData stateData_;

    void ResetStateData()
    {
        stateData_ = {};
    }

    GpuQueue gpuQueue_;
    // if device has enabled multiple queues this is true
    bool enableMultiQueue_ { false };
    // true if valid multi-queue gpu resource transfers are created in render node graph
    bool validReleaseAcquire_ { false };
    // true if has any global descriptor set bindings, needed for global descriptor set dependencies
    bool hadGlobalDescriptorSetBindings_ { false };

    // true if render pass has been begun with subpasscount > 1 and not all subpasses given
    bool hasMultiRpCommandListSubpasses_ { false };
    MultiRenderPassCommandListData multiRpCommandListData_ {};

    BASE_NS::vector<RenderCommandWithType> renderCommands_;

    // store all custom barriers to this list and provide indices in "barrier point"
    // barriers are batched in this point and command is commit to the real GPU command buffer
    BASE_NS::vector<CommandBarrier> customBarriers_;

    // store all vertex and index buffer barriers to this list and provide indices in "barrier point"
    // barriers are batched in this point and command is commit to the real GPU command buffer
    // obviously barriers are only needed in begin render pass barrier point
    BASE_NS::vector<VertexBuffer> rpVertexInputBufferBarriers_;

    // store all renderpass draw indirect barriers to this list and provide indices in "barrier point"
    // barriers are batched in this point and command is commit to the real GPU command buffer
    // obviously barriers are only needed in begin render pass barrier point
    BASE_NS::vector<VertexBuffer> rpIndirectBufferBarriers_;

    // store all descriptor set handles to this list and provide indices in "barrier point"
    // barriers are batched in this point and command is commit to the real GPU command buffer
    BASE_NS::vector<RenderHandle> descriptorSetHandlesForBarriers_;

    // store all descriptor set handles which are updated to this list
    BASE_NS::vector<RenderHandle> descriptorSetHandlesForUpdates_;

    // stores all backend commands, ptr used in command list
    BASE_NS::vector<IRenderBackendCommand::Ptr> backendCommands_;
    // process backend commands
    BASE_NS::vector<ProcessBackendCommand> processBackendCommands_;

    // linear allocator for render command list commands
    LinearAllocatorStruct allocator_;

#if (RENDER_DEBUG_MARKERS_ENABLED == 1)
    // need to end at some point
    struct DebugMarkerStack {
        uint32_t stackCounter { 0U };
        uint32_t commandCount { 0U }; // to prevent command list backend processing with only debug markers
    };
    DebugMarkerStack debugMarkerStack_;
#endif
};
RENDER_END_NAMESPACE()

#endif // CORE_RENDER_RENDER_COMMAND_LIST_H
