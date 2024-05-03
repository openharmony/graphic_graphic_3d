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

#ifndef CORE__RENDER__NODE__RENDER_NODE_DEFAULT_SHADOW_RENDER_SLOT_H
#define CORE__RENDER__NODE__RENDER_NODE_DEFAULT_SHADOW_RENDER_SLOT_H

#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/formats.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

CORE3D_BEGIN_NAMESPACE()
class IRenderDataStoreDefaultCamera;
class IRenderDataStoreDefaultScene;

class RenderNodeDefaultShadowRenderSlot final : public RENDER_NS::IRenderNode {
public:
    RenderNodeDefaultShadowRenderSlot() = default;
    ~RenderNodeDefaultShadowRenderSlot() override = default;

    void InitNode(RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr) override;
    void PreExecuteFrame() override;
    void ExecuteFrame(RENDER_NS::IRenderCommandList& cmdList) override;
    ExecuteFlags GetExecuteFlags() const override;

    struct ShadowBuffers {
        BASE_NS::string depthName;
        BASE_NS::string vsmColorName;

        RENDER_NS::RenderHandleReference depthHandle;
        RENDER_NS::RenderHandleReference vsmColorHandle;

        BASE_NS::Format depthFormat { BASE_NS::Format::BASE_FORMAT_D16_UNORM };
        BASE_NS::Format colorFormat { BASE_NS::Format::BASE_FORMAT_R16G16_UNORM };

        // full size of atlas
        uint32_t width { 2u };
        uint32_t height { 2u };

        IRenderDataStoreDefaultLight::ShadowTypes shadowTypes {};
    };

    struct ShaderStateData {
        RENDER_NS::RenderHandle shader;
        RENDER_NS::RenderHandle gfxState;
        uint64_t hash { 0 };
        RENDER_NS::RenderHandle defaultShader;
        RENDER_NS::RenderHandle defaultShaderState;
    };
    struct ShadowTypeShaders {
        RENDER_NS::RenderHandle basic;
        RENDER_NS::RenderHandle basicState;
    };

    // for plugin / factory interface
    static constexpr BASE_NS::Uid UID { "ce5b861e-09a0-4dfa-84f1-c1a9986f1fdf" };
    static constexpr char const* const TYPE_NAME = "RenderNodeDefaultShadowRenderSlot";
    static constexpr IRenderNode::BackendFlags BACKEND_FLAGS = IRenderNode::BackendFlagBits::BACKEND_FLAG_BITS_DEFAULT;
    static constexpr IRenderNode::ClassType CLASS_TYPE = IRenderNode::ClassType::CLASS_TYPE_NODE;
    static IRenderNode* Create();
    static void Destroy(IRenderNode* instance);

private:
    RENDER_NS::IRenderNodeContextManager* renderNodeContextMgr_ { nullptr };

    struct ObjectCounts {
        uint32_t maxSlotMeshCount { 0u };
        uint32_t maxSlotSubmeshCount { 0u };
        uint32_t maxSlotSkinCount { 0u };
        uint32_t maxSlotMaterialCount { 0u };
    };
    struct UboHandles {
        RENDER_NS::RenderHandleReference generalData;

        RENDER_NS::RenderHandle mesh;
        RENDER_NS::RenderHandle skinJoint;
        RENDER_NS::RenderHandle camera;
    };

    void ParseRenderNodeInputs();
    void RenderSubmeshes(RENDER_NS::IRenderCommandList& cmdList,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial,
        const IRenderDataStoreDefaultLight::ShadowType shadowType, const RenderCamera& camera, const RenderLight& light,
        const uint32_t shadowPassIdx);
    void UpdateSet01(RENDER_NS::IRenderCommandList& cmdList, const uint32_t shadowPassIdx);

    void UpdateGeneralDataUniformBuffers(const IRenderDataStoreDefaultLight& dataStoreLight);
    void CreateDefaultShaderData();
    RENDER_NS::RenderHandle CreateNewPso(const ShaderStateData& ssd,
        const RENDER_NS::ShaderSpecializationConstantDataView& specialization, const RenderSubmeshFlags submeshFlags);
    RENDER_NS::RenderPass CreateRenderPass(const ShadowBuffers& buffers);
    void ProcessSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
        const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t shadowCameraIdx);
    void ProcessBuffersAndDescriptors(const ObjectCounts& objectCounts);
    void UpdateCurrentScene(
        const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultLight& dataStoreLight);

    RENDER_NS::RenderHandle GetSubmeshPso(const ShaderStateData& ssd,
        const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
        const RenderSubmeshFlags submeshFlags);

    struct JsonInputs {
        RENDER_NS::RenderSlotSortType sortType { RENDER_NS::RenderSlotSortType::NONE };
        RENDER_NS::RenderSlotCullType cullType { RENDER_NS::RenderSlotCullType::NONE };

        uint32_t nodeFlags { 0u };
        uint32_t renderSlotId { 0u };
        uint32_t renderSlotVsmId { 0u };
    };
    JsonInputs jsonInputs_;

    struct CurrentScene {
        BASE_NS::Math::UVec2 res { 0u, 0u };
        RENDER_NS::ViewportDesc viewportDesc;
        RENDER_NS::ScissorDesc scissorDesc;
        BASE_NS::Math::Vec4 sceneTimingData { 0.0f, 0.0f, 0.0f, 0.0f };

        uint32_t renderSlotId { ~0u };
    };
    CurrentScene currentScene_;

    SceneRenderDataStores stores_;
    ShadowBuffers shadowBuffers_;
    ObjectCounts objectCounts_;

    struct AllDescriptorSets {
        RENDER_NS::IDescriptorSetBinder::Ptr set0[DefaultMaterialLightingConstants::MAX_SHADOW_COUNT];
        RENDER_NS::IDescriptorSetBinder::Ptr set1[DefaultMaterialLightingConstants::MAX_SHADOW_COUNT];
    };
    AllDescriptorSets allDescriptorSets_;

    ShadowTypeShaders pcfShaders_;
    ShadowTypeShaders vsmShaders_;

    struct PerShaderData {
        RENDER_NS::RenderHandle shaderHandle;
        RENDER_NS::RenderHandle psoHandle;
        RENDER_NS::RenderHandle stateHandle;
    };
    struct AllShaderData {
        BASE_NS::vector<PerShaderData> perShaderData;
        // shader hash, per shader data index
        BASE_NS::unordered_map<uint64_t, uint32_t> shaderIdToData;

        bool slotHasShaders { false };
        RENDER_NS::RenderHandle defaultPlHandle;
        RENDER_NS::RenderHandle defaultVidHandle;
        RENDER_NS::PipelineLayout defaultPipelineLayout;
        BASE_NS::vector<RENDER_NS::ShaderSpecialization::Constant> defaultSpecilizationConstants;
    };
    AllShaderData allShaderData_;

    UboHandles uboHandles_;

    RENDER_NS::RenderPass renderPass_;
    BASE_NS::vector<SlotSubmeshIndex> sortedSlotSubmeshes_;

    bool validShadowNode_ { true };
    uint32_t shadowCount_ { 0U };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE__RENDER_NODE_DEFAULT_SHADOW_RENDER_SLOT_H
