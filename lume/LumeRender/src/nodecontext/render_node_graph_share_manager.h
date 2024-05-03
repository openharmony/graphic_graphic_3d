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

#ifndef RENDER_RENDER__RENDER_NODE_GRAPH_SHARE_MANAGER_H
#define RENDER_RENDER__RENDER_NODE_GRAPH_SHARE_MANAGER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
struct RenderNodeGraphOutputResource;

/**
 * Global storage for specific one frame GPU resources.
 */
class RenderNodeGraphGlobalShareDataManager {
public:
    RenderNodeGraphGlobalShareDataManager() = default;
    ~RenderNodeGraphGlobalShareDataManager() = default;

    void BeginFrame();

    void SetGlobalRenderNodeResources(const BASE_NS::string_view nodeName,
        const BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> resources);
    BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> GetGlobalRenderNodeResources(
        const BASE_NS::string_view nodeName) const;

private:
    // data is only written in synchronous path in render node Init() PreExecuteFrame()
    // read is parallel
    struct RenderNodeDataSet {
        BASE_NS::vector<IRenderNodeGraphShareManager::NamedResource> resources;
    };
    BASE_NS::unordered_map<BASE_NS::string, RenderNodeDataSet> renderNodes_;
};

/**
 * Data container.
 */
class RenderNodeGraphShareDataManager {
public:
    RenderNodeGraphShareDataManager(const BASE_NS::array_view<const RenderNodeGraphOutputResource> rngOutputResources);
    ~RenderNodeGraphShareDataManager() = default;

    // prevRngShareDataMgr to access previous render node graph share manager
    // NOTE: a list of all RNG share data managers should be added, for named fetch of resources
    void BeginFrame(RenderNodeGraphGlobalShareDataManager* rngGlobalShareDataMgr,
        const RenderNodeGraphShareDataManager* prevRngShareDataMgr, const uint32_t renderNodeCount,
        const BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> inputs,
        const BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> outputs);
    // prevents certain method data access and bakes shared data
    void PrepareExecuteFrame();

    void RegisterRenderNodeOutput(
        const uint32_t renderNodeIdx, const BASE_NS::string_view name, const RenderHandle& handle);
    void RegisterRenderNodeName(
        const uint32_t renderNodeIdx, const BASE_NS::string_view name, const BASE_NS::string_view globalUniqueName);

    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphInputs() const;
    BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> GetNamedRenderNodeGraphInputs() const;
    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphOutputs() const;
    BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> GetNamedRenderNodeGraphOutputs() const;
    BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> GetRenderNodeOutputs(
        const uint32_t renderNodeIdx) const;

    uint32_t GetRenderNodeIndex(const BASE_NS::string_view renderNodeName) const;

    BASE_NS::array_view<const RenderHandle> GetPrevRenderNodeGraphOutputs() const;
    BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> GetNamedPrevRenderNodeGraphOutputs() const;

    void RegisterGlobalRenderNodeOutput(
        const uint32_t renderNodeIdx, const BASE_NS::string_view name, const RenderHandle& handle);
    BASE_NS::array_view<const IRenderNodeGraphShareManager::NamedResource> GetGlobalRenderNodeResources(
        const BASE_NS::string_view nodeName) const;

    static constexpr uint32_t MAX_RENDER_NODE_GRAPH_RES_COUNT { 8u };

private:
    // render node graph specified outputs
    const BASE_NS::vector<RenderNodeGraphOutputResource> rngOutputResources_;
    // indices to render nodes
    uint32_t outputMap_[MAX_RENDER_NODE_GRAPH_RES_COUNT] { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };

    RenderNodeGraphGlobalShareDataManager* rngGlobalShareDataMgr_ { nullptr };
    const RenderNodeGraphShareDataManager* prevRngShareDataMgr_ { nullptr };

    // checks if should be registered as render node output
    void RegisterAsRenderNodeGraphOutput(
        const uint32_t renderNodeIdx, const BASE_NS::string_view name, const RenderHandle& handle);

    struct GraphInputOutput {
        PLUGIN_STATIC_ASSERT(MAX_RENDER_NODE_GRAPH_RES_COUNT == 8u);
        RenderHandle inputs[MAX_RENDER_NODE_GRAPH_RES_COUNT] { {}, {}, {}, {}, {}, {}, {}, {} };
        IRenderNodeGraphShareManager::NamedResource namedInputs[MAX_RENDER_NODE_GRAPH_RES_COUNT] {};
        RenderHandle outputs[MAX_RENDER_NODE_GRAPH_RES_COUNT] { {}, {}, {}, {}, {}, {}, {}, {} };
        IRenderNodeGraphShareManager::NamedResource namedOutputs[MAX_RENDER_NODE_GRAPH_RES_COUNT] {};
        BASE_NS::array_view<RenderHandle> inputView;
        BASE_NS::array_view<IRenderNodeGraphShareManager::NamedResource> namedInputView;
        BASE_NS::array_view<RenderHandle> outputView;
        BASE_NS::array_view<IRenderNodeGraphShareManager::NamedResource> namedOutputView;
    };
    // render node graph inputs/outputs which can be set from the render node graph or render node graph manager
    GraphInputOutput inOut_;

    struct RenderNodeResources {
        BASE_NS::vector<IRenderNodeGraphShareManager::NamedResource> outputs;
        RenderDataConstants::RenderDataFixedString name;
        RenderDataConstants::RenderDataFixedString globalUniqueName;
    };

    // per render node registered inputs/outputs
    BASE_NS::vector<RenderNodeResources> renderNodeResources_;

    bool lockedAccess_ { false };
};

/**
 * Acts as a render node specific interface to data.
 */
class RenderNodeGraphShareManager final : public IRenderNodeGraphShareManager {
public:
    explicit RenderNodeGraphShareManager(RenderNodeGraphShareDataManager&);
    ~RenderNodeGraphShareManager() override;

    void Init(
        const uint32_t renderNodeIdx, const BASE_NS::string_view name, const BASE_NS::string_view globalUniqueName);
    void BeginFrame(const uint32_t renderNodeIdx);

    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphInputs() const override;
    BASE_NS::array_view<const NamedResource> GetNamedRenderNodeGraphInputs() const override;
    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphOutputs() const override;
    BASE_NS::array_view<const NamedResource> GetNamedRenderNodeGraphOutputs() const override;
    RenderHandle GetRenderNodeGraphInput(const uint32_t index) const override;
    RenderHandle GetRenderNodeGraphOutput(const uint32_t index) const override;
    BASE_NS::array_view<const RenderHandle> GetPrevRenderNodeGraphOutputs() const override;
    BASE_NS::array_view<const NamedResource> GetNamedPrevRenderNodeGraphOutputs() const override;
    RenderHandle GetPrevRenderNodeGraphOutput(const uint32_t index) const override;
    RenderHandle GetNamedPrevRenderNodeGraphOutput(const BASE_NS::string_view resourceName) const override;

    void RegisterRenderNodeOutputs(const BASE_NS::array_view<const RenderHandle> outputs) override;
    void RegisterRenderNodeOutput(const BASE_NS::string_view name, const RenderHandle& handle) override;

    RenderHandle GetRegisteredRenderNodeOutput(
        const BASE_NS::string_view renderNodeName, const uint32_t index) const override;
    RenderHandle GetRegisteredRenderNodeOutput(
        const BASE_NS::string_view renderNodeName, const BASE_NS::string_view resourceName) const override;
    RenderHandle GetRegisteredPrevRenderNodeOutput(const uint32_t index) const override;

    void RegisterGlobalRenderNodeOutput(const BASE_NS::string_view name, const RenderHandle& handle) override;
    BASE_NS::array_view<const NamedResource> GetRegisteredGlobalRenderNodeOutputs(
        const BASE_NS::string_view nodeName) const override;

    // IInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
    RenderNodeGraphShareDataManager& renderNodeGraphShareDataMgr_;

    // render node index within a render node graph
    uint32_t renderNodeIdx_ { 0u };
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_GRAPH_SHARE_MANAGER_H
