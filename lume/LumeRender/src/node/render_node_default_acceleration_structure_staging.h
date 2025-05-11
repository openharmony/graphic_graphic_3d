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

#ifndef RENDER_NODE_RENDER_NODE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
#define RENDER_NODE_RENDER_NODE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H

#include <base/containers/string.h>
#include <base/util/uid.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

#include "datastore/render_data_store_default_acceleration_structure_staging.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
struct RenderNodeGraphInputs;

class RenderNodeDefaultAccelerationStructureStaging final : public IRenderNode {
public:
    RenderNodeDefaultAccelerationStructureStaging() = default;
    ~RenderNodeDefaultAccelerationStructureStaging() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "377f5d6c-3951-4a0b-9d5b-7ef8d5e6f235" };
    static constexpr const char* TYPE_NAME = "CORE_RN_DEFAULT_ACCELERATION_STRUCTURE_STAGING";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS =
        IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_EXPLICIT_VULKAN;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    void ExecuteFrameProcessInstanceData(
        IRenderCommandList& cmdList, const AsConsumeStruct& fullData, const AsConsumeStruct::AsData& data);
    void ExecuteFrameProcessGeometryData(
        IRenderCommandList& cmdList, const AsConsumeStruct& fullData, const AsConsumeStruct::AsData& data);
    void ExecuteFrameProcessScratch(const AsConsumeStruct& fullData);

    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    BASE_NS::string dsName_;

    // helper vector
    BASE_NS::vector<AsInstance> asInstanceHelper_;
    BASE_NS::vector<uint32_t> scratchOffsetHelper_;
    uint32_t frameScratchOffsetIndex_ { 0U };

    RenderHandleReference scratchBuffer_;
    RenderHandle rawScratchBuffer_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_NODE_RENDER_NODE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
