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

#include "render_data_store_default_material.h"

#include <algorithm>
#include <cstdint>

#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/array_view.h>
#include <base/math/float_packer.h>
#include <core/log.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/resource_handle.h>

#include "render_data_store_default_material.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;

using RENDER_NS::IShaderManager;
using RENDER_NS::RenderHandleReference;

namespace {
// for packing defines
#include <3d/shaders/common/3d_dm_structures_common.h>
} // namespace
namespace {
constexpr uint32_t SHADER_DEFAULT_RENDER_SLOT_COUNT { 3u };

constexpr size_t MEMORY_ALIGNMENT { 64 };

static constexpr uint32_t MATERIAL_TYPE_SHIFT { 28u };
static constexpr uint32_t MATERIAL_TYPE_MASK { 0xF0000000u };
static constexpr uint32_t MATERIAL_FLAGS_SHIFT { 8u };
static constexpr uint32_t MATERIAL_FLAGS_MASK { 0x0FFFff00u };
static constexpr uint32_t SUBMESH_FLAGS_MASK { 0x00000ffu };

inline void HashCombine32Bit(uint32_t& seed, const uint32_t v)
{
    constexpr const uint32_t goldenRatio = 0x9e3779b9;
    constexpr const uint32_t rotl = 6U;
    constexpr const uint32_t rotr = 2U;
    seed ^= hash(v) + goldenRatio + (seed << rotl) + (seed >> rotr);
}

#if (CORE3D_VALIDATION_ENABLED == 1)
void ValidateSubmesh(
    const RenderSubmesh& submesh, const vector<RenderDataDefaultMaterial::CustomResourceData>& customResourceData)
{
    if (((submesh.submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) == 0) &&
        submesh.skinJointIndex != RenderSceneDataConstants::INVALID_INDEX) {
        CORE_LOG_W("CORE3D_VALIDATION: skin bit is not set for submesh flags");
    }
    if ((submesh.customResourcesIndex != RenderSceneDataConstants::INVALID_INDEX) &&
        (submesh.customResourcesIndex >= static_cast<uint32_t>(customResourceData.size()))) {
        CORE_LOG_W("CORE3D_VALIDATION: invalid custom resource index");
    }
}
#endif

inline Math::UVec4 PackMaterialUVec(const Math::Vec4& up0, const Math::Vec4& up1)
{
    return Math::UVec4 {
        Math::PackHalf2X16({ up0.x, up0.y }),
        Math::PackHalf2X16({ up0.z, up0.w }),
        Math::PackHalf2X16({ up1.x, up1.y }),
        Math::PackHalf2X16({ up1.z, up1.w }),
    };
}

inline void ExtentRenderMaterialFlagsFromSubmeshValues(const RenderSubmeshFlags submeshFlags, RenderMaterialFlags& rmf)
{
    // if there are normal maps and there are tangets, we allow the normal map flag
    if ((rmf & RenderMaterialFlagBits::RENDER_MATERIAL_NORMAL_MAP_BIT) &&
        (submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_TANGENTS_BIT)) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_NORMAL_MAP_BIT;
    } else {
        // remove flag if there were only normal maps but no tangents
        rmf &= (~RenderMaterialFlagBits::RENDER_MATERIAL_NORMAL_MAP_BIT);
    }
}

void ExtentRenderMaterialFlagsForComplexity(const RenderMaterialType materialType, RenderMaterialFlags& rmf)
{
    constexpr RenderMaterialFlags complexMask { RENDER_MATERIAL_CLEAR_COAT_BIT | RENDER_MATERIAL_TRANSMISSION_BIT |
                                                RENDER_MATERIAL_SHEEN_BIT | RENDER_MATERIAL_SPECULAR_BIT };
    if ((materialType == RenderMaterialType::CUSTOM) || (materialType == RenderMaterialType::CUSTOM_COMPLEX)) {
        rmf |= (materialType == RenderMaterialType::CUSTOM_COMPLEX) ? RENDER_MATERIAL_COMPLEX_BIT
                                                                    : RENDER_MATERIAL_BASIC_BIT;
        if (materialType < RenderMaterialType::CUSTOM) {
            rmf |= ((rmf & complexMask) || (materialType == RenderMaterialType::SPECULAR_GLOSSINESS))
                       ? RENDER_MATERIAL_COMPLEX_BIT
                       : RENDER_MATERIAL_BASIC_BIT;
        }
    } else {
        rmf |= ((rmf & complexMask) > 0) ? RENDER_MATERIAL_COMPLEX_BIT : RENDER_MATERIAL_BASIC_BIT;
    }
}

inline constexpr uint32_t HashSubmeshMaterials(const RenderMaterialType materialType,
    const RenderMaterialFlags materialFlags, const RenderSubmeshFlags submeshFlags)
{
    return (((uint32_t)materialType << MATERIAL_TYPE_SHIFT) & MATERIAL_TYPE_MASK) |
           ((materialFlags << MATERIAL_FLAGS_SHIFT) & MATERIAL_FLAGS_MASK) | (submeshFlags & SUBMESH_FLAGS_MASK);
}

inline uint64_t HashMaterialId(const uint64_t id, const uint32_t instanceCount)
{
    return Hash(id, static_cast<uint64_t>(instanceCount));
}

void* AllocateMatrixMemory(RenderDataStoreDefaultMaterial::LinearAllocatorStruct& allocator, const size_t byteSize)
{
    void* data = nullptr;
    if (!allocator.allocators.empty()) {
        data = allocator.allocators[allocator.currentIndex]->Allocate(byteSize);
    }

    if (data) {
        return data;
    } else { // current allocator is out of memory
        allocator.allocators.push_back(make_unique<LinearAllocator>(byteSize, MEMORY_ALIGNMENT));
        allocator.currentIndex = (uint32_t)(allocator.allocators.size() - 1);
        data = allocator.allocators[allocator.currentIndex]->Allocate(byteSize);
        CORE_ASSERT_MSG(data, "render data store default material allocation : out of memory");
        return data;
    }
}

Math::Mat4X4* AllocateMatrices(RenderDataStoreDefaultMaterial::LinearAllocatorStruct& allocator, const size_t count)
{
    const size_t byteSize = count * sizeof(Math::Mat4X4);
    return static_cast<Math::Mat4X4*>(AllocateMatrixMemory(allocator, byteSize));
}

RenderDataDefaultMaterial::AllMaterialUniforms MaterialUniformsPackedFromInput(
    const RenderDataDefaultMaterial::InputMaterialUniforms& input)
{
    static constexpr const RenderDataDefaultMaterial::AllMaterialUniforms initial {};
    RenderDataDefaultMaterial::AllMaterialUniforms amu = initial;
    // premultiplication needs to be handled already with some factors like baseColor
    for (uint32_t idx = 0; idx < RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT; ++idx) {
        amu.factors.factors[idx] = input.textureData[idx].factor;
    }
    uint32_t transformBits = 0;
    {
        constexpr auto supportedMaterials = RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT;
        for (uint32_t i = 0; i < supportedMaterials; ++i) {
            const auto& tex = input.textureData[i];

            /* TRS matrix for 2D transformations:
             * { scaleX * cos(rot),  scaleY * sin(rot), transX }
             * { scaleX * -sin(rot), scaleY * cos(rot), transY }
             * { 0,                  0,                 1      }
             */
            Math::Vec4 rotateScale;
            if (tex.rotation == 0.f) {
                rotateScale.x = tex.scale.x;
                rotateScale.w = tex.scale.y;
            } else {
                const float sinR = Math::sin(tex.rotation);
                const float cosR = Math::cos(tex.rotation);
                rotateScale = { tex.scale.x * cosR, tex.scale.y * sinR, tex.scale.x * -sinR, tex.scale.y * cosR };
            }
            const Math::Vec2 translate = { tex.translation.x, tex.translation.y };

            // set transform bit for texture index
            static constexpr const auto identityRs = Math::Vec4(1.f, 0.f, 0.f, 1.f);
            const bool hasTransform = (translate.x != 0.f) || (translate.y != 0.f) || (rotateScale != identityRs);
            if (!hasTransform) {
                // this matches packing identityRs.xy, identityRs.zw, translation.xy, and 0,0
                static constexpr const auto identity = Math::UVec4(0x3c000000, 0x00003c00, 0x0, 0x0);
                amu.transforms.packed[CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX + i] = identity;
            } else {
                // using PackMaterialUVec costs packing the last unused zeros
                amu.transforms.packed[CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX + i] =
                    Math::UVec4 { Math::PackHalf2X16({ rotateScale.x, rotateScale.y }),
                        Math::PackHalf2X16({ rotateScale.z, rotateScale.w }),
                        Math::PackHalf2X16({ translate.x, translate.y }), 0 };
                transformBits |= static_cast<uint32_t>(hasTransform) << i;
            }
        }
    }
    const uint32_t transformInfoBits = (input.texCoordSetBits << 16u) | transformBits; // 16: left remove 16 bits
    const Math::UVec4 indices = { uint32_t(input.id >> 32u), uint32_t(input.id & 0xFFFFffff), 0u, 0u };

    // finalize factors
    amu.factors.factors[RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT] = { input.alphaCutoff,
        *reinterpret_cast<const float*>(&transformInfoBits), 0.0f, 0.0f };
    amu.factors.indices = indices;
    // finalize transforms
    amu.transforms.packed[CORE_MATERIAL_PACK_ADDITIONAL_IDX] = { Math::PackHalf2X16({ input.alphaCutoff, 0.f }),
        transformInfoBits, 0, 0 };
    amu.transforms.indices = indices;
    return amu;
}
} // namespace

RenderDataStoreDefaultMaterial::RenderDataStoreDefaultMaterial(
    RENDER_NS::IRenderContext& renderContext, const string_view name)
    : name_(name), shaderMgr_(renderContext.GetDevice().GetShaderManager())
{
    GetDefaultRenderSlots();
}

void RenderDataStoreDefaultMaterial::PostRender()
{
    Clear();
}

void RenderDataStoreDefaultMaterial::Clear()
{
    submeshes_.clear();
    meshData_.clear();
    submeshJointMatrixIndices_.clear();
    customResourceData_.clear();

    materialAllUniforms_.clear();
    materialHandles_.clear();
    materialData_.clear();
    materialIdToIndices_.clear();
    materialCustomPropertyOffsets_.clear();
    materialCustomPropertyData_.clear();
    submeshMaterialFlags_.clear();

    for (auto& slotRef : slotToSubmeshIndices_) { // does not remove slots from use
        slotRef.second.indices.clear();
        slotRef.second.materialData.clear();
        slotRef.second.objectCounts = {};
    }

    if (!submeshJointMatricesAllocator_.allocators.empty()) {
        submeshJointMatricesAllocator_.currentIndex = 0;
        if (submeshJointMatricesAllocator_.allocators.size() == 1) { // size is good for this frame
            submeshJointMatricesAllocator_.allocators[submeshJointMatricesAllocator_.currentIndex]->Reset();
        } else if (submeshJointMatricesAllocator_.allocators.size() > 1) {
            size_t fullByteSize = 0;
            for (auto& ref : submeshJointMatricesAllocator_.allocators) {
                fullByteSize += ref->GetCurrentByteSize();
                ref.reset();
            }
            submeshJointMatricesAllocator_.allocators.clear();
            // create new single allocation for combined previous size and some extra bytes
            submeshJointMatricesAllocator_.allocators.push_back(
                make_unique<LinearAllocator>(fullByteSize, MEMORY_ALIGNMENT));
        }
    }

    // NOTE: re-fetch if default slots are invalid
    if (materialRenderSlots_.opaqueMask != 0) {
        GetDefaultRenderSlots();
    }
}

void RenderDataStoreDefaultMaterial::GetDefaultRenderSlots()
{
    materialRenderSlots_.defaultOpaqueRenderSlot =
        shaderMgr_.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
    materialRenderSlots_.opaqueMask = (1ull << uint64_t(materialRenderSlots_.defaultOpaqueRenderSlot));
    materialRenderSlots_.opaqueMask |=
        (1ull << uint64_t(shaderMgr_.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_DEFERRED_OPAQUE)));

    materialRenderSlots_.defaultTranslucentRenderSlot =
        shaderMgr_.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT);
    materialRenderSlots_.translucentMask = (1ull << uint64_t(materialRenderSlots_.defaultTranslucentRenderSlot));

    materialRenderSlots_.defaultDepthRenderSlot =
        shaderMgr_.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH);
    materialRenderSlots_.depthMask = (1ull << uint64_t(materialRenderSlots_.defaultDepthRenderSlot));
    materialRenderSlots_.depthMask |=
        (1ull << uint64_t(shaderMgr_.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH_VSM)));
}

uint32_t RenderDataStoreDefaultMaterial::AddMaterialData(
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandles& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData, const array_view<const uint8_t> customData)
{
    CORE_ASSERT(materialAllUniforms_.size() == materialData_.size());
    const uint32_t materialIndex = static_cast<uint32_t>(materialAllUniforms_.size());

    materialHandles_.push_back(materialHandles);
    materialAllUniforms_.push_back(MaterialUniformsPackedFromInput(materialUniforms));
    materialData_.push_back(materialData);
    auto& currMaterialData = materialData_.back();
    ExtentRenderMaterialFlagsForComplexity(currMaterialData.materialType, currMaterialData.renderMaterialFlags);

    MaterialCustomPropertyOffset mcpo;
    if (!customData.empty()) {
        const auto offset = static_cast<uint32_t>(materialCustomPropertyData_.size_in_bytes());
        const auto maxByteSize = Math::min(static_cast<uint32_t>(customData.size_bytes()),
            RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_PROPERTY_BYTE_SIZE);
        mcpo.offset = offset;
        mcpo.byteSize = maxByteSize;
        materialCustomPropertyData_.resize(offset + maxByteSize);
        CloneData(materialCustomPropertyData_.data() + offset, maxByteSize, customData.data(), maxByteSize);
    }
    materialCustomPropertyOffsets_.push_back(mcpo);

    return materialIndex;
}

uint32_t RenderDataStoreDefaultMaterial::AddMaterialData(const uint64_t id,
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandles& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData, const array_view<const uint8_t> customData)
{
    CORE_STATIC_ASSERT(RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT == MaterialComponent::TEXTURE_COUNT);

    CORE_ASSERT(materialAllUniforms_.size() == materialData_.size());
    auto searchId = HashMaterialId(id, static_cast<uint32_t>(1U));
    if (const auto iter = materialIdToIndices_.find(searchId); iter != materialIdToIndices_.cend()) {
        CORE_ASSERT(iter->second.materialIndex < static_cast<uint32_t>(materialAllUniforms_.size()));
        return iter->second.materialIndex;
    } else {
        const uint32_t materialIndex = AddMaterialData(materialUniforms, materialHandles, materialData, customData);
        materialIdToIndices_[searchId] = { materialIndex, RenderSceneDataConstants::INVALID_INDEX };
        return materialIndex;
    }
}

uint32_t RenderDataStoreDefaultMaterial::AllocateMaterials(uint64_t id, uint32_t instanceCount)
{
    if (!instanceCount) {
        return RenderSceneDataConstants::INVALID_INDEX;
    }
    auto searchId = HashMaterialId(id, instanceCount);
    if (const auto iter = materialIdToIndices_.find(searchId); iter != materialIdToIndices_.cend()) {
        CORE_ASSERT(iter->second.materialIndex < static_cast<uint32_t>(materialAllUniforms_.size()));
        // NOTE: should probably know how many instances have been created, insert if instanceCount if bigger, update
        // materialIdToIndex_.
        return iter->second.materialIndex;
    } else {
        const uint32_t materialIndex = static_cast<uint32_t>(materialAllUniforms_.size());
        materialHandles_.resize(materialIndex + instanceCount);
        materialAllUniforms_.resize(materialIndex + instanceCount);
        materialData_.resize(materialIndex + instanceCount);
        materialCustomPropertyOffsets_.resize(materialIndex + instanceCount);
        materialIdToIndices_[searchId] = { materialIndex, RenderSceneDataConstants::INVALID_INDEX };
        return materialIndex;
    }
}

void RenderDataStoreDefaultMaterial::AddInstanceMaterialData(uint32_t materialIndex, uint32_t materialInstanceIndex,
    uint32_t materialInstanceCount, const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandles& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData,
    const BASE_NS::array_view<const uint8_t> customPropertyData)
{
    if ((materialIndex + materialInstanceIndex + materialInstanceCount) <= materialHandles_.size()) {
        materialHandles_[materialIndex + materialInstanceIndex] = materialHandles;
        auto& currMaterialData = materialData_[materialIndex + materialInstanceIndex];
        currMaterialData = materialData;
        ExtentRenderMaterialFlagsForComplexity(currMaterialData.materialType, currMaterialData.renderMaterialFlags);

        // uniforms and custom property data
        AddInstanceMaterialData(
            materialIndex, materialInstanceIndex, materialInstanceCount, materialUniforms, customPropertyData);
    }
}

void RenderDataStoreDefaultMaterial::AddInstanceMaterialData(uint32_t materialIndex, uint32_t materialInstanceIndex,
    uint32_t materialInstanceCount, const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const BASE_NS::array_view<const uint8_t> customPropertyData)
{
    if ((materialIndex + materialInstanceIndex + materialInstanceCount) <= materialAllUniforms_.size()) {
        materialAllUniforms_[materialIndex + materialInstanceIndex] = MaterialUniformsPackedFromInput(materialUniforms);
        if (!customPropertyData.empty()) {
            const auto offset = static_cast<uint32_t>(materialCustomPropertyData_.size_in_bytes());
            const auto maxByteSize = Math::min(static_cast<uint32_t>(customPropertyData.size_bytes()),
                RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_PROPERTY_BYTE_SIZE);
            auto& mcpo = materialCustomPropertyOffsets_[materialIndex + materialInstanceIndex];
            mcpo.offset = offset;
            mcpo.byteSize = maxByteSize;
            materialCustomPropertyData_.resize(offset + maxByteSize);
            CloneData(materialCustomPropertyData_.data() + offset, maxByteSize, customPropertyData.data(), maxByteSize);
        }
        // duplicate the data, note handles are not duplicated
        const auto& baseUniforms = materialAllUniforms_[materialIndex + materialInstanceIndex];
        for (uint32_t idx = 1u; idx < materialInstanceCount; ++idx) {
            const uint32_t currMaterialIndex = materialIndex + materialInstanceIndex + idx;
            CORE_ASSERT(currMaterialIndex < static_cast<uint32_t>(materialAllUniforms_.size()));
            materialAllUniforms_[currMaterialIndex] = baseUniforms;
        }
    }
}

uint32_t RenderDataStoreDefaultMaterial::AddMeshData(const RenderMeshData& meshData)
{
    const uint32_t meshIndex = static_cast<uint32_t>(meshData_.size());
    meshData_.push_back(meshData);
    return meshIndex;
}

uint32_t RenderDataStoreDefaultMaterial::AddSkinJointMatrices(
    const array_view<const Math::Mat4X4> skinJointMatrices, const array_view<const Math::Mat4X4> prevSkinJointMatrices)
{
    // check max supported joint count
    const uint32_t jointCount =
        std::min(RenderDataDefaultMaterial::MAX_SKIN_MATRIX_COUNT, static_cast<uint32_t>(skinJointMatrices.size()));
    uint32_t skinJointIndex = RenderSceneDataConstants::INVALID_INDEX;
    if (jointCount > 0) {
        const uint32_t byteSize = sizeof(Math::Mat4X4) * jointCount;
        const uint32_t fullJointCount = jointCount * 2u;
        Math::Mat4X4* jointMatrixData = AllocateMatrices(submeshJointMatricesAllocator_, fullJointCount);
        if (jointMatrixData) {
            CloneData(jointMatrixData, byteSize, skinJointMatrices.data(), byteSize);
            if (skinJointMatrices.size() == prevSkinJointMatrices.size()) {
                CloneData(jointMatrixData + jointCount, byteSize, prevSkinJointMatrices.data(), byteSize);
            } else {
                // copy current to previous if given prevSkinJointMatrices is not valid
                CloneData(jointMatrixData + jointCount, byteSize, skinJointMatrices.data(), byteSize);
            }
        }
        skinJointIndex = static_cast<uint32_t>(submeshJointMatrixIndices_.size());
        submeshJointMatrixIndices_.push_back(
            RenderDataDefaultMaterial::JointMatrixData { jointMatrixData, fullJointCount });
    }
    return skinJointIndex;
}

uint32_t RenderDataStoreDefaultMaterial::AddMaterialCustomResources(
    const uint64_t id, const array_view<const RenderHandleReference> bindings)
{
    uint32_t customResIndex = RenderSceneDataConstants::INVALID_INDEX;
    auto searchId = HashMaterialId(id, static_cast<uint32_t>(1U));
    if (auto iter = materialIdToIndices_.find(searchId); iter != materialIdToIndices_.end()) {
        if (iter->second.materialCustomResourceIndex <= static_cast<uint32_t>(customResourceData_.size())) {
            customResIndex = iter->second.materialCustomResourceIndex;
        } else {
            if (!bindings.empty()) {
                customResIndex = static_cast<uint32_t>(customResourceData_.size());
                customResourceData_.push_back({});
                auto& dataRef = customResourceData_.back();
                const uint32_t maxCount = Math::min(static_cast<uint32_t>(bindings.size()),
                    RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT);
                for (uint32_t idx = 0; idx < maxCount; ++idx) {
                    if (bindings[idx]) {
                        dataRef.resourceHandles[dataRef.resourceHandleCount++] = bindings[idx];
                    }
                }
            }
            iter->second.materialCustomResourceIndex = customResIndex;
        }
    } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_ONCE_W("AddMaterialCustomResources_" + to_string(id),
            "CORE3D_VALIDATION: AddMaterialCustomResources cannot add custom resources to non-existing material");
#endif
    }
    return customResIndex;
}

void RenderDataStoreDefaultMaterial::AddSubmesh(const RenderSubmesh& submesh)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (submesh.materialIndex >= static_cast<uint32_t>(materialData_.size())) {
        CORE_LOG_W("CORE_VALIDATION: submesh cannot be sent to rendering without a valid materialIndex");
    }
#endif
    if (submesh.materialIndex < static_cast<uint32_t>(materialData_.size())) {
        uint32_t renderSlotCount = 0u;

        // default support for 3 render slots
        IShaderManager::RenderSlotData renderSlotData[SHADER_DEFAULT_RENDER_SLOT_COUNT] {};

        const auto& matData = materialData_[submesh.materialIndex];
        renderSlotData[0u].shader = matData.materialShader.shader;
        renderSlotData[0u].graphicsState = matData.materialShader.graphicsState;
        constexpr uint32_t INVALID_RENDER_SLOT_ID = ~0u;
        uint32_t renderSlotId = matData.customRenderSlotId;
        if ((renderSlotId == INVALID_RENDER_SLOT_ID) && renderSlotData[0].graphicsState) {
            renderSlotId = shaderMgr_.GetRenderSlotId(renderSlotData[0u].graphicsState);
        }
        if ((renderSlotId == INVALID_RENDER_SLOT_ID) && renderSlotData[0].shader) {
            renderSlotId = shaderMgr_.GetRenderSlotId(renderSlotData[0u].shader);
        }
        // if all fails, render as opaque
        if (renderSlotId == INVALID_RENDER_SLOT_ID) {
            renderSlotId = materialRenderSlots_.defaultOpaqueRenderSlot;
        }
        renderSlotData[renderSlotCount].renderSlotId = renderSlotId;
        renderSlotCount++;

        if (matData.renderMaterialFlags & RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_CASTER_BIT) {
            renderSlotData[renderSlotCount].renderSlotId = materialRenderSlots_.defaultDepthRenderSlot;
            renderSlotData[renderSlotCount].shader = matData.depthShader.shader;
            renderSlotData[renderSlotCount].graphicsState = matData.depthShader.graphicsState;
            renderSlotCount++;
        }
        AddSubmesh(submesh, { renderSlotData, renderSlotCount });
    }
}

void RenderDataStoreDefaultMaterial::AddSubmesh(
    const RenderSubmesh& submesh, const array_view<const IShaderManager::RenderSlotData> renderSlotAndShaders)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    ValidateSubmesh(submesh, customResourceData_);
#endif

    const uint32_t submeshIndex = static_cast<uint32_t>(submeshes_.size());
    submeshes_.push_back(submesh);
    auto& currSubmesh = submeshes_.back();
    if (currSubmesh.meshIndex >= static_cast<uint32_t>(meshData_.size())) {
        CORE_LOG_W("invalid mesh index (%u) given", currSubmesh.meshIndex);
        currSubmesh.meshIndex = 0;
    }
    if ((currSubmesh.submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) &&
        (currSubmesh.skinJointIndex >= static_cast<uint32_t>(submeshJointMatrixIndices_.size()))) {
        CORE_LOG_W("invalid skin joint index (%u) given", currSubmesh.skinJointIndex);
        currSubmesh.skinJointIndex = RenderSceneDataConstants::INVALID_INDEX;
    }

    if (currSubmesh.materialIndex >= static_cast<uint32_t>(materialAllUniforms_.size())) {
        // NOTE: shouldn't come here with basic usage
        currSubmesh.materialIndex = AddMaterialData({}, {}, {}, {});
    } else {
        CORE_ASSERT(materialData_.size() == materialAllUniforms_.size());
    }
    const RenderDataDefaultMaterial::MaterialData& perMaterialData = materialData_[currSubmesh.materialIndex];

    auto submeshRenderMaterialFlags = perMaterialData.renderMaterialFlags;
    ExtentRenderMaterialFlagsFromSubmeshValues(currSubmesh.submeshFlags, submeshRenderMaterialFlags);

    const uint32_t renderHash =
        HashSubmeshMaterials(perMaterialData.materialType, submeshRenderMaterialFlags, currSubmesh.submeshFlags);
    submeshMaterialFlags_.push_back(
        RenderDataDefaultMaterial::SubmeshMaterialFlags { perMaterialData.materialType, currSubmesh.submeshFlags,
            perMaterialData.extraMaterialRenderingFlags, submeshRenderMaterialFlags, renderHash });

    const uint16_t renderSortLayerHash = (static_cast<uint16_t>(currSubmesh.renderSortLayer) << 8u) |
                                         (static_cast<uint16_t>(currSubmesh.renderSortLayerOrder) & 0xffu);
    // add submeshs to slots
    for (const auto& slotRef : renderSlotAndShaders) {
        SlotSubmeshData& dataRef = slotToSubmeshIndices_[slotRef.renderSlotId];
        dataRef.indices.push_back(submeshIndex);
        // hash for sorting (material index is certain for the same material)
        // shader and gfx state is not needed, because it's already in the material, or they are defaults
        // inverse winding can affect the final graphics state but it's in renderHash
        // we has with material index, and render hash
        // id generation does not matter to us, because this is per frame
        uint32_t renderSortHash = currSubmesh.materialIndex;
        HashCombine32Bit(renderSortHash, renderHash);
        dataRef.materialData.push_back(
            { renderSortLayerHash, renderSortHash, submeshRenderMaterialFlags, slotRef.shader, slotRef.graphicsState });

        dataRef.objectCounts.submeshCount++;
        if (currSubmesh.skinJointIndex != RenderSceneDataConstants::INVALID_INDEX) {
            dataRef.objectCounts.skinCount++;
        }
        if ((currSubmesh.customResourcesIndex != RenderSceneDataConstants::INVALID_INDEX) &&
            (currSubmesh.customResourcesIndex < static_cast<uint32_t>(customResourceData_.size()))) {
            customResourceData_[currSubmesh.customResourcesIndex].shaderHandle = perMaterialData.materialShader.shader;
        }
    }
}

void RenderDataStoreDefaultMaterial::SetRenderSlots(const RenderDataDefaultMaterial::MaterialSlotType materialSlotType,
    const BASE_NS::array_view<const uint32_t> renderSlotIds)
{
    uint64_t mask = 0;
    for (const auto renderSlotId : renderSlotIds) {
        mask |= 1ULL << uint64_t(renderSlotId);
    }
    if (materialSlotType == RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_OPAQUE) {
        materialRenderSlots_.opaqueMask = mask;
    } else if (materialSlotType == RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_TRANSLUCENT) {
        materialRenderSlots_.translucentMask = mask;
    } else if (materialSlotType == RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_DEPTH) {
        materialRenderSlots_.depthMask = mask;
    }
}

uint64_t RenderDataStoreDefaultMaterial::GetRenderSlotMask(
    const RenderDataDefaultMaterial::MaterialSlotType materialSlotType) const
{
    uint64_t mask = 0;
    if (materialSlotType == RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_OPAQUE) {
        mask = materialRenderSlots_.opaqueMask;
    } else if (materialSlotType == RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_TRANSLUCENT) {
        mask = materialRenderSlots_.translucentMask;
    } else if (materialSlotType == RenderDataDefaultMaterial::MaterialSlotType::SLOT_TYPE_DEPTH) {
        mask = materialRenderSlots_.depthMask;
    }
    return mask;
}

uint32_t RenderDataStoreDefaultMaterial::GetRenderSlotIdFromMasks(const uint32_t renderSlotId) const
{
    // compare to masks
    const uint64_t renderSlotMask = 1ULL << uint64_t(renderSlotId);
    uint32_t newRenderSlotId = renderSlotId;
    if (renderSlotMask & materialRenderSlots_.opaqueMask) {
        newRenderSlotId = materialRenderSlots_.defaultOpaqueRenderSlot;
    } else if (renderSlotMask & materialRenderSlots_.translucentMask) {
        newRenderSlotId = materialRenderSlots_.defaultTranslucentRenderSlot;
    } else if (renderSlotMask & materialRenderSlots_.depthMask) {
        newRenderSlotId = materialRenderSlots_.defaultDepthRenderSlot;
    }
    return newRenderSlotId;
}

array_view<const uint32_t> RenderDataStoreDefaultMaterial::GetSlotSubmeshIndices(const uint32_t renderSlotId) const
{
    const uint32_t rsId = GetRenderSlotIdFromMasks(renderSlotId);
    if (const auto iter = slotToSubmeshIndices_.find(rsId); iter != slotToSubmeshIndices_.cend()) {
        const auto& slotRef = iter->second;
        return slotRef.indices;
    } else {
        return {};
    }
}

array_view<const RenderDataDefaultMaterial::SlotMaterialData>
RenderDataStoreDefaultMaterial::GetSlotSubmeshMaterialData(const uint32_t renderSlotId) const
{
    const uint32_t rsId = GetRenderSlotIdFromMasks(renderSlotId);
    if (const auto iter = slotToSubmeshIndices_.find(rsId); iter != slotToSubmeshIndices_.cend()) {
        const auto& slotRef = iter->second;
        return slotRef.materialData;
    } else {
        return {};
    }
}

RenderDataDefaultMaterial::ObjectCounts RenderDataStoreDefaultMaterial::GetSlotObjectCounts(
    const uint32_t renderSlotId) const
{
    const uint32_t rsId = GetRenderSlotIdFromMasks(renderSlotId);
    if (const auto iter = slotToSubmeshIndices_.find(rsId); iter != slotToSubmeshIndices_.cend()) {
        return { static_cast<uint32_t>(meshData_.size()), iter->second.objectCounts.submeshCount,
            iter->second.objectCounts.skinCount, static_cast<uint32_t>(materialAllUniforms_.size()) };
    } else {
        return {};
    }
}

RenderDataDefaultMaterial::ObjectCounts RenderDataStoreDefaultMaterial::GetObjectCounts() const
{
    return {
        static_cast<uint32_t>(meshData_.size()),
        static_cast<uint32_t>(submeshes_.size()),
        static_cast<uint32_t>(submeshJointMatrixIndices_.size()),
        static_cast<uint32_t>(materialAllUniforms_.size()),
    };
}

array_view<const RenderSubmesh> RenderDataStoreDefaultMaterial::GetSubmeshes() const
{
    return submeshes_;
}

array_view<const RenderMeshData> RenderDataStoreDefaultMaterial::GetMeshData() const
{
    return meshData_;
}

array_view<const RenderDataDefaultMaterial::JointMatrixData>
RenderDataStoreDefaultMaterial::GetMeshJointMatrices() const
{
    return submeshJointMatrixIndices_;
}

array_view<const Math::Mat4X4> RenderDataStoreDefaultMaterial::GetSubmeshJointMatrixData(
    const uint32_t skinJointIndex) const
{
    if (skinJointIndex < static_cast<uint32_t>(submeshJointMatrixIndices_.size())) {
        const RenderDataDefaultMaterial::JointMatrixData& jm = submeshJointMatrixIndices_[skinJointIndex];
        return array_view<const Math::Mat4X4>(jm.data, static_cast<size_t>(jm.count));
    } else {
        return {};
    }
}

array_view<const RenderDataDefaultMaterial::AllMaterialUniforms>
RenderDataStoreDefaultMaterial::GetMaterialUniforms() const
{
    return materialAllUniforms_;
}

array_view<const RenderDataDefaultMaterial::MaterialHandles> RenderDataStoreDefaultMaterial::GetMaterialHandles() const
{
    return materialHandles_;
}

array_view<const uint8_t> RenderDataStoreDefaultMaterial::GetMaterialCustomPropertyData(
    const uint32_t materialIndex) const
{
    if ((!materialCustomPropertyData_.empty()) &&
        (materialIndex < static_cast<uint32_t>(materialCustomPropertyOffsets_.size()))) {
        const auto& offsets = materialCustomPropertyOffsets_[materialIndex];
        CORE_ASSERT((offsets.offset + offsets.byteSize) <= materialCustomPropertyData_.size_in_bytes());
        if ((offsets.offset + offsets.byteSize) <= materialCustomPropertyData_.size_in_bytes()) {
            return { &materialCustomPropertyData_[offsets.offset], offsets.byteSize };
        } else {
            return {};
        }
    } else {
        return {};
    }
}

array_view<const RenderDataDefaultMaterial::SubmeshMaterialFlags>
RenderDataStoreDefaultMaterial::GetSubmeshMaterialFlags() const
{
    return submeshMaterialFlags_;
}

array_view<const RenderDataDefaultMaterial::CustomResourceData>
RenderDataStoreDefaultMaterial::GetCustomResourceHandles() const
{
    return customResourceData_;
}

uint32_t RenderDataStoreDefaultMaterial::GenerateRenderHash(
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& flags) const
{
    return HashSubmeshMaterials(flags.materialType, flags.renderMaterialFlags, flags.submeshFlags);
}

uint32_t RenderDataStoreDefaultMaterial::GetMaterialIndex(const uint64_t id) const
{
    auto searchId = HashMaterialId(id, static_cast<uint32_t>(1U));
    if (const auto iter = materialIdToIndices_.find(searchId); iter != materialIdToIndices_.cend()) {
        return iter->second.materialIndex;
    } else {
        return RenderSceneDataConstants::INVALID_INDEX;
    }
}

uint32_t RenderDataStoreDefaultMaterial::GetMaterialCustomResourceIndex(const uint64_t id) const
{
    auto searchId = HashMaterialId(id, static_cast<uint32_t>(1U));
    if (const auto iter = materialIdToIndices_.find(searchId); iter != materialIdToIndices_.cend()) {
        return iter->second.materialCustomResourceIndex;
    } else {
        return RenderSceneDataConstants::INVALID_INDEX;
    }
}

RenderDataDefaultMaterial::MaterialIndices RenderDataStoreDefaultMaterial::GetMaterialIndices(const uint64_t id) const
{
    auto searchId = HashMaterialId(id, static_cast<uint32_t>(1U));
    if (const auto iter = materialIdToIndices_.find(searchId); iter != materialIdToIndices_.cend()) {
        return iter->second;
    } else {
        return {};
    }
}

RenderDataDefaultMaterial::MaterialIndices RenderDataStoreDefaultMaterial::GetMaterialIndices(
    const uint64_t id, const uint32_t instanceCount) const
{
    auto searchId = HashMaterialId(id, instanceCount);
    if (const auto iter = materialIdToIndices_.find(searchId); iter != materialIdToIndices_.cend()) {
        return iter->second;
    } else {
        return {};
    }
}

// for plugin / factory interface
RENDER_NS::IRenderDataStore* RenderDataStoreDefaultMaterial::Create(
    RENDER_NS::IRenderContext& renderContext, char const* name)
{
    return new RenderDataStoreDefaultMaterial(renderContext, name);
}

void RenderDataStoreDefaultMaterial::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreDefaultMaterial*>(instance);
}
CORE3D_END_NAMESPACE()
