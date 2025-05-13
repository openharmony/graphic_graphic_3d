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

#include "render_post_process_taa_node.h"

#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "render_post_process_taa.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RenderPostProcessTaaNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0),
    MEMBER_PROPERTY(depth, "depth", 0), MEMBER_PROPERTY(velocity, "velocity", 0),
    MEMBER_PROPERTY(history, "history", 0))
DATA_TYPE_METADATA(RenderPostProcessTaaNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr string_view TAA_SHADER_NAME = "rendershaders://shader/fullscreen_taa.shader";
} // namespace

RenderPostProcessTaaNode::RenderPostProcessTaaNode()
    : inputProperties_(
          &nodeInputsData, array_view(PropertyType::DataType<RenderPostProcessTaaNode::NodeInputs>::properties)),
      outputProperties_(
          &nodeOutputsData, array_view(PropertyType::DataType<RenderPostProcessTaaNode::NodeOutputs>::properties))

{}

IPropertyHandle* RenderPostProcessTaaNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessTaaNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

void RenderPostProcessTaaNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

IRenderNode::ExecuteFlags RenderPostProcessTaaNode::GetExecuteFlags() const
{
    if (effectProperties_.enabled) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessTaaNode::Init(
    const IRenderPostProcess::Ptr& postProcess, IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    postProcess_ = postProcess;

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    samplerHandle_ =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    shaderData_ = shaderMgr.GetShaderDataByShaderName(TAA_SHADER_NAME);

    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    descriptorCounts_ = renderNodeUtil.GetDescriptorCounts(shaderData_.pipelineLayoutData);

    valid_ = true;
}

void RenderPostProcessTaaNode::PreExecute()
{
    if (valid_ && postProcess_) {
        const array_view<const uint8_t> propertyView = postProcess_->GetData();
        // this node is directly dependant
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessTaaNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessTaaNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessTaaNode::EffectProperties&)(*propertyView.data());
        }
    } else {
        effectProperties_.enabled = false;
    }
}

RenderPass RenderPostProcessTaaNode::CreateRenderPass(const RenderHandle input) const
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

BASE_NS::Math::Vec4 RenderPostProcessTaaNode::GetFactorTaa() const
{
    constexpr float alpha = 0.1f;

    uint32_t varianceClippingBicubic = 0;
    if (effectProperties_.taaConfiguration.useBicubic) {
        varianceClippingBicubic |= (1 << TAA_USE_BICUBIC_BIT);
    }
    if (effectProperties_.taaConfiguration.useVarianceClipping) {
        varianceClippingBicubic |= (1 << TAA_USE_VARIANCE_CLIPPING_BIT);
    }
    if (effectProperties_.taaConfiguration.useyCoCG) {
        varianceClippingBicubic |= (1 << TAA_USE_YCOCG_BIT);
    }
    if (effectProperties_.taaConfiguration.ignoreBicubicEdges) {
        varianceClippingBicubic |= (1 << TAA_IGNORE_EDGES_BIT);
    }

    const float vcb = *(reinterpret_cast<float*>(&varianceClippingBicubic));

    return { static_cast<float>(effectProperties_.taaConfiguration.sharpness),
        static_cast<float>(effectProperties_.taaConfiguration.quality), vcb, alpha };
}

void RenderPostProcessTaaNode::Execute(IRenderCommandList& cmdList)
{
    CheckDescriptorSetNeed();

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderTAA", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    auto renderPass = CreateRenderPass(nodeOutputsData.output.handle);
    if (!RenderHandleUtil::IsValid(pso_)) {
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.shader);
        pso_ = psoMgr.GetGraphicsPsoHandle(shaderData_.shader, gfxHandle, shaderData_.pipelineLayout, {}, {},
            { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(pso_);

    {
        auto& binder = *taaBinder_;
        binder.ClearBindings();
        uint32_t binding = 0u;
        binder.BindImage(binding, nodeInputsData.depth.handle);
        binder.BindImage(++binding, nodeInputsData.input.handle);
        binder.BindImage(++binding, nodeInputsData.velocity.handle);
        binder.BindImage(++binding, nodeInputsData.history.handle);
        binder.BindSampler(++binding, samplerHandle_);

        const RenderHandle set = binder.GetDescriptorSetHandle();
        const DescriptorSetLayoutBindingResources resources = binder.GetDescriptorSetLayoutBindingResources();

        cmdList.UpdateDescriptorSet(set, resources);
        cmdList.BindDescriptorSet(0U, set);
    }

    if (shaderData_.pipelineLayoutData.pushConstant.byteSize > 0U) {
        const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
            GetFactorTaa() };
        cmdList.PushConstantData(shaderData_.pipelineLayoutData.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessTaaNode::CheckDescriptorSetNeed()
{
    if (!taaBinder_) {
        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        taaBinder_ = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.pipelineLayoutData),
            shaderData_.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }
}

DescriptorCounts RenderPostProcessTaaNode::GetRenderDescriptorCounts() const
{
    return descriptorCounts_;
}

RENDER_END_NAMESPACE()
