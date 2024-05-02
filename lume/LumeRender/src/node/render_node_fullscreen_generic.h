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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_FULLSCREEN_GENERIC_H
#define RENDER_RENDER__NODE__RENDER_NODE_FULLSCREEN_GENERIC_H

#include <base/util/uid.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
struct RenderNodeGraphInputs;

class RenderNodeFullscreenGeneric final : public IRenderNode {
public:
    RenderNodeFullscreenGeneric() = default;
    ~RenderNodeFullscreenGeneric() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "ea17a490-c3b7-453a-851f-79a20e324159" };
    static constexpr char const* TYPE_NAME = "RenderNodeFullscreenGeneric";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();
    RenderHandle GetPsoHandle();

    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::InputRenderPass renderPass;
        RenderNodeGraphInputs::InputResources resources;

        RenderNodeGraphInputs::RenderDataStore renderDataStore;
        RenderNodeGraphInputs::RenderDataStore renderDataStoreSpecialization;

        bool hasChangeableRenderPassHandles { false };
        bool hasChangeableResourceHandles { false };
    };
    JsonInputs jsonInputs_;

    RenderNodeHandles::InputRenderPass inputRenderPass_;
    RenderNodeHandles::InputResources inputResources_;
    struct PipelineData {
        RenderHandle shader;
        RenderHandle graphicsState;
        RenderHandle pso;
        RenderHandle pipelineLayout;

        PipelineLayout pipelineLayoutData;
    };
    PipelineData pipelineData_;

    // data store push constant
    bool useDataStorePushConstant_ { false };

    // data store shader specialization
    bool useDataStoreShaderSpecialization_ { false };
    struct ShaderSpecilizationData {
        BASE_NS::vector<ShaderSpecialization::Constant> constants;
        BASE_NS::vector<uint32_t> data;
    };
    ShaderSpecilizationData shaderSpecializationData_;

    IPipelineDescriptorSetBinder::Ptr pipelineDescriptorSetBinder_;
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_FULLSCREEN_GENERIC_H
