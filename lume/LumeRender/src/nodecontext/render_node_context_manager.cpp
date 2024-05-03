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

#include "render_node_context_manager.h"

#include <cstdint>

#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_node_context_manager.h>

#include "datastore/render_data_store_manager.h"
#include "device/gpu_resource_manager.h"
#include "device/shader_manager.h"
#include "nodecontext/node_context_descriptor_set_manager.h"
#include "nodecontext/node_context_pso_manager.h"
#include "nodecontext/render_command_list.h"
#include "nodecontext/render_node_graph_share_manager.h"
#include "nodecontext/render_node_parser_util.h"
#include "nodecontext/render_node_util.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderNodeContextManager::RenderNodeContextManager(const CreateInfo& createInfo)
    : renderContext_(createInfo.renderContext), renderNodeGraphData_(createInfo.renderNodeGraphData),
      renderNodeGraphInputs_(createInfo.renderNodeGraphInputs),
      fullName_(renderNodeGraphData_.renderNodeGraphName + createInfo.name), nodeName_(createInfo.name),
      nodeJson_(createInfo.nodeJson), renderNodeGraphShareDataMgr_(createInfo.renderNodeGraphShareDataMgr),
      descriptorSetMgr_(createInfo.descriptorSetMgr), psoMgr_(createInfo.psoMgr), renderCommandList_(createInfo.cmdList)
{
    IDevice& dev = createInfo.renderContext.GetDevice();
    renderNodeGpuResourceMgr_ = make_unique<RenderNodeGpuResourceManager>(
        static_cast<GpuResourceManager&>(renderContext_.GetDevice().GetGpuResourceManager()));
    renderNodeShaderMgr_ = make_unique<RenderNodeShaderManager>((ShaderManager&)dev.GetShaderManager());
    renderNodeRenderDataStoreMgr_ = make_unique<RenderNodeRenderDataStoreManager>(
        (RenderDataStoreManager&)renderContext_.GetRenderDataStoreManager());
    renderNodeUtil_ = make_unique<RenderNodeUtil>(*this);
    renderNodeGraphShareMgr_ = make_unique<RenderNodeGraphShareManager>(renderNodeGraphShareDataMgr_);
    renderNodeParserUtil_ = make_unique<RenderNodeParserUtil>(RenderNodeParserUtil::CreateInfo {});

    // there are only build-in render node context interfaces
    contextInterfaces_.push_back({ RenderNodeContextManager::UID, this });
    contextInterfaces_.push_back({ RenderCommandList::UID, &renderCommandList_ });
    contextInterfaces_.push_back({ RenderNodeGraphShareManager::UID, renderNodeGraphShareMgr_.get() });
    contextInterfaces_.push_back({ RenderNodeParserUtil::UID, renderNodeParserUtil_.get() });
}

RenderNodeContextManager::~RenderNodeContextManager() = default;

void RenderNodeContextManager::BeginFrame(const uint32_t renderNodeIdx, const PerFrameTimings& frameTimings)
{
    if (renderNodeIdx_ == ~0u) {
        renderNodeGraphShareMgr_->Init(renderNodeIdx, nodeName_, fullName_);
    }
    renderNodeIdx_ = renderNodeIdx;
    renderNodeGraphShareMgr_->BeginFrame(renderNodeIdx);
    {
        // automatic timings to render nodes
        constexpr double uToMsDiv = 1000.0;
        constexpr double uToSDiv = 1000000.0;
        const float deltaTime = static_cast<float>(
            static_cast<double>(frameTimings.deltaTimeUs) / uToMsDiv); // real delta time used for scene as well
        const float totalTime = static_cast<float>(static_cast<double>(frameTimings.totalTimeUs) / uToSDiv);
        const uint32_t frameIndex =
            static_cast<uint32_t>((frameTimings.frameIndex % std::numeric_limits<uint32_t>::max()));
        auto& timings = renderNodeGraphData_.renderingConfiguration.renderTimings;
        timings = { deltaTime, deltaTime, totalTime, *reinterpret_cast<const float*>(&frameIndex) };
    }
}

const IRenderNodeRenderDataStoreManager& RenderNodeContextManager::GetRenderDataStoreManager() const
{
    return *renderNodeRenderDataStoreMgr_;
}

const IRenderNodeShaderManager& RenderNodeContextManager::GetShaderManager() const
{
    return *renderNodeShaderMgr_;
}

IRenderNodeGpuResourceManager& RenderNodeContextManager::GetGpuResourceManager()
{
    return *renderNodeGpuResourceMgr_;
}

INodeContextDescriptorSetManager& RenderNodeContextManager::GetDescriptorSetManager()
{
    return descriptorSetMgr_;
}

INodeContextPsoManager& RenderNodeContextManager::GetPsoManager()
{
    return psoMgr_;
}

IRenderNodeGraphShareManager& RenderNodeContextManager::GetRenderNodeGraphShareManager()
{
    return *renderNodeGraphShareMgr_;
}

const IRenderNodeUtil& RenderNodeContextManager::GetRenderNodeUtil() const
{
    return *renderNodeUtil_;
}

const IRenderNodeParserUtil& RenderNodeContextManager::GetRenderNodeParserUtil() const
{
    return *renderNodeParserUtil_;
}

const RenderNodeGraphData& RenderNodeContextManager::GetRenderNodeGraphData() const
{
    return renderNodeGraphData_;
}

string_view RenderNodeContextManager::GetName() const
{
    return fullName_;
}

string_view RenderNodeContextManager::GetNodeName() const
{
    return nodeName_;
}

CORE_NS::json::value RenderNodeContextManager::GetNodeJson() const
{
    return CORE_NS::json::parse(nodeJson_.data());
}

const RenderNodeGraphInputs& RenderNodeContextManager::GetRenderNodeGraphInputs() const
{
    return renderNodeGraphInputs_;
}

IRenderContext& RenderNodeContextManager::GetRenderContext() const
{
    return renderContext_;
}

CORE_NS::IInterface* RenderNodeContextManager::GetRenderNodeContextInterface(const Uid& uid) const
{
    for (const auto& ref : contextInterfaces_) {
        if (ref.uid == uid) {
            return ref.contextInterface;
        }
    }
    return nullptr;
}

const CORE_NS::IInterface* RenderNodeContextManager::GetInterface(const BASE_NS::Uid& uid) const
{
    if ((uid == IRenderNodeContextManager::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* RenderNodeContextManager::GetInterface(const BASE_NS::Uid& uid)
{
    if ((uid == IRenderNodeContextManager::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderNodeContextManager::Ref() {}

void RenderNodeContextManager::Unref() {}
RENDER_END_NAMESPACE()
