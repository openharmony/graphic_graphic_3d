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

#include "render_command_list.h"

#include <cinttypes>
#include <cstdint>

#include <base/containers/array_view.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/render_data_structures.h>

#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "nodecontext/node_context_descriptor_set_manager.h"
#include "nodecontext/node_context_pso_manager.h"
#include "nodecontext/render_node_context_manager.h"
#include "util/linear_allocator.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
PLUGIN_STATIC_ASSERT(PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT == 4);
PLUGIN_STATIC_ASSERT(PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT == 8u);
namespace {
#if (RENDER_VALIDATION_ENABLED == 1)
void ValidateImageUsageFlags(const string_view nodeName, const GpuResourceManager& gpuResourceMgr,
    const RenderHandle handl, const ImageUsageFlags imageUsageFlags, const string_view str)
{
    if ((gpuResourceMgr.GetImageDescriptor(handl).usageFlags & imageUsageFlags) == 0) {
        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateImageUsageFlags_",
            "RENDER_VALIDATION: gpu image (handle: %" PRIu64
            ") (name: %s), not created with needed flags: %s, (node: %s)",
            handl.id, gpuResourceMgr.GetName(handl).c_str(), str.data(), nodeName.data());
    }
}

void ValidateBufferUsageFlags(const string_view nodeName, const GpuResourceManager& gpuResourceMgr,
    const RenderHandle handl, const BufferUsageFlags bufferUsageFlags, const string_view str)
{
    if ((gpuResourceMgr.GetBufferDescriptor(handl).usageFlags & bufferUsageFlags) == 0) {
        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateBufferUsageFlags_",
            "RENDER_VALIDATION: gpu buffer (handle: %" PRIu64
            ") (name: %s), not created with needed flags: %s, (node: %s)",
            handl.id, gpuResourceMgr.GetName(handl).c_str(), str.data(), nodeName.data());
    }
}

void ValidateDescriptorTypeBinding(const string_view nodeName, const GpuResourceManager& gpuResourceMgr,
    const DescriptorSetLayoutBindingResources& bindingRes)
{
    for (const auto& ref : bindingRes.buffers) {
        if (!RenderHandleUtil::IsGpuBuffer(ref.resource.handle)) {
            PLUGIN_LOG_E("RENDER_VALIDATION: invalid GPU buffer");
        }
        if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) {
            ValidateBufferUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle,
                CORE_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT, "CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
            ValidateBufferUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle,
                CORE_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT, "CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            ValidateBufferUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle,
                CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            ValidateBufferUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle,
                CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT, "CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            ValidateBufferUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle,
                CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, "CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            ValidateBufferUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle,
                CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT, "CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE) {
        } else {
            PLUGIN_LOG_E("RENDER_VALIDATION: unsupported buffer descriptor type: %u", ref.binding.descriptorType);
        }
    }
    for (const auto& ref : bindingRes.images) {
        if ((ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
            (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE)) {
            ValidateImageUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle, CORE_IMAGE_USAGE_SAMPLED_BIT,
                "CORE_IMAGE_USAGE_SAMPLED_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            ValidateImageUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle, CORE_IMAGE_USAGE_STORAGE_BIT,
                "CORE_IMAGE_USAGE_STORAGE_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
            ValidateImageUsageFlags(nodeName, gpuResourceMgr, ref.resource.handle,
                CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, "CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
        } else {
            PLUGIN_LOG_E("RENDER_VALIDATION: unsupported image descriptor type: %u", ref.binding.descriptorType);
        }
    }
    for (const auto& ref : bindingRes.samplers) {
        if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLER) {
        } else {
            PLUGIN_LOG_E("RENDER_VALIDATION: unsupported sampler descriptor type: %u", ref.binding.descriptorType);
        }
    }
}

void ValidateRenderPassAttachment(const string_view nodeName, const GpuResourceManager& gpuResourceMgr,
    const RenderPassDesc& renderPassDesc, const array_view<const RenderPassSubpassDesc> subpassDescs)
{
    const GpuImageDesc baseDesc = gpuResourceMgr.GetImageDescriptor(renderPassDesc.attachmentHandles[0]);
    const uint32_t baseWidth = baseDesc.width;
    const uint32_t baseHeight = baseDesc.height;
    // NOTE: we do not check fragment shading rate attachment size
    for (uint32_t attachmentIdx = 1; attachmentIdx < renderPassDesc.attachmentCount; ++attachmentIdx) {
        const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(renderPassDesc.attachmentHandles[attachmentIdx]);
        if (desc.width != baseWidth || desc.height != baseHeight) {
            for (const auto& subpassRef : subpassDescs) {
                auto CheckAttachments = [](const auto& indices, const uint32_t count, const uint32_t attachmentIndex) {
                    for (uint32_t idx = 0; idx < count; ++idx) {
                        if (indices[idx] == attachmentIndex) {
                            return false;
                        }
                    }
                    return true;
                };
                bool valid = true;
                valid &=
                    CheckAttachments(subpassRef.colorAttachmentIndices, subpassRef.colorAttachmentCount, attachmentIdx);
                valid &=
                    CheckAttachments(subpassRef.inputAttachmentIndices, subpassRef.inputAttachmentCount, attachmentIdx);
                valid &= CheckAttachments(
                    subpassRef.resolveAttachmentIndices, subpassRef.resolveAttachmentCount, attachmentIdx);
                if ((subpassRef.depthAttachmentIndex == attachmentIdx) ||
                    (subpassRef.depthResolveAttachmentIndex == attachmentIdx)) {
                    valid = false;
                }
                if (!valid) {
                    if (RenderHandleUtil::IsSwapchain(renderPassDesc.attachmentHandles[attachmentIdx]) &&
                        RenderHandleUtil::IsDepthImage(renderPassDesc.attachmentHandles[0])) {
                        PLUGIN_LOG_ONCE_W(nodeName + "_RCL_ValidateSize1_",
                            "RENDER_VALIDATION: Depth and swapchain input missmatch: baseWidth:%u baseHeight:%u "
                            "currWidth:%u currHeight:%u",
                            baseWidth, baseHeight, desc.width, desc.height);
                    } else {
                        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateSize1_",
                            "RENDER_VALIDATION: render pass attachment size does not match with attachment index: %u",
                            attachmentIdx);
                        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateSize1_",
                            "RENDER_VALIDATION: baseWidth:%u baseHeight:%u currWidth:%u currHeight:%u", baseWidth,
                            baseHeight, desc.width, desc.height);
                    }
                }
            }
        }
    }
    if ((renderPassDesc.renderArea.extentWidth == 0) || (renderPassDesc.renderArea.extentHeight == 0)) {
        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateRaExtent_",
            "RENDER_VALIDATION: render area cannot be zero (width: %u, height: %u) (node: %s)",
            renderPassDesc.renderArea.extentWidth, renderPassDesc.renderArea.extentHeight, nodeName.data());
    }
    if ((renderPassDesc.renderArea.offsetX >= static_cast<int32_t>(baseWidth)) ||
        (renderPassDesc.renderArea.offsetY >= static_cast<int32_t>(baseHeight))) {
        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateRaOffset_",
            "RENDER_VALIDATION: render area offset cannot go out of screen (offsetX: %i, offsetY: %i) (baseWidth: "
            "%u, baseHeight: %u, (node: %s)",
            renderPassDesc.renderArea.offsetX, renderPassDesc.renderArea.offsetY, baseWidth, baseHeight,
            nodeName.data());
    }
}

void ValidateImageSubresourceRange(const GpuResourceManager& gpuResourceMgr, const RenderHandle handle,
    const ImageSubresourceRange& imageSubresourceRange)
{
    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(handle);
    if (imageSubresourceRange.baseMipLevel >= desc.mipCount) {
        PLUGIN_LOG_E("RENDER_VALIDATION : ImageSubresourceRange mipLevel: %u, is greater or equal to mipCount: %u",
            imageSubresourceRange.baseMipLevel, desc.mipCount);
    }
    if (imageSubresourceRange.baseArrayLayer >= desc.layerCount) {
        PLUGIN_LOG_E("RENDER_VALIDATION : ImageSubresourceRange layer: %u, is greater or equal to layerCount: %u",
            imageSubresourceRange.baseArrayLayer, desc.layerCount);
    }
}

void ValidateViewport(const string_view nodeName, const ViewportDesc& vd)
{
    if ((vd.width < 1.0f) || (vd.height < 1.0f)) {
        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateViewport_",
            "RENDER_VALIDATION : viewport width (%f) and height (%f) must be one or larger (node: %s)", vd.width,
            vd.height, nodeName.data());
    }
}

void ValidateScissor(const string_view nodeName, const ScissorDesc& sd)
{
    if ((sd.extentWidth == 0) || (sd.extentHeight == 0)) {
        PLUGIN_LOG_ONCE_E(nodeName + "_RCL_ValidateScissor_",
            "RENDER_VALIDATION : scissor extentWidth (%u) and scissor extentHeight (%u) cannot be zero (node: %s)",
            sd.extentWidth, sd.extentHeight, nodeName.data());
    }
}

void ValidateFragmentShadingRate(const Size2D& size)
{
    bool valid = true;
    if ((size.width == 0) || (size.height == 0)) {
        valid = false;
    } else if ((size.width == 3u) || (size.height == 3u)) {
        valid = false;
    } else if ((size.width > 4u) || (size.height > 4u)) {
        valid = false;
    }
    if (!valid) {
        PLUGIN_LOG_W("RENDER_VALIDATION_ENABLED: fragmentSize must be less than or equal to 4 and the value must be a "
                     "power of two (width = %u, height = %u)",
            size.width, size.height);
    }
}
#endif // RENDER_VALIDATION_ENABLED

constexpr uint32_t INVALID_CL_IDX { ~0u };

constexpr size_t BYTE_SIZE_ALIGNMENT { 64 };
constexpr size_t FRAME_RESERVE_EXTRA_DIVIDE { 8 };
constexpr size_t MIN_ALLOCATION_SIZE { 1024 * 2 };

// automatic acquire and release barriers
constexpr uint32_t INITIAL_MULTI_QUEUE_BARRIER_COUNT { 2u };

size_t GetAlignedBytesize(const size_t byteSize, const size_t alignment)
{
    return (byteSize + alignment - 1) & (~(alignment - 1));
}

void* AllocateRenderData(
    RenderCommandList::LinearAllocatorStruct& allocator, const size_t alignment, const size_t byteSz)
{
    PLUGIN_ASSERT(byteSz > 0);
    void* rc = nullptr;
    if (!allocator.allocators.empty()) {
        const size_t currentIndex = allocator.allocators.size() - 1;
        rc = allocator.allocators[currentIndex]->Allocate(byteSz, alignment);
    }

    if (rc == nullptr) { // current allocator is out of memory
        size_t allocatorByteSize = Math::max(MIN_ALLOCATION_SIZE, GetAlignedBytesize(byteSz, BYTE_SIZE_ALIGNMENT));
        const size_t currentIndex = allocator.allocators.size();
        if (currentIndex > 0) {
            allocatorByteSize =
                Math::max(allocatorByteSize, allocator.allocators[currentIndex - 1]->GetCurrentByteSize() * 2u);
        }
        allocator.allocators.push_back(make_unique<LinearAllocator>(allocatorByteSize));

        rc = allocator.allocators[currentIndex]->Allocate(byteSz, alignment);
        if (rc == nullptr) {
            PLUGIN_LOG_E("RenderCommandList: render command list allocation : out of memory");
            PLUGIN_ASSERT(false);
        }
    }
    return rc;
}

template<typename T>
T* AllocateRenderData(RenderCommandList::LinearAllocatorStruct& allocator, uint32_t count)
{
    return static_cast<T*>(AllocateRenderData(allocator, std::alignment_of<T>::value, sizeof(T) * count));
}

template<typename T>
T* AllocateRenderCommand(RenderCommandList::LinearAllocatorStruct& allocator)
{
    return static_cast<T*>(AllocateRenderData(allocator, std::alignment_of<T>::value, sizeof(T)));
}
} // namespace

RenderCommandList::RenderCommandList(const BASE_NS::string_view nodeName,
    NodeContextDescriptorSetManager& nodeContextDescriptorSetMgr, const GpuResourceManager& gpuResourceMgr,
    const NodeContextPsoManager& nodeContextPsoMgr, const GpuQueue& queue, const bool enableMultiQueue)
    : IRenderCommandList(), nodeName_(nodeName),
#if (RENDER_VALIDATION_ENABLED == 1)
      gpuResourceMgr_(gpuResourceMgr), psoMgr_(nodeContextPsoMgr),
#endif
      nodeContextDescriptorSetManager_(nodeContextDescriptorSetMgr), gpuQueue_(queue),
      enableMultiQueue_(enableMultiQueue)
{}

void RenderCommandList::BeginFrame()
{
    if (allocator_.allocators.size() == 1) { // size is good for this frame
        allocator_.allocators[0]->Reset();
    } else if (allocator_.allocators.size() > 1) {
        size_t fullByteSize = 0;
        size_t alignment = 0;
        for (auto& ref : allocator_.allocators) {
            fullByteSize += ref->GetCurrentByteSize();
            alignment = Math::max(alignment, (size_t)ref->GetAlignment());
            ref.reset();
        }
        allocator_.allocators.clear();

        // add some room for current frame allocation for new render commands
        const size_t extraBytes = Math::max(fullByteSize / FRAME_RESERVE_EXTRA_DIVIDE, BYTE_SIZE_ALIGNMENT);
        fullByteSize += extraBytes;

        // create new single allocation for combined previous size and some extra bytes
        const size_t memAllocationByteSize = GetAlignedBytesize(fullByteSize, BYTE_SIZE_ALIGNMENT);
        allocator_.allocators.push_back(make_unique<LinearAllocator>(memAllocationByteSize, alignment));
    }

    ResetStateData();

    const auto clearAndReserve = [](auto& vec) {
        const size_t count = vec.size();
        vec.clear();
        vec.reserve(count);
    };

    clearAndReserve(renderCommands_);
    clearAndReserve(customBarriers_);
    clearAndReserve(rpVertexInputBufferBarriers_);
    clearAndReserve(rpIndirectBufferBarriers_);
    clearAndReserve(descriptorSetHandlesForBarriers_);
    clearAndReserve(descriptorSetHandlesForUpdates_);

    validReleaseAcquire_ = false;
    hasMultiRpCommandListSubpasses_ = false;
    multiRpCommandListData_ = {};
}

void RenderCommandList::SetValidGpuQueueReleaseAcquireBarriers()
{
    if (enableMultiQueue_) {
        validReleaseAcquire_ = true;
    }
}

void RenderCommandList::BeforeRenderNodeExecuteFrame()
{
    // add possible barrier point for gpu queue transfer acquire
    if ((gpuQueue_.type != GpuQueue::QueueType::UNDEFINED) && enableMultiQueue_) {
        AddBarrierPoint(RenderCommandType::GPU_QUEUE_TRANSFER_ACQUIRE);
    }
}

void RenderCommandList::AfterRenderNodeExecuteFrame()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RENDER_VALIDATION: EndRenderPass() not called?");
    }
    if (!stateData_.automaticBarriersEnabled) {
        PLUGIN_LOG_E("RENDER_VALIDATION: EndDisableAutomaticBarriers() not called?");
    }
#endif

    if ((gpuQueue_.type != GpuQueue::QueueType::UNDEFINED) && enableMultiQueue_) {
        if (stateData_.currentCustomBarrierIndices.dirtyCustomBarriers) {
            AddBarrierPoint(RenderCommandType::BARRIER_POINT);
        }

        // add possible barrier point for gpu queue transfer release
        AddBarrierPoint(RenderCommandType::GPU_QUEUE_TRANSFER_RELEASE);
    }
}

array_view<const RenderCommandWithType> RenderCommandList::GetRenderCommands() const
{
    if ((!stateData_.validCommandList) || stateData_.renderPassHasBegun) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_GetRenderCommands_",
            "RenderCommandList: invalid state data in render command list (node: %s)", nodeName_.c_str());
#endif
        return {};
    } else {
        return array_view<const RenderCommandWithType>(renderCommands_.data(), renderCommands_.size());
    }
}

bool RenderCommandList::HasValidRenderCommands() const
{
    const uint32_t renderCommandCount = GetRenderCommandCount();
    bool valid = false;
    if (enableMultiQueue_) {
        if (renderCommandCount == INITIAL_MULTI_QUEUE_BARRIER_COUNT) { // only acquire and release barrier commands
            // if there are patched explicit resource barriers, we need to execute this cmdlist in the backend
            valid = validReleaseAcquire_;
        } else if (renderCommandCount > INITIAL_MULTI_QUEUE_BARRIER_COUNT) {
            valid = true;
        }
    } else {
        valid = (renderCommandCount > 0);
    }
    valid = valid && stateData_.validCommandList;

    return valid;
}

uint32_t RenderCommandList::GetRenderCommandCount() const
{
    return (uint32_t)renderCommands_.size();
}

GpuQueue RenderCommandList::GetGpuQueue() const
{
    return gpuQueue_;
}

bool RenderCommandList::HasMultiRenderCommandListSubpasses() const
{
    return hasMultiRpCommandListSubpasses_;
}

MultiRenderPassCommandListData RenderCommandList::GetMultiRenderCommandListData() const
{
    return multiRpCommandListData_;
}

array_view<const CommandBarrier> RenderCommandList::GetCustomBarriers() const
{
    return array_view<const CommandBarrier>(customBarriers_.data(), customBarriers_.size());
}

array_view<const VertexBuffer> RenderCommandList::GetRenderpassVertexInputBufferBarriers() const
{
    return array_view<const VertexBuffer>(rpVertexInputBufferBarriers_.data(), rpVertexInputBufferBarriers_.size());
}

array_view<const VertexBuffer> RenderCommandList::GetRenderpassIndirectBufferBarriers() const
{
    return array_view<const VertexBuffer>(rpIndirectBufferBarriers_.data(), rpIndirectBufferBarriers_.size());
}

array_view<const RenderHandle> RenderCommandList::GetDescriptorSetHandles() const
{
    return { descriptorSetHandlesForBarriers_.data(), descriptorSetHandlesForBarriers_.size() };
}

array_view<const RenderHandle> RenderCommandList::GetUpdateDescriptorSetHandles() const
{
    return { descriptorSetHandlesForUpdates_.data(), descriptorSetHandlesForUpdates_.size() };
}

void RenderCommandList::AddBarrierPoint(const RenderCommandType renderCommandType)
{
    if (!stateData_.automaticBarriersEnabled) {
        return; // no barrier point added
    }

    RenderCommandBarrierPoint* data = AllocateRenderCommand<RenderCommandBarrierPoint>(allocator_);
    if (data) {
        *data = {}; // zero initialize

        data->renderCommandType = renderCommandType;
        data->barrierPointIndex = stateData_.currentBarrierPointIndex++;

        // update new index (within render pass there might not be any dirty descriptor sets at this stage)
        const uint32_t descriptorSetBeginIndex = static_cast<uint32_t>(descriptorSetHandlesForBarriers_.size());
        data->descriptorSetHandleIndexBegin = descriptorSetBeginIndex;
        data->descriptorSetHandleCount = 0U;
        // update new index (only valid with render pass)
        data->vertexIndexBarrierIndexBegin = static_cast<uint32_t>(rpVertexInputBufferBarriers_.size());
        data->vertexIndexBarrierCount = 0U;
        // update new index (only valid with render pass)
        data->indirectBufferBarrierIndexBegin = static_cast<uint32_t>(rpIndirectBufferBarriers_.size());
        data->indirectBufferBarrierCount = 0U;

        // barriers are always needed e.g. when dynamic resource is bound for writing in multiple dispatches
        const bool handleDescriptorSets = stateData_.dirtyDescriptorSetsForBarriers ||
                                          renderCommandType == RenderCommandType::DISPATCH ||
                                          renderCommandType == RenderCommandType::DISPATCH_INDIRECT;
        if (handleDescriptorSets) {
            stateData_.dirtyDescriptorSetsForBarriers = false;
            for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
                // only add descriptor set handles for barriers if there are dynamic barrier resources
                if (stateData_.currentBoundSets[idx].hasDynamicBarrierResources) {
                    descriptorSetHandlesForBarriers_.push_back(stateData_.currentBoundSets[idx].descriptorSetHandle);
                }
            }
            data->descriptorSetHandleCount =
                (uint32_t)descriptorSetHandlesForBarriers_.size() - descriptorSetBeginIndex;
        }

        const bool handleCustomBarriers =
            ((!customBarriers_.empty()) && stateData_.currentCustomBarrierIndices.dirtyCustomBarriers);
        if (handleCustomBarriers) {
            const int32_t newCount = static_cast<int32_t>(customBarriers_.size()) -
                stateData_.currentCustomBarrierIndices.prevSize;
            if (newCount > 0) {
                data->customBarrierIndexBegin = static_cast<uint32_t>(stateData_.currentCustomBarrierIndices.prevSize);
                data->customBarrierCount = static_cast<uint32_t>(newCount);

                stateData_.currentCustomBarrierIndices.prevSize = static_cast<int32_t>(customBarriers_.size());
                stateData_.currentCustomBarrierIndices.dirtyCustomBarriers = false;
            }
        }

        // store current barrier point for render command list
        // * binding descriptor sets (with dynamic barrier resources)
        // * binding vertex and index buffers (with dynamic barrier resources)
        // * indirect args buffer (with dynamic barrier resources)
        // inside a render pass adds barriers directly to the RenderCommandBarrierPoint behind this pointer
        stateData_.currentBarrierPoint = data;

        renderCommands_.push_back({ RenderCommandType::BARRIER_POINT, data });
    }
}

void RenderCommandList::Draw(
    const uint32_t vertexCount, const uint32_t instanceCount, const uint32_t firstVertex, const uint32_t firstInstance)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!stateData_.renderPassHasBegun) {
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_Draw_",
            "RENDER_VALIDATION: RenderCommandList: render pass not active (begin before draw)");
    }
#endif

    if (vertexCount > 0 && stateData_.renderPassHasBegun) { // prevent zero draws
        ValidatePipeline();
        ValidatePipelineLayout();

        RenderCommandDraw* data = AllocateRenderCommand<RenderCommandDraw>(allocator_);
        if (data) {
            data->drawType = DrawType::DRAW;
            data->vertexCount = vertexCount;
            data->instanceCount = instanceCount;
            data->firstVertex = firstVertex;
            data->firstInstance = firstInstance;
            data->indexCount = 0;
            data->firstIndex = 0;
            data->vertexOffset = 0;

            renderCommands_.push_back({ RenderCommandType::DRAW, data });
        }
    }
}

void RenderCommandList::DrawIndexed(const uint32_t indexCount, const uint32_t instanceCount, const uint32_t firstIndex,
    const int32_t vertexOffset, const uint32_t firstInstance)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!stateData_.renderPassHasBegun) {
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_DrawIndexed_",
            "RENDER_VALIDATION: RenderCommandList: render pass not active (begin before draw).");
    }
#endif

    if (indexCount > 0 && stateData_.renderPassHasBegun) { // prevent zero draws
        ValidatePipeline();
        ValidatePipelineLayout();

        RenderCommandDraw* data = AllocateRenderCommand<RenderCommandDraw>(allocator_);
        if (data) {
            data->drawType = DrawType::DRAW_INDEXED;
            data->vertexCount = 0;
            data->instanceCount = instanceCount;
            data->firstVertex = 0;
            data->firstInstance = firstInstance;
            data->indexCount = indexCount;
            data->firstIndex = firstIndex;
            data->vertexOffset = vertexOffset;

            renderCommands_.push_back({ RenderCommandType::DRAW, data });
        }
    }
}

void RenderCommandList::DrawIndirect(
    const RenderHandle bufferHandle, const uint32_t offset, const uint32_t drawCount, const uint32_t stride)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RENDER_VALIDATION: render pass not active (begin before draw)");
    }
    if (!RenderHandleUtil::IsGpuBuffer(bufferHandle)) {
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_DI_buffer_", "RENDER_VALIDATION: DrawIndirect buffer handle invalid.");
    }
#endif

    if (stateData_.renderPassHasBegun && RenderHandleUtil::IsGpuBuffer(bufferHandle)) {
        ValidatePipeline();
        ValidatePipelineLayout();

        RenderCommandDrawIndirect* data = AllocateRenderCommand<RenderCommandDrawIndirect>(allocator_);
        if (data) {
            data->drawType = DrawType::DRAW_INDIRECT;
            data->argsHandle = bufferHandle;
            data->offset = offset;
            data->drawCount = drawCount;
            data->stride = stride;

            // add possible indirect buffer barrier before render pass
            if (RenderHandleUtil::IsDynamicResource(bufferHandle)) {
                constexpr uint32_t drawIndirectCommandSize { 4U * sizeof(uint32_t) };
                PLUGIN_ASSERT(stateData_.currentBarrierPoint);
                stateData_.currentBarrierPoint->indirectBufferBarrierCount++;
                rpIndirectBufferBarriers_.push_back({ bufferHandle, offset, drawIndirectCommandSize });
            }

            renderCommands_.push_back({ RenderCommandType::DRAW_INDIRECT, data });
        }
    }
}

void RenderCommandList::DrawIndexedIndirect(
    const RenderHandle bufferHandle, const uint32_t offset, const uint32_t drawCount, const uint32_t stride)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RENDER_VALIDATION: render pass not active (begin before draw)");
    }
    if (!RenderHandleUtil::IsGpuBuffer(bufferHandle)) {
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_DII_buffer_", "RENDER_VALIDATION: DrawIndirect buffer handle invalid.");
    }
#endif

    if (stateData_.renderPassHasBegun && RenderHandleUtil::IsGpuBuffer(bufferHandle)) {
        ValidatePipeline();
        ValidatePipelineLayout();

        RenderCommandDrawIndirect* data = AllocateRenderCommand<RenderCommandDrawIndirect>(allocator_);
        if (data) {
            data->drawType = DrawType::DRAW_INDEXED_INDIRECT;
            data->argsHandle = bufferHandle;
            data->offset = offset;
            data->drawCount = drawCount;
            data->stride = stride;

            // add possible indirect buffer barrier before render pass
            if (RenderHandleUtil::IsDynamicResource(bufferHandle)) {
                constexpr uint32_t drawIndirectCommandSize { 5U * sizeof(uint32_t) };
                PLUGIN_ASSERT(stateData_.currentBarrierPoint);
                stateData_.currentBarrierPoint->indirectBufferBarrierCount++;
                rpIndirectBufferBarriers_.push_back({ bufferHandle, offset, drawIndirectCommandSize });
            }

            renderCommands_.push_back({ RenderCommandType::DRAW_INDIRECT, data });
        }
    }
}

void RenderCommandList::Dispatch(const uint32_t groupCountX, const uint32_t groupCountY, const uint32_t groupCountZ)
{
    if (groupCountX > 0 && groupCountY > 0 && groupCountZ > 0) { // prevent zero dispatches
        ValidatePipeline();
        ValidatePipelineLayout();

        AddBarrierPoint(RenderCommandType::DISPATCH);

        RenderCommandDispatch* data = AllocateRenderCommand<RenderCommandDispatch>(allocator_);
        if (data) {
            data->groupCountX = groupCountX;
            data->groupCountY = groupCountY;
            data->groupCountZ = groupCountZ;

            renderCommands_.push_back({ RenderCommandType::DISPATCH, data });
        }
    }
}

void RenderCommandList::DispatchIndirect(const RenderHandle bufferHandle, const uint32_t offset)
{
    ValidatePipeline();
    ValidatePipelineLayout();

    AddBarrierPoint(RenderCommandType::DISPATCH_INDIRECT);

    RenderCommandDispatchIndirect* data = AllocateRenderCommand<RenderCommandDispatchIndirect>(allocator_);
    if (data) {
        data->argsHandle = bufferHandle;
        data->offset = offset;

        renderCommands_.push_back({ RenderCommandType::DISPATCH_INDIRECT, data });
    }
}

void RenderCommandList::BindPipeline(const RenderHandle psoHandle)
{
    // NOTE: we cannot early out with the same pso handle
    // the render pass and it's hashes might have been changed
    // the final pso needs to be hashed with final render pass
    // the backends try to check the re-binding of the same pipeline
    // another approach would be to check when render pass changes to re-bind psos if needed

    bool valid = RenderHandleUtil::IsValid(psoHandle);

    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(psoHandle);
    PipelineBindPoint pipelineBindPoint {};
    if (handleType == RenderHandleType::COMPUTE_PSO) {
        pipelineBindPoint = PipelineBindPoint::CORE_PIPELINE_BIND_POINT_COMPUTE;
    } else if (handleType == RenderHandleType::GRAPHICS_PSO) {
        pipelineBindPoint = PipelineBindPoint::CORE_PIPELINE_BIND_POINT_GRAPHICS;
    } else {
        valid = false;
    }

    stateData_.checkBindPipelineLayout = true;
#if (RENDER_VALIDATION_ENABLED == 1)
    if (pipelineBindPoint == PipelineBindPoint::CORE_PIPELINE_BIND_POINT_GRAPHICS) {
        if (!stateData_.renderPassHasBegun) {
            valid = false;
            PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_BindPipeline_",
                "RENDER_VALIDATION: RenderCommandList: bind pipeline after render pass begin.");
        }
    }
#endif

    stateData_.validPso = valid;
    ValidatePipeline();

    stateData_.currentPsoHandle = psoHandle;
    stateData_.currentPsoBindPoint = pipelineBindPoint;

    RenderCommandBindPipeline* data = AllocateRenderCommand<RenderCommandBindPipeline>(allocator_);
    if (data) {
        data->psoHandle = psoHandle;
        data->pipelineBindPoint = pipelineBindPoint;

        renderCommands_.push_back({ RenderCommandType::BIND_PIPELINE, data });
    }
}

void RenderCommandList::PushConstantData(
    const RENDER_NS::PushConstant& pushConstant, const BASE_NS::array_view<const uint8_t> data)
{
    ValidatePipeline();

    // push constant is not used/allocated if byte size is bigger than supported max
    if ((pushConstant.byteSize > 0) &&
        (pushConstant.byteSize <= PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE) && (!data.empty())) {
        RenderCommandPushConstant* rc = AllocateRenderCommand<RenderCommandPushConstant>(allocator_);
        // use aligment of uint32 as currently the push constants are uint32s
        // the data is allocated by shader/pipeline needs
        uint8_t* pushData =
            static_cast<uint8_t*>(AllocateRenderData(allocator_, std::alignment_of<uint32_t>(), pushConstant.byteSize));
        if (rc && pushData) {
            rc->psoHandle = stateData_.currentPsoHandle;
            rc->pushConstant = pushConstant;
            rc->data = pushData;
            // the max amount of visible data is copied
            const size_t minData = Math::min(static_cast<size_t>(pushConstant.byteSize), data.size_bytes());
            const bool res = CloneData(rc->data, pushConstant.byteSize, data.data(), minData);
            PLUGIN_UNUSED(res);
            PLUGIN_ASSERT(res);

            renderCommands_.push_back(RenderCommandWithType { RenderCommandType::PUSH_CONSTANT, rc });
        }
    } else if (pushConstant.byteSize > 0) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: push constant byte size must be smaller or equal to %u bytes.",
            PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE);
#endif
    }
}

void RenderCommandList::PushConstant(const RENDER_NS::PushConstant& pushConstant, const uint8_t* data)
{
    if ((pushConstant.byteSize > 0) && data) {
        PushConstantData(pushConstant, { data, pushConstant.byteSize });
    }
}

void RenderCommandList::BindVertexBuffers(const array_view<const VertexBuffer> vertexBuffers)
{
    ValidatePipeline();

#if (RENDER_VALIDATION_ENABLED == 1)
    if (vertexBuffers.size() > PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT) {
        PLUGIN_LOG_W("RENDER_VALIDATION : max vertex buffer count exceeded, binding only max vertex buffer count");
    }
#endif

    if (!vertexBuffers.empty()) {
        RenderCommandBindVertexBuffers* data = AllocateRenderCommand<RenderCommandBindVertexBuffers>(allocator_);
        if (data) {
            VertexBuffer dynamicBarrierVertexBuffers[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
            uint32_t dynamicBarrierVertexBufferCount = 0;
            const uint32_t vertexBufferCount =
                Math::min(PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT, (uint32_t)vertexBuffers.size());
            data->vertexBufferCount = vertexBufferCount;
            RenderHandle previousVbHandle; // often all vertex buffers are withing the same buffer with offsets
            for (size_t idx = 0; idx < vertexBufferCount; ++idx) {
                data->vertexBuffers[idx] = vertexBuffers[idx];
                const RenderHandle currVbHandle = vertexBuffers[idx].bufferHandle;
                if ((previousVbHandle.id != currVbHandle.id) && RenderHandleUtil::IsDynamicResource(currVbHandle) &&
                    (vertexBuffers[idx].byteSize > 0)) {
                    // NOTE: we do not try to create perfect barriers with vertex inputs (just barrier the whole rc)
                    dynamicBarrierVertexBuffers[dynamicBarrierVertexBufferCount++] = { currVbHandle, 0,
                        PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE };
                    previousVbHandle = currVbHandle;
                }
            }

            // add possible vertex/index buffer barriers before render pass
            if (stateData_.renderPassHasBegun && (dynamicBarrierVertexBufferCount > 0)) {
                PLUGIN_ASSERT(stateData_.currentBarrierPoint);
                stateData_.currentBarrierPoint->vertexIndexBarrierCount += dynamicBarrierVertexBufferCount;
                const size_t currCount = rpVertexInputBufferBarriers_.size();
                rpVertexInputBufferBarriers_.resize(currCount + static_cast<size_t>(dynamicBarrierVertexBufferCount));
                for (uint32_t dynIdx = 0; dynIdx < dynamicBarrierVertexBufferCount; ++dynIdx) {
                    rpVertexInputBufferBarriers_[currCount + dynIdx] = dynamicBarrierVertexBuffers[dynIdx];
                }
            }

            renderCommands_.push_back({ RenderCommandType::BIND_VERTEX_BUFFERS, data });
        }
    }
}

void RenderCommandList::BindIndexBuffer(const IndexBuffer& indexBuffer)
{
    ValidatePipeline();

    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(indexBuffer.bufferHandle);
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((indexBuffer.indexType > IndexType::CORE_INDEX_TYPE_UINT32) || (handleType != RenderHandleType::GPU_BUFFER)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid index buffer binding");
    }
#endif

    RenderCommandBindIndexBuffer* data = AllocateRenderCommand<RenderCommandBindIndexBuffer>(allocator_);
    if (data && (handleType == RenderHandleType::GPU_BUFFER)) {
        data->indexBuffer = indexBuffer;
        if (RenderHandleUtil::IsDynamicResource(indexBuffer.bufferHandle)) {
            stateData_.currentBarrierPoint->vertexIndexBarrierCount++;
            rpVertexInputBufferBarriers_.push_back(
                { indexBuffer.bufferHandle, indexBuffer.bufferOffset, indexBuffer.byteSize });
        }
        renderCommands_.push_back({ RenderCommandType::BIND_INDEX_BUFFER, data });
    }
}

void RenderCommandList::BeginRenderPass(
    const RenderPassDesc& renderPassDesc, const array_view<const RenderPassSubpassDesc> subpassDescs)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((renderPassDesc.attachmentCount == 0) || (renderPassDesc.subpassCount == 0)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid RenderPassDesc in BeginRenderPass");
    }
#endif

    // TODO: needs to be missing multipass related stuff

    if (renderPassDesc.subpassCount != static_cast<uint32_t>(subpassDescs.size())) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_ValidateRenderPass_subpass_",
            "RENDER_VALIDATION: BeginRenderPass renderPassDesc.subpassCount (%u) must match subpassDescs size (%u)",
            renderPassDesc.subpassCount, static_cast<uint32_t>(subpassDescs.size()));
#endif
        stateData_.validCommandList = false;
    }
    ValidateRenderPass(renderPassDesc);
    if (!stateData_.validCommandList) {
        return;
    }

    stateData_.renderPassHasBegun = true;
    stateData_.renderPassStartIndex = 0;
    stateData_.renderPassSubpassCount = renderPassDesc.subpassCount;

    if (renderPassDesc.attachmentCount > 0) {
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateRenderPassAttachment(nodeName_, gpuResourceMgr_, renderPassDesc, subpassDescs);
#endif
        AddBarrierPoint(RenderCommandType::BEGIN_RENDER_PASS);

        if (auto* data = AllocateRenderCommand<RenderCommandBeginRenderPass>(allocator_); data) {
            // NOTE: hashed in the backend
            PLUGIN_ASSERT(renderPassDesc.subpassCount == (uint32_t)subpassDescs.size());

            data->beginType = RenderPassBeginType::RENDER_PASS_BEGIN;
            data->renderPassDesc = renderPassDesc;
            data->renderPassDesc.renderArea.extentWidth = Math::max(1u, data->renderPassDesc.renderArea.extentWidth);
            data->renderPassDesc.renderArea.extentHeight = Math::max(1u, data->renderPassDesc.renderArea.extentHeight);
            data->subpassStartIndex = 0;
            // if false -> initial layout is undefined
            data->enableAutomaticLayoutChanges = stateData_.automaticBarriersEnabled;

            data->subpasses = { AllocateRenderData<RenderPassSubpassDesc>(allocator_, renderPassDesc.subpassCount),
                renderPassDesc.subpassCount };
            data->subpassResourceStates = { AllocateRenderData<RenderPassAttachmentResourceStates>(
                                                allocator_, renderPassDesc.subpassCount),
                renderPassDesc.subpassCount };
            if ((!data->subpasses.data()) || (!data->subpassResourceStates.data())) {
                return;
            }

            CloneData(
                data->subpasses.data(), data->subpasses.size_bytes(), subpassDescs.data(), subpassDescs.size_bytes());

            bool valid = true;
            for (size_t subpassIdx = 0; subpassIdx < subpassDescs.size(); ++subpassIdx) {
                const auto& subpassRef = subpassDescs[subpassIdx];

                RenderPassAttachmentResourceStates& subpassResourceStates = data->subpassResourceStates[subpassIdx];
                subpassResourceStates = {};

                valid = valid && ProcessInputAttachments(renderPassDesc, subpassRef, subpassResourceStates);
                valid = valid && ProcessColorAttachments(renderPassDesc, subpassRef, subpassResourceStates);
                valid = valid && ProcessResolveAttachments(renderPassDesc, subpassRef, subpassResourceStates);
                valid = valid && ProcessDepthAttachments(renderPassDesc, subpassRef, subpassResourceStates);
                valid =
                    valid && ProcessFragmentShadingRateAttachments(renderPassDesc, subpassRef, subpassResourceStates);
#if (RENDER_VULKAN_FSR_ENABLED != 1)
                data->subpasses[subpassIdx].fragmentShadingRateAttachmentCount = 0u;
#endif
            }
            if (!valid) {
                stateData_.validCommandList = false;
            }

            // render pass layouts will be updated by render graph
            renderCommands_.push_back({ RenderCommandType::BEGIN_RENDER_PASS, data });
        }
    }
}

void RenderCommandList::BeginRenderPass(
    const RenderPassDesc& renderPassDesc, const uint32_t subpassStartIdx, const RenderPassSubpassDesc& subpassDesc)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((renderPassDesc.attachmentCount == 0) || (renderPassDesc.subpassCount == 0)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid RenderPassDesc in BeginRenderPass");
    }
#endif

    if (subpassStartIdx >= renderPassDesc.subpassCount) {
        PLUGIN_LOG_E("RCL:BeginRenderPass: subpassStartIdx(%u) must be smaller than renderPassDesc.subpassCount (%u)",
            subpassStartIdx, renderPassDesc.subpassCount);
        stateData_.validCommandList = false;
    }

    ValidateRenderPass(renderPassDesc);
    if (!stateData_.validCommandList) {
        return;
    }

    stateData_.renderPassHasBegun = true;
    stateData_.renderPassStartIndex = subpassStartIdx;
    stateData_.renderPassSubpassCount = renderPassDesc.subpassCount;

    if (renderPassDesc.attachmentCount > 0) {
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateRenderPassAttachment(nodeName_, gpuResourceMgr_, renderPassDesc, { &subpassDesc, 1u });
#endif
        AddBarrierPoint(RenderCommandType::BEGIN_RENDER_PASS);

        if (hasMultiRpCommandListSubpasses_) {
            PLUGIN_LOG_E("RenderCommandList: BeginRenderPass: creating multiple render node subpasses not supported");
            stateData_.validCommandList = false;
        } else if (renderPassDesc.subpassCount > 1) {
            hasMultiRpCommandListSubpasses_ = true;
            multiRpCommandListData_.secondaryCmdLists =
                (renderPassDesc.subpassContents == CORE_SUBPASS_CONTENTS_SECONDARY_COMMAND_LISTS) ? true : false;
            if ((!renderCommands_.empty()) && (renderCommands_.back().type == RenderCommandType::BARRIER_POINT)) {
                multiRpCommandListData_.rpBarrierCmdIndex = static_cast<uint32_t>(renderCommands_.size()) - 1u;
            }
        }
        multiRpCommandListData_.subpassCount = renderPassDesc.subpassCount;
        multiRpCommandListData_.rpBeginCmdIndex = static_cast<uint32_t>(renderCommands_.size());

        if (auto* data = AllocateRenderCommand<RenderCommandBeginRenderPass>(allocator_); data) {
            // NOTE: hashed in the backend
            data->beginType = RenderPassBeginType::RENDER_PASS_BEGIN;
            data->renderPassDesc = renderPassDesc;
            data->subpassStartIndex = subpassStartIdx;
            // if false -> initial layout is undefined
            data->enableAutomaticLayoutChanges = stateData_.automaticBarriersEnabled;

            data->subpasses = { AllocateRenderData<RenderPassSubpassDesc>(allocator_, renderPassDesc.subpassCount),
                renderPassDesc.subpassCount };
            data->subpassResourceStates = { AllocateRenderData<RenderPassAttachmentResourceStates>(
                                                allocator_, renderPassDesc.subpassCount),
                renderPassDesc.subpassCount };
            if ((!data->subpasses.data()) || (!data->subpassResourceStates.data())) {
                return;
            }

            bool valid = true;
            for (size_t subpassIdx = 0; subpassIdx < data->subpasses.size(); ++subpassIdx) {
                RenderPassAttachmentResourceStates& subpassResourceStates = data->subpassResourceStates[subpassIdx];
                subpassResourceStates = {};
                data->subpasses[subpassIdx] = {};

                if (subpassIdx == subpassStartIdx) {
                    data->subpasses[subpassIdx] = subpassDesc;
                    valid = valid && ProcessInputAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                    valid = valid && ProcessColorAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                    valid = valid && ProcessResolveAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                    valid = valid && ProcessDepthAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                    valid = valid &&
                            ProcessFragmentShadingRateAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
#if (RENDER_VULKAN_FSR_ENABLED != 1)
                    data->subpasses[subpassIdx].fragmentShadingRateAttachmentCount = 0u;
#endif
                }
            }
            if (!valid) {
                stateData_.validCommandList = false;
            }

            // render pass layouts will be updated by render graph
            renderCommands_.push_back({ RenderCommandType::BEGIN_RENDER_PASS, data });
        }
    }
}

bool RenderCommandList::ProcessInputAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    bool valid = true;
    for (uint32_t idx = 0; idx < subpassRef.inputAttachmentCount; ++idx) {
        const uint32_t attachmentIndex = subpassRef.inputAttachmentIndices[idx];
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        if (!RenderHandleUtil::IsGpuImage(handle)) {
            valid = false;
        }

        // NOTE: mipLevel and layers are not updated to GpuResourceState
        // NOTE: validation needed for invalid handles
        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |= CORE_ACCESS_INPUT_ATTACHMENT_READ_BIT;
        refState.pipelineStageFlags |= CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        refState.gpuQueue = gpuQueue_;
        // if used e.g. as input and color attachment use general layout
        if (subpassResourceStates.layouts[attachmentIndex] != CORE_IMAGE_LAYOUT_UNDEFINED) {
            subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_GENERAL;
        } else {
            subpassResourceStates.layouts[attachmentIndex] = (RenderHandleUtil::IsDepthImage(handle))
                                                                 ? CORE_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
                                                                 : CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateImageUsageFlags(nodeName_, gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, "CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
#endif
    }
    return valid;
}

bool RenderCommandList::ProcessColorAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    bool valid = true;
    for (uint32_t idx = 0; idx < subpassRef.colorAttachmentCount; ++idx) {
        const uint32_t attachmentIndex = subpassRef.colorAttachmentIndices[idx];
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        if (!RenderHandleUtil::IsGpuImage(handle)) {
            valid = false;
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateImageUsageFlags(nodeName_, gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
#endif

        // NOTE: mipLevel and layers are not updated to GpuResourceState
        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |= (CORE_ACCESS_COLOR_ATTACHMENT_READ_BIT | CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);
        refState.pipelineStageFlags |= CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        refState.gpuQueue = gpuQueue_;
        // if used e.g. as input and color attachment use general layout
        subpassResourceStates.layouts[attachmentIndex] =
            (subpassResourceStates.layouts[attachmentIndex] != ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED)
                ? CORE_IMAGE_LAYOUT_GENERAL
                : CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    return valid;
}

bool RenderCommandList::ProcessResolveAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    bool valid = true;
    for (uint32_t idx = 0; idx < subpassRef.resolveAttachmentCount; ++idx) {
        const uint32_t attachmentIndex = subpassRef.resolveAttachmentIndices[idx];
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        if (!RenderHandleUtil::IsGpuImage(handle)) {
            valid = false;
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateImageUsageFlags(nodeName_, gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, "CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
#endif

        // NOTE: mipLevel and layers are not updated to GpuResourceState
        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |= CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        refState.pipelineStageFlags |= CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        refState.gpuQueue = gpuQueue_;
        subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
    return valid;
}

bool RenderCommandList::ProcessDepthAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    bool valid = true;
    if (subpassRef.depthAttachmentCount == 1) {
        const uint32_t attachmentIndex = subpassRef.depthAttachmentIndex;
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        if (!RenderHandleUtil::IsDepthImage(handle)) {
            valid = false;
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateImageUsageFlags(nodeName_, gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT");
#endif

        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |=
            (CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
        refState.pipelineStageFlags |=
            (CORE_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | CORE_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        refState.gpuQueue = gpuQueue_;
        subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    if ((subpassRef.depthAttachmentCount == 1) && (subpassRef.depthResolveAttachmentCount == 1)) {
        const uint32_t attachmentIndex = subpassRef.depthResolveAttachmentIndex;
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        if (!RenderHandleUtil::IsDepthImage(handle)) {
            valid = false;
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateImageUsageFlags(nodeName_, gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT");
#endif

        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |= CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        refState.pipelineStageFlags |= CORE_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        refState.gpuQueue = gpuQueue_;
        subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    return valid;
}

bool RenderCommandList::ProcessFragmentShadingRateAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    bool valid = true;
    if (subpassRef.fragmentShadingRateAttachmentCount == 1) {
#if (RENDER_VULKAN_FSR_ENABLED == 1)
        const uint32_t attachmentIndex = subpassRef.fragmentShadingRateAttachmentIndex;
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        if (!RenderHandleUtil::IsGpuImage(handle)) {
            valid = false;
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateImageUsageFlags(nodeName_, gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT");
#endif

        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |= CORE_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT;
        refState.pipelineStageFlags |= CORE_PIPELINE_STAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT;
        refState.gpuQueue = gpuQueue_;
        subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL;
#else
        PLUGIN_LOG_ONCE_I("vk_fsr_disabled_flag",
            "RENDER_VALIDATION: Fragment shading rate disabled and all related attachments ignored.");
#endif
    }
    return valid;
}

void RenderCommandList::NextSubpass(const SubpassContents& subpassContents)
{
    RenderCommandNextSubpass* data = AllocateRenderCommand<RenderCommandNextSubpass>(allocator_);
    if (data) {
        data->subpassContents = subpassContents;
        data->renderCommandListIndex = 0; // will be updated in the render graph

        renderCommands_.push_back({ RenderCommandType::NEXT_SUBPASS, data });
    }
}

void RenderCommandList::EndRenderPass()
{
    if (!stateData_.renderPassHasBegun) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E(
            nodeName_ + "_RCL_EndRenderPass_", "RenderCommandList: render pass needs to begin before calling end");
#endif
        stateData_.validCommandList = false;
        return;
    }

    if (hasMultiRpCommandListSubpasses_ && (multiRpCommandListData_.rpBeginCmdIndex != INVALID_CL_IDX)) {
        multiRpCommandListData_.rpEndCmdIndex = static_cast<uint32_t>(renderCommands_.size());
    }

    RenderCommandEndRenderPass* data = AllocateRenderCommand<RenderCommandEndRenderPass>(allocator_);
    if (data) {
        // will be updated in render graph if multi render command list render pass
        data->endType = RenderPassEndType::END_RENDER_PASS;
        data->subpassStartIndex = stateData_.renderPassStartIndex;
        data->subpassCount = stateData_.renderPassSubpassCount;

        renderCommands_.push_back({ RenderCommandType::END_RENDER_PASS, data });
    }

    stateData_.renderPassHasBegun = false;
    stateData_.renderPassStartIndex = 0;
    stateData_.renderPassSubpassCount = 0;
}

void RenderCommandList::BeginDisableAutomaticBarrierPoints()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!stateData_.automaticBarriersEnabled) {
        PLUGIN_LOG_E("RENDER_VALIDATION: EndDisableAutomaticBarrierPoints not called?");
    }
#endif
    PLUGIN_ASSERT(stateData_.automaticBarriersEnabled);

    // barrier point for pending barriers
    AddBarrierPoint(RenderCommandType::BARRIER_POINT);
    stateData_.automaticBarriersEnabled = false;
}

void RenderCommandList::EndDisableAutomaticBarrierPoints()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (stateData_.automaticBarriersEnabled) {
        PLUGIN_LOG_E("RENDER_VALIDATION: BeginDisableAutomaticBarrierPoints not called?");
    }
#endif
    PLUGIN_ASSERT(!stateData_.automaticBarriersEnabled);

    stateData_.automaticBarriersEnabled = true;
}

void RenderCommandList::AddCustomBarrierPoint()
{
    const bool barrierState = stateData_.automaticBarriersEnabled;
    stateData_.automaticBarriersEnabled = true; // flag checked in AddBarrierPoint
    AddBarrierPoint(RenderCommandType::BARRIER_POINT);
    stateData_.automaticBarriersEnabled = barrierState;
}

void RenderCommandList::CustomMemoryBarrier(const GeneralBarrier& source, const GeneralBarrier& destination)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RENDER_VALIDATION: barriers are not allowed inside render passes");
    }
#endif

    CommandBarrier cb {
        RenderHandleUtil::CreateGpuResourceHandle(RenderHandleType::UNDEFINED, 0, 0, 0, 0),
        {
            source.accessFlags,
            source.pipelineStageFlags,
        },
        {},
        {
            destination.accessFlags,
            destination.pipelineStageFlags,
        },
        {},
    };

    customBarriers_.push_back(move(cb));

    stateData_.currentCustomBarrierIndices.dirtyCustomBarriers = true;
}

void RenderCommandList::CustomBufferBarrier(const RenderHandle handle, const BufferResourceBarrier& source,
    const BufferResourceBarrier& destination, const uint32_t byteOffset, const uint32_t byteSize)
{
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);

#if (RENDER_VALIDATION_ENABLED == 1)
    if (stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RENDER_VALIDATION: barriers are not allowed inside render passes");
    }
    if (byteSize == 0) {
        PLUGIN_LOG_ONCE_W("RENDER_VALIDATION_custom_buffer_barrier",
            "RENDER_VALIDATION: do not create zero size custom buffer barriers");
    }
    if (handleType != RenderHandleType::GPU_BUFFER) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle type to CustomBufferBarrier");
    }
#endif

    if ((byteSize > 0) && (handleType == RenderHandleType::GPU_BUFFER)) {
        ResourceBarrier src;
        src.accessFlags = source.accessFlags;
        src.pipelineStageFlags = source.pipelineStageFlags;
        src.optionalByteOffset = byteOffset;
        src.optionalByteSize = byteSize;

        ResourceBarrier dst;
        dst.accessFlags = destination.accessFlags;
        dst.pipelineStageFlags = destination.pipelineStageFlags;
        dst.optionalByteOffset = byteOffset;
        dst.optionalByteSize = byteSize;

        CommandBarrier cb {
            handle,
            std::move(src),
            {},
            std::move(dst),
            {},
        };

        customBarriers_.push_back(move(cb));

        stateData_.currentCustomBarrierIndices.dirtyCustomBarriers = true;
    }
}

void RenderCommandList::CustomImageBarrier(const RenderHandle handle, const ImageResourceBarrier& destination,
    const ImageSubresourceRange& imageSubresourceRange)
{
    // specific layout MAX_ENUM to state that we fetch the correct state
    ImageResourceBarrier source { 0, 0, ImageLayout::CORE_IMAGE_LAYOUT_MAX_ENUM };
    CustomImageBarrier(handle, source, destination, imageSubresourceRange);
}

void RenderCommandList::CustomImageBarrier(const RenderHandle handle, const ImageResourceBarrier& source,
    const ImageResourceBarrier& destination, const ImageSubresourceRange& imageSubresourceRange)
{
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);

#if (RENDER_VALIDATION_ENABLED == 1)
    if (stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RENDER_VALIDATION: barriers are not allowed inside render passes");
    }
    if (handleType != RenderHandleType::GPU_IMAGE) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid handle type to CustomImageBarrier");
    }
    ValidateImageSubresourceRange(gpuResourceMgr_, handle, imageSubresourceRange);
#endif

    if (handleType == RenderHandleType::GPU_IMAGE) {
        ResourceBarrier src;
        src.accessFlags = source.accessFlags;
        src.pipelineStageFlags = source.pipelineStageFlags;
        src.optionalImageLayout = source.imageLayout;
        src.optionalImageSubresourceRange = imageSubresourceRange;

        ResourceBarrier dst;
        dst.accessFlags = destination.accessFlags;
        dst.pipelineStageFlags = destination.pipelineStageFlags;
        dst.optionalImageLayout = destination.imageLayout;
        dst.optionalImageSubresourceRange = imageSubresourceRange;

        CommandBarrier cb {
            handle,
            std::move(src),
            {},
            std::move(dst),
            {},
        };

        customBarriers_.push_back(std::move(cb));

        stateData_.currentCustomBarrierIndices.dirtyCustomBarriers = true;
    }
}

void RenderCommandList::CopyBufferToBuffer(
    const RenderHandle sourceHandle, const RenderHandle destinationHandle, const BufferCopy& bufferCopy)
{
    if (RenderHandleUtil::IsGpuBuffer(sourceHandle) && RenderHandleUtil::IsGpuBuffer(destinationHandle)) {
        // NOTE: combine copies, and only single combined barrier?
        if (RenderHandleUtil::IsDynamicResource(sourceHandle) ||
            RenderHandleUtil::IsDynamicResource(destinationHandle)) {
            AddBarrierPoint(RenderCommandType::COPY_BUFFER);
        }

        RenderCommandCopyBuffer* data = AllocateRenderCommand<RenderCommandCopyBuffer>(allocator_);
        if (data) {
            data->srcHandle = sourceHandle;
            data->dstHandle = destinationHandle;
            data->bufferCopy = bufferCopy;

            renderCommands_.push_back({ RenderCommandType::COPY_BUFFER, data });
        }
    } else {
        PLUGIN_LOG_E("RenderCommandList: invalid CopyBufferToBuffer");
    }
}

void RenderCommandList::CopyBufferToImage(
    const RenderHandle sourceHandle, const RenderHandle destinationHandle, const BufferImageCopy& bufferImageCopy)
{
    if (RenderHandleUtil::IsGpuBuffer(sourceHandle) && RenderHandleUtil::IsGpuImage(destinationHandle)) {
        // NOTE: combine copies, and only single combined barrier?
        if (RenderHandleUtil::IsDynamicResource(sourceHandle) ||
            RenderHandleUtil::IsDynamicResource(destinationHandle)) {
            AddBarrierPoint(RenderCommandType::COPY_BUFFER_IMAGE);
        }

        RenderCommandCopyBufferImage* data = AllocateRenderCommand<RenderCommandCopyBufferImage>(allocator_);
        if (data) {
            data->copyType = RenderCommandCopyBufferImage::CopyType::BUFFER_TO_IMAGE;
            data->srcHandle = sourceHandle;
            data->dstHandle = destinationHandle;
            data->bufferImageCopy = bufferImageCopy;

            renderCommands_.push_back({ RenderCommandType::COPY_BUFFER_IMAGE, data });
        }
    } else {
        PLUGIN_LOG_E("RenderCommandList: invalid CopyBufferToImage");
    }
}

void RenderCommandList::CopyImageToBuffer(
    const RenderHandle sourceHandle, const RenderHandle destinationHandle, const BufferImageCopy& bufferImageCopy)
{
    if (RenderHandleUtil::IsGpuImage(sourceHandle) && RenderHandleUtil::IsGpuBuffer(destinationHandle)) {
        // NOTE: combine copies, and only single combined barrier?
        if (RenderHandleUtil::IsDynamicResource(sourceHandle) ||
            RenderHandleUtil::IsDynamicResource(destinationHandle)) {
            AddBarrierPoint(RenderCommandType::COPY_BUFFER_IMAGE);
        }

        RenderCommandCopyBufferImage* data = AllocateRenderCommand<RenderCommandCopyBufferImage>(allocator_);
        if (data) {
            data->copyType = RenderCommandCopyBufferImage::CopyType::IMAGE_TO_BUFFER;
            data->srcHandle = sourceHandle;
            data->dstHandle = destinationHandle;
            data->bufferImageCopy = bufferImageCopy;

            renderCommands_.push_back({ RenderCommandType::COPY_BUFFER_IMAGE, data });
        }
    } else {
        PLUGIN_LOG_E("RenderCommandList: invalid CopyImageToBuffer");
    }
}

void RenderCommandList::CopyImageToImage(
    const RenderHandle sourceHandle, const RenderHandle destinationHandle, const ImageCopy& imageCopy)
{
    if (RenderHandleUtil::IsGpuImage(sourceHandle) && RenderHandleUtil::IsGpuImage(destinationHandle)) {
        // NOTE: combine copies, and only single combined barrier?
        if (RenderHandleUtil::IsDynamicResource(sourceHandle) ||
            RenderHandleUtil::IsDynamicResource(destinationHandle)) {
            AddBarrierPoint(RenderCommandType::COPY_IMAGE);
        }

        RenderCommandCopyImage* data = AllocateRenderCommand<RenderCommandCopyImage>(allocator_);
        if (data) {
            data->srcHandle = sourceHandle;
            data->dstHandle = destinationHandle;
            data->imageCopy = imageCopy;

            renderCommands_.push_back({ RenderCommandType::COPY_IMAGE, data });
        }
    } else {
        PLUGIN_LOG_E("RenderCommandList: invalid CopyImageToImage");
    }
}

void RenderCommandList::BlitImage(const RenderHandle sourceHandle, const RenderHandle destinationHandle,
    const ImageBlit& imageBlit, const Filter filter)
{
    if (!stateData_.renderPassHasBegun) {
        if (RenderHandleUtil::IsGpuImage(sourceHandle) && RenderHandleUtil::IsGpuImage(destinationHandle)) {
            if (RenderHandleUtil::IsDynamicResource(sourceHandle) ||
                RenderHandleUtil::IsDynamicResource(destinationHandle)) {
                AddBarrierPoint(RenderCommandType::BLIT_IMAGE);
            }

            RenderCommandBlitImage* data = AllocateRenderCommand<RenderCommandBlitImage>(allocator_);
            if (data) {
                data->srcHandle = sourceHandle;
                data->dstHandle = destinationHandle;
                data->imageBlit = imageBlit;
                data->filter = filter;
                // NOTE: desired layouts (barrier point needs to respect these)
                data->srcImageLayout = ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                data->dstImageLayout = ImageLayout::CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

                renderCommands_.push_back({ RenderCommandType::BLIT_IMAGE, data });
            }
        }
    } else {
        PLUGIN_LOG_E("RenderCommandList: BlitImage can only be called outside of render pass");
    }
}

void RenderCommandList::UpdateDescriptorSets(const BASE_NS::array_view<const RenderHandle> handles,
    const BASE_NS::array_view<const DescriptorSetLayoutBindingResources> bindingResources)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (handles.size() != bindingResources.size()) {
        PLUGIN_LOG_W("RENDER_VALIDATION: UpdateDescriptorSets handles and bindingResources size does not match");
    }
#endif
    const uint32_t count = static_cast<uint32_t>(Math::min(handles.size(), bindingResources.size()));
    if (count > 0U) {
        for (uint32_t idx = 0; idx < count; ++idx) {
            const auto& handleRef = handles[idx];
            const auto& bindingResRef = bindingResources[idx];
#if (RENDER_VALIDATION_ENABLED == 1)
            ValidateDescriptorTypeBinding(nodeName_, gpuResourceMgr_, bindingResRef);
#endif
#if (RENDER_VALIDATION_ENABLED == 1)
            if (bindingResRef.bindingMask != bindingResRef.descriptorSetBindingMask) {
                PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_UpdateDescriptorSets_bm_",
                    "RENDER_VALIDATION: invalid bindings in descriptor set update (node:%s)", nodeName_.c_str());
            }
#endif
            const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handleRef);
            if (handleType == RenderHandleType::DESCRIPTOR_SET) {
                const bool valid =
                    nodeContextDescriptorSetManager_.UpdateCpuDescriptorSet(handleRef, bindingResRef, gpuQueue_);
                if (valid) {
                    descriptorSetHandlesForUpdates_.push_back(handleRef);
                } else {
#if (RENDER_VALIDATION_ENABLED == 1)
                    PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_UpdateDescriptorSet_invalid_",
                        "RenderCommandList: invalid descriptor set bindings with update (node:%s)", nodeName_.c_str());
#endif
                }
            } else {
                PLUGIN_LOG_E("RenderCommandList: invalid handle for UpdateDescriptorSet");
            }
        }
    }
}

void RenderCommandList::UpdateDescriptorSet(
    const RenderHandle handle, const DescriptorSetLayoutBindingResources& bindingResources)
{
    UpdateDescriptorSets({ &handle, 1U }, { &bindingResources, 1U });
}

void RenderCommandList::BindDescriptorSets(
    const uint32_t firstSet, const BASE_NS::array_view<const BindDescriptorSetData> descriptorSetData)
{
    if (descriptorSetData.empty()) {
        return;
    }
    const uint32_t maxSetNumber = firstSet + static_cast<uint32_t>(descriptorSetData.size());
    if (maxSetNumber > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        PLUGIN_LOG_E("RenderCommandList::BindDescriptorSets: firstSet + handles.size() (%u) exceeds max count (%u)",
            maxSetNumber, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
        return;
    }

    ValidatePipeline();

#if (RENDER_VALIDATION_ENABLED == 1)
    if ((descriptorSetData.size() > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid inputs to BindDescriptorSets");
    }
    for (const auto& ref : descriptorSetData) {
        if (ref.dynamicOffsets.size() > PipelineLayoutConstants::MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT) {
            PLUGIN_LOG_E("RENDER_VALIDATION: invalid inputs to BindDescriptorSets");
        }
    }
#endif

    if (auto* data = AllocateRenderCommand<RenderCommandBindDescriptorSets>(allocator_); data) {
        *data = {}; // default

        data->psoHandle = stateData_.currentPsoHandle;
        data->firstSet = firstSet;
        data->setCount = static_cast<uint32_t>(descriptorSetData.size());

        uint32_t descriptorSetCounterForBarriers = 0;
        uint32_t currSet = firstSet;
        for (const auto& ref : descriptorSetData) {
            if (currSet < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
                // allocate offsets for this set
                if (!ref.dynamicOffsets.empty()) {
                    const uint32_t dynCount = static_cast<uint32_t>(ref.dynamicOffsets.size());
                    if (auto* doData = AllocateRenderData<uint32_t>(allocator_, dynCount); doData) {
                        auto& dynRef = data->descriptorSetDynamicOffsets[currSet];
                        dynRef.dynamicOffsets = doData;
                        dynRef.dynamicOffsetCount = dynCount;
                        CloneData(dynRef.dynamicOffsets, dynCount * sizeof(uint32_t), ref.dynamicOffsets.data(),
                            ref.dynamicOffsets.size_bytes());
                    }
                }

                data->descriptorSetHandles[currSet] = ref.handle;

                const bool hasDynamicBarrierResources =
                    nodeContextDescriptorSetManager_.HasDynamicBarrierResources(ref.handle);
                if (stateData_.renderPassHasBegun && hasDynamicBarrierResources) {
                    descriptorSetHandlesForBarriers_.push_back(ref.handle);
                    descriptorSetCounterForBarriers++;
                }
                stateData_.currentBoundSets[currSet].hasDynamicBarrierResources = hasDynamicBarrierResources;
                stateData_.currentBoundSets[currSet].descriptorSetHandle = ref.handle;
                stateData_.currentBoundSetsMask |= (1 << currSet);
                ++currSet;
            }
        }

        renderCommands_.push_back({ RenderCommandType::BIND_DESCRIPTOR_SETS, data });

        // if the currentBarrierPoint is null there has been some invalid bindings earlier
        if (stateData_.renderPassHasBegun && stateData_.currentBarrierPoint) {
            // add possible barriers before render pass
            stateData_.currentBarrierPoint->descriptorSetHandleCount += descriptorSetCounterForBarriers;
        } else if (stateData_.automaticBarriersEnabled) {
            stateData_.dirtyDescriptorSetsForBarriers = true;
        }
    }
}

void RenderCommandList::BindDescriptorSet(const uint32_t set, const BindDescriptorSetData& desriptorSetData)
{
    BindDescriptorSets(set, { &desriptorSetData, 1U });
}

void RenderCommandList::BindDescriptorSets(const uint32_t firstSet, const array_view<const RenderHandle> handles)
{
    BindDescriptorSetData bdsd[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT];
    const uint32_t count = Math::min((uint32_t)handles.size(), PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
    for (uint32_t idx = 0U; idx < count; ++idx) {
        bdsd[idx].handle = handles[idx];
    }
    BindDescriptorSets(firstSet, { bdsd, count });
}

void RenderCommandList::BindDescriptorSet(const uint32_t set, const RenderHandle handle)
{
    BindDescriptorSetData bdsd = { handle, {} };
    BindDescriptorSets(set, { &bdsd, 1U });
}

void RenderCommandList::BindDescriptorSet(
    const uint32_t set, const RenderHandle handle, const array_view<const uint32_t> dynamicOffsets)
{
    BindDescriptorSetData bdsd = { handle, dynamicOffsets };
    BindDescriptorSets(set, { &bdsd, 1U });
}

void RenderCommandList::BuildAccelerationStructures(const AccelerationStructureBuildGeometryData& geometry,
    const BASE_NS::array_view<const AccelerationStructureGeometryTrianglesData> triangles,
    const BASE_NS::array_view<const AccelerationStructureGeometryAabbsData> aabbs,
    const BASE_NS::array_view<const AccelerationStructureGeometryInstancesData> instances)
{
    if (!(triangles.empty() && aabbs.empty() && instances.empty())) {
#if (RENDER_VULKAN_RT_ENABLED == 1)
        RenderCommandBuildAccelerationStructure* data =
            AllocateRenderCommand<RenderCommandBuildAccelerationStructure>(allocator_);
        if (data) {
            data->type = geometry.info.type;
            data->flags = geometry.info.flags;
            data->mode = geometry.info.mode;
            data->srcAccelerationStructure = geometry.srcAccelerationStructure;
            data->dstAccelerationStructure = geometry.dstAccelerationStructure;
            data->scratchBuffer = geometry.scratchBuffer.handle;
            data->scratchOffset = geometry.scratchBuffer.offset;

            if (!triangles.empty()) {
                AccelerationStructureGeometryTrianglesData* trianglesData =
                    static_cast<AccelerationStructureGeometryTrianglesData*>(
                        AllocateRenderData(allocator_, std::alignment_of<AccelerationStructureGeometryTrianglesData>(),
                            sizeof(AccelerationStructureGeometryTrianglesData) * triangles.size()));
                data->trianglesData = trianglesData;
                data->trianglesView = { data->trianglesData, triangles.size() };
                for (size_t idx = 0; idx < triangles.size(); ++idx) {
                    data->trianglesView[idx] = triangles[idx];
                }
            }
            if (!aabbs.empty()) {
                AccelerationStructureGeometryAabbsData* aabbsData =
                    static_cast<AccelerationStructureGeometryAabbsData*>(
                        AllocateRenderData(allocator_, std::alignment_of<AccelerationStructureGeometryAabbsData>(),
                            sizeof(AccelerationStructureGeometryAabbsData) * aabbs.size()));
                data->aabbsData = aabbsData;
                data->aabbsView = { data->aabbsData, aabbs.size() };
                for (size_t idx = 0; idx < aabbs.size(); ++idx) {
                    data->aabbsView[idx] = aabbs[idx];
                }
            }
            if (!instances.empty()) {
                AccelerationStructureGeometryInstancesData* instancesData =
                    static_cast<AccelerationStructureGeometryInstancesData*>(
                        AllocateRenderData(allocator_, std::alignment_of<AccelerationStructureGeometryInstancesData>(),
                            sizeof(AccelerationStructureGeometryInstancesData) * instances.size()));
                data->instancesData = instancesData;
                data->instancesView = { data->instancesData, instances.size() };
                for (size_t idx = 0; idx < instances.size(); ++idx) {
                    data->instancesView[idx] = instances[idx];
                }
            }
            renderCommands_.push_back({ RenderCommandType::BUILD_ACCELERATION_STRUCTURE, data });
        }
#endif
    }
}

void RenderCommandList::ClearColorImage(
    const RenderHandle handle, const ClearColorValue color, const array_view<const ImageSubresourceRange> ranges)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    {
        if (!RenderHandleUtil::IsGpuImage(handle)) {
            PLUGIN_LOG_W("RENDER_VALIDATION: Invalid image handle given to ClearColorImage");
        }
        if (ranges.empty()) {
            PLUGIN_LOG_W("RENDER_VALIDATION: Invalid ranges given to ClearColorImage");
        }
        {
            const GpuImageDesc desc = gpuResourceMgr_.GetImageDescriptor(handle);
            if ((desc.usageFlags & CORE_IMAGE_USAGE_TRANSFER_DST_BIT) == 0) {
                PLUGIN_LOG_E("RENDER_VALIDATION: Image missing usage flag TRANSFER_DST for ClearColorImage command");
            }
        }
    }
#endif
    if (RenderHandleUtil::IsGpuImage(handle) && (!ranges.empty())) {
        AddBarrierPoint(RenderCommandType::CLEAR_COLOR_IMAGE);

        RenderCommandClearColorImage* data = AllocateRenderCommand<RenderCommandClearColorImage>(allocator_);
        if (data) {
            data->handle = handle;
            data->imageLayout = CORE_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            data->color = color;
            data->ranges = { AllocateRenderData<ImageSubresourceRange>(
                                 allocator_, static_cast<uint32_t>(ranges.size())),
                ranges.size() };
            if (!data->ranges.data()) {
                return;
            }
            CloneData(data->ranges.data(), data->ranges.size_bytes(), ranges.data(), ranges.size_bytes());

            renderCommands_.push_back({ RenderCommandType::CLEAR_COLOR_IMAGE, data });
        }
    }
}

void RenderCommandList::SetDynamicStateViewport(const ViewportDesc& viewportDesc)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateViewport(nodeName_, viewportDesc);
#endif
    RenderCommandDynamicStateViewport* data = AllocateRenderCommand<RenderCommandDynamicStateViewport>(allocator_);
    if (data) {
        data->viewportDesc = viewportDesc;
        data->viewportDesc.width = Math::max(1.0f, data->viewportDesc.width);
        data->viewportDesc.height = Math::max(1.0f, data->viewportDesc.height);
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_VIEWPORT, data });
    }
}

void RenderCommandList::SetDynamicStateScissor(const ScissorDesc& scissorDesc)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateScissor(nodeName_, scissorDesc);
#endif
    RenderCommandDynamicStateScissor* data = AllocateRenderCommand<RenderCommandDynamicStateScissor>(allocator_);
    if (data) {
        data->scissorDesc = scissorDesc;
        data->scissorDesc.extentWidth = Math::max(1u, data->scissorDesc.extentWidth);
        data->scissorDesc.extentHeight = Math::max(1u, data->scissorDesc.extentHeight);
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_SCISSOR, data });
    }
}

void RenderCommandList::SetDynamicStateLineWidth(const float lineWidth)
{
    RenderCommandDynamicStateLineWidth* data = AllocateRenderCommand<RenderCommandDynamicStateLineWidth>(allocator_);
    if (data) {
        data->lineWidth = lineWidth;
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_LINE_WIDTH, data });
    }
}

void RenderCommandList::SetDynamicStateDepthBias(
    const float depthBiasConstantFactor, const float depthBiasClamp, const float depthBiasSlopeFactor)
{
    RenderCommandDynamicStateDepthBias* data = AllocateRenderCommand<RenderCommandDynamicStateDepthBias>(allocator_);
    if (data) {
        data->depthBiasConstantFactor = depthBiasConstantFactor;
        data->depthBiasClamp = depthBiasClamp;
        data->depthBiasSlopeFactor = depthBiasSlopeFactor;
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_DEPTH_BIAS, data });
    }
}

void RenderCommandList::SetDynamicStateBlendConstants(const array_view<const float> blendConstants)
{
    constexpr uint32_t THRESHOLD = 4;
#if (RENDER_VALIDATION_ENABLED == 1)
    if (blendConstants.size() > THRESHOLD) {
        PLUGIN_LOG_E("RenderCommandList: blend constant count (%zu) exceeds supported max (%u)", blendConstants.size(),
            THRESHOLD);
    }
#endif
    RenderCommandDynamicStateBlendConstants* data =
        AllocateRenderCommand<RenderCommandDynamicStateBlendConstants>(allocator_);
    if (data) {
        *data = {};
        const uint32_t bcCount = Math::min(static_cast<uint32_t>(blendConstants.size()), THRESHOLD);
        for (uint32_t idx = 0; idx < bcCount; ++idx) {
            data->blendConstants[idx] = blendConstants[idx];
        }
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_BLEND_CONSTANTS, data });
    }
}

void RenderCommandList::SetDynamicStateDepthBounds(const float minDepthBounds, const float maxDepthBounds)
{
    RenderCommandDynamicStateDepthBounds* data =
        AllocateRenderCommand<RenderCommandDynamicStateDepthBounds>(allocator_);
    if (data) {
        data->minDepthBounds = minDepthBounds;
        data->maxDepthBounds = maxDepthBounds;
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_DEPTH_BOUNDS, data });
    }
}

void RenderCommandList::SetDynamicStateStencilCompareMask(const StencilFaceFlags faceMask, const uint32_t compareMask)
{
    RenderCommandDynamicStateStencil* data = AllocateRenderCommand<RenderCommandDynamicStateStencil>(allocator_);
    if (data) {
        data->dynamicState = StencilDynamicState::COMPARE_MASK;
        data->faceMask = faceMask;
        data->mask = compareMask;
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_STENCIL, data });
    }
}

void RenderCommandList::SetDynamicStateStencilWriteMask(const StencilFaceFlags faceMask, const uint32_t writeMask)
{
    RenderCommandDynamicStateStencil* data = AllocateRenderCommand<RenderCommandDynamicStateStencil>(allocator_);
    if (data) {
        data->dynamicState = StencilDynamicState::WRITE_MASK;
        data->faceMask = faceMask;
        data->mask = writeMask;
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_STENCIL, data });
    }
}

void RenderCommandList::SetDynamicStateStencilReference(const StencilFaceFlags faceMask, const uint32_t reference)
{
    RenderCommandDynamicStateStencil* data = AllocateRenderCommand<RenderCommandDynamicStateStencil>(allocator_);
    if (data) {
        data->dynamicState = StencilDynamicState::REFERENCE;
        data->faceMask = faceMask;
        data->mask = reference;
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_STENCIL, data });
    }
}

void RenderCommandList::SetDynamicStateFragmentShadingRate(
    const Size2D& fragmentSize, const FragmentShadingRateCombinerOps& combinerOps)
{
    RenderCommandDynamicStateFragmentShadingRate* data =
        AllocateRenderCommand<RenderCommandDynamicStateFragmentShadingRate>(allocator_);
    if (data) {
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateFragmentShadingRate(fragmentSize);
#endif
        // valid values for sizes from 0-4
        constexpr uint32_t maxValue { 4u };
        constexpr uint32_t valueMapper[maxValue + 1u] = { 1u, 1u, 2u, 2u, 4u };
        Size2D fs = fragmentSize;
        fs.width = (fs.width <= maxValue) ? valueMapper[fs.width] : maxValue;
        fs.height = (fs.height <= maxValue) ? valueMapper[fs.height] : maxValue;

        data->fragmentSize = fs;
        data->combinerOps = combinerOps;
        renderCommands_.push_back({ RenderCommandType::DYNAMIC_STATE_FRAGMENT_SHADING_RATE, data });
    }
}

void RenderCommandList::SetExecuteBackendFramePosition()
{
    if (stateData_.executeBackendFrameSet == false) {
        AddBarrierPoint(RenderCommandType::EXECUTE_BACKEND_FRAME_POSITION);

        RenderCommandExecuteBackendFramePosition* data =
            AllocateRenderCommand<RenderCommandExecuteBackendFramePosition>(allocator_);
        if (data) {
            data->id = 0;
            renderCommands_.push_back({ RenderCommandType::EXECUTE_BACKEND_FRAME_POSITION, data });
            stateData_.executeBackendFrameSet = true;
        }
    } else {
        PLUGIN_LOG_E("RenderCommandList: there can be only one SetExecuteBackendFramePosition() -call per frame");
    }
}

void RenderCommandList::ValidateRenderPass(const RenderPassDesc& renderPassDesc)
{
    if (stateData_.renderPassHasBegun) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_ValidateRenderPass_hasbegun_",
            "RenderCommandList: render pass is active, needs to be end before starting a new (node: %s)",
            nodeName_.c_str());
#endif
        stateData_.validCommandList = false;
    }
    // validate render pass attachments
    for (uint32_t idx = 0; idx < renderPassDesc.attachmentCount; ++idx) {
        if (!RenderHandleUtil::IsValid(renderPassDesc.attachmentHandles[idx])) {
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_ValidateRenderPass_attachments_",
                "RenderCommandList: Invalid render pass attachment handle in index: %u (node:%s)", idx,
                nodeName_.c_str());
#endif
            stateData_.validCommandList = false;
        }
    }
}

void RenderCommandList::ValidatePipeline()
{
    if (!stateData_.validPso) {
        stateData_.validCommandList = false;
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_E(nodeName_ + "_RCL_ValidatePipeline_", "RenderCommandList: PSO not bound.");
#endif
    }
}

void RenderCommandList::ValidatePipelineLayout()
{
    if (stateData_.checkBindPipelineLayout) {
        stateData_.checkBindPipelineLayout = false;
        // fast check without validation
        const uint32_t pipelineLayoutSetsMask =
            RenderHandleUtil::GetPipelineLayoutDescriptorSetMask(stateData_.currentPsoHandle);
        if ((stateData_.currentBoundSetsMask & pipelineLayoutSetsMask) != pipelineLayoutSetsMask) {
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_ONCE_E(
                "RenderCommandList::ValidatePipelineLayout", "RenderCommandList: not all needed descriptor sets bound");
#endif
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        const RenderHandleType rhType = RenderHandleUtil::GetHandleType(stateData_.currentPsoHandle);
        const PipelineLayout& pl = (rhType == RenderHandleType::COMPUTE_PSO)
                                       ? psoMgr_.GetComputePsoPipelineLayout(stateData_.currentPsoHandle)
                                       : psoMgr_.GetGraphicsPsoPipelineLayout(stateData_.currentPsoHandle);
        const uint32_t plDescriptorSetCount = pl.descriptorSetCount;
        uint32_t bindCount = 0;
        uint32_t bindSetIndices[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] { ~0u, ~0u, ~0u, ~0u };
        for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
            const DescriptorSetBind& currSet = stateData_.currentBoundSets[idx];
            if (RenderHandleUtil::IsValid(currSet.descriptorSetHandle)) {
                bindCount++;
                bindSetIndices[idx] = idx;
            }
        }
        if (bindCount < plDescriptorSetCount) {
            PLUGIN_LOG_E("RENDER_VALIDATION: not all pipeline layout required descriptor sets bound");
        }
#endif
    }
}

const CORE_NS::IInterface* RenderCommandList::GetInterface(const BASE_NS::Uid& uid) const
{
    if ((uid == IRenderCommandList::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* RenderCommandList::GetInterface(const BASE_NS::Uid& uid)
{
    if ((uid == IRenderCommandList::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderCommandList::Ref() {}

void RenderCommandList::Unref() {}
RENDER_END_NAMESPACE()
