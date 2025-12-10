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

#ifndef RENDER_NODE_RENDER_NODE_QUEUE_TRANSFER_H
#define RENDER_NODE_RENDER_NODE_QUEUE_TRANSFER_H

#include <base/util/uid.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

#include "device/gpu_resource_manager.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
class IRenderNodeGpuResourceManager;
struct RenderNodeGraphInputs;

// This node is no-op used for multi queue resource ownership transfers. We need a command list to make release a
// resource on the correct, queue so this node's command list is used for releasing resources.
class RenderNodeQueueTransfer final : public IRenderNode {
public:
    RenderNodeQueueTransfer() = default;
    ~RenderNodeQueueTransfer() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "22a56820-befb-4e12-83ee-3cad3f685b0a" };
    static constexpr const char* TYPE_NAME = "CORE_RN_QUEUE_TRANSFER";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
};
RENDER_END_NAMESPACE()

#endif // RENDER_NODE_RENDER_NODE_QUEUE_TRANSFER_H
