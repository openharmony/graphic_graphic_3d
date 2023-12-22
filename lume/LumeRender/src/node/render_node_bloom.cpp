/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "render_node_bloom.h"

#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t UBO_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };
constexpr uint32_t MAX_POST_PROCESS_EFFECT_COUNT { 2 };

RenderHandleReference CreatePostProcessDataUniformBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == UBO_OFFSET_ALIGNMENT);
    PLUGIN_STATIC_ASSERT(sizeof(LocalPostProcessStruct) == UBO_OFFSET_ALIGNMENT);
    return gpuResourceMgr.Create(handle,
        GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, UBO_OFFSET_ALIGNMENT * MAX_POST_PROCESS_EFFECT_COUNT });
}
} // namespace

void RenderNodeBloom::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_E("RenderNodeBloom: render data store configuration not set in render node graph");
    }

    postProcessUbo_ =
        CreatePostProcessDataUniformBuffer(renderNodeContextMgr_->GetGpuResourceManager(), postProcessUbo_);

    ProcessPostProcessConfiguration(renderNodeContextMgr_->GetRenderDataStoreManager());
    renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(renderBloom_.GetDescriptorCounts());
    const RenderBloom::BloomInfo info { inputResources_.customInputImages[0].handle,
        inputResources_.customOutputImages[0].handle, postProcessUbo_.GetHandle(),
        ppConfig_.bloomConfiguration.useCompute };
    renderBloom_.Init(renderNodeContextMgr, info);
}

void RenderNodeBloom::PreExecuteFrame()
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    ProcessPostProcessConfiguration(renderNodeContextMgr_->GetRenderDataStoreManager());
    UpdatePostProcessData(ppConfig_);

    const RenderBloom::BloomInfo info { inputResources_.customInputImages[0].handle,
        inputResources_.customOutputImages[0].handle, postProcessUbo_.GetHandle(),
        ppConfig_.bloomConfiguration.useCompute };
    renderBloom_.PreExecute(*renderNodeContextMgr_, info, ppConfig_);
}

void RenderNodeBloom::ExecuteFrame(IRenderCommandList& cmdList)
{
    renderBloom_.Execute(*renderNodeContextMgr_, cmdList, ppConfig_);
}

void RenderNodeBloom::ProcessPostProcessConfiguration(const IRenderNodeRenderDataStoreManager& dataStoreMgr)
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto const dataStore = static_cast<IRenderDataStorePod const*>(
            dataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName));
        if (dataStore) {
            auto const dataView = dataStore->Get(jsonInputs_.renderDataStore.configurationName);
            if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                ppConfig_ = *((const PostProcessConfiguration*)dataView.data());
            }
        }
    }
}

void RenderNodeBloom::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
}

void RenderNodeBloom::UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const RenderPostProcessConfiguration rppc =
        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(postProcessConfiguration);
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(postProcessUbo_.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(RenderPostProcessConfiguration);
        if (!CloneData(data, size_t(dataEnd - data), &rppc, sizeof(RenderPostProcessConfiguration))) {
            PLUGIN_LOG_E("post process ubo copying failed.");
        }
        gpuResourceMgr.UnmapBuffer(postProcessUbo_.GetHandle());
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeBloom::Create()
{
    return new RenderNodeBloom();
}

void RenderNodeBloom::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeBloom*>(instance);
}
RENDER_END_NAMESPACE()
