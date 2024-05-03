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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_DEPTH_RENDER_SLOT_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_DEPTH_RENDER_SLOT_H

#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;

class RenderNodeDefaultDepthRenderSlot final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultDepthRenderSlot() = default;
    ~RenderNodeDefaultDepthRenderSlot() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override
    {
        return 0U;
    }

    struct BufferHandles {
        RENDER_NS::RenderHandle generalData;

        RENDER_NS::RenderHandle mesh;
        RENDER_NS::RenderHandle skinJoint;
        RENDER_NS::RenderHandle camera;

        RENDER_NS::RenderHandle defaultBuffer;
    };

    struct PerShaderData {
        RENDER_NS::RenderHandle shaderHandle;
        RENDER_NS::RenderHandle psoHandle;
        RENDER_NS::RenderHandle graphicsStateHandle;
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
    static constexpr BASE_NS::Uid UID { "fd9f70bc-e13c-4a02-a6b9-c45eed50d8f9" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultDepthRenderSlot";
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

        uint32_t nodeFlags { 0u };
        uint32_t renderSlotId { 0u };
        uint32_t shaderRenderSlotId { 0u };
        uint32_t stateRenderSlotId { 0u };
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
    };
    struct CurrentScene {
        RenderCamera camera;
        RENDER_NS::ViewportDesc viewportDesc;
        RENDER_NS::ScissorDesc scissorDesc;
        uint32_t cameraIdx { 0u };
    };
    struct SpecializationData {
        static constexpr uint32_t MAX_FLAG_COUNT { 16u };
        uint32_t flags[MAX_FLAG_COUNT];

        uint32_t maxSpecializationCount { 0u };
    };

    void ParseRenderNodeInputs();
    void RenderSubmeshes(RENDER_NS::IRenderCommandList& cmdList,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const IRenderDataStoreDefaultCamera& dataStoreCamera);
    void UpdateSet01(RENDER_NS::IRenderCommandList& cmdList);
    void CreateDefaultShaderData();
    // unique scene name as input
    void GetSceneUniformBuffers(const BASE_NS::string_view us);
    void GetCameraUniformBuffers(const BASE_NS::string_view us);
    RENDER_NS::RenderHandle CreateNewPso(const ShaderStateData& ssd,
        const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
        const RenderSubmeshFlags submeshFlags);
    void ProcessSlotSubmeshes(
        const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultMaterial& dataStoreMaterial);
    void UpdateCurrentScene(
        const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultCamera& dataStoreCamera);
    void ProcessBuffersAndDescriptors(const ObjectCounts& objectCounts);
    void ResetAndUpdateDescriptorSets();

    RENDER_NS::RenderHandle GetSubmeshPso(const ShaderStateData& ssd,
        const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
        const RenderSubmeshFlags submeshFlags);
    RENDER_NS::ShaderSpecializationConstantDataView GetShaderSpecializationView(
        const RENDER_NS::GraphicsState& gfxState,
        const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
        const RenderSubmeshFlags submeshFlags);

    CurrentScene currentScene_;
    SceneRenderDataStores stores_;

    ObjectCounts objectCounts_;
    BufferHandles buffers_;

    struct AllDescriptorSets {
        static constexpr uint32_t SINGLE_SET_COUNT { 2u };
        RENDER_NS::IDescriptorSetBinder::Ptr set01[SINGLE_SET_COUNT];
    };
    AllDescriptorSets allDescriptorSets_;
    AllShaderData allShaderData_;
    SpecializationData specializationData_;

    RENDER_NS::RenderPass renderPass_;
    // the base default render node graph from RNG setup
    RENDER_NS::RenderPass rngRenderPass_;

    RENDER_NS::RenderPostProcessConfiguration currentRenderPPConfiguration_;
    BASE_NS::vector<SlotSubmeshIndex> sortedSlotSubmeshes_;
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_DEPTH_RENDER_SLOT_H
