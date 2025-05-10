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

#include "render_post_process_dof_node.h"

#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <core/plugin/intf_class_factory.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

#include "datastore/render_data_store_pod.h"
#include "datastore/render_data_store_post_process.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "postprocesses/render_post_process_blur.h"
#include "render_post_process_dof.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(
    RenderPostProcessDofNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0), MEMBER_PROPERTY(depth, "depth", 0))
DATA_TYPE_METADATA(RenderPostProcessDofNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr string_view DOF_BLUR_SHADER_NAME = "rendershaders://shader/depth_of_field_blur.shader";
constexpr string_view DOF_SHADER_NAME = "rendershaders://shader/depth_of_field.shader";
constexpr int32_t BUFFER_SIZE_IN_BYTES = sizeof(RenderPostProcessDofNode::DofConfig);
constexpr GpuBufferDesc UBO_DESC { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0U };

RenderHandleReference CreateGpuBuffers(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    // Create buffer for the shader data.
    GpuBufferDesc desc = UBO_DESC;
    desc.byteSize = BUFFER_SIZE_IN_BYTES;
    return gpuResourceMgr.Create(handle, desc);
}

void UpdateBuffer(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle& handle,
    const RenderPostProcessDofNode::DofConfig& shaderParameters)
{
    if (void* data = gpuResourceMgr.MapBuffer(handle); data) {
        CloneData(data, sizeof(RenderPostProcessDofNode::DofConfig), &shaderParameters,
            sizeof(RenderPostProcessDofNode::DofConfig));
        gpuResourceMgr.UnmapBuffer(handle);
    }
}
} // namespace

RenderPostProcessDofNode::RenderPostProcessDofNode()
    : inputProperties_(
          &nodeInputsData, array_view(PropertyType::DataType<RenderPostProcessDofNode::NodeInputs>::properties)),
      outputProperties_(
          &nodeOutputsData, array_view(PropertyType::DataType<RenderPostProcessDofNode::NodeOutputs>::properties))

{}

IPropertyHandle* RenderPostProcessDofNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessDofNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

void RenderPostProcessDofNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

IRenderNode::ExecuteFlags RenderPostProcessDofNode::GetExecuteFlags() const
{
    if (effectProperties_.enabled) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessDofNode::Init(
    const IRenderPostProcess::Ptr& postProcess, IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    postProcess_ = postProcess;

    auto* renderClassFactory = renderNodeContextMgr_->GetRenderContext().GetInterface<IClassFactory>();
    if (renderClassFactory) {
        auto CreatePostProcessInterface = [&](const auto uid, auto& pp, auto& ppNode) {
            if (pp = CreateInstance<IRenderPostProcess>(*renderClassFactory, uid); pp) {
                ppNode = CreateInstance<IRenderPostProcessNode>(*renderClassFactory, pp->GetRenderPostProcessNodeUid());
            }
        };

        CreatePostProcessInterface(
            RenderPostProcessBlur::UID, ppRenderBlurInterface_.postProcess, ppRenderBlurInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessBlur::UID, ppRenderNearBlurInterface_.postProcess,
            ppRenderNearBlurInterface_.postProcessNode);
        CreatePostProcessInterface(RenderPostProcessBlur::UID, ppRenderFarBlurInterface_.postProcess,
            ppRenderFarBlurInterface_.postProcessNode);
    }

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    samplers_.nearest =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_NEAREST_CLAMP);
    samplers_.mipLinear =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP);
    gpuBuffer_ = CreateGpuBuffers(gpuResourceMgr, gpuBuffer_);

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    dofShaderData_ = shaderMgr.GetShaderDataByShaderName(DOF_SHADER_NAME);
    dofBlurShaderData_ = shaderMgr.GetShaderDataByShaderName(DOF_BLUR_SHADER_NAME);

    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    RENDER_NS::DescriptorCounts blurDescriptorCounts =
        renderNodeUtil.GetDescriptorCounts(dofBlurShaderData_.pipelineLayoutData);
    descriptorCounts_ = renderNodeUtil.GetDescriptorCounts(dofShaderData_.pipelineLayoutData);
    descriptorCounts_.counts.insert(
        descriptorCounts_.counts.end(), blurDescriptorCounts.counts.begin(), blurDescriptorCounts.counts.end());
    descriptorCounts_.counts.insert(descriptorCounts_.counts.end(),
        ppRenderNearBlurInterface_.postProcessNode->GetRenderDescriptorCounts().counts.begin(),
        ppRenderNearBlurInterface_.postProcessNode->GetRenderDescriptorCounts().counts.end());
    descriptorCounts_.counts.insert(descriptorCounts_.counts.end(),
        ppRenderFarBlurInterface_.postProcessNode->GetRenderDescriptorCounts().counts.begin(),
        ppRenderFarBlurInterface_.postProcessNode->GetRenderDescriptorCounts().counts.end());

    if (ppRenderNearBlurInterface_.postProcessNode) {
        RenderPostProcessBlur& pp = static_cast<RenderPostProcessBlur&>(*ppRenderNearBlurInterface_.postProcess);
        pp.propertiesData.blurShaderType =
            RenderPostProcessBlurNode::BlurShaderType::BLUR_SHADER_TYPE_RGBA_ALPHA_WEIGHT;
        ppRenderNearBlurInterface_.postProcessNode->Init(
            ppRenderNearBlurInterface_.postProcess, *renderNodeContextMgr_);
    }
    if (ppRenderFarBlurInterface_.postProcessNode) {
        RenderPostProcessBlur& pp = static_cast<RenderPostProcessBlur&>(*ppRenderFarBlurInterface_.postProcess);
        pp.propertiesData.blurShaderType =
            RenderPostProcessBlurNode::BlurShaderType::BLUR_SHADER_TYPE_RGBA_ALPHA_WEIGHT;
        ppRenderFarBlurInterface_.postProcessNode->Init(ppRenderFarBlurInterface_.postProcess, *renderNodeContextMgr_);
    }

    valid_ = true;
}

void RenderPostProcessDofNode::PreExecute()
{
    if (valid_ && postProcess_ &&
        (ppRenderNearBlurInterface_.postProcessNode && ppRenderFarBlurInterface_.postProcessNode)) {
        const array_view<const uint8_t> propertyView = postProcess_->GetData();
        // this node is directly dependant
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessDofNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessDofNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessDofNode::EffectProperties&)(*propertyView.data());
        }
    } else {
        effectProperties_.enabled = false;
    }

    if (ppRenderNearBlurInterface_.postProcessNode && ppRenderNearBlurInterface_.postProcess) {
        // copy properties to new property post process
        RenderPostProcessBlur& pp = static_cast<RenderPostProcessBlur&>(*ppRenderNearBlurInterface_.postProcess);
        pp.propertiesData.enabled = true;

        RenderPostProcessBlurNode& ppNode =
            static_cast<RenderPostProcessBlurNode&>(*ppRenderNearBlurInterface_.postProcessNode);
        ppNode.nodeInputsData.input = effectProperties_.nearMip;
        ppNode.PreExecute();
    }
    if (ppRenderFarBlurInterface_.postProcessNode && ppRenderFarBlurInterface_.postProcess) {
        // copy properties to new property post process
        RenderPostProcessBlur& pp = static_cast<RenderPostProcessBlur&>(*ppRenderFarBlurInterface_.postProcess);
        pp.propertiesData.enabled = true;

        RenderPostProcessBlurNode& ppNode =
            static_cast<RenderPostProcessBlurNode&>(*ppRenderFarBlurInterface_.postProcessNode);
        ppNode.nodeInputsData.input = effectProperties_.farMip;
        ppNode.PreExecute();
    }
}

RenderPass RenderPostProcessDofNode::CreateRenderPass(const RenderHandle input)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(input);
    RenderPass rp;
    rp.renderPassDesc.attachmentCount = 1u;
    rp.renderPassDesc.attachmentHandles[0u] = input;
    rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    rp.renderPassDesc.renderArea = { 0, 0, desc.width, desc.height };

    rp.renderPassDesc.subpassCount = 1u;
    rp.subpassDesc.colorAttachmentCount = 1u;
    rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
    rp.subpassStartIndex = 0u;
    return rp;
}

BASE_NS::Math::Vec4 RenderPostProcessDofNode::GetFactorDof() const
{
    const float focusStart =
        (effectProperties_.dofConfiguration.focusPoint - (effectProperties_.dofConfiguration.focusRange / 2));
    const float focusEnd =
        (effectProperties_.dofConfiguration.focusPoint + (effectProperties_.dofConfiguration.focusRange / 2));
    const float nearTransitionStart = (focusStart - effectProperties_.dofConfiguration.nearTransitionRange);
    const float farTransitionEnd = (focusEnd + effectProperties_.dofConfiguration.farTransitionRange);

    return { nearTransitionStart, focusStart, focusEnd, farTransitionEnd };
}

BASE_NS::Math::Vec4 RenderPostProcessDofNode::GetFactorDof2() const
{
    return { effectProperties_.dofConfiguration.nearBlur, effectProperties_.dofConfiguration.farBlur,
        effectProperties_.dofConfiguration.nearPlane, effectProperties_.dofConfiguration.farPlane };
}

void RenderPostProcessDofNode::Execute(IRenderCommandList& cmdList)
{
    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderDoF", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    CheckDescriptorSetNeed();

    // NOTE: updates set 0 for dof
    ExecuteDofBlur(cmdList);

    auto renderPass = CreateRenderPass(nodeOutputsData.output.handle);
    if (!RenderHandleUtil::IsValid(psos_.dofPso)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(dofShaderData_.shader);
        psos_.dofPso = psoMgr.GetGraphicsPsoHandle(dofShaderData_.shader, gfxHandle, dofShaderData_.pipelineLayout, {},
            {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.dofPso);

    {
        auto& binder = *binders_.dofBinder;
        binder.ClearBindings();
        uint32_t binding = 0u;
        BindableImage input = nodeInputsData.input;
        input.samplerHandle = samplers_.mipLinear;
        binder.BindImage(binding, input);
        binder.BindImage(++binding, effectProperties_.mipImages[0U].GetHandle(), samplers_.mipLinear);
        binder.BindImage(++binding, effectProperties_.mipImages[1U].GetHandle(), samplers_.mipLinear);
        BindableImage depth = nodeInputsData.depth;
        depth.samplerHandle = samplers_.nearest;
        binder.BindImage(++binding, depth);
        binder.BindBuffer(++binding, gpuBuffer_.GetHandle(), 0U);

        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0u, binder.GetDescriptorSetHandle());
    }

    if (dofShaderData_.pipelineLayoutData.pushConstant.byteSize > 0U) {
        const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
        cmdList.PushConstantData(dofShaderData_.pipelineLayoutData.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessDofNode::ExecuteDofBlur(IRenderCommandList& cmdList)
{
    RenderPass rp;
    {
        const GpuImageDesc desc = renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(
            effectProperties_.mipImages[0U].GetHandle());
        rp.renderPassDesc.attachmentCount = 2u;
        rp.renderPassDesc.attachmentHandles[0u] = effectProperties_.mipImages[0U].GetHandle();
        rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
        rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        rp.renderPassDesc.attachmentHandles[1u] = effectProperties_.mipImages[1U].GetHandle();
        rp.renderPassDesc.attachments[1u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
        rp.renderPassDesc.attachments[1u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        rp.renderPassDesc.renderArea = { 0, 0, desc.width, desc.height };

        rp.renderPassDesc.subpassCount = 1u;
        rp.subpassDesc.colorAttachmentCount = 2u;
        rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
        rp.subpassDesc.colorAttachmentIndices[1u] = 1u;
        rp.subpassStartIndex = 0u;
    }
    if (!RenderHandleUtil::IsValid(psos_.dofBlurPso)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(dofBlurShaderData_.shader);
        psos_.dofBlurPso = psoMgr.GetGraphicsPsoHandle(dofBlurShaderData_.shader, gfxHandle,
            dofBlurShaderData_.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }
    cmdList.BeginRenderPass(rp.renderPassDesc, rp.subpassStartIndex, rp.subpassDesc);
    cmdList.BindPipeline(psos_.dofBlurPso);

    shaderParameters_.factors0 = GetFactorDof();
    shaderParameters_.factors1 = GetFactorDof2();

    UpdateBuffer(renderNodeContextMgr_->GetGpuResourceManager(), gpuBuffer_.GetHandle(), shaderParameters_);

    {
        auto& binder = *binders_.dofBlurBinder;
        binder.ClearBindings();
        uint32_t binding = 0u;
        BindableImage input = nodeInputsData.input;
        input.samplerHandle = samplers_.mipLinear;
        binder.BindImage(binding, input);
        BindableImage depth = nodeInputsData.depth;
        depth.samplerHandle = samplers_.nearest;
        binder.BindImage(++binding, depth);
        binder.BindBuffer(++binding, gpuBuffer_.GetHandle(), 0u);

        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder.GetDescriptorSetHandle());
    }

    {
        const float fWidth = static_cast<float>(rp.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(rp.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
        cmdList.PushConstantData(dofBlurShaderData_.pipelineLayoutData.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(rp));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(rp));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
    if (ppRenderNearBlurInterface_.postProcessNode && ppRenderNearBlurInterface_.postProcess) {
        RenderPostProcessBlur& pp = static_cast<RenderPostProcessBlur&>(*ppRenderNearBlurInterface_.postProcess);
        pp.propertiesData.blurConfiguration.maxMipLevel =
            static_cast<uint32_t>(Math::round(effectProperties_.dofConfiguration.nearBlur));
        ppRenderNearBlurInterface_.postProcessNode->Execute(cmdList);
    }
    if (ppRenderFarBlurInterface_.postProcessNode && ppRenderFarBlurInterface_.postProcess) {
        RenderPostProcessBlur& pp = static_cast<RenderPostProcessBlur&>(*ppRenderFarBlurInterface_.postProcess);
        pp.propertiesData.blurConfiguration.maxMipLevel =
            static_cast<uint32_t>(Math::round(effectProperties_.dofConfiguration.farBlur));
        ppRenderFarBlurInterface_.postProcessNode->Execute(cmdList);
    }
}

void RenderPostProcessDofNode::CheckDescriptorSetNeed()
{
    if (!binders_.dofBinder) {
        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

        binders_.dofBlurBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, dofBlurShaderData_.pipelineLayoutData),
            dofBlurShaderData_.pipelineLayoutData.descriptorSetLayouts[0].bindings);
        binders_.dofBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, dofShaderData_.pipelineLayoutData),
            dofShaderData_.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }
}

DescriptorCounts RenderPostProcessDofNode::GetRenderDescriptorCounts() const
{
    return descriptorCounts_;
}

RENDER_END_NAMESPACE()
