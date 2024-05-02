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

#ifndef RENDER_RENDER__RENDER_NODE_GRAPH_MANAGER_H
#define RENDER_RENDER__RENDER_NODE_GRAPH_MANAGER_H

#include <cstdint>
#include <mutex>

#include <base/containers/string.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/namespace.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/resource_handle.h>

#include "nodecontext/render_node_graph_node_store.h"
#include "resource_handle_impl.h"

CORE_BEGIN_NAMESPACE()
class IFileManager;
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
class Device;
class RenderNodeManager;
class RenderNodeGraphLoader;

/**
RenderDataManager class.
Implementation for render data manager class.
*/
class RenderNodeGraphManager final : public IRenderNodeGraphManager {
public:
    RenderNodeGraphManager(Device& device, CORE_NS::IFileManager& fileMgr);
    ~RenderNodeGraphManager() override;

    RenderNodeGraphManager(const RenderNodeGraphManager&) = delete;
    RenderNodeGraphManager& operator=(const RenderNodeGraphManager&) = delete;

    RenderHandleReference Get(const RenderHandle& handle) const override;
    RenderHandleReference Create(const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc,
        const BASE_NS::string_view renderNodeGraphName,
        const BASE_NS::string_view renderNodeGraphDataStoreName) override;
    RenderHandleReference Create(const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc,
        const BASE_NS::string_view renderNodeGraphName) override;
    RenderHandleReference Create(const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc) override;

    // should be called once per frame for destruction of old render node graphs,
    // and for inserting or erasing individual render nodes
    void HandlePendingAllocations();

    void PushBackRenderNode(const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc) override;
    void EraseRenderNode(const RenderHandleReference& handle, const BASE_NS::string_view renderNodeName) override;
    void InsertBeforeRenderNode(const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc,
        const BASE_NS::string_view renderNodeName) override;
    void InsertAfterRenderNode(const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc,
        const BASE_NS::string_view renderNodeName) override;
    void UpdateRenderNodeGraph(
        const RenderHandleReference& handle, const RenderNodeGraphDescInfo& graphDescInfo) override;
    RenderNodeGraphDescInfo GetInfo(const RenderHandleReference& handle) const override;
    void SetRenderNodeGraphResources(const RenderHandleReference& handle,
        const BASE_NS::array_view<const RenderHandleReference> inputs,
        const BASE_NS::array_view<const RenderHandleReference> outputs) override;
    RenderNodeGraphResourceInfo GetRenderNodeGraphResources(const RenderHandleReference& handle) const override;

    RenderNodeGraphNodeStore* Get(RenderHandle handle);

    RenderHandleReference LoadAndCreate(const RenderNodeGraphUsageType usage, const BASE_NS::string_view uri) override;
    IRenderNodeGraphLoader& GetRenderNodeGraphLoader() override;

    RenderNodeManager& GetRenderNodeManager() const;

    void Destroy(BASE_NS::string_view typeName);

private:
    // not locked inside (needs to be locked from outside)
    void Destroy(const RenderHandle handle);

    struct PendingRenderNodeGraph {
        enum class Type : uint8_t {
            ALLOC = 0,
            DEALLOC = 1,
        };
        Type type { Type::ALLOC };

        RenderHandle renderNodeGraphHandle;
        BASE_NS::fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> renderNodeGraphName;
        BASE_NS::fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> renderNodeGraphDataStoreName;
        BASE_NS::fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> renderNodeGraphUri;
        RenderNodeGraphDesc renderNodeGraphDesc;
        RenderNodeGraphUsageType usageType { RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC };
    };
    // If individual nodes are insert or erased, we need a pending destruction
    struct PendingRenderNode {
        enum class Type : uint8_t {
            ALLOC = 0,
            DEALLOC = 1,
        };
        enum class PosType : uint8_t {
            BACK = 0,
            BEFORE = 1,
            AFTER = 2,
        };
        Type type { Type::ALLOC };

        RenderHandle renderNodeGraphHandle;
        RenderNodeDesc renderNodeDesc;

        PosType posType { PosType::BACK };
        RenderDataConstants::RenderDataFixedString renderNodeName;
    };
    struct PendingRenderNodeGraphDestruction {
        uint64_t frameIndex { 0u };
        BASE_NS::vector<RenderNodeContextData> renderNodeGraphContextData;
    };
    // If individual nodes are erased, we need a pending destruction
    struct PendingRenderNodeDestruction {
        uint64_t frameIndex { 0u };
        RenderNodeContextData renderNodeContextData;
    };

    void DynamicRenderNodeOpImpl(const RenderHandle handle, const RenderNodeDesc& renderNodeDesc,
        const PendingRenderNode::Type type, const PendingRenderNode::PosType posType,
        BASE_NS::string_view renderNodeName);

    // needs to be locked when called
    void PendingCreate(const PendingRenderNodeGraph& renderNodeGraph);
    void PendingDestroy(const RenderHandle nodeGraphHandle);
    void PendingAllocRenderNode(const RenderHandle nodeGraphHandle, const PendingRenderNode& pendingNode);
    void PendingDeallocRenderNode(const RenderHandle nodeGraphHandle, const BASE_NS::string_view renderNodeName);
    void UpdateRenderNodeGraphResources();

    Device& device_;
    BASE_NS::unique_ptr<RenderNodeManager> renderNodeMgr_;
    BASE_NS::unique_ptr<RenderNodeGraphLoader> renderNodeGraphLoader_;

    BASE_NS::vector<BASE_NS::unique_ptr<RenderNodeGraphNodeStore>> nodeGraphData_;
    BASE_NS::vector<RenderHandleReference> nodeGraphHandles_;
    // client-side
    struct RenderNodeGraphShareHandles {
        RenderHandleReference inputs[RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT] { {}, {}, {}, {} };
        RenderHandleReference outputs[RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT] { {}, {}, {}, {} };
        uint32_t inputCount { 0 };
        uint32_t outputCount { 0 };
    };
    BASE_NS::vector<RenderNodeGraphShareHandles> nodeGraphShareData_;

    BASE_NS::vector<uint64_t> availableHandleIds_;

    BASE_NS::vector<PendingRenderNodeGraphDestruction> pendingRenderNodeGraphDestructions_;
    BASE_NS::vector<PendingRenderNodeDestruction> pendingRenderNodeDestructions_;
    BASE_NS::vector<PendingRenderNodeGraph> pendingRenderNodeGraphs_;
    BASE_NS::vector<PendingRenderNode> pendingRenderNodes_;

    // mutex for all owned data
    mutable std::mutex mutex_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__RENDER_NODE_GRAPH_MANAGER_H
