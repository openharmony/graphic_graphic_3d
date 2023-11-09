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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_DEFERRED_SHADING_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_DEFERRED_SHADING_H

#include <3d/render/intf_render_data_store_default_light.h>
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

class RenderNodeDefaultMaterialDeferredShading final : public RENDER_NS::IRenderNode {
public:
    explicit RenderNodeDefaultMaterialDeferredShading() = default;
    ~RenderNodeDefaultMaterialDeferredShading() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "2abdac7a-de6c-4e82-b562-665a0553fc55" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultMaterialDeferredShading";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

    struct BufferHandles {
        RENDER_NS::RenderHandle environment;
        RENDER_NS::RenderHandle fog;
        RENDER_NS::RenderHandle generalData;
        RENDER_NS::RenderHandle postProcess;
        RENDER_NS::RenderHandle camera;
        RENDER_NS::RenderHandle light;
        RENDER_NS::RenderHandle lightCluster;

        RENDER_NS::RenderHandle defaultBuffer;
    };
    struct ShadowBuffers {
        RENDER_NS::RenderHandle pcfDepthHandle;
        RENDER_NS::RenderHandle vsmColorHandle;

        RENDER_NS::RenderHandleReference pcfSamplerHandle;
        RENDER_NS::RenderHandleReference vsmSamplerHandle;
    };
    struct AllShaderData {
        uint64_t psoHash { 0 };

        RENDER_NS::RenderHandle psoHandle;
        RENDER_NS::RenderHandle shaderHandle;
        RENDER_NS::RenderHandle stateHandle;
        RENDER_NS::RenderHandle plHandle;
        BASE_NS::vector<RENDER_NS::ShaderSpecialization::Constant> defaultSpecilizationConstants;
    };

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    void ParseRenderNodeInputs();
    void RenderData(RENDER_NS::IRenderCommandList& cmdList);
    void UpdateSet0(RENDER_NS::IRenderCommandList& cmdList);
    void UpdateSet1(RENDER_NS::IRenderCommandList& cmdList);
    void UpdatePostProcessConfiguration();
    void UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight);
    RENDER_NS::RenderHandle GetPsoHandle();
    void GetSceneUniformBuffers(const BASE_NS::string_view uniqueSceneName);
    void GetCameraUniformBuffers();
    void CreateDefaultShaderData();
    void CreateDescriptorSets();

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::RenderDataStore renderDataStore;

        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };

        uint32_t nodeFlags { 0u };
        uint32_t renderSlotId { 0u };

        RENDER_NS::RenderNodeGraphInputs::InputRenderPass renderPass;
    };
    JsonInputs jsonInputs_;
    RENDER_NS::RenderNodeHandles::InputRenderPass inputRenderPass_;

    SceneRenderDataStores stores_;

    BufferHandles bufferHandles_;
    ShadowBuffers shadowBuffers_;

    struct CreatedTargets {
        RENDER_NS::RenderHandleReference cubemapSampler;
    };
    CreatedTargets createdTargets_;

    struct SamplerHandles {
        RENDER_NS::RenderHandle cubemap;
        RENDER_NS::RenderHandle linear;
        RENDER_NS::RenderHandle nearest;
        RENDER_NS::RenderHandle linearMip;
    };
    SamplerHandles samplerHandles_;

    RENDER_NS::RenderHandle defaultColorPrePassHandle_;

    struct DescriptorSets {
        RENDER_NS::IDescriptorSetBinder::Ptr set0;
        RENDER_NS::IDescriptorSetBinder::Ptr set1;
    };
    DescriptorSets allDescriptorSets_;
    AllShaderData allShaderData_;

    struct CurrentScene {
        RenderCamera camera;
        RENDER_NS::ViewportDesc viewportDesc;
        RENDER_NS::ScissorDesc scissorDesc;

        RENDER_NS::RenderHandle prePassColorTarget;

        bool hasShadow { false };
        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes {};
        IRenderDataStoreDefaultLight::LightingFlags lightingFlags { 0u };
    };
    CurrentScene currentScene_;

    RENDER_NS::RenderPostProcessConfiguration currentRenderPPConfiguration_;
    RENDER_NS::RenderHandle defaultSkyBoxRadianceCubemap_;
    RENDER_NS::RenderPass renderPass_;
    RENDER_NS::RenderHandle shader_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_DEFERRED_SHADING_H
