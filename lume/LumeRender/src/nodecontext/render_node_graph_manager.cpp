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

#include "render_node_graph_manager.h"

#include <algorithm>
#include <cinttypes>
#include <cstdint>

#include <base/math/mathf.h>
#include <core/io/intf_file_manager.h>
#if (RENDER_PERF_ENABLED == 1)
#include <core/implementation_uids.h>
#include <core/perf/intf_performance_data_manager.h>
#endif

#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/device.h"
#include "device/gpu_resource_handle_util.h"
#include "device/gpu_resource_manager.h"
#include "loader/render_node_graph_loader.h"
#include "nodecontext/render_node_graph_node_store.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
void ValidateBackendFlags(
    const string_view name, const DeviceBackendType backendType, const IRenderNode::BackendFlags backendFlags)
{
#if (RENDER_VALIDATION_ENABLED == 1)
    if (backendFlags != IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT) {
        if ((backendType == DeviceBackendType::VULKAN) &&
            ((backendFlags & IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_EXPLICIT_VULKAN) == 0)) {
            PLUGIN_LOG_E("unsupported (missing vulkan) render node backend flags for render node %s", name.data());
        } else if ((backendType == DeviceBackendType::OPENGLES) &&
                   ((backendFlags & IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_EXPLICIT_GLES) == 0)) {
            PLUGIN_LOG_E("unsupported (missing gles) render node backend flags for render node %s", name.data());
        } else if ((backendType == DeviceBackendType::OPENGL) &&
                   ((backendFlags & IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_EXPLICIT_GL) == 0)) {
            PLUGIN_LOG_E("unsupported (missing gl) render node backend flags for render node %s", name.data());
        }
    }
#endif
}
} // namespace

RenderNodeGraphManager::RenderNodeGraphManager(Device& device, CORE_NS::IFileManager& fileMgr)
    : device_(device), renderNodeMgr_(make_unique<RenderNodeManager>()),
      renderNodeGraphLoader_(make_unique<RenderNodeGraphLoader>(fileMgr))
{}

RenderNodeGraphManager::~RenderNodeGraphManager()
{
#if (RENDER_VALIDATION_ENABLED == 1)
    uint32_t aliveRngCounter = 0;
    for (const auto& ref : nodeGraphHandles_) {
        if (ref && (ref.GetRefCount() > 1)) {
            aliveRngCounter++;
        }
    }
    if (aliveRngCounter > 0) {
        PLUGIN_LOG_W(
            "RENDER_VALIDATION: Not all render node graph handle references released (count: %u)", aliveRngCounter);
    }
#endif
}

RenderHandleReference RenderNodeGraphManager::Get(const RenderHandle& handle) const
{
    if (RenderHandleUtil::GetHandleType(handle) == RenderHandleType::RENDER_NODE_GRAPH) {
        const uint32_t arrayIndex = RenderHandleUtil::GetIndexPart(handle);
        const auto lock = std::lock_guard(mutex_);

        if (arrayIndex < static_cast<uint32_t>(nodeGraphHandles_.size())) {
            return nodeGraphHandles_[arrayIndex];
        } else {
            PLUGIN_LOG_E("invalid render node graph handle (id: %" PRIu64 ")", handle.id);
        }
    }
    return RenderHandleReference {};
}

RenderHandleReference RenderNodeGraphManager::LoadAndCreate(const RenderNodeGraphUsageType usage, const string_view uri)
{
    if (const auto result = renderNodeGraphLoader_->Load(uri); !result.error.empty()) {
        PLUGIN_LOG_W("Load and create for render node graph failed: %s %s ", uri.data(), result.error.c_str());
        return {};
    } else {
        return Create(usage, result.desc);
    }
}

IRenderNodeGraphLoader& RenderNodeGraphManager::GetRenderNodeGraphLoader()
{
    return *renderNodeGraphLoader_;
}

RenderHandleReference RenderNodeGraphManager::Create(const RenderNodeGraphUsageType usage,
    const RenderNodeGraphDesc& desc, const string_view renderNodeGraphName,
    const string_view renderNodeGraphDataStoreName)
{
    const auto lock = std::lock_guard(mutex_);

    uint64_t handleId = INVALID_RESOURCE_HANDLE;
    bool newHandle = true;
    if (availableHandleIds_.empty()) { // no available indices
        handleId = static_cast<uint64_t>(nodeGraphData_.size()) << RenderHandleUtil::RES_HANDLE_ID_SHIFT;
        newHandle = true;
    } else {
        handleId = availableHandleIds_.back();
        availableHandleIds_.pop_back();
        newHandle = false;
    }

    const uint32_t indexPart = RenderHandleUtil::GetIndexPart(handleId);
    const uint32_t generationIndexPart = RenderHandleUtil::GetGenerationIndexPart(handleId) + 1; // next gen

    const RenderHandle handle =
        RenderHandleUtil::CreateHandle(RenderHandleType::RENDER_NODE_GRAPH, indexPart, generationIndexPart);
    RenderHandleReference rhr {
        handle,
        IRenderReferenceCounter::Ptr(new RenderReferenceCounter()),
    };
    if (newHandle) {
        nodeGraphData_.emplace_back(); // deferred
        nodeGraphHandles_.push_back(move(rhr));
        nodeGraphShareData_.emplace_back();
    } else {
        nodeGraphData_[indexPart] = {}; // deferred
        nodeGraphHandles_[indexPart] = move(rhr);
        nodeGraphShareData_[indexPart] = {};
    }
    pendingRenderNodeGraphs_.push_back({
        PendingRenderNodeGraph::Type::ALLOC,
        handle,
        renderNodeGraphName.empty() ? string_view(desc.renderNodeGraphName) : renderNodeGraphName,
        renderNodeGraphDataStoreName.empty() ? string_view(desc.renderNodeGraphDataStoreName)
                                             : renderNodeGraphDataStoreName,
        desc.renderNodeGraphUri,
        desc,
        usage,
    });

    PLUGIN_ASSERT(indexPart < static_cast<uint32_t>(nodeGraphHandles_.size()));
    return nodeGraphHandles_[indexPart];
}

RenderHandleReference RenderNodeGraphManager::Create(
    const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc, const string_view renderNodeGraphName)
{
    return Create(usage, desc, renderNodeGraphName, {});
}

RenderHandleReference RenderNodeGraphManager::Create(
    const RenderNodeGraphUsageType usage, const RenderNodeGraphDesc& desc)
{
    return Create(usage, desc, {}, {});
}

void RenderNodeGraphManager::Destroy(const RenderHandle handle)
{
    if (RenderHandleUtil::GetHandleType(handle) == RenderHandleType::RENDER_NODE_GRAPH) {
        const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
        const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(handle);
        if (index < static_cast<uint32_t>(nodeGraphHandles_.size())) {
            const uint32_t storedGenerationIdx =
                RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
            // ignore if not correct generation index
            if (generationIdx == storedGenerationIdx) {
                pendingRenderNodeGraphs_.push_back({ PendingRenderNodeGraph::Type::DEALLOC, handle, "", "", {}, {} });
            }
        }
    }
}

void RenderNodeGraphManager::HandlePendingAllocations()
{
    // deferred multi-frame destruction for RenderNodeContextData needed
    const uint64_t minAge = static_cast<uint64_t>(device_.GetCommandBufferingCount()) + 1;
    const uint64_t ageLimit = (device_.GetFrameCount() < minAge) ? 0 : (device_.GetFrameCount() - minAge);

    {
        // needs to be locked the whole time
        // methods access private members like nodeGraphData_
        const auto lock = std::lock_guard(mutex_);

        // check render node graphs for destruction
        for (auto& rngRef : nodeGraphHandles_) {
            if (rngRef && (rngRef.GetRefCount() <= 1)) {
                Destroy(rngRef.GetHandle());
            }
        }

        // alloc/dealloc individual render node graphs
        for (const auto& ref : pendingRenderNodeGraphs_) {
            if (ref.type == PendingRenderNodeGraph::Type::ALLOC) {
                PendingCreate(ref);
            } else if (ref.type == PendingRenderNodeGraph::Type::DEALLOC) {
                PendingDestroy(ref.renderNodeGraphHandle);
            }
        }
        pendingRenderNodeGraphs_.clear();

        // alloc/dealloc individual render nodes
        for (auto& ref : pendingRenderNodes_) {
            if (ref.type == PendingRenderNode::Type::ALLOC) {
                PendingAllocRenderNode(ref.renderNodeGraphHandle, ref);
            } else if (ref.type == PendingRenderNode::Type::DEALLOC) {
                PendingDeallocRenderNode(ref.renderNodeGraphHandle, ref.renderNodeDesc.nodeName);
            }
        }
        pendingRenderNodes_.clear();

        // check for updated render node graph inputs / outputs
        UpdateRenderNodeGraphResources();

        // multi-frame deferred destructions

        // render node graph destruction
        if (!pendingRenderNodeGraphDestructions_.empty()) {
            const auto oldResources =
                std::partition(pendingRenderNodeGraphDestructions_.begin(), pendingRenderNodeGraphDestructions_.end(),
                    [ageLimit](const auto& destructionQueue) { return destructionQueue.frameIndex >= ageLimit; });
            pendingRenderNodeGraphDestructions_.erase(oldResources, pendingRenderNodeGraphDestructions_.end());
        }
        // individual render node destruction
        if (!pendingRenderNodeDestructions_.empty()) {
            const auto oldResources =
                std::partition(pendingRenderNodeDestructions_.begin(), pendingRenderNodeDestructions_.end(),
                    [ageLimit](const auto& destructionQueue) { return destructionQueue.frameIndex >= ageLimit; });
            pendingRenderNodeDestructions_.erase(oldResources, pendingRenderNodeDestructions_.end());
        }
    }
}

void RenderNodeGraphManager::PendingCreate(const PendingRenderNodeGraph& renderNodeGraph)
{
    unique_ptr<RenderNodeGraphNodeStore> nodeStore = make_unique<RenderNodeGraphNodeStore>();
    nodeStore->dynamic = (renderNodeGraph.usageType == RenderNodeGraphUsageType::RENDER_NODE_GRAPH_DYNAMIC);
    nodeStore->initialized = false;
    nodeStore->renderNodeGraphName = renderNodeGraph.renderNodeGraphName;
    nodeStore->renderNodeGraphDataStoreName = renderNodeGraph.renderNodeGraphDataStoreName;
    nodeStore->renderNodeGraphUri = renderNodeGraph.renderNodeGraphUri;
    // many of the resources are not yet available as handles
    nodeStore->renderNodeGraphShareDataMgr =
        make_unique<RenderNodeGraphShareDataManager>(renderNodeGraph.renderNodeGraphDesc.outputResources);

    const size_t reserveSize = renderNodeGraph.renderNodeGraphDesc.nodes.size();
    nodeStore->renderNodeData.reserve(reserveSize);
    nodeStore->renderNodeContextData.reserve(reserveSize);
    for (const auto& nodeDesc : renderNodeGraph.renderNodeGraphDesc.nodes) {
        // combined name is used
        const RenderDataConstants::RenderDataFixedString combinedNodeName =
            string_view(nodeStore->renderNodeGraphName + nodeDesc.nodeName);
        auto node = renderNodeMgr_->CreateRenderNode(nodeDesc.typeName.c_str());
        if (node) {
            nodeStore->renderNodeData.push_back({
                move(node),
                nodeDesc.typeName,
                combinedNodeName,
                nodeDesc.nodeName,
                make_unique<RenderNodeGraphInputs>(nodeDesc.description),
                nodeDesc.nodeJson,
            });
            auto& contextRef = nodeStore->renderNodeContextData.emplace_back();
            const RenderNodeManager::RenderNodeTypeInfoFlags typeInfoFlags =
                renderNodeMgr_->GetRenderNodeTypeInfoFlags(nodeDesc.typeName.c_str());
            const IRenderNode::ClassType backendNode = static_cast<IRenderNode::ClassType>(typeInfoFlags.classType);
            contextRef.renderBackendNode =
                (backendNode == IRenderNode::ClassType::CLASS_TYPE_BACKEND_NODE)
                    ? reinterpret_cast<IRenderBackendNode*>(nodeStore->renderNodeData.back().node.get())
                    : nullptr;
            ValidateBackendFlags(combinedNodeName, device_.GetBackendType(), typeInfoFlags.backendFlags);
        } else {
            PLUGIN_LOG_W("render node type: %s, named: %s, not found and not used", nodeDesc.typeName.c_str(),
                nodeDesc.nodeName.c_str());
        }
    }

    // NOTE: currently locked from outside
    const uint32_t indexPart = RenderHandleUtil::GetIndexPart(renderNodeGraph.renderNodeGraphHandle);
    PLUGIN_ASSERT(indexPart < nodeGraphData_.size());
    nodeGraphData_[indexPart] = move(nodeStore);
}

// needs to be locked when called
void RenderNodeGraphManager::PendingDestroy(const RenderHandle handle)
{
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    if (index < nodeGraphData_.size()) {
        const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(handle);
        const uint32_t storedGenerationIdx =
            RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
        // silently ignore if not correct generation (might be destroyed already)
        if (generationIdx == storedGenerationIdx) {
            if (nodeGraphData_[index]) {
#if (RENDER_PERF_ENABLED == 1)
                if (auto* inst =
                        CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(CORE_NS::UID_PERFORMANCE_FACTORY);
                    inst) {
                    if (CORE_NS::IPerformanceDataManager* perfData = inst->Get("RenderNode"); perfData) {
                        for (const auto& rnRef : nodeGraphData_[index]->renderNodeData)
                            perfData->RemoveData(rnRef.fullName);
                    }
                }
#endif
                // destroy all expect RenderNodeContextData which has command buffers and such
                // add to destruction queue
                pendingRenderNodeGraphDestructions_.push_back(PendingRenderNodeGraphDestruction {
                    device_.GetFrameCount(), move(nodeGraphData_[index]->renderNodeContextData) });
                nodeGraphData_[index] = nullptr;
                nodeGraphHandles_[index] = {};
                nodeGraphShareData_[index] = {};

                // NOTE: this does not erase the RenderNodeGraphNodeStore element from the nodeGraphData_
                // i.e. the data is invalidated in nodeGraphData_

                availableHandleIds_.push_back(handle.id);
            }
        }
    } else {
        PLUGIN_LOG_E("invalid handle (%" PRIu64 ") given to render node Destroy", handle.id);
    }
}

// needs to be locked when called
void RenderNodeGraphManager::PendingDeallocRenderNode(
    const RenderHandle nodeGraphHandle, const string_view renderNodeName)
{
    const uint32_t index = RenderHandleUtil::GetIndexPart(nodeGraphHandle);
    if (index < nodeGraphData_.size()) {
        const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandle);
        const uint32_t storedGenerationIdx =
            RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
        // silently ignore if not correct generation (render node graph might be destroyed already)
        if (generationIdx == storedGenerationIdx) {
            PLUGIN_ASSERT(nodeGraphData_[index]);
            auto& nodeStoreRef = *nodeGraphData_[index];
            if (nodeStoreRef.dynamic) {
                uint32_t eraseIndex = ~0u;
                for (size_t eraseIdx = 0; eraseIdx < nodeStoreRef.renderNodeData.size(); ++eraseIdx) {
                    if (nodeStoreRef.renderNodeData[eraseIdx].node &&
                        (nodeStoreRef.renderNodeData[eraseIdx].nodeName == renderNodeName)) {
                        eraseIndex = static_cast<uint32_t>(eraseIdx);
                        break;
                    }
                }
                if (eraseIndex <= nodeStoreRef.renderNodeData.size()) {
#if (RENDER_PERF_ENABLED == 1)
                    if (auto* inst = CORE_NS::GetInstance<CORE_NS::IPerformanceDataManagerFactory>(
                            CORE_NS::UID_PERFORMANCE_FACTORY);
                        inst) {
                        if (CORE_NS::IPerformanceDataManager* perfData = inst->Get("RenderNode"); perfData) {
                            perfData->RemoveData(nodeStoreRef.renderNodeData[eraseIndex].fullName);
                        }
                    }
#endif
                    // only RenderNodeContextData needs deferred destruction
                    pendingRenderNodeDestructions_.push_back({
                        device_.GetFrameCount(),
                        move(nodeStoreRef.renderNodeContextData[eraseIndex]),
                    });
                    // NOTE: this erases the elements from the vectors
                    const int32_t iEraseIndex = static_cast<int32_t>(eraseIndex);
                    nodeStoreRef.renderNodeData.erase(nodeStoreRef.renderNodeData.begin() + iEraseIndex);
                    nodeStoreRef.renderNodeContextData.erase(nodeStoreRef.renderNodeContextData.begin() + iEraseIndex);
                } else {
                    PLUGIN_LOG_E("invalid render node name for erase (%s)", renderNodeName.data());
                }
            } else {
                PLUGIN_LOG_E(
                    "render node (name:%s) cannot be erased from non-dynamic render node graph", renderNodeName.data());
            }
        }
    }
}

// needs to be locked when called
void RenderNodeGraphManager::PendingAllocRenderNode(
    const RenderHandle nodeGraphHandle, const PendingRenderNode& pendingNode)
{
    if (const uint32_t index = RenderHandleUtil::GetIndexPart(nodeGraphHandle); index < nodeGraphData_.size()) {
        const uint32_t genIdx = RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandle);
        const uint32_t sGenIdx = RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
        // silently ignore if not correct generation (render node graph might be destroyed already)
        const auto& desc = pendingNode.renderNodeDesc;
        if (genIdx == sGenIdx) {
            PLUGIN_ASSERT(nodeGraphData_[index]);
            auto& nsRef = *nodeGraphData_[index];
            if (nsRef.dynamic) {
                const RenderDataConstants::RenderDataFixedString combinedNodeName =
                    string_view(nsRef.renderNodeGraphName + desc.nodeName);
                if (auto node = renderNodeMgr_->CreateRenderNode(desc.typeName.c_str()); node) {
                    uint32_t pos = ~0u;
                    if (pendingNode.posType == PendingRenderNode::PosType::BACK) {
                        pos = static_cast<uint32_t>(nsRef.renderNodeData.size());
                    } else {
                        for (uint32_t rnIdx = 0; rnIdx < nsRef.renderNodeData.size(); ++rnIdx) {
                            const auto& currDataRef = nsRef.renderNodeData[rnIdx];
                            if (currDataRef.node && (pendingNode.renderNodeName == currDataRef.nodeName)) {
                                pos = (pendingNode.posType == PendingRenderNode::PosType::AFTER) ? rnIdx + 1 : rnIdx;
                                break;
                            }
                        }
                    }
                    if ((pos <= static_cast<uint32_t>(nsRef.renderNodeData.size())) && (pos != ~0u)) {
                        const RenderNodeManager::RenderNodeTypeInfoFlags tiFlags =
                            renderNodeMgr_->GetRenderNodeTypeInfoFlags(desc.typeName.c_str());
                        const IRenderNode::ClassType backendNode =
                            static_cast<IRenderNode::ClassType>(tiFlags.classType);
                        RenderNodeContextData rncd;
                        rncd.renderBackendNode = (backendNode == IRenderNode::ClassType::CLASS_TYPE_BACKEND_NODE)
                                                     ? reinterpret_cast<IRenderBackendNode*>(node.get())
                                                     : nullptr;
                        rncd.initialized = false;
                        ValidateBackendFlags(desc.typeName, device_.GetBackendType(), tiFlags.backendFlags);
                        nsRef.renderNodeContextData.insert(
                            nsRef.renderNodeContextData.cbegin() + static_cast<ptrdiff_t>(pos), move(rncd));
                        nsRef.renderNodeData.insert(nsRef.renderNodeData.cbegin() + static_cast<ptrdiff_t>(pos),
                            { move(node), desc.typeName, combinedNodeName, desc.nodeName,
                                make_unique<RenderNodeGraphInputs>(desc.description), desc.nodeJson });
                        // new node needs initialization and info on top-level (render node graph)
                        nsRef.initialized = false;
                    } else {
                        PLUGIN_LOG_W("RNT: %s, named: %s, insert pos NF", desc.typeName.c_str(), desc.nodeName.c_str());
                    }
                } else {
                    PLUGIN_LOG_W("RN type: %s, named: %s, not found", desc.typeName.c_str(), desc.nodeName.c_str());
                }
            } else {
                PLUGIN_LOG_E("RN (name:%s) cannot be add to non-dynamic render node graph", desc.nodeName.c_str());
            }
        }
    }
}

void RenderNodeGraphManager::UpdateRenderNodeGraphResources()
{
    for (size_t index = 0; index < nodeGraphShareData_.size(); ++index) {
        PLUGIN_ASSERT(nodeGraphData_.size() == nodeGraphShareData_.size());
        auto& srcData = nodeGraphShareData_[index];
        if (nodeGraphData_[index] && ((srcData.inputCount > 0) || (srcData.outputCount > 0))) {
            auto& ngd = *nodeGraphData_[index];
            auto& dstData = ngd.renderNodeGraphShareData;
            dstData = {};
            dstData.inputCount = srcData.inputCount;
            dstData.outputCount = srcData.outputCount;
            for (uint32_t idx = 0; idx < dstData.inputCount; ++idx) {
                dstData.inputs[idx] = { "", srcData.inputs[idx].GetHandle() };
            }
            for (uint32_t idx = 0; idx < dstData.outputCount; ++idx) {
                dstData.outputs[idx] = { "", srcData.outputs[idx].GetHandle() };
            }
        }
    }
}

void RenderNodeGraphManager::DynamicRenderNodeOpImpl(const RenderHandle handle, const RenderNodeDesc& renderNodeDesc,
    const PendingRenderNode::Type type, const PendingRenderNode::PosType posType, const string_view renderNodeName)
{
    if (RenderHandleUtil::GetHandleType(handle) == RenderHandleType::RENDER_NODE_GRAPH) {
        const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
        const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(handle);
        const auto lock = std::lock_guard(mutex_);

        if (index < static_cast<uint32_t>(nodeGraphHandles_.size())) {
            const uint32_t storedGenerationIdx =
                RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
            // silently ignored if not current generation
            if (generationIdx == storedGenerationIdx) {
                pendingRenderNodes_.push_back({ type, handle, renderNodeDesc, posType, renderNodeName });
            }
        }
    } else {
        PLUGIN_LOG_E("invalid render node graph handle for dynamic render node operation");
    }
}

void RenderNodeGraphManager::PushBackRenderNode(
    const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc)
{
    const RenderHandle rawHandle = handle.GetHandle();
    DynamicRenderNodeOpImpl(
        rawHandle, renderNodeDesc, PendingRenderNode::Type::ALLOC, PendingRenderNode::PosType::BACK, {});
}

void RenderNodeGraphManager::InsertBeforeRenderNode(
    const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc, const string_view renderNodeName)
{
    const RenderHandle rawHandle = handle.GetHandle();
    DynamicRenderNodeOpImpl(
        rawHandle, renderNodeDesc, PendingRenderNode::Type::ALLOC, PendingRenderNode::PosType::BEFORE, renderNodeName);
}

void RenderNodeGraphManager::InsertAfterRenderNode(
    const RenderHandleReference& handle, const RenderNodeDesc& renderNodeDesc, const string_view renderNodeName)
{
    const RenderHandle rawHandle = handle.GetHandle();
    DynamicRenderNodeOpImpl(
        rawHandle, renderNodeDesc, PendingRenderNode::Type::ALLOC, PendingRenderNode::PosType::AFTER, renderNodeName);
}

void RenderNodeGraphManager::EraseRenderNode(const RenderHandleReference& handle, const string_view renderNodeName)
{
    const RenderHandle rawHandle = handle.GetHandle();
    RenderNodeDesc desc;
    desc.nodeName = renderNodeName;
    DynamicRenderNodeOpImpl(
        rawHandle, desc, PendingRenderNode::Type::DEALLOC, PendingRenderNode::PosType::BACK, renderNodeName);
}

void RenderNodeGraphManager::UpdateRenderNodeGraph(const RenderHandleReference&, const RenderNodeGraphDescInfo&)
{
    // NOTE: not yet implemented
    // will provide flags for e.g. disabling render nodes
}

RenderNodeGraphDescInfo RenderNodeGraphManager::GetInfo(const RenderHandleReference& handle) const
{
    const RenderHandle rawHandle = handle.GetHandle();
    RenderHandleType const type = RenderHandleUtil::GetHandleType(rawHandle);
    if (RenderHandleType::RENDER_NODE_GRAPH != type) {
        return {};
    }

    {
        uint32_t const index = RenderHandleUtil::GetIndexPart(rawHandle);
        const auto lock = std::lock_guard(mutex_);

        if (index < nodeGraphData_.size()) {
            const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(rawHandle);
            const uint32_t storedGenerationIdx =
                RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
            if (generationIdx == storedGenerationIdx) {
                if (nodeGraphData_[index]) {
                    const auto& ngd = *nodeGraphData_[index];
                    RenderNodeGraphDescInfo rngdi;
                    rngdi.renderNodeGraphName = ngd.renderNodeGraphName;
                    rngdi.renderNodeGraphDataStoreName = ngd.renderNodeGraphDataStoreName;
                    rngdi.renderNodeGraphUri = ngd.renderNodeGraphUri;
                    rngdi.nodes.resize(ngd.renderNodeData.size());
                    for (size_t nodeIdx = 0; nodeIdx < ngd.renderNodeData.size(); ++nodeIdx) {
                        if (ngd.renderNodeData[nodeIdx].node) {
                            rngdi.nodes[nodeIdx].typeName = ngd.renderNodeData[nodeIdx].typeName;
                            rngdi.nodes[nodeIdx].nodeName = ngd.renderNodeData[nodeIdx].nodeName;
                            rngdi.nodes[nodeIdx].flags = 0u; // NOTE: not yet used
                        }
                    }
                    return rngdi;
                }
            } else {
                PLUGIN_LOG_E("invalid generation index (%u != %u) with render node graph handle (%" PRIu64 ")",
                    generationIdx, storedGenerationIdx, rawHandle.id);
            }
        }
    }
    return {};
}

void RenderNodeGraphManager::SetRenderNodeGraphResources(const RenderHandleReference& handle,
    const array_view<const RenderHandleReference> inputs, const array_view<const RenderHandleReference> outputs)
{
    const RenderHandle rawHandle = handle.GetHandle();
    if (RenderHandleType::RENDER_NODE_GRAPH != RenderHandleUtil::GetHandleType(rawHandle)) {
        return;
    }

    uint32_t const index = RenderHandleUtil::GetIndexPart(rawHandle);
    const auto lock = std::lock_guard(mutex_);

    // NOTE: should lock and handle/touch only the client side data
    if (index < nodeGraphShareData_.size()) {
        const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(rawHandle);
        const uint32_t storedGenerationIdx =
            RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
        if (generationIdx == storedGenerationIdx) {
#if (RENDER_VALIDATION_ENABLED == 1)
            if (inputs.size() > RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT) {
                PLUGIN_LOG_W("RENDER_VALIDATION: render node graph resource input count (%u) exceeds limit (%u)",
                    static_cast<uint32_t>(inputs.size()), RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT);
            }
            if (outputs.size() > RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT) {
                PLUGIN_LOG_W("RENDER_VALIDATION: render node graph resource output count (%u) exceeds limit (%u)",
                    static_cast<uint32_t>(outputs.size()), RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT);
            }
#endif
            auto& clientData = nodeGraphShareData_[index];
            clientData = {};
            clientData.inputCount = Math::min(
                RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT, static_cast<uint32_t>(inputs.size()));
            clientData.outputCount = Math::min(
                RenderNodeGraphShareData::MAX_RENDER_NODE_GRAPH_RES_COUNT, static_cast<uint32_t>(outputs.size()));
            for (uint32_t idx = 0; idx < clientData.inputCount; ++idx) {
                clientData.inputs[idx] = inputs[idx];
#if (RENDER_VALIDATION_ENABLED == 1)
                if (!inputs[idx]) {
                    PLUGIN_LOG_W("RENDER_VALIDATION: inv input handle (idx:%u) given as input to RNG", idx);
                }
#endif
            }
            for (uint32_t idx = 0; idx < clientData.outputCount; ++idx) {
                clientData.outputs[idx] = outputs[idx];
#if (RENDER_VALIDATION_ENABLED == 1)
                if (!outputs[idx]) {
                    PLUGIN_LOG_W("RENDER_VALIDATION: inv output handle (idx:%u) given as output to RNG", idx);
                }
#endif
            }
        } else {
            PLUGIN_LOG_E("invalid generation index (%u != %u) with render node graph handle (%" PRIu64 ")",
                generationIdx, storedGenerationIdx, rawHandle.id);
        }
    }
}

RenderNodeGraphResourceInfo RenderNodeGraphManager::GetRenderNodeGraphResources(
    const RenderHandleReference& handle) const
{
    const RenderHandle rawHandle = handle.GetHandle();
    RenderHandleType const type = RenderHandleUtil::GetHandleType(rawHandle);
    if (RenderHandleType::RENDER_NODE_GRAPH != type) {
        return {};
    }
    RenderNodeGraphResourceInfo info;
    {
        uint32_t const index = RenderHandleUtil::GetIndexPart(rawHandle);
        const auto lock = std::lock_guard(mutex_);

        // NOTE: should lock and handle/touch only the client side data
        if (index < nodeGraphShareData_.size()) {
            const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(rawHandle);
            const uint32_t storedGenerationIdx =
                RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
            if (generationIdx == storedGenerationIdx) {
                if (nodeGraphData_[index]) {
                    const auto& clientData = nodeGraphShareData_[index];
                    for (uint32_t idx = 0; idx < clientData.inputCount; ++idx) {
                        info.inputResources.push_back(clientData.inputs[idx]);
                    }
                    for (uint32_t idx = 0; idx < clientData.outputCount; ++idx) {
                        info.outputResources.push_back(clientData.outputs[idx]);
                    }
                }
            } else {
                PLUGIN_LOG_E("invalid generation index (%u != %u) with render node graph handle (%" PRIu64 ")",
                    generationIdx, storedGenerationIdx, rawHandle.id);
            }
        }
    }
    return info;
}

RenderNodeGraphNodeStore* RenderNodeGraphManager::Get(RenderHandle handle)
{
    RenderHandleType const type = RenderHandleUtil::GetHandleType(handle);
    if (RenderHandleType::RENDER_NODE_GRAPH != type) {
        return nullptr;
    }

    {
        uint32_t const index = RenderHandleUtil::GetIndexPart(handle);
        const auto lock = std::lock_guard(mutex_);

        if (index < nodeGraphData_.size()) {
            const uint32_t generationIdx = RenderHandleUtil::GetGenerationIndexPart(handle);
            const uint32_t storedGenerationIdx =
                RenderHandleUtil::GetGenerationIndexPart(nodeGraphHandles_[index].GetHandle());
            if (generationIdx == storedGenerationIdx) {
                return nodeGraphData_[index].get();
            } else {
                PLUGIN_LOG_E("invalid generation index (%u != %u) with render node graph handle (%" PRIu64 ")",
                    generationIdx, storedGenerationIdx, handle.id);
            }
        }
    }

    return nullptr;
}

RenderNodeManager& RenderNodeGraphManager::GetRenderNodeManager() const
{
    return *renderNodeMgr_;
}

void RenderNodeGraphManager::Destroy(const string_view typeName)
{
    device_.Activate();
    for (const auto& store : nodeGraphData_) {
        for (;;) {
            auto pos = std::find_if(store->renderNodeData.begin(), store->renderNodeData.end(),
                [typeName](const RenderNodeGraphNodeStore::RenderNodeData& data) { return data.typeName == typeName; });
            if (pos != store->renderNodeData.end()) {
                const auto index = std::distance(store->renderNodeData.begin(), pos);
                store->renderNodeData.erase(pos);
                store->renderNodeContextData.erase(store->renderNodeContextData.begin() + index);
            } else {
                break;
            }
        }
    }
    device_.Deactivate();
}
RENDER_END_NAMESPACE()
