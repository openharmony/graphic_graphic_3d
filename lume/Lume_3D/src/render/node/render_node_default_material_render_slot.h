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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_RENDER_SLOT_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_RENDER_SLOT_H

#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;

class RenderNodeDefaultMaterialRenderSlot final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultMaterialRenderSlot() = default;
    ~RenderNodeDefaultMaterialRenderSlot() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        // render pass always open
        return 0U;
    }

    struct MaterialHandleStruct {
        RENDER_NS::BindableImage resources[RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT];
    };

    struct ShadowBuffers {
        RENDER_NS::RenderHandle depthHandle;
        RENDER_NS::RenderHandle vsmColorHandle;

        RENDER_NS::RenderHandle pcfSamplerHandle;
        RENDER_NS::RenderHandle vsmSamplerHandle;
    };

    struct BufferHandles {
        RENDER_NS::RenderHandle mat;
        RENDER_NS::RenderHandle matTransform;
        RENDER_NS::RenderHandle matCustom;
        RENDER_NS::RenderHandle mesh;
        RENDER_NS::RenderHandle skinJoint;
        RENDER_NS::RenderHandle environment;
        RENDER_NS::RenderHandle fog;
        RENDER_NS::RenderHandle generalData;
        RENDER_NS::RenderHandle postProcess;

        RENDER_NS::RenderHandle camera;
        RENDER_NS::RenderHandle light;
        RENDER_NS::RenderHandle lightCluster;

        RENDER_NS::RenderHandle defaultBuffer;
    };

    struct PerShaderData {
        RENDER_NS::RenderHandle shaderHandle;
        RENDER_NS::RenderHandle psoHandle;
        RENDER_NS::RenderHandle graphicsStateHandle;
        bool needsCustomSetBindings { false };
    };
    struct AllShaderData {
        BASE_NS::vector<PerShaderData> perShaderData;
        // shader hash, per shader data index
        BASE_NS::unordered_map<uint64_t, uint32_t> shaderIdToData;

        bool slotHasShaders { false };
        RENDER_NS::RenderHandle defaultShaderHandle;
        RENDER_NS::RenderHandle defaultStateHandle;
        RENDER_NS::RenderHandle defaultPlHandle;
        RENDER_NS::RenderHandle defaultVidHandle;
        RENDER_NS::PipelineLayout defaultPipelineLayout;
        BASE_NS::vector<RENDER_NS::ShaderSpecialization::Constant> defaultSpecilizationConstants;
    };

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "80758e28-f064-45e6-878d-624652598405" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultMaterialRenderSlot";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };
    struct JsonInputs {
        RENDER_NS::RenderNodeGraphInputs::RenderDataStore renderDataStore;

        BASE_NS::string customCameraName;
        uint64_t customCameraId { INVALID_CAM_ID };

        RENDER_NS::RenderSlotSortType sortType { RENDER_NS::RenderSlotSortType::NONE };
        RENDER_NS::RenderSlotCullType cullType { RENDER_NS::RenderSlotCullType::NONE };

        RenderSceneFlags nodeFlags { 0u };
        uint32_t renderSlotId { 0u };
        uint32_t shaderRenderSlotId { 0u };
        uint32_t stateRenderSlotId { 0u };
        uint32_t shaderRenderSlotBaseId { 0u };
        uint32_t shaderRenderSlotMultiviewId { 0u };
        bool initialExplicitShader { false };
        bool explicitShader { false };
        RenderSubmeshFlags nodeSubmeshExtraFlags { 0 };
        RenderMaterialFlags nodeMaterialDiscardFlags { 0 };

        RENDER_NS::RenderNodeGraphInputs::InputRenderPass renderPass;
        bool hasChangeableRenderPassHandles { false };
    };
    JsonInputs jsonInputs_;
    RENDER_NS::RenderNodeHandles::InputRenderPass inputRenderPass_;

    struct ShaderStateData {
        RENDER_NS::RenderHandle shader;
        RENDER_NS::RenderHandle gfxState;
        uint64_t hash { 0 };
    };
    struct ObjectCounts {
        uint32_t maxSlotMeshCount { 0u };
        uint32_t maxSlotSubmeshCount { 0u };
        uint32_t maxSlotSkinCount { 0u };
        uint32_t maxSlotMaterialCount { 0u };
    };
    struct CurrentScene {
        RenderCamera camera;
        RENDER_NS::RenderHandle cameraEnvRadianceHandle;
        RENDER_NS::ViewportDesc viewportDesc;
        RENDER_NS::ScissorDesc scissorDesc;

        RENDER_NS::RenderHandle prePassColorTarget;

        bool hasShadow { false };
        uint32_t cameraIdx { 0u };
        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes {};
        IRenderDataStoreDefaultLight::LightingFlags lightingFlags { 0u };
        RenderCamera::ShaderFlags cameraShaderFlags { 0u }; // evaluated based on camera and scene flags
    };
    struct SpecializationData {
        static constexpr uint32_t MAX_FLAG_COUNT { 16u };
        uint32_t flags[MAX_FLAG_COUNT];

        uint32_t maxSpecializationCount { 0u };
    };
    struct PsoAndInfo {
        RENDER_NS::RenderHandle pso;
        bool set3 { false };
    };

    void ParseRenderNodeInputs();
    void RenderSubmeshes(RENDER_NS::IRenderCommandList& cmdList,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const IRenderDataStoreDefaultCamera& dataStoreCamera);
    void UpdateSet01(RENDER_NS::IRenderCommandList& cmdList);
    void UpdateAndBindSet2(
        RENDER_NS::IRenderCommandList& cmdList, const MaterialHandleStruct& materialHandles, const uint32_t objIdx);
    bool UpdateAndBindSet3(RENDER_NS::IRenderCommandList& cmdList,
        const RenderDataDefaultMaterial::CustomResourceData& customResourceData);
    void CreateDefaultShaderData();
    // unique scene name as input
    void GetSceneUniformBuffers(const BASE_NS::string_view us);
    PsoAndInfo CreateNewPso(const ShaderStateData& ssd,
        const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
        const RenderSubmeshFlags submeshFlags, const IRenderDataStoreDefaultLight::LightingFlags lightingFlags,
        const RenderCamera::ShaderFlags cameraShaderFlags);
    void ProcessSlotSubmeshes(
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight);
    void ProcessBuffersAndDescriptors(const ObjectCounts& objectCounts);
    void ResetAndUpdateDescriptorSets();

    void UpdatePostProcessConfiguration();
    PsoAndInfo GetSubmeshPso(const ShaderStateData& ssd,
        const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
        const RenderSubmeshFlags submeshFlags, const IRenderDataStoreDefaultLight::LightingFlags lightingFlags,
        const RenderCamera::ShaderFlags cameraShaderFlags);
    RENDER_NS::ShaderSpecializationConstantDataView GetShaderSpecializationView(
        const RENDER_NS::GraphicsState& gfxState,
        const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
        const RenderSubmeshFlags submeshFlags, const IRenderDataStoreDefaultLight::LightingFlags lightingFlags,
        const RenderCamera::ShaderFlags cameraShaderFlags);
    BASE_NS::array_view<const RENDER_NS::DynamicStateEnum> GetDynamicStates() const;
    void EvaluateFogBits();
    void ResetRenderSlotData(const uint32_t shaderRenderSlotId, const bool multiView);

    MaterialHandleStruct defaultMaterialStruct_;

    CurrentScene currentScene_;
    SceneRenderDataStores stores_;

    ObjectCounts objectCounts_;

    SceneBufferHandles sceneBuffers_;
    SceneCameraBufferHandles cameraBuffers_;
    ShadowBuffers shadowBuffers_;

    struct DefaultSamplers {
        RENDER_NS::RenderHandle cubemapHandle;

        RENDER_NS::RenderHandle linearHandle;
        RENDER_NS::RenderHandle nearestHandle;
        RENDER_NS::RenderHandle linearMipHandle;
    };
    DefaultSamplers defaultSamplers_;

    RENDER_NS::RenderHandle defaultColorPrePassHandle_;

    struct AllDescriptorSets {
        static constexpr uint32_t SINGLE_SET_COUNT { 2u };
        RENDER_NS::IDescriptorSetBinder::Ptr set01[SINGLE_SET_COUNT];
        BASE_NS::vector<RENDER_NS::IDescriptorSetBinder::Ptr> sets2;
    };
    AllDescriptorSets allDescriptorSets_;
    AllShaderData allShaderData_;
    SpecializationData specializationData_;

    RENDER_NS::RenderPass renderPass_;
    // the base default render node graph from RNG setup
    RENDER_NS::RenderPass rngRenderPass_;
    bool fsrEnabled_ { false };

    RENDER_NS::RenderPostProcessConfiguration currentRenderPPConfiguration_;
    BASE_NS::vector<SlotSubmeshIndex> sortedSlotSubmeshes_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_MATERIAL_RENDER_SLOT_H
