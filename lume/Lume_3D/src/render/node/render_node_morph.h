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

#ifndef CORE__RENDER__NODE__RENDER_NODE_MORPH_H
#define CORE__RENDER__NODE__RENDER_NODE_MORPH_H

#include <3d/render/intf_render_data_store_morph.h>
#include <base/containers/array_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class RenderNodeMorph final : public RENDER_NS::IRenderNode {
public:
    RenderNodeMorph() = default;
    ~RenderNodeMorph() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "8e5bd3ad-f95b-4c47-9cf2-a9d908aa7c72" };
    static constexpr char const* const TYPE_NAME = "RenderNodeMorph";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    void UpdateWeightsAndTargets(BASE_NS::array_view<const RenderDataMorph::Submesh> submeshes);
    void ComputeMorphs(
        RENDER_NS::IRenderCommandList& cmdList, BASE_NS::array_view<const RenderDataMorph::Submesh> submeshes);

    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    SceneRenderDataStores stores_;

    uint32_t maxObjectCount_ { 0 };
    uint32_t maxStructCount_ { 0 };
    uint32_t bufferSize_ { 0 };
    RENDER_NS::RenderHandleReference morphTargetBufferHandle_;

    RENDER_NS::RenderHandle psoHandle_;

    RENDER_NS::PipelineLayout pipelineLayout_;
    RENDER_NS::ShaderThreadGroup threadGroupSize_ { 1u, 1u, 1u };

    struct AllDescriptorSets {
        RENDER_NS::IDescriptorSetBinder::Ptr params;
        BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> inputs;
        BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> outputs;
    };
    AllDescriptorSets allDescriptorSets_;

    bool hasExecuteData_ { false };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_MORPH_H
