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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_ENV_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_ENV_H

#include <3d/render/render_data_defines_3d.h>
#include <base/containers/string.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;

class RenderNodeDefaultEnv final : public RENDER_NS::IRenderNode {
public:
    explicit RenderNodeDefaultEnv() = default;
    ~RenderNodeDefaultEnv() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "e3bc29b2-c1d0-4322-a41a-449354fd5a42" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultEnv";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct DefaultImages {
        RENDER_NS::RenderHandle cubeHandle;
        RENDER_NS::RenderHandle texHandle;
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();

    void RenderData(RENDER_NS::IRenderCommandList& cmdList);
    bool UpdateAndBindCustomSet(RENDER_NS::IRenderCommandList& cmdList, const RenderCamera::Environment& renderEnv);
    void UpdateCurrentScene(
        const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultCamera& dataStoreCamera);
    RENDER_NS::RenderHandle GetPso(const RENDER_NS::RenderHandle shaderHandle,
        const RenderCamera::Environment::BackgroundType bgType,
        const RENDER_NS::RenderPostProcessConfiguration& renderPostProcessConfiguration);
    void CreateDescriptorSets();
    // unique scene name
    void GetSceneUniformBuffers(const BASE_NS::string_view us);
    void UpdatePostProcessConfiguration();
    BASE_NS::array_view<const RENDER_NS::DynamicStateEnum> GetDynamicStates() const;
    void ResetRenderSlotData(const bool enableMultiview);
    void EvaluateFogBits();

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::RenderDataStore renderDataStore;

        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };
        uint32_t nodeFlags { 0u };

        RENDER_NS::RenderNodeGraphInputs::InputRenderPass renderPass;
        bool hasChangeableRenderPassHandles { false };
    };
    JsonInputs jsonInputs_;
    RENDER_NS::RenderNodeHandles::InputRenderPass inputRenderPass_;

    SceneBufferHandles sceneBuffers_;
    SceneCameraBufferHandles cameraBuffers_;
    SceneRenderDataStores stores_;

    RENDER_NS::RenderHandle cubemapSampler;

    RenderCamera::Environment::BackgroundType currentBgType_ { RenderCamera::Environment::BG_TYPE_NONE };
    RenderCamera::ShaderFlags currentCameraShaderFlags_ { 0U };

    struct DescriptorSets {
        RENDER_NS::IDescriptorSetBinder::Ptr set0;
        RENDER_NS::IDescriptorSetBinder::Ptr set1;
    };
    DescriptorSets allDescriptorSets_;

    RENDER_NS::PipelineLayout defaultPipelineLayout_;
    RENDER_NS::RenderHandle shaderHandle_;
    RENDER_NS::RenderHandle psoHandle_;
    bool enableMultiView_ { false };
    bool customSet2_ { false };

    struct CurrentScene {
        RenderCamera camera;
        RENDER_NS::ViewportDesc viewportDesc;
        RENDER_NS::ScissorDesc scissorDesc;

        RenderCamera::ShaderFlags cameraShaderFlags { 0u }; // evaluated based on camera and scene flags
    };
    CurrentScene currentScene_;
    DefaultImages defaultImages_;

    RENDER_NS::RenderPass renderPass_;
    // the base default render node graph from RNG setup
    RENDER_NS::RenderPass rngRenderPass_;
    bool fsrEnabled_ { false };

    RENDER_NS::RenderPostProcessConfiguration currentRenderPPConfiguration_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_ENV_H
