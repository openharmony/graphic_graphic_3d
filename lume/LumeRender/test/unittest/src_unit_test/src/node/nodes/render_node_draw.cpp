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

#include "node/nodes/render_node_draw.h"

#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {
static constexpr string_view VERTEX_BUFFER_NAME { "VertexBuffer" };
} // namespace

void RenderNodeDraw::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    valid_ = true;

    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    vertexBufferHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetBufferHandle(VERTEX_BUFFER_NAME);

    IShaderManager& shaderMgr = renderNodeContextMgr.GetRenderContext().GetDevice().GetShaderManager();
    auto shaderHandle = shaderMgr.GetShaderHandle("rendershaders://shader/RenderCommandListTest.shader");
    const ShaderSpecializationConstantDataView specialization { {}, {} };
    constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
    const RenderHandle graphicsState =
        renderNodeContextMgr_->GetShaderManager().GetGraphicsStateHandleByShaderHandle(shaderHandle.GetHandle());
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    auto pipelineLayout = renderNodeUtil.CreatePipelineLayout(shaderHandle.GetHandle());
    const auto& inputLayout = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle);
    psoHandle_ = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(shaderHandle.GetHandle(), graphicsState,
        pipelineLayout, inputLayout, specialization, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
}

void RenderNodeDraw::PreExecuteFrame() {}

void RenderNodeDraw::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_ || !RENDER_NS::RenderHandleUtil::IsValid(psoHandle_)) {
        return;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    const RenderPass renderPass = renderNodeUtil.CreateRenderPass(inputRenderPass_);
    const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
    const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

    cmdList.BindPipeline(psoHandle_);

    cmdList.SetDynamicStateViewport(viewportDesc);
    cmdList.SetDynamicStateScissor(scissorDesc);

    VertexBuffer vbo;
    vbo.bufferHandle = vertexBufferHandle_;
    cmdList.BindVertexBuffers({ &vbo, 1 });

    if (count_ == 0) {
        cmdList.Draw(3u, 1u, 0u, 0u);
    } else {
        cmdList.Draw(3u, 2u, 0u, 0u);
    }

    cmdList.EndRenderPass();
    count_++;
}

void RenderNodeDraw::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    inputRenderPassJson_ = parserUtil.GetInputRenderPass(jsonVal, "renderPass");

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(inputRenderPassJson_);
}

// for plugin / factory interface
IRenderNode* RenderNodeDraw::Create()
{
    return new RenderNodeDraw();
}

void RenderNodeDraw::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDraw*>(instance);
}
RENDER_END_NAMESPACE()