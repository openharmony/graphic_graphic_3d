/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
    }
}

inline constexpr uint32_t HashSubmeshMaterials(const RenderMaterialType materialType,
    const RenderMaterialFlags materialFlags, const RenderSubmeshFlags submeshFlags)
{
    return (((uint32_t)materialType << MATERIAL_TYPE_SHIFT) & MATERIAL_TYPE_MASK) |
           ((materialFlags << MATERIAL_FLAGS_SHIFT) & MATERIAL_FLAGS_MASK) | (submeshFlags & SUBMESH_FLAGS_MASK);
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
        allocator.allocators.emplace_back(make_unique<LinearAllocator>(byteSize, MEMORY_ALIGNMENT));
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

void UnpackMaterialUVec(const Math::UVec4& packed, Math::Vec4& up0, Math::Vec4& up1)
{
    {
        const Math::Vec2 upxy = Math::UnpackHalf2X16(packed.x);
        const Math::Vec2 upzw = Math::UnpackHalf2X16(packed.y);
        up0 = Math::Vec4(upxy.x, upxy.y, upzw.x, upzw.y);
    }
    {
        const Math::Vec2 upxy = Math::UnpackHalf2X16(packed.z);
        const Math::Vec2 upzw = Math::UnpackHalf2X16(packed.w);
        up1 = Math::Vec4(upxy.x, upxy.y, upzw.x, upzw.y);
    }
}

RenderDataDefaultMaterial::AllMaterialUniforms MaterialUniformsPackedFromInput(
    const RenderDataDefaultMaterial::InputMaterialUniforms& input)
{
    RenderDataDefaultMaterial::AllMaterialUniforms amu;
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
            constexpr auto identityRs = Math::Vec4(1.f, 0.f, 0.f, 1.f);
            const bool hasTransform = (translate.x != 0.f) || (translate.y != 0.f) || (rotateScale != identityRs);
            if (!hasTransform) {
                // this matches packing identityRs.xy, identityRs.zw, translation.xy, and 0,0
                constexpr auto identity = Math::UVec4(0x3c000000, 0x00003c00, 0x0, 0x0);
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
    materialIdToIndex_.clear();
    materialIdToCustomResourceIndex_.clear();
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
            submeshJointMatricesAllocator_.allocators.emplace_back(
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

    materialHandles_.emplace_back(materialHandles);
    materialAllUniforms_.emplace_back(MaterialUniformsPackedFromInput(materialUniforms));
    materialData_.emplace_back(materialData);
    auto& currMaterialData = materialData_.back();
    ExtentRenderMaterialFlagsForComplexity(currMaterialData.materialType, currMaterialData.renderMaterialFlags);

    MaterialCustomPropertyOffset mcpo;
    if (!customData.empty()) {
        const uint32_t maxByteSize = Math::min(static_cast<uint32_t>(customData.size_bytes()),
            RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_PROPERTY_BYTE_SIZE);
        mcpo.offset = static_cast<uint32_t>(materialCustomPropertyData_.size_in_bytes());
        mcpo.byteSize = maxByteSize;
        materialCustomPropertyData_.resize(materialCustomPropertyData_.size_in_bytes() + maxByteSize);
        CloneData(materialCustomPropertyData_.data(), materialCustomPropertyData_.size_in_bytes(), customData.data(),
            customData.size_bytes());
    }
    materialCustomPropertyOffsets_.emplace_back(mcpo);

    return materialIndex;
}

uint32_t RenderDataStoreDefaultMaterial::AddMaterialData(const uint64_t id,
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandles& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData, const array_view<const uint8_t> customData)
{
    CORE_STATIC_ASSERT(RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT == MaterialComponent::TEXTURE_COUNT);

    CORE_ASSERT(materialAllUniforms_.size() == materialData_.size());
    if (const auto iter = materialIdToIndex_.find(id); iter != materialIdToIndex_.cend()) {
        CORE_ASSERT(iter->second < static_cast<uint32_t>(materialAllUniforms_.size()));
        return iter->second;
    } else {
        const uint32_t materialIndex = AddMaterialData(materialUniforms, materialHandles, materialData, customData);
        materialIdToIndex_[id] = materialIndex;
        return materialIndex;
    }
}

uint32_t RenderDataStoreDefaultMaterial::AddMeshData(const RenderMeshData& meshData)
{
    const uint32_t meshIndex = static_cast<uint32_t>(meshData_.size());
    meshData_.emplace_back(meshData);
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
    const array_view<const RenderHandleReference> bindings)
{
    uint32_t index = RenderSceneDataConstants::INVALID_INDEX;
    if (!bindings.empty()) {
        index = static_cast<uint32_t>(customResourceData_.size());
        customResourceData_.push_back({});
        auto& dataRef = customResourceData_.back();
        const uint32_t maxCount = Math::min(
            static_cast<uint32_t>(bindings.size()), RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT);
        for (uint32_t idx = 0; idx < maxCount; ++idx) {
            if (bindings[idx]) {
                dataRef.resourceHandles[dataRef.resourceHandleCount++] = bindings[idx];
            }
        }
    }
    return index;
}

uint32_t RenderDataStoreDefaultMaterial::AddMaterialCustomResources(
    const uint64_t id, const array_view<const RenderHandleReference> bindings)
{
    if (const auto iter = materialIdToCustomResourceIndex_.find(id); iter != materialIdToCustomResourceIndex_.cend()) {
        CORE_ASSERT(iter->second < static_cast<uint32_t>(customResourceData_.size()));
        return iter->second;
    } else {
        const uint32_t customResIndex = AddMaterialCustomResources(bindings);
        materialIdToCustomResourceIndex_[id] = customResIndex;
        return customResIndex;
    }
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
    submeshes_.emplace_back(submesh);
    auto& currSubmesh = submeshes_.back();
    if (submesh.meshIndex >= static_cast<uint32_t>(meshData_.size())) {
        CORE_LOG_W("invalid mesh index (%u) given", submesh.meshIndex);
        currSubmesh.meshIndex = 0;
    }
    if ((submesh.submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) &&
        (submesh.skinJointIndex >= static_cast<uint32_t>(submeshJointMatrixIndices_.size()))) {
        CORE_LOG_W("invalid skin joint index (%u) given", submesh.skinJointIndex);
        currSubmesh.skinJointIndex = RenderSceneDataConstants::INVALID_INDEX;
    }

    RenderDataDefaultMaterial::MaterialData perMaterialData;
    const uint32_t materialCount = static_cast<uint32_t>(materialAllUniforms_.size());
    if (submesh.materialIndex < materialCount) {
        CORE_ASSERT(materialData_.size() == materialAllUniforms_.size());
        perMaterialData = materialData_[submesh.materialIndex];
    } else {
        // NOTE: shouldn't come here with basic usage
        currSubmesh.materialIndex = AddMaterialData({}, {}, {}, {});
        perMaterialData = materialData_[submesh.materialIndex];
    }

    ExtentRenderMaterialFlagsFromSubmeshValues(submesh.submeshFlags, perMaterialData.renderMaterialFlags);

    const uint32_t renderHash =
        HashSubmeshMaterials(perMaterialData.materialType, perMaterialData.renderMaterialFlags, submesh.submeshFlags);
    submeshMaterialFlags_.emplace_back(
        RenderDataDefaultMaterial::SubmeshMaterialFlags { perMaterialData.materialType, submesh.submeshFlags,
            perMaterialData.extraMaterialRenderingFlags, perMaterialData.renderMaterialFlags, renderHash });

    const uint16_t renderSortLayerHash = (static_cast<uint16_t>(submesh.renderSortLayer) << 8u) |
                                         (static_cast<uint16_t>(submesh.renderSortLayerOrder) & 0xffu);
    // add submeshs to slots
    for (const auto& slotRef : renderSlotAndShaders) {
        SlotSubmeshData& dataRef = slotToSubmeshIndices_[slotRef.renderSlotId];
        dataRef.indices.emplace_back(submeshIndex);
        // hash for sorting (material index is certain for the same material)
        // shader and gfx state is not needed, because it's already in the material, or they are defaults
        // inverse winding can affect the final graphics state but it's in renderHash
        // we has with material index, and render hash
        // id generation does not matter to us, because this is per frame
        uint32_t renderSortHash = submesh.materialIndex;
        HashCombine32Bit(renderSortHash, renderHash);
        dataRef.materialData.push_back({ renderSortLayerHash, renderSortHash, perMaterialData.renderMaterialFlags,
            slotRef.shader, slotRef.graphicsState });

        dataRef.objectCounts.submeshCount++;
        if (submesh.skinJointIndex != RenderSceneDataConstants::INVALID_INDEX) {
            dataRef.objectCounts.skinCount++;
        }
        if ((submesh.customResourcesIndex != RenderSceneDataConstants::INVALID_INDEX) &&
            (submesh.customResourcesIndex < static_cast<uint32_t>(customResourceData_.size()))) {
            customResourceData_[submesh.customResourcesIndex].shaderHandle = perMaterialData.materialShader.shader;
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
    if (materialIndex < static_cast<uint32_t>(materialCustomPropertyOffsets_.size())) {
        const auto& offsets = materialCustomPropertyOffsets_[materialIndex];
        CORE_ASSERT((offsets.offset + offsets.byteSize) <= materialCustomPropertyData_.size_in_bytes());
        return { &materialCustomPropertyData_[offsets.offset], offsets.byteSize };
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
    if (const auto iter = materialIdToIndex_.find(id); iter != materialIdToIndex_.cend()) {
        return iter->second;
    } else {
        return RenderSceneDataConstants::INVALID_INDEX;
    }
}

uint32_t RenderDataStoreDefaultMaterial::GetMaterialCustomResourceIndex(const uint64_t id) const
{
    if (const auto iter = materialIdToCustomResourceIndex_.find(id); iter != materialIdToCustomResourceIndex_.cend()) {
        return iter->second;
    } else {
        return RenderSceneDataConstants::INVALID_INDEX;
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
