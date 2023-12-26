/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "render_node_default_material_render_slot.h"

#include <algorithm>

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/math/matrix_util.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

#if (CORE3D_DEV_ENABLED == 1)
#include "render/datastore/render_data_store_default_material.h"
#endif

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr DynamicStateFlags DYN_STATE_FLAGS = CORE_DYNAMIC_STATE_VIEWPORT | CORE_DYNAMIC_STATE_SCISSOR;

constexpr float CUBE_MAP_LOD_COEFF { 8.0f };
constexpr uint32_t UBO_BIND_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };

static constexpr uint64_t LIGHTING_FLAGS_SHIFT { 32u };
static constexpr uint64_t LIGHTING_FLAGS_MASK { 0xF00000000u };
static constexpr uint64_t POST_PROCESS_FLAGS_SHIFT { 36u };
static constexpr uint64_t POST_PROCESS_FLAGS_MASK { 0xF000000000u };
static constexpr uint64_t RENDER_HASH_FLAGS_MASK { 0xFFFFffffu };

// our light weight straight to screen post processes are only interested in these
static constexpr uint32_t POST_PROCESS_IMPORTANT_FLAGS_MASK { 0xffu };

void GetDefaultMaterialGpuResources(const IRenderNodeGpuResourceManager& gpuResourceMgr,
    RenderNodeDefaultMaterialRenderSlot::MaterialHandleStruct& defaultMat)
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

RenderHandleReference CreateCubeNodeSampler(IRenderNodeGpuResourceManager& gpuResourceMgr)
{
    GpuSamplerDesc sampler {
        Filter::CORE_FILTER_LINEAR,                                  // magFilter
        Filter::CORE_FILTER_LINEAR,                                  // minFilter
        Filter::CORE_FILTER_LINEAR,                                  // mipMapMode
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
    };
    sampler.minLod = 0.0f;
    sampler.maxLod = CUBE_MAP_LOD_COEFF;
    return gpuResourceMgr.Create(sampler);
}

RenderNodeDefaultMaterialRenderSlot::ShadowBuffers GetShadowBufferNodeData(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view sceneName)
{
    RenderNodeDefaultMaterialRenderSlot::ShadowBuffers sb;
    GpuSamplerDesc sampler {
        Filter::CORE_FILTER_LINEAR,                                    // magFilter
        Filter::CORE_FILTER_LINEAR,                                    // minFilter
        Filter::CORE_FILTER_LINEAR,                                    // mipMapMode
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // addressModeU
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // addressModeV
        SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER, // addressModeW
    };
    sampler.borderColor = CORE_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    sb.vsmSamplerHandle = gpuResourceMgr.Create(sampler);

    sampler.compareOp = CompareOp::CORE_COMPARE_OP_GREATER;
    sampler.enableCompareOp = true;
    sb.pcfSamplerHandle = gpuResourceMgr.Create(sampler);

    sb.depthHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_DEPTH_BUFFER_NAME);
    sb.vsmColorHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_VSM_COLOR_BUFFER_NAME);
    if (!RenderHandleUtil::IsValid(sb.depthHandle)) {
        sb.depthHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE_WHITE");
    }
    if (!RenderHandleUtil::IsValid(sb.vsmColorHandle)) {
        sb.vsmColorHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    }

    return sb;
}

inline uint64_t HashShaderDataAndSubmesh(const uint64_t shaderDataHash, const uint32_t renderHash,
    const IRenderDataStoreDefaultLight::LightingFlags lightingFlags,
    const PostProcessConfiguration::PostProcessEnableFlags postProcessFlags)
{
    const uint32_t ppEnabled = (postProcessFlags > 0);
    uint64_t hash = ((uint64_t)renderHash & RENDER_HASH_FLAGS_MASK) |
                    (((uint64_t)lightingFlags << LIGHTING_FLAGS_SHIFT) & LIGHTING_FLAGS_MASK) |
                    (((uint64_t)ppEnabled << POST_PROCESS_FLAGS_SHIFT) & POST_PROCESS_FLAGS_MASK);
    HashCombine(hash, shaderDataHash);
    return hash;
}

inline bool IsInverseWinding(const RenderSubmeshFlags submeshFlags, const RenderSceneFlags sceneRenderingFlags,
    const RenderCamera::Flags cameraRenderingFlags)
{
    const bool flipWinding = (sceneRenderingFlags & RENDER_SCENE_FLIP_WINDING_BIT) |
                             (cameraRenderingFlags & RenderCamera::CAMERA_FLAG_INVERSE_WINDING_BIT);
    const bool isNegative = flipWinding
                                ? !((submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT) > 0)
                                : ((submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT) > 0);
    return isNegative;
}

bool UpdateCurrentMaterialHandles(const RenderSubmesh& currSubmesh,
    const RenderNodeDefaultMaterialRenderSlot::MaterialHandleStruct& defaultMat,
    const RenderDataDefaultMaterial::MaterialHandles& currSubmeshHandles,
    RenderNodeDefaultMaterialRenderSlot::MaterialHandleStruct& currMaterialHandles)
{
    bool updateMaterialHandles = false;
    auto updateIfNeeded = [](const RenderHandle& currImage, const RenderHandle& currSampler,
                              const BindableImage& defaultImage, BindableImage& slotImage) {
        const auto& handle = RenderHandleUtil::IsValid(currImage) ? currImage : defaultImage.handle;
        bool updated = false;
        if (slotImage.handle.id != handle.id) {
            slotImage.handle = handle;
            updated = true;
        }
        const auto& samplerHandle = RenderHandleUtil::IsValid(currSampler) ? currSampler : defaultImage.samplerHandle;
        if (slotImage.samplerHandle.id != samplerHandle.id) {
            slotImage.samplerHandle = samplerHandle;
            updated = true;
        }
        return updated;
    };
    for (uint32_t idx = 0; idx < countof(currSubmeshHandles.images); ++idx) {
        updateMaterialHandles |=
            updateIfNeeded(currSubmeshHandles.images[idx].GetHandle(), currSubmeshHandles.samplers[idx].GetHandle(),
                defaultMat.resources[idx], currMaterialHandles.resources[idx]);
    }
    return updateMaterialHandles;
}
} // namespace

void RenderNodeDefaultMaterialRenderSlot::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    // reset
    currentScene_ = {};
    allShaderData_ = {};
    objectCounts_ = {};

    if ((jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) &&
        jsonInputs_.renderDataStore.dataStoreName.empty()) {
        CORE_LOG_V("%s: render data store post process configuration not set in render node graph",
            renderNodeContextMgr_->GetName().data());
    }
    renderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    CreateDefaultShaderData();

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    defaultSamplers_.cubemapHandle = CreateCubeNodeSampler(gpuResourceMgr);
    defaultSamplers_.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    defaultSamplers_.nearestHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    defaultSamplers_.linearMipHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");
    defaultColorPrePassHandle_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");

    buffers_.defaultBuffer = gpuResourceMgr.GetBufferHandle("CORE_DEFAULT_GPU_BUFFER");
    GetSceneUniformBuffers(stores_.dataStoreNameScene);
    GetCameraUniformBuffers();

    shadowBuffers_ = GetShadowBufferNodeData(gpuResourceMgr, stores_.dataStoreNameScene);

    GetDefaultMaterialGpuResources(gpuResourceMgr, defaultMaterialStruct_);
    defaultSkyBoxRadianceCubemap_ =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP);
}

void RenderNodeDefaultMaterialRenderSlot::PreExecuteFrame()
{
    // re-create needed gpu resources
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const IRenderDataStoreDefaultMaterial* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
    // we need to have all buffers created (reason to have at least 1u)
    if (dataStoreMaterial) {
        const auto dsOc = dataStoreMaterial->GetSlotObjectCounts(jsonInputs_.renderSlotId);
        const ObjectCounts oc {
            Math::max(dsOc.meshCount, 1u),
            Math::max(dsOc.submeshCount, 1u),
            Math::max(dsOc.skinCount, 1u),
            Math::max(dsOc.materialCount, 1u),
        };
        ProcessBuffersAndDescriptors(oc);
    } else if (objectCounts_.maxSlotMeshCount == 0) {
        ProcessBuffersAndDescriptors({ 1u, 1u, 1u, 1u });
    }
}

void RenderNodeDefaultMaterialRenderSlot::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    const bool validRenderDataStore = dataStoreScene && dataStoreMaterial && dataStoreCamera && dataStoreLight;
    if (validRenderDataStore) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);
    } else {
        CORE_LOG_E("invalid render data stores in RenderNodeDefaultMaterialRenderSlot");
    }

    cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);

    if (validRenderDataStore) {
        const auto cameras = dataStoreCamera->GetCameras();
        const auto scene = dataStoreScene->GetScene();

        const bool hasShaders = allShaderData_.slotHasShaders;
        const bool hasCamera =
            (!cameras.empty() && (currentScene_.cameraIdx < (uint32_t)cameras.size())) ? true : false;

        ProcessSlotSubmeshes(*dataStoreCamera, *dataStoreMaterial);
        const bool hasSubmeshes = (!sortedSlotSubmeshes_.empty());
        if (hasShaders && hasCamera && hasSubmeshes) {
            UpdatePostProcessConfiguration();
            RenderSubmeshes(cmdList, *dataStoreMaterial, *dataStoreCamera);
        }
    }

    cmdList.EndRenderPass();
}

void RenderNodeDefaultMaterialRenderSlot::RenderSubmeshes(IRenderCommandList& cmdList,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const IRenderDataStoreDefaultCamera& dataStoreCamera)
{
    const size_t submeshCount = sortedSlotSubmeshes_.size();
    CORE_ASSERT(submeshCount <= objectCounts_.maxSlotSubmeshCount);

    // dynamic state
    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);

    // set 0, update camera, general data, and lighting
    UpdateSet0(cmdList);
    // set 1, update mesh matrices, material data, and skinning data (uses dynamic offset)
    UpdateSet1(cmdList);

    // first two sets are only bound for the first submesh (inside for loop)
    // no bindless, we need to update images per object
    RenderHandle boundPsoHandle = {};
    uint64_t boundShaderHash = 0;
    uint32_t currMaterialIndex = ~0u;
    MaterialHandleStruct currentMaterialHandles;
    bool initialBindDone = false; // cannot be checked from the idx

    const auto& materialHandles = dataStoreMaterial.GetMaterialHandles();
    const auto& submeshMaterialFlags = dataStoreMaterial.GetSubmeshMaterialFlags();
    const auto& submeshes = dataStoreMaterial.GetSubmeshes();
    const auto& customResourceHandles = dataStoreMaterial.GetCustomResourceHandles();
    const uint64_t camLayerMask = currentScene_.camera.layerMask;
    for (size_t idx = 0; idx < submeshCount; ++idx) {
        const auto& ssp = sortedSlotSubmeshes_[idx];
        const uint32_t submeshIndex = ssp.submeshIndex;
        const auto& currSubmesh = submeshes[submeshIndex];
        const auto& currSubmeshMatHandles = materialHandles[currSubmesh.materialIndex];
        const RenderSubmeshFlags submeshFlags = currSubmesh.submeshFlags | jsonInputs_.nodeSubmeshExtraFlags;
        auto currMaterialFlags = submeshMaterialFlags[submeshIndex];

        // sorted slot submeshes should already have removed layers if default sorting was used
        if (((camLayerMask & currSubmesh.layerMask) == 0) ||
            ((jsonInputs_.nodeFlags & RENDER_SCENE_DISCARD_MATERIAL_BIT) &&
                (currMaterialFlags.extraMaterialRenderingFlags &
                    RenderExtraRenderingFlagBits::RENDER_EXTRA_RENDERING_DISCARD_BIT))) {
            continue;
        }

        uint32_t renderHash = currMaterialFlags.renderHash;
        if (!currentScene_.hasShadow) { // remove shadow if not in scene
            currMaterialFlags.renderMaterialFlags =
                currMaterialFlags.renderMaterialFlags & (~RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_RECEIVER_BIT);
            renderHash = dataStoreMaterial.GenerateRenderHash(currMaterialFlags);
        }
        // get shader and graphics state and start hashing
        ShaderStateData ssd { ssp.shaderHandle, ssp.gfxStateHandle, 0 };
        ssd.hash = (ssd.shader.id << 32U) | (ssd.gfxState.id & 0xFFFFffff);
        // current shader state is fetched for build-in and custom shaders (decision is made later)
        ssd.hash = HashShaderDataAndSubmesh(
            ssd.hash, renderHash, currentScene_.lightingFlags, currentRenderPPConfiguration_.flags.x);
        if (ssd.hash != boundShaderHash) {
            const RenderHandle psoHandle =
                GetSubmeshPso(ssd, currMaterialFlags, submeshFlags, currentScene_.lightingFlags);
            if (psoHandle.id != boundPsoHandle.id) {
                boundShaderHash = ssd.hash;
                boundPsoHandle = psoHandle;
                cmdList.BindPipeline(boundPsoHandle);
                initialBindDone = false;
            }
        }

        // bind first set only the first time
        if (!initialBindDone) {
            RenderHandle setResourceHandles[2u] { allDescriptorSets_.set01[0u]->GetDescriptorSetHandle(),
                allDescriptorSets_.set01[1u]->GetDescriptorSetHandle() };
            cmdList.BindDescriptorSets(0, { setResourceHandles, 2u });
        }

        // set 1 (mesh matrix, skin matrices, material, material user data)
        const uint32_t currMeshMatrixOffset = currSubmesh.meshIndex * UBO_BIND_OFFSET_ALIGNMENT;
        const uint32_t currMaterialOffset = currSubmesh.materialIndex * UBO_BIND_OFFSET_ALIGNMENT;
        const uint32_t currMaterialTransformOffset = currSubmesh.materialIndex * UBO_BIND_OFFSET_ALIGNMENT;
        const uint32_t currUserMaterialOffset = currSubmesh.materialIndex * UBO_BIND_OFFSET_ALIGNMENT;
        uint32_t currJointMatrixOffset = 0u;
        if (submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) {
            currJointMatrixOffset =
                currSubmesh.skinJointIndex * static_cast<uint32_t>(sizeof(DefaultMaterialSkinStruct));
        }
        const uint32_t dynamicOffsets[] = { currMeshMatrixOffset, currJointMatrixOffset, currMaterialOffset,
            currMaterialTransformOffset, currUserMaterialOffset };
        // set to bind, handle to resource, offsets for dynamic descs
        cmdList.BindDescriptorSet(1u, allDescriptorSets_.set01[1u]->GetDescriptorSetHandle(), dynamicOffsets);

        // material gpu resources
        const bool needsMaterialUpdate = UpdateCurrentMaterialHandles(
            currSubmesh, defaultMaterialStruct_, currSubmeshMatHandles, currentMaterialHandles);
        if (needsMaterialUpdate || (!initialBindDone) || (currMaterialIndex != currSubmesh.materialIndex)) {
            UpdateAndBindSet2(cmdList, currentMaterialHandles, static_cast<uint32_t>(idx));
        }

        initialBindDone = true;

        // custom set 3 resources
        if (currSubmesh.customResourcesIndex < static_cast<uint32_t>(customResourceHandles.size())) {
            if (!UpdateAndBindSet3(cmdList, customResourceHandles[currSubmesh.customResourcesIndex])) {
                continue; // we prevent drawing
            }
        }

        // vertex buffers and draw
        if (currSubmesh.vertexBufferCount > 0) {
            VertexBuffer vbs[RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
            for (uint32_t vbIdx = 0; vbIdx < currSubmesh.vertexBufferCount; ++vbIdx) {
                vbs[vbIdx] = ConvertVertexBuffer(currSubmesh.vertexBuffers[vbIdx]);
            }
            cmdList.BindVertexBuffers({ vbs, currSubmesh.vertexBufferCount });
        }
        const auto& dc = currSubmesh.drawCommand;
        const RenderVertexBuffer& iArgs = currSubmesh.indirectArgsBuffer;
        const bool indirectDraw = iArgs.bufferHandle ? true : false;
        if (currSubmesh.indexBuffer.indexType != IndexType::CORE_INDEX_TYPE_MAX_ENUM) {
            cmdList.BindIndexBuffer(ConvertIndexBuffer(currSubmesh.indexBuffer));
            if (indirectDraw) {
                cmdList.DrawIndexedIndirect(iArgs.bufferHandle.GetHandle(), iArgs.bufferOffset, 1u, 0u);
            } else {
                cmdList.DrawIndexed(dc.indexCount, dc.instanceCount, 0, 0, 0);
            }
        } else {
            if (indirectDraw) {
                cmdList.DrawIndirect(iArgs.bufferHandle.GetHandle(), iArgs.bufferOffset, 1u, 0u);
            } else {
                cmdList.Draw(dc.vertexCount, dc.instanceCount, 0, 0);
            }
        }
    }
}

void RenderNodeDefaultMaterialRenderSlot::UpdateSet0(IRenderCommandList& cmdList)
{
    auto& binder = *allDescriptorSets_.set01[0u];
    uint32_t bindingIndex = 0;
    binder.BindBuffer(bindingIndex++, buffers_.camera, 0u);
    binder.BindBuffer(bindingIndex++, buffers_.generalData, 0u);

    const RenderHandle radianceCubemap = (currentScene_.camera.environment.radianceCubemap)
                                             ? currentScene_.camera.environment.radianceCubemap.GetHandle()
                                             : defaultSkyBoxRadianceCubemap_;
    const RenderHandle colorPrePass = RenderHandleUtil::IsValid(currentScene_.prePassColorTarget)
                                          ? currentScene_.prePassColorTarget
                                          : defaultColorPrePassHandle_;

    binder.BindBuffer(bindingIndex++, buffers_.environment, 0u);
    binder.BindBuffer(bindingIndex++, buffers_.fog, 0u);
    binder.BindBuffer(bindingIndex++, buffers_.light, 0u);
    binder.BindBuffer(bindingIndex++, buffers_.postProcess, 0u);
    binder.BindBuffer(bindingIndex++, buffers_.lightCluster, 0u);
    binder.BindImage(bindingIndex++, colorPrePass, defaultSamplers_.linearMipHandle);
    binder.BindImage(bindingIndex++, shadowBuffers_.vsmColorHandle, shadowBuffers_.vsmSamplerHandle.GetHandle());
    binder.BindImage(bindingIndex++, shadowBuffers_.depthHandle, shadowBuffers_.pcfSamplerHandle.GetHandle());
    binder.BindImage(bindingIndex++, radianceCubemap, defaultSamplers_.cubemapHandle.GetHandle());

    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

void RenderNodeDefaultMaterialRenderSlot::UpdateSet1(IRenderCommandList& cmdList)
{
    // NOTE: should be PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE or current size
    constexpr uint32_t skinSize = sizeof(DefaultMaterialSkinStruct);
    auto& binder = *allDescriptorSets_.set01[1u];
    uint32_t bindingIdx = 0u;
    binder.BindBuffer(bindingIdx++, buffers_.mesh, 0u, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
    binder.BindBuffer(bindingIdx++, buffers_.skinJoint, 0u, skinSize);
    binder.BindBuffer(bindingIdx++, buffers_.mat, 0, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
    binder.BindBuffer(bindingIdx++, buffers_.matTransform, 0, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
    binder.BindBuffer(bindingIdx++, buffers_.matCustom, 0, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

void RenderNodeDefaultMaterialRenderSlot::UpdateAndBindSet2(
    IRenderCommandList& cmdList, const MaterialHandleStruct& materialHandles, const uint32_t objIdx)
{
    auto& binder = *allDescriptorSets_.sets2[objIdx];
    uint32_t bindingIdx = 0u;

    // base color is bound separately to support e.g. gles automatic OES shader modification
    binder.BindImage(bindingIdx++, materialHandles.resources[MaterialComponent::TextureIndex::BASE_COLOR]);

    CORE_STATIC_ASSERT(MaterialComponent::TextureIndex::BASE_COLOR == 0);
    // skip baseColor as it's bound already
    constexpr size_t theCount = RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT - 1;
    binder.BindImages(bindingIdx++, array_view(materialHandles.resources + 1, theCount));

    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(2u, binder.GetDescriptorSetHandle()); // set to bind
}

bool RenderNodeDefaultMaterialRenderSlot::UpdateAndBindSet3(
    IRenderCommandList& cmdList, const RenderDataDefaultMaterial::CustomResourceData& customResourceData)
{
    constexpr uint32_t fixedCustomSet3 = 3u;
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    RenderHandle currPlHandle =
        shaderMgr.GetPipelineLayoutHandleByShaderHandle(customResourceData.shaderHandle.GetHandle());
    const PipelineLayout& plRef = shaderMgr.GetPipelineLayout(currPlHandle);
    const auto& descBindings = plRef.descriptorSetLayouts[fixedCustomSet3].bindings;
    const RenderHandle descSetHandle = descriptorSetMgr.CreateOneFrameDescriptorSet(descBindings);
    bool valid = false;
    if (RenderHandleUtil::IsValid(descSetHandle) && (descBindings.size() == customResourceData.resourceHandleCount)) {
        IDescriptorSetBinder::Ptr binderPtr = descriptorSetMgr.CreateDescriptorSetBinder(descSetHandle, descBindings);
        if (binderPtr) {
            auto& binder = *binderPtr;
            for (uint32_t idx = 0; idx < customResourceData.resourceHandleCount; ++idx) {
                const RenderHandle& currRes = customResourceData.resourceHandles[idx].GetHandle();
                if (gpuResourceMgr.IsGpuBuffer(currRes)) {
                    binder.BindBuffer(idx, currRes, 0);
                } else if (gpuResourceMgr.IsGpuImage(currRes)) {
                    binder.BindImage(idx, currRes);
                } else if (gpuResourceMgr.IsGpuSampler(currRes)) {
                    binder.BindSampler(idx, currRes);
                }
            }

            // user generated setup, we check for validity of all bindings in the descriptor set
            if (binder.GetDescriptorSetLayoutBindingValidity()) {
                cmdList.UpdateDescriptorSet(
                    binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
                cmdList.BindDescriptorSet(fixedCustomSet3, binder.GetDescriptorSetHandle());
                valid = true;
            }
        }
    }
    if (!valid) {
        CORE_LOG_ONCE_E("material_render_slot_custom_set3_issue",
            "invalid bindings with custom shader descriptor set 3 (render node: %s)",
            renderNodeContextMgr_->GetName().data());
    }
    return valid;
}

RenderHandle RenderNodeDefaultMaterialRenderSlot::GetSubmeshPso(const ShaderStateData& ssd,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags, const RenderSubmeshFlags submeshFlags,
    const IRenderDataStoreDefaultLight::LightingFlags lightFlags)
{
    if (const auto dataIter = allShaderData_.shaderIdToData.find(ssd.hash);
        dataIter != allShaderData_.shaderIdToData.cend()) {
        return allShaderData_.perShaderData[dataIter->second].psoHandle;
    }

    // specialization for not found hash
    // expand material flags for shader with possible discard
    constexpr size_t maxFlagCount { 16u };
    uint32_t specializationFlags[maxFlagCount];
    const size_t maxSpecializations = Math::min(maxFlagCount, allShaderData_.defaultSpecilizationConstants.size());
    for (size_t idx = 0; idx < maxSpecializations; ++idx) {
        const auto& ref = allShaderData_.defaultSpecilizationConstants[idx];
        const uint32_t constantId = ref.offset / sizeof(uint32_t);

        if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT) {
            if (ref.id == 0u) {
                specializationFlags[constantId] = submeshFlags;
            }
        } else if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT) {
            if (ref.id == 0u) {
                specializationFlags[constantId] = static_cast<uint32_t>(submeshMaterialFlags.materialType);
            } else if (ref.id == 1u) {
                specializationFlags[constantId] = submeshMaterialFlags.renderMaterialFlags;
            } else if (ref.id == 2u) {
                specializationFlags[constantId] = lightFlags;
            } else if (ref.id == 3u) {
                specializationFlags[constantId] = currentRenderPPConfiguration_.flags.x;
            }
        }
    }

    return CreateNewPso(ssd,
        { { allShaderData_.defaultSpecilizationConstants.data(), maxSpecializations },
            { specializationFlags, maxSpecializations } },
        submeshFlags);
}

void RenderNodeDefaultMaterialRenderSlot::UpdatePostProcessConfiguration()
{
    if (jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) {
        if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
            auto const& dataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
            auto const dataStore = static_cast<IRenderDataStorePod const*>(
                dataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName));
            if (dataStore) {
                auto const dataView = dataStore->Get(jsonInputs_.renderDataStore.configurationName);
                if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                    const PostProcessConfiguration* data = (const PostProcessConfiguration*)dataView.data();
                    currentRenderPPConfiguration_ =
                        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(*data);
                    currentRenderPPConfiguration_.flags.x =
                        (currentRenderPPConfiguration_.flags.x & POST_PROCESS_IMPORTANT_FLAGS_MASK);
                }
            }
        }
    }
}

void RenderNodeDefaultMaterialRenderSlot::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    const auto scene = dataStoreScene.GetScene();
    bool hasCustomCamera = false;
    bool isNamedCamera = false; // NOTE: legacy support will be removed
    uint32_t cameraIdx = scene.cameraIndex;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraId);
        hasCustomCamera = true;
    } else if (!(jsonInputs_.customCameraName.empty())) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraName);
        hasCustomCamera = true;
        isNamedCamera = true;
    }

    if (const auto cameras = dataStoreCamera.GetCameras(); cameraIdx < (uint32_t)cameras.size()) {
        // store current frame camera
        currentScene_.camera = cameras[cameraIdx];
    }

    currentScene_.prePassColorTarget = currentScene_.camera.prePassColorTarget.GetHandle();
    if (!RenderHandleUtil::IsValid(currentScene_.prePassColorTarget) &&
        (!currentScene_.camera.prePassColorTargetName.empty())) {
        currentScene_.prePassColorTarget =
            renderNodeContextMgr_->GetGpuResourceManager().GetImageHandle(currentScene_.camera.prePassColorTargetName);
    }

    // renderpass needs to be valid (created in init)
    if (hasCustomCamera) {
        // uses camera based clear-setup
        RenderNodeSceneUtil::UpdateRenderPassFromCustomCamera(currentScene_.camera, isNamedCamera, renderPass_);
    } else {
        RenderNodeSceneUtil::UpdateRenderPassFromCamera(currentScene_.camera, renderPass_);
    }
    currentScene_.viewportDesc = RenderNodeSceneUtil::CreateViewportFromCamera(currentScene_.camera);
    currentScene_.scissorDesc = RenderNodeSceneUtil::CreateScissorFromCamera(currentScene_.camera);

    const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight.GetLightCounts();
    currentScene_.hasShadow = (lightCounts.shadowCount > 0) ? true : false;
    currentScene_.cameraIdx = cameraIdx;
    currentScene_.shadowTypes = dataStoreLight.GetShadowTypes();
    currentScene_.lightingFlags = dataStoreLight.GetLightingFlags();
}

void RenderNodeDefaultMaterialRenderSlot::CreateDefaultShaderData()
{
    allShaderData_ = {};

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    allShaderData_.defaultPlHandle =
        shaderMgr.GetPipelineLayoutHandle(DefaultMaterialShaderConstants::PIPELINE_LAYOUT_FORWARD);
    allShaderData_.defaultPipelineLayout = shaderMgr.GetPipelineLayout(allShaderData_.defaultPlHandle);
    allShaderData_.defaultVidHandle =
        shaderMgr.GetVertexInputDeclarationHandle(DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD);

    {
        // get the default material shader and default shader state
        const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(jsonInputs_.renderSlotId);
        allShaderData_.defaultShaderHandle = rsd.shader.GetHandle();
        allShaderData_.defaultStateHandle = rsd.graphicsState.GetHandle();
        if (shaderMgr.IsShader(allShaderData_.defaultShaderHandle)) {
            allShaderData_.slotHasShaders = true;
            const ShaderSpecilizationConstantView& sscv =
                shaderMgr.GetReflectionSpecialization(allShaderData_.defaultShaderHandle);
            allShaderData_.defaultSpecilizationConstants.resize(sscv.constants.size());
            for (uint32_t idx = 0; idx < (uint32_t)allShaderData_.defaultSpecilizationConstants.size(); ++idx) {
                allShaderData_.defaultSpecilizationConstants[idx] = sscv.constants[idx];
            }
        } else {
            CORE_LOG_I("RenderNode: %s, no default shaders for render slot id %u",
                renderNodeContextMgr_->GetName().data(), jsonInputs_.renderSlotId);
        }
    }
}

void RenderNodeDefaultMaterialRenderSlot::GetSceneUniformBuffers(const string_view us)
{
    const auto& gpuMgr = renderNodeContextMgr_->GetGpuResourceManager();
    buffers_.camera = gpuMgr.GetBufferHandle(us + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);
    buffers_.light = gpuMgr.GetBufferHandle(us + DefaultMaterialLightingConstants::LIGHT_DATA_BUFFER_NAME);
    buffers_.lightCluster =
        gpuMgr.GetBufferHandle(us + DefaultMaterialLightingConstants::LIGHT_CLUSTER_DATA_BUFFER_NAME);
    buffers_.mat = gpuMgr.GetBufferHandle(us + DefaultMaterialMaterialConstants::MATERIAL_DATA_BUFFER_NAME);
    buffers_.matTransform =
        gpuMgr.GetBufferHandle(us + DefaultMaterialMaterialConstants::MATERIAL_TRANSFORM_DATA_BUFFER_NAME);
    buffers_.matCustom = gpuMgr.GetBufferHandle(us + DefaultMaterialMaterialConstants::MATERIAL_USER_DATA_BUFFER_NAME);
    buffers_.mesh = gpuMgr.GetBufferHandle(us + DefaultMaterialMaterialConstants::MESH_DATA_BUFFER_NAME);
    buffers_.skinJoint = gpuMgr.GetBufferHandle(us + DefaultMaterialMaterialConstants::SKIN_DATA_BUFFER_NAME);

    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(buffers_.defaultBuffer, valid, buffers_.camera);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.light);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.lightCluster);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.mat);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.matTransform);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.matCustom);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.mesh);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.skinJoint);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all scene buffers not found.", renderNodeContextMgr_->GetName().data());
    }
}

void RenderNodeDefaultMaterialRenderSlot::GetCameraUniformBuffers()
{
    string camName;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        camName = jsonInputs_.customCameraName;
    }
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    buffers_.generalData =
        gpuResourceMgr.GetBufferHandle(DefaultMaterialCameraConstants::CAMERA_GENERAL_BUFFER_PREFIX_NAME + camName);
    buffers_.environment =
        gpuResourceMgr.GetBufferHandle(DefaultMaterialCameraConstants::CAMERA_ENVIRONMENT_BUFFER_PREFIX_NAME + camName);
    buffers_.fog =
        gpuResourceMgr.GetBufferHandle(DefaultMaterialCameraConstants::CAMERA_FOG_BUFFER_PREFIX_NAME + camName);
    buffers_.postProcess = gpuResourceMgr.GetBufferHandle(
        DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_BUFFER_PREFIX_NAME + camName);

    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(buffers_.defaultBuffer, valid, buffers_.generalData);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.environment);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.fog);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.postProcess);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all camera buffers found.", renderNodeContextMgr_->GetName().data());
    }
}

RenderHandle RenderNodeDefaultMaterialRenderSlot::CreateNewPso(
    const ShaderStateData& ssd, const ShaderSpecializationConstantDataView& spec, const RenderSubmeshFlags submeshFlags)
{
    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    // NOTE: The easiest route would be to input shader and graphics state to material component
    RenderHandle currShader;
    RenderHandle currPl;
    RenderHandle currVid;
    RenderHandle currState;
    // first try to find matching shader
    if (RenderHandleUtil::IsValid(ssd.shader)) {
        currShader = ssd.shader; // force given shader
        const RenderHandle slotShader = shaderMgr.GetShaderHandle(ssd.shader, jsonInputs_.renderSlotId);
        if (RenderHandleUtil::IsValid(slotShader)) {
            currShader = slotShader; // override with render slot variant
        }
        // if not explicit gfx state given, check if shader has graphics state for this slot
        if (!RenderHandleUtil::IsValid(ssd.gfxState)) {
            const auto gfxStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(currShader);
            if (shaderMgr.GetRenderSlotId(gfxStateHandle) == jsonInputs_.renderSlotId) {
                currState = gfxStateHandle;
            }
        }
        currVid = shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(currShader);
        currPl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(currShader);
    }
    if (RenderHandleUtil::IsValid(ssd.gfxState)) {
        const RenderHandle slotState = shaderMgr.GetGraphicsStateHandle(ssd.gfxState, jsonInputs_.renderSlotId);
        if (RenderHandleUtil::IsValid(slotState)) {
            currState = slotState;
        }
    }

    // fallback to defaults if needed
    currShader = RenderHandleUtil::IsValid(currShader) ? currShader : allShaderData_.defaultShaderHandle;
    currPl = RenderHandleUtil::IsValid(currPl) ? currPl : allShaderData_.defaultPlHandle;
    currVid = RenderHandleUtil::IsValid(currVid) ? currVid : allShaderData_.defaultVidHandle;
    currState = RenderHandleUtil::IsValid(currState) ? currState : allShaderData_.defaultStateHandle;

    auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
    RenderHandle psoHandle;
    if (IsInverseWinding(submeshFlags, jsonInputs_.nodeFlags, currentScene_.camera.flags)) {
        // we create a new graphics state with inverse winding
        GraphicsState gfxState = shaderMgr.GetGraphicsState(currState);
        gfxState.rasterizationState.frontFace = FrontFace::CORE_FRONT_FACE_CLOCKWISE;
        psoHandle = psoMgr.GetGraphicsPsoHandle(currShader, gfxState, shaderMgr.GetPipelineLayout(currPl),
            shaderMgr.GetVertexInputDeclarationView(currVid), spec, DYN_STATE_FLAGS);
    } else {
        psoHandle = psoMgr.GetGraphicsPsoHandle(currShader, currState, currPl, currVid, spec, DYN_STATE_FLAGS);
    }

    allShaderData_.perShaderData.emplace_back(PerShaderData { currShader, psoHandle, currState });
    allShaderData_.shaderIdToData[ssd.hash] = (uint32_t)allShaderData_.perShaderData.size() - 1;
    return psoHandle;
}

void RenderNodeDefaultMaterialRenderSlot::ProcessBuffersAndDescriptors(const ObjectCounts& objectCounts)
{
    constexpr uint32_t overEstimate { 16u };
    bool updateDescriptorSets = false;
    if (objectCounts_.maxSlotMeshCount < objectCounts.maxSlotMeshCount) {
        updateDescriptorSets = true;
        objectCounts_.maxSlotMeshCount = objectCounts.maxSlotMeshCount + (objectCounts.maxSlotMeshCount / overEstimate);
    }
    if (objectCounts_.maxSlotSkinCount < objectCounts.maxSlotSkinCount) {
        updateDescriptorSets = true;
        objectCounts_.maxSlotSkinCount = objectCounts.maxSlotSkinCount + (objectCounts.maxSlotSkinCount / overEstimate);
    }
    if (objectCounts_.maxSlotSubmeshCount < objectCounts.maxSlotSubmeshCount) {
        updateDescriptorSets = true;
        objectCounts_.maxSlotSubmeshCount =
            objectCounts.maxSlotSubmeshCount + (objectCounts.maxSlotSubmeshCount / overEstimate);
    }

    if (updateDescriptorSets) {
        ResetAndUpdateDescriptorSets();
    }
}

void RenderNodeDefaultMaterialRenderSlot::ResetAndUpdateDescriptorSets()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    // known and calculated (could run through descriptorsets and calculate automatically as well)
    const DescriptorCounts dc { {
        // camera + general data + post process + 3 light buffers + (mat + mat user data) * objectCount
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 6u },
        // light cluster
        { CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u },
        // mesh and material data
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 5u },
        // set0 (4) + per submesh material images
        { CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            4u + objectCounts_.maxSlotSubmeshCount * RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT },
    } };
    descriptorSetMgr.ResetAndReserve(dc);

    for (uint32_t setIdx = 0; setIdx < AllDescriptorSets::SINGLE_SET_COUNT; ++setIdx) {
        const RenderHandle descriptorSetHandle =
            descriptorSetMgr.CreateDescriptorSet(setIdx, allShaderData_.defaultPipelineLayout);
        allDescriptorSets_.set01[setIdx] = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetHandle, allShaderData_.defaultPipelineLayout.descriptorSetLayouts[setIdx].bindings);
    }
    {
        const uint32_t set = 2u;
        allDescriptorSets_.sets2.clear();
        allDescriptorSets_.sets2.resize(objectCounts_.maxSlotSubmeshCount);
        for (uint32_t idx = 0; idx < objectCounts_.maxSlotSubmeshCount; ++idx) {
            const RenderHandle descriptorSetHandle =
                descriptorSetMgr.CreateDescriptorSet(set, allShaderData_.defaultPipelineLayout);
            allDescriptorSets_.sets2[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, allShaderData_.defaultPipelineLayout.descriptorSetLayouts[set].bindings);
        }
    }
}

void RenderNodeDefaultMaterialRenderSlot::ProcessSlotSubmeshes(
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    // currentScene has been updated prior, has the correct camera (scene camera or custom camera)
    const IRenderNodeSceneUtil::RenderSlotInfo rsi { jsonInputs_.renderSlotId, jsonInputs_.sortType,
        jsonInputs_.cullType, jsonInputs_.nodeMaterialDiscardFlags };
    RenderNodeSceneUtil::GetRenderSlotSubmeshes(
        dataStoreCamera, dataStoreMaterial, currentScene_.cameraIdx, rsi, sortedSlotSubmeshes_);
}

void RenderNodeDefaultMaterialRenderSlot::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    jsonInputs_.sortType = parserUtil.GetRenderSlotSortType(jsonVal, "renderSlotSortType");
    jsonInputs_.cullType = parserUtil.GetRenderSlotCullType(jsonVal, "renderSlotCullType");
    jsonInputs_.nodeFlags = static_cast<uint32_t>(parserUtil.GetUintValue(jsonVal, "nodeFlags"));
    if (jsonInputs_.nodeFlags == ~0u) {
        jsonInputs_.nodeFlags = 0;
    }
    jsonInputs_.nodeMaterialDiscardFlags =
        static_cast<uint32_t>(parserUtil.GetUintValue(jsonVal, "nodeMaterialDiscardFlags"));
    if (jsonInputs_.nodeMaterialDiscardFlags == ~0u) {
        jsonInputs_.nodeMaterialDiscardFlags = 0;
    }
    // automatic default material velocity named target based parsing to add velocity calculations bit
    for (const auto& ref : jsonInputs_.renderPass.attachments) {
        if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_VELOCITY_NORMAL) {
            jsonInputs_.nodeSubmeshExtraFlags |= RenderSubmeshFlagBits::RENDER_SUBMESH_VELOCITY_BIT;
        }
    }

    const string renderSlot = parserUtil.GetStringValue(jsonVal, "renderSlot");
    jsonInputs_.renderSlotId = renderNodeContextMgr_->GetShaderManager().GetRenderSlotId(renderSlot);

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultMaterialRenderSlot::Create()
{
    return new RenderNodeDefaultMaterialRenderSlot();
}

void RenderNodeDefaultMaterialRenderSlot::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultMaterialRenderSlot*>(instance);
}
CORE3D_END_NAMESPACE()
