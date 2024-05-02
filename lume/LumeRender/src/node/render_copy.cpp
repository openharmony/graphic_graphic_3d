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

#include "render_copy.h"

#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
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

RenderHandle CreatePso(
    IRenderNodeContextManager& renderNodeContextMgr, const RenderHandle& shader, const PipelineLayout& pipelineLayout)
{
    auto& psoMgr = renderNodeContextMgr.GetPsoManager();
    const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shader);
    return psoMgr.GetGraphicsPsoHandle(
        shader, graphicsStateHandle, pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
}
} // namespace

void RenderCopy::Init(IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo)
{
    copyInfo_ = copyInfo;
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr.GetShaderManager();
    {
        renderData_ = {};
        renderData_.shader = shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_copy.shader");
        renderData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayout(renderData_.shader);
        renderData_.sampler = renderNodeContextMgr.GetGpuResourceManager().GetSamplerHandle(
            DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);

        renderData_.shaderLayer = shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_copy_layer.shader");
        renderData_.pipelineLayoutLayer = shaderMgr.GetReflectionPipelineLayout(renderData_.shaderLayer);
    }
    {
        // single binder for both
        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
        const auto& bindings = renderData_.pipelineLayout.descriptorSetLayouts[0U].bindings;
        const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
        binder_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
    }
}

void RenderCopy::PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const CopyInfo& copyInfo)
{
    copyInfo_ = copyInfo;
}

void RenderCopy::Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList)
{
    // extra blit from input to ouput
    if (RenderHandleUtil::IsGpuImage(copyInfo_.input.handle) && RenderHandleUtil::IsGpuImage(copyInfo_.output.handle) &&
        binder_) {
        auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();

        auto renderPass = CreateRenderPass(gpuResourceMgr, copyInfo_.output.handle);
        RenderHandle pso;
        if (copyInfo_.copyType == CopyType::LAYER_COPY) {
            if (!RenderHandleUtil::IsValid(renderData_.psoLayer)) {
                renderData_.psoLayer =
                    CreatePso(renderNodeContextMgr, renderData_.shaderLayer, renderData_.pipelineLayoutLayer);
            }
            pso = renderData_.psoLayer;
        } else {
            if (!RenderHandleUtil::IsValid(renderData_.pso)) {
                renderData_.pso = CreatePso(renderNodeContextMgr, renderData_.shader, renderData_.pipelineLayout);
            }
            pso = renderData_.pso;
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(pso);

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

        const auto& pl =
            (copyInfo_.copyType == CopyType::LAYER_COPY) ? renderData_.pipelineLayoutLayer : renderData_.pipelineLayout;
        if (pl.pushConstant.byteSize > 0) {
            const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
            const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
            const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                { static_cast<float>(copyInfo_.input.layer), 0.0f, 0.0f, 0.0f } };
            cmdList.PushConstantData(pl.pushConstant, arrayviewU8(pc));
        }

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }
}

DescriptorCounts RenderCopy::GetDescriptorCounts() const
{
    // prepare only for a single copy operation per frame
    return DescriptorCounts { {
        { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1U },
        { CORE_DESCRIPTOR_TYPE_SAMPLER, 1U },
    } };
}
RENDER_END_NAMESPACE()
