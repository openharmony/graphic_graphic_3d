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

#include "render_node_dotfield_render.h"

#include <3d/implementation_uids.h>
#include <3d/namespace.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include "render_data_store_default_dotfield.h"

namespace {
#include "app/shaders/common/dotfield_common.h"
#include "app/shaders/common/dotfield_struct_common.h"
} // namespace

#include <algorithm>
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;
using namespace CORE3D_NS;

namespace Dotfield {
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr string_view DEFAULT_DOTFIELD_RENDER_DATA_STORE_NAME_POST_FIX = "RenderDataStoreDefaultDotfield";
} // namespace

void RenderNodeDotfieldRender::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    shaderToPsoData_.clear();

    const auto& renderNodeUtil = renderNodeContextMgr.GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    renderPass_ = renderNodeUtil.CreateRenderPass(inputRenderPass_);
    if (auto* renderNodeSceneUtil = CORE_NS::GetInstance<CORE3D_NS::IRenderNodeSceneUtil>(
            *(renderNodeContextMgr_->GetRenderContext().GetInterface<IClassRegister>()),
            CORE3D_NS::UID_RENDER_NODE_SCENE_UTIL);
        renderNodeSceneUtil) {
        const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
        stores_ = renderNodeSceneUtil->GetSceneRenderDataStores(
            renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
    }
    cameraUniformBufferHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetBufferHandle(
        stores_.dataStoreNameScene + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);

    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
    {
        const DescriptorCounts dc { {
            { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u },
        } };
        descriptorSetMgr.ResetAndReserve(dc);
    }
    {
        const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr.GetShaderManager();
        shaderHandle_ = shaderMgr.GetShaderHandle("dotfieldshaders://shader/default_material_dotfield.shader");
        const RenderHandle plHandle =
            shaderMgr.GetPipelineLayoutHandle("dotfieldpipelinelayouts://default_material_dotfield.shaderpl");
        const PipelineLayout pipelineLayout = shaderMgr.GetPipelineLayout(plHandle);

        CORE_ASSERT(pipelineLayout.pushConstant.byteSize == sizeof(DotfieldMaterialPushConstantStruct));
        const uint32_t set = 0;
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, pipelineLayout);
        binders_.set0 = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetHandle, pipelineLayout.descriptorSetLayouts[set].bindings);
    }
}

void RenderNodeDotfieldRender::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
}

void RenderNodeDotfieldRender::PreExecuteFrame(){}

void RenderNodeDotfieldRender::ExecuteFrame(IRenderCommandList& cmdList)
{
    cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);

    RenderData(*renderNodeContextMgr_, cmdList);

    cmdList.EndRenderPass();
}

void RenderNodeDotfieldRender::RenderData(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList)
{
    const RenderDataStoreDefaultDotfield* dataStore = static_cast<RenderDataStoreDefaultDotfield*>(
        renderNodeContextMgr.GetRenderDataStoreManager().GetRenderDataStore(
            stores_.dataStoreNamePrefix + DEFAULT_DOTFIELD_RENDER_DATA_STORE_NAME_POST_FIX.data()));
    if (!dataStore) {
        return;
    }

    const auto& dotfieldPrimitives = dataStore->GetDotfieldPrimitives();
    if (dotfieldPrimitives.empty()) {
        return;
    }

    const auto& bufferData = dataStore->GetBufferData();

    const ViewportDesc viewportDesc = renderNodeContextMgr.GetRenderNodeUtil().CreateDefaultViewport(renderPass_);
    cmdList.SetDynamicStateViewport(viewportDesc);

    const ScissorDesc scissorDesc = renderNodeContextMgr.GetRenderNodeUtil().CreateDefaultScissor(renderPass_);
    cmdList.SetDynamicStateScissor(scissorDesc);

    auto& binder0 = *binders_.set0;
    binder0.BindBuffer(0, cameraUniformBufferHandle_, 0);
    cmdList.UpdateDescriptorSet(binder0.GetDescriptorSetHandle(), binder0.GetDescriptorSetLayoutBindingResources());

    const uint32_t primitiveCount = static_cast<uint32_t>(dotfieldPrimitives.size());
    const uint32_t currFrameIndex = bufferData.currFrameIndex;

    for (uint32_t idx = 0; idx < primitiveCount; ++idx) {
        const auto& dotfieldPrimitive = dotfieldPrimitives[idx];
        const auto& currEffect = bufferData.buffers[idx];

        const PsoData psoData = GetPsoData(renderNodeContextMgr, shaderHandle_);
        cmdList.BindPipeline(psoData.psoHandle);

        if (idx == 0) {
            cmdList.BindDescriptorSet(0, binder0.GetDescriptorSetHandle());
        }

        float time = dataStore->GetTime();
        const DotfieldMaterialPushConstantStruct pc = { { dotfieldPrimitive.size.x, dotfieldPrimitive.size.y, idx, 0 },
            { time, dotfieldPrimitive.pointScale, 0, 0 }, dotfieldPrimitive.colors, dotfieldPrimitive.matrix };
        cmdList.PushConstant(psoData.pushConstant, reinterpret_cast<const uint8_t*>(&pc));

        const VertexBuffer vb[] = {
            { currEffect.dataBuffer[currFrameIndex].GetHandle(), 0 },
        };
        cmdList.BindVertexBuffers(array_view(vb));

        cmdList.Draw(1, dotfieldPrimitive.size.x * dotfieldPrimitive.size.y, 0, 0);
    }
}

RenderNodeDotfieldRender::PsoData RenderNodeDotfieldRender::GetPsoData(
    IRenderNodeContextManager& renderNodeContextMgr, const RenderHandle& shaderHandle)
{
    if (const auto iter = shaderToPsoData_.find(shaderHandle.id); iter != shaderToPsoData_.cend()) {
        return iter->second;
    } else { // new
        const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
        const RenderHandle plHandle = shaderMgr.GetPipelineLayoutHandleByShaderHandle(shaderHandle);
        const PipelineLayout pipelineLayout = shaderMgr.GetPipelineLayout(plHandle);
        const RenderHandle vidHandle = shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(shaderHandle);
        const VertexInputDeclarationView vertexInputDeclaration = shaderMgr.GetVertexInputDeclarationView(vidHandle);
        const PipelineLayout& reflPipelineLayout = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
        const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle);

        const RenderHandle psoHandle =
            renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(shaderHandle, graphicsStateHandle, pipelineLayout,
                vertexInputDeclaration, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        const PsoData psoData = { psoHandle, reflPipelineLayout.pushConstant };
        shaderToPsoData_[shaderHandle.id] = psoData;
        return psoData;
    }
}

IRenderNode* RenderNodeDotfieldRender::Create()
{
    return new RenderNodeDotfieldRender();
}

void RenderNodeDotfieldRender::Destroy(IRenderNode* instance)
{
    delete (RenderNodeDotfieldRender*)instance;
}
} // namespace Dotfield
