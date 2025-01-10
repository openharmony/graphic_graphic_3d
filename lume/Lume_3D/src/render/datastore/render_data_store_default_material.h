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

#ifndef CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_MATERIAL_H
#define CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_MATERIAL_H

#include <atomic>
#include <cstdint>

#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/array_view.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/device/intf_shader_manager.h>

#include "util/linear_allocator.h"

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
class IGpuResourceManager;
class IShaderManager;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
/**
RenderDataStoreDefaultMaterial implementation.
*/
class RenderDataStoreDefaultMaterial final : public IRenderDataStoreDefaultMaterial {
public:
    static constexpr uint64_t SLOT_SORT_HASH_MASK { 0xFFFFffff };
    static constexpr uint64_t SLOT_SORT_HASH_SHIFT { 32u };
    static constexpr uint64_t SLOT_SORT_MAX_DEPTH { 0xFFFFffff };
    static constexpr uint64_t SLOT_SORT_DEPTH_SHIFT { 32u };

    RenderDataStoreDefaultMaterial(RENDER_NS::IRenderContext& renderContext, const BASE_NS::string_view name);
    ~RenderDataStoreDefaultMaterial() override = default;

    struct LinearAllocatorStruct {
        uint32_t currentIndex { 0 };
        BASE_NS::vector<BASE_NS::unique_ptr<LinearAllocator>> allocators;
    };

    // IRenderDataStore
    void PreRender() override {}
    // Reset and start indexing from the beginning. i.e. frame boundary reset.
    void PostRender() override;
    void PreRenderBackend() override {}
    void PostRenderBackend() override {}
    void Clear() override;

    uint32_t GetFlags() const override
    {
        return 0;
    }

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    // IRenderDataStoreDefaultMaterial
    uint32_t AddMeshData(const RenderMeshData& meshData) override;
    uint32_t UpdateMaterialData(const uint64_t id,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData) override;
    uint32_t UpdateMaterialData(const uint64_t id,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customData) override;
    uint32_t UpdateMaterialData(const uint64_t id,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customPropertyData,
        const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customBindings) override;

    void DestroyMaterialData(const uint64_t id) override;
    void DestroyMaterialData(const BASE_NS::array_view<uint64_t> ids) override;

    RenderFrameMaterialIndices AddFrameMaterialData(uint64_t id) override;
    RenderFrameMaterialIndices AddFrameMaterialData(uint64_t id, uint32_t instanceCount) override;
    RenderFrameMaterialIndices AddFrameMaterialData(BASE_NS::array_view<const uint64_t> id) override;
    RenderFrameMaterialIndices AddFrameMaterialData(
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customPropertyData,
        const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customBindings) override;
    RenderFrameMaterialIndices AddFrameMaterialInstanceData(
        uint32_t materialIndex, uint32_t frameOffset, uint32_t instanceIndex) override;

    BASE_NS::array_view<const uint32_t> GetMaterialFrameIndices() const override;

    uint32_t GetMaterialIndex(const uint64_t id) const override;
    uint32_t AddSkinJointMatrices(const BASE_NS::array_view<const BASE_NS::Math::Mat4X4> skinJointMatrices,
        const BASE_NS::array_view<const BASE_NS::Math::Mat4X4> prevSkinJointMatrices) override;

    void AddSubmesh(const RenderSubmeshWithHandleReference& submesh) override;
    void AddSubmesh(const RenderSubmeshWithHandleReference& submesh,
        const BASE_NS::array_view<const RENDER_NS::IShaderManager::RenderSlotData> renderSlotAndShaders) override;

    void SetRenderSlots(const RenderDataDefaultMaterial::MaterialSlotType materialSlotType,
        const BASE_NS::array_view<const uint32_t> renderSlotIds) override;
    uint64_t GetRenderSlotMask(const RenderDataDefaultMaterial::MaterialSlotType materialSlotType) const override;

    BASE_NS::array_view<const uint32_t> GetSlotSubmeshIndices(const uint32_t renderSlotId) const override;
    BASE_NS::array_view<const RenderDataDefaultMaterial::SlotMaterialData> GetSlotSubmeshMaterialData(
        const uint32_t renderSlotId) const override;
    RenderDataDefaultMaterial::ObjectCounts GetSlotObjectCounts(const uint32_t renderSlotId) const override;

    RenderDataDefaultMaterial::ObjectCounts GetObjectCounts() const override;
    BASE_NS::array_view<const RenderMeshData> GetMeshData() const override;
    BASE_NS::array_view<const RenderDataDefaultMaterial::JointMatrixData> GetMeshJointMatrices() const override;
    BASE_NS::array_view<const RenderSubmesh> GetSubmeshes() const override;
    BASE_NS::array_view<const BASE_NS::Math::Mat4X4> GetSubmeshJointMatrixData(
        const uint32_t skinJointIndex) const override;

    BASE_NS::array_view<const RenderDataDefaultMaterial::AllMaterialUniforms> GetMaterialUniforms() const override;
    BASE_NS::array_view<const RenderDataDefaultMaterial::MaterialHandles> GetMaterialHandles() const override;
    BASE_NS::array_view<const uint8_t> GetMaterialCustomPropertyData(const uint32_t materialIndex) const override;
    BASE_NS::array_view<const RenderDataDefaultMaterial::SubmeshMaterialFlags> GetSubmeshMaterialFlags() const override;

    BASE_NS::array_view<const RenderDataDefaultMaterial::CustomResourceData> GetCustomResourceHandles() const override;

    uint32_t GenerateRenderHash(const RenderDataDefaultMaterial::SubmeshMaterialFlags& flags) const override;

    RenderFrameMaterialIndices AddRenderSlotSubmeshesFrameMaterialData(const uint32_t toSlotId,
        BASE_NS::array_view<const uint32_t> fromSlotIds,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customPropertyData,
        const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customBindings) override;

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreDefaultMaterial";
    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(RENDER_NS::IRenderContext& renderContext, char const* name);

    // container for MaterialData and frame info
    struct MaterialDataContainer {
        RenderDataDefaultMaterial::MaterialData md;
        // provided information if the material in in use this frame
        // this is then used to add references of resources
        bool frameReferenced { false };
        bool noId { false };
    };
    struct CustomPropertyData {
        // vector inside a vector
        BASE_NS::vector<uint8_t> data;
    };
    struct AllMaterialData {
        // NOTE: Uses packing, needs to be modified if changed
        BASE_NS::vector<RenderDataDefaultMaterial::AllMaterialUniforms> allUniforms;
        BASE_NS::vector<RenderDataDefaultMaterial::MaterialHandles> handles;
        BASE_NS::vector<MaterialDataContainer> data;
        BASE_NS::vector<CustomPropertyData> customPropertyData;
        BASE_NS::vector<RenderDataDefaultMaterial::CustomResourceData> customResourceData;
        // material id is normally Entity.id, index to material vectors
        BASE_NS::unordered_map<uint64_t, uint32_t> materialIdToIndex;

        // The processing order of materials for a frame (takes into account instancing)
        // The same material index can there multiple times
        BASE_NS::vector<uint32_t> frameIndices;
        BASE_NS::unordered_map<uint64_t, uint32_t> idHashToFrameIndex;

        BASE_NS::vector<uint32_t> availableIndices;
    };

private:
    void FillSubmeshImpl(BASE_NS::array_view<const RENDER_NS::IShaderManager::RenderSlotData> renderSlotAndShaders,
        uint32_t submeshIndex, RenderSubmesh& submesh);
    uint32_t AddMaterialDataImpl(uint32_t matIndex,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customData,
        const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customResourceData);
    void UpdateFrameMaterialResourceReferences(uint32_t materialIndex);

    void GetDefaultRenderSlots();
    uint32_t GetRenderSlotIdFromMasks(const uint32_t renderSlotId) const;

    const BASE_NS::string name_;
    RENDER_NS::IGpuResourceManager& gpuResourceMgr_;
    RENDER_NS::IShaderManager& shaderMgr_;

    BASE_NS::vector<RenderSubmesh> submeshes_;
    BASE_NS::vector<RenderMeshData> meshData_;
    BASE_NS::vector<RenderDataDefaultMaterial::JointMatrixData> submeshJointMatrixIndices_;

    LinearAllocatorStruct submeshJointMatricesAllocator_;

    AllMaterialData matData_;

    // per submesh
    BASE_NS::vector<RenderDataDefaultMaterial::SubmeshMaterialFlags> submeshMaterialFlags_;

    // separate store for references
    BASE_NS::vector<RENDER_NS::RenderHandleReference> handleReferences_;

    struct SlotSubmeshData {
        BASE_NS::vector<uint32_t> indices;
        BASE_NS::vector<RenderDataDefaultMaterial::SlotMaterialData> materialData;

        RenderDataDefaultMaterial::ObjectCounts objectCounts;
    };
    BASE_NS::unordered_map<uint32_t, SlotSubmeshData> slotToSubmeshIndices_;

    struct MaterialRenderSlots {
        uint32_t defaultOpaqueRenderSlot { ~0u };
        uint64_t opaqueMask { 0 };

        uint32_t defaultTranslucentRenderSlot { ~0u };
        uint64_t translucentMask { 0 };

        uint32_t defaultDepthRenderSlot { ~0u };
        uint64_t depthMask { 0 };
    };
    MaterialRenderSlots materialRenderSlots_;

    std::atomic_int32_t refcnt_ { 0 };
};
CORE3D_END_NAMESPACE()

#endif // CORE__RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_MATERIAL_H
