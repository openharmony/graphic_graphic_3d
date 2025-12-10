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

#include "render_post_process_flare_node.h"

#include <base/containers/unique_ptr.h>
#include <base/math/matrix_util.h>
#include <core/log.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

#include "default_engine_constants.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RenderPostProcessFlareNode::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0),
    MEMBER_PROPERTY(flarePos, "Flare Position", 0), MEMBER_PROPERTY(intensity, "Effect Intensity", 0))

DATA_TYPE_METADATA(RenderPostProcessFlareNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0))
DATA_TYPE_METADATA(RenderPostProcessFlareNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr string_view SHADER_NAME { "rendershaders://shader/fullscreen_flare.shader" };

RenderPassDesc::RenderArea GetImageRenderArea(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle handle)
{
    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(handle);
    return { 0U, 0U, desc.width, desc.height };
}
} // namespace

RenderPostProcessFlareNode::RenderPostProcessFlareNode()
    : properties_(&propertiesData, PropertyType::DataType<EffectProperties>::MetaDataFromType()),
      inputProperties_(&nodeInputsData, PropertyType::DataType<NodeInputs>::MetaDataFromType()),
      outputProperties_(&nodeOutputsData, PropertyType::DataType<NodeOutputs>::MetaDataFromType())
{}

IPropertyHandle* RenderPostProcessFlareNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessFlareNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

DescriptorCounts RenderPostProcessFlareNode::GetRenderDescriptorCounts() const
{
    return descriptorCounts_;
}

void RenderPostProcessFlareNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

void RenderPostProcessFlareNode::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    // clear
    pipelineData_ = {};

    renderNodeContextMgr_ = &renderNodeContextMgr;

    // default inputs
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    defaultInput_.handle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    defaultInput_.samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");

    // load shaders
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    pipelineData_.gsd = shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(SHADER_NAME));

    // create psos
    INodeContextPsoManager& psoMgr = renderNodeContextMgr_->GetPsoManager();
    pipelineData_.pso = psoMgr.GetGraphicsPsoHandle(pipelineData_.gsd, {}, DYNAMIC_STATES);

    // get needed descriptor set counts
    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    descriptorCounts_ = renderNodeUtil.GetDescriptorCounts(pipelineData_.gsd.pipelineLayoutData);

    valid_ = true;
}

void RenderPostProcessFlareNode::PreExecuteFrame()
{
    if (valid_) {
        const array_view<const uint8_t> propertyView = GetData();
        // this node is directly dependant
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessFlareNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessFlareNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessFlareNode::EffectProperties&)(*propertyView.data());
        }
    } else {
        effectProperties_.enabled = false;
    }

    if (effectProperties_.enabled) {
        // check input and output
        EvaluateOutput();
    }
}

IRenderNode::ExecuteFlags RenderPostProcessFlareNode::GetExecuteFlags() const
{
    if (effectProperties_.enabled) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessFlareNode::ExecuteFrame(IRenderCommandList& cmdList)
{
    CORE_ASSERT(effectProperties_.enabled);

    EvaluateOutput();

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderLensFlare", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    BindableImage currOutput = nodeOutputsData.output;
    if (!RenderHandleUtil::IsValid(currOutput.handle)) {
        return;
    }
    // update the output
    nodeOutputsData.output = currOutput;

    const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const RenderPassDesc::RenderArea renderArea =
        useRequestedRenderArea_ ? renderAreaRequest_.area : GetImageRenderArea(gpuResourceMgr, currOutput.handle);
    if ((renderArea.extentWidth == 0) || (renderArea.extentHeight == 0)) {
        return;
    }

    // render
    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.renderArea = renderArea;
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_LOAD;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachmentHandles[0] = { currOutput.handle };
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;

    cmdList.BeginRenderPass(renderPass.renderPassDesc, 0U, renderPass.subpassDesc);

    // dynamic state
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    cmdList.SetDynamicStateViewport(renderNodeUtil.CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeUtil.CreateDefaultScissor(renderPass));

    // bind pso
    cmdList.BindPipeline(pipelineData_.pso);

    if (pipelineData_.gsd.pipelineLayoutData.pushConstant.byteSize > 0U) {
        const PushConstantStruct pc = GetPushDataStruct(renderArea.extentWidth, renderArea.extentHeight);
        cmdList.PushConstantData(pipelineData_.gsd.pipelineLayoutData.pushConstant, arrayviewU8(pc));
    }

    // draw
    cmdList.Draw(3U, 1U, 0U, 0U);

    cmdList.EndRenderPass();
}

RenderPostProcessFlareNode::PushConstantStruct RenderPostProcessFlareNode::GetPushDataStruct(
    const uint32_t width, const uint32_t height) const
{
    const Math::Vec3 flarePos = effectProperties_.flarePos;
    float intensity = effectProperties_.intensity;
    // NOTE: the shader is currently still run even though the intensity is zero
    // signed culling for z
    if ((flarePos.x < 0.0f) || (flarePos.x > 1.0f) || (flarePos.y < 0.0f) || (flarePos.y > 1.0f) ||
        (flarePos.z < 0.0f)) {
        intensity = 0.0f;
    }

    const float time = renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderTimings.z;
    PushConstantStruct pcV { { time, flarePos.x, flarePos.y, flarePos.z },
        { static_cast<float>(width), static_cast<float>(height), intensity, 0.0f } };

    return pcV;
}

void RenderPostProcessFlareNode::EvaluateOutput()
{
    if (RenderHandleUtil::IsValid(nodeInputsData.input.handle)) {
        nodeOutputsData.output = nodeInputsData.input;
    }
}
RENDER_END_NAMESPACE()
