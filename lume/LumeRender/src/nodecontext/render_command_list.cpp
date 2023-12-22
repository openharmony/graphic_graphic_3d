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
namespace {
#if (RENDER_VALIDATION_ENABLED == 1)
void ValidateImageUsageFlags(const GpuResourceManager& gpuResourceMgr, const RenderHandle handl,
    const ImageUsageFlags imageUsageFlags, const string_view str)
{
    if ((gpuResourceMgr.GetImageDescriptor(handl).usageFlags & imageUsageFlags) == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: gpu image (handle: %" PRIu64
                     ") (name: %s), not created with needed flags: %s ",
            handl.id, gpuResourceMgr.GetName(handl).c_str(), str.data());
    }
}

void ValidateBufferUsageFlags(const GpuResourceManager& gpuResourceMgr, const RenderHandle handl,
    const BufferUsageFlags bufferUsageFlags, const string_view str)
{
    if ((gpuResourceMgr.GetBufferDescriptor(handl).usageFlags & bufferUsageFlags) == 0) {
        PLUGIN_LOG_E("RENDER_VALIDATION: gpu buffer (handle: %" PRIu64
                     ") (name: %s), not created with needed flags: %s",
            handl.id, gpuResourceMgr.GetName(handl).c_str(), str.data());
    }
}

void ValidateDescriptorTypeBinding(
    const GpuResourceManager& gpuResourceMgr, const DescriptorSetLayoutBindingResources& bindingRes)
{
    for (const auto& ref : bindingRes.buffers) {
        if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER) {
            ValidateBufferUsageFlags(gpuResourceMgr, ref.resource.handle, CORE_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT,
                "CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER) {
            ValidateBufferUsageFlags(gpuResourceMgr, ref.resource.handle, CORE_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
                "CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
            ValidateBufferUsageFlags(gpuResourceMgr, ref.resource.handle, CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                "CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
            ValidateBufferUsageFlags(gpuResourceMgr, ref.resource.handle, CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                "CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) {
            ValidateBufferUsageFlags(gpuResourceMgr, ref.resource.handle, CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                "CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) {
            ValidateBufferUsageFlags(gpuResourceMgr, ref.resource.handle, CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                "CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE) {
        } else {
            PLUGIN_LOG_E("RENDER_VALIDATION: unsupported buffer descriptor type: %u", ref.binding.descriptorType);
        }
    }
    for (const auto& ref : bindingRes.images) {
        if ((ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) ||
            (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE)) {
            ValidateImageUsageFlags(
                gpuResourceMgr, ref.resource.handle, CORE_IMAGE_USAGE_SAMPLED_BIT, "CORE_IMAGE_USAGE_SAMPLED_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE) {
            ValidateImageUsageFlags(
                gpuResourceMgr, ref.resource.handle, CORE_IMAGE_USAGE_STORAGE_BIT, "CORE_IMAGE_USAGE_STORAGE_BIT");
        } else if (ref.binding.descriptorType == CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT) {
            ValidateImageUsageFlags(gpuResourceMgr, ref.resource.handle, CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
                "CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
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

void ValidateRenderPassAttachment(const GpuResourceManager& gpuResourceMgr, const RenderPassDesc& renderPassDsc)
{
    const GpuImageDesc baseDesc = gpuResourceMgr.GetImageDescriptor(renderPassDsc.attachmentHandles[0]);
    const uint32_t baseWidth = baseDesc.width;
    const uint32_t baseHeight = baseDesc.height;
    for (uint32_t attachmentIdx = 1; attachmentIdx < renderPassDsc.attachmentCount; ++attachmentIdx) {
        const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(renderPassDsc.attachmentHandles[attachmentIdx]);
        if (desc.width != baseWidth || desc.height != baseHeight) {
            PLUGIN_LOG_E("RENDER_VALIDATION: render pass attachment size does not match with attachment index: %u",
                attachmentIdx);
            PLUGIN_LOG_E("RENDER_VALIDATION: baseWidth:%u baseHeight:%u currWidth:%u currHeight:%u", baseWidth,
                baseHeight, desc.width, desc.height);
        }
    }
    if ((renderPassDsc.renderArea.extentWidth == 0) || (renderPassDsc.renderArea.extentHeight == 0)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: render area cannot be zero (width: %u, height: %u)",
            renderPassDsc.renderArea.extentWidth, renderPassDsc.renderArea.extentHeight);
    }
    if ((renderPassDsc.renderArea.offsetX >= static_cast<int32_t>(baseWidth)) ||
        (renderPassDsc.renderArea.offsetY >= static_cast<int32_t>(baseHeight))) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: render area offset cannot go out of screen (offsetX: %i, offsetY: %i) (baseWidth: "
            "%u, "
            "baseHeight: %u)",
            renderPassDsc.renderArea.offsetX, renderPassDsc.renderArea.offsetY, baseWidth, baseHeight);
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

void ValidateViewport(const ViewportDesc& vd)
{
    if ((vd.width < 1.0f) || (vd.height < 1.0f)) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION : viewport width (%f) and height (%f) must be one or larger", vd.width, vd.height);
    }
}

void ValidateScissor(const ScissorDesc& sd)
{
    if ((sd.extentWidth == 0) || (sd.extentHeight == 0)) {
        PLUGIN_LOG_E("RENDER_VALIDATION : scissor extentWidth (%u) and scissor extentHeight (%u) cannot be zero",
            sd.extentWidth, sd.extentHeight);
    }
}
#endif

constexpr size_t MEMORY_ALIGNMENT { 16 };
constexpr size_t BYTE_SIZE_ALIGNMENT { 64 };
constexpr size_t FRAME_RESERVE_EXTRA_DIVIDE { 8 };
constexpr size_t MIN_ALLOCATION_SIZE { 1024 * 2 };

// automatic acquire and release barriers
constexpr uint32_t INITIAL_MULTI_QUEUE_BARRIER_COUNT { 2u };

size_t GetAlignedBytesize(const size_t byteSize, const size_t alignment)
{
    return (byteSize + alignment - 1) & (~(alignment - 1));
}

void* AllocateRenderData(RenderCommandList::LinearAllocatorStruct& allocator, const size_t byteSz)
{
    PLUGIN_ASSERT(byteSz > 0);
    void* rc = nullptr;
    if (!allocator.allocators.empty()) {
        const size_t currentIndex = allocator.allocators.size() - 1;
        rc = allocator.allocators[currentIndex]->Allocate(byteSz);
    }

    if (rc == nullptr) { // current allocator is out of memory
        size_t allocatorByteSize = Math::max(MIN_ALLOCATION_SIZE, GetAlignedBytesize(byteSz, BYTE_SIZE_ALIGNMENT));
        const size_t currentIndex = allocator.allocators.size();
        if (currentIndex > 0) {
            allocatorByteSize =
                Math::max(allocatorByteSize, allocator.allocators[currentIndex - 1]->GetCurrentByteSize() * 2u);
        }
        allocator.allocators.emplace_back(make_unique<LinearAllocator>(allocatorByteSize, MEMORY_ALIGNMENT));

        rc = allocator.allocators[currentIndex]->Allocate(byteSz);
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
    return static_cast<T*>(AllocateRenderData(allocator, sizeof(T) * count));
}

template<typename T>
T* AllocateRenderCommand(RenderCommandList::LinearAllocatorStruct& allocator)
{
    return static_cast<T*>(AllocateRenderData(allocator, sizeof(T)));
}
} // namespace

RenderCommandList::RenderCommandList(NodeContextDescriptorSetManager& nodeContextDescriptorSetMgr,
    const GpuResourceManager& gpuResourceMgr, const NodeContextPsoManager& nodeContextPsoMgr, const GpuQueue& queue,
    const bool enableMultiQueue)
    : IRenderCommandList(),
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
        allocator_.allocators.emplace_back(make_unique<LinearAllocator>(memAllocationByteSize, alignment));
    }

    ResetStateData();

    const auto clearAndReserve = [](auto& vec) {
        const size_t count = vec.size();
        vec.clear();
        vec.reserve(count);
    };

    clearAndReserve(renderCommands_);
    clearAndReserve(customBarriers_);
    clearAndReserve(vertexInputBufferBarriers_);
    clearAndReserve(descriptorSetHandlesForBarriers_);

    validReleaseAcquire_ = false;
    hasMultiRenderCommandListSubpasses_ = false;
    multiRendercommandListSubpassCount_ = 1;
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
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((!stateData_.validCommandList) || stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid state data in render command list");
    }
#endif

    return array_view<const RenderCommandWithType>(renderCommands_.data(), renderCommands_.size());
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
    return hasMultiRenderCommandListSubpasses_;
}

uint32_t RenderCommandList::GetMultiRenderCommandListSubpassCount() const
{
    return multiRendercommandListSubpassCount_;
}

array_view<const CommandBarrier> RenderCommandList::GetCustomBarriers() const
{
    return array_view<const CommandBarrier>(customBarriers_.data(), customBarriers_.size());
}

array_view<const VertexBuffer> RenderCommandList::GetVertexInputBufferBarriers() const
{
    return array_view<const VertexBuffer>(vertexInputBufferBarriers_.data(), vertexInputBufferBarriers_.size());
}

array_view<const RenderHandle> RenderCommandList::GetDescriptorSetHandles() const
{
    return { descriptorSetHandlesForBarriers_.data(), descriptorSetHandlesForBarriers_.size() };
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
        const uint32_t descriptorSetBeginIndex = (uint32_t)descriptorSetHandlesForBarriers_.size();
        data->descriptorSetHandleIndexBegin = descriptorSetBeginIndex;
        data->descriptorSetHandleCount = 0;
        // update new index (only valid with render pass)
        data->vertexIndexBarrierIndexBegin = (uint32_t)vertexInputBufferBarriers_.size();
        data->vertexIndexBarrierCount = 0;

        // barriers are always needed e.g. when dynamic resource is bound for writing in multiple dispatches
        const bool handleDescriptorSets = stateData_.dirtyDescriptorSetsForBarriers ||
                                          renderCommandType == RenderCommandType::DISPATCH ||
                                          renderCommandType == RenderCommandType::DISPATCH_INDIRECT;
        if (handleDescriptorSets) {
            stateData_.dirtyDescriptorSetsForBarriers = false;
            for (uint32_t idx = 0; idx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++idx) {
                // only add descriptor set handles for barriers if there are dynamic barrier resources
                if (stateData_.currentBoundSets[idx].hasDynamicBarrierResources) {
                    descriptorSetHandlesForBarriers_.emplace_back(stateData_.currentBoundSets[idx].descriptorSetHandle);
                }
            }
            data->descriptorSetHandleCount =
                (uint32_t)descriptorSetHandlesForBarriers_.size() - descriptorSetBeginIndex;
        }

        const bool handleCustomBarriers =
            ((!customBarriers_.empty()) && stateData_.currentCustomBarrierIndices.dirtyCustomBarriers);
        if (handleCustomBarriers) {
            const int32_t newCount = (int32_t)customBarriers_.size() - stateData_.currentCustomBarrierIndices.prevSize;
            if (newCount > 0) {
                data->customBarrierIndexBegin = (uint32_t)stateData_.currentCustomBarrierIndices.prevSize;
                data->customBarrierCount = (uint32_t)newCount;

                stateData_.currentCustomBarrierIndices.prevSize = (int32_t)customBarriers_.size();
                stateData_.currentCustomBarrierIndices.dirtyCustomBarriers = false;
            }
        }

        // store current barrier point for render command list
        // * binding descriptor sets (with dynamic barrier resources)
        // * binding vertex and index buffers (with dynamic barrier resources)
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
        PLUGIN_LOG_E("RENDER_VALIDATION: render pass not active (begin before draw)");
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
        PLUGIN_LOG_E("RENDER_VALIDATION: render pass not active (begin before draw)");
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
#endif

    if (stateData_.renderPassHasBegun) {
        ValidatePipeline();
        ValidatePipelineLayout();

        RenderCommandDrawIndirect* data = AllocateRenderCommand<RenderCommandDrawIndirect>(allocator_);
        if (data) {
            data->drawType = DrawType::DRAW_INDIRECT;
            data->argsHandle = bufferHandle;
            data->offset = offset;
            data->drawCount = drawCount;
            data->stride = stride;

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
#endif

    if (stateData_.renderPassHasBegun) {
        ValidatePipeline();
        ValidatePipelineLayout();

        RenderCommandDrawIndirect* data = AllocateRenderCommand<RenderCommandDrawIndirect>(allocator_);
        if (data) {
            data->drawType = DrawType::DRAW_INDEXED_INDIRECT;
            data->argsHandle = bufferHandle;
            data->offset = offset;
            data->drawCount = drawCount;
            data->stride = stride;

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
    if (stateData_.currentPsoHandle.id == psoHandle.id) {
        return; // early out
    }

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
            PLUGIN_LOG_E("RENDER_VALIDATION: bind pipeline after render pass begin");
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

void RenderCommandList::PushConstant(const RENDER_NS::PushConstant& pushConstant, const uint8_t* data)
{
    ValidatePipeline();

    // push constant is not used/allocated if byte size is bigger than supported max
    if ((pushConstant.byteSize > 0) &&
        (pushConstant.byteSize <= PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE) && data) {
        RenderCommandPushConstant* rc = AllocateRenderCommand<RenderCommandPushConstant>(allocator_);
        uint8_t* pushData = static_cast<uint8_t*>(AllocateRenderData(allocator_, pushConstant.byteSize));
        if (rc && pushData) {
            rc->psoHandle = stateData_.currentPsoHandle;
            rc->pushConstant = pushConstant;
            rc->data = pushData;
            const bool res = CloneData(rc->data, pushConstant.byteSize, data, pushConstant.byteSize);
            PLUGIN_UNUSED(res);
            PLUGIN_ASSERT(res);

            renderCommands_.emplace_back(RenderCommandWithType { RenderCommandType::PUSH_CONSTANT, rc });
        }
    } else if (pushConstant.byteSize > 0) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RENDER_VALIDATION: push constant byte size must be smaller or equal to %u bytes.",
            PipelineLayoutConstants::MAX_PUSH_CONSTANT_BYTE_SIZE);
#endif
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
                const size_t currCount = vertexInputBufferBarriers_.size();
                vertexInputBufferBarriers_.resize(currCount + static_cast<size_t>(dynamicBarrierVertexBufferCount));
                for (uint32_t dynIdx = 0; dynIdx < dynamicBarrierVertexBufferCount; ++dynIdx) {
                    vertexInputBufferBarriers_[currCount + dynIdx] = dynamicBarrierVertexBuffers[dynIdx];
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
            vertexInputBufferBarriers_.push_back(
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

    if (renderPassDesc.subpassCount != static_cast<uint32_t>(subpassDescs.size())) {
        PLUGIN_LOG_E(
            "RENDER_VALIDATION: BeginRenderPass renderPassDesc.subpassCount (%u) must match subpassDescs size (%u)",
            renderPassDesc.subpassCount, static_cast<uint32_t>(subpassDescs.size()));
        stateData_.validCommandList = false;
    }
    if (stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("RenderCommandList: render pass is active, needs to be end before starting a new");
        stateData_.validCommandList = false;
    }
    stateData_.renderPassHasBegun = true;
    stateData_.renderPassStartIndex = 0;
    stateData_.renderPassSubpassCount = renderPassDesc.subpassCount;

    if (renderPassDesc.attachmentCount > 0) {
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateRenderPassAttachment(gpuResourceMgr_, renderPassDesc);
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
            if (!data->subpasses.data()) {
                return;
            }

            data->subpassResourceStates = { AllocateRenderData<RenderPassAttachmentResourceStates>(
                                                allocator_, renderPassDesc.subpassCount),
                renderPassDesc.subpassCount };
            if (!data->subpassResourceStates.data()) {
                return;
            }

            CloneData(
                data->subpasses.data(), data->subpasses.size_bytes(), subpassDescs.data(), subpassDescs.size_bytes());

            for (size_t subpassIdx = 0; subpassIdx < subpassDescs.size(); ++subpassIdx) {
                const auto& subpassRef = subpassDescs[subpassIdx];

                RenderPassAttachmentResourceStates& subpassResourceStates = data->subpassResourceStates[subpassIdx];
                subpassResourceStates = {};

                ProcessInputAttachments(renderPassDesc, subpassRef, subpassResourceStates);
                ProcessColorAttachments(renderPassDesc, subpassRef, subpassResourceStates);
                ProcessResolveAttachments(renderPassDesc, subpassRef, subpassResourceStates);
                ProcessDepthAttachments(renderPassDesc, subpassRef, subpassResourceStates);
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

    if (stateData_.renderPassHasBegun) {
        PLUGIN_LOG_E("BeginRenderPass: render pass is active, needs to be end before starting a new");
        stateData_.validCommandList = false;
    }
    if (subpassStartIdx >= renderPassDesc.subpassCount) {
        PLUGIN_LOG_E(
            "RenderCommandList: BeginRenderPass: subpassStartIdx (%u) must be smaller than renderPassDesc.subpassCount "
            "(%u)",
            subpassStartIdx, renderPassDesc.subpassCount);
        stateData_.validCommandList = false;
    }

    if (hasMultiRenderCommandListSubpasses_) {
        PLUGIN_LOG_E("RenderCommandList: BeginRenderPass: creating multiple render node subpasses not supported");
        stateData_.validCommandList = false;
    } else if (renderPassDesc.subpassCount > 1) {
        hasMultiRenderCommandListSubpasses_ = true;
        multiRendercommandListSubpassCount_ = renderPassDesc.subpassCount;
    }

    stateData_.renderPassHasBegun = true;
    stateData_.renderPassStartIndex = subpassStartIdx;
    stateData_.renderPassSubpassCount = renderPassDesc.subpassCount;

    if (renderPassDesc.attachmentCount > 0) {
#if (RENDER_VALIDATION_ENABLED == 1)
        ValidateRenderPassAttachment(gpuResourceMgr_, renderPassDesc);
#endif
        AddBarrierPoint(RenderCommandType::BEGIN_RENDER_PASS);

        if (auto* data = AllocateRenderCommand<RenderCommandBeginRenderPass>(allocator_); data) {
            // NOTE: hashed in the backend
            data->beginType = RenderPassBeginType::RENDER_PASS_BEGIN;
            data->renderPassDesc = renderPassDesc;
            data->subpassStartIndex = subpassStartIdx;
            // if false -> initial layout is undefined
            data->enableAutomaticLayoutChanges = stateData_.automaticBarriersEnabled;

            data->subpasses = { AllocateRenderData<RenderPassSubpassDesc>(allocator_, renderPassDesc.subpassCount),
                renderPassDesc.subpassCount };
            if (!data->subpasses.data()) {
                return;
            }

            data->subpassResourceStates = { AllocateRenderData<RenderPassAttachmentResourceStates>(
                                                allocator_, renderPassDesc.subpassCount),
                renderPassDesc.subpassCount };
            if (!data->subpassResourceStates.data()) {
                return;
            }

            for (size_t subpassIdx = 0; subpassIdx < data->subpasses.size(); ++subpassIdx) {
                RenderPassAttachmentResourceStates& subpassResourceStates = data->subpassResourceStates[subpassIdx];
                subpassResourceStates = {};
                data->subpasses[subpassIdx] = {};

                if (subpassIdx == subpassStartIdx) {
                    data->subpasses[subpassIdx] = subpassDesc;
                    ProcessInputAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                    ProcessColorAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                    ProcessResolveAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                    ProcessDepthAttachments(renderPassDesc, subpassDesc, subpassResourceStates);
                }
            }

            // render pass layouts will be updated by render graph
            renderCommands_.push_back({ RenderCommandType::BEGIN_RENDER_PASS, data });
        }
    }
}

void RenderCommandList::ProcessInputAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    for (uint32_t idx = 0; idx < subpassRef.inputAttachmentCount; ++idx) {
        const uint32_t attachmentIndex = subpassRef.inputAttachmentIndices[idx];

        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
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
        ValidateImageUsageFlags(gpuResourceMgr_, handle, ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT");
#endif
    }
}

void RenderCommandList::ProcessColorAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    for (uint32_t idx = 0; idx < subpassRef.colorAttachmentCount; ++idx) {
        const uint32_t attachmentIndex = subpassRef.colorAttachmentIndices[idx];

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

#if (RENDER_VALIDATION_ENABLED == 1)
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        ValidateImageUsageFlags(gpuResourceMgr_, handle, ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
#endif
    }
}

void RenderCommandList::ProcessResolveAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    for (uint32_t idx = 0; idx < subpassRef.resolveAttachmentCount; ++idx) {
        const uint32_t attachmentIndex = subpassRef.resolveAttachmentIndices[idx];

        // NOTE: mipLevel and layers are not updated to GpuResourceState
        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |= CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        refState.pipelineStageFlags |= CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        refState.gpuQueue = gpuQueue_;
        subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

#if (RENDER_VALIDATION_ENABLED == 1)
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        ValidateImageUsageFlags(gpuResourceMgr_, handle, ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT");
#endif
    }
}

void RenderCommandList::ProcessDepthAttachments(const RenderPassDesc& renderPassDsc,
    const RenderPassSubpassDesc& subpassRef, RenderPassAttachmentResourceStates& subpassResourceStates)
{
    if (subpassRef.depthAttachmentCount == 1) {
        const uint32_t attachmentIndex = subpassRef.depthAttachmentIndex;

        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |=
            (CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT);
        refState.pipelineStageFlags |=
            (CORE_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | CORE_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
        refState.gpuQueue = gpuQueue_;
        subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

#if (RENDER_VALIDATION_ENABLED == 1)
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        ValidateImageUsageFlags(gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT");
#endif
    }
    if ((subpassRef.depthAttachmentCount == 1) && (subpassRef.depthResolveAttachmentCount == 1)) {
        const uint32_t attachmentIndex = subpassRef.depthResolveAttachmentIndex;

        GpuResourceState& refState = subpassResourceStates.states[attachmentIndex];
        refState.shaderStageFlags |= CORE_SHADER_STAGE_FRAGMENT_BIT;
        refState.accessFlags |= CORE_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        refState.pipelineStageFlags |= CORE_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        refState.gpuQueue = gpuQueue_;
        subpassResourceStates.layouts[attachmentIndex] = CORE_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

#if (RENDER_VALIDATION_ENABLED == 1)
        const RenderHandle handle = renderPassDsc.attachmentHandles[attachmentIndex];
        ValidateImageUsageFlags(gpuResourceMgr_, handle,
            ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
            "CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT");
#endif
    }
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
        PLUGIN_LOG_E("RenderCommandList: render pass needs to begin before calling end");
        stateData_.validCommandList = false;
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

    customBarriers_.emplace_back(std::move(cb));

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

        customBarriers_.emplace_back(std::move(cb));

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

        customBarriers_.emplace_back(std::move(cb));

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

void RenderCommandList::UpdateDescriptorSet(
    const RenderHandle handle, const DescriptorSetLayoutBindingResources& bindingResources)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateDescriptorTypeBinding(gpuResourceMgr_, bindingResources);
#endif
#if (RENDER_VALIDATION_ENABLED == 1)
    if (bindingResources.bindingMask != bindingResources.descriptorSetBindingMask) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid bindings in descriptor set update");
    }
#endif
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(handle);
    if (handleType == RenderHandleType::DESCRIPTOR_SET) {
        nodeContextDescriptorSetManager_.UpdateCpuDescriptorSet(handle, bindingResources, gpuQueue_);
        RenderCommandUpdateDescriptorSets* data = AllocateRenderCommand<RenderCommandUpdateDescriptorSets>(allocator_);
        if (data) {
            *data = {}; // default
            data->descriptorSetHandles[0] = handle;

            renderCommands_.push_back({ RenderCommandType::UPDATE_DESCRIPTOR_SETS, data });
        }
    } else {
        PLUGIN_LOG_E("RenderCommandList: invalid handle for UpdateDescriptorSet");
    }
}

void RenderCommandList::BindDescriptorSets(const uint32_t firstSet, const array_view<const RenderHandle> handles,
    const array_view<const uint32_t> dynamicOffsets)
{
    const uint32_t maxSetNumber = firstSet + static_cast<uint32_t>(handles.size());
    if (maxSetNumber > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
        PLUGIN_LOG_E("RenderCommandList::BindDescriptorSets: firstSet + handles.size() (%u) exceeds max count (%u)",
            maxSetNumber, PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
        return;
    }

    ValidatePipeline();

#if (RENDER_VALIDATION_ENABLED == 1)
    if ((handles.size() > PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) ||
        (dynamicOffsets.size() > PipelineLayoutConstants::MAX_DYNAMIC_DESCRIPTOR_OFFSET_COUNT)) {
        PLUGIN_LOG_E("RENDER_VALIDATION: invalid inputs to BindDescriptorSets");
    }
#endif

    if (auto* data = AllocateRenderCommand<RenderCommandBindDescriptorSets>(allocator_); data) {
        *data = {}; // default

        if (!dynamicOffsets.empty()) {
            if (auto* doData = AllocateRenderData(allocator_, dynamicOffsets.size() * sizeof(uint32_t)); doData) {
                data->dynamicOffsets = reinterpret_cast<uint32_t*>(doData);
                data->dynamicOffsetCount = static_cast<uint32_t>(dynamicOffsets.size());
                CloneData(data->dynamicOffsets, dynamicOffsets.size_bytes(), dynamicOffsets.data(),
                    dynamicOffsets.size_bytes());
            }
        }

        data->psoHandle = stateData_.currentPsoHandle;
        data->firstSet = firstSet;
        data->setCount = static_cast<uint32_t>(handles.size());

        uint32_t descriptorSetCounterForBarriers = 0;
        uint32_t currSet = firstSet;
        for (const RenderHandle currHandle : handles) {
            PLUGIN_ASSERT(currSet < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT);
            data->descriptorSetHandles[currSet] = currHandle;

            const bool hasDynamicBarrierResources =
                nodeContextDescriptorSetManager_.HasDynamicBarrierResources(currHandle);
            if (stateData_.renderPassHasBegun && hasDynamicBarrierResources) {
                descriptorSetHandlesForBarriers_.emplace_back(currHandle);
                descriptorSetCounterForBarriers++;
            }
            stateData_.currentBoundSets[currSet].hasDynamicBarrierResources = hasDynamicBarrierResources;
            stateData_.currentBoundSets[currSet].descriptorSetHandle = currHandle;
            stateData_.currentBoundSetsMask |= (1 << currSet);
            ++currSet;
        }

        renderCommands_.push_back({ RenderCommandType::BIND_DESCRIPTOR_SETS, data });

        if (stateData_.renderPassHasBegun) { // add possible barriers before render pass
            PLUGIN_ASSERT(stateData_.currentBarrierPoint);
            stateData_.currentBarrierPoint->descriptorSetHandleCount += descriptorSetCounterForBarriers;
        } else if (stateData_.automaticBarriersEnabled) {
            stateData_.dirtyDescriptorSetsForBarriers = true;
        }
    }
}

void RenderCommandList::BindDescriptorSets(const uint32_t firstSet, const array_view<const RenderHandle> handles)
{
    BindDescriptorSets(firstSet, handles, {});
}

void RenderCommandList::BindDescriptorSet(const uint32_t set, const RenderHandle handle)
{
    BindDescriptorSets(set, array_view<const RenderHandle>(&handle, 1), {});
}

void RenderCommandList::BindDescriptorSet(
    const uint32_t set, const RenderHandle handle, const array_view<const uint32_t> dynamicOffsets)
{
    BindDescriptorSets(set, array_view<const RenderHandle>(&handle, 1), dynamicOffsets);
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
                    static_cast<AccelerationStructureGeometryTrianglesData*>(AllocateRenderData(
                        allocator_, sizeof(AccelerationStructureGeometryTrianglesData) * triangles.size()));
                data->trianglesData = trianglesData;
                data->trianglesView = { data->trianglesData, triangles.size() };
                for (size_t idx = 0; idx < triangles.size(); ++idx) {
                    data->trianglesView[idx] = triangles[idx];
                }
            }
            if (!aabbs.empty()) {
                AccelerationStructureGeometryAabbsData* aabbsData =
                    static_cast<AccelerationStructureGeometryAabbsData*>(
                        AllocateRenderData(allocator_, sizeof(AccelerationStructureGeometryAabbsData) * aabbs.size()));
                data->aabbsData = aabbsData;
                data->aabbsView = { data->aabbsData, aabbs.size() };
                for (size_t idx = 0; idx < aabbs.size(); ++idx) {
                    data->aabbsView[idx] = aabbs[idx];
                }
            }
            if (!instances.empty()) {
                AccelerationStructureGeometryInstancesData* instancesData =
                    static_cast<AccelerationStructureGeometryInstancesData*>(AllocateRenderData(
                        allocator_, sizeof(AccelerationStructureGeometryInstancesData) * instances.size()));
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

void RenderCommandList::SetDynamicStateViewport(const ViewportDesc& viewportDesc)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    ValidateViewport(viewportDesc);
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
    ValidateScissor(scissorDesc);
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

void RenderCommandList::ValidatePipeline()
{
    if (!stateData_.validPso) {
        stateData_.validCommandList = false;
        PLUGIN_LOG_E("RenderCommandList: pso not bound");
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
            PLUGIN_LOG_ONCE_E(
                "RenderCommandList::ValidatePipelineLayout", "RenderCommandList: not all needed descriptor sets bound");
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
RENDER_END_NAMESPACE()
