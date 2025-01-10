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

#ifndef API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_MATERIAL_H
#define API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_MATERIAL_H

#include <cstdint>

#include <3d/ecs/components/material_component.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/array_view.h>
#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/device/intf_shader_manager.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
/** @ingroup group_render_irenderdatastoredefaultmaterial */
/** RenderDataDefaultMaterial.
 */
struct RenderDataDefaultMaterial {
    /** min supported roughness value. clamped to this - 1.0.
    Avoids division with zero in shader BRDF calculations and prevents "zero" size speculars. */
    static constexpr float MIN_ROUGHNESS { 0.089f };
    /** max supported skin matrix count */
    static constexpr uint32_t MAX_SKIN_MATRIX_COUNT { 128u };

    /** Count of uvec4 variables for material uniforms (must match 3d_dm_structure_common.h)
     * Aligned for 256 bytes with indices.
     */
    static constexpr uint32_t MATERIAL_FACTOR_UNIFORM_VEC4_COUNT { 15u };
    static constexpr uint32_t MATERIAL_PACKED_UNIFORM_UVEC4_COUNT { 15u };

    /** Count of uvec4 variables for material uniforms (must match 3d_dm_structure_common.h) */
    static constexpr uint32_t MATERIAL_TEXTURE_COUNT { 11u };

    /** max default material custom resources */
    static constexpr uint32_t MAX_MATERIAL_CUSTOM_RESOURCE_COUNT { 8u };

    /** max default material custom resources */
    static constexpr uint32_t MAX_MESH_USER_VEC4_COUNT { 8u };

    /** max default material custom resources */
    static constexpr uint32_t MAX_MATERIAL_CUSTOM_PROPERTY_BYTE_SIZE { 256u };

    /** default depth rendering related important flags for render material flags */
    static constexpr uint32_t RENDER_MATERIAL_DEPTH_FLAGS {
        RenderMaterialFlagBits::RENDER_MATERIAL_GPU_INSTANCING_BIT
    };
    /** default depth rendering related important flags for submesh flags */
    static constexpr uint32_t RENDER_SUBMESH_DEPTH_FLAGS { RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT };

    /** Input material uniforms
     * There's no conversion happening to these, so pre-multiplications etc needs to happen prior
     */
    struct InputMaterialUniforms {
        // the factor values are not the same default values as in MaterialComponent
        struct TextureData {
            BASE_NS::Math::Vec4 factor { 0.0f, 0.0f, 0.0f, 0.0f };
            BASE_NS::Math::Vec2 translation { 0.0f, 0.0f };
            float rotation { 0.0f };
            BASE_NS::Math::Vec2 scale { 1.0f, 1.0f };
        };
        TextureData textureData[MATERIAL_TEXTURE_COUNT];
        // separate values which is pushed to shader alpha cutoff
        float alphaCutoff { 1.0f };
        // texcoord set bits, selection for uv0 or uv1
        uint32_t texCoordSetBits { 0u };
        // texcoord transform set bits
        uint32_t texTransformSetBits { 0u };
        /** material id */
        uint64_t id { 0 };
    };

    /** Material uniforms (NOTE: rotScaleTrans could be packed more)
     */
    struct MaterialUniforms {
        // Factors as half4
        // 0: baseColor
        // 1: normal
        // 2: material
        // 3: emissive
        // 4: ao
        // 5: clearcoat
        // 6: clearcoat roughness
        // 7: clearcoat normal
        // 8: sheen
        // 9: transmission
        // 10: specular
        // 11: alpha cutoff

        BASE_NS::Math::Vec4 factors[MATERIAL_FACTOR_UNIFORM_VEC4_COUNT];

        // unpacked, .xy material id,
        BASE_NS::Math::UVec4 indices { 0u, 0u, 0u, 0u };
    };

    struct MaterialPackedUniforms {
        // Texture transforms as half4 ({2x2 mat}, {2d offset, unused})
        // 0: rotScaleTrans baseColor
        // 1: rotScaleTrans normal
        // 2: rotScaleTrans material
        // 3: rotScaleTrans emissive
        // 4: rotScaleTrans ao
        // 5: rotScaleTrans clearcoat
        // 6: rotScaleTrans clearcoat roughness
        // 7: rotScaleTrans clearcoat normal
        // 8: rotScaleTrans sheen
        // 9: rotScaleTrans transmission
        // 10: rotScaleTrans specular

        //
        // 12: alphaCutoff, unused, useTexcoord | hasTransform
        BASE_NS::Math::UVec4 packed[MATERIAL_PACKED_UNIFORM_UVEC4_COUNT];

        // unpacked, .xy material id,
        BASE_NS::Math::UVec4 indices { 0u, 0u, 0u, 0u };
    };

    /*
     * Combined uniforms for factors and packed transforms
     **/
    struct AllMaterialUniforms {
        /** Material factor uniforms */
        MaterialUniforms factors;
        /** Material packed transforms */
        MaterialPackedUniforms transforms;
    };

    /** Material handles with handle reference (used as inputs)
     */
    struct MaterialHandlesWithHandleReference {
        /** Source images for different texture types */
        RENDER_NS::RenderHandleReference images[MATERIAL_TEXTURE_COUNT];
        /** Samplers for different texture types */
        RENDER_NS::RenderHandleReference samplers[MATERIAL_TEXTURE_COUNT];
    };
    /** Material handles with raw handles
     */
    struct MaterialHandles {
        /** Source images for different texture types */
        RENDER_NS::RenderHandle images[MATERIAL_TEXTURE_COUNT];
        /** Samplers for different texture types */
        RENDER_NS::RenderHandle samplers[MATERIAL_TEXTURE_COUNT];
    };

    /** Material shader with handle references (used as inputs)
     */
    struct MaterialShaderWithHandleReference {
        /** Shader to be used. (If invalid, a default is chosen by the default material renderer) */
        RENDER_NS::RenderHandleReference shader;
        /** Shader graphics state to be used. (If invalid, a default is chosen by the default material renderer) */
        RENDER_NS::RenderHandleReference graphicsState;
    };

    /** Material data
     */
    struct MaterialData {
        /** Render material type */
        RenderMaterialType materialType { RenderMaterialType::METALLIC_ROUGHNESS };
        /** Render extra rendering flags */
        RenderExtraRenderingFlags extraMaterialRenderingFlags { 0u };
        /** Render material flags, same as MaterialComponent */
        RenderMaterialFlags renderMaterialFlags { RENDER_MATERIAL_SHADOW_RECEIVER_BIT |
                                                  RENDER_MATERIAL_SHADOW_CASTER_BIT |
                                                  RENDER_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT |
                                                  RENDER_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT };

        /** Custom render slot id */
        uint32_t customRenderSlotId { ~0u };
        /** Material shader */
        MaterialShaderWithHandleReference materialShader;
        /** Depth shader */
        MaterialShaderWithHandleReference depthShader;

        /** Render sort layer id. Valid values are 0 - 63 */
        uint8_t renderSortLayer { RenderSceneDataConstants::DEFAULT_RENDER_SORT_LAYER_ID };
        /** Render sort layer id. Valid values are 0 - 255 */
        uint8_t renderSortLayerOrder { 0 };
    };

    /** Submesh material flags
     */
    struct SubmeshMaterialFlags {
        /** Material type */
        RenderMaterialType materialType { RenderMaterialType::METALLIC_ROUGHNESS };
        /* Render submesh's submesh flags, dublicated here for convenience */
        RenderSubmeshFlags submeshFlags { 0u };
        /** Extra material rendering flags */
        RenderExtraRenderingFlags extraMaterialRenderingFlags { 0u };

        /** Render material flags */
        RenderMaterialFlags renderMaterialFlags { 0u };

        /** 32 bit hash based on the variables above */
        uint32_t renderHash { 0u };
        /** 32 bit hash based on the variables above targeted for default depth rendering hashing */
        uint32_t renderDepthHash { 0u };
    };

    struct SlotMaterialData {
        /** Combined sort layer from render submesh */
        uint16_t combinedRenderSortLayer { 0u };
        /** Combined of render materials flags hash, material idx, shader id */
        uint32_t renderSortHash { 0u };
        /** Render material flags */
        RenderMaterialFlags renderMaterialFlags { 0u };
        /** Custom shader handle or invalid handle */
        RENDER_NS::RenderHandle shader;
        /** Custom graphics state handle or invalid handle */
        RENDER_NS::RenderHandle gfxState;
    };

    /** Object counts for rendering.
     */
    struct ObjectCounts {
        /** Mesh count, NOTE: is currently global */
        uint32_t meshCount { 0u };
        /** Submesh count */
        uint32_t submeshCount { 0u };
        /** Skin count */
        uint32_t skinCount { 0u };
        /** Material count (frame material count) */
        uint32_t materialCount { 0u };
        /** Unique material count */
        uint32_t uniqueMaterialCount { 0u };
    };

    /** Per mesh skin joint matrices.
     */
    struct JointMatrixData {
        /** Matrices */
        BASE_NS::Math::Mat4X4* data { nullptr };
        /** Matrix count */
        uint32_t count { 0 };
    };

    /** Material custom resources.
     */
    struct CustomResourceData {
        /** invalid handle if custom material shader not given.
         * With default material build-in render nodes must have compatible pipeline layout.
         * With custom render slots and custom render nodes can be anything.
         */
        RENDER_NS::RenderHandle shaderHandle;

        /** handle count */
        uint32_t resourceHandleCount { 0u };
        /** invalid handles if not given */
        RENDER_NS::RenderHandle resourceHandles[MAX_MATERIAL_CUSTOM_RESOURCE_COUNT];
    };

    /** Material slot types. Where
     */
    enum class MaterialSlotType : uint32_t {
        /** Basic opaque slot */
        SLOT_TYPE_OPAQUE = 0,
        /** Basic translucent slot */
        SLOT_TYPE_TRANSLUCENT = 1,
        /** Basic depth slot for shadows and depth pre-pass */
        SLOT_TYPE_DEPTH = 2,
    };
};

/** @ingroup group_render_irenderdatastoredefaultmaterial */
/** RenderDataStoreDefaultMaterial interface.
 * Not internally syncronized.
 * Default material can be fetched with material id
 * 0xFFFFffff and 0xFFFFFFFFffffffff
 */
class IRenderDataStoreDefaultMaterial : public RENDER_NS::IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "fdd9f86b-f5fc-45da-8832-41cbd649ed49" };

    ~IRenderDataStoreDefaultMaterial() override = default;

    /** Add mesh data.
     * @param meshData All mesh data.
     * @return Mesh index for submesh to use.
     */
    virtual uint32_t AddMeshData(const RenderMeshData& meshData) = 0;

    /** Update (or create) rendering material data.
     * Automatic hashing with id. (E.g. material entity id)
     * Final rendering material flags are per submesh (RenderDataDefaultMaterial::SubmeshMaterialFlags)
     * @param id Material component id. In typical ECS usage material entity id.
     * @param materialUniforms input material uniforms.
     * @param materialHandles raw GPU resource handles.
     * @param materialData Material data.
     * @return Returns material index.
     */
    virtual uint32_t UpdateMaterialData(const uint64_t id,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData) = 0;

    /** Update (or create) rendering material data.
     * Automatic hashing with id. (E.g. material entity id)
     * Final rendering material flags are per submesh (RenderDataDefaultMaterial::SubmeshMaterialFlags)
     * @param id Material component id. In typical ECS usage material entity id.
     * @param materialUniforms input material uniforms.
     * @param materialHandles raw GPU resource handles.
     * @param materialData Material data.
     * @param customPropertyData Custom property data per material.
     * @return Returns material index.
     */
    virtual uint32_t UpdateMaterialData(const uint64_t id,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customPropertyData) = 0;

    /** Update (or create) rendering material data.
     * Automatic hashing with id. (E.g. material entity id)
     * Final rendering material flags are per submesh (RenderDataDefaultMaterial::SubmeshMaterialFlags)
     * @param id Material component id. In typical ECS usage material entity id.
     * @param materialUniforms input material uniforms.
     * @param materialHandles raw GPU resource handles.
     * @param materialData Material data.
     * @param customPropertyData Custom property data per material.
     * @param customBindings Custom bindings data per material.
     * @return Returns material index.
     */
    virtual uint32_t UpdateMaterialData(const uint64_t id,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customPropertyData,
        const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customBindings) = 0;

    /** Destroy material rendering data explicitly.
     * NOTE: The references for the resources might be zero already at this point.
     * In typical cases should be automatically called by some ECS system when material is destroyed
     * @param id Material component id. In typical ECS usage material entity id.
     */
    virtual void DestroyMaterialData(const uint64_t id) = 0;

    /** Destroy material rendering data explicitly.
     * NOTE: The references for the resources might be zero already at this point.
     * In typical cases should be automatically called by some ECS system when material is destroyed
     * @param ids Array view of material component ids. In typical ECS usage material entity id.
     */
    virtual void DestroyMaterialData(const BASE_NS::array_view<uint64_t> ids) = 0;

    /** Add material and get material index and frame offset
     * @param id Material component id. In typical ECS usage material entity id.
     * @return RenderFrameMaterialIndices material index and frame offset
     */
    virtual RenderFrameMaterialIndices AddFrameMaterialData(uint64_t id) = 0;

    /** Add material and get material index and frame offset
     * @param id Material component id. In typical ECS usage material entity id.
     * @param instanceCount Instance count of the same material.
     * @return RenderFrameMaterialIndices material index and frame offset
     */
    virtual RenderFrameMaterialIndices AddFrameMaterialData(uint64_t id, uint32_t instanceCount) = 0;

    /** Add material and get material index and frame offset
     * Automatic hashing with id. (E.g. material entity id)
     * Final rendering material flags are per submesh (RenderDataDefaultMaterial::SubmeshMaterialFlags)
     * @param id Material component id. In typical ECS usage material entity id.
     * @return RenderFrameMaterialIndices material index and frame offset
     */
    virtual RenderFrameMaterialIndices AddFrameMaterialData(BASE_NS::array_view<const uint64_t> ids) = 0;

    /** Add material without an id to be rendered this frame.
     * Prefer using id based material updates.
     * The data is automatically cleared after the rendering has been processed.
     * @param materialUniforms input material uniforms.
     * @param materialHandles raw GPU resource handles.
     * @param materialData Material data.
     * @param customPropertyData Custom property data per material.
     * @param customBindings Custom bindings data per material.
     */
    virtual RenderFrameMaterialIndices AddFrameMaterialData(
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customPropertyData,
        const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customBindings) = 0;

    /** Add material instance data and get material index and frame offset
     * Final rendering material flags are per submesh (RenderDataDefaultMaterial::SubmeshMaterialFlags)
     * @param materialIndex Material index (from get material index or update).
     * @param frameOffset Frame offset for the base instance material.
     * @param instanceIndex Offset to base frame offset for this instance.
     * @return Material frame offset index for submesh to use.
     */
    virtual RenderFrameMaterialIndices AddFrameMaterialInstanceData(
        uint32_t materialIndex, uint32_t frameOffset, uint32_t instanceIndex) = 0;

    /** Get material index.
     * Default material can be fetched with material id
     * 0xFFFFffff and 0xFFFFFFFFffffffff
     * @param id Material component id. In typical ECS usage material entity id.
     * @return Material index to material array.
     */
    virtual uint32_t GetMaterialIndex(uint64_t id) const = 0;

    /** Get material frame indices for material arrays.
     * Returns all the material offsets which have been added.
     * Same material index might be in these offset multiple times due to instancing.
     * @return Material indices to material arrays.
     */
    virtual BASE_NS::array_view<const uint32_t> GetMaterialFrameIndices() const = 0;

    /** Add skin joint matrices and get an index for render submesh.
     * If skinJointMatrices.size() != previousSkinJointMatrices.size()
     * The skinJointMatrices are copied to previous frame buffer.
     * @param skinJointMatrices All skin joint matrices.
     * @param prevSkinJointMatrices All skin joint matrices for previous frame.
     * @return Skin joint matrices index for submesh to use. (if no skin joints are given
     * RenderSceneDataConstants::INVALID_INDEX is returned)
     */
    virtual uint32_t AddSkinJointMatrices(const BASE_NS::array_view<const BASE_NS::Math::Mat4X4> skinJointMatrices,
        const BASE_NS::array_view<const BASE_NS::Math::Mat4X4> prevSkinJointMatrices) = 0;

    /** Add submeshes safely with default material to rendering. Render slots are evaluated automatically.
     * @param submesh Submesh.
     */
    virtual void AddSubmesh(const RenderSubmeshWithHandleReference& submesh) = 0;

    /** Add submeshes safely with default material to rendering.
     * @param submesh Submesh.
     * @param renderSlotAndShaders All the render slots where the submesh is handled.
     */
    virtual void AddSubmesh(const RenderSubmeshWithHandleReference& submesh,
        const BASE_NS::array_view<const RENDER_NS::IShaderManager::RenderSlotData> renderSlotAndShaders) = 0;

    /** Set render slots for material types.
     * @param materialSlotType Material slot type.
     * @param renderSlotIds All render slot ids.
     */
    virtual void SetRenderSlots(const RenderDataDefaultMaterial::MaterialSlotType materialSlotType,
        const BASE_NS::array_view<const uint32_t> renderSlotIds) = 0;

    /** Get render slot mask for material type.
     * @param materialSlotType Material slot type.
     * @return Mask of render slot ids.
     */
    virtual uint64_t GetRenderSlotMask(const RenderDataDefaultMaterial::MaterialSlotType materialSlotType) const = 0;

    /** Get all object count which were send to rendering
     * @return ObjectCounts
     */
    virtual RenderDataDefaultMaterial::ObjectCounts GetObjectCounts() const = 0;

    /** Get list of mesh data
     */
    virtual BASE_NS::array_view<const RenderMeshData> GetMeshData() const = 0;

    /** Get list of mesh skin joint matrices
     */
    virtual BASE_NS::array_view<const RenderDataDefaultMaterial::JointMatrixData> GetMeshJointMatrices() const = 0;

    /** Get slot submesh indices
     * @param renderSlotId Index
     */
    virtual BASE_NS::array_view<const uint32_t> GetSlotSubmeshIndices(const uint32_t renderSlotId) const = 0;

    /** Get slot submesh shader handles
     * @param renderSlotId Index
     */
    virtual BASE_NS::array_view<const RenderDataDefaultMaterial::SlotMaterialData> GetSlotSubmeshMaterialData(
        const uint32_t renderSlotId) const = 0;

    /** Get object counts per render slot
     * @param renderSlotId Index
     */
    virtual RenderDataDefaultMaterial::ObjectCounts GetSlotObjectCounts(const uint32_t renderSlotId) const = 0;

    /** Get list of render submeshes
     */
    virtual BASE_NS::array_view<const RenderSubmesh> GetSubmeshes() const = 0;

    /** Get submesh joint matrix data
     * @param skinJointIndex Skin joint index from RenderSubmesh got from AddSkinJointMatrices().
     */
    virtual BASE_NS::array_view<const BASE_NS::Math::Mat4X4> GetSubmeshJointMatrixData(
        const uint32_t skinJointIndex) const = 0;

    /** Get material uniforms (per material) */
    virtual BASE_NS::array_view<const RenderDataDefaultMaterial::AllMaterialUniforms> GetMaterialUniforms() const = 0;
    /** Get material handles (per material) */
    virtual BASE_NS::array_view<const RenderDataDefaultMaterial::MaterialHandles> GetMaterialHandles() const = 0;
    /** Get material custom property data with material index.
     * @param materialIndex Index of material from RenderSubmesh and an index if going through e.g. material unforms
     */
    virtual BASE_NS::array_view<const uint8_t> GetMaterialCustomPropertyData(const uint32_t materialIndex) const = 0;
    /** Get submesh material flags (per submesh) */
    virtual BASE_NS::array_view<const RenderDataDefaultMaterial::SubmeshMaterialFlags>
    GetSubmeshMaterialFlags() const = 0;

    /** Get custom resource handles */
    virtual BASE_NS::array_view<const RenderDataDefaultMaterial::CustomResourceData>
    GetCustomResourceHandles() const = 0;

    /** Generate render hash (32 bits as RenderDataDefaultMaterial::SubmeshMaterialFlags::renderHash) */
    virtual uint32_t GenerateRenderHash(const RenderDataDefaultMaterial::SubmeshMaterialFlags& flags) const = 0;

    /** Add debug materials for all slot submeshes to be rendered this frame.
     * The data is automatically cleared after the rendering has been processed.
     * This should mainly be used for debugging purposes.
     * @param toSlotId the (debug) render slot which we add a single material for e.g. debugging.
     * @param fromSlotIds the common render slots with submeshes which we are rendering with additional materials.
     * @param materialUniforms input material uniforms.
     * @param materialHandles raw GPU resource handles.
     * @param materialData Material data.
     * @param customPropertyData Custom property data per material.
     * @param customBindings Custom bindings data per material.
     */
    virtual RenderFrameMaterialIndices AddRenderSlotSubmeshesFrameMaterialData(const uint32_t toSlotId,
        BASE_NS::array_view<const uint32_t> fromSlotIds,
        const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
        const RenderDataDefaultMaterial::MaterialData& materialData,
        const BASE_NS::array_view<const uint8_t> customPropertyData,
        const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customBindings) = 0;

protected:
    IRenderDataStoreDefaultMaterial() = default;
};
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_MATERIAL_H
