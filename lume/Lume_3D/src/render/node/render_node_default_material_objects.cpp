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

#include "render_node_default_material_objects.h"

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/allocator.h>
#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/resource_handle.h>

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr uint32_t UBO_BIND_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };
constexpr uint32_t MIN_UBO_OBJECT_COUNT { CORE_UNIFORM_BUFFER_MAX_BIND_SIZE / UBO_BIND_OFFSET_ALIGNMENT };

constexpr size_t Align(size_t value, size_t align)
{
    if (value == 0) {
        return 0;
    }
    return ((value + align) / align) * align;
}
} // namespace

void RenderNodeDefaultMaterialObjects::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
    ProcessBuffers({ 1u, 1u, 1u, 1u });
}

void RenderNodeDefaultMaterialObjects::PreExecuteFrame()
{
    // re-create needed gpu resources
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const IRenderDataStoreDefaultMaterial* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
    // we need to have all buffers created (reason to have at least 1u)
    if (dataStoreMaterial) {
        const auto dsOc = dataStoreMaterial->GetObjectCounts();
        const ObjectCounts oc {
            Math::max(dsOc.meshCount, 1u),
            Math::max(dsOc.submeshCount, 1u),
            Math::max(dsOc.skinCount, 1u),
            Math::max(dsOc.materialCount, 1u),
        };
        ProcessBuffers(oc);
    } else {
        ProcessBuffers({ 1u, 1u, 1u, 1u });
    }
}

void RenderNodeDefaultMaterialObjects::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));

    if (dataStoreMaterial) {
        UpdateBuffers(*dataStoreMaterial);
    } else {
        CORE_LOG_E("invalid render data store in RenderNodeDefaultMaterialObjects");
    }
}

void RenderNodeDefaultMaterialObjects::UpdateBuffers(const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    UpdateMeshBuffer(dataStoreMaterial);
    UpdateSkinBuffer(dataStoreMaterial);
    UpdateMaterialBuffers(dataStoreMaterial);
}

void RenderNodeDefaultMaterialObjects::UpdateMeshBuffer(const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    // mesh data is copied to single buffer with UBO_BIND_OFFSET_ALIGNMENT alignments
    if (auto meshDataPtr = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.mesh.GetHandle())); meshDataPtr) {
        constexpr uint32_t meshByteSize = sizeof(DefaultMaterialSingleMeshStruct);
        CORE_STATIC_ASSERT(meshByteSize >= UBO_BIND_OFFSET_ALIGNMENT);
        CORE_STATIC_ASSERT(sizeof(RenderMeshData) == sizeof(DefaultMaterialSingleMeshStruct));
        const auto* meshDataPtrEnd = meshDataPtr + meshByteSize * objectCounts_.maxMeshCount;
        if (const auto meshData = dataStoreMaterial.GetMeshData(); !meshData.empty()) {
            // clone all at once, they are in order
            const size_t cloneByteSize = meshData.size_bytes();
            if (!CloneData(meshDataPtr, size_t(meshDataPtrEnd - meshDataPtr), meshData.data(), cloneByteSize)) {
                CORE_LOG_I("meshData ubo copying failed");
            }
        }

        gpuResourceMgr.UnmapBuffer(ubos_.mesh.GetHandle());
    }
}

void RenderNodeDefaultMaterialObjects::UpdateSkinBuffer(const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    // skindata is copied to a single buffer with sizeof(DefaultMaterialSkinStruct) offset
    // skin offset for submesh is calculated submesh.skinIndex * sizeof(DefaultMaterialSkinStruct)
    // NOTE: the size could be optimized, but render data store should calculate correct size with alignment
    if (auto skinData = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.submeshSkin.GetHandle())); skinData) {
        CORE_STATIC_ASSERT(
            RenderDataDefaultMaterial::MAX_SKIN_MATRIX_COUNT * 2u == CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT);
        const auto* skinDataEnd = skinData + sizeof(DefaultMaterialSkinStruct) * objectCounts_.maxSkinCount;
        const auto meshJointMatrices = dataStoreMaterial.GetMeshJointMatrices();
        for (const auto& jointRef : meshJointMatrices) {
            // we copy first current frame matrices, then previous frame
            const size_t copyCount = static_cast<size_t>(jointRef.count / 2u);
            const size_t copySize = copyCount * sizeof(Math::Mat4X4);
            const size_t ptrOffset = CORE_DEFAULT_MATERIAL_PREV_JOINT_OFFSET * sizeof(Math::Mat4X4);
            if (!CloneData(skinData, size_t(skinDataEnd - skinData), jointRef.data, copySize)) {
                CORE_LOG_I("skinData ubo copying failed");
            }
            if (!CloneData(skinData + ptrOffset, size_t(skinDataEnd - (skinData + ptrOffset)),
                    jointRef.data + copyCount, copySize)) {
                CORE_LOG_I("skinData ubo copying failed");
            }
            skinData = skinData + sizeof(DefaultMaterialSkinStruct);
        }

        gpuResourceMgr.UnmapBuffer(ubos_.submeshSkin.GetHandle());
    }
}

void RenderNodeDefaultMaterialObjects::UpdateMaterialBuffers(const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    // material data is copied to single buffer with UBO_BIND_OFFSET_ALIGNMENT alignment
    auto matFactorData = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.mat.GetHandle()));
    auto matTransformData = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.matTransform.GetHandle()));
    auto userMaterialData = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.userMat.GetHandle()));
    if (matFactorData && matTransformData && userMaterialData) {
        const auto* matFactorDataEnd = matFactorData + UBO_BIND_OFFSET_ALIGNMENT * objectCounts_.maxMaterialCount;
        const auto* matTransformDataEnd = matTransformData + UBO_BIND_OFFSET_ALIGNMENT * objectCounts_.maxMaterialCount;
        const auto* userMaterialDataEnd = userMaterialData + UBO_BIND_OFFSET_ALIGNMENT * objectCounts_.maxMaterialCount;
        const auto materialUniforms = dataStoreMaterial.GetMaterialUniforms();
        for (uint32_t matIdx = 0; matIdx < materialUniforms.size(); ++matIdx) {
            const RenderDataDefaultMaterial::AllMaterialUniforms& uniforms = materialUniforms[matIdx];
            if (!CloneData(matFactorData, size_t(matFactorDataEnd - matFactorData), &uniforms.factors,
                    sizeof(RenderDataDefaultMaterial::MaterialUniforms))) {
                CORE_LOG_I("materialFactorData ubo copying failed");
            }
            if (!CloneData(matTransformData, size_t(matTransformDataEnd - matTransformData), &uniforms.transforms,
                    sizeof(RenderDataDefaultMaterial::MaterialPackedUniforms))) {
                CORE_LOG_I("materialTransformData ubo copying failed");
            }
            const auto materialCustomProperties = dataStoreMaterial.GetMaterialCustomPropertyData(matIdx);
            if (!materialCustomProperties.empty()) {
                CORE_ASSERT(materialCustomProperties.size_bytes() <= UBO_BIND_OFFSET_ALIGNMENT);
                if (!CloneData(userMaterialData, size_t(userMaterialDataEnd - userMaterialData),
                        materialCustomProperties.data(), materialCustomProperties.size_bytes())) {
                    CORE_LOG_I("userMaterialData ubo copying failed");
                }
            }
            matFactorData = matFactorData + UBO_BIND_OFFSET_ALIGNMENT;
            matTransformData = matTransformData + UBO_BIND_OFFSET_ALIGNMENT;
            userMaterialData = userMaterialData + UBO_BIND_OFFSET_ALIGNMENT;
        }

        gpuResourceMgr.UnmapBuffer(ubos_.mat.GetHandle());
        gpuResourceMgr.UnmapBuffer(ubos_.matTransform.GetHandle());
        gpuResourceMgr.UnmapBuffer(ubos_.userMat.GetHandle());
    }
}

void RenderNodeDefaultMaterialObjects::ProcessBuffers(const ObjectCounts& objectCounts)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    constexpr uint32_t overEstimate { 16u };
    constexpr uint32_t baseStructSize = CORE_UNIFORM_BUFFER_MAX_BIND_SIZE;
    constexpr uint32_t singleComponentStructSize = UBO_BIND_OFFSET_ALIGNMENT;
    const string_view us = stores_.dataStoreNameScene;
    // instancing utilization for mesh and materials
    if (objectCounts_.maxMeshCount < objectCounts.maxMeshCount) {
        // mesh matrix uses max ubo bind size to utilize gpu instancing
        CORE_STATIC_ASSERT(sizeof(DefaultMaterialMeshStruct) <= PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
        objectCounts_.maxMeshCount =
            objectCounts.maxMeshCount + (objectCounts.maxMeshCount / overEstimate) + MIN_UBO_OBJECT_COUNT;

        const uint32_t byteSize =
            static_cast<uint32_t>(Align(objectCounts_.maxMeshCount * singleComponentStructSize, baseStructSize));
        objectCounts_.maxMeshCount = (byteSize / singleComponentStructSize) - MIN_UBO_OBJECT_COUNT;
        CORE_ASSERT((int32_t(byteSize / singleComponentStructSize) - int32_t(MIN_UBO_OBJECT_COUNT)) > 0);

        ubos_.mesh = gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MESH_DATA_BUFFER_NAME,
            GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, byteSize });
    }
    if (objectCounts_.maxSkinCount < objectCounts.maxSkinCount) {
        objectCounts_.maxSkinCount = objectCounts.maxSkinCount + (objectCounts.maxSkinCount / overEstimate);

        ubos_.submeshSkin = gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::SKIN_DATA_BUFFER_NAME,
            GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                static_cast<uint32_t>(sizeof(DefaultMaterialSkinStruct)) * objectCounts_.maxSkinCount });
    }
    if (objectCounts_.maxMaterialCount < objectCounts.maxMaterialCount) {
        CORE_STATIC_ASSERT(sizeof(RenderDataDefaultMaterial::MaterialUniforms) <= UBO_BIND_OFFSET_ALIGNMENT);
        objectCounts_.maxMaterialCount =
            objectCounts.maxMaterialCount + (objectCounts.maxMaterialCount / overEstimate) + MIN_UBO_OBJECT_COUNT;

        const uint32_t byteSize =
            static_cast<uint32_t>(Align(objectCounts_.maxMaterialCount * singleComponentStructSize, baseStructSize));
        objectCounts_.maxMaterialCount = (byteSize / singleComponentStructSize) - MIN_UBO_OBJECT_COUNT;
        CORE_ASSERT((int32_t(byteSize / singleComponentStructSize) - int32_t(MIN_UBO_OBJECT_COUNT)) > 0);

        const GpuBufferDesc bufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, byteSize };

        ubos_.mat = gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MATERIAL_DATA_BUFFER_NAME, bufferDesc);

        ubos_.matTransform = gpuResourceMgr.Create(
            us + DefaultMaterialMaterialConstants::MATERIAL_TRANSFORM_DATA_BUFFER_NAME, bufferDesc);

        ubos_.userMat =
            gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MATERIAL_USER_DATA_BUFFER_NAME, bufferDesc);
    }
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultMaterialObjects::Create()
{
    return new RenderNodeDefaultMaterialObjects();
}

void RenderNodeDefaultMaterialObjects::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultMaterialObjects*>(instance);
}
CORE3D_END_NAMESPACE()
