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

#include "render_node_back_buffer.h"

#include <base/containers/string.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
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
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

// shaders
#include "render/shaders/common/render_post_process_structs_common.h"

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

PostProcessTonemapStruct FillPushConstant(
    const GpuImageDesc& dstImageDesc, const RenderPostProcessConfiguration& currentRenderPostProcessConfiguration_)
{
    PostProcessTonemapStruct pushData;

    const float fWidth = static_cast<float>(dstImageDesc.width);
    const float fHeight = static_cast<float>(dstImageDesc.height);

    pushData.texSizeInvTexSize[0u] = fWidth;
    pushData.texSizeInvTexSize[1u] = fHeight;
    pushData.texSizeInvTexSize[2u] = 1.0f / fWidth;
    pushData.texSizeInvTexSize[3u] = 1.0f / fHeight;
    pushData.flags = currentRenderPostProcessConfiguration_.flags;
    pushData.tonemap = currentRenderPostProcessConfiguration_.factors[PostProcessConfiguration::INDEX_TONEMAP];
    pushData.vignette = currentRenderPostProcessConfiguration_.factors[PostProcessConfiguration::INDEX_VIGNETTE];
    pushData.colorFringe = currentRenderPostProcessConfiguration_.factors[PostProcessConfiguration::INDEX_COLOR_FRINGE];
    pushData.dither = currentRenderPostProcessConfiguration_.factors[PostProcessConfiguration::INDEX_DITHER];
    return pushData;
}
} // namespace

void RenderNodeBackBuffer::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    psoHandle_ = {};

    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_V("RenderNodeBackBuffer: render data store configuration not set in render node graph");
    }

    const auto& renderNodeUtil = renderNodeContextMgr.GetRenderNodeUtil();
    renderPass_ = renderNodeUtil.CreateRenderPass(inputRenderPass_);

    pipelineLayout_ = renderNodeUtil.CreatePipelineLayout(shader_);
    {
        const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
    }

    pipelineDescriptorSetBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(pipelineLayout_);
    renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
}

void RenderNodeBackBuffer::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const PostProcessConfiguration ppConfig = GetPostProcessConfiguration(renderDataStoreMgr);

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    }
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
        renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
    }

    // Size update mut be done before specialization
    const RenderHandle dstImageHandle = UpdateColorAttachmentSize();

    CheckForPsoSpecilization(ppConfig);

    const uint32_t firstSetIndex = pipelineDescriptorSetBinder_->GetFirstSet();
    for (auto refIndex : pipelineDescriptorSetBinder_->GetSetIndices()) {
        const auto descHandle = pipelineDescriptorSetBinder_->GetDescriptorSetHandle(refIndex);
        const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(refIndex);
        cmdList.UpdateDescriptorSet(descHandle, bindings);
    }

    cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);
    cmdList.BindPipeline(psoHandle_);

    // bind all sets
    cmdList.BindDescriptorSets(firstSetIndex, pipelineDescriptorSetBinder_->GetDescriptorSetHandles());

    // dynamic state
    cmdList.SetDynamicStateViewport(currentViewportDesc_);
    cmdList.SetDynamicStateScissor(currentScissorDesc_);

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const GpuImageDesc dstImageDesc = gpuResourceMgr.GetImageDescriptor(dstImageHandle);

    if (pipelineLayout_.pushConstant.byteSize > 0) {
        const PostProcessTonemapStruct pushData =
            FillPushConstant(dstImageDesc, currentRenderPostProcessConfiguration_);
        cmdList.PushConstant(pipelineLayout_.pushConstant, reinterpret_cast<const uint8_t*>(&pushData));
    }

    cmdList.Draw(3u, 1u, 0u, 0u); // vertexCount 3, drawing one triangle
    cmdList.EndRenderPass();
}

void RenderNodeBackBuffer::CheckForPsoSpecilization(const PostProcessConfiguration& postProcessConfiguration)
{
    const RenderPostProcessConfiguration renderPostProcessConfiguration =
        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(postProcessConfiguration);
    if (!RenderHandleUtil::IsValid(psoHandle_)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const RenderHandle graphicsState =
            renderNodeContextMgr_->GetShaderManager().GetGraphicsStateHandleByShaderHandle(shader_);
        psoHandle_ = psoMgr.GetGraphicsPsoHandle(
            shader_, graphicsState, pipelineLayout_, {}, {}, { DYNAMIC_STATES, BASE_NS::countof(DYNAMIC_STATES) });
    }

    // store new values
    currentRenderPostProcessConfiguration_ = renderPostProcessConfiguration;
}

PostProcessConfiguration RenderNodeBackBuffer::GetPostProcessConfiguration(
    const IRenderNodeRenderDataStoreManager& dataStoreMgr)
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto const dataStore = static_cast<IRenderDataStorePod const*>(
            dataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName));
        if (dataStore) {
            auto const dataView = dataStore->Get(jsonInputs_.renderDataStore.configurationName);
            const PostProcessConfiguration* data = (const PostProcessConfiguration*)dataView.data();
            if (data) {
                return *data;
            }
        }
    }
    return {};
}

RenderHandle RenderNodeBackBuffer::UpdateColorAttachmentSize()
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    RenderHandle dstImageHandle;
    if (!inputRenderPass_.attachments.empty()) {
        dstImageHandle = inputRenderPass_.attachments[0].handle;

        const GpuImageDesc& desc = gpuResourceMgr.GetImageDescriptor(inputRenderPass_.attachments[0].handle);
        if (desc.width != currentBackBuffer_.width || desc.height != currentBackBuffer_.height ||
            desc.format != currentBackBuffer_.format) {
            currentBackBuffer_.width = desc.width;
            currentBackBuffer_.height = desc.height;
            currentBackBuffer_.format = desc.format;

            // re-create render pass (swapchain/backbuffer size may have changed)
            const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
            renderPass_ = renderNodeUtil.CreateRenderPass(inputRenderPass_);

            currentViewportDesc_.x = 0.0f;
            currentViewportDesc_.y = 0.0f;
            currentViewportDesc_.width = static_cast<float>(currentBackBuffer_.width);
            currentViewportDesc_.height = static_cast<float>(currentBackBuffer_.height);

            currentScissorDesc_.offsetX = 0;
            currentScissorDesc_.offsetY = 0;
            currentScissorDesc_.extentWidth = currentBackBuffer_.width;
            currentScissorDesc_.extentHeight = currentBackBuffer_.height;
        }
    }
    return dstImageHandle;
}

void RenderNodeBackBuffer::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    shader_ = shaderMgr.GetShaderHandle(shaderName);

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);

    jsonInputs_.hasChangeableRenderPassHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.renderPass);
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);
}

// for plugin / factory interface
IRenderNode* RenderNodeBackBuffer::Create()
{
    return new RenderNodeBackBuffer();
}

void RenderNodeBackBuffer::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeBackBuffer*>(instance);
}
RENDER_END_NAMESPACE()
