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

#ifndef RENDER_NODE_DOTFIELD_SIMULATION_H
#define RENDER_NODE_DOTFIELD_SIMULATION_H
#include <3d/render/intf_render_node_scene_util.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/device/pipeline_layout_desc.h>
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
class RenderNodeDotfieldSimulation final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDotfieldSimulation() = default;
    ~RenderNodeDotfieldSimulation() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }
    static constexpr BASE_NS::Uid UID { "c82a51c3-1be3-4479-b867-bd92a2a9e98b" };
    static constexpr char const* TYPE_NAME = "RenderNodeDotfieldSimulation";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct Binders {
        struct SimulateBinders {
            RENDER_NS::IDescriptorSetBinder::Ptr argsBuffersSet0;
            BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> prevBuffersSet1;
            BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> currBuffersSet2;
            RENDER_NS::PipelineLayout pl;
        };
        SimulateBinders simulate;
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };
    CORE3D_NS::SceneRenderDataStores stores_;

    struct PsoData {
        RENDER_NS::RenderHandle psoHandle;
        RENDER_NS::PushConstant pushConstant;
    };

    void ComputeSimulate(
        RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, RENDER_NS::IRenderCommandList& cmdList);

    PsoData GetPsoData(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const RENDER_NS::PipelineLayout& pl,
        const RENDER_NS::RenderHandle& shaderHandle, bool firstFrame);

    BASE_NS::unordered_map<uint32_t, PsoData> specializationToPsoData_;
    Binders binders_;
    RENDER_NS::RenderHandle shaderHandle_;
    bool firstFrame_ { true };
};
} // namespace Dotfield

#endif // RENDER_NODE_DOTFIELD_SIMULATION_H
