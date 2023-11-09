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

#ifndef RENDER_RENDER__NODE__RENDER_NODE_COMBINED_POST_PROCESS_H
#define RENDER_RENDER__NODE__RENDER_NODE_COMBINED_POST_PROCESS_H

#include <base/util/uid.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "node/render_bloom.h"
#include "node/render_blur.h"

RENDER_BEGIN_NAMESPACE()
class IGpuResourceManager;
class IRenderCommandList;
class IRenderNodeContextManager;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderNodeCombinedPostProcess final : public IRenderNode {
public:
    RenderNodeCombinedPostProcess() = default;
    ~RenderNodeCombinedPostProcess() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "82fbcb54-5762-499f-a039-b3862b316f33" };
    static constexpr char const* TYPE_NAME = "CORE_RN_COMBINED_POST_PROCESS";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();
    void UpdateImageData();

    void InitCreateShaderResources();
    void InitCreateBinders();

    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::InputResources resources;
        bool hasChangeableResourceHandles { false };

        uint32_t colorIndex { ~0u };
        uint32_t depthIndex { ~0u };
        uint32_t velocityIndex { ~0u };
        uint32_t historyIndex { ~0u };
        uint32_t historyNextIndex { ~0u };
    };
    JsonInputs jsonInputs_;
    RenderNodeHandles::InputResources inputResources_;
    RenderNodeGraphInputs::RenderDataStore renderDataStore_;

    struct PsoSpecializationData {
        TonemapConfiguration::TonemapType tonemapType {};
        bool enableVignette { false };
        bool enableColorFringe { false };
        bool enableTonemap { false };
    };
    void CheckForPsoSpecilization();
    void UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration);
    void ProcessPostProcessConfiguration();
    void ExecuteCombine(IRenderCommandList& cmdList);
    void ExecuteFXAA(IRenderCommandList& cmdList);
    void ExecuteBlit(IRenderCommandList& cmdList);
    void ExecuteTAA(IRenderCommandList& cmdList);

    struct EffectData {
        RenderHandle shader;
        RenderHandle pl;
        RenderHandle pso;

        PipelineLayout pipelineLayout;
    };

    RenderHandle psoHandle_;
    RenderHandle copyPsoHandle_;
    RenderPass renderPass_;

    struct ConfigurationNames {
        BASE_NS::string renderDataStoreName;
        BASE_NS::string postProcessConfigurationName;
    };
    ConfigurationNames configurationNames_;

    PipelineLayout pipelineLayout_;
    PipelineLayout copyPipelineLayout_;

    struct AllBinders {
        // main, taa, fxaa
        static constexpr uint32_t GLOBAL_SET0_COUNT { 3u };

        IDescriptorSetBinder::Ptr globalSet0[GLOBAL_SET0_COUNT];
        uint32_t globalSetIndex { 0u }; // needs to be reset every frame

        IDescriptorSetBinder::Ptr mainBinder;
        IDescriptorSetBinder::Ptr taaBinder;
        IDescriptorSetBinder::Ptr fxaaBinder;

        IDescriptorSetBinder::Ptr copyBinder;
    };
    AllBinders binders_;

    RenderHandleReference intermediateImage_;

    RenderHandle combinedShader_;
    RenderHandle copyShader_;
    RenderHandle sampler_;
    PostProcessConfiguration ppConfig_;

    struct ImageData {
        // input and output as in render node graph
        RenderHandle globalInput;
        RenderHandle globalOutput;

        // input and output used in post processes
        // TAA will set the input to post processes to be historyOutput
        RenderHandle input;
        RenderHandle output;
        // TAA related
        RenderHandle history;
        RenderHandle historyNext;

        RenderHandle depth;
        // Required by TAA
        RenderHandle velocity;

        RenderHandle dirtMask;
        RenderHandle black;
        RenderHandle white;
    };
    ImageData images_;

    EffectData fxaaData_;
    EffectData taaData_;

    RenderBloom renderBloom_;
    RenderBlur renderBlur_;

    struct UboHandles {
        // first 256 aligned is global post process
        // after (256) we have effect local data
        RenderHandleReference postProcess;

        uint32_t taaIndex { 1u };
        uint32_t bloomIndex { 2u };
        uint32_t blurIndex { 3u };
        uint32_t fxaaIndex { 4u };
        uint32_t combinedIndex { 5u };
    };
    UboHandles ubos_;

    BASE_NS::Math::Vec4 fxaaTargetSize_ { 0.0f, 0.0f, 0.0f, 0.0f };

    bool validInputs_ { true };
    bool validInputsForTaa_ { false };
};
RENDER_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_COMBINED_POST_PROCESS_H
