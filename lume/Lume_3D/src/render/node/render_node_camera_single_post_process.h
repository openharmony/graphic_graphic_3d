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

#ifndef CORE3D_RENDER__NODE__RENDER_NODE_CAMERA_SINGLE_POST_PROCESS_H
#define CORE3D_RENDER__NODE__RENDER_NODE_CAMERA_SINGLE_POST_PROCESS_H

#include <3d/namespace.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <base/containers/string.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/node/render_copy_helper.h"
#include "render/render_node_scene_util.h"

RENDER_BEGIN_NAMESPACE()
struct RenderNodeGraphInputs;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;

class RenderNodeCameraSinglePostProcess final : public RENDER_NS::IRenderNode {
public:
    RenderNodeCameraSinglePostProcess() = default;
    ~RenderNodeCameraSinglePostProcess() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "6257f029-f747-44c7-8de9-87b41b8afd6a" };
    static constexpr char const* TYPE_NAME = "RenderNodeCameraSinglePostProcess";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct ShadowBuffers {
        RENDER_NS::RenderHandle pcfDepthHandle;
        RENDER_NS::RenderHandle vsmColorHandle;

        RENDER_NS::RenderHandle pcfSamplerHandle;
        RENDER_NS::RenderHandle vsmSamplerHandle;
    };
    struct DefaultImagesAndSamplers {
        RENDER_NS::RenderHandle cubemapHandle;

        RENDER_NS::RenderHandle linearHandle;
        RENDER_NS::RenderHandle nearestHandle;
        RENDER_NS::RenderHandle linearMipHandle;

        RENDER_NS::RenderHandle colorPrePassHandle;
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void RegisterOutputs();
    void InitCreateBinders();
    void ParseRenderNodeInputs();
    void UpdateGlobalPostProcessUbo();
    void UpdateImageData();
    void ProcessPostProcessConfiguration();
    void GetSceneUniformBuffers(const BASE_NS::string_view us);
    void UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight);
    void UpdateSet0(RENDER_NS::IRenderCommandList& cmdList);
    void BindDefaultResources(const uint32_t set, const RENDER_NS::DescriptorSetLayoutBindingResources& bindings);
    void ExecuteSinglePostProcess(RENDER_NS::IRenderCommandList& cmdList);

    enum class DefaultOutputImage : uint32_t {
        OUTPUT = 0,
        INPUT_OUTPUT_COPY = 1,
        INPUT = 2,
        WHITE = 3,
        BLACK = 4,
    };

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    // Json resources which might need re-fetching
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::InputRenderPass renderPass;
        RENDER_NS::RenderNodeGraphInputs::InputResources resources;
        RENDER_NS::RenderNodeGraphInputs::InputResources dispatchResources;

        RENDER_NS::RenderNodeGraphInputs::RenderDataStore renderDataStore;

        BASE_NS::string ppName;
        DefaultOutputImage defaultOutputImage { DefaultOutputImage::OUTPUT };

        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };

        bool hasChangeableRenderPassHandles { false };
        bool hasChangeableResourceHandles { false };
        bool hasChangeableDispatchHandles { false };

        uint32_t inputIdx { ~0u };
        uint32_t outputIdx { ~0u };
    };
    JsonInputs jsonInputs_;

    struct BuiltInVariables {
        RENDER_NS::RenderHandle input;

        RENDER_NS::RenderHandle output;

        RENDER_NS::RenderHandle defBuffer;
        RENDER_NS::RenderHandle defBlackImage;
        RENDER_NS::RenderHandle defWhiteImage;
        RENDER_NS::RenderHandle defSampler;
    };
    BuiltInVariables builtInVariables_;

    SceneBufferHandles sceneBuffers_;
    SceneCameraBufferHandles cameraBuffers_;

    DefaultImagesAndSamplers defaultImagesAndSamplers_;

    struct CurrentScene {
        RenderCamera camera;
        RENDER_NS::RenderHandle cameraEnvRadianceHandle;
        RENDER_NS::RenderHandle prePassColorTarget;

        bool hasShadow { false };
        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes {};
        IRenderDataStoreDefaultLight::LightingFlags lightingFlags { 0u };
    };
    CurrentScene currentScene_;

    ShadowBuffers shadowBuffers_;

    RENDER_NS::RenderNodeHandles::InputRenderPass inputRenderPass_;
    RENDER_NS::RenderNodeHandles::InputResources inputResources_;
    RENDER_NS::RenderNodeHandles::InputResources dispatchResources_;
    RENDER_NS::RenderHandle shader_;
    RENDER_NS::PipelineLayout pipelineLayout_;
    RENDER_NS::RenderHandle psoHandle_;
    RENDER_NS::PushConstant pushConstant_;

    RENDER_NS::IPipelineDescriptorSetBinder::Ptr pipelineDescriptorSetBinder_;

    RENDER_NS::PostProcessConfiguration ppGlobalConfig_;
    RENDER_NS::IRenderDataStorePostProcess::PostProcess ppLocalConfig_;

    bool graphics_ { true };
    RENDER_NS::ShaderThreadGroup threadGroupSize_ { 1u, 1u, 1u };

    SceneRenderDataStores stores_;

    struct UboHandles {
        // first 512 aligned is global post process
        // after (256) we have effect local data
        RENDER_NS::RenderHandleReference postProcess;
    };
    UboHandles ubos_;

    RenderCopyHelper renderCopy_;

    bool useAutoBindSet0_ { false };
    bool valid_ { false };
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER__NODE__RENDER_NODE_CAMERA_SINGLE_POST_PROCESS_H
