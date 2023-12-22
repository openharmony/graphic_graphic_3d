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

#include "render_node_default_material_deferred_shading.h"

#include <algorithm>

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/string.h>
#include <base/math/matrix.h>
#include <base/math/matrix_util.h>
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

#include "render/render_node_scene_util.h"

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>
} // namespace
CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr float CUBE_MAP_LOD_COEFF { 8.0f };
// our light weight straight to screen post processes are only interested in these
static constexpr uint32_t POST_PROCESS_IMPORTANT_FLAGS_MASK { 0xffu };

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

RenderNodeDefaultMaterialDeferredShading::ShadowBuffers GetShadowBufferNodeData(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view sceneName)
{
    RenderNodeDefaultMaterialDeferredShading::ShadowBuffers sb;
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

    sb.pcfDepthHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_DEPTH_BUFFER_NAME);
    sb.vsmColorHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_VSM_COLOR_BUFFER_NAME);
    if (!RenderHandleUtil::IsValid(sb.pcfDepthHandle)) {
        sb.pcfDepthHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE_WHITE");
    }
    if (!RenderHandleUtil::IsValid(sb.vsmColorHandle)) {
        sb.vsmColorHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    }

    return sb;
}
} // namespace

void RenderNodeDefaultMaterialDeferredShading::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    currentScene_ = {};

    if ((jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) &&
        jsonInputs_.renderDataStore.dataStoreName.empty()) {
        CORE_LOG_V("%s: render data store post process configuration not set in render node graph",
            renderNodeContextMgr_->GetName().data());
    }

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    bufferHandles_.defaultBuffer = gpuResourceMgr.GetBufferHandle("CORE_DEFAULT_GPU_BUFFER");
    GetSceneUniformBuffers(stores_.dataStoreNameScene);
    GetCameraUniformBuffers();
    shadowBuffers_ = GetShadowBufferNodeData(gpuResourceMgr, stores_.dataStoreNameScene);

    renderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);

    createdTargets_.cubemapSampler = CreateCubeNodeSampler(gpuResourceMgr);
    samplerHandles_.cubemap = createdTargets_.cubemapSampler.GetHandle();
    samplerHandles_.linear = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    samplerHandles_.nearest = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    samplerHandles_.linearMip = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");

    defaultColorPrePassHandle_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    defaultSkyBoxRadianceCubemap_ =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP);
    allShaderData_.shaderHandle = shader_;
    CreateDefaultShaderData();
    CreateDescriptorSets();
}

void RenderNodeDefaultMaterialDeferredShading::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeDefaultMaterialDeferredShading::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    const bool validRenderDataStore = dataStoreScene && dataStoreCamera && dataStoreLight;
    if (validRenderDataStore) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);
    }

    cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);
    if (validRenderDataStore) {
        UpdatePostProcessConfiguration();
        RenderData(cmdList);
    }
    cmdList.EndRenderPass();
}

void RenderNodeDefaultMaterialDeferredShading::RenderData(IRenderCommandList& cmdList)
{
    const RenderHandle psoHandle = GetPsoHandle();
    if (!RenderHandleUtil::IsValid(psoHandle)) {
        return; // early out
    }
    cmdList.BindPipeline(psoHandle);

    // set 0, update camera, general data, and lighting
    UpdateSet0(cmdList);
    // set 1, update input attachments
    UpdateSet1(cmdList);

    // dynamic state
    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);

    // bind all sets
    {
        const RenderHandle descriptorSets[] = {
            allDescriptorSets_.set0->GetDescriptorSetHandle(),
            allDescriptorSets_.set1->GetDescriptorSetHandle(),
        };
        cmdList.BindDescriptorSets(0u, { descriptorSets, countof(descriptorSets) });
    }

    cmdList.Draw(3u, 1u, 0u, 0u);
}

void RenderNodeDefaultMaterialDeferredShading::UpdateSet0(IRenderCommandList& cmdList)
{
    auto& binder = *allDescriptorSets_.set0;
    uint32_t bindingIndex = 0;
    binder.BindBuffer(bindingIndex++, bufferHandles_.camera, 0u);
    binder.BindBuffer(bindingIndex++, bufferHandles_.generalData, 0u);

    const RenderHandle radianceCubemap = (currentScene_.camera.environment.radianceCubemap)
                                             ? currentScene_.camera.environment.radianceCubemap.GetHandle()
                                             : defaultSkyBoxRadianceCubemap_;
    const RenderHandle colorPrePass = RenderHandleUtil::IsValid(currentScene_.prePassColorTarget)
                                          ? currentScene_.prePassColorTarget
                                          : defaultColorPrePassHandle_;

    binder.BindBuffer(bindingIndex++, bufferHandles_.environment, 0u);
    binder.BindBuffer(bindingIndex++, bufferHandles_.fog, 0u);
    binder.BindBuffer(bindingIndex++, bufferHandles_.light, 0u);
    binder.BindBuffer(bindingIndex++, bufferHandles_.postProcess, 0u);
    binder.BindBuffer(bindingIndex++, bufferHandles_.lightCluster, 0u);
    binder.BindImage(bindingIndex++, colorPrePass, samplerHandles_.linearMip);
    binder.BindImage(bindingIndex++, shadowBuffers_.vsmColorHandle, shadowBuffers_.vsmSamplerHandle.GetHandle());
    binder.BindImage(bindingIndex++, shadowBuffers_.pcfDepthHandle, shadowBuffers_.pcfSamplerHandle.GetHandle());
    binder.BindImage(bindingIndex++, radianceCubemap, samplerHandles_.cubemap);

    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

void RenderNodeDefaultMaterialDeferredShading::UpdateSet1(IRenderCommandList& cmdList)
{
    const auto& renderPassDesc = renderPass_.renderPassDesc;
    const auto& subpassDesc = renderPass_.subpassDesc;
    auto& binder = *allDescriptorSets_.set1;
    for (uint32_t bindingIdx = 0; bindingIdx < subpassDesc.inputAttachmentCount; ++bindingIdx) {
        binder.BindImage(bindingIdx, renderPassDesc.attachmentHandles[subpassDesc.inputAttachmentIndices[bindingIdx]]);
    }

    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

void RenderNodeDefaultMaterialDeferredShading::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
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

    // renderpass needs to be valid (created in init)
    if (hasCustomCamera) {
        // uses camera based clear-setup
        RenderNodeSceneUtil::UpdateRenderPassFromCustomCamera(currentScene_.camera, isNamedCamera, renderPass_);
    } else {
        RenderNodeSceneUtil::UpdateRenderPassFromCamera(currentScene_.camera, renderPass_);
    }
    currentScene_.viewportDesc = RenderNodeSceneUtil::CreateViewportFromCamera(currentScene_.camera);
    currentScene_.scissorDesc = RenderNodeSceneUtil::CreateScissorFromCamera(currentScene_.camera);
    currentScene_.viewportDesc.minDepth = 1.0f;
    currentScene_.viewportDesc.maxDepth = 1.0f;

    const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight.GetLightCounts();
    currentScene_.hasShadow = ((lightCounts.dirShadow > 0) || (lightCounts.spotShadow > 0)) ? true : false;
    currentScene_.shadowTypes = dataStoreLight.GetShadowTypes();
    currentScene_.lightingFlags = dataStoreLight.GetLightingFlags();
}

RenderHandle RenderNodeDefaultMaterialDeferredShading::GetPsoHandle()
{
    uint64_t hash = 0;
    BASE_NS::HashCombine(hash, currentScene_.lightingFlags);
    if ((!RenderHandleUtil::IsValid(allShaderData_.psoHandle)) || (hash != allShaderData_.psoHash)) {
        // only lighting flags can currently change dynamically
        allShaderData_.psoHash = hash;

        constexpr size_t maxFlagCount { 16u };
        uint32_t specializationFlags[maxFlagCount];
        const size_t maxSpecializations = Math::min(maxFlagCount, allShaderData_.defaultSpecilizationConstants.size());
        for (size_t idx = 0; idx < maxSpecializations; ++idx) {
            const auto& ref = allShaderData_.defaultSpecilizationConstants[idx];
            const uint32_t constantId = ref.offset / sizeof(uint32_t);

            if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT) {
                if (ref.id == 0u) {
                    specializationFlags[constantId] = 0;
                } else if (ref.id == 1u) {
                    specializationFlags[constantId] = 0;
                } else if (ref.id == 2u) {
                    specializationFlags[constantId] = currentScene_.lightingFlags;
                } else if (ref.id == 3u) {
                    specializationFlags[constantId] = currentRenderPPConfiguration_.flags.x;
                }
            }
        }

        const DynamicStateFlags dynamicStateFlags =
            DynamicStateFlagBits::CORE_DYNAMIC_STATE_VIEWPORT | DynamicStateFlagBits::CORE_DYNAMIC_STATE_SCISSOR;
        const ShaderSpecializationConstantDataView specialization { allShaderData_.defaultSpecilizationConstants,
            specializationFlags };
        allShaderData_.psoHandle =
            renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(allShaderData_.shaderHandle,
                allShaderData_.stateHandle, allShaderData_.plHandle, {}, specialization, dynamicStateFlags);
    }
    return allShaderData_.psoHandle;
}

void RenderNodeDefaultMaterialDeferredShading::CreateDefaultShaderData()
{
    // shader cannot be cleared
    allShaderData_.defaultSpecilizationConstants = {};
    allShaderData_.stateHandle = {};
    allShaderData_.plHandle = {};
    allShaderData_.psoHandle = {};
    allShaderData_.psoHash = 0;

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    if (shaderMgr.IsComputeShader(allShaderData_.shaderHandle) || shaderMgr.IsShader(allShaderData_.shaderHandle)) {
        allShaderData_.plHandle = shaderMgr.GetPipelineLayoutHandleByShaderHandle(allShaderData_.shaderHandle);
        allShaderData_.stateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(allShaderData_.shaderHandle);
        const ShaderSpecilizationConstantView& sscv =
            shaderMgr.GetReflectionSpecialization(allShaderData_.shaderHandle);
        allShaderData_.defaultSpecilizationConstants.resize(sscv.constants.size());
        for (uint32_t idx = 0; idx < (uint32_t)allShaderData_.defaultSpecilizationConstants.size(); ++idx) {
            allShaderData_.defaultSpecilizationConstants[idx] = sscv.constants[idx];
        }
    } else {
        CORE_LOG_W("RenderNode: %s, invalid shader given", renderNodeContextMgr_->GetName().data());
    }
}

void RenderNodeDefaultMaterialDeferredShading::CreateDescriptorSets()
{
    auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    PipelineLayout pl = renderNodeUtil.CreatePipelineLayout(allShaderData_.shaderHandle);
    {
        // automatic calculation
        const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pl);
        descriptorSetMgr.ResetAndReserve(dc);
    }

    {
        const uint32_t set = 0u;
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, pl);
        allDescriptorSets_.set0 =
            descriptorSetMgr.CreateDescriptorSetBinder(descriptorSetHandle, pl.descriptorSetLayouts[set].bindings);
    }
    {
        const uint32_t set = 1u;
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, pl);
        allDescriptorSets_.set1 =
            descriptorSetMgr.CreateDescriptorSetBinder(descriptorSetHandle, pl.descriptorSetLayouts[set].bindings);
    }
}

void RenderNodeDefaultMaterialDeferredShading::GetSceneUniformBuffers(const string_view uniqueSceneName)
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    bufferHandles_.camera =
        gpuResourceMgr.GetBufferHandle(uniqueSceneName + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);
    bufferHandles_.light =
        gpuResourceMgr.GetBufferHandle(uniqueSceneName + DefaultMaterialLightingConstants::LIGHT_DATA_BUFFER_NAME);
    bufferHandles_.lightCluster = gpuResourceMgr.GetBufferHandle(
        uniqueSceneName + DefaultMaterialLightingConstants::LIGHT_CLUSTER_DATA_BUFFER_NAME);

    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.camera);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.light);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.lightCluster);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all scene buffers not found.", renderNodeContextMgr_->GetName().data());
    }
}

void RenderNodeDefaultMaterialDeferredShading::GetCameraUniformBuffers()
{
    string camName;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        camName = jsonInputs_.customCameraName;
    }
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    bufferHandles_.generalData =
        gpuResourceMgr.GetBufferHandle(DefaultMaterialCameraConstants::CAMERA_GENERAL_BUFFER_PREFIX_NAME + camName);
    bufferHandles_.environment =
        gpuResourceMgr.GetBufferHandle(DefaultMaterialCameraConstants::CAMERA_ENVIRONMENT_BUFFER_PREFIX_NAME + camName);
    bufferHandles_.fog =
        gpuResourceMgr.GetBufferHandle(DefaultMaterialCameraConstants::CAMERA_FOG_BUFFER_PREFIX_NAME + camName);
    bufferHandles_.postProcess = gpuResourceMgr.GetBufferHandle(
        DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_BUFFER_PREFIX_NAME + camName);

    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.generalData);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.environment);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.fog);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.postProcess);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all camera buffers found.", renderNodeContextMgr_->GetName().data());
    }
}

void RenderNodeDefaultMaterialDeferredShading::UpdatePostProcessConfiguration()
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

void RenderNodeDefaultMaterialDeferredShading::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    jsonInputs_.nodeFlags = static_cast<uint32_t>(parserUtil.GetUintValue(jsonVal, "nodeFlags"));
    if (jsonInputs_.nodeFlags == ~0u) {
        jsonInputs_.nodeFlags = 0;
    }

    const string renderSlot = parserUtil.GetStringValue(jsonVal, "renderSlot");
    jsonInputs_.renderSlotId = renderNodeContextMgr_->GetShaderManager().GetRenderSlotId(renderSlot);

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    shader_ = shaderMgr.GetShaderHandle(shaderName);

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultMaterialDeferredShading::Create()
{
    return new RenderNodeDefaultMaterialDeferredShading();
}

void RenderNodeDefaultMaterialDeferredShading::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultMaterialDeferredShading*>(instance);
}
CORE3D_END_NAMESPACE()
