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

#include "render_copy_helper.h"

#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>

using namespace BASE_NS;
using namespace RENDER_NS;

CORE3D_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

RenderPass CreateRenderPass(const IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle input)
{
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
} // namespace

void RenderCopyHelper::Init(IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo)
{
    copyInfo_ = copyInfo;
    {
        const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr.GetShaderManager();
        renderData_ = {};
        renderData_.shader = shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_copy.shader");
        renderData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayout(renderData_.shader);
        renderData_.sampler =
            renderNodeContextMgr.GetGpuResourceManager().GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    }
    {
        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
        const auto& bindings = renderData_.pipelineLayout.descriptorSetLayouts[0U].bindings;
        const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
        binder_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
    }
}

void RenderCopyHelper::PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo)
{
    copyInfo_ = copyInfo;
}

void RenderCopyHelper::Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList)
{
    // extra copy from input to ouput
    if ((RenderHandleUtil::GetHandleType(copyInfo_.input) == RenderHandleType::GPU_IMAGE) &&
        (RenderHandleUtil::GetHandleType(copyInfo_.output) == RenderHandleType::GPU_IMAGE) && binder_) {
        auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
        auto renderPass = CreateRenderPass(gpuResourceMgr, copyInfo_.output);
        if (!RenderHandleUtil::IsValid(renderData_.pso)) {
            auto& psoMgr = renderNodeContextMgr.GetPsoManager();
            const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
            const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(renderData_.shader);
            renderData_.pso = psoMgr.GetGraphicsPsoHandle(renderData_.shader, graphicsStateHandle,
                renderData_.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(renderData_.pso);

        {
            auto& binder = *binder_;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindSampler(
                binding, RenderHandleUtil::IsValid(copyInfo_.sampler) ? copyInfo_.sampler : renderData_.sampler);
            binder.BindImage(++binding, copyInfo_.input);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0u, binder.GetDescriptorSetHandle());
        }

        // dynamic state
        const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr.GetRenderNodeUtil();
        const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }
}

DescriptorCounts RenderCopyHelper::GetDescriptorCounts() const
{
    // expected high max mip count
    return DescriptorCounts { {
        { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U },
        { CORE_DESCRIPTOR_TYPE_SAMPLER, 1U },
    } };
}
CORE3D_END_NAMESPACE()
