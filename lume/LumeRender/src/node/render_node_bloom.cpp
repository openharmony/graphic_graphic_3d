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

#include "render_node_bloom.h"

#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "core/plugin/intf_class_factory.h"
#include "datastore/render_data_store_pod.h"
#include "postprocesses/render_post_process_bloom.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
inline BindableImage GetBindableImage(const RenderNodeResource& res)
{
    return BindableImage { res.handle, res.mip, res.layer, ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED, res.secondHandle };
}
} // namespace

void RenderNodeBloom::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    valid_ = true;

    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();
    CreatePostProcessInterface();

    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_E("RenderNodeBloom: render data store configuration not set in render node graph");
    }
    if (jsonInputs_.renderDataStore.typeName != RenderDataStorePod::TYPE_NAME) {
        PLUGIN_LOG_E("RenderNodeBloom: render data store type name not supported (%s != %s)",
            jsonInputs_.renderDataStore.typeName.data(), RenderDataStorePod::TYPE_NAME);
        valid_ = false;
    }

    if (ppRenderBloomInterface_.postProcessNode) {
        ppRenderBloomInterface_.postProcessNode->Init(ppRenderBloomInterface_.postProcess, *renderNodeContextMgr_);
    }

    renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(
        ppRenderBloomInterface_.postProcessNode->GetRenderDescriptorCounts());
}

void RenderNodeBloom::PreExecuteFrame()
{
    if (!valid_) {
        return;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    ProcessPostProcessConfiguration(renderNodeContextMgr_->GetRenderDataStoreManager());

    RenderPostProcessBloom& pp = static_cast<RenderPostProcessBloom&>(*ppRenderBloomInterface_.postProcess);
    pp.propertiesData.bloomConfiguration = ppConfig_.bloomConfiguration;
    pp.propertiesData.enabled = ((ppConfig_.enableFlags & PostProcessConfiguration::ENABLE_BLOOM_BIT) > 0);

    RenderPostProcessBloomNode& ppNode =
        static_cast<RenderPostProcessBloomNode&>(*ppRenderBloomInterface_.postProcessNode);
    ppNode.nodeInputsData.input = GetBindableImage(inputResources_.customInputImages[0]);
    ppNode.nodeOutputsData.output = GetBindableImage(inputResources_.customOutputImages[0]);

    ppRenderBloomInterface_.postProcessNode->PreExecute();
}

void RenderNodeBloom::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }

    ppRenderBloomInterface_.postProcessNode->Execute(cmdList);
}

void RenderNodeBloom::CreatePostProcessInterface()
{
    auto* renderClassFactory = renderNodeContextMgr_->GetRenderContext().GetInterface<IClassFactory>();
    if (renderClassFactory) {
        auto CreatePostProcessInterface = [&](const auto uid, auto& pp, auto& ppNode) {
            if (pp = CreateInstance<IRenderPostProcess>(*renderClassFactory, uid); pp) {
                ppNode = CreateInstance<IRenderPostProcessNode>(*renderClassFactory, pp->GetRenderPostProcessNodeUid());
            }
        };

        CreatePostProcessInterface(
            RenderPostProcessBloom::UID, ppRenderBloomInterface_.postProcess, ppRenderBloomInterface_.postProcessNode);
    }
    if (!(ppRenderBloomInterface_.postProcess && ppRenderBloomInterface_.postProcess)) {
        valid_ = false;
    }
}

IRenderNode::ExecuteFlags RenderNodeBloom::GetExecuteFlags() const
{
    // At the moment bloom needs typically copy even though it would not be in use
    return ExecuteFlagBits::EXECUTE_FLAG_BITS_DEFAULT;
}

void RenderNodeBloom::ProcessPostProcessConfiguration(const IRenderNodeRenderDataStoreManager& dataStoreMgr)
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        if (const IRenderDataStore* ds = dataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName);
            ds) {
            if (jsonInputs_.renderDataStore.typeName == RenderDataStorePod::TYPE_NAME) {
                auto const dataStore = static_cast<const IRenderDataStorePod*>(ds);
                auto const dataView = dataStore->Get(jsonInputs_.renderDataStore.configurationName);
                if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                    ppConfig_ = *((const PostProcessConfiguration*)dataView.data());
                }
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
