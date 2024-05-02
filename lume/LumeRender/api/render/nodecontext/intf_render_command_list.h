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

#ifndef API_RENDER_IRENDER_COMMAND_LIST_H
#define API_RENDER_IRENDER_COMMAND_LIST_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>
#include <render/device/gpu_resource_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
struct RenderPassDesc;
struct RenderPassSubpassDesc;
struct PushConstant;
struct IndexBuffer;
struct VertexBuffer;
struct ImageBlit;

/** @ingroup group_render_irendercommandlist */
/** Call methods to add render commands to a render command list.
 * Render command list is unique for every render node.
 * Most of the methods and their inputs are familiar from different graphics APIs.
 *
 * RenderCommandList does not do heavy processing on unnecessary state changes etc.
 * Prefer setting data, bindings etc. only once and do not add empty draw and/or dispatches.
 */
class IRenderCommandList : public CORE_NS::IInterface {
public:
    static constexpr auto UID = BASE_NS::Uid("744d8a3c-82cd-44f5-9fcb-769b33ceca1b");

    /** Can only be called inside of a render pass.
     * @param vertexCount Vertex count
     * @param instanceCount Instance count
     * @param firstVertex First vertex
     * @param firstInstance First instance
     */
    virtual void Draw(const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex,
        const uint32_t firstInstance) = 0;

    /** Can only be called inside of a render pass.
     * @param indexCount Index count
     * @param instanceCount Instance count
     * @param firstIndex First index
     * @param vertexOffset Vertex offset
     * @param firstInstance First instance
     */
    virtual void DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex,
        const int32_t vertexOffset, const uint32_t firstInstance) = 0;

    /** Draw with indirect arguments.
     * Can only be called inside of a render pass.
     * @param bufferHandle Buffer handle
     * @param offset Offset
     * @param drawCount Draw count
     * @param stride Stride
     */
    virtual void DrawIndirect(
        const RenderHandle bufferHandle, const uint32_t offset, const uint32_t drawCount, const uint32_t stride) = 0;

    /** Can only be called inside of a render pass.
     * @param bufferHandle Buffer handle
     * @param offset Offset
     * @param drawCount Draw count
     * @param stride Stride
     */
    virtual void DrawIndexedIndirect(
        const RenderHandle bufferHandle, const uint32_t offset, const uint32_t drawCount, const uint32_t stride) = 0;

    /** Dispatch a compute program.
     * @param groupCountX Group count x
     * @param groupCountY Group count y
     * @param groupCountZ Group count z
     */
    virtual void Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ) = 0;

    /** Dispatch a compute program with indirect arguments.
     * @param bufferHandle Buffer handle
     * @param offset Offset
     */
    virtual void DispatchIndirect(const RenderHandle bufferHandle, const uint32_t offset) = 0;

    /** Bind pipeline
     * @param psoHandle PSO handle
     */
    virtual void BindPipeline(const RenderHandle psoHandle) = 0;

    /** Push constant. The data is copied.
     * @param pushConstant Push constant
     * @param data Data
     */
    virtual void PushConstantData(
        const struct PushConstant& pushConstant, const BASE_NS::array_view<const uint8_t> data) = 0;

    /** Push constant. The data is copied. Prefer using PushConstantData -method. This will be deprecated.
     * @param pushConstant Push constant
     * @param data Data
     */
    virtual void PushConstant(const struct PushConstant& pushConstant, const uint8_t* data) = 0;

    /** Bind vertex buffers
     * @param vertexBuffers Vertex buffers
     */
    virtual void BindVertexBuffers(const BASE_NS::array_view<const VertexBuffer> vertexBuffers) = 0;

    /** Bind index buffer
     * @param indexBuffer Index buffer
     */
    virtual void BindIndexBuffer(const IndexBuffer& indexBuffer) = 0;

    /** Draw-calls can only be made inside of a render pass.
     * A render pass definition without content. Render pass needs to have
     * SubpassContents::CORE_SUBPASS_CONTENTS_INLINE set.
     * All subpasses given with subpassDescs.
     * @param renderPassDesc Render pass descriptor
     * @param subpassDescs Subpass descriptors
     */
    virtual void BeginRenderPass(
        const RenderPassDesc& renderPassDesc, const BASE_NS::array_view<const RenderPassSubpassDesc> subpassDescs) = 0;

    /** Draw-calls can only be made inside of a render pass.
     * A render pass definition without content. Render pass needs to have
     * SubpassContents::CORE_SUBPASS_CONTENTS_INLINE set.
     * Subpasses are patched to this render pass from other render command lists, if renderPassDesc.subpassCount > 1.
     * There cannot be multiple "open" render passes from other render command lists.
     * @param renderPassDesc Render pass descriptor
     * @param subpassStartIndex Subpass start index
     * @param subpassDesc Subpass descriptor
     */
    virtual void BeginRenderPass(const RenderPassDesc& renderPassDesc, const uint32_t subpassStartIndex,
        const RenderPassSubpassDesc& subpassDesc) = 0;

    /** Must be called when a render pass ends. */
    virtual void EndRenderPass() = 0;

    /** Next subpass
     * @param subpassContents subpass contents
     */
    virtual void NextSubpass(const SubpassContents& subpassContents) = 0;

    /** This will disable automatic barriers until EndExecuteAutomaticBarriers is called.
     * This will add a barrier point for current pending barriers.
     * Use when manually adding barriers and executing them in a single spot.
     * In addition, disables initial image layouts in render passes and uses undefined layouts.
     */
    virtual void BeginDisableAutomaticBarrierPoints() = 0;

    /** Re-enable automatic barriers.
     * Needs to be called after BeginDisableAutomaticBarrierPoints to re-enable automatic barriers.
     */
    virtual void EndDisableAutomaticBarrierPoints() = 0;

    /** This will add custom barrier point where all pending (custom) barriers will be executed.
     */
    virtual void AddCustomBarrierPoint() = 0;

    /** Add custom memory barrier.
     * Can be used for a memory barrier.
     * @param src General barrier source state
     * @param dst General barrier destination state
     */
    virtual void CustomMemoryBarrier(const GeneralBarrier& src, const GeneralBarrier& dst) = 0;

    /** Add custom barriers for a GPU buffer.
     * Can only be called outside of render pass.
     * @param handle Handle to resource
     * @param src Source buffer resource barrier
     * @param dst Destination buffer resource barrier
     * @param byteOffset Byte offset
     * @param byteSize Byte size (use PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE for the whole buffer)
     */
    virtual void CustomBufferBarrier(const RenderHandle handle, const BufferResourceBarrier& src,
        const BufferResourceBarrier& dst, const uint32_t byteOffset, const uint32_t byteSize) = 0;

    /** Add custom barriers for a GPU image.
     * Can only be called outside of render pass.
     * Checks the current/correct state and layout, and updates to new given dst state.
     * @param handle Handle to resource
     * @param dst Destination image resource barrier
     * @param imageSubresourceRange ImageSubresourceRange
     */
    virtual void CustomImageBarrier(const RenderHandle handle, const ImageResourceBarrier& dst,
        const ImageSubresourceRange& imageSubresourceRange) = 0;
    /** Add custom barriers for a GPU image.
     * Can only be called outside of render pass.
     * Does not check the current/correct state and layout. Can be used e.g. as a barrier from undefined state.
     * @param handle Handle to resource
     * @param src Source image resource barrier
     * @param dst Destination image resource barrier
     * @param imageSubresourceRange ImageSubresourceRange
     */
    virtual void CustomImageBarrier(const RenderHandle handle, const ImageResourceBarrier& src,
        const ImageResourceBarrier& dst, const ImageSubresourceRange& imageSubresourceRange) = 0;

    /** Copy buffer to buffer
     * @param srcHandle Source handle
     * @param dstHandle Destination handle
     * @param bufferCopy Buffer copy
     */
    virtual void CopyBufferToBuffer(
        const RenderHandle srcHandle, const RenderHandle dstHandle, const BufferCopy& bufferCopy) = 0;

    /** Copy buffer to image
     * @param srcHandle Source handle
     * @param dstHandle Destination handle
     * @param bufferImageCopy Buffer image copy
     */
    virtual void CopyBufferToImage(
        const RenderHandle srcHandle, const RenderHandle dstHandle, const BufferImageCopy& bufferImageCopy) = 0;

    /** Copy image to buffer
     * @param srcHandle Source handle
     * @param dstHandle Destination handle
     * @param bufferImageCopy Buffer image copy
     */
    virtual void CopyImageToBuffer(
        const RenderHandle srcHandle, const RenderHandle dstHandle, const BufferImageCopy& bufferImageCopy) = 0;

    /** Copy image to image
     * @param srcHandle Source handle
     * @param dstHandle Destination handle
     * @param imageCopy Image copy
     */
    virtual void CopyImageToImage(
        const RenderHandle srcHandle, const RenderHandle dstHandle, const ImageCopy& imageCopy) = 0;

    /** Blit source image to destination image.
     * Can only be called outside of render pass.
     * @param srcImageHandle Source handle
     * @param dstImageHandle Destination handle
     * @param imageBlit Image blit
     * @param filter Filtering mode
     */
    virtual void BlitImage(const RenderHandle srcImageHandle, const RenderHandle dstImageHandle,
        const ImageBlit& imageBlit, const Filter filter) = 0;

    /** Update a single descriptor set with given bindings.
     * @param handle Handle used in update
     * @param bindingResources Binding resources
     */
    virtual void UpdateDescriptorSet(
        const RenderHandle handle, const DescriptorSetLayoutBindingResources& bindingResources) = 0;

    /** Update descriptor sets with given bindings. Array view sizes must match.
     * @param handles Handles for descriptor sets
     * @param bindingResources Binding resources for descriptor sets
     */
    virtual void UpdateDescriptorSets(const BASE_NS::array_view<const RenderHandle> handles,
        const BASE_NS::array_view<const DescriptorSetLayoutBindingResources> bindingResources) = 0;

    /** Bind a single descriptor set to pipeline.
     * There can be maximum of 4 sets. I.e. the maximum set index is 3.
     * @param set Set to bind
     * @param handle Handle to resource
     */
    virtual void BindDescriptorSet(const uint32_t set, const RenderHandle handle) = 0;

    /** Bind a single descriptor set to pipeline.
     * There can be maximum of 4 sets. I.e. the maximum set index is 3.
     * @param set Set to bind
     * @param handle Handle to resource
     * @param dynamicOffsets Byte offsets for dynamic descriptors
     */
    virtual void BindDescriptorSet(
        const uint32_t set, const RenderHandle handle, const BASE_NS::array_view<const uint32_t> dynamicOffsets) = 0;

    /** Bind multiple descriptor sets to pipeline.
     * There can be maximum of 4 sets. I.e. the maximum set index is 3.
     * Descriptor sets needs to be a contiguous set.
     * @param firstSet First set index
     * @param handles Handles to resources
     */
    virtual void BindDescriptorSets(const uint32_t firstSet, const BASE_NS::array_view<const RenderHandle> handles) = 0;

    /** BindDescriptorSetData
     */
    struct BindDescriptorSetData {
        /** Descriptor set handle */
        RenderHandle handle;
        /** Descriptor set dynamic buffer offsets */
        BASE_NS::array_view<const uint32_t> dynamicOffsets;
    };

    /** Bind a single descriptor set to pipeline.
     * There can be maximum of 4 sets. I.e. the maximum set index is 3.
     * @param set Set to bind
     * @param descriptorSetData Descriptor set data
     */
    virtual void BindDescriptorSet(const uint32_t set, const BindDescriptorSetData& desriptorSetData) = 0;

    /** Bind multiple descriptor sets to pipeline some with dynamic offsets.
     * There can be maximum of 4 sets. I.e. the maximum set index is 3.
     * Descriptor sets needs to be a contiguous set.
     * @param firstSet First set index
     * @param descriptorSetData Descriptor set data
     */
    virtual void BindDescriptorSets(
        const uint32_t firstSet, const BASE_NS::array_view<const BindDescriptorSetData> descriptorSetData) = 0;

    /** Build acceleration structures
     * @param geometry Acceleration structure build geometry data
     * @param triangles Geometry triangles
     * @param aabbs Geometry aabbs
     * @param instances Geometry instances
     */
    virtual void BuildAccelerationStructures(const AccelerationStructureBuildGeometryData& geometry,
        const BASE_NS::array_view<const AccelerationStructureGeometryTrianglesData> triangles,
        const BASE_NS::array_view<const AccelerationStructureGeometryAabbsData> aabbs,
        const BASE_NS::array_view<const AccelerationStructureGeometryInstancesData> instances) = 0;

    /** Clear color image.
     * This should only be needed when initializing images to some values.
     * Often render pass attachment clears and shader clears are needed.
     * Some backends might not support this, i.e. one might need to use higher level paths for e.g. OpenGLES
     * @param handle Handle of the image
     * @param color Clear color values
     * @param ranges Array view of image subresource ranges
     */
    virtual void ClearColorImage(const RenderHandle handle, const ClearColorValue color,
        const BASE_NS::array_view<const ImageSubresourceRange> ranges) = 0;

    /** Set dynamic state viewport
     * @param viewportDesc Viewport descriptor
     */
    virtual void SetDynamicStateViewport(const ViewportDesc& viewportDesc) = 0;

    /** Set dynamic state scissor
     * @param scissorDesc Scissor descriptor
     */
    virtual void SetDynamicStateScissor(const ScissorDesc& scissorDesc) = 0;

    /** Set dynamic state line width
     * @param lineWidth Line width
     */
    virtual void SetDynamicStateLineWidth(const float lineWidth) = 0;

    /** Set dynamic state depth bias
     * @param depthBiasConstantFactor Depth bias constant factor
     * @param depthBiasClamp Depth bias clamp
     * @param depthBiasSlopeFactor Depth bias slope factor
     */
    virtual void SetDynamicStateDepthBias(
        const float depthBiasConstantFactor, const float depthBiasClamp, const float depthBiasSlopeFactor) = 0;

    /** Set dynamic state blend constants
     * @param blendConstants Blend constants
     */
    virtual void SetDynamicStateBlendConstants(const BASE_NS::array_view<const float> blendConstants) = 0;

    /** Set dynamic state depth bounds
     * @param minDepthBounds Min depth bounds
     * @param maxDepthBounds Max depth bounds
     */
    virtual void SetDynamicStateDepthBounds(const float minDepthBounds, const float maxDepthBounds) = 0;

    /** Set dynamic state stencil compare mask
     * @param faceMask Face mask
     * @param compareMask Compare mask
     */
    virtual void SetDynamicStateStencilCompareMask(const StencilFaceFlags faceMask, const uint32_t compareMask) = 0;

    /** Set dynamic state stencil write mask
     * @param faceMask Face mask
     * @param writeMask Write mask
     */
    virtual void SetDynamicStateStencilWriteMask(const StencilFaceFlags faceMask, const uint32_t writeMask) = 0;

    /** Set dynamic state stencil reference
     * @param faceMask Face mask
     * @param reference Reference
     */
    virtual void SetDynamicStateStencilReference(const StencilFaceFlags faceMask, const uint32_t reference) = 0;

    /** Set dynamic state fragmend shading rate.
     * @param fragmentSize Fragment size, pipeline fragment shading rate. (Valid values 1, 2, 4)
     * @param combinerOps Combiner operations
     */
    virtual void SetDynamicStateFragmentShadingRate(
        const Size2D& fragmentSize, const FragmentShadingRateCombinerOps& combinerOps) = 0;

    /** Set execute backend frame position. The position where the method is ran.
     * Often this might be the only command in the render command list, when using backend nodes.
     * This can be set only once during render command list setup per frame.
     */
    virtual void SetExecuteBackendFramePosition() = 0;

protected:
    IRenderCommandList() = default;
    virtual ~IRenderCommandList() = default;

    IRenderCommandList(const IRenderCommandList&) = delete;
    IRenderCommandList& operator=(const IRenderCommandList&) = delete;
    IRenderCommandList(IRenderCommandList&&) = delete;
    IRenderCommandList& operator=(IRenderCommandList&&) = delete;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_COMMAND_LIST_H
