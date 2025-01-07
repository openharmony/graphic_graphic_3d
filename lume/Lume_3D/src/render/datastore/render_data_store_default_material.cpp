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
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/resource_handle.h>

#include "render_data_store_default_material.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

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

static constexpr uint32_t COMBINED_GPU_INSTANCING_REMOVAL { ~(
    RenderMaterialFlagBits::RENDER_MATERIAL_GPU_INSTANCING_BIT |
    RenderMaterialFlagBits::RENDER_MATERIAL_GPU_INSTANCING_MATERIAL_BIT) };

static constexpr RenderDataDefaultMaterial::AllMaterialUniforms INIT_ALL_MATERIAL_UNIFORMS {};

inline void HashCombine32Bit(uint32_t& seed, const uint32_t v)
{
    constexpr const uint32_t goldenRatio = 0x9e3779b9;
    constexpr const uint32_t rotl = 6U;
    constexpr const uint32_t rotr = 2U;
    seed ^= hash(v) + goldenRatio + (seed << rotl) + (seed >> rotr);
}

inline constexpr RenderDataDefaultMaterial::MaterialHandles ConvertMaterialHandleReferences(
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& inputHandles,
    vector<RenderHandleReference>& references)
{
    RenderDataDefaultMaterial::MaterialHandles mh;
    for (uint32_t idx = 0; idx < RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT; ++idx) {
        if (inputHandles.images[idx]) {
            mh.images[idx] = inputHandles.images[idx].GetHandle();
            references.push_back(inputHandles.images[idx]);
        }
        if (inputHandles.samplers[idx]) {
            mh.samplers[idx] = inputHandles.samplers[idx].GetHandle();
            references.push_back(inputHandles.samplers[idx]);
        }
    }
    return mh;
}

inline constexpr uint16_t GetRenderSortLayerHash(const RenderSubmesh& submesh)
{
    // if submesh layers are defaults -> use material sort values (the defaults are the same)
    if ((submesh.layers.meshRenderSortLayer == RenderSceneDataConstants::DEFAULT_RENDER_SORT_LAYER_ID) &&
        (submesh.layers.meshRenderSortLayerOrder == 0)) {
        return (static_cast<uint16_t>(submesh.layers.materialRenderSortLayer) << 8u) |
               (static_cast<uint16_t>(submesh.layers.materialRenderSortLayerOrder) & 0xffu);
    } else {
        return (static_cast<uint16_t>(submesh.layers.meshRenderSortLayer) << 8u) |
               (static_cast<uint16_t>(submesh.layers.meshRenderSortLayerOrder) & 0xffu);
    }
}

RenderSubmesh ConvertRenderSubmeshInput(
    const RenderSubmeshWithHandleReference& input, vector<RenderHandleReference>& references)
{
    RenderSubmeshBuffers rsb;
    const auto& ib = input.buffers.indexBuffer;
    if (ib.bufferHandle) {
        references.push_back(ib.bufferHandle);
        rsb.indexBuffer = { ib.bufferHandle.GetHandle(), ib.bufferOffset, ib.byteSize, ib.indexType };
    }
    const auto& iargs = input.buffers.indirectArgsBuffer;
    if (iargs.bufferHandle) {
        references.push_back(iargs.bufferHandle);
        rsb.indirectArgsBuffer = { iargs.bufferHandle.GetHandle(), iargs.bufferOffset, iargs.byteSize };
    }
    const RenderHandle safetyBuffer = input.buffers.vertexBuffers[0U].bufferHandle.GetHandle();
    RenderHandle prevHandle {};
    for (uint32_t idx = 0; idx < RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT; ++idx) {
        const auto& vb = input.buffers.vertexBuffers[idx];
        // add safety handles to invalid buffers
        if (vb.bufferHandle) {
            rsb.vertexBuffers[idx] = { vb.bufferHandle.GetHandle(), vb.bufferOffset, vb.byteSize };
            // often we have the same buffer (GPU resource)
            if (rsb.vertexBuffers[idx].bufferHandle != prevHandle) {
                references.push_back(vb.bufferHandle);
            }
        } else {
            rsb.vertexBuffers[idx] = { safetyBuffer, 0U, 0U };
        }
        prevHandle = rsb.vertexBuffers[idx].bufferHandle;
    }
    // NOTE: we will get max amount of vertex buffers if there is at least one
    rsb.vertexBufferCount = RenderHandleUtil::IsValid(rsb.vertexBuffers[0U].bufferHandle)
                                ? RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT
                                : 0U;
    rsb.inputAssembly = input.buffers.inputAssembly;

    return RenderSubmesh { input.submeshFlags, input.renderSubmeshMaterialFlags, input.indices, input.layers,
        input.bounds, input.drawCommand, move(rsb) };
}

#if (CORE3D_VALIDATION_ENABLED == 1)
void ValidateSubmesh(const RenderSubmeshWithHandleReference& submesh)
{
    if (((submesh.submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) == 0) &&
        submesh.indices.skinJointIndex != RenderSceneDataConstants::INVALID_INDEX) {
        CORE_LOG_W("CORE3D_VALIDATION: skin bit is not set for submesh flags");
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

inline void ExtentRenderMaterialFlagsFromSubmeshValues(const RenderSubmeshFlags& submeshFlags, RenderMaterialFlags& rmf)
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

inline constexpr void PatchRenderMaterialSortLayers(
    const RenderDataDefaultMaterial::MaterialData& matData, RenderSubmeshLayers& layers)
{
    // batch render material sort layers if default
    if (layers.materialRenderSortLayer == RenderSceneDataConstants::DEFAULT_RENDER_SORT_LAYER_ID) {
        layers.materialRenderSortLayer = matData.renderSortLayer;
    }
    if (layers.materialRenderSortLayerOrder == 0U) {
        layers.materialRenderSortLayerOrder = matData.renderSortLayerOrder;
    }
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
    RenderDataDefaultMaterial::AllMaterialUniforms amu = INIT_ALL_MATERIAL_UNIFORMS;
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
            const Math::Vec4 rotateScale =
                [](const RenderDataDefaultMaterial::InputMaterialUniforms::TextureData& data) {
                    if (data.rotation == 0.f) {
                        return Math::Vec4 { data.scale.x, 0.f, 0.f, data.scale.y };
                    }
                    const float sinR = Math::sin(data.rotation);
                    const float cosR = Math::cos(data.rotation);
                    return Math::Vec4 { data.scale.x * cosR, data.scale.y * sinR, data.scale.x * -sinR,
                        data.scale.y * cosR };
                }(tex);

            // set transform bit for texture index
            static constexpr const auto identityRs = Math::Vec4(1.f, 0.f, 0.f, 1.f);
            const bool hasTransform =
                (tex.translation.x != 0.f) || (tex.translation.y != 0.f) || (rotateScale != identityRs);
            if (!hasTransform) {
                // this matches packing identityRs.xy, identityRs.zw, translation.xy, and 0,0
                static constexpr const auto identity = Math::UVec4(0x3c000000, 0x00003c00, 0x0, 0x0);
                amu.transforms.packed[CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX + i] = identity;
            } else {
                // using PackMaterialUVec costs packing the last unused zeros
                amu.transforms.packed[CORE_MATERIAL_PACK_TEX_BASE_COLOR_UV_IDX + i] =
                    Math::UVec4 { Math::PackHalf2X16({ rotateScale.x, rotateScale.y }),
                        Math::PackHalf2X16({ rotateScale.z, rotateScale.w }),
                        Math::PackHalf2X16({ tex.translation.x, tex.translation.y }), 0 };
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

void DestroyMaterialByIndex(const uint32_t index, RenderDataStoreDefaultMaterial::AllMaterialData& matData)
{
    // explicit clear
    if (index < matData.data.size()) {
        matData.data[index] = {};
        matData.allUniforms[index] = {};
        matData.handles[index] = {};
        matData.customPropertyData[index] = {};
        matData.customResourceData[index] = {};

        // add for re-use
        matData.availableIndices.push_back(index);
    }
}

uint32_t GetCertainMaterialIndex(const uint32_t index, const RenderDataStoreDefaultMaterial::AllMaterialData& matData)
{
    if (index >= static_cast<uint32_t>(matData.data.size())) {
        // built-in material in 0
        CORE_ASSERT(!matData.data.empty());
        if (!matData.data.empty()) {
            return 0;
        }
    }
    return index;
}

constexpr RenderDataDefaultMaterial::InputMaterialUniforms GetDefaultInputMaterialUniforms()
{
    CORE_STATIC_ASSERT(RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT == 11U);

    // needs to match material component for default data
    RenderDataDefaultMaterial::InputMaterialUniforms imu;
    imu.textureData[0U].factor = { 1.f, 1.f, 1.f, 1.f };   // base color opaque white
    imu.textureData[1U].factor = { 1.f, 0.f, 0.f, 0.f };   // normal scale 1
    imu.textureData[2U].factor = { 0.f, 1.f, 1.f, 0.04f }; // material (empty, roughness, metallic, reflectance)
    imu.textureData[3U].factor = { 0.f, 0.f, 0.f, 1.f };   // emissive 0
    imu.textureData[4U].factor = { 1.f, 0.f, 0.f, 0.f };   // ambient occlusion 1
    imu.textureData[5U].factor = { 0.f, 0.f, 0.f, 0.f };   // clearcoat intensity 0
    imu.textureData[6U].factor = { 0.f, 0.f, 0.f, 0.f };   // clearcoat roughness 0
    imu.textureData[7U].factor = { 1.f, 0.f, 0.f, 0.f };   // clearcoat normal scale 1
    imu.textureData[8U].factor = { 0.f, 0.f, 0.f, 0.f };   // sheen color black, roughness 0
    imu.textureData[9U].factor = { 0.f, 0.f, 0.f, 0.f };   // transmission 0
    imu.textureData[10U].factor = { 1.f, 1.f, 1.f, 1.f };  // specular white

    imu.id = 0xFFFFffffU;

    return imu;
}
} // namespace

RenderDataStoreDefaultMaterial::RenderDataStoreDefaultMaterial(
    RENDER_NS::IRenderContext& renderContext, const string_view name)
    : name_(name), gpuResourceMgr_(renderContext.GetDevice().GetGpuResourceManager()),
      shaderMgr_(renderContext.GetDevice().GetShaderManager())
{
    GetDefaultRenderSlots();

    // add default materials (~0U and max ~0ULL)
    const uint32_t materialIndex = AddMaterialDataImpl(~0U, GetDefaultInputMaterialUniforms(), {}, {}, {}, {});
    CORE_ASSERT(materialIndex == 0);
    matData_.materialIdToIndex.insert_or_assign(0xFFFFffff, materialIndex);
    matData_.materialIdToIndex.insert_or_assign(0xFFFFFFFFffffffff, materialIndex);
}

void RenderDataStoreDefaultMaterial::PostRender()
{
    Clear();
}

void RenderDataStoreDefaultMaterial::Clear()
{
    // NOTE: clear is at the moment called typically two times
    // this could be further optimized to know if clear has already been called

    // release references
    handleReferences_.clear();

    submeshes_.clear();
    meshData_.clear();
    submeshJointMatrixIndices_.clear();

    {
        // NOTE: material data is not cleared automatically anymore
        // we keep the data but update the resource references if data is used
        // separate destroy

        matData_.frameIndices.clear();
        matData_.idHashToFrameIndex.clear();

#if (CORE3D_VALIDATION_ENABLED == 1)
        vector<uint32_t> noIdRemoval;
#endif
        // check that we have id of material (if not, then it's a single frame (usually) debug material
        for (size_t idx = 0; idx < matData_.data.size(); ++idx) {
            if (matData_.data[idx].noId) {
                DestroyMaterialByIndex(static_cast<uint32_t>(idx), matData_);
#if (CORE3D_VALIDATION_ENABLED == 1)
                // should not have id
                noIdRemoval.push_back(static_cast<uint32_t>(idx));
#endif
            }
        }
#if (CORE3D_VALIDATION_ENABLED == 1)
        for (const auto& ref : matData_.materialIdToIndex) {
            for (const auto& noIdRef : noIdRemoval) {
                if (ref.second == noIdRef) {
                    CORE_LOG_E("CORE3D_VALIDATION: Material removal issue");
                }
            }
        }
        noIdRemoval.clear();
#endif
    }

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

void RenderDataStoreDefaultMaterial::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderDataStoreDefaultMaterial::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderDataStoreDefaultMaterial::GetRefCount()
{
    return refcnt_;
}

void RenderDataStoreDefaultMaterial::DestroyMaterialData(const uint64_t id)
{
    // explicit material destruction
    // NOTE: does not clear resource handle references for this frame

    CORE_ASSERT(matData_.allUniforms.size() == matData_.data.size());
    if (auto iter = matData_.materialIdToIndex.find(id); iter != matData_.materialIdToIndex.cend()) {
        CORE_ASSERT(iter->second < matData_.allUniforms.size());
        DestroyMaterialByIndex(iter->second, matData_);

        // destroy from material map
        matData_.materialIdToIndex.erase(iter);
    }
}

void RenderDataStoreDefaultMaterial::DestroyMaterialData(const BASE_NS::array_view<uint64_t> ids)
{
    for (const auto& idRef : ids) {
        DestroyMaterialData(idRef);
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

uint32_t RenderDataStoreDefaultMaterial::AddMaterialDataImpl(const uint32_t matIndex,
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData, const array_view<const uint8_t> customData,
    const array_view<const RenderHandleReference> customResourceData)
{
    uint32_t materialIndex = matIndex;
    // matData_.frameIndices can have higher counts)
    CORE_ASSERT(matData_.allUniforms.size() == matData_.data.size());
    CORE_ASSERT(matData_.handles.size() == matData_.customPropertyData.size());
    if ((materialIndex == ~0U) && (matData_.availableIndices.empty())) {
        // totally new indices and material
        materialIndex = static_cast<uint32_t>(matData_.allUniforms.size());
        matData_.handles.push_back(ConvertMaterialHandleReferences(materialHandles, handleReferences_));
        matData_.allUniforms.push_back(MaterialUniformsPackedFromInput(materialUniforms));
        matData_.customPropertyData.push_back({});
        matData_.customResourceData.push_back({});
        matData_.data.push_back({});
    } else {
        if ((materialIndex == ~0U) && (!matData_.availableIndices.empty())) {
            materialIndex = matData_.availableIndices.back();
            matData_.availableIndices.pop_back();
        }
        if (materialIndex >= matData_.allUniforms.size()) {
            CORE_ASSERT(true);
            return ~0U;
        }
        matData_.handles[materialIndex] = ConvertMaterialHandleReferences(materialHandles, handleReferences_);
        matData_.allUniforms[materialIndex] = MaterialUniformsPackedFromInput(materialUniforms);
        matData_.customPropertyData[materialIndex].data.clear();
        matData_.customResourceData[materialIndex] = {};
    }

    // NOTE: when material is added/updated, we need to keep the handle reference
    // the user might not have local references

    // not referenced yet this frame for rendering (false)
    matData_.data[materialIndex].frameReferenced = false;
    matData_.data[materialIndex].noId = false;
    auto& currMaterialData = matData_.data[materialIndex].md;
    currMaterialData = materialData;
    ExtentRenderMaterialFlagsForComplexity(currMaterialData.materialType, currMaterialData.renderMaterialFlags);

    if (!customData.empty()) {
        const auto maxByteSize = Math::min(static_cast<uint32_t>(customData.size_bytes()),
            RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_PROPERTY_BYTE_SIZE);
        auto& cpdRef = matData_.customPropertyData[materialIndex];
        cpdRef.data.resize(maxByteSize);
        CloneData(cpdRef.data.data(), maxByteSize, customData.data(), maxByteSize);
    }

    if (!customResourceData.empty()) {
        auto& dataRef = matData_.customResourceData[materialIndex];
        dataRef.resourceHandleCount = 0U;
        dataRef.shaderHandle = materialData.materialShader.shader.GetHandle();
        const uint32_t maxCount = Math::min(static_cast<uint32_t>(customResourceData.size()),
            RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT);
        for (uint32_t idx = 0; idx < maxCount; ++idx) {
            if (customResourceData[idx]) {
                // NOTE: when material is added/updated, we need to keep the handle reference
                // the user might not have local references
                handleReferences_.push_back(customResourceData[idx]);
                dataRef.resourceHandles[dataRef.resourceHandleCount++] = customResourceData[idx].GetHandle();
            }
        }
    }

    return materialIndex;
}

uint32_t RenderDataStoreDefaultMaterial::UpdateMaterialData(const uint64_t id,
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData, const array_view<const uint8_t> customData,
    const array_view<const RenderHandleReference> customBindings)
{
    CORE_STATIC_ASSERT(RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT == MaterialComponent::TEXTURE_COUNT);

    CORE_ASSERT(matData_.allUniforms.size() == matData_.data.size());
    if (const auto iter = matData_.materialIdToIndex.find(id); iter != matData_.materialIdToIndex.cend()) {
        CORE_ASSERT(iter->second < matData_.allUniforms.size());
        const uint32_t materialIndex = AddMaterialDataImpl(
            iter->second, materialUniforms, materialHandles, materialData, customData, customBindings);
        CORE_UNUSED(materialIndex);
        CORE_ASSERT(materialIndex == iter->second);
        return iter->second;
    } else {
        // create new
        const uint32_t materialIndex =
            AddMaterialDataImpl(~0U, materialUniforms, materialHandles, materialData, customData, customBindings);
        matData_.materialIdToIndex.insert_or_assign(id, materialIndex);
        return materialIndex;
    }
}

uint32_t RenderDataStoreDefaultMaterial::UpdateMaterialData(const uint64_t id,
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData, const array_view<const uint8_t> customData)
{
    return UpdateMaterialData(id, materialUniforms, materialHandles, materialData, customData, {});
}

uint32_t RenderDataStoreDefaultMaterial::UpdateMaterialData(const uint64_t id,
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData)
{
    return UpdateMaterialData(id, materialUniforms, materialHandles, materialData, {}, {});
}

RenderFrameMaterialIndices RenderDataStoreDefaultMaterial::AddFrameMaterialData(
    const uint64_t id, const uint32_t instanceCount)
{
    // we expect the material to be in data
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (const auto iter = matData_.materialIdToIndex.find(id); iter == matData_.materialIdToIndex.cend()) {
        const string name = string("AddFrameMaterialData" + BASE_NS::to_hex(id));
        CORE_LOG_ONCE_W(name, "AddFrameMaterialData id not updated prior add");
    }
#endif
    // NOTE: we need to update the reference counts for rendering time
    // material resource references are not kept alive when just updating the materials
    // with this approach the resources can be destroyed during the application time

    const uint64_t searchId = HashMaterialId(id, static_cast<uint32_t>(instanceCount));
    if (const auto iter = matData_.idHashToFrameIndex.find(searchId); iter != matData_.idHashToFrameIndex.cend()) {
        if (iter->second < static_cast<uint32_t>(matData_.frameIndices.size())) {
            // these have been already updated with reference counts, just return the indices
            return { matData_.frameIndices[iter->second], iter->second };
        }
    } else if (const auto matIter = matData_.materialIdToIndex.find(id); matIter != matData_.materialIdToIndex.cend()) {
        // append instance count amount
        const uint32_t frameMaterialOffset = static_cast<uint32_t>(matData_.frameIndices.size());
        matData_.idHashToFrameIndex.insert_or_assign(searchId, frameMaterialOffset);
        RenderFrameMaterialIndices rfmi { matIter->second, frameMaterialOffset };
        matData_.frameIndices.append(instanceCount, matIter->second); // add material index

        // update reference counts for this frame if needed
        UpdateFrameMaterialResourceReferences(matIter->second);

        return rfmi;
    }
    return {};
}

RenderFrameMaterialIndices RenderDataStoreDefaultMaterial::AddFrameMaterialData(const uint64_t id)
{
    return AddFrameMaterialData(id, 1U);
}

RenderFrameMaterialIndices RenderDataStoreDefaultMaterial::AddFrameMaterialData(
    const BASE_NS::array_view<const uint64_t> ids)
{
    if (!ids.empty()) {
        RenderFrameMaterialIndices rfmi = AddFrameMaterialData(ids[0U], 1U);
        for (size_t idx = 1; idx < ids.size(); ++idx) {
            AddFrameMaterialData(ids[idx], 1U);
        }
        return rfmi;
    } else {
        return {};
    }
}

RenderFrameMaterialIndices RenderDataStoreDefaultMaterial::AddFrameMaterialData(
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData,
    const BASE_NS::array_view<const uint8_t> customPropertyData,
    const BASE_NS::array_view<const RENDER_NS::RenderHandleReference> customBindings)
{
    // NOTE: this data is added as is
    // cannot be retrieved with id or anything
    // mostly used for some debug meshes etc.
    const uint32_t materialIndex =
        AddMaterialDataImpl(~0U, materialUniforms, materialHandles, materialData, customPropertyData, customBindings);
    // add for automatic destruction after rendering
    if (materialIndex < matData_.data.size()) {
        matData_.data[materialIndex].noId = true;
    }

    const uint32_t materialOffset = static_cast<uint32_t>(matData_.frameIndices.size());
    matData_.frameIndices.push_back(materialIndex); // material index
    return { materialIndex, materialOffset };
}

RenderFrameMaterialIndices RenderDataStoreDefaultMaterial::AddFrameMaterialInstanceData(
    uint32_t materialIndex, uint32_t frameOffset, uint32_t instanceIndex)
{
    const uint32_t frameFullOffset = frameOffset + instanceIndex;
    if (frameFullOffset < static_cast<uint32_t>(matData_.frameIndices.size())) {
        matData_.frameIndices[frameFullOffset] = materialIndex;
        return { materialIndex, frameFullOffset };
    }
    return {};
}

void RenderDataStoreDefaultMaterial::UpdateFrameMaterialResourceReferences(const uint32_t materialIndex)
{
    CORE_ASSERT(matData_.data.size() == matData_.handles.size());
    CORE_ASSERT(matData_.data.size() == matData_.customResourceData.size());
    if (materialIndex < static_cast<uint32_t>(matData_.handles.size())) {
        auto& matDataRef = matData_.data[materialIndex];
        if (!matDataRef.frameReferenced) {
            // add references
            matDataRef.frameReferenced = true;
            const auto& handles = matData_.handles[materialIndex];
            // NOTE: access to GPU resource manager is locking behaviour
            // this can be evaluated further and possibly add array_view for get to get all with a one lock
            for (uint32_t idx = 0; idx < RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT; ++idx) {
                if (RenderHandleUtil::IsValid(handles.images[idx])) {
                    handleReferences_.push_back(gpuResourceMgr_.Get(handles.images[idx]));
                }
                if (RenderHandleUtil::IsValid(handles.samplers[idx])) {
                    handleReferences_.push_back(gpuResourceMgr_.Get(handles.samplers[idx]));
                }
            }
            const auto& customHandles = matData_.customResourceData[materialIndex];
            for (uint32_t idx = 0; idx < RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT; ++idx) {
                if (RenderHandleUtil::IsValid(customHandles.resourceHandles[idx])) {
                    handleReferences_.push_back(gpuResourceMgr_.Get(customHandles.resourceHandles[idx]));
                }
            }
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

void RenderDataStoreDefaultMaterial::AddSubmesh(const RenderSubmeshWithHandleReference& submesh)
{
    const uint32_t materialIndex = GetCertainMaterialIndex(submesh.indices.materialIndex, matData_);
    if (materialIndex < static_cast<uint32_t>(matData_.data.size())) {
        uint32_t renderSlotCount = 0u;

        // default support for 3 render slots
        IShaderManager::RenderSlotData renderSlotData[SHADER_DEFAULT_RENDER_SLOT_COUNT] {};

        const auto& matData = matData_.data[materialIndex].md;
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

void RenderDataStoreDefaultMaterial::AddSubmesh(const RenderSubmeshWithHandleReference& submesh,
    const array_view<const IShaderManager::RenderSlotData> renderSlotAndShaders)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    ValidateSubmesh(submesh);
#endif
    const uint32_t submeshIndex = static_cast<uint32_t>(submeshes_.size());
    submeshes_.push_back(ConvertRenderSubmeshInput(submesh, handleReferences_));
    auto& currSubmesh = submeshes_.back();

    FillSubmeshImpl(renderSlotAndShaders, submeshIndex, currSubmesh);
}

void RenderDataStoreDefaultMaterial::FillSubmeshImpl(
    const array_view<const IShaderManager::RenderSlotData> renderSlotAndShaders, const uint32_t submeshIndex,
    RenderSubmesh& submesh)
{
    const uint32_t materialIndex = GetCertainMaterialIndex(submesh.indices.materialIndex, matData_);
    submesh.indices.materialIndex = materialIndex; // if invalid -> store the default material
    if (submesh.indices.meshIndex >= static_cast<uint32_t>(meshData_.size())) {
        CORE_LOG_W("invalid mesh index (%u) given", submesh.indices.meshIndex);
        submesh.indices.meshIndex = 0;
    }
    if ((submesh.submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) &&
        (submesh.indices.skinJointIndex >= static_cast<uint32_t>(submeshJointMatrixIndices_.size()))) {
        CORE_LOG_W("invalid skin joint index (%u) given", submesh.indices.skinJointIndex);
        submesh.indices.skinJointIndex = RenderSceneDataConstants::INVALID_INDEX;
    }

    if (submesh.indices.materialIndex >= static_cast<uint32_t>(matData_.allUniforms.size())) {
        // NOTE: shouldn't come here with basic usage
        RenderFrameMaterialIndices rfmi = AddFrameMaterialData({}, {}, {}, {}, {});
        submesh.indices.materialIndex = rfmi.index;
        submesh.indices.materialFrameOffset = rfmi.frameOffset;
    } else {
        CORE_ASSERT(matData_.data.size() == matData_.allUniforms.size());
    }
    const RenderDataDefaultMaterial::MaterialData& perMatData = matData_.data[submesh.indices.materialIndex].md;
    // batch render material sort layers if default values
    PatchRenderMaterialSortLayers(perMatData, submesh.layers);

    // combine with submesh specific flags
    RenderMaterialFlags submeshRenderMaterialFlags =
        perMatData.renderMaterialFlags | submesh.renderSubmeshMaterialFlags;
    // remove instancing related things if not available
    if (submesh.drawCommand.instanceCount) {
        submeshRenderMaterialFlags |= RenderMaterialFlagBits::RENDER_MATERIAL_GPU_INSTANCING_BIT;
    } else {
        submeshRenderMaterialFlags &= COMBINED_GPU_INSTANCING_REMOVAL;
    }
    ExtentRenderMaterialFlagsFromSubmeshValues(submesh.submeshFlags, submeshRenderMaterialFlags);

    const uint32_t renderHash =
        HashSubmeshMaterials(perMatData.materialType, submeshRenderMaterialFlags, submesh.submeshFlags);
    // depth optimized flags
    const uint32_t renderDepthHash = HashSubmeshMaterials(perMatData.materialType,
        submeshRenderMaterialFlags & RenderDataDefaultMaterial::RENDER_MATERIAL_DEPTH_FLAGS,
        submesh.submeshFlags & RenderDataDefaultMaterial::RENDER_SUBMESH_DEPTH_FLAGS);
    submeshMaterialFlags_.push_back(
        RenderDataDefaultMaterial::SubmeshMaterialFlags { perMatData.materialType, submesh.submeshFlags,
            perMatData.extraMaterialRenderingFlags, submeshRenderMaterialFlags, renderHash, renderDepthHash });

    const uint16_t renderSortLayerHash = GetRenderSortLayerHash(submesh);
    // add submeshs to slots
    for (const auto& slotRef : renderSlotAndShaders) {
        SlotSubmeshData& dataRef = slotToSubmeshIndices_[slotRef.renderSlotId];
        dataRef.indices.push_back(submeshIndex);
        // hash for sorting (material index is certain for the same material)
        // shader and gfx state is not needed, because it's already in the material, or they are defaults
        // inverse winding can affect the final graphics state but it's in renderHash
        // we hash with material index, and render hash
        // id generation does not matter to us, because this is per frame
        uint32_t renderSortHash = submesh.indices.materialIndex;
        HashCombine32Bit(renderSortHash, renderHash);
        dataRef.materialData.push_back({ renderSortLayerHash, renderSortHash, submeshRenderMaterialFlags,
            slotRef.shader.GetHandle(), slotRef.graphicsState.GetHandle() });

        dataRef.objectCounts.submeshCount++;
        if (submesh.indices.skinJointIndex != RenderSceneDataConstants::INVALID_INDEX) {
            dataRef.objectCounts.skinCount++;
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
    const uint64_t renderSlotMask = 1ULL << uint64_t(renderSlotId & 0x3F); // 63
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
        return {
            static_cast<uint32_t>(meshData_.size()),
            iter->second.objectCounts.submeshCount,
            iter->second.objectCounts.skinCount,
            static_cast<uint32_t>(matData_.frameIndices.size()),
            static_cast<uint32_t>(matData_.allUniforms.size()),
        };
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
        static_cast<uint32_t>(matData_.frameIndices.size()),
        static_cast<uint32_t>(matData_.allUniforms.size()),
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
    return matData_.allUniforms;
}

array_view<const uint32_t> RenderDataStoreDefaultMaterial::GetMaterialFrameIndices() const
{
    return matData_.frameIndices;
}

array_view<const RenderDataDefaultMaterial::MaterialHandles> RenderDataStoreDefaultMaterial::GetMaterialHandles() const
{
    return matData_.handles;
}

array_view<const uint8_t> RenderDataStoreDefaultMaterial::GetMaterialCustomPropertyData(
    const uint32_t materialIndex) const
{
    if (materialIndex < static_cast<uint32_t>(matData_.customPropertyData.size())) {
        return matData_.customPropertyData[materialIndex].data;
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
    return matData_.customResourceData;
}

uint32_t RenderDataStoreDefaultMaterial::GenerateRenderHash(
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& flags) const
{
    return HashSubmeshMaterials(flags.materialType, flags.renderMaterialFlags, flags.submeshFlags);
}

uint32_t RenderDataStoreDefaultMaterial::GetMaterialIndex(const uint64_t id) const
{
    if (const auto iter = matData_.materialIdToIndex.find(id); iter != matData_.materialIdToIndex.cend()) {
        return iter->second;
    } else {
        return RenderSceneDataConstants::INVALID_INDEX;
    }
}

RenderFrameMaterialIndices RenderDataStoreDefaultMaterial::AddRenderSlotSubmeshesFrameMaterialData(
    const uint32_t toSlotId, const array_view<const uint32_t> fromSlotIds,
    const RenderDataDefaultMaterial::InputMaterialUniforms& materialUniforms,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& materialHandles,
    const RenderDataDefaultMaterial::MaterialData& materialData, const array_view<const uint8_t> customPropertyData,
    const array_view<const RenderHandleReference> customBindings)
{
    const RenderFrameMaterialIndices rfmi =
        AddFrameMaterialData(materialUniforms, materialHandles, materialData, customPropertyData, customBindings);

    const IShaderManager::RenderSlotData rds { toSlotId, {}, {}, {}, {} };
    const array_view<const IShaderManager::RenderSlotData> rdsView = { &rds, 1U };
    for (uint32_t slotIdx = 0U; slotIdx < fromSlotIds.size(); ++slotIdx) {
        const uint32_t currSlotIdx = fromSlotIds[slotIdx];
        const auto slotSubmeshIndices = GetSlotSubmeshIndices(currSlotIdx);
        submeshes_.reserve(submeshes_.size() + slotSubmeshIndices.size());
        for (uint32_t ssIdx = 0U; ssIdx < static_cast<uint32_t>(slotSubmeshIndices.size()); ++ssIdx) {
            const uint32_t currSubmeshIdx = slotSubmeshIndices[ssIdx];
            if (currSubmeshIdx < static_cast<uint32_t>(submeshes_.size())) {
                // duplicate
                RenderSubmesh newSubmesh = submeshes_[currSubmeshIdx];
                newSubmesh.indices.materialIndex = rfmi.index;
                newSubmesh.indices.materialFrameOffset = rfmi.frameOffset;

                const uint32_t submeshIndex = static_cast<uint32_t>(submeshes_.size());
                submeshes_.push_back(newSubmesh);

                auto& currSubmesh = submeshes_.back();
                FillSubmeshImpl(rdsView, submeshIndex, currSubmesh);
            }
        }
    }
    return rfmi;
}

// for plugin / factory interface
refcnt_ptr<IRenderDataStore> RenderDataStoreDefaultMaterial::Create(
    RENDER_NS::IRenderContext& renderContext, char const* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreDefaultMaterial(renderContext, name));
}
CORE3D_END_NAMESPACE()
