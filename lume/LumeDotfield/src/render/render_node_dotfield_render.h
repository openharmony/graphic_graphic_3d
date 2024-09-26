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

#ifndef RENDER_NODE_DOTFIELD_RENDER_H
#define RENDER_NODE_DOTFIELD_RENDER_H
#include <3d/render/intf_render_node_scene_util.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

namespace RENDER_NS {
class IRenderCommandList;
class IRenderNodeContextManager;
class IPipelineDescriptorSetBinder;
} // namespace RENDER_NS

namespace Dotfield {
class RenderNodeDotfieldRender final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDotfieldRender() = default;
    ~RenderNodeDotfieldRender() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }
    
    static constexpr BASE_NS::Uid UID { "4ce00e60-68c9-46c8-aea8-87c4e7994d27" };
    static constexpr char const* TYPE_NAME = "RenderNodeDotfieldRender";
    static constexpr RENDER_NS::IRenderNode::BackendFlags BACKEND_FLAGS =
        RENDER_NS::IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr RENDER_NS::IRenderNode::ClassType CLASS_TYPE = RENDER_NS::IRenderNode::ClassType::CLASS_TYPE_NODE;
    static RENDER_NS::IRenderNode* Create();
    static void Destroy(RENDER_NS::IRenderNode* instance);

private:
    void ParseRenderNodeInputs();

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    CORE3D_NS::SceneRenderDataStores stores_;

    struct PsoData {
        RENDER_NS::RenderHandle psoHandle;
        RENDER_NS::PushConstant pushConstant;
    };

    struct RenderBinders {
        RENDER_NS::IDescriptorSetBinder::Ptr set0;
        RENDER_NS::IDescriptorSetBinder::Ptr set1;
    };

    void RenderData(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, RENDER_NS::IRenderCommandList& cmdList);
    PsoData GetPsoData(
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const RENDER_NS::RenderHandle& shaderHandle);

    BASE_NS::unordered_map<uint64_t, PsoData> shaderToPsoData_;
    RenderBinders binders_;
    RENDER_NS::RenderNodeHandles::InputRenderPass inputRenderPass_;
    RENDER_NS::RenderPass renderPass_;
    RENDER_NS::RenderHandle cameraUniformBufferHandle_;
    RENDER_NS::RenderHandle shaderHandle_;
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::InputRenderPass renderPass;
    };
    JsonInputs jsonInputs_;
};
} // namespace Dotfield

#endif // RENDER_NODE_DOTFIELD_SIMULATION_H
