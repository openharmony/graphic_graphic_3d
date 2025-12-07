/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef RENDER_NODECONTEXT_RENDER_NODE_POST_PROCESS_INTERFACE_UTIL_H
#define RENDER_NODECONTEXT_RENDER_NODE_POST_PROCESS_INTERFACE_UTIL_H

#include <cstdint>

#include <base/containers/string.h>
#include <render/datastore/intf_render_data_store_render_post_processes.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_post_process_interface_util.h>
#include <render/nodecontext/intf_render_post_process.h>
#include <render/nodecontext/intf_render_post_process_node.h>
#include <render/render_data_structures.h>

#include "nodecontext/render_node_copy_util.h"

RENDER_BEGIN_NAMESPACE()
class IGpuResourceManager;
class IRenderCommandList;
class IRenderNodeContextManager;
class IRenderNodeRenderDataStoreManager;
struct RenderNodeGraphInputs;

class RenderNodePostProcessInterfaceUtil final {
public:
    RenderNodePostProcessInterfaceUtil() = default;
    ~RenderNodePostProcessInterfaceUtil() = default;

    void Init(IRenderNodeContextManager& renderNodeContextMgr);
    void PreExecuteFrame();
    void ExecuteFrame(IRenderCommandList& cmdList);
    IRenderNode::ExecuteFlags GetExecuteFlags() const;

    void SetInfo(const IRenderNodePostProcessInterfaceUtil::Info& info);

private:
    IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();
    void UpdateImageData();
    void ProcessPostProcessConfiguration();
    void RegisterOutputs();
    void PrepareIntermediateTargets(RenderHandle input, RenderHandle output);

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
        uint32_t depthIndex { ~0u };
        uint32_t velocityIndex { ~0u };
        uint32_t historyIndex { ~0u };
        uint32_t historyNextIndex { ~0u };

        uint32_t outputIdx { ~0u };

        uint32_t cameraIdx { ~0u };
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

        RenderHandle depth;
        RenderHandle velocity;
        RenderHandle history;
        RenderHandle historyNext;

        RenderHandle camera;
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
        IRenderDataStoreRenderPostProcesses::PostProcessPipeline prevPipeline;

        // keep track for new post processes
        bool newPostProcesses { false };
    };
    AllPostProcesses allPostProcesses_;
    bool valid_ { false };

    BASE_NS::Math::UVec2 outputSize_ { 0U, 0U };
    struct TemporaryImages {
        uint32_t idx = 0U;
        uint32_t imageCount = 0U;
        RenderHandleReference images[2U];

        BindableImage GetIntermediateImage(const RenderHandle& input)
        {
            if (imageCount == 0U) {
                return {};
            }
            if (input == images[idx].GetHandle()) {
                idx = (idx + 1) % static_cast<uint32_t>(imageCount);
            }
            return { images[idx].GetHandle() };
        }
    };
    TemporaryImages ti_;

    BASE_NS::string dataStorePrefix_;
    BASE_NS::string dataStoreFullName_;
};

class RenderNodePostProcessInterfaceUtilImpl final : public IRenderNodePostProcessInterfaceUtil {
public:
    RenderNodePostProcessInterfaceUtilImpl() = default;
    ~RenderNodePostProcessInterfaceUtilImpl() override = default;

    void Init(IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecute() override;
    void Execute(IRenderCommandList& cmdList) override;

    void SetInfo(const Info& info) override;

    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const override;
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) override;

    void Ref() override;
    void Unref() override;

private:
    RenderNodePostProcessInterfaceUtil rn_;

    uint32_t refCount_ { 0U };
};
RENDER_END_NAMESPACE()

#endif // RENDER_NODECONTEXT_RENDER_NODE_POST_PROCESS_INTERFACE_UTIL_H
