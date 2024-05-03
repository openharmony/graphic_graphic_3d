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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_MIP_CHAIN_GENERIC_H
#define RENDER_RENDER__NODE__RENDER_NODE_MIP_CHAIN_GENERIC_H

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/util/formats.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/resource_handle.h>

#include "node/render_copy.h"

RENDER_BEGIN_NAMESPACE()
class IRenderCommandList;
struct RenderNodeGraphRenderNodeSetupData;

class RenderNodeMipChainPostProcess final : public IRenderNode {
public:
    RenderNodeMipChainPostProcess() = default;
    ~RenderNodeMipChainPostProcess() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "1d891c63-28bb-44c4-98b4-64959419c9b1" };
    static constexpr char const* TYPE_NAME = "CORE_RN_MIP_CHAIN_POST_PROCESS";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    void ParseRenderNodeInputs();
    void ProcessPostProcessConfiguration();
    void UpdateGlobalPostProcessUbo();
    RenderPass GetBaseRenderPass();
    void RenderGraphics(IRenderCommandList& cmdList);
    void RenderCompute(IRenderCommandList& cmdList);
    void BindDefaultResources(const uint32_t set, const DescriptorSetLayoutBindingResources& bindings);
    void RegisterOutputs(const RenderHandle output);
    void UpdateImageData();
    void UpdateGlobalSet(IRenderCommandList& cmdList);
    bool GetRequiresPreCopy() const;

    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    enum class DefaultOutputImage : uint32_t {
        OUTPUT = 0,
        INPUT_OUTPUT_COPY = 1,
        INPUT = 2,
        WHITE = 3,
        BLACK = 4,
    };
    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::InputRenderPass renderPass;

        RenderNodeGraphInputs::InputResources resources;
        RenderNodeGraphInputs::InputResources dispatchResources;

        RenderNodeGraphInputs::RenderDataStore renderDataStore;
        RenderNodeGraphInputs::RenderDataStore renderDataStoreSpecialization;

        BASE_NS::string ppName;
        DefaultOutputImage defaultOutputImage { DefaultOutputImage::OUTPUT };

        bool upRamp { false };
        bool hasChangeableRenderPassHandles { false };
        bool hasChangeableResourceHandles { false };
        bool hasChangeableDispatchHandles { false };

        uint32_t inputIdx { ~0u };
        uint32_t outputIdx { ~0u };
    };
    JsonInputs jsonInputs_;

    struct BuiltInVariables {
        RenderHandle input;

        RenderHandle output;

        RenderHandle defBuffer;
        RenderHandle defBlackImage;
        RenderHandle defWhiteImage;
        RenderHandle defSampler;

        // the flag for the post built-in post process
        uint32_t postProcessFlag { 0u };
    };
    BuiltInVariables builtInVariables_;

    RenderNodeHandles::InputRenderPass inputRenderPass_;
    RenderNodeHandles::InputResources inputResources_;
    RenderNodeHandles::InputResources dispatchResources_;

    struct PipelineData {
        RenderHandle shader;
        RenderHandle graphicsState;
        RenderHandle pso;
        RenderHandle pipelineLayout;

        PipelineLayout pipelineLayoutData;

        ShaderThreadGroup threadGroupSize { 1U, 1U, 1U };
        bool graphics { true };
    };
    PipelineData pipelineData_;

    PostProcessConfiguration ppGlobalConfig_;
    IRenderDataStorePostProcess::PostProcess ppLocalConfig_;

    struct UboHandles {
        // first 512 aligned is global post process
        // after (256) we have effect local data
        RenderHandleReference postProcess;

        uint32_t effectLocalDataIndex { 1u };
    };
    UboHandles ubos_;

    RenderCopy renderCopy_;
    bool useAutoBindSet0_ { false };
    bool valid_ { false };

    IDescriptorSetBinder::Ptr globalSet0_;
    BASE_NS::vector<IDescriptorSetBinder::Ptr> binders_;
    RenderHandle samplerHandle_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_RENDER__NODE__RENDER_NODE_MIP_CHAIN_GENERIC_H
