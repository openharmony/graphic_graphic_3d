/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_RENDER__NODE__RENDER_NODE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
#define RENDER_RENDER__NODE__RENDER_NODE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H

#include <base/containers/string.h>
#include <base/util/uid.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
struct RenderNodeGraphInputs;

class RenderDataStoreDefaultAccelerationStructureStaging;

class RenderNodeDefaultAccelerationStructureStaging final : public IRenderNode {
public:
    RenderNodeDefaultAccelerationStructureStaging() = default;
    ~RenderNodeDefaultAccelerationStructureStaging() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "377f5d6c-3951-4a0b-9d5b-7ef8d5e6f235" };
    static constexpr char const* TYPE_NAME = "CORE_RN_DEFAULT_ACCELERATION_STRUCTURE_STAGING";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS =
        IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_EXPLICIT_VULKAN;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    void ExecuteFrameProcessInstanceData(RenderDataStoreDefaultAccelerationStructureStaging* dataStore);

    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    BASE_NS::string dsName_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_RENDER__NODE__RENDER_NODE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
