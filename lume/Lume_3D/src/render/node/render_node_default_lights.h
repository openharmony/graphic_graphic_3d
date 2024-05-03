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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_LIGHTS_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_LIGHTS_H

#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class RenderNodeDefaultLights final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultLights() = default;
    ~RenderNodeDefaultLights() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        // expect to have lights or need to reset GPU data
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "8757af6a-adde-471f-b475-8f8c0962d8b6" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultLights";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    SceneRenderDataStores stores_;

    RENDER_NS::RenderHandleReference lightBufferHandle_;
    RENDER_NS::RenderHandleReference lightClusterBufferHandle_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_LIGHTS_H
