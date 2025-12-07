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

#include "render_post_process_cmaa2_node.h"

#include <base/containers/unique_ptr.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()

DECLARE_PROPERTY_TYPE(RenderPostProcessCmaa2Node::EffectProperties::Quality);

ENUM_TYPE_METADATA(RenderPostProcessCmaa2Node::EffectProperties::Quality, ENUM_VALUE(LOW, "Low Quality"),
    ENUM_VALUE(MEDIUM, "Medium Quality"), ENUM_VALUE(HIGH, "High Quality"), ENUM_VALUE(ULTRA, "Ultra Quality"))

DATA_TYPE_METADATA(RenderPostProcessCmaa2Node::EffectProperties, MEMBER_PROPERTY(enabled, "enabled", 0),
    MEMBER_PROPERTY(quality, "Cmaa2 Quality", 0))
DATA_TYPE_METADATA(RenderPostProcessCmaa2Node::NodeInputs, MEMBER_PROPERTY(input, "input", 0))
DATA_TYPE_METADATA(RenderPostProcessCmaa2Node::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr string_view EDGE_DETECTION_SHADER_NAME { "rendershaders://shader/cmaa2/cmaa2_edge_detect.shader" };
constexpr string_view PROCESS_CANDIDATES_SHADER_NAME { "rendershaders://shader/cmaa2/cmaa2_process_candidates.shader" };
constexpr string_view DEFERRED_APPLY_SHADER_NAME { "rendershaders://shader/cmaa2/cmaa2_blend_apply.shader" };
constexpr string_view CLEAR_NAME { "rendershaders://shader/cmaa2/cmaa2_clear.shader" };
constexpr string_view BLIT_NAME { "rendershaders://shader/cmaa2/cmaa2_blit.shader" };
constexpr string_view IARGS_NAME { "rendershaders://shader/cmaa2/cmaa2_indirect_args.shader" };

constexpr uint32_t CMAA2_CS_INPUT_KERNEL_SIZE_X = 16u;
constexpr uint32_t CMAA2_CS_INPUT_KERNEL_SIZE_Y = 16u;
constexpr uint32_t CMAA2_CS_OUTPUT_KERNEL_SIZE_X = CMAA2_CS_INPUT_KERNEL_SIZE_X - 2u;
constexpr uint32_t CMAA2_CS_OUTPUT_KERNEL_SIZE_Y = CMAA2_CS_INPUT_KERNEL_SIZE_Y - 2u;

constexpr uint32_t UNIFORM_BUFFER_SIZE = 4u * sizeof(float);

constexpr GpuBufferDesc UNIFORM_BUFFER_DESC { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0u };

constexpr GpuBufferDesc STORAGE_BUFFER_DESC { CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS, 0u };

constexpr GpuBufferDesc INDIRECT_BUFFER_DESC { CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                                                   CORE_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS, 0u };

RenderHandleReference CreateUniformBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    GpuBufferDesc desc = UNIFORM_BUFFER_DESC;
    desc.byteSize = UNIFORM_BUFFER_SIZE;
    return gpuResourceMgr.Create(handle, desc);
}

RenderHandleReference CreateStorageBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle, uint32_t sizeInBytes)
{
    GpuBufferDesc desc = STORAGE_BUFFER_DESC;
    desc.byteSize = sizeInBytes;
    return gpuResourceMgr.Create(handle, desc);
}

void UpdateUniformBuffer(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle& handle,
    const RenderPostProcessCmaa2Node::ShaderParameters& shaderParameters)
{
    if (void* data = gpuResourceMgr.MapBuffer(handle); data) {
        CloneData(data, sizeof(RenderPostProcessCmaa2Node::ShaderParameters), &shaderParameters,
            sizeof(RenderPostProcessCmaa2Node::ShaderParameters));
        gpuResourceMgr.UnmapBuffer(handle);
    }
}

uint32_t DivideRoundUp(uint32_t value, uint32_t divisor)
{
    return (value + divisor - 1u) / divisor;
}

} // namespace

RenderPostProcessCmaa2Node::RenderPostProcessCmaa2Node()
    : properties_(&propertiesData_, PropertyType::DataType<EffectProperties>::MetaDataFromType()),
      inputProperties_(&nodeInputsData_, PropertyType::DataType<NodeInputs>::MetaDataFromType()),
      outputProperties_(&nodeOutputsData_, PropertyType::DataType<NodeOutputs>::MetaDataFromType())
{}

RenderPostProcessCmaa2Node::~RenderPostProcessCmaa2Node() = default;

IPropertyHandle* RenderPostProcessCmaa2Node::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessCmaa2Node::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

DescriptorCounts RenderPostProcessCmaa2Node::GetRenderDescriptorCounts() const
{
    return descriptorCounts_;
}

void RenderPostProcessCmaa2Node::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

void RenderPostProcessCmaa2Node::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    // clear
    psos_ = {};
    shaderData_ = {};
    binders_ = {};

    renderNodeContextMgr_ = &renderNodeContextMgr;

    // default inputs
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    defaultInput_.handle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    defaultInput_.samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    defaultSampler_.samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");

    uniformBuffer_ = CreateUniformBuffer(gpuResourceMgr, uniformBuffer_);

    // load shaders
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    shaderData_.clearPass = shaderMgr.GetShaderDataByShaderHandle(shaderMgr.GetShaderHandle(CLEAR_NAME));
    shaderData_.edgeDetectionPass =
        shaderMgr.GetShaderDataByShaderHandle(shaderMgr.GetShaderHandle(EDGE_DETECTION_SHADER_NAME));
    shaderData_.processCandidatesPass =
        shaderMgr.GetShaderDataByShaderHandle(shaderMgr.GetShaderHandle(PROCESS_CANDIDATES_SHADER_NAME));
    shaderData_.deferredApplyPass =
        shaderMgr.GetShaderDataByShaderHandle(shaderMgr.GetShaderHandle(DEFERRED_APPLY_SHADER_NAME));
    shaderData_.iargs = shaderMgr.GetShaderDataByShaderHandle(shaderMgr.GetShaderHandle(IARGS_NAME));

    shaderData_.blit = shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(BLIT_NAME));

    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();

    const DescriptorCounts clearCounts = renderNodeUtil.GetDescriptorCounts(shaderData_.clearPass.pipelineLayoutData);
    const DescriptorCounts iargsCounts = renderNodeUtil.GetDescriptorCounts(shaderData_.iargs.pipelineLayoutData);
    const DescriptorCounts blitCounts = renderNodeUtil.GetDescriptorCounts(shaderData_.blit.pipelineLayoutData);
    const DescriptorCounts edgeDetectionCounts =
        renderNodeUtil.GetDescriptorCounts(shaderData_.edgeDetectionPass.pipelineLayoutData);
    const DescriptorCounts processCandidatesCounts =
        renderNodeUtil.GetDescriptorCounts(shaderData_.processCandidatesPass.pipelineLayoutData);
    const DescriptorCounts deferredApplyCounts =
        renderNodeUtil.GetDescriptorCounts(shaderData_.deferredApplyPass.pipelineLayoutData);

    descriptorCounts_.counts.clear();
    descriptorCounts_.counts.reserve(clearCounts.counts.size() + edgeDetectionCounts.counts.size() +
                                     processCandidatesCounts.counts.size() + deferredApplyCounts.counts.size() +
                                     blitCounts.counts.size() + iargsCounts.counts.size());
    descriptorCounts_.counts.append(clearCounts.counts.cbegin(), clearCounts.counts.cend());
    descriptorCounts_.counts.append(iargsCounts.counts.cbegin(), iargsCounts.counts.cend());
    descriptorCounts_.counts.append(edgeDetectionCounts.counts.cbegin(), edgeDetectionCounts.counts.cend());
    descriptorCounts_.counts.append(processCandidatesCounts.counts.cbegin(), processCandidatesCounts.counts.cend());
    descriptorCounts_.counts.append(deferredApplyCounts.counts.cbegin(), deferredApplyCounts.counts.cend());
    descriptorCounts_.counts.append(blitCounts.counts.cbegin(), blitCounts.counts.cend());

    valid_ = true;
}

void RenderPostProcessCmaa2Node::PreExecuteFrame()
{
    if (!valid_) {
        return;
    }
    enabled_ = propertiesData_.enabled;

    if (enabled_) {
        EvaluateOutputImageCreation();

        if (RenderHandleUtil::IsValid(nodeInputsData_.input.handle)) {
            const GpuImageDesc& imgDesc =
                renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(nodeInputsData_.input.handle);
            const Math::UVec2 size(imgDesc.width, imgDesc.height);
            CreateTargets(size);
            CreateBuffers(size);
        }
    }
}

IRenderNode::ExecuteFlags RenderPostProcessCmaa2Node::GetExecuteFlags() const
{
    if (enabled_ && valid_) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessCmaa2Node::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }

    CreatePsos();
    CheckDescriptorSetNeed();

    if (!RenderHandleUtil::IsValid(currOutput_.handle)) {
        return;
    }

    currInput_ = RenderHandleUtil::IsValid(nodeInputsData_.input.handle) ? nodeInputsData_.input : defaultInput_;
    if (!RenderHandleUtil::IsValid(currInput_.samplerHandle)) {
        currInput_.samplerHandle = defaultInput_.samplerHandle;
    }

    currOutput_.handle = RenderHandleUtil::IsValid(nodeOutputsData_.output.handle)
                             ? nodeOutputsData_.output.handle
                             : ownOutputImageData_.handle.GetHandle();

    // update shader parameters
    shaderParameters_.resX = static_cast<float>(targets_.resolution.x);
    shaderParameters_.resY = static_cast<float>(targets_.resolution.y);

    switch (propertiesData_.quality) {
        case EffectProperties::Quality::ULTRA:
            shaderParameters_.edgeThreshold = EDGE_THRESHOLD_ULTRA;
            break;

        case EffectProperties::Quality::HIGH:
            shaderParameters_.edgeThreshold = EDGE_THRESHOLD_HIGH;
            break;

        case EffectProperties::Quality::MEDIUM:
            shaderParameters_.edgeThreshold = EDGE_THRESHOLD_MEDIUM;
            break;

        case EffectProperties::Quality::LOW:
            shaderParameters_.edgeThreshold = EDGE_THRESHOLD_LOW;
            break;

        default:
            break;
    }
    shaderParameters_.padding = 0.0f;

    UpdateUniformBuffer(renderNodeContextMgr_->GetGpuResourceManager(), uniformBuffer_.GetHandle(), shaderParameters_);

    ClearControlBuffer(cmdList);
    ClearImages(cmdList);

    RenderEdgeDetectionPass(cmdList);
    UpdateIndirectArgs(cmdList);

    RenderProcessCandidatesPass(cmdList);
    UpdateIndirectArgs(cmdList);

    RenderDeferredApplyPass(cmdList);
    RenderBlit(cmdList);
}

void RenderPostProcessCmaa2Node::ClearControlBuffer(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(computeBuffers_.controlBuffer.GetHandle())) {
        return;
    }

    auto& binder = binders_.clearPass;
    if (!binder) {
        return;
    }

    cmdList.BindPipeline(psos_.clearPass);

    binder->ClearBindings();
    binder->BindBuffer(0u, computeBuffers_.controlBuffer.GetHandle(), 0u);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    cmdList.Dispatch(1u, 1u, 1u);
}

void RenderPostProcessCmaa2Node::ClearImages(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(targets_.blendItemHeads.GetHandle())) {
        return;
    }

    constexpr ImageSubresourceRange imgSubresourceRange { ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

    // clear blend heads
    {
        static constexpr auto clearColor =
            RENDER_NS::ClearColorValue({ 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu, 0xFFFFFFFFu });
        cmdList.ClearColorImage(targets_.blendItemHeads.GetHandle(), clearColor, { &imgSubresourceRange, 1 });
    }

    // clear render target
    {
        static constexpr auto clearColor = RENDER_NS::ClearColorValue({ 0.0f, 0.0f, 0.0f, 0.0f });
        cmdList.ClearColorImage(targets_.outComp.GetHandle(), clearColor, { &imgSubresourceRange, 1 });
    }
}

void RenderPostProcessCmaa2Node::RenderEdgeDetectionPass(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(currInput_.handle)) {
        CORE_LOG_E("RenderEdgeDetectionPass: input invalid!");
        return;
    }

    auto& binder = binders_.edgeDetectionPass;
    if (!binder) {
        return;
    }

    cmdList.BindPipeline(psos_.edgeDetectionPass);
    binder->ClearBindings();

    binder->BindBuffer(0u, uniformBuffer_.GetHandle(), 0u);
    binder->BindImage(1u, currInput_);
    binder->BindImage(2u, BindableImage { targets_.edgeImage.GetHandle(), {}, ImageLayout::CORE_IMAGE_LAYOUT_GENERAL });
    binder->BindBuffer(3u, computeBuffers_.shapeCandidatesBuffer.GetHandle(), 0u);
    binder->BindBuffer(4u, computeBuffers_.controlBuffer.GetHandle(), 0u);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    const uint32_t groupsX = DivideRoundUp(DivideRoundUp(targets_.resolution.x, 2u), CMAA2_CS_OUTPUT_KERNEL_SIZE_X);
    const uint32_t groupsY = DivideRoundUp(DivideRoundUp(targets_.resolution.y, 2u), CMAA2_CS_OUTPUT_KERNEL_SIZE_Y);
    cmdList.Dispatch(groupsX, groupsY, 1u);
}

void RenderPostProcessCmaa2Node::RenderProcessCandidatesPass(IRenderCommandList& cmdList)
{
    auto& binder = binders_.processCandidatesPass;
    if (!binder) {
        return;
    }

    cmdList.BindPipeline(psos_.processCandidatesPass);
    binder->ClearBindings();

    binder->BindImage(0u, currInput_);
    binder->BindImage(1u, BindableImage { targets_.edgeImage.GetHandle(), {}, ImageLayout::CORE_IMAGE_LAYOUT_GENERAL });
    binder->BindBuffer(2u, computeBuffers_.shapeCandidatesBuffer.GetHandle(), 0u);
    binder->BindBuffer(3u, computeBuffers_.controlBuffer.GetHandle(), 0u);
    binder->BindBuffer(4u, computeBuffers_.blendLocationList.GetHandle(), 0u);
    binder->BindBuffer(5u, computeBuffers_.blendItemList.GetHandle(), 0u);
    binder->BindImage(
        6u, BindableImage { targets_.blendItemHeads.GetHandle(), {}, ImageLayout::CORE_IMAGE_LAYOUT_GENERAL });

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    cmdList.DispatchIndirect(dispatchIndirectHandle_.GetHandle(), 0);
}

void RenderPostProcessCmaa2Node::RenderDeferredApplyPass(IRenderCommandList& cmdList)
{
    auto& binder = binders_.deferredApplyPass;
    if (!binder) {
        return;
    }

    cmdList.BindPipeline(psos_.deferredApplyPass);
    binder->ClearBindings();

    BindableImage outBindable;
    outBindable.handle = targets_.outComp.GetHandle();

    binder->BindImage(0u, outBindable);
    binder->BindBuffer(1u, computeBuffers_.controlBuffer.GetHandle(), 0u);
    binder->BindBuffer(2u, computeBuffers_.blendLocationList.GetHandle(), 0u);
    binder->BindBuffer(3u, computeBuffers_.blendItemList.GetHandle(), 0u);
    binder->BindImage(4u, BindableImage { targets_.blendItemHeads.GetHandle() });

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    cmdList.DispatchIndirect(dispatchIndirectHandle_.GetHandle(), sizeof(uint32_t) * 16u);
}

void RenderPostProcessCmaa2Node::RenderBlit(RENDER_NS::IRenderCommandList& cmdList)
{
    auto& binder = binders_.blit;
    if (!binder) {
        return;
    }

    const auto renderPass = CreateRenderPass(currOutput_.handle, targets_.resolution);
    binder->ClearBindings();

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.blit);

    BindableImage cmaaTarget;
    cmaaTarget.handle = targets_.outComp.GetHandle();
    cmaaTarget.samplerHandle = defaultSampler_.samplerHandle;
    binder->BindImage(0u, cmaaTarget);
    binder->BindImage(1u, currInput_);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessCmaa2Node::UpdateIndirectArgs(RENDER_NS::IRenderCommandList& cmdList)
{
    auto& binder = binders_.iargs;
    if (!binder) {
        return;
    }

    cmdList.BindPipeline(psos_.iargs);
    binder->ClearBindings();

    binder->BindBuffer(0u, computeBuffers_.controlBuffer.GetHandle(), 0u);
    binder->BindBuffer(1u, dispatchIndirectHandle_.GetHandle(), 0u);
    binder->BindBuffer(2u, dispatchIndirectHandle_.GetHandle(), sizeof(uint32_t) * 16u);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    cmdList.Dispatch(1, 1, 1);
}

RenderPass RenderPostProcessCmaa2Node::CreateRenderPass(
    const RenderHandle output, const BASE_NS::Math::UVec2& resolution) const
{
    RenderPass rp;
    rp.renderPassDesc.attachmentCount = 1u;
    rp.renderPassDesc.attachmentHandles[0u] = output;
    rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD;
    rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    rp.renderPassDesc.renderArea = { 0, 0, resolution.x, resolution.y };

    rp.renderPassDesc.subpassCount = 1u;
    rp.subpassDesc.colorAttachmentCount = 1u;
    rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
    rp.subpassStartIndex = 0u;
    return rp;
}

void RenderPostProcessCmaa2Node::CreateTargets(const BASE_NS::Math::UVec2 baseSize)
{
    if (baseSize.x != baseSize_.x || baseSize.y != baseSize_.y) {
        baseSize_ = baseSize;
        targets_.resolution = baseSize;
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

        GpuImageDesc desc {
            ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R8_UINT,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS,
            targets_.resolution.x / 2u,
            targets_.resolution.y,
            1u,
            1u,
            1u,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };
        targets_.edgeImage = gpuResourceMgr.Create(targets_.edgeImage, desc);

        desc.format = Format::BASE_FORMAT_R32_UINT;
        desc.usageFlags =
            CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_DST_BIT;
        desc.height = targets_.resolution.y / 2u;
        targets_.blendItemHeads = gpuResourceMgr.Create(targets_.blendItemHeads, desc);

        desc.format = Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32;
        desc.width = targets_.resolution.x;
        desc.height = targets_.resolution.y;
        targets_.outComp = gpuResourceMgr.Create(targets_.outComp, desc);
    }
}

void RenderPostProcessCmaa2Node::CreateBuffers(const BASE_NS::Math::UVec2 baseSize)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

    const uint32_t pixelCount = baseSize.x * baseSize.y;

    const uint32_t controlBufferSize = sizeof(ControlBufferData);
    computeBuffers_.controlBuffer =
        CreateStorageBuffer(gpuResourceMgr, computeBuffers_.controlBuffer, controlBufferSize);

    const uint32_t candidatesSize = (pixelCount / 16u) * sizeof(uint32_t);
    computeBuffers_.shapeCandidatesBuffer =
        CreateStorageBuffer(gpuResourceMgr, computeBuffers_.shapeCandidatesBuffer, candidatesSize);

    const uint32_t blendLocationSize = (pixelCount / 4u) * sizeof(uint32_t);
    computeBuffers_.blendLocationList =
        CreateStorageBuffer(gpuResourceMgr, computeBuffers_.blendLocationList, blendLocationSize);

    const uint32_t blendItemSize = pixelCount * 2u * sizeof(uint32_t);
    computeBuffers_.blendItemList = CreateStorageBuffer(gpuResourceMgr, computeBuffers_.blendItemList, blendItemSize);

    {
        GpuBufferDesc desc = INDIRECT_BUFFER_DESC;
        desc.byteSize = sizeof(uint32_t) * 16u * 2u; // 16 is the minimum buffer offset

        dispatchIndirectHandle_ = gpuResourceMgr.Create(dispatchIndirectHandle_, desc);
    }
}

void RenderPostProcessCmaa2Node::CreatePsos()
{
    if (RenderHandleUtil::IsValid(psos_.edgeDetectionPass) && RenderHandleUtil::IsValid(psos_.processCandidatesPass) &&
        RenderHandleUtil::IsValid(psos_.deferredApplyPass)) {
        return;
    }

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    INodeContextPsoManager& psoMgr = renderNodeContextMgr_->GetPsoManager();

    if (!RenderHandleUtil::IsValid(psos_.edgeDetectionPass)) {
        psos_.edgeDetectionPass = psoMgr.GetComputePsoHandle(
            shaderData_.edgeDetectionPass.shader, shaderData_.edgeDetectionPass.pipelineLayout, {});
    }

    if (!RenderHandleUtil::IsValid(psos_.clearPass)) {
        psos_.clearPass =
            psoMgr.GetComputePsoHandle(shaderData_.clearPass.shader, shaderData_.clearPass.pipelineLayout, {});
    }

    if (!RenderHandleUtil::IsValid(psos_.iargs)) {
        psos_.iargs = psoMgr.GetComputePsoHandle(shaderData_.iargs.shader, shaderData_.iargs.pipelineLayout, {});
    }

    if (!RenderHandleUtil::IsValid(psos_.processCandidatesPass)) {
        psos_.processCandidatesPass = psoMgr.GetComputePsoHandle(
            shaderData_.processCandidatesPass.shader, shaderData_.processCandidatesPass.pipelineLayout, {});
    }

    if (!RenderHandleUtil::IsValid(psos_.blit)) {
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.blit.shader);
        psos_.blit = psoMgr.GetGraphicsPsoHandle(shaderData_.blit.shader, gfxHandle, shaderData_.blit.pipelineLayout,
            {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    if (!RenderHandleUtil::IsValid(psos_.deferredApplyPass)) {
        psos_.deferredApplyPass = psoMgr.GetComputePsoHandle(
            shaderData_.deferredApplyPass.shader, shaderData_.deferredApplyPass.pipelineLayout, {});
    }
}

void RenderPostProcessCmaa2Node::CheckDescriptorSetNeed()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    if (!binders_.clearPass) {
        binders_.clearPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.clearPass.pipelineLayoutData),
            shaderData_.clearPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.iargs) {
        binders_.iargs = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.iargs.pipelineLayoutData),
            shaderData_.iargs.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.edgeDetectionPass) {
        binders_.edgeDetectionPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.edgeDetectionPass.pipelineLayoutData),
            shaderData_.edgeDetectionPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.processCandidatesPass) {
        binders_.processCandidatesPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.processCandidatesPass.pipelineLayoutData),
            shaderData_.processCandidatesPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.deferredApplyPass) {
        binders_.deferredApplyPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.deferredApplyPass.pipelineLayoutData),
            shaderData_.deferredApplyPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.blit) {
        binders_.blit = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.blit.pipelineLayoutData),
            shaderData_.blit.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }
}

void RenderPostProcessCmaa2Node::EvaluateOutputImageCreation()
{
    if (RenderHandleUtil::IsValid(nodeOutputsData_.output.handle)) {
        currOutput_ = nodeOutputsData_.output;
    } else {
        IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        Math::UVec2 size { 1u, 1u };
        if (useRequestedRenderArea_) {
            const RenderPassDesc::RenderArea& area = renderAreaRequest_.area;
            size = { area.offsetX + area.extentWidth, area.offsetY + area.extentHeight };
        }
        if ((size.x != ownOutputImageData_.width) || (size.y != ownOutputImageData_.height)) {
            GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(currInput_.handle);
            if (desc.format == BASE_FORMAT_UNDEFINED) {
                desc.format = BASE_FORMAT_R8G8B8A8_SRGB;
            }
            desc.width = size.x;
            desc.height = size.y;
            desc.usageFlags |= ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
            desc.engineCreationFlags |=
                EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS |
                EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            ownOutputImageData_.handle = gpuResourceMgr.Create(desc);
            ownOutputImageData_.width = size.x;
            ownOutputImageData_.height = size.y;
        }
    }
}

RENDER_END_NAMESPACE()