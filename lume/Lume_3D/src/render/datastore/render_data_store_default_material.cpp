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

#include "render_data_store_default_material.h"

#include <algorithm>
#include <cstdint>

#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/array_view.h>
#include <base/math/float_packer.h>
#include <base/math/matrix_util.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store_default_acceleration_structure_staging.h>
#include <render/datastore/intf_render_data_store_manager.h>
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
constexpr size_t MEMORY_ALIGNMENT { 64 };
constexpr bool ENABLE_RT { true };

static constexpr uint32_t MATERIAL_TYPE_SHIFT { 28u };
static constexpr uint32_t MATERIAL_TYPE_MASK { 0xF0000000u };
static constexpr uint32_t MATERIAL_FLAGS_SHIFT { 8u };
static constexpr uint32_t MATERIAL_FLAGS_MASK { 0x0FFFff00u };
static constexpr uint32_t SUBMESH_FLAGS_MASK { 0x00000ffu };

static constexpr uint32_t COMBINED_GPU_INSTANCING_REMOVAL { ~(
    RenderMaterialFlagBits::RENDER_MATERIAL_GPU_INSTANCING_BIT |
    RenderMaterialFlagBits::RENDER_MATERIAL_GPU_INSTANCING_MATERIAL_BIT) };

static constexpr RenderDataDefaultMaterial::AllMaterialUniforms INIT_ALL_MATERIAL_UNIFORMS {};

static constexpr float FMAX_VAL_AABB { std::numeric_limits<float>::max() };

static constexpr string_view DS_ACCELERATION_STRUCTURE { "RenderDataStoreDefaultAccelerationStructureStaging" };

struct MinAndMax {
    BASE_NS::Math::Vec3 minAABB { FMAX_VAL_AABB, FMAX_VAL_AABB, FMAX_VAL_AABB };
    BASE_NS::Math::Vec3 maxAABB { -FMAX_VAL_AABB, -FMAX_VAL_AABB, -FMAX_VAL_AABB };
};

static constexpr RenderMeshBatchData DEFAULT_RENDER_MESH_BATCH_DATA {};

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

RenderSubmeshBuffers ConvertRenderSubmeshBuffers(
    const RenderSubmeshBuffersWithHandleReference& buffers, vector<RenderHandleReference>& handleReferences)
{
    // convert and add references
    RenderSubmeshBuffers rsb;
    const auto& ib = buffers.indexBuffer;
    if (ib.bufferHandle) {
        handleReferences.push_back(ib.bufferHandle);
        rsb.indexBuffer = { ib.bufferHandle.GetHandle(), ib.bufferOffset, ib.byteSize, ib.indexType };
    }
    const auto& iargs = buffers.indirectArgsBuffer;
    if (iargs.bufferHandle) {
        handleReferences.push_back(iargs.bufferHandle);
        rsb.indirectArgsBuffer = { iargs.bufferHandle.GetHandle(), iargs.bufferOffset, iargs.byteSize };
    }
    const RenderHandle safetyBuffer = buffers.vertexBuffers[0U].bufferHandle.GetHandle();
    RenderHandle prevHandle {};
    for (uint32_t idx = 0; idx < RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT; ++idx) {
        const auto& vb = buffers.vertexBuffers[idx];
        // add safety handles to invalid buffers
        if (vb.bufferHandle) {
            const RenderHandle currHandle = vb.bufferHandle.GetHandle();
            if (currHandle != prevHandle) {
                handleReferences.push_back(vb.bufferHandle);
            }
            rsb.vertexBuffers[idx] = { currHandle, vb.bufferOffset, vb.byteSize };
        } else {
            rsb.vertexBuffers[idx] = { safetyBuffer, 0U, 0U };
        }
        prevHandle = rsb.vertexBuffers[idx].bufferHandle;
    }
    // NOTE: we will get max amount of vertex buffers if there is at least one
    rsb.vertexBufferCount = RenderHandleUtil::IsValid(rsb.vertexBuffers[0U].bufferHandle)
                                ? RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT
                                : 0U;
    rsb.inputAssembly = buffers.inputAssembly;
    return rsb;
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

RenderSubmeshBounds GetSubmeshBounds(const RenderMeshAabbData renderMeshAabbData,
    const RenderMeshSkinData& meshSkinData, const RenderMeshBatchData renderMeshBatchData, const uint32_t submeshIdx,
    const bool useJoints, const uint32_t batchCount)
{
    RenderMinAndMax mam = [&]() {
        if (useJoints) {
            return meshSkinData.aabb;
        } else if (submeshIdx < renderMeshAabbData.submeshAabb.size()) {
            return renderMeshAabbData.submeshAabb[submeshIdx];
        } else {
            // NOTE: there should always be data
            return renderMeshAabbData.aabb;
        }
    }();
    if (batchCount > 0) {
        mam.minAabb = Math::min(mam.minAabb, renderMeshBatchData.aabb.minAabb);
        mam.maxAabb = Math::max(mam.maxAabb, renderMeshBatchData.aabb.maxAabb);
    }
    RenderSubmeshBounds bounds;
    bounds.worldCenter = (mam.minAabb + mam.maxAabb) * 0.5f;
    bounds.worldRadius = Math::sqrt(
        Math::max(Math::Distance2(bounds.worldCenter, mam.minAabb), Math::Distance2(bounds.worldCenter, mam.maxAabb)));
    return bounds;
}

void DestroyMaterialByIndex(const uint32_t index, RenderDataStoreDefaultMaterial::AllMaterialData& matData)
{
    if (index < matData.data.size()) {
        matData.data[index] = {};
        matData.allUniforms[index] = {};
        matData.handles[index] = {};
        matData.customPropertyData[index] = {};
        matData.customResourceData[index] = {};
        matData.renderSlotData[index] = {};

        // add for re-use
        matData.availableIndices.push_back(index);
    }
}

inline void DestroyMeshByIndex(const uint32_t index, RenderDataStoreDefaultMaterial::AllMeshData& meshData)
{
    if (index < meshData.data.size()) {
        meshData.data[index] = {};

        // add for re-use
        meshData.availableIndices.push_back(index);
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

RenderFrameMaterialIndices GetCertainMaterialIndices(
    const RenderFrameMaterialIndices& indices, const RenderDataStoreDefaultMaterial::AllMaterialData& matData)
{
    if (indices.index >= static_cast<uint32_t>(matData.data.size())) {
        // built-in material in 0
        CORE_ASSERT(!matData.data.empty());
        if (!matData.data.empty()) {
            return { 0, 0 };
        }
    }
    return indices;
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

void FillMaterialDefaultRenderSlotData(const IShaderManager& shaderMgr,
    const RenderDataStoreDefaultMaterial::MaterialRenderSlots& materialRenderSlots,
    const RenderDataDefaultMaterial::MaterialData& matData,
    RenderDataStoreDefaultMaterial::MaterialDefaultRenderSlotData& renderSlotData)
{
    renderSlotData.count = 0U;
    // default support for 2 render slots
    renderSlotData.data[0U].shader = matData.materialShader.shader;
    renderSlotData.data[0U].graphicsState = matData.materialShader.graphicsState;
    constexpr uint32_t INVALID_RENDER_SLOT_ID = ~0u;
    uint32_t renderSlotId = matData.customRenderSlotId;
    if ((renderSlotId == INVALID_RENDER_SLOT_ID) && renderSlotData.data[0U].graphicsState) {
        renderSlotId = shaderMgr.GetRenderSlotId(renderSlotData.data[0U].graphicsState);
    }
    if ((renderSlotId == INVALID_RENDER_SLOT_ID) && renderSlotData.data[0U].shader) {
        renderSlotId = shaderMgr.GetRenderSlotId(renderSlotData.data[0U].shader);
    }
    // if all fails, render as opaque
    if (renderSlotId == INVALID_RENDER_SLOT_ID) {
        renderSlotId = materialRenderSlots.defaultOpaqueRenderSlot;
    }
    renderSlotData.data[renderSlotData.count].renderSlotId = renderSlotId;
    renderSlotData.count++;

    if (matData.renderMaterialFlags & RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_CASTER_BIT) {
        renderSlotData.data[renderSlotData.count].renderSlotId = materialRenderSlots.defaultDepthRenderSlot;
        renderSlotData.data[renderSlotData.count].shader = matData.depthShader.shader;
        renderSlotData.data[renderSlotData.count].graphicsState = matData.depthShader.graphicsState;
        renderSlotData.count++;
    }
    CORE_ASSERT(renderSlotData.count <= RenderDataStoreDefaultMaterial::SHADER_DEFAULT_RENDER_SLOT_COUNT);
}
} // namespace

RenderDataStoreDefaultMaterial::RenderDataStoreDefaultMaterial(
    RENDER_NS::IRenderContext& renderContext, const string_view name)
    : name_(name), renderContext_(renderContext), gpuResourceMgr_(renderContext.GetDevice().GetGpuResourceManager()),
      shaderMgr_(renderContext.GetDevice().GetShaderManager())
{
    GetDefaultRenderSlots();

    // add default materials (~0U and max ~0ULL)
    const uint32_t materialIndex = AddMaterialDataImpl(~0U, GetDefaultInputMaterialUniforms(), {}, {}, {}, {});
    CORE_ASSERT(materialIndex == 0);
    matData_.materialIdToIndex.insert_or_assign(0xFFFFffff, materialIndex);
    matData_.materialIdToIndex.insert_or_assign(0xFFFFFFFFffffffff, materialIndex);

    // get flags for rt
    if constexpr (ENABLE_RT) {
        CORE_NS::IClassRegister *cr = renderContext.GetInterface<CORE_NS::IClassRegister>();
        if (cr) {
            IGraphicsContext* graphicsContext = CORE_NS::GetInstance<IGraphicsContext>(
                *cr, UID_GRAPHICS_CONTEXT);
            if (graphicsContext) {
                const IGraphicsContext::CreateInfo ci = graphicsContext->GetCreateInfo();
                if (ci.createFlags & IGraphicsContext::CreateInfo::ENABLE_ACCELERATION_STRUCTURES_BIT) {
                    rtEnabled_ = true;
                }
            }
        } else {
            CORE_LOG_E("get null ClassRegister");
        }
    }
}

void RenderDataStoreDefaultMaterial::PostRender()
{
    Clear();
}

void RenderDataStoreDefaultMaterial::Clear()
{
    // NOTE: clear is at the moment called typically two times
    // this could be further optimized to know if clear has already been called

    renderFrameObjectInfo_ = {};

    // release references
    handleReferences_.clear();

    meshData_.frameMeshData.clear();
    meshData_.frameSubmeshes.clear();
    meshData_.frameJointMatrixIndices.clear();
    meshData_.frameSubmeshMaterialFlags.clear();
    meshData_.frameMeshBlasInstanceData.clear();
    for (auto& meshRef : meshData_.data) {
        meshRef.frameReferenced = false; // clear for this frame processing
    }

    // NOTE: material data is not cleared automatically anymore
    // we keep the data but update the resource references if data is used
    // separate destroy
    {
        matData_.frameIndices.clear();
        matData_.idHashToFrameIndex.clear();

#if (CORE3D_VALIDATION_ENABLED == 1)
        vector<uint32_t> noIdRemoval;
#endif
        // check that we have id of material (if not, then it's a single frame (usually) debug material
        for (size_t idx = 0; idx < matData_.data.size(); ++idx) {
            auto& matRef = matData_.data[idx];
            matRef.frameReferenced = false;
            if (matRef.noId) {
                DestroyMaterialByIndex(static_cast<uint32_t>(idx), matData_);
#if (CORE3D_VALIDATION_ENABLED == 1)
                // should not have an id
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

    for (auto& slotRef : slotToSubmeshIndices_) { // does not remove slots from use
        slotRef.second.indices.clear();
        slotRef.second.materialData.clear();
        slotRef.second.objectCounts = {};
    }

    if (!meshJointMatricesAllocator_.allocators.empty()) {
        meshJointMatricesAllocator_.currentIndex = 0;
        if (meshJointMatricesAllocator_.allocators.size() == 1) { // size is good for this frame
            meshJointMatricesAllocator_.allocators[meshJointMatricesAllocator_.currentIndex]->Reset();
        } else if (meshJointMatricesAllocator_.allocators.size() > 1) {
            size_t fullByteSize = 0;
            for (auto& ref : meshJointMatricesAllocator_.allocators) {
                fullByteSize += ref->GetCurrentByteSize();
                ref.reset();
            }
            meshJointMatricesAllocator_.allocators.clear();
            // create new single allocation for combined previous size and some extra bytes
            meshJointMatricesAllocator_.allocators.push_back(
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

void RenderDataStoreDefaultMaterial::DestroyMeshData(const uint64_t id)
{
    if (auto iter = meshData_.meshIdToIndex.find(id); iter != meshData_.meshIdToIndex.cend()) {
        CORE_ASSERT(iter->second < meshData_.data.size());
        DestroyMeshByIndex(iter->second, meshData_);

        // destroy from mesh map
        meshData_.meshIdToIndex.erase(iter);
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
    CORE_ASSERT(matData_.renderSlotData.size() == matData_.customPropertyData.size());
    if ((materialIndex == ~0U) && (matData_.availableIndices.empty())) {
        // totally new indices and material
        materialIndex = static_cast<uint32_t>(matData_.allUniforms.size());
        matData_.handles.push_back(ConvertMaterialHandleReferences(materialHandles, handleReferences_));
        matData_.allUniforms.push_back(MaterialUniformsPackedFromInput(materialUniforms));
        matData_.customPropertyData.push_back({});
        matData_.customResourceData.push_back({});
        matData_.data.push_back({});
        matData_.renderSlotData.push_back({});
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
    auto& currRenderSlotData = matData_.renderSlotData[materialIndex];
    ExtentRenderMaterialFlagsForComplexity(currMaterialData.materialType, currMaterialData.renderMaterialFlags);
    FillMaterialDefaultRenderSlotData(shaderMgr_, materialRenderSlots_, currMaterialData, currRenderSlotData);

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
            // update render frame object info
            renderFrameObjectInfo_.renderMaterialFlags |= matDataRef.md.renderMaterialFlags;
        }
    }
}

uint32_t RenderDataStoreDefaultMaterial::AddMeshData(const RenderMeshData& meshData)
{
    // DEPRECATED support
    const uint32_t renderMeshIdx = static_cast<uint32_t>(meshData_.frameMeshData.size());
    meshData_.frameMeshData.push_back(meshData);
    return renderMeshIdx;
}

void RenderDataStoreDefaultMaterial::AddFrameRenderMeshDataAdditionalMaterial(
    const uint64_t matId, const uint32_t submeshFrameIndex, RenderSubmesh& renderSubmesh)
{
    RenderFrameMaterialIndices matIndices = GetCertainMaterialIndices(AddFrameMaterialData(matId, 1U), matData_);
    renderSubmesh.indices.materialIndex = matIndices.index;
    renderSubmesh.indices.materialFrameOffset = matIndices.frameOffset;
    const auto& matData = matData_.data[matIndices.index].md;
    PatchRenderMaterialSortLayers(matData, renderSubmesh.layers);

    const auto& matRenderSlotData = matData_.renderSlotData[matIndices.index];
    FillSubmeshImpl({ matRenderSlotData.data, matRenderSlotData.count }, submeshFrameIndex, renderSubmesh);
}

void RenderDataStoreDefaultMaterial::AddFrameRenderMeshDataImpl(const RenderMeshData& meshData,
    const RenderMeshAabbData& meshAabbData, const RenderMeshSkinData& meshSkinData,
    const RenderMeshBatchData& batchData, const uint32_t batchCount)
{
    // full render mesh process data
    if (auto iter = meshData_.meshIdToIndex.find(meshData.meshId); iter != meshData_.meshIdToIndex.end()) {
        CORE_ASSERT(iter->second < meshData_.data.size());
        if (iter->second < meshData_.data.size()) {
            auto& mesh = meshData_.data[iter->second];

            if constexpr (ENABLE_RT) {
                if (rtEnabled_) {
                    // update for tlas update
                    UpdateFrameMeshBlasInstanceData(mesh, meshData.world);
                }
            }

            const bool frameReferenced = mesh.frameReferenced;
            mesh.frameReferenced = true; // mark for referenced this frame

            const uint32_t skinJointIndex =
                AddFrameSkinJointMatricesImpl(meshSkinData.skinJointMatrices, meshSkinData.prevSkinJointMatrices);
            const bool useJoints = (skinJointIndex != RenderSceneDataConstants::INVALID_INDEX);
            const RenderSubmeshFlags rsAndFlags =
                useJoints ? 0xFFFFffff : (~RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT);
            const RenderSubmeshFlags rsOrFlags = (Math::Determinant(meshData.world) < 0.0f)
                                                     ? RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT
                                                     : 0U;

            // NOTE: When object is skinned we use the mesh bounding box for all the submeshes because currently
            // there is no way to know here which joints affect one specific renderSubmesh.

            const uint32_t renderMeshIdx = static_cast<uint32_t>(meshData_.frameMeshData.size());
            meshData_.frameMeshData.push_back(meshData);
            // process submeshes for rendering
            for (uint32_t submeshIdx = 0; submeshIdx < mesh.submeshes.size(); ++submeshIdx) {
                CORE_ASSERT(meshData_.frameSubmeshes.size() == meshData_.frameSubmeshMaterialFlags.size());

                const uint32_t submeshFrameIndex = static_cast<uint32_t>(meshData_.frameSubmeshes.size());
                meshData_.frameSubmeshes.push_back({});
                auto& rs = meshData_.frameSubmeshes.back();

                auto& submesh = mesh.submeshes[submeshIdx];
                submesh.frameReferenced = frameReferenced; // process if needed
                UpdateFrameMeshResourceReferences(submesh);

                rs.bounds = GetSubmeshBounds(meshAabbData, meshSkinData, batchData, submeshIdx, useJoints, batchCount);
                rs.buffers = submesh.sd.buffers;
                rs.drawCommand = submesh.sd.drawCommand;
                rs.submeshFlags = (submesh.sd.submeshFlags & rsAndFlags) | rsOrFlags;

                rs.layers.layerMask = meshData.layerMask;
                rs.layers.sceneId = static_cast<uint32_t>(meshData.sceneId); // ID is originally 32 bit.
                rs.layers.meshRenderSortLayer = submesh.sd.meshRenderSortLayer;
                rs.layers.meshRenderSortLayerOrder = submesh.sd.meshRenderSortLayerOrder;

                const uint32_t matInstanceCount = (batchCount > 0) ? rs.drawCommand.instanceCount : 1U;
                RenderFrameMaterialIndices matIndices =
                    GetCertainMaterialIndices(AddFrameMaterialData(submesh.sd.material, matInstanceCount), matData_);
                rs.indices = { meshData.id, mesh.md.meshId, submeshFrameIndex, renderMeshIdx, skinJointIndex,
                    matIndices.index, matIndices.frameOffset };

                rs.renderSubmeshMaterialFlags = (submesh.sd.renderSubmeshMaterialFlags | batchData.materialFlags);
                const auto& matData = matData_.data[matIndices.index].md;
                PatchRenderMaterialSortLayers(matData, rs.layers);

                const auto& matRenderSlotData = matData_.renderSlotData[matIndices.index];
                FillSubmeshImpl({ matRenderSlotData.data, matRenderSlotData.count }, submeshFrameIndex, rs);

                // add additional materials
                for (uint32_t mIdx = submesh.sd.additionalMaterialOffset; mIdx < submesh.sd.additionalMaterialCount;
                     ++mIdx) {
                    if (mIdx < mesh.submeshAdditionalMaterials.size()) {
                        const uint32_t addSubmeshFrameIndex = static_cast<uint32_t>(meshData_.frameSubmeshes.size());
                        meshData_.frameSubmeshes.push_back(rs); // start with the current setup
                        auto& addRs = meshData_.frameSubmeshes.back();
                        AddFrameRenderMeshDataAdditionalMaterial(
                            mesh.submeshAdditionalMaterials[mIdx], addSubmeshFrameIndex, addRs);
                    }
                }
            }
        }
    }
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (!meshData_.meshIdToIndex.contains(meshData.meshId)) {
        const string str = string("AddFrameRenderMeshDataImpl_") + to_string(meshData.meshId);
        CORE_LOG_ONCE_W(str, "CORE3D_VALIDATION: Mesh id not found for render mesh");
    }
#endif
}

void RenderDataStoreDefaultMaterial::AddFrameRenderMeshData(const array_view<const RenderMeshData> meshData,
    const RenderMeshAabbData& meshAabbData, const RenderMeshSkinData& meshSkinData,
    const RenderMeshBatchData& batchData)
{
    bool initialProcess = true;
    uint32_t batchIndex = 0;
    const uint32_t batchCount = (static_cast<uint32_t>(meshData.size()) - 1);
    for (const auto& meshDataRef : meshData) {
        if (auto iter = meshData_.meshIdToIndex.find(meshDataRef.meshId); iter != meshData_.meshIdToIndex.end()) {
            CORE_ASSERT(iter->second < meshData_.data.size());
            if (iter->second < meshData_.data.size()) {
                auto& mesh = meshData_.data[iter->second];

                if constexpr (ENABLE_RT) {
                    if (rtEnabled_) {
                        // update for tlas update
                        UpdateFrameMeshBlasInstanceData(mesh, meshDataRef.world);
                    }
                }
                const bool frameReferenced = mesh.frameReferenced;
                mesh.frameReferenced = true; // mark for referenced this frame
                uint32_t skinJointIndex = ~0U;
                RenderSubmeshFlags rsAndFlags = 0U;
                RenderSubmeshFlags rsOrFlags = 0U;
                bool useJoints = false;
                if (initialProcess) {
                    materialFrameOffsets_.clear();
                    materialFrameOffsets_.reserve(mesh.submeshes.size());
                    skinJointIndex = AddFrameSkinJointMatricesImpl(
                        meshSkinData.skinJointMatrices, meshSkinData.prevSkinJointMatrices);
                    useJoints = (skinJointIndex != RenderSceneDataConstants::INVALID_INDEX);
                    rsAndFlags = useJoints ? 0xFFFFffff : (~RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT);
                    rsOrFlags = (Math::Determinant(meshDataRef.world) < 0.0f)
                                    ? RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT
                                    : 0U;
                }
                // NOTE: When object is skinned we use the mesh bounding box for all the submeshes because currently
                // there is no way to know here which joints affect one specific renderSubmesh.

                const uint32_t renderMeshIdx = static_cast<uint32_t>(meshData_.frameMeshData.size());
                meshData_.frameMeshData.push_back(meshDataRef);
                for (uint32_t submeshIdx = 0; submeshIdx < mesh.submeshes.size(); ++submeshIdx) {
                    CORE_ASSERT(meshData_.frameSubmeshes.size() == meshData_.frameSubmeshMaterialFlags.size());
                    auto& submesh = mesh.submeshes[submeshIdx];
                    submesh.frameReferenced = frameReferenced; // process if needed
                    UpdateFrameMeshResourceReferences(submesh);

                    if (initialProcess) {
                        const uint32_t submeshFrameIndex = static_cast<uint32_t>(meshData_.frameSubmeshes.size());
                        meshData_.frameSubmeshes.push_back({});
                        auto& rs = meshData_.frameSubmeshes.back();

                        rs.bounds =
                            GetSubmeshBounds(meshAabbData, meshSkinData, batchData, submeshIdx, useJoints, batchCount);
                        rs.buffers = submesh.sd.buffers;
                        rs.drawCommand = submesh.sd.drawCommand;
                        // additional batch instance counts
                        rs.drawCommand.instanceCount += batchCount;
                        rs.submeshFlags = (submesh.sd.submeshFlags & rsAndFlags) | rsOrFlags;
                        rs.renderSubmeshMaterialFlags =
                            (submesh.sd.renderSubmeshMaterialFlags | batchData.materialFlags);

                        rs.layers.layerMask = meshDataRef.layerMask;
                        rs.layers.sceneId = static_cast<uint32_t>(meshDataRef.sceneId); // ID is originally 32 bit.
                        rs.layers.meshRenderSortLayer = submesh.sd.meshRenderSortLayer;
                        rs.layers.meshRenderSortLayerOrder = submesh.sd.meshRenderSortLayerOrder;

                        const uint32_t matInstanceCount = (batchCount > 0) ? rs.drawCommand.instanceCount : 1U;
                        RenderFrameMaterialIndices matIndices = GetCertainMaterialIndices(
                            AddFrameMaterialData(submesh.sd.material, matInstanceCount), matData_);
                        // add for the batch materials
                        materialFrameOffsets_.push_back(matIndices.frameOffset);
                        rs.indices = { meshDataRef.id, mesh.md.meshId, submeshFrameIndex, renderMeshIdx, skinJointIndex,
                            matIndices.index, matIndices.frameOffset };

                        const auto& matData = matData_.data[matIndices.index].md;
                        PatchRenderMaterialSortLayers(matData, rs.layers);

                        const auto& matRenderSlotData = matData_.renderSlotData[matIndices.index];
                        FillSubmeshImpl({ matRenderSlotData.data, matRenderSlotData.count }, submeshFrameIndex, rs);

                        // add additional materials
                        for (uint32_t mIdx = submesh.sd.additionalMaterialOffset;
                             mIdx < submesh.sd.additionalMaterialCount; ++mIdx) {
                            if (mIdx < mesh.submeshAdditionalMaterials.size()) {
                                const uint32_t addSubmeshFrameIndex =
                                    static_cast<uint32_t>(meshData_.frameSubmeshes.size());
                                meshData_.frameSubmeshes.push_back(rs); // start with the current setup
                                auto& addRs = meshData_.frameSubmeshes.back();
                                AddFrameRenderMeshDataAdditionalMaterial(
                                    mesh.submeshAdditionalMaterials[mIdx], addSubmeshFrameIndex, addRs);
                            }
                        }
                    } else {
                        // material offset for submesh
                        const uint32_t baseFrameOffset =
                            (submeshIdx < static_cast<uint32_t>(materialFrameOffsets_.size()))
                                ? materialFrameOffsets_[submeshIdx]
                                : 0U;
                        const uint32_t matIndex = GetMaterialIndex(submesh.sd.material);
                        AddFrameMaterialInstanceData(matIndex, baseFrameOffset, batchIndex);
                    }
                }
            }
        }
        // next batch index for submesh for offsets etc.
        batchIndex++;
        initialProcess = false;
    }
}

void RenderDataStoreDefaultMaterial::AddFrameRenderMeshData(
    const RenderMeshData& meshData, const RenderMeshAabbData& meshAabbData, const RenderMeshSkinData& meshSkinData)
{
    AddFrameRenderMeshDataImpl(meshData, meshAabbData, meshSkinData, DEFAULT_RENDER_MESH_BATCH_DATA, 0U);
}

void RenderDataStoreDefaultMaterial::UpdateFrameMeshResourceReferences(SubmeshDataContainer& submeshDataRef)
{
    if (!submeshDataRef.frameReferenced) {
        // add references
        submeshDataRef.frameReferenced = true;
        // NOTE: access to GPU resource manager is locking behaviour
        // this can be evaluated further and possibly add array_view for get to get all with a one lock
        auto AddValidReference = [&](const RenderHandle& handle) {
            if (RenderHandleUtil::IsValid(handle)) {
                handleReferences_.push_back(gpuResourceMgr_.Get(handle));
            }
        };
        AddValidReference(submeshDataRef.sd.buffers.indexBuffer.bufferHandle);
        AddValidReference(submeshDataRef.sd.buffers.indirectArgsBuffer.bufferHandle);
        const uint32_t vertexBufferCount = Math::min(
            submeshDataRef.sd.buffers.vertexBufferCount, RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
        for (uint32_t idx = 0; idx < vertexBufferCount; ++idx) {
            AddValidReference(submeshDataRef.sd.buffers.vertexBuffers[idx].bufferHandle);
        }
    }
}

void RenderDataStoreDefaultMaterial::UpdateMeshData(const uint64_t id, const MeshDataWithHandleReference& meshData)
{
    auto& md = meshData_;
    uint32_t index = ~0U;

    if (const auto iter = md.meshIdToIndex.find(id); iter != md.meshIdToIndex.cend()) {
        CORE_ASSERT(iter->second < md.data.size());
        index = iter->second;
    } else {
        // create new
        if (md.availableIndices.empty()) {
            index = static_cast<uint32_t>(md.data.size());
            md.data.push_back({});
        } else {
            // use available index
            index = md.availableIndices.back();
            md.availableIndices.pop_back();
        }
    }

    if (index < md.data.size()) {
        md.meshIdToIndex.insert_or_assign(id, index);

        auto& data = md.data[index];
        // clear
        const size_t baseAdditionalMaterialCount = data.submeshAdditionalMaterials.size();
        data.submeshAdditionalMaterials.clear();
        data.submeshes.clear();

        data.frameReferenced = true; // submeshes referenced for this frame below

        data.md.meshId = meshData.meshId;
        // NOTE: no aabb data
        data.submeshes.resize(meshData.submeshes.size());
        data.submeshAdditionalMaterials.reserve(baseAdditionalMaterialCount);
        for (size_t smIdx = 0; smIdx < data.submeshes.size(); ++smIdx) {
            const auto& readRef = meshData.submeshes[smIdx];
            auto& writeRef = data.submeshes[smIdx];
            // NOTE: no AABB
            writeRef.sd.submeshFlags = readRef.submeshFlags;
            writeRef.sd.renderSubmeshMaterialFlags = 0U;
            writeRef.sd.drawCommand = readRef.drawCommand;

            // adds references for this frame
            writeRef.sd.buffers = ConvertRenderSubmeshBuffers(readRef.buffers, handleReferences_);
            writeRef.frameReferenced = true;

            writeRef.sd.material = readRef.materialId;
            writeRef.sd.meshRenderSortLayer = readRef.meshRenderSortLayer;
            writeRef.sd.meshRenderSortLayerOrder = readRef.meshRenderSortLayerOrder;

            if (!readRef.additionalMaterials.empty()) {
                writeRef.sd.additionalMaterialOffset = static_cast<uint32_t>(data.submeshAdditionalMaterials.size());
                data.submeshAdditionalMaterials.resize(
                    data.submeshAdditionalMaterials.size() + readRef.additionalMaterials.size());
                writeRef.sd.additionalMaterialCount = static_cast<uint32_t>(data.submeshAdditionalMaterials.size()) -
                                                      writeRef.sd.additionalMaterialOffset;
            }
        }

        if constexpr (ENABLE_RT) {
            if (rtEnabled_) {
                UpdateMeshBlasData(data);
            }
        }
    }
}

void RenderDataStoreDefaultMaterial::UpdateMeshBlasData(MeshDataContainer& meshData)
{
    CORE_ASSERT(rtEnabled_);
    if (!dsAcceleration_) {
        dsAcceleration_ = refcnt_ptr<IRenderDataStoreDefaultAccelerationStructureStaging>(
            renderContext_.GetRenderDataStoreManager().GetRenderDataStore(DS_ACCELERATION_STRUCTURE));
    }
    if (dsAcceleration_) {
        // create blas and instance reference
        vector<AsGeometryTrianglesDataWithHandleReference> trianglesData;
        vector<AsGeometryTrianglesInfo> trianglesInfo;
        trianglesData.reserve(meshData.submeshes.size());
        trianglesInfo.reserve(meshData.submeshes.size());
        for (const auto& submesh : meshData.submeshes) {
            AsGeometryTrianglesDataWithHandleReference data;
            data.info.geometryFlags = GeometryFlagBits::GEOMETRY_OPAQUE_BIT;
            data.info.vertexFormat = BASE_NS::Format::BASE_FORMAT_R32G32B32_SFLOAT;
            data.info.vertexStride = sizeof(float) * 3U; // NOTE: hard coded
            data.info.indexType = submesh.sd.buffers.indexBuffer.indexType;
            data.info.maxVertex = submesh.sd.buffers.vertexBuffers[0].byteSize / data.info.vertexStride;
            data.info.indexCount =
                submesh.sd.buffers.indexBuffer.byteSize /
                ((data.info.indexType == IndexType::CORE_INDEX_TYPE_UINT32) ? sizeof(uint32_t) : sizeof(uint16_t));
            data.vertexData = { gpuResourceMgr_.Get(submesh.sd.buffers.vertexBuffers[0].bufferHandle),
                submesh.sd.buffers.vertexBuffers[0].bufferOffset };
            data.indexData = { gpuResourceMgr_.Get(submesh.sd.buffers.indexBuffer.bufferHandle),
                submesh.sd.buffers.indexBuffer.bufferOffset };
            trianglesData.push_back(data);
            trianglesInfo.push_back(data.info);
        }
        // create buffer for bottom level structures
        {
            AsBuildGeometryInfo geometryInfo;
            geometryInfo.type = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            geometryInfo.flags =
                BuildAccelerationStructureFlagBits::CORE_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT;
            AsBuildSizes asbs =
                renderContext_.GetDevice().GetAccelerationStructureBuildSizes(geometryInfo, trianglesInfo, {}, {});
            if (asbs.accelerationStructureSize) {
                const GpuAccelerationStructureDesc desc {
                    AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
                    GpuBufferDesc {
                        CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT,
                        CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS,
                        asbs.accelerationStructureSize,
                    }
                };
                meshData.blas.blas = gpuResourceMgr_.Create(desc);
            }
        }
        // send for build to bottom level acceleration structure staging
        {
            AsBuildGeometryDataWithHandleReference geometryData;
            geometryData.info.type = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
            geometryData.dstAccelerationStructure = meshData.blas.blas;
            dsAcceleration_->BuildAccelerationStructure(geometryData, trianglesData);
        }
        // instance reference which will be pushes to TLAS later
        meshData.blas.instance = { Math::IDENTITY_4X3, 0U, 0xFF, 0U,
            GeometryInstanceFlagBits::CORE_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT |
                GeometryInstanceFlagBits::CORE_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT,
            meshData.blas.blas };
    }
}

void RenderDataStoreDefaultMaterial::UpdateFrameMeshBlasInstanceData(
    const MeshDataContainer& meshData, const Math::Mat4X4& transform)
{
    CORE_ASSERT(rtEnabled_);
    const auto& instRef = meshData.blas.instance;

    AsInstance asInstance {};
    asInstance.transform.x = transform.x;
    asInstance.transform.y = transform.y;
    asInstance.transform.z = transform.z;
    asInstance.transform.w = transform.w;
    asInstance.accelerationStructure = instRef.accelerationStructure.GetHandle();
    asInstance.flags = instRef.flags;
    asInstance.instanceCustomIndex = instRef.instanceCustomIndex;
    asInstance.mask = instRef.mask;
    asInstance.shaderBindingTableOffset = instRef.shaderBindingTableOffset;
    meshData_.frameMeshBlasInstanceData.push_back(asInstance);
}

uint32_t RenderDataStoreDefaultMaterial::AddFrameSkinJointMatricesImpl(
    const array_view<const Math::Mat4X4> skinJointMatrices, const array_view<const Math::Mat4X4> prevSkinJointMatrices)
{
    // check max supported joint count
    const uint32_t jointCount =
        std::min(RenderDataDefaultMaterial::MAX_SKIN_MATRIX_COUNT, static_cast<uint32_t>(skinJointMatrices.size()));
    uint32_t skinJointIndex = RenderSceneDataConstants::INVALID_INDEX;
    if (jointCount > 0) {
        const uint32_t byteSize = sizeof(Math::Mat4X4) * jointCount;
        const uint32_t fullJointCount = jointCount * 2u;
        Math::Mat4X4* jointMatrixData = AllocateMatrices(meshJointMatricesAllocator_, fullJointCount);
        if (jointMatrixData) {
            CloneData(jointMatrixData, byteSize, skinJointMatrices.data(), byteSize);
            if (skinJointMatrices.size() == prevSkinJointMatrices.size()) {
                CloneData(jointMatrixData + jointCount, byteSize, prevSkinJointMatrices.data(), byteSize);
            } else {
                // copy current to previous if given prevSkinJointMatrices is not valid
                CloneData(jointMatrixData + jointCount, byteSize, skinJointMatrices.data(), byteSize);
            }
        }
        skinJointIndex = static_cast<uint32_t>(meshData_.frameJointMatrixIndices.size());
        meshData_.frameJointMatrixIndices.push_back(
            RenderDataDefaultMaterial::JointMatrixData { jointMatrixData, fullJointCount });
    }
    return skinJointIndex;
}

void RenderDataStoreDefaultMaterial::AddSubmesh(const RenderSubmeshWithHandleReference& submesh)
{
    const uint32_t materialIndex = GetCertainMaterialIndex(submesh.indices.materialIndex, matData_);
    if (materialIndex < static_cast<uint32_t>(matData_.data.size())) {
        CORE_ASSERT(matData_.data.size() == matData_.renderSlotData.size());
        const auto& rsData = matData_.renderSlotData[materialIndex];
        AddSubmesh(submesh, { rsData.data, rsData.count });
    }
}

void RenderDataStoreDefaultMaterial::AddSubmesh(const RenderSubmeshWithHandleReference& submesh,
    const array_view<const IShaderManager::RenderSlotData> renderSlotAndShaders)
{
    // DEPRECATED support
#if (CORE3D_VALIDATION_ENABLED == 1)
    ValidateSubmesh(submesh);
#endif
    const uint32_t submeshIndex = static_cast<uint32_t>(meshData_.frameSubmeshes.size());
    meshData_.frameSubmeshes.push_back(ConvertRenderSubmeshInput(submesh, handleReferences_));
    auto& currSubmesh = meshData_.frameSubmeshes.back();

    FillSubmeshImpl(renderSlotAndShaders, submeshIndex, currSubmesh);
}

void RenderDataStoreDefaultMaterial::FillSubmeshImpl(
    const array_view<const IShaderManager::RenderSlotData> renderSlotAndShaders, const uint32_t submeshIndex,
    RenderSubmesh& submesh)
{
    const uint32_t materialIndex = GetCertainMaterialIndex(submesh.indices.materialIndex, matData_);
    submesh.indices.materialIndex = materialIndex; // if invalid -> store the default material
    if (submesh.indices.meshIndex >= static_cast<uint32_t>(meshData_.frameMeshData.size())) {
        CORE_LOG_W("invalid mesh index (%u) given", submesh.indices.meshIndex);
        submesh.indices.meshIndex = 0;
    }
    if ((submesh.submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) &&
        (submesh.indices.skinJointIndex >= static_cast<uint32_t>(meshData_.frameJointMatrixIndices.size()))) {
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
    if (submesh.drawCommand.instanceCount > 1U) {
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
    meshData_.frameSubmeshMaterialFlags.push_back(
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
        // custom camera id for camera material effect
        const uint32_t cameraId = perMatData.customCameraId;
        dataRef.materialData.push_back({ slotRef.shader.GetHandle(), slotRef.graphicsState.GetHandle(),
            submeshRenderMaterialFlags, cameraId, renderSortHash, renderSortLayerHash });

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
        return RenderDataDefaultMaterial::ObjectCounts {
            static_cast<uint32_t>(meshData_.frameMeshData.size()),
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
    return RenderDataDefaultMaterial::ObjectCounts {
        static_cast<uint32_t>(meshData_.frameMeshData.size()),
        static_cast<uint32_t>(meshData_.frameSubmeshes.size()),
        static_cast<uint32_t>(meshData_.frameJointMatrixIndices.size()),
        static_cast<uint32_t>(matData_.frameIndices.size()),
        static_cast<uint32_t>(matData_.allUniforms.size()),
    };
}

array_view<const RenderSubmesh> RenderDataStoreDefaultMaterial::GetSubmeshes() const
{
    return meshData_.frameSubmeshes;
}

array_view<const RenderMeshData> RenderDataStoreDefaultMaterial::GetMeshData() const
{
    return meshData_.frameMeshData;
}

array_view<const RenderDataDefaultMaterial::JointMatrixData>
RenderDataStoreDefaultMaterial::GetMeshJointMatrices() const
{
    return meshData_.frameJointMatrixIndices;
}

array_view<const Math::Mat4X4> RenderDataStoreDefaultMaterial::GetSubmeshJointMatrixData(
    const uint32_t skinJointIndex) const
{
    if (skinJointIndex < static_cast<uint32_t>(meshData_.frameJointMatrixIndices.size())) {
        const RenderDataDefaultMaterial::JointMatrixData& jm = meshData_.frameJointMatrixIndices[skinJointIndex];
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
    return meshData_.frameSubmeshMaterialFlags;
}

array_view<const RenderDataDefaultMaterial::CustomResourceData>
RenderDataStoreDefaultMaterial::GetCustomResourceHandles() const
{
    return matData_.customResourceData;
}

RenderFrameObjectInfo RenderDataStoreDefaultMaterial::GetRenderFrameObjectInfo() const
{
    return renderFrameObjectInfo_;
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
        meshData_.frameSubmeshes.reserve(meshData_.frameSubmeshes.size() + slotSubmeshIndices.size());
        for (uint32_t ssIdx = 0U; ssIdx < static_cast<uint32_t>(slotSubmeshIndices.size()); ++ssIdx) {
            const uint32_t currSubmeshIdx = slotSubmeshIndices[ssIdx];
            if (currSubmeshIdx < static_cast<uint32_t>(meshData_.frameSubmeshes.size())) {
                // duplicate
                RenderSubmesh newSubmesh = meshData_.frameSubmeshes[currSubmeshIdx];
                newSubmesh.indices.materialIndex = rfmi.index;
                newSubmesh.indices.materialFrameOffset = rfmi.frameOffset;

                const uint32_t submeshIndex = static_cast<uint32_t>(meshData_.frameSubmeshes.size());
                meshData_.frameSubmeshes.push_back(newSubmesh);

                auto& currSubmesh = meshData_.frameSubmeshes.back();
                FillSubmeshImpl(rdsView, submeshIndex, currSubmesh);
            }
        }
    }
    return rfmi;
}

array_view<const AsInstance> RenderDataStoreDefaultMaterial::GetMeshBlasData() const
{
    return meshData_.frameMeshBlasInstanceData;
}

// for plugin / factory interface
refcnt_ptr<IRenderDataStore> RenderDataStoreDefaultMaterial::Create(
    RENDER_NS::IRenderContext& renderContext, char const* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreDefaultMaterial(renderContext, name));
}
CORE3D_END_NAMESPACE()
