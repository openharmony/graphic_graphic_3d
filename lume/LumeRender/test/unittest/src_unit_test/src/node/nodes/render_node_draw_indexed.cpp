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

#include "node/nodes/render_node_draw_indexed.h"

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
static constexpr string_view INDEX_BUFFER_32_NAME { "IndexBuffer32" };
static constexpr string_view INDEX_BUFFER_16_NAME { "IndexBuffer16" };
} // namespace

void RenderNodeDrawIndexed::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    valid_ = true;

    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    vertexBufferHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetBufferHandle(VERTEX_BUFFER_NAME);
    indexBuffer32Handle_ = renderNodeContextMgr.GetGpuResourceManager().GetBufferHandle(INDEX_BUFFER_32_NAME);
    indexBuffer16Handle_ = renderNodeContextMgr.GetGpuResourceManager().GetBufferHandle(INDEX_BUFFER_16_NAME);

    IShaderManager& shaderMgr = renderNodeContextMgr.GetRenderContext().GetDevice().GetShaderManager();
    auto shaderHandle = shaderMgr.GetShaderHandle("rendershaders://shader/RenderCommandListTest.shader");
    constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
    const ShaderSpecializationConstantDataView specialization { {}, {} };
    const RenderHandle graphicsState =
        renderNodeContextMgr_->GetShaderManager().GetGraphicsStateHandleByShaderHandle(shaderHandle.GetHandle());
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    auto pipelineLayout = renderNodeUtil.CreatePipelineLayout(shaderHandle.GetHandle());
    const auto& inputLayout = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle);
    psoHandle_ = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(shaderHandle.GetHandle(), graphicsState,
        pipelineLayout, inputLayout, specialization, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
}

void RenderNodeDrawIndexed::PreExecuteFrame() {}

void RenderNodeDrawIndexed::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    const RenderPass renderPass = renderNodeUtil.CreateRenderPass(inputRenderPass_);
    const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
    const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
#if NDEBUG
    if (count_ >= 1) {
        IShaderManager& shaderMgr = renderNodeContextMgr_->GetRenderContext().GetDevice().GetShaderManager();
        auto shaderHandle = shaderMgr.GetShaderHandle("rendershaders://shader/RenderCommandListTest.shader");
        constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT,
            CORE_DYNAMIC_STATE_ENUM_SCISSOR };
        const ShaderSpecializationConstantDataView specialization { {}, {} };
        const RenderHandle graphicsStateHandle =
            renderNodeContextMgr_->GetShaderManager().GetGraphicsStateHandleByShaderHandle(shaderHandle.GetHandle());

        GraphicsState graphicsState = renderNodeContextMgr_->GetShaderManager().GetGraphicsState(graphicsStateHandle);
        if (count_ <= 12) {
            graphicsState.colorBlendState.colorAttachments[0].enableBlend = true;
            graphicsState.colorBlendState.colorAttachments[0].colorWriteMask = CORE_COLOR_COMPONENT_R_BIT;
            graphicsState.colorBlendState.colorAttachments[0].srcAlphaBlendFactor = CORE_BLEND_FACTOR_CONSTANT_ALPHA;
            graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor = CORE_BLEND_FACTOR_CONSTANT_COLOR;
            graphicsState.colorBlendState.colorAttachments[0].dstAlphaBlendFactor = CORE_BLEND_FACTOR_CONSTANT_ALPHA;
            graphicsState.colorBlendState.colorAttachments[0].dstColorBlendFactor = CORE_BLEND_FACTOR_CONSTANT_COLOR;

            graphicsState.colorBlendState.colorAttachments[0].colorBlendOp = CORE_BLEND_OP_ADD;
            graphicsState.colorBlendState.colorAttachments[0].alphaBlendOp = CORE_BLEND_OP_MAX;

            graphicsState.colorBlendState.colorBlendConstants[0] = 0.34f;
            graphicsState.colorBlendState.colorBlendConstants[1] = 0.2f;

            graphicsState.depthStencilState.enableStencilTest = true;
            graphicsState.depthStencilState.backStencilOpState.reference = 3u;
            graphicsState.depthStencilState.backStencilOpState.compareMask = 5u;
            graphicsState.depthStencilState.backStencilOpState.writeMask = 5u;

            graphicsState.depthStencilState.enableDepthTest = true;
            graphicsState.depthStencilState.depthCompareOp = CORE_COMPARE_OP_ALWAYS;
            graphicsState.depthStencilState.enableDepthWrite = true;

            graphicsState.rasterizationState.polygonMode = CORE_POLYGON_MODE_LINE;
            graphicsState.rasterizationState.enableDepthClamp = true;
            graphicsState.rasterizationState.enableRasterizerDiscard = true;
            graphicsState.rasterizationState.enableDepthBias = true;
            graphicsState.rasterizationState.depthBiasConstantFactor = 4.21f;
            graphicsState.rasterizationState.depthBiasSlopeFactor = 2.f;
            graphicsState.rasterizationState.cullModeFlags = CORE_CULL_MODE_FRONT_AND_BACK;
            graphicsState.rasterizationState.frontFace = CORE_FRONT_FACE_CLOCKWISE;
            graphicsState.rasterizationState.lineWidth = 4.2f;

            if (count_ == 1) {
                graphicsState.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_POINT_LIST;
                graphicsState.colorBlendState.colorAttachments[0].colorBlendOp = BlendOp::CORE_BLEND_OP_SUBTRACT;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ZERO;
                graphicsState.depthStencilState.depthCompareOp = CompareOp::CORE_COMPARE_OP_LESS;
                graphicsState.depthStencilState.backStencilOpState.passOp = StencilOp::CORE_STENCIL_OP_ZERO;
                graphicsState.depthStencilState.backStencilOpState.failOp = StencilOp::CORE_STENCIL_OP_REPLACE;
                graphicsState.depthStencilState.frontStencilOpState.passOp =
                    StencilOp::CORE_STENCIL_OP_INCREMENT_AND_CLAMP;
                graphicsState.depthStencilState.frontStencilOpState.failOp =
                    StencilOp::CORE_STENCIL_OP_DECREMENT_AND_CLAMP;
            } else if (count_ == 2) {
                graphicsState.inputAssembly.primitiveTopology =
                    PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
                graphicsState.colorBlendState.colorAttachments[0].colorBlendOp =
                    BlendOp::CORE_BLEND_OP_REVERSE_SUBTRACT;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ONE;
                graphicsState.depthStencilState.depthCompareOp = CompareOp::CORE_COMPARE_OP_EQUAL;
                graphicsState.depthStencilState.backStencilOpState.passOp = StencilOp::CORE_STENCIL_OP_INVERT;
                graphicsState.depthStencilState.backStencilOpState.failOp =
                    StencilOp::CORE_STENCIL_OP_INCREMENT_AND_WRAP;
                graphicsState.depthStencilState.frontStencilOpState.passOp =
                    StencilOp::CORE_STENCIL_OP_DECREMENT_AND_WRAP;
                graphicsState.depthStencilState.frontStencilOpState.failOp = StencilOp::CORE_STENCIL_OP_KEEP;
            } else if (count_ == 3) {
                graphicsState.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP;
                graphicsState.colorBlendState.colorAttachments[0].colorBlendOp = BlendOp::CORE_BLEND_OP_MIN;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_SRC_COLOR;
                graphicsState.depthStencilState.depthCompareOp = CompareOp::CORE_COMPARE_OP_LESS_OR_EQUAL;
            } else if (count_ == 4) {
                graphicsState.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
                graphicsState.colorBlendState.colorAttachments[0].colorBlendOp = BlendOp::CORE_BLEND_OP_MAX;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
                graphicsState.depthStencilState.depthCompareOp = CompareOp::CORE_COMPARE_OP_GREATER;
            } else if (count_ == 5) {
                graphicsState.inputAssembly.primitiveTopology =
                    PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_DST_COLOR;
                graphicsState.depthStencilState.depthCompareOp = CompareOp::CORE_COMPARE_OP_NOT_EQUAL;
                graphicsState.rasterizationState.cullModeFlags = CORE_CULL_MODE_BACK_BIT;
            } else if (count_ == 6) {
                graphicsState.inputAssembly.primitiveTopology =
                    PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_LINE_STRIP_WITH_ADJACENCY;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
                graphicsState.depthStencilState.depthCompareOp = CompareOp::CORE_COMPARE_OP_GREATER_OR_EQUAL;
                graphicsState.rasterizationState.cullModeFlags = CORE_CULL_MODE_FRONT_BIT;
            } else if (count_ == 7) {
                graphicsState.inputAssembly.primitiveTopology =
                    PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_SRC_ALPHA;
                graphicsState.rasterizationState.polygonMode = CORE_POLYGON_MODE_POINT;
            } else if (count_ == 8) {
                graphicsState.inputAssembly.primitiveTopology =
                    PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP_WITH_ADJACENCY;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
            } else if (count_ == 9) {
                graphicsState.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_DST_ALPHA;
            } else if (count_ == 10) {
                graphicsState.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_LINE_LIST;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            } else if (count_ == 11) {
                graphicsState.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
            } else if (count_ == 12) {
                graphicsState.inputAssembly.primitiveTopology = PrimitiveTopology::CORE_PRIMITIVE_TOPOLOGY_LINE_LIST;
                graphicsState.colorBlendState.colorAttachments[0].srcColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA;
                graphicsState.colorBlendState.colorAttachments[0].dstColorBlendFactor =
                    BlendFactor::CORE_BLEND_FACTOR_SRC_ALPHA_SATURATE;
            }
        }

        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        auto pipelineLayout = renderNodeUtil.CreatePipelineLayout(shaderHandle.GetHandle());
        const auto& inputLayout = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle);
        if (count_ == 1) {
            psoHandle_ = renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(
                shaderHandle.GetHandle(), graphicsState, pipelineLayout, inputLayout, specialization, {});
        } else {
            psoHandle_ =
                renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(shaderHandle.GetHandle(), graphicsState,
                    pipelineLayout, inputLayout, specialization, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
    }
#endif // NDEBUG
    cmdList.BindPipeline(psoHandle_);

#if NDEBUG
    if (count_ != 1) {
#endif
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);
#if NDEBUG
    }
#endif

    VertexBuffer vbo;
    vbo.bufferHandle = vertexBufferHandle_;
    cmdList.BindVertexBuffers({ &vbo, 1 });

    IndexBuffer ibo;
    if (count_ % 2 == 0) {
        ibo.bufferHandle = indexBuffer32Handle_;
        ibo.indexType = IndexType::CORE_INDEX_TYPE_UINT32;
    } else {
        ibo.bufferHandle = indexBuffer16Handle_;
        ibo.indexType = IndexType::CORE_INDEX_TYPE_UINT16;
    }
    cmdList.BindIndexBuffer(ibo);

    if (count_ < 2) {
        cmdList.DrawIndexed(6, 1, 0, 0, 0);
    } else {
        cmdList.DrawIndexed(6, 2, 0, 0, 0);
    }

    cmdList.EndRenderPass();
    count_++;
}

void RenderNodeDrawIndexed::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    inputRenderPassJson_ = parserUtil.GetInputRenderPass(jsonVal, "renderPass");

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(inputRenderPassJson_);
}

// for plugin / factory interface
IRenderNode* RenderNodeDrawIndexed::Create()
{
    return new RenderNodeDrawIndexed();
}

void RenderNodeDrawIndexed::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDrawIndexed*>(instance);
}
RENDER_END_NAMESPACE()