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

#ifndef RENDER_NODE_TEST_UTIL_H
#define RENDER_NODE_TEST_UTIL_H

#include <base/util/uid.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
class IRenderNodeContextManager;
class IPipelineDescriptorSetBinder;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderNodeTestUtil final : public IRenderNode {
public:
    RenderNodeTestUtil() = default;
    ~RenderNodeTestUtil() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "cd639ac6-f880-4a00-9f29-4dc90e9dc1e2" };
    static constexpr const  char* TYPE_NAME = "RenderNodeTestUtil";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();

    RenderNodeGraphInputs::InputRenderPass inputRenderPassJson_;
    RenderNodeHandles::InputRenderPass inputRenderPass_;

    bool valid_ { false };
};
RENDER_END_NAMESPACE()

#endif // RENDER_NODE_TEST_UTIL_H