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

#include "render_node_default_material_objects.h"

#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <base/containers/allocator.h>
#include <base/containers/type_traits.h>
#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/resource_handle.h>

#include "render/datastore/render_data_store_default_material.h"

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr bool ENABLE_RT { true };

constexpr uint32_t UBO_BIND_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };
constexpr uint32_t MIN_UBO_OBJECT_COUNT { CORE_UNIFORM_BUFFER_MAX_BIND_SIZE / UBO_BIND_OFFSET_ALIGNMENT };
constexpr uint32_t MIN_MATERIAL_DESC_SET_COUNT { 64U };

constexpr GpuBufferDesc UBO_DESC { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0U };

constexpr size_t Align(size_t value, size_t align)
{
    if (value == 0) {
        return 0;
    }
    return ((value + align) / align) * align;
}

void GetDefaultMaterialGpuResources(const IRenderNodeGpuResourceManager& gpuResourceMgr,
    RenderNodeDefaultMaterialObjects::MaterialHandleStruct& defaultMat)
{
    defaultMat.resources[MaterialComponent::TextureIndex::BASE_COLOR].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_BASE_COLOR);
    defaultMat.resources[MaterialComponent::TextureIndex::NORMAL].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_NORMAL);
    defaultMat.resources[MaterialComponent::TextureIndex::MATERIAL].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_MATERIAL);
    defaultMat.resources[MaterialComponent::TextureIndex::EMISSIVE].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_EMISSIVE);
    defaultMat.resources[MaterialComponent::TextureIndex::AO].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_AO);

    defaultMat.resources[MaterialComponent::TextureIndex::CLEARCOAT].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_CLEARCOAT);
    defaultMat.resources[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_CLEARCOAT_ROUGHNESS);
    defaultMat.resources[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_CLEARCOAT_NORMAL);

    defaultMat.resources[MaterialComponent::TextureIndex::SHEEN].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_SHEEN);

    defaultMat.resources[MaterialComponent::TextureIndex::TRANSMISSION].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_TRANSMISSION);

    defaultMat.resources[MaterialComponent::TextureIndex::SPECULAR].handle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_SPECULAR);

    const RenderHandle samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT");
    for (uint32_t idx = 0; idx < countof(defaultMat.resources); ++idx) {
        defaultMat.resources[idx].samplerHandle = samplerHandle;
    }
}

// returns if there were changes and needs a descriptor set update
bool UpdateCurrentMaterialHandles(IRenderNodeGpuResourceManager& gpuResourceMgr, const bool forceUpdate,
    const RenderNodeDefaultMaterialObjects::MaterialHandleStruct& defaultMat,
    const RenderDataDefaultMaterial::MaterialHandles& matHandles,
    RenderNodeDefaultMaterialObjects::MaterialHandleStruct& storedMaterialHandles)
{
    RenderNodeDefaultMaterialObjects::MaterialHandleStruct currMaterialHandles;
    bool hasChanges = false;
    for (uint32_t idx = 0; idx < countof(matHandles.images); ++idx) {
        const auto& defaultRes = defaultMat.resources[idx];
        const auto& srcImage = matHandles.images[idx];
        const auto& srcSampler = matHandles.samplers[idx];
        auto& dst = currMaterialHandles.resources[idx];

        // fetch updated with generation counter raw handles
        if (RenderHandleUtil::IsValid(srcImage)) {
            dst.handle = gpuResourceMgr.GetRawHandle(srcImage);
        } else {
            dst.handle = defaultRes.handle;
        }
        if (RenderHandleUtil::IsValid(srcSampler)) {
            dst.samplerHandle = gpuResourceMgr.GetRawHandle(srcSampler);
        } else {
            dst.samplerHandle = defaultRes.samplerHandle;
        }

        // check if there are changes
        auto& stored = storedMaterialHandles.resources[idx];
        if ((stored.handle != dst.handle) || (stored.samplerHandle != dst.samplerHandle) ||
            (dst.handle.id & RenderHandleUtil::RENDER_HANDLE_NEEDS_UPDATE_MASK)) {
            hasChanges = true;
            stored.handle = dst.handle;
            stored.samplerHandle = dst.samplerHandle;
        }
    }
    return hasChanges;
}
} // namespace

void RenderNodeDefaultMaterialObjects::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    // get flags for rt
    if constexpr (ENABLE_RT) {
        rtEnabled_ = false;
        IRenderContext &renderContext = renderNodeContextMgr_->GetRenderContext();
        CORE_NS::IClassRegister *cr = renderContext.GetInterface<CORE_NS::IClassRegister>();
        if (cr != nullptr) {
            IGraphicsContext *graphicsContext = CORE_NS::GetInstance<IGraphicsContext>(*cr, UID_GRAPHICS_CONTEXT);
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

    globalDescs_ = {};

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const IShaderManager::RenderSlotData shaderRsd = shaderMgr.GetRenderSlotData(
        shaderMgr.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE));
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (!shaderRsd.pipelineLayout) {
        CORE_LOG_W("CORE3D_VALIDATION: Missing default material pipeline layout");
    }
#endif
    defaultMaterialPipelineLayout_ = shaderMgr.GetPipelineLayout(shaderRsd.pipelineLayout.GetHandle());
    GetDefaultMaterialGpuResources(gpuResourceMgr, defaultMaterialStruct_);

    ProcessBuffers({ 1u, 1u, 1u, 1u, 1u });
}

void RenderNodeDefaultMaterialObjects::PreExecuteFrame()
{
    // NOTE: global binders and buffers needs to processed in PreExecuteFrame
    // that other render nodes have access to those
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    // we need to have all buffers created (reason to have at least 1u)
    if (const IRenderDataStoreDefaultMaterial* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
        dataStoreMaterial) {
        const auto dsOc = dataStoreMaterial->GetObjectCounts();
        const ObjectCounts oc {
            Math::max(dsOc.meshCount, 1u),
            Math::max(dsOc.submeshCount, 1u),
            Math::max(dsOc.skinCount, 1u),
            Math::max(dsOc.materialCount, 1u),
            Math::max(dsOc.uniqueMaterialCount, 1u),
        };
        ProcessBuffers(oc);
    } else {
        ProcessBuffers({ 1u, 1u, 1u, 1u, 1u });
    }
}

void RenderNodeDefaultMaterialObjects::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));

    if (dataStoreMaterial) {
        UpdateMeshBuffer(*dataStoreMaterial);
        UpdateSkinBuffer(*dataStoreMaterial);
        UpdateMaterialBuffers(*dataStoreMaterial);
        UpdateTlasBuffers(cmdList, *dataStoreMaterial);

        UpdateDescriptorSets(cmdList, *dataStoreMaterial);
    } else {
        CORE_LOG_E("invalid render data store in RenderNodeDefaultMaterialObjects");
    }
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
    if (!matFactorData || !matTransformData || !userMaterialData) {
        return;
    }
    const auto* matFactorDataEnd = matFactorData + UBO_BIND_OFFSET_ALIGNMENT * objectCounts_.maxMaterialCount;
    const auto* matTransformDataEnd = matTransformData + UBO_BIND_OFFSET_ALIGNMENT * objectCounts_.maxMaterialCount;
    const auto* userMaterialDataEnd = userMaterialData + UBO_BIND_OFFSET_ALIGNMENT * objectCounts_.maxMaterialCount;
    const auto materialUniforms = dataStoreMaterial.GetMaterialUniforms();
    const auto materialFrameIndices = dataStoreMaterial.GetMaterialFrameIndices();
    for (uint32_t matOff = 0; matOff < materialFrameIndices.size(); ++matOff) {
        const uint32_t matIdx = materialFrameIndices[matOff];
        if (matIdx >= materialUniforms.size()) {
            continue;
        }

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

void RenderNodeDefaultMaterialObjects::UpdateDescriptorSets(
    IRenderCommandList& cmdList, const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    // loop through all materials at the moment
    {
        const auto& materialHandles = dataStoreMaterial.GetMaterialHandles();
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (globalDescs_.descriptorSets.size() != globalDescs_.materials.size()) {
            const string tmpStr =
                renderNodeContextMgr_->GetName() + "RenderNodeDefaultMaterialObjects::UpdateDescriptorSets";
            CORE_LOG_ONCE_W(tmpStr, "CORE3D_VALIDATION: Invalid default material descriptor set setup");
        }
#endif
        IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        for (size_t idx = 0; idx < materialHandles.size(); ++idx) {
            if (idx >= globalDescs_.descriptorSets.size()) {
                break; // prevent possible issues
            }
            auto* binder = globalDescs_.descriptorSets[idx].get();
            if (binder && (idx < materialHandles.size())) {
                const auto& matHandles = materialHandles[idx];
                MaterialHandleStruct& storedMatHandles = globalDescs_.materials[idx];
                const bool updateDescSet = UpdateCurrentMaterialHandles(
                    gpuResourceMgr, globalDescs_.forceUpdate, defaultMaterialStruct_, matHandles, storedMatHandles);

                if (globalDescs_.forceUpdate || updateDescSet) {
                    uint32_t bindingIdx = 0u;
                    // base color is bound separately to support automatic hwbuffer/OES shader modification
                    binder->BindImage(
                        bindingIdx++, storedMatHandles.resources[MaterialComponent::TextureIndex::BASE_COLOR]);

                    CORE_STATIC_ASSERT(MaterialComponent::TextureIndex::BASE_COLOR == 0);
                    // skip baseColor as it's bound already
                    constexpr size_t theCount = RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT - 1;
                    binder->BindImages(bindingIdx++, array_view(storedMatHandles.resources + 1, theCount));

                    cmdList.UpdateDescriptorSet(
                        binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
                }
            }
        }
        globalDescs_.forceUpdate = false;
    }
    if (auto* binder = globalDescs_.dmSet1Binder.get(); binder) {
        // NOTE: should be PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE or current size
        constexpr uint32_t skinSize = sizeof(DefaultMaterialSkinStruct);
        uint32_t bindingIdx = 0u;
        binder->BindBuffer(bindingIdx++, ubos_.mesh.GetHandle(), 0U, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
        binder->BindBuffer(bindingIdx++, ubos_.submeshSkin.GetHandle(), 0U, skinSize);
        binder->BindBuffer(bindingIdx++, ubos_.mat.GetHandle(), 0U, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
        binder->BindBuffer(
            bindingIdx++, ubos_.matTransform.GetHandle(), 0U, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
        binder->BindBuffer(
            bindingIdx++, ubos_.userMat.GetHandle(), 0U, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    }
    // update only once
    if (!globalDescs_.dmSet2Ready) {
        if (auto* binder = globalDescs_.dmSet2Binder.get(); binder) {
            uint32_t bindingIdx = 0u;
            // base color is bound separately to support automatic hwbuffer/OES shader modification
            binder->BindImage(
                bindingIdx++, defaultMaterialStruct_.resources[MaterialComponent::TextureIndex::BASE_COLOR]);

            CORE_STATIC_ASSERT(MaterialComponent::TextureIndex::BASE_COLOR == 0);
            // skip baseColor as it's bound already
            constexpr size_t theCount = RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT - 1;
            binder->BindImages(bindingIdx++, array_view(defaultMaterialStruct_.resources + 1, theCount));

            cmdList.UpdateDescriptorSet(
                binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
            globalDescs_.dmSet2Ready = true;
        }
    }
}

void RenderNodeDefaultMaterialObjects::ProcessBuffers(const ObjectCounts& objectCounts)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    constexpr uint32_t overEstimate { 16u };
    constexpr uint32_t baseStructSize = CORE_UNIFORM_BUFFER_MAX_BIND_SIZE;
    constexpr uint32_t singleComponentStructSize = UBO_BIND_OFFSET_ALIGNMENT;
    const string_view us = stores_.dataStoreNameScene;
    GpuBufferDesc bDesc = UBO_DESC;
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

        bDesc.byteSize = byteSize;
        ubos_.mesh = gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MESH_DATA_BUFFER_NAME, bDesc);

        // rt
        if constexpr (ENABLE_RT) {
            if (rtEnabled_) {
                ProcessTlasBuffers();
            }
        }
    }
    if (objectCounts_.maxSkinCount < objectCounts.maxSkinCount) {
        objectCounts_.maxSkinCount = objectCounts.maxSkinCount + (objectCounts.maxSkinCount / overEstimate);

        bDesc.byteSize = static_cast<uint32_t>(sizeof(DefaultMaterialSkinStruct)) * objectCounts_.maxSkinCount;
        ubos_.submeshSkin = gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::SKIN_DATA_BUFFER_NAME, bDesc);
    }
    if (objectCounts_.maxMaterialCount < objectCounts.maxMaterialCount) {
        CORE_STATIC_ASSERT(sizeof(RenderDataDefaultMaterial::MaterialUniforms) <= UBO_BIND_OFFSET_ALIGNMENT);
        objectCounts_.maxMaterialCount =
            objectCounts.maxMaterialCount + (objectCounts.maxMaterialCount / overEstimate) + MIN_UBO_OBJECT_COUNT;

        const uint32_t byteSize =
            static_cast<uint32_t>(Align(objectCounts_.maxMaterialCount * singleComponentStructSize, baseStructSize));
        objectCounts_.maxMaterialCount = (byteSize / singleComponentStructSize) - MIN_UBO_OBJECT_COUNT;
        CORE_ASSERT((int32_t(byteSize / singleComponentStructSize) - int32_t(MIN_UBO_OBJECT_COUNT)) > 0);

        bDesc.byteSize = byteSize;
        ubos_.mat = gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MATERIAL_DATA_BUFFER_NAME, bDesc);
        ubos_.matTransform =
            gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MATERIAL_TRANSFORM_DATA_BUFFER_NAME, bDesc);
        ubos_.userMat =
            gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MATERIAL_USER_DATA_BUFFER_NAME, bDesc);
    }
    if (objectCounts_.maxUniqueMaterialCount < objectCounts.maxUniqueMaterialCount) {
        CORE_STATIC_ASSERT(sizeof(RenderDataDefaultMaterial::MaterialUniforms) <= UBO_BIND_OFFSET_ALIGNMENT);
        objectCounts_.maxUniqueMaterialCount = objectCounts.maxUniqueMaterialCount +
                                               (objectCounts.maxUniqueMaterialCount / overEstimate) +
                                               MIN_MATERIAL_DESC_SET_COUNT;
        objectCounts_.maxUniqueMaterialCount =
            static_cast<uint32_t>(Align(objectCounts_.maxUniqueMaterialCount, MIN_MATERIAL_DESC_SET_COUNT));
        // global descriptor sets
        INodeContextDescriptorSetManager& dsMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        constexpr uint32_t set = 2U;
        globalDescs_.handles.clear();
        globalDescs_.descriptorSets.clear();
        globalDescs_.forceUpdate = true;
        const auto& bindings = defaultMaterialPipelineLayout_.descriptorSetLayouts[set].bindings;
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (bindings.empty()) {
            CORE_LOG_W("CORE3D_VALIDATION: Zero bindings in default material pipeline layout set 2");
        }
#endif
        globalDescs_.handles = dsMgr.CreateGlobalDescriptorSets(
            us + DefaultMaterialMaterialConstants::MATERIAL_RESOURCES_GLOBAL_DESCRIPTOR_SET_NAME, bindings,
            objectCounts_.maxUniqueMaterialCount);
        globalDescs_.descriptorSets.resize(globalDescs_.handles.size());
        globalDescs_.materials.resize(globalDescs_.handles.size());
        for (size_t idx = 0; idx < globalDescs_.handles.size(); ++idx) {
            globalDescs_.descriptorSets[idx] =
                dsMgr.CreateDescriptorSetBinder(globalDescs_.handles[idx].GetHandle(), bindings);
        }
    }
    ProcessGlobalBinders();
}

void RenderNodeDefaultMaterialObjects::ProcessGlobalBinders()
{
    if (!globalDescs_.dmSet1Binder) {
        const string_view us = stores_.dataStoreNameScene;
        INodeContextDescriptorSetManager& dsMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        globalDescs_.dmSet1 = {};
        globalDescs_.dmSet1Binder = {};
        constexpr uint32_t set = 1U;
        const auto& bindings = defaultMaterialPipelineLayout_.descriptorSetLayouts[set].bindings;
        globalDescs_.dmSet1 = dsMgr.CreateGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_SET1_GLOBAL_DESCRIPTOR_SET_NAME, bindings);
        globalDescs_.dmSet1Binder = dsMgr.CreateDescriptorSetBinder(globalDescs_.dmSet1.GetHandle(), bindings);
    }
    if (!globalDescs_.dmSet2Binder) {
        const string_view us = stores_.dataStoreNameScene;
        INodeContextDescriptorSetManager& dsMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        globalDescs_.dmSet2 = {};
        globalDescs_.dmSet2Binder = {};
        constexpr uint32_t set = 2U;
        const auto& bindings = defaultMaterialPipelineLayout_.descriptorSetLayouts[set].bindings;
        globalDescs_.dmSet2 = dsMgr.CreateGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_DEFAULT_RESOURCE_GLOBAL_DESCRIPTOR_SET_NAME, bindings);
        globalDescs_.dmSet2Binder = dsMgr.CreateDescriptorSetBinder(globalDescs_.dmSet2.GetHandle(), bindings);
    }
}

void RenderNodeDefaultMaterialObjects::ProcessTlasBuffers()
{
    CORE_ASSERT(rtEnabled_);
    // NOTE: does not overestimate
    // re-allocates every time
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const string_view us = stores_.dataStoreNameScene;

    IDevice& device = renderNodeContextMgr_->GetRenderContext().GetDevice();
    {
        AsGeometryInstancesInfo asInstances { false, 0U, objectCounts_.maxMeshCount };

        AsBuildGeometryInfo geometryInfo;
        geometryInfo.type = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
        // build
        geometryInfo.mode = BuildAccelerationStructureMode::CORE_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD;
        geometryInfo.flags = BuildAccelerationStructureFlagBits::CORE_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT;
        tlas_.asBuildSizes = device.GetAccelerationStructureBuildSizes(geometryInfo, {}, {}, { &asInstances, 1U });
        const GpuAccelerationStructureDesc desc { AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
            GpuBufferDesc {
                CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT,
                CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_BARRIERS,
                tlas_.asBuildSizes.accelerationStructureSize,
            } };
        tlas_.as = gpuResourceMgr.Create(us + DefaultMaterialMaterialConstants::MATERIAL_TLAS_PREFIX_NAME, desc);
        tlas_.createNewBuffer = true;
    }
    // GPU buffer is made during ExecuteFrame
}

void RenderNodeDefaultMaterialObjects::UpdateTlasBuffers(
    IRenderCommandList& cmdList, const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    if constexpr (ENABLE_RT) {
        if (rtEnabled_) {
            if (tlas_.createNewBuffer) {
                tlas_.createNewBuffer = false;

                auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
                const uint32_t byteSize =
                    PipelineStateConstants::ACCELERATION_STRUCTURE_INSTANCE_SIZE * objectCounts_.maxMeshCount;
                const GpuBufferDesc desc {
                    BufferUsageFlagBits::CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                        BufferUsageFlagBits::
                            CORE_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT, // usageFlags
                    MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT, // memoryPropertyFlags
                    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,                    // engineCreationFlags
                    byteSize,                                                           // byteSize
                    BASE_NS::Format::BASE_FORMAT_UNDEFINED,                             // format
                };
                // name not needed, only access here
                tlas_.asInstanceBuffer = gpuResourceMgr.Create(tlas_.asInstanceBuffer, desc);

                // allocate scratch
                const GpuBufferDesc scratchDesc {
                    BufferUsageFlagBits::CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                        BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT, // usageFlags
                    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                         // memoryPropertyFlags
                    0U,                                                            // engineCreationFlags
                    tlas_.asBuildSizes.buildScratchSize,                           // byteSize
                    BASE_NS::Format::BASE_FORMAT_UNDEFINED,                        // format
                };
                tlas_.scratch = gpuResourceMgr.Create(tlas_.scratch, scratchDesc);
            }

            const RenderDataStoreDefaultMaterial& dsMat = (const RenderDataStoreDefaultMaterial&)dataStoreMaterial;
            const auto blasData = dsMat.GetMeshBlasData();
            if (!blasData.empty()) {
                cmdList.CopyAccelerationStructureInstances({ tlas_.asInstanceBuffer.GetHandle(), 0U }, blasData);

                const uint32_t blasCount = static_cast<uint32_t>(blasData.size());
                AsGeometryInstancesData asInstances;
                asInstances.info = { false, 0U, blasCount };
                asInstances.data = { tlas_.asInstanceBuffer.GetHandle(), 0U };

                AsBuildGeometryData geometry;
                geometry.dstAccelerationStructure = tlas_.as.GetHandle();
                geometry.scratchBuffer = { tlas_.scratch.GetHandle(), 0U };
                geometry.srcAccelerationStructure = {};
                geometry.info.type = AccelerationStructureType::CORE_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
                geometry.info.mode = BuildAccelerationStructureMode::CORE_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD;
                geometry.info.flags =
                    BuildAccelerationStructureFlagBits::CORE_BUILD_ACCELERATION_STRUCTURE_ALLOW_UPDATE_BIT;
                cmdList.BuildAccelerationStructures(geometry, {}, {}, { &asInstances, 1U });
            }
        }
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
