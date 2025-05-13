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

#include "render/default_constants.h"
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
constexpr string_view POST_PROCESS_DATA_STORE_TYPE_NAME { "RenderDataStorePod" };
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr DynamicStateEnum DYNAMIC_STATES_FSR[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR,
    CORE_DYNAMIC_STATE_ENUM_FRAGMENT_SHADING_RATE };

constexpr uint32_t UBO_BIND_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };

static constexpr uint64_t LIGHTING_FLAGS_SHIFT { 32ULL };
static constexpr uint64_t LIGHTING_FLAGS_MASK { 0xF00000000ULL };
static constexpr uint64_t POST_PROCESS_FLAGS_SHIFT { 36ULL };
static constexpr uint64_t POST_PROCESS_FLAGS_MASK { 0xF000000000ULL };
static constexpr uint64_t CAMERA_FLAGS_SHIFT { 40ULL };
static constexpr uint64_t CAMERA_FLAGS_MASK { 0xF0000000000ULL };
static constexpr uint64_t RENDER_HASH_FLAGS_MASK { 0xFFFFffffULL };
static constexpr uint64_t PRIMITIVE_TOPOLOGY_SHIFT { 44ULL };
// CORE_PRIMITIVE_TOPOLOGY_PATCH_LIST = 10
// CORE_PRIMITIVE_TOPOLOGY_MAX_ENUM is ignored in hashing
static constexpr uint64_t PRIMITIVE_TOPOLOGY_MASK { 0xF00000000000ULL };

// our light weight straight to screen post processes are only interested in these
static constexpr uint32_t POST_PROCESS_IMPORTANT_FLAGS_MASK { 0xffu };

static constexpr uint32_t FIXED_CUSTOM_SET3 { 3u };

inline void GetMultiViewCameraIndices(
    const IRenderDataStoreDefaultCamera& rds, const RenderCamera& cam, vector<uint32_t>& mvIndices)
{
    CORE_STATIC_ASSERT(RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT == 7U);
    const uint32_t inputCount =
        Math::min(cam.multiViewCameraCount, RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT);
    mvIndices.clear();
    mvIndices.reserve(inputCount);
    for (uint32_t idx = 0U; idx < inputCount; ++idx) {
        const uint64_t id = cam.multiViewCameraIds[idx];
        if (id != RenderSceneDataConstants::INVALID_ID) {
            mvIndices.push_back(Math::min(rds.GetCameraIndex(id), CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT - 1U));
        }
    }
}

inline uint64_t HashShaderDataAndSubmesh(const uint64_t shaderDataHash, const uint32_t renderHash,
    const IRenderDataStoreDefaultLight::LightingFlags lightingFlags, const RenderCamera::ShaderFlags& cameraShaderFlags,
    const PostProcessConfiguration::PostProcessEnableFlags postProcessFlags, const GraphicsState::InputAssembly& ia)
{
    const uint32_t ppEnabled = (postProcessFlags > 0);
    const uint64_t iaHash = uint32_t(ia.enablePrimitiveRestart) | (ia.primitiveTopology << 1U);
    uint64_t hash = ((uint64_t)renderHash & RENDER_HASH_FLAGS_MASK) |
                    (((uint64_t)lightingFlags << LIGHTING_FLAGS_SHIFT) & LIGHTING_FLAGS_MASK) |
                    (((uint64_t)ppEnabled << POST_PROCESS_FLAGS_SHIFT) & POST_PROCESS_FLAGS_MASK) |
                    (((uint64_t)cameraShaderFlags << CAMERA_FLAGS_SHIFT) & CAMERA_FLAGS_MASK) |
                    (((uint64_t)iaHash << PRIMITIVE_TOPOLOGY_SHIFT) & PRIMITIVE_TOPOLOGY_MASK);
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

void BindVertextBufferAndDraw(IRenderCommandList& cmdList, const RenderSubmesh& currSubmesh)
{
    // vertex buffers and draw
    if (currSubmesh.buffers.vertexBufferCount > 0U) {
        cmdList.BindVertexBuffers({ currSubmesh.buffers.vertexBuffers, currSubmesh.buffers.vertexBufferCount });
    }
    const auto& dc = currSubmesh.drawCommand;
    const VertexBuffer& iArgs = currSubmesh.buffers.indirectArgsBuffer;
    const bool indirectDraw = RenderHandleUtil::IsValid(iArgs.bufferHandle);
    if ((currSubmesh.buffers.indexBuffer.byteSize > 0U) &&
        RenderHandleUtil::IsValid(currSubmesh.buffers.indexBuffer.bufferHandle)) {
        cmdList.BindIndexBuffer(currSubmesh.buffers.indexBuffer);
        if (indirectDraw) {
            cmdList.DrawIndexedIndirect(
                iArgs.bufferHandle, iArgs.bufferOffset, dc.drawCountIndirect, dc.strideIndirect);
        } else {
            cmdList.DrawIndexed(dc.indexCount, dc.instanceCount, 0, 0, 0);
        }
    } else {
        if (indirectDraw) {
            cmdList.DrawIndirect(iArgs.bufferHandle, iArgs.bufferOffset, dc.drawCountIndirect, dc.strideIndirect);
        } else {
            cmdList.Draw(dc.vertexCount, dc.instanceCount, 0, 0);
        }
    }
}

RenderDataDefaultMaterial::SubmeshMaterialFlags GetSubmeshMaterialFlags(
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const bool instanced, const bool hasShadows)
{
    // create a new copy and modify if needed (force instancing on and off)
    RenderDataDefaultMaterial::SubmeshMaterialFlags materialFlags = submeshMaterialFlags;
    if (!hasShadows) { // remove shadow if not in scene
        materialFlags.renderMaterialFlags &= (~RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_RECEIVER_BIT);
        materialFlags.renderHash = dataStoreMaterial.GenerateRenderHash(materialFlags);
    }
    return materialFlags;
}

RenderNodeDefaultMaterialRenderSlot::FrameGlobalDescriptorSets GetFrameGlobalDescriptorSets(
    IRenderNodeContextManager* rncm, const SceneRenderDataStores& stores, const string& cameraName)
{
    RenderNodeDefaultMaterialRenderSlot::FrameGlobalDescriptorSets fgds;
    if (rncm) {
        // re-fetch global descriptor sets every frame
        const INodeContextDescriptorSetManager& dsMgr = rncm->GetDescriptorSetManager();
        const string_view us = stores.dataStoreNameScene;
        fgds.set0 = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_SET0_GLOBAL_DESCRIPTOR_SET_PREFIX_NAME + cameraName);
        fgds.set1 = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_SET1_GLOBAL_DESCRIPTOR_SET_NAME);
        fgds.set2 = dsMgr.GetGlobalDescriptorSets(
            us + DefaultMaterialMaterialConstants::MATERIAL_RESOURCES_GLOBAL_DESCRIPTOR_SET_NAME);
        fgds.set2Default = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_DEFAULT_RESOURCE_GLOBAL_DESCRIPTOR_SET_NAME);
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (fgds.set2.empty()) {
            CORE_LOG_ONCE_W("core3d_global_descriptor_set_render_slot_issues",
                "CORE3D_VALIDATION: Global descriptor set for default material env not found");
        }
#endif
        fgds.valid = RenderHandleUtil::IsValid(fgds.set0) && RenderHandleUtil::IsValid(fgds.set1) &&
                     RenderHandleUtil::IsValid(fgds.set2Default);
        if (!fgds.valid) {
            CORE_LOG_ONCE_E("core3d_global_descriptor_set_rs_all_issues",
                "Global descriptor set 0/1/2 for default material not "
                "found (RenderNodeDefaultCameraController needed)");
        }
    }
    return fgds;
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

    if ((jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) &&
        jsonInputs_.renderDataStore.dataStoreName.empty()) {
        CORE_LOG_V("%s: render data store post process configuration not set in render node graph",
            renderNodeContextMgr_->GetName().data());
    }
    rngRenderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    CreateDefaultShaderData();

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    defaultSamplers_.cubemapHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    defaultSamplers_.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    defaultSamplers_.nearestHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    defaultSamplers_.linearMipHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");
    defaultColorPrePassHandle_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
}

void RenderNodeDefaultMaterialRenderSlot::PreExecuteFrame()
{
    // re-create needed gpu resources
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

#if (CORE3D_VALIDATION_ENABLED == 1)
    RENDER_DEBUG_MARKER_COL_SCOPE(
        cmdList, "3DMaterial" + jsonInputs_.renderSlotName, DefaultDebugConstants::DEFAULT_DEBUG_COLOR);
#else
    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "3DMaterial", DefaultDebugConstants::DEFAULT_DEBUG_COLOR);
#endif

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
    // re-fetch global descriptor sets every frame
    const FrameGlobalDescriptorSets fgds = GetFrameGlobalDescriptorSets(renderNodeContextMgr_, stores_, cameraName_);
    if (!fgds.valid) {
        return; // cannot continue
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);
    if (fsrEnabled_) {
        cmdList.SetDynamicStateFragmentShadingRate(
            { 1u, 1u }, FragmentShadingRateCombinerOps { CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE,
                            CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE });
    }

    // first two sets are only bound for the first submesh (inside for loop)
    // no bindless, we need to update images per object
    PipelineInfo pipelineInfo;
    uint32_t currMaterialIndex = ~0u;
    bool initialBindDone = false; // cannot be checked from the idx

    const auto& submeshMaterialFlags = dataStoreMaterial.GetSubmeshMaterialFlags();
    const auto& submeshes = dataStoreMaterial.GetSubmeshes();
    const auto& customResourceHandles = dataStoreMaterial.GetCustomResourceHandles();
    const uint64_t camLayerMask = currentScene_.camera.layerMask;
    const uint64_t camScene = currentScene_.camera.sceneId;

    for (const auto& ssp : sortedSlotSubmeshes_) {
        const uint32_t submeshIndex = ssp.submeshIndex;
        const auto& currSubmesh = submeshes[submeshIndex];
        if (currSubmesh.layers.sceneId != camScene) {
            continue;
        }
        // sorted slot submeshes should already have removed layers if default sorting was used
        if (((camLayerMask & currSubmesh.layers.layerMask) == 0U) ||
            ((jsonInputs_.nodeFlags & RENDER_SCENE_DISCARD_MATERIAL_BIT) &&
                (submeshMaterialFlags[submeshIndex].extraMaterialRenderingFlags &
                    RenderExtraRenderingFlagBits::RENDER_EXTRA_RENDERING_DISCARD_BIT))) {
            continue;
        }
        const auto materialSubmeshFlags = GetSubmeshMaterialFlags(submeshMaterialFlags[submeshIndex], dataStoreMaterial,
            (currSubmesh.drawCommand.instanceCount > 1U), currentScene_.hasShadow);
        const RenderSubmeshFlags submeshFlags = currSubmesh.submeshFlags | jsonInputs_.nodeSubmeshExtraFlags;

        BindPipeline(cmdList, ssp, materialSubmeshFlags, submeshFlags, currSubmesh.buffers.inputAssembly, pipelineInfo);

        // bind first set only the first time
        if (!initialBindDone) {
            cmdList.BindDescriptorSet(0, fgds.set0);
        }

        currMaterialIndex = BindSet1And2(cmdList, currSubmesh, submeshFlags, initialBindDone, fgds, currMaterialIndex);
        initialBindDone = true;

        // custom set 3 resources
        if (pipelineInfo.boundCustomSetNeed) {
            // if we do not have custom resources, and if we have binding issues we skip
            if ((currSubmesh.indices.materialIndex >= static_cast<uint32_t>(customResourceHandles.size())) ||
                (customResourceHandles[currSubmesh.indices.materialIndex].resourceHandleCount == 0U) ||
                !UpdateAndBindSet3(cmdList, customResourceHandles[currSubmesh.indices.materialIndex])) {
#if (CORE3D_VALIDATION_ENABLED == 1)
                CORE_LOG_ONCE_W("material_render_slot_custom_set3_issue",
                    "invalid bindings with custom shader descriptor set 3 (render node: %s)",
                    renderNodeContextMgr_->GetName().data());
#endif
                continue; // we prevent drawing
            }
        }

        BindVertextBufferAndDraw(cmdList, currSubmesh);
    }
}

void RenderNodeDefaultMaterialRenderSlot::BindPipeline(IRenderCommandList& cmdList, const SlotSubmeshIndex& ssp,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& renderSubmeshMaterialFlags,
    const RenderSubmeshFlags submeshFlags, const GraphicsState::InputAssembly& inputAssembly,
    PipelineInfo& pipelineInfo)
{
    ShaderStateData ssd { ssp.shaderHandle, ssp.gfxStateHandle, 0 };
    ssd.hash = (ssd.shader.id << 32U) | (ssd.gfxState.id & 0xFFFFffff);
    // current shader state is fetched for build-in and custom shaders (decision is made later)
    ssd.hash = HashShaderDataAndSubmesh(ssd.hash, renderSubmeshMaterialFlags.renderHash, currentScene_.lightingFlags,
        currentScene_.cameraShaderFlags, currentRenderPPConfiguration_.flags.x, inputAssembly);
    if (ssd.hash != pipelineInfo.boundShaderHash) {
        const PsoAndInfo psoAndInfo = GetSubmeshPso(ssd, inputAssembly, renderSubmeshMaterialFlags, submeshFlags,
            currentScene_.lightingFlags, currentScene_.cameraShaderFlags);
        if (psoAndInfo.pso != pipelineInfo.boundPsoHandle) {
            pipelineInfo.boundShaderHash = ssd.hash;
            pipelineInfo.boundPsoHandle = psoAndInfo.pso;
            cmdList.BindPipeline(pipelineInfo.boundPsoHandle);
            pipelineInfo.boundCustomSetNeed = psoAndInfo.set3;
        }
    }
}

uint32_t RenderNodeDefaultMaterialRenderSlot::BindSet1And2(IRenderCommandList& cmdList,
    const RenderSubmesh& currSubmesh, const RenderSubmeshFlags submeshFlags, const bool initialBindDone,
    const FrameGlobalDescriptorSets& fgds, uint32_t currMaterialIndex)

{
    // set 1 (mesh matrix, skin matrices, material, material user data)
    const uint32_t currMatOffset = currSubmesh.indices.materialFrameOffset * UBO_BIND_OFFSET_ALIGNMENT;
    const uint32_t dynamicOffsets[] = { currSubmesh.indices.meshIndex * UBO_BIND_OFFSET_ALIGNMENT,
        (submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT)
            ? currSubmesh.indices.skinJointIndex * static_cast<uint32_t>(sizeof(DefaultMaterialSkinStruct))
            : 0U,
        currMatOffset, currMatOffset, currMatOffset };
    // set to bind, handle to resource, offsets for dynamic descs
    IRenderCommandList::BindDescriptorSetData bindSets[2U] {};
    uint32_t bindSetCount = 0U;
    bindSets[bindSetCount++] = { fgds.set1, dynamicOffsets };

    // update material descriptor set
    if ((!initialBindDone) || (currMaterialIndex != currSubmesh.indices.materialIndex)) {
        currMaterialIndex = currSubmesh.indices.materialIndex;
        // safety check for global material sets
        const RenderHandle set2Handle =
            (currMaterialIndex < fgds.set2.size()) ? fgds.set2[currMaterialIndex] : fgds.set2Default;
        bindSets[bindSetCount++] = { set2Handle, {} };
    }

    // bind sets 1 and possibly 2
    cmdList.BindDescriptorSets(1U, { bindSets, bindSetCount });
    return currMaterialIndex;
}

bool RenderNodeDefaultMaterialRenderSlot::UpdateAndBindSet3(
    IRenderCommandList& cmdList, const RenderDataDefaultMaterial::CustomResourceData& customResourceData)
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    // with default pipeline layout with default bindings
    RenderHandle currPlHandle = allShaderData_.defaultPlSet3
                                    ? allShaderData_.defaultPlHandle
                                    : shaderMgr.GetPipelineLayoutHandleByShaderHandle(customResourceData.shaderHandle);
    if (!RenderHandleUtil::IsValid(currPlHandle)) {
        currPlHandle = shaderMgr.GetReflectionPipelineLayoutHandle(customResourceData.shaderHandle);
    }
    const PipelineLayout& plRef = shaderMgr.GetPipelineLayout(currPlHandle);
    if (plRef.descriptorSetLayouts[FIXED_CUSTOM_SET3].bindings.empty()) {
        return false; // early out
    }
    static_assert(FIXED_CUSTOM_SET3 <
                  BASE_NS::extent_v<decltype(BASE_NS::remove_reference_t<decltype(plRef)>::descriptorSetLayouts)>);
    const auto& descBindings = plRef.descriptorSetLayouts[FIXED_CUSTOM_SET3].bindings;
    const RenderHandle descSetHandle = descriptorSetMgr.CreateOneFrameDescriptorSet(descBindings);
    if (!RenderHandleUtil::IsValid(descSetHandle) || (descBindings.size() != customResourceData.resourceHandleCount)) {
        return false;
    }
    IDescriptorSetBinder::Ptr binderPtr = descriptorSetMgr.CreateDescriptorSetBinder(descSetHandle, descBindings);
    if (!binderPtr) {
        return false;
    }
    auto& binder = *binderPtr;
    for (uint32_t idx = 0; idx < customResourceData.resourceHandleCount; ++idx) {
        CORE_ASSERT(idx < descBindings.size());
        const RenderHandle& currRes = customResourceData.resourceHandles[idx];
        if (gpuResourceMgr.IsGpuBuffer(currRes)) {
            binder.BindBuffer(idx, currRes, 0);
        } else if (gpuResourceMgr.IsGpuImage(currRes)) {
            if (descBindings[idx].descriptorType == DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                binder.BindImage(idx, currRes, defaultSamplers_.linearMipHandle);
            } else {
                binder.BindImage(idx, currRes);
            }
        } else if (gpuResourceMgr.IsGpuSampler(currRes)) {
            binder.BindSampler(idx, currRes);
        }
    }

    // user generated setup, we check for validity of all bindings in the descriptor set
    if (!binder.GetDescriptorSetLayoutBindingValidity()) {
        return false;
    }
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(FIXED_CUSTOM_SET3, binder.GetDescriptorSetHandle());
    return true;
}

RenderNodeDefaultMaterialRenderSlot::PsoAndInfo RenderNodeDefaultMaterialRenderSlot::GetSubmeshPso(
    const ShaderStateData& ssd, const GraphicsState::InputAssembly& ia,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags, const RenderSubmeshFlags submeshFlags,
    const IRenderDataStoreDefaultLight::LightingFlags lightingFlags, const RenderCamera::ShaderFlags cameraShaderFlags)
{
    if (const auto dataIter = allShaderData_.shaderIdToData.find(ssd.hash);
        dataIter != allShaderData_.shaderIdToData.cend()) {
        const auto& ref = allShaderData_.perShaderData[dataIter->second];
        return { ref.psoHandle, ref.needsCustomSetBindings };
    }

    return CreateNewPso(ssd, ia, submeshMaterialFlags, submeshFlags, lightingFlags, cameraShaderFlags);
}

void RenderNodeDefaultMaterialRenderSlot::UpdatePostProcessConfiguration()
{
    if (jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) {
        if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
            auto const& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
            if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName); ds) {
                if (jsonInputs_.renderDataStore.typeName == POST_PROCESS_DATA_STORE_TYPE_NAME) {
                    auto const dataStore = static_cast<const IRenderDataStorePod*>(ds);
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
}

void RenderNodeDefaultMaterialRenderSlot::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
        rngRenderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    }
    // get default RNG based render pass setup
    renderPass_ = rngRenderPass_;

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

    const auto camHandles = RenderNodeSceneUtil::GetSceneCameraImageHandles(
        *renderNodeContextMgr_, stores_.dataStoreNameScene, currentScene_.camera.name, currentScene_.camera);
    currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;

    if (!currentScene_.camera.prePassColorTargetName.empty()) {
        currentScene_.prePassColorTarget =
            renderNodeContextMgr_->GetGpuResourceManager().GetImageHandle(currentScene_.camera.prePassColorTargetName);
    }

    // renderpass needs to be valid (created in init)
    if (hasCustomCamera) {
        // uses camera based loadOp clear override if given
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
    currentScene_.cameraShaderFlags = currentScene_.camera.shaderFlags;
    // remove fog explicitly if render node graph input and/or default render slot usage states so
    if (jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DISABLE_FOG_BIT) {
        currentScene_.cameraShaderFlags &= (~RenderCamera::ShaderFlagBits::CAMERA_SHADER_FOG_BIT);
    }
    GetMultiViewCameraIndices(dataStoreCamera, currentScene_.camera, currentScene_.mvCameraIndices);
    // add multi-view flags if needed
    if (renderPass_.subpassDesc.viewMask > 1U) {
        ResetRenderSlotData(jsonInputs_.shaderRenderSlotMultiviewId, true);
    } else {
        ResetRenderSlotData(jsonInputs_.shaderRenderSlotBaseId, false);
    }
}

void RenderNodeDefaultMaterialRenderSlot::CreateDefaultShaderData()
{
    allShaderData_ = {};

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    // get the default material shader and default shader state
    const IShaderManager::RenderSlotData shaderRsd = shaderMgr.GetRenderSlotData(jsonInputs_.shaderRenderSlotId);
    allShaderData_.defaultShaderHandle = shaderRsd.shader.GetHandle();
    allShaderData_.defaultStateHandle = shaderRsd.graphicsState.GetHandle();
    // get the pl and vid defaults
    allShaderData_.defaultPlHandle =
        (shaderRsd.pipelineLayout)
            ? shaderRsd.pipelineLayout.GetHandle()
            : shaderMgr.GetPipelineLayoutHandle(DefaultMaterialShaderConstants::PIPELINE_LAYOUT_FORWARD);
    allShaderData_.defaultPipelineLayout = shaderMgr.GetPipelineLayout(allShaderData_.defaultPlHandle);
    allShaderData_.defaultTmpPipelineLayout = allShaderData_.defaultPipelineLayout;
    allShaderData_.defaultVidHandle = (shaderRsd.vertexInputDeclaration)
                                          ? shaderRsd.vertexInputDeclaration.GetHandle()
                                          : shaderMgr.GetVertexInputDeclarationHandle(
                                                DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD);
    if (!allShaderData_.defaultPipelineLayout.descriptorSetLayouts[FIXED_CUSTOM_SET3].bindings.empty()) {
        allShaderData_.defaultPlSet3 = true;
    }

    if (shaderMgr.IsShader(allShaderData_.defaultShaderHandle)) {
        allShaderData_.slotHasShaders = true;
        const ShaderSpecializationConstantView& sscv =
            shaderMgr.GetReflectionSpecialization(allShaderData_.defaultShaderHandle);
        allShaderData_.defaultSpecilizationConstants.resize(sscv.constants.size());
        for (uint32_t idx = 0; idx < (uint32_t)allShaderData_.defaultSpecilizationConstants.size(); ++idx) {
            allShaderData_.defaultSpecilizationConstants[idx] = sscv.constants[idx];
        }
        specializationData_.maxSpecializationCount =
            Math::min(static_cast<uint32_t>(allShaderData_.defaultSpecilizationConstants.size()),
                SpecializationData::MAX_FLAG_COUNT);
    } else {
        CORE_LOG_I("RenderNode: %s, no default shaders for render slot id %u", renderNodeContextMgr_->GetName().data(),
            jsonInputs_.shaderRenderSlotId);
    }
    if (jsonInputs_.shaderRenderSlotId != jsonInputs_.stateRenderSlotId) {
        const IShaderManager::RenderSlotData stateRsd = shaderMgr.GetRenderSlotData(jsonInputs_.stateRenderSlotId);
        if (stateRsd.graphicsState) {
            allShaderData_.defaultStateHandle = stateRsd.graphicsState.GetHandle();
        } else {
            CORE_LOG_I("RenderNode: %s, no default state for render slot id %u",
                renderNodeContextMgr_->GetName().data(), jsonInputs_.stateRenderSlotId);
        }
    }
}

namespace {
// updates graphics state based on params
inline GraphicsState GetNewGraphicsState(const IRenderNodeShaderManager& shaderMgr, const RenderHandle& handle,
    const bool inverseWinding, const bool customInputAssembly, const GraphicsState::InputAssembly& ia)
{
    // we create a new graphics state based on current
    GraphicsState gfxState = shaderMgr.GetGraphicsState(handle);
    // update state
    if (inverseWinding) {
        gfxState.rasterizationState.frontFace = FrontFace::CORE_FRONT_FACE_CLOCKWISE;
    }
    if (customInputAssembly) {
        gfxState.inputAssembly = ia;
    }
    return gfxState;
}
} // namespace

RenderNodeDefaultMaterialRenderSlot::PsoAndInfo RenderNodeDefaultMaterialRenderSlot::CreateNewPso(
    const ShaderStateData& ssd, const GraphicsState::InputAssembly& ia,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMatFlags, const RenderSubmeshFlags submeshFlags,
    const IRenderDataStoreDefaultLight::LightingFlags lightingFlags, const RenderCamera::ShaderFlags camShaderFlags)
{
    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    // NOTE: The easiest route would be to input shader and graphics state to material component
    RenderHandle currShader;
    RenderHandle currVid;
    RenderHandle currState;
    // first try to find matching shader
    if (RenderHandleUtil::GetHandleType(ssd.shader) == RenderHandleType::SHADER_STATE_OBJECT) {
        // we force the given shader if explicit shader render slot is not given
        if (!jsonInputs_.explicitShader) {
            currShader = ssd.shader;
        }
        const RenderHandle slotShader = shaderMgr.GetShaderHandle(ssd.shader, jsonInputs_.shaderRenderSlotId);
        if (RenderHandleUtil::IsValid(slotShader)) {
            currShader = slotShader; // override with render slot variant
        }
        // if not explicit gfx state given, check if shader has graphics state for this slot
        if (!RenderHandleUtil::IsValid(ssd.gfxState)) {
            const auto gfxStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(currShader);
            if (shaderMgr.GetRenderSlotId(gfxStateHandle) == jsonInputs_.stateRenderSlotId) {
                currState = gfxStateHandle;
            }
        }
        currVid = shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(currShader);
    }
    if (RenderHandleUtil::GetHandleType(ssd.gfxState) == RenderHandleType::GRAPHICS_STATE) {
        const RenderHandle slotState = shaderMgr.GetGraphicsStateHandle(ssd.gfxState, jsonInputs_.stateRenderSlotId);
        if (RenderHandleUtil::IsValid(slotState)) {
            currState = slotState;
        }
    }

    bool needsCustomSet = false;
    const PipelineLayout& pl = GetEvaluatedPipelineLayout(currShader, needsCustomSet);

    // fallback to defaults if needed
    currShader = RenderHandleUtil::IsValid(currShader) ? currShader : allShaderData_.defaultShaderHandle;
    currVid = RenderHandleUtil::IsValid(currVid) ? currVid : allShaderData_.defaultVidHandle;
    currState = RenderHandleUtil::IsValid(currState) ? currState : allShaderData_.defaultStateHandle;

    auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
    RenderHandle psoHandle;
    const bool inverseWinding = IsInverseWinding(submeshFlags, jsonInputs_.nodeFlags, currentScene_.camera.flags);
    const bool customIa = (ia.primitiveTopology != CORE_PRIMITIVE_TOPOLOGY_MAX_ENUM) || (ia.enablePrimitiveRestart);
    const VertexInputDeclarationView vid = shaderMgr.GetVertexInputDeclarationView(currVid);
    if (inverseWinding || customIa) {
        const GraphicsState state = GetNewGraphicsState(shaderMgr, currState, inverseWinding, customIa, ia);
        const auto spec = GetShaderSpecView(state, submeshMatFlags, submeshFlags, lightingFlags, camShaderFlags);
        psoHandle = psoMgr.GetGraphicsPsoHandle(currShader, state, pl, vid, spec, GetDynamicStates());

    } else {
        // graphics state in default mode
        const GraphicsState& state = shaderMgr.GetGraphicsState(currState);
        const auto spec = GetShaderSpecView(state, submeshMatFlags, submeshFlags, lightingFlags, camShaderFlags);
        psoHandle = psoMgr.GetGraphicsPsoHandle(currShader, state, pl, vid, spec, GetDynamicStates());
    }

    allShaderData_.perShaderData.push_back(PerShaderData { currShader, psoHandle, currState, needsCustomSet });
    allShaderData_.shaderIdToData[ssd.hash] = (uint32_t)allShaderData_.perShaderData.size() - 1;
    return { psoHandle, needsCustomSet };
}

ShaderSpecializationConstantDataView RenderNodeDefaultMaterialRenderSlot::GetShaderSpecView(
    const RENDER_NS::GraphicsState& gfxState, const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMatFlags,
    const RenderSubmeshFlags submeshFlags, const IRenderDataStoreDefaultLight::LightingFlags lightingFlags,
    const RenderCamera::ShaderFlags camShaderFlags)
{
    RenderMaterialFlags combinedMaterialFlags = submeshMatFlags.renderMaterialFlags;
    if (gfxState.colorBlendState.colorAttachmentCount > 0) {
        // enable opaque flag if blending is not enabled with the first color attachment
        combinedMaterialFlags |= (gfxState.colorBlendState.colorAttachments[0].enableBlend)
                                     ? 0u
                                     : RenderMaterialFlagBits::RENDER_MATERIAL_OPAQUE_BIT;
    }
    for (uint32_t idx = 0; idx < specializationData_.maxSpecializationCount; ++idx) {
        const auto& ref = allShaderData_.defaultSpecilizationConstants[idx];
        const uint32_t constantId = ref.offset / sizeof(uint32_t);

        // NOTE: vertex and fragment have different specializations for the zero index
        if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT) {
            if (ref.id == CORE_DM_CONSTANT_ID_SUBMESH_FLAGS) {
                specializationData_.flags[constantId] = submeshFlags;
            } else if (ref.id == CORE_DM_CONSTANT_ID_MATERIAL_FLAGS) {
                specializationData_.flags[constantId] = combinedMaterialFlags;
            }
        } else if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT) {
            if (ref.id == CORE_DM_CONSTANT_ID_MATERIAL_TYPE) {
                specializationData_.flags[constantId] = static_cast<uint32_t>(submeshMatFlags.materialType);
            } else if (ref.id == CORE_DM_CONSTANT_ID_MATERIAL_FLAGS) {
                specializationData_.flags[constantId] = combinedMaterialFlags;
            } else if (ref.id == CORE_DM_CONSTANT_ID_LIGHTING_FLAGS) {
                specializationData_.flags[constantId] = lightingFlags;
            } else if (ref.id == CORE_DM_CONSTANT_ID_POST_PROCESS_FLAGS) {
                specializationData_.flags[constantId] = currentRenderPPConfiguration_.flags.x;
            } else if (ref.id == CORE_DM_CONSTANT_ID_CAMERA_FLAGS) {
                specializationData_.flags[constantId] = camShaderFlags;
            }
        }
    }

    return { { allShaderData_.defaultSpecilizationConstants.data(), specializationData_.maxSpecializationCount },
        { specializationData_.flags, specializationData_.maxSpecializationCount } };
}

void RenderNodeDefaultMaterialRenderSlot::ProcessSlotSubmeshes(
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    // currentScene has been updated prior, has the correct camera (scene camera or custom camera)
    const IRenderNodeSceneUtil::RenderSlotInfo rsi { jsonInputs_.renderSlotId, jsonInputs_.sortType,
        jsonInputs_.cullType, jsonInputs_.nodeMaterialDiscardFlags };

    RenderNodeSceneUtil::GetRenderSlotSubmeshes(dataStoreCamera, dataStoreMaterial, currentScene_.cameraIdx,
        currentScene_.mvCameraIndices, rsi, sortedSlotSubmeshes_);
}

array_view<const DynamicStateEnum> RenderNodeDefaultMaterialRenderSlot::GetDynamicStates() const
{
    if (fsrEnabled_) {
        return { DYNAMIC_STATES_FSR, countof(DYNAMIC_STATES_FSR) };
    } else {
        return { DYNAMIC_STATES, countof(DYNAMIC_STATES) };
    }
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

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    jsonInputs_.renderSlotName = parserUtil.GetStringValue(jsonVal, "renderSlot");
    jsonInputs_.renderSlotId = shaderMgr.GetRenderSlotId(jsonInputs_.renderSlotName);
    jsonInputs_.shaderRenderSlotId = jsonInputs_.renderSlotId;
    jsonInputs_.stateRenderSlotId = jsonInputs_.renderSlotId;
    const string shaderRenderSlot = parserUtil.GetStringValue(jsonVal, "shaderRenderSlot");
    if (!shaderRenderSlot.empty()) {
        const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(shaderRenderSlot);
        if (renderSlotId != ~0U) {
            jsonInputs_.shaderRenderSlotId = renderSlotId;
            jsonInputs_.initialExplicitShader = true;
            jsonInputs_.explicitShader = true;
        }
    }
    jsonInputs_.shaderRenderSlotBaseId = jsonInputs_.shaderRenderSlotId;
    const string stateRenderSlot = parserUtil.GetStringValue(jsonVal, "stateRenderSlot");
    if (!stateRenderSlot.empty()) {
        const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(stateRenderSlot);
        jsonInputs_.stateRenderSlotId = (renderSlotId != ~0U) ? renderSlotId : jsonInputs_.renderSlotId;
    }
    const string shaderMultiviewRenderSlot = parserUtil.GetStringValue(jsonVal, "shaderMultiviewRenderSlot");
    if (!shaderMultiviewRenderSlot.empty()) {
        jsonInputs_.shaderRenderSlotMultiviewId = shaderMgr.GetRenderSlotId(shaderMultiviewRenderSlot);
    }

    EvaluateFogBits();

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    if ((inputRenderPass_.fragmentShadingRateAttachmentIndex < inputRenderPass_.attachments.size()) &&
        RenderHandleUtil::IsValid(
            inputRenderPass_.attachments[inputRenderPass_.fragmentShadingRateAttachmentIndex].handle)) {
        fsrEnabled_ = true;
    }
    jsonInputs_.hasChangeableRenderPassHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.renderPass);

    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        cameraName_ = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        cameraName_ = jsonInputs_.customCameraName;
    }
}

void RenderNodeDefaultMaterialRenderSlot::ResetRenderSlotData(const uint32_t shaderRenderSlotId, const bool multiView)
{
    // can be reset to multi-view usage or reset back to default usage
    if (shaderRenderSlotId != jsonInputs_.shaderRenderSlotId) {
        jsonInputs_.shaderRenderSlotId = shaderRenderSlotId;
        jsonInputs_.explicitShader = jsonInputs_.initialExplicitShader || multiView;
        // reset
        CreateDefaultShaderData();
    }
}

void RenderNodeDefaultMaterialRenderSlot::EvaluateFogBits()
{
    // if no explicit bits set we check default render slot usages
    if ((jsonInputs_.nodeFlags & (RENDER_SCENE_ENABLE_FOG_BIT | RENDER_SCENE_DISABLE_FOG_BIT)) == 0) {
        // check default render slots
        const uint32_t opaqueSlotId = renderNodeContextMgr_->GetShaderManager().GetRenderSlotId(
            DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
        const uint32_t translucentSlotId = renderNodeContextMgr_->GetShaderManager().GetRenderSlotId(
            DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT);
        if ((jsonInputs_.renderSlotId == opaqueSlotId) || (jsonInputs_.renderSlotId == translucentSlotId)) {
            jsonInputs_.nodeFlags |= RenderSceneFlagBits::RENDER_SCENE_ENABLE_FOG_BIT;
        }
    }
}

const PipelineLayout& RenderNodeDefaultMaterialRenderSlot::GetEvaluatedPipelineLayout(
    const RenderHandle& currShader, bool& needsCustomSet)
{
    // the inputs need to be in certain "state"
    // currShader is valid if there's custom shader

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    // clear custom tmp pipeline layouts
    auto& tmpPl = allShaderData_.defaultTmpPipelineLayout;

    auto UpdateCustomPl = [](const DescriptorSetLayout& dsl, PipelineLayout& tmpPl) {
        if (!dsl.bindings.empty()) {
            tmpPl.descriptorSetLayouts[FIXED_CUSTOM_SET3] = dsl;
            return true;
        }
        return false;
    };

    const bool def3NoShader = ((!RenderHandleUtil::IsValid(currShader)) && (allShaderData_.defaultPlSet3));
    if (RenderHandleUtil::IsValid(currShader) || def3NoShader) {
        const RenderHandle shader = def3NoShader ? allShaderData_.defaultShaderHandle : currShader;
        RenderHandle reflPl;
        RenderHandle currPl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(shader);
        if (RenderHandleUtil::IsValid(currPl)) {
            const auto& plSet = shaderMgr.GetPipelineLayout(currPl).descriptorSetLayouts[FIXED_CUSTOM_SET3];
            needsCustomSet = UpdateCustomPl(plSet, tmpPl);
        }
        if ((!needsCustomSet) && (!RenderHandleUtil::IsValid(currPl))) {
            reflPl = shaderMgr.GetReflectionPipelineLayoutHandle(shader);
            const auto& plSet = shaderMgr.GetPipelineLayout(reflPl).descriptorSetLayouts[FIXED_CUSTOM_SET3];
            needsCustomSet = UpdateCustomPl(plSet, tmpPl);
        }
#if (CORE3D_VALIDATION_ENABLED == 1)
        {
            if (!RenderHandleUtil::IsValid(reflPl)) {
                reflPl = shaderMgr.GetReflectionPipelineLayoutHandle(shader);
            }
            const IShaderManager::CompatibilityFlags flags =
                shaderMgr.GetCompatibilityFlags(allShaderData_.defaultPlHandle, reflPl);
            if (flags == 0) {
                const auto idDesc = shaderMgr.GetIdDesc(shader);
                CORE_LOG_W("Compatibility issue with 3D default material shaders (name: %s, path: %s)",
                    idDesc.displayName.c_str(), idDesc.path.c_str());
            }
        }
#endif
    }

    // return modified custom pipeline layout or the default
    if (needsCustomSet) {
        return allShaderData_.defaultTmpPipelineLayout;
    } else {
        return allShaderData_.defaultPipelineLayout;
    }
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
