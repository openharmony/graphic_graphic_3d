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

#include "render_node_graph_share_manager.h"

#include <cstdint>

#include <base/math/mathf.h>
#include <render/namespace.h>

#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderNodeGraphShareDataManager::RenderNodeGraphShareDataManager() {}

void RenderNodeGraphShareDataManager::BeginFrame(const uint32_t renderNodeCount,
    const array_view<const RenderHandle> inputs, const array_view<const RenderHandle> outputs)
{
    lockedAccess_ = false;

    // copy / clear all
    inOut_ = {};
    inOut_.inputView = { inOut_.inputs, inputs.size() };
    inOut_.outputView = { inOut_.outputs, outputs.size() };
    for (size_t idx = 0; idx < inOut_.inputView.size(); ++idx) {
        inOut_.inputView[idx] = inputs[idx];
    }
    for (size_t idx = 0; idx < inOut_.outputView.size(); ++idx) {
        inOut_.outputView[idx] = outputs[idx];
    }

    renderNodeResources_.resize(renderNodeCount);

    // clear only view to gpu resource handles
    for (auto& rnRef : renderNodeResources_) {
        rnRef.outputs.clear();
    }
}

void RenderNodeGraphShareDataManager::PrepareExecuteFrame()
{
    lockedAccess_ = true;
}

void RenderNodeGraphShareDataManager::RegisterRenderNodeGraphOutputs(const array_view<const RenderHandle> outputs)
{
    if (lockedAccess_) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("RENDER_VALIDATION: render node graph outputs cannot be registered in ExecuteFrame()");
#endif
        return;
    }

    inOut_.outputView = {};
    inOut_.outputView = { inOut_.outputs, outputs.size() };
    for (size_t idx = 0; idx < inOut_.outputView.size(); ++idx) {
        inOut_.outputView[idx] = outputs[idx];
    }
}

void RenderNodeGraphShareDataManager::RegisterRenderNodeOutput(
    const uint32_t renderNodeIdx, const string_view name, const RenderHandle& handle)
{
    if (lockedAccess_) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(
            "RegisterRenderNodeOutput_val", "RENDER_VALIDATION: outputs cannot be registered in ExecuteFrame()");
#endif
        return; // early out
    }

    PLUGIN_ASSERT(renderNodeIdx < static_cast<uint32_t>(renderNodeResources_.size()));
    auto& rnRef = renderNodeResources_[renderNodeIdx];
    if (rnRef.outputs.size() >= MAX_RENDER_NODE_GRAPH_RES_COUNT) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W("RegisterRenderNodeOutput_valcount", "RENDER_VALIDATION: only %u outputs supported",
            MAX_RENDER_NODE_GRAPH_RES_COUNT);
#endif
        return; // early out
    }

    if (RenderHandleUtil::IsValid(handle)) {
        rnRef.outputs.emplace_back(Resource { string(name), handle });
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(string(rnRef.name + name) + string("_invha"),
            "RENDER_VALIDATION: invalid handle registered as render node output (name: %s) (render node name: %s)",
            name.data(), rnRef.name.data());
#endif
    }
}

void RenderNodeGraphShareDataManager::RegisterRenderNodeName(const uint32_t renderNodeIdx, const string_view name)
{
    PLUGIN_ASSERT(renderNodeIdx < static_cast<uint32_t>(renderNodeResources_.size()));
    renderNodeResources_[renderNodeIdx].name = name;
}

array_view<const RenderHandle> RenderNodeGraphShareDataManager::GetRenderNodeGraphInputs() const
{
    return inOut_.inputView;
}

array_view<const RenderHandle> RenderNodeGraphShareDataManager::GetRenderNodeGraphOutputs() const
{
    return inOut_.outputView;
}

array_view<const RenderNodeGraphShareDataManager::Resource> RenderNodeGraphShareDataManager::GetRenderNodeOutputs(
    const uint32_t renderNodeIdx) const
{
    if (renderNodeIdx < static_cast<uint32_t>(renderNodeResources_.size())) {
        return renderNodeResources_[renderNodeIdx].outputs;
    }
    return {};
}

uint32_t RenderNodeGraphShareDataManager::GetRenderNodeIndex(const string_view renderNodeName) const
{
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(renderNodeResources_.size()); ++idx) {
        if (renderNodeName == renderNodeResources_[idx].name.data()) {
            return idx;
        }
    }
    return ~0u;
}

RenderNodeGraphShareManager::RenderNodeGraphShareManager(RenderNodeGraphShareDataManager& renderNodeGraphShareDataMgr)
    : renderNodeGraphShareDataMgr_(renderNodeGraphShareDataMgr)
{}

RenderNodeGraphShareManager::~RenderNodeGraphShareManager() = default;

void RenderNodeGraphShareManager::Init(const uint32_t renderNodeIdx, const string_view name)
{
    renderNodeIdx_ = renderNodeIdx;
    renderNodeGraphShareDataMgr_.RegisterRenderNodeName(renderNodeIdx_, name);
}

void RenderNodeGraphShareManager::BeginFrame(const uint32_t renderNodeIdx)
{
    renderNodeIdx_ = renderNodeIdx;
}

array_view<const RenderHandle> RenderNodeGraphShareManager::GetRenderNodeGraphInputs() const
{
    return renderNodeGraphShareDataMgr_.GetRenderNodeGraphInputs();
}

array_view<const RenderHandle> RenderNodeGraphShareManager::GetRenderNodeGraphOutputs() const
{
    return renderNodeGraphShareDataMgr_.GetRenderNodeGraphOutputs();
}

RenderHandle RenderNodeGraphShareManager::GetRenderNodeGraphInput(const uint32_t index) const
{
    const auto& ref = renderNodeGraphShareDataMgr_.GetRenderNodeGraphInputs();
    return (index < ref.size()) ? ref[index] : RenderHandle {};
}

RenderHandle RenderNodeGraphShareManager::GetRenderNodeGraphOutput(const uint32_t index) const
{
    const auto& ref = renderNodeGraphShareDataMgr_.GetRenderNodeGraphOutputs();
    return (index < ref.size()) ? ref[index] : RenderHandle {};
}

void RenderNodeGraphShareManager::RegisterRenderNodeGraphOutputs(const array_view<const RenderHandle> outputHandles)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (outputHandles.size() > RenderNodeGraphShareDataManager::MAX_RENDER_NODE_GRAPH_RES_COUNT) {
        PLUGIN_LOG_W("RENDER_VALIDATION: render node tries to register %u render node graph outputs (max count: %u)",
            RenderNodeGraphShareDataManager::MAX_RENDER_NODE_GRAPH_RES_COUNT,
            static_cast<uint32_t>(outputHandles.size()));
    }
#endif
    renderNodeGraphShareDataMgr_.RegisterRenderNodeGraphOutputs(outputHandles);
}

void RenderNodeGraphShareManager::RegisterRenderNodeOutputs(const array_view<const RenderHandle> outputs)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (outputs.size() > RenderNodeGraphShareDataManager::MAX_RENDER_NODE_GRAPH_RES_COUNT) {
        PLUGIN_LOG_W("RENDER_VALIDATION: render node tries to register %u outputs (max count: %u)",
            static_cast<uint32_t>(outputs.size()), RenderNodeGraphShareDataManager::MAX_RENDER_NODE_GRAPH_RES_COUNT);
    }
#endif
    for (const auto& ref : outputs) {
        renderNodeGraphShareDataMgr_.RegisterRenderNodeOutput(renderNodeIdx_, {}, ref);
    }
}

void RenderNodeGraphShareManager::RegisterRenderNodeOutput(const string_view name, const RenderHandle& handle)
{
    renderNodeGraphShareDataMgr_.RegisterRenderNodeOutput(renderNodeIdx_, name, handle);
}

RenderHandle RenderNodeGraphShareManager::GetRegisteredRenderNodeOutput(
    const string_view renderNodeName, const uint32_t index) const
{
    const uint32_t rnIdx = renderNodeGraphShareDataMgr_.GetRenderNodeIndex(renderNodeName);
    const auto& ref = renderNodeGraphShareDataMgr_.GetRenderNodeOutputs(rnIdx);
    return (index < ref.size()) ? ref[index].handle : RenderHandle {};
}

RenderHandle RenderNodeGraphShareManager::GetRegisteredRenderNodeOutput(
    const string_view renderNodeName, const string_view resourceName) const
{
    const uint32_t rnIdx = renderNodeGraphShareDataMgr_.GetRenderNodeIndex(renderNodeName);
    const auto& ref = renderNodeGraphShareDataMgr_.GetRenderNodeOutputs(rnIdx);
    // check for name from the resources
    for (const auto& r : ref) {
        if (r.name == resourceName) {
            return r.handle;
        }
    }
    return RenderHandle {};
}

RenderHandle RenderNodeGraphShareManager::GetRegisteredPrevRenderNodeOutput(const uint32_t index) const
{
    const uint32_t rnIdx = renderNodeIdx_ - 1u;
    const auto& ref = renderNodeGraphShareDataMgr_.GetRenderNodeOutputs(rnIdx);
    return (index < ref.size()) ? ref[index].handle : RenderHandle {};
}
RENDER_END_NAMESPACE()
