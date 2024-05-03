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

#ifndef RENDER_RENDER__RENDER_NODE_CONTEXT_MANAGER_H
#define RENDER_RENDER__RENDER_NODE_CONTEXT_MANAGER_H

#include <cstdint>

#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_context_manager.h>

#include "datastore/render_data_store_manager.h"
#include "nodecontext/render_node_util.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class INodeContextDescriptorSetManager;
class INodeContextPsoManager;
class IRenderNodeGraphShareManager;
class IRenderNodeUtil;
class NodeContextDescriptorSetManager;
class NodeContextPsoManager;
class RenderDataStoreManager;
class RenderNodeGraphShareManager;
class RenderNodeGraphShareDataManager;
class RenderNodeParserUtil;

class ShaderManager;
class IRenderNodeGpuResourceManager;
class RenderNodeShaderManager;
class RenderNodeGpuResourceManager;
class RenderCommandList;

/**
class RenderNodeContextManager.
*/
class RenderNodeContextManager final : public IRenderNodeContextManager {
public:
    struct CreateInfo {
        IRenderContext& renderContext;
        const RenderNodeGraphData& renderNodeGraphData;
        const RenderNodeGraphInputs& renderNodeGraphInputs;
        const BASE_NS::string_view& name;
        const BASE_NS::string_view& nodeJson;
        NodeContextDescriptorSetManager& descriptorSetMgr;
        NodeContextPsoManager& psoMgr;
        RenderCommandList& cmdList;
        RenderNodeGraphShareDataManager& renderNodeGraphShareDataMgr;
    };
    struct PerFrameTimings {
        uint64_t totalTimeUs { 0 };
        uint64_t deltaTimeUs { 0 };
        uint64_t frameIndex { 0 };
    };

    explicit RenderNodeContextManager(const CreateInfo& createInfo);
    ~RenderNodeContextManager() override;

    void BeginFrame(const uint32_t renderNodeIdx, const PerFrameTimings& frameTimings);

    const IRenderNodeRenderDataStoreManager& GetRenderDataStoreManager() const override;
    const IRenderNodeShaderManager& GetShaderManager() const override;
    IRenderNodeGpuResourceManager& GetGpuResourceManager() override;

    INodeContextDescriptorSetManager& GetDescriptorSetManager() override;
    INodeContextPsoManager& GetPsoManager() override;
    IRenderNodeGraphShareManager& GetRenderNodeGraphShareManager() override;

    const IRenderNodeUtil& GetRenderNodeUtil() const override;
    const IRenderNodeParserUtil& GetRenderNodeParserUtil() const override;
    const RenderNodeGraphData& GetRenderNodeGraphData() const override;
    BASE_NS::string_view GetName() const override;
    BASE_NS::string_view GetNodeName() const override;
    CORE_NS::json::value GetNodeJson() const override;
    const RenderNodeGraphInputs& GetRenderNodeGraphInputs() const override;

    IRenderContext& GetRenderContext() const override;

    CORE_NS::IInterface* GetRenderNodeContextInterface(const BASE_NS::Uid& uid) const override;

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;
    void Ref() override;
    void Unref() override;

private:
    IRenderContext& renderContext_;
    RenderNodeGraphData renderNodeGraphData_;
    const RenderNodeGraphInputs renderNodeGraphInputs_;
    const BASE_NS::string fullName_;
    const BASE_NS::string nodeName_;
    const BASE_NS::string nodeJson_;

    RenderNodeGraphShareDataManager& renderNodeGraphShareDataMgr_;
    NodeContextDescriptorSetManager& descriptorSetMgr_;
    NodeContextPsoManager& psoMgr_;
    RenderCommandList& renderCommandList_;

    struct ContextInterface {
        BASE_NS::Uid uid {};
        CORE_NS::IInterface* contextInterface { nullptr };
    };
    BASE_NS::vector<ContextInterface> contextInterfaces_;

    BASE_NS::unique_ptr<RenderNodeGpuResourceManager> renderNodeGpuResourceMgr_;
    BASE_NS::unique_ptr<RenderNodeShaderManager> renderNodeShaderMgr_;
    BASE_NS::unique_ptr<RenderNodeRenderDataStoreManager> renderNodeRenderDataStoreMgr_;
    BASE_NS::unique_ptr<RenderNodeUtil> renderNodeUtil_;
    BASE_NS::unique_ptr<RenderNodeGraphShareManager> renderNodeGraphShareMgr_;
    BASE_NS::unique_ptr<RenderNodeParserUtil> renderNodeParserUtil_;

    // index in render node graph
    uint32_t renderNodeIdx_ { ~0u };
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_CONTEXT_MANAGER_H
