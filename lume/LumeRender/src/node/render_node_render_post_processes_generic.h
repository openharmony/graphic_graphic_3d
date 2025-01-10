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

#ifndef RENDER_NODE_RENDER_NODE_RENDER_POST_PROCESSES_GENERIC_H
#define RENDER_NODE_RENDER_NODE_RENDER_POST_PROCESSES_GENERIC_H

#include <base/containers/string.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_render_post_processes.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_post_process_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "nodecontext/render_node_copy_util.h"

RENDER_BEGIN_NAMESPACE()
class IGpuResourceManager;
class IRenderCommandList;
class IRenderNodeContextManager;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderNodeRenderPostProcessesGeneric final : public IRenderNode {
public:
    RenderNodeRenderPostProcessesGeneric() = default;
    ~RenderNodeRenderPostProcessesGeneric() override = default;

    void InitNode(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "0e82f5f1-9122-40f2-bd5e-5e43728417ab" };
    static constexpr const char* TYPE_NAME = "RenderNodeRenderPostProcessesGeneric";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();
    void UpdateImageData();
    void ProcessPostProcessConfiguration();
    void RegisterOutputs();

    enum class DefaultInOutImage : uint32_t {
        OUTPUT = 0,
        INPUT_OUTPUT_COPY = 1,
        INPUT = 2,
        WHITE = 3,
        BLACK = 4,
    };
    // Json resources which might need re-fetching
    struct JsonInputs {
        RenderNodeGraphInputs::InputResources resources;
        RenderNodeGraphInputs::RenderDataStore renderDataStore;

        DefaultInOutImage defaultOutputImage { DefaultInOutImage::OUTPUT };
        DefaultInOutImage defaultInputImage { DefaultInOutImage::INPUT };

        uint32_t inputIdx { ~0u };
        uint32_t outputIdx { ~0u };
    };
    JsonInputs jsonInputs_;

    struct BuiltInVariables {
        RenderHandle input;
        RenderHandle output;

        RenderHandle defInput;
        RenderHandle defOutput;

        RenderHandle defBlackImage;
        RenderHandle defWhiteImage;
        RenderHandle defSampler;
    };
    BuiltInVariables builtInVariables_;

    RenderNodeHandles::InputResources inputResources_;
    RenderNodeCopyUtil renderCopy_;
    bool copyInitNeeded_ { true };

    struct PostProcessPipelineNode {
        uint64_t id { ~0ULL };
        IRenderPostProcessNode::Ptr ppNode;
    };
    struct AllPostProcesses {
        // instanciated when there are new post processes set to the pipeline
        BASE_NS::vector<PostProcessPipelineNode> postProcessNodeInstances;
        // all post processes, fetched every frame
        IRenderDataStoreRenderPostProcesses::PostProcessPipeline pipeline;

        // keep track for new post processes
        bool newPostProcesses { false };
        uint32_t postProcessCount { 0U };
    };
    AllPostProcesses allPostProcesses_;
    bool valid_ { false };
};
RENDER_END_NAMESPACE()

#endif // RENDER_NODE_RENDER_NODE_RENDER_POST_PROCESSES_GENERIC_H
