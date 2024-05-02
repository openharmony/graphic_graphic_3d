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

#include "render_node_graph_share_manager.h"

#include <cstdint>

#include <base/math/mathf.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>

#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

using namespace BASE_NS;

namespace {
#if (RENDER_VALIDATION_ENABLED == 1)
constexpr size_t A_BIG_TEST_NUMBER { 128U };
#endif
} // namespace

RENDER_BEGIN_NAMESPACE()
void RenderNodeGraphGlobalShareDataManager::BeginFrame()
{
    renderNodes_.clear();
}

void RenderNodeGraphGlobalShareDataManager::SetGlobalRenderNodeResources(
    const string_view nodeName, const array_view<const IRenderNodeGraphShareManager::NamedResource> resources)
{
    auto& ref = renderNodes_[nodeName];
    ref.resources.insert(ref.resources.end(), resources.begin(), resources.end());
#if (RENDER_VALIDATION_ENABLED == 1)
    // BeginFrame() not called?
    PLUGIN_ASSERT_MSG(ref.resources.size() < A_BIG_TEST_NUMBER,
        "RENDER_VALIDATION: BeginFrame not called for global share data mgr?");
#endif
}

array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphGlobalShareDataManager::GetGlobalRenderNodeResources(const string_view nodeName) const
{
    if (auto iter = renderNodes_.find(nodeName); iter != renderNodes_.cend()) {
        return iter->second.resources;
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_W("RENDER_VALIDATION: No global render node for resources found (%s)", nodeName.data());
#endif
        return {};
    }
}

RenderNodeGraphShareDataManager::RenderNodeGraphShareDataManager(
    const array_view<const RenderNodeGraphOutputResource> rngOutputResources)
    : rngOutputResources_(rngOutputResources.begin(), rngOutputResources.end())
{}

// NOTE: needs to have outputs at from which render node they are coming from
void RenderNodeGraphShareDataManager::BeginFrame(RenderNodeGraphGlobalShareDataManager* rngGlobalShareDataMgr,
    const RenderNodeGraphShareDataManager* prevRngShareDataMgr, const uint32_t renderNodeCount,
    const array_view<const IRenderNodeGraphShareManager::NamedResource> inputs,
    const array_view<const IRenderNodeGraphShareManager::NamedResource> outputs)
{
    rngGlobalShareDataMgr_ = rngGlobalShareDataMgr;
    prevRngShareDataMgr_ = prevRngShareDataMgr;
    lockedAccess_ = false;

    // copy / clear all
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((outputs.size() > 0) && (rngOutputResources_.size() > 0) && (outputs.size() != rngOutputResources_.size())) {
        // NOTE: render node graph name needed
        PLUGIN_LOG_ONCE_W("RenderNodeGraphShareDataManager::BeginFrame_output_miss",
            "RENDER_VALIDATION: given output size missmatch with render node graph outputs.");
    }
#endif
    const uint32_t outputCount = static_cast<uint32_t>(Math::max(rngOutputResources_.size(), outputs.size()));
    inOut_ = {};
    inOut_.inputView = { inOut_.inputs, inputs.size() };
    inOut_.outputView = { inOut_.outputs, outputCount };
    inOut_.namedInputView = { inOut_.namedInputs, inputs.size() };
    inOut_.namedOutputView = { inOut_.namedOutputs, outputCount };
    for (size_t idx = 0; idx < inOut_.inputView.size(); ++idx) {
        inOut_.inputView[idx] = inputs[idx].handle;
        inOut_.namedInputView[idx] = inputs[idx];
    }
    for (size_t idx = 0; idx < inOut_.outputView.size(); ++idx) {
        // NOTE: decision needed for output management
        if (idx < outputs.size()) {
            inOut_.outputView[idx] = outputs[idx].handle;
            inOut_.namedOutputView[idx] = outputs[idx];
        } else {
            inOut_.outputView[idx] = {};
            inOut_.namedOutputView[idx] =
                (idx < rngOutputResources_.size())
                    ? IRenderNodeGraphShareManager::NamedResource { rngOutputResources_[idx].name, {} }
                    : IRenderNodeGraphShareManager::NamedResource {};
        }
    }
    // NOTE: output map is not cleared

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
        // check if render node output is used as render graph output as well
        RegisterAsRenderNodeGraphOutput(renderNodeIdx, name, handle);
        rnRef.outputs.push_back(IRenderNodeGraphShareManager::NamedResource { name, handle });
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(string(rnRef.name + name) + string("_invha"),
            "RENDER_VALIDATION: invalid handle registered as render node output (name: %s) (render node name: %s)",
            name.data(), rnRef.name.data());
#endif
    }
}

void RenderNodeGraphShareDataManager::RegisterAsRenderNodeGraphOutput(
    const uint32_t renderNodeIdx, const string_view name, const RenderHandle& handle)
{
    if (name.empty()) {
        return;
    }

    bool rnNeedsMap = false;
    // check if render node index matches the map list first
    const uint32_t outputCount = static_cast<uint32_t>(inOut_.outputView.size());
    for (uint32_t idx = 0; idx < outputCount; ++idx) {
        if (outputMap_[idx] == renderNodeIdx) {
            rnNeedsMap = true;
            break;
        }
    }
    if (rnNeedsMap) {
        // do string match if this specific render node output is used as render graph output as well
        for (uint32_t idx = 0; idx < outputCount; ++idx) {
            if (inOut_.namedOutputView[idx].name == name) {
                inOut_.namedOutputView[idx].handle = handle;
                break;
            }
        }
    }
}

void RenderNodeGraphShareDataManager::RegisterRenderNodeName(
    const uint32_t renderNodeIdx, const string_view name, const string_view globalUniqueName)
{
    PLUGIN_ASSERT(renderNodeIdx < static_cast<uint32_t>(renderNodeResources_.size()));
    renderNodeResources_[renderNodeIdx].name = name;
    renderNodeResources_[renderNodeIdx].globalUniqueName = globalUniqueName;
    // add render node index to easier mapping and checking without string comparison
    const uint32_t maxCount =
        Math::min(static_cast<uint32_t>(rngOutputResources_.size()), MAX_RENDER_NODE_GRAPH_RES_COUNT);
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        const auto& rngOutputRef = rngOutputResources_[idx];
        if (rngOutputRef.nodeName == name) {
            outputMap_[idx] = renderNodeIdx;
        }
    }
}

array_view<const RenderHandle> RenderNodeGraphShareDataManager::GetRenderNodeGraphInputs() const
{
    return inOut_.inputView;
}

array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareDataManager::GetNamedRenderNodeGraphInputs() const
{
    return inOut_.namedInputView;
}

array_view<const RenderHandle> RenderNodeGraphShareDataManager::GetRenderNodeGraphOutputs() const
{
    return inOut_.outputView;
}

array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareDataManager::GetNamedRenderNodeGraphOutputs() const
{
    return inOut_.namedOutputView;
}

array_view<const IRenderNodeGraphShareManager::NamedResource> RenderNodeGraphShareDataManager::GetRenderNodeOutputs(
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

array_view<const RenderHandle> RenderNodeGraphShareDataManager::GetPrevRenderNodeGraphOutputs() const
{
    if (prevRngShareDataMgr_) {
        return prevRngShareDataMgr_->GetRenderNodeGraphOutputs();
    } else {
        return {};
    }
}

void RenderNodeGraphShareDataManager::RegisterGlobalRenderNodeOutput(
    const uint32_t renderNodeIdx, const BASE_NS::string_view name, const RenderHandle& handle)
{
    if (lockedAccess_) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(
            "RegisterGlobalRenderNodeOutput_val", "RENDER_VALIDATION: outputs cannot be registered in ExecuteFrame()");
#endif
        return; // early out
    }

    if (rngGlobalShareDataMgr_ && (renderNodeIdx < static_cast<uint32_t>(renderNodeResources_.size()))) {
        const IRenderNodeGraphShareManager::NamedResource namedResource { name, handle };
        // NOTE: global unique name
        rngGlobalShareDataMgr_->SetGlobalRenderNodeResources(
            renderNodeResources_[renderNodeIdx].globalUniqueName, { &namedResource, 1U });
    }
}

BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareDataManager::GetGlobalRenderNodeResources(const BASE_NS::string_view nodeName) const
{
    if (rngGlobalShareDataMgr_) {
        return rngGlobalShareDataMgr_->GetGlobalRenderNodeResources(nodeName);
    } else {
        return {};
    }
}

array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareDataManager::GetNamedPrevRenderNodeGraphOutputs() const
{
    if (prevRngShareDataMgr_) {
        return prevRngShareDataMgr_->GetNamedRenderNodeGraphOutputs();
    } else {
        return {};
    }
}

RenderNodeGraphShareManager::RenderNodeGraphShareManager(RenderNodeGraphShareDataManager& renderNodeGraphShareDataMgr)
    : renderNodeGraphShareDataMgr_(renderNodeGraphShareDataMgr)
{}

RenderNodeGraphShareManager::~RenderNodeGraphShareManager() = default;

void RenderNodeGraphShareManager::Init(
    const uint32_t renderNodeIdx, const string_view name, const string_view globalUniqueName)
{
    renderNodeIdx_ = renderNodeIdx;
    renderNodeGraphShareDataMgr_.RegisterRenderNodeName(renderNodeIdx_, name, globalUniqueName);
}

void RenderNodeGraphShareManager::BeginFrame(const uint32_t renderNodeIdx)
{
    renderNodeIdx_ = renderNodeIdx;
}

array_view<const RenderHandle> RenderNodeGraphShareManager::GetRenderNodeGraphInputs() const
{
    return renderNodeGraphShareDataMgr_.GetRenderNodeGraphInputs();
}

array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareManager::GetNamedRenderNodeGraphInputs() const
{
    return renderNodeGraphShareDataMgr_.GetNamedRenderNodeGraphInputs();
}

array_view<const RenderHandle> RenderNodeGraphShareManager::GetRenderNodeGraphOutputs() const
{
    return renderNodeGraphShareDataMgr_.GetRenderNodeGraphOutputs();
}

array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareManager::GetNamedRenderNodeGraphOutputs() const
{
    return renderNodeGraphShareDataMgr_.GetNamedRenderNodeGraphOutputs();
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

array_view<const RenderHandle> RenderNodeGraphShareManager::GetPrevRenderNodeGraphOutputs() const
{
    return renderNodeGraphShareDataMgr_.GetPrevRenderNodeGraphOutputs();
}

array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareManager::GetNamedPrevRenderNodeGraphOutputs() const
{
    return renderNodeGraphShareDataMgr_.GetNamedPrevRenderNodeGraphOutputs();
}

RenderHandle RenderNodeGraphShareManager::GetPrevRenderNodeGraphOutput(const uint32_t index) const
{
    const auto& ref = renderNodeGraphShareDataMgr_.GetPrevRenderNodeGraphOutputs();
    return (index < ref.size()) ? ref[index] : RenderHandle {};
}

RenderHandle RenderNodeGraphShareManager::GetNamedPrevRenderNodeGraphOutput(const string_view resourceName) const
{
    const auto& ref = renderNodeGraphShareDataMgr_.GetNamedPrevRenderNodeGraphOutputs();
    for (const auto& resRef : ref) {
        if (resRef.name == resourceName) {
            return resRef.handle;
        }
    }
    return RenderHandle {};
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

void RenderNodeGraphShareManager::RegisterGlobalRenderNodeOutput(
    const BASE_NS::string_view name, const RenderHandle& handle)
{
    renderNodeGraphShareDataMgr_.RegisterGlobalRenderNodeOutput(renderNodeIdx_, name, handle);
}

BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource>
RenderNodeGraphShareManager::GetRegisteredGlobalRenderNodeOutputs(const BASE_NS::string_view nodeName) const
{
    return renderNodeGraphShareDataMgr_.GetGlobalRenderNodeResources(nodeName);
}

const CORE_NS::IInterface* RenderNodeGraphShareManager::GetInterface(const BASE_NS::Uid& uid) const
{
    if ((uid == IRenderNodeGraphShareManager::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

CORE_NS::IInterface* RenderNodeGraphShareManager::GetInterface(const BASE_NS::Uid& uid)
{
    if ((uid == IRenderNodeGraphShareManager::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderNodeGraphShareManager::Ref() {}

void RenderNodeGraphShareManager::Unref() {}
RENDER_END_NAMESPACE()
