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

#ifndef RENDER_RENDER__RENDER_NODE_GRAPH_SHARE_MANAGER_H
#define RENDER_RENDER__RENDER_NODE_GRAPH_SHARE_MANAGER_H

#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>

#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
class RenderNodeGraphShareDataManager {
public:
    RenderNodeGraphShareDataManager();
    ~RenderNodeGraphShareDataManager() = default;

    struct Resource {
        BASE_NS::string name;
        RenderHandle handle;
    };

    void BeginFrame(const uint32_t renderNodeCount, const BASE_NS::array_view<const RenderHandle> inputs,
        const BASE_NS::array_view<const RenderHandle> outputs);
    // prevents certain method data access and bakes shared data
    void PrepareExecuteFrame();

    void RegisterRenderNodeGraphOutputs(const BASE_NS::array_view<const RenderHandle> outputs);
    void RegisterRenderNodeOutput(
        const uint32_t renderNodeIdx, const BASE_NS::string_view name, const RenderHandle& handle);
    void RegisterRenderNodeName(const uint32_t renderNodeIdx, const BASE_NS::string_view name);

    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphInputs() const;
    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphOutputs() const;
    BASE_NS::array_view<const Resource> GetRenderNodeOutputs(const uint32_t renderNodeIdx) const;

    uint32_t GetRenderNodeIndex(const BASE_NS::string_view renderNodeName) const;

    static constexpr uint32_t MAX_RENDER_NODE_GRAPH_RES_COUNT { 8u };

private:
    struct InputOutput {
        PLUGIN_STATIC_ASSERT(MAX_RENDER_NODE_GRAPH_RES_COUNT == 8u);
        RenderHandle inputs[MAX_RENDER_NODE_GRAPH_RES_COUNT] { {}, {}, {}, {}, {}, {}, {}, {} };
        RenderHandle outputs[MAX_RENDER_NODE_GRAPH_RES_COUNT] { {}, {}, {}, {}, {}, {}, {}, {} };
        BASE_NS::array_view<RenderHandle> inputView;
        BASE_NS::array_view<RenderHandle> outputView;
    };
    // render node graph inputs/outputs which can be set through render node graph manager
    InputOutput inOut_;

    struct RenderNodeResources {
        BASE_NS::vector<Resource> outputs;
        RenderDataConstants::RenderDataFixedString name;
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

    void Init(const uint32_t renderNodeIdx, const BASE_NS::string_view name);
    void BeginFrame(const uint32_t renderNodeIdx);

    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphInputs() const override;
    BASE_NS::array_view<const RenderHandle> GetRenderNodeGraphOutputs() const override;
    RenderHandle GetRenderNodeGraphInput(const uint32_t index) const override;
    RenderHandle GetRenderNodeGraphOutput(const uint32_t index) const override;

    void RegisterRenderNodeGraphOutputs(const BASE_NS::array_view<const RenderHandle> outputs) override;
    void RegisterRenderNodeOutputs(const BASE_NS::array_view<const RenderHandle> outputs) override;
    void RegisterRenderNodeOutput(const BASE_NS::string_view name, const RenderHandle& handle) override;

    // NOTE: add method to register IRenderNodeInterface classes for frame
    // e.g. one could share additional data like viewports etc.

    RenderHandle GetRegisteredRenderNodeOutput(
        const BASE_NS::string_view renderNodeName, const uint32_t index) const override;
    RenderHandle GetRegisteredRenderNodeOutput(
        const BASE_NS::string_view renderNodeName, const BASE_NS::string_view resourceName) const override;
    RenderHandle GetRegisteredPrevRenderNodeOutput(const uint32_t index) const override;

    // for IRenderNodeContextInterface
    BASE_NS::string_view GetTypeName() const override
    {
        return "IRenderNodeGraphShareManager";
    }
    BASE_NS::Uid GetUid() const override
    {
        return IRenderNodeGraphShareManager::UID;
    }

private:
    RenderNodeGraphShareDataManager& renderNodeGraphShareDataMgr_;

    // render node index within a render node graph
    uint32_t renderNodeIdx_ { 0u };
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_GRAPH_SHARE_MANAGER_H
