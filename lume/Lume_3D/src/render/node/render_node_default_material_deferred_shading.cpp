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
constexpr string_view RENDER_DATA_STORE_POD_NAME { "RenderDataStorePod" };
constexpr string_view RENDER_DATA_STORE_POST_PROCESS_NAME { "RenderDataStorePostProcess" };
constexpr uint32_t BUILT_IN_SETS_COUNT { 2u };
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

RenderNodeDefaultMaterialDeferredShading::ShadowBuffers GetShadowBufferNodeData(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view sceneName)
{
    RenderNodeDefaultMaterialDeferredShading::ShadowBuffers sb;
    sb.vsmSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_VSM_SHADOW_SAMPLER);
    sb.pcfSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_PCF_SHADOW_SAMPLER);

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

RenderHandleReference CreatePostProcessDataUniformBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    CORE_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    CORE_STATIC_ASSERT(
        sizeof(LocalPostProcessStruct) == PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE);
    return gpuResourceMgr.Create(
        handle, GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                    sizeof(GlobalPostProcessStruct) + sizeof(LocalPostProcessStruct) });
}
} // namespace

void RenderNodeDefaultMaterialDeferredShading::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    allShaderData_ = {};
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    allDescriptorSets_ = {};
    currentScene_ = {};
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    ubos_.postProcess = CreatePostProcessDataUniformBuffer(gpuResourceMgr, ubos_.postProcess);

    if ((jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) &&
        jsonInputs_.renderDataStore.dataStoreName.empty()) {
        CORE_LOG_V("%s: render data store post process configuration not set in render node graph",
            renderNodeContextMgr_->GetName().data());
    }

    GetSceneUniformBuffers(stores_.dataStoreNameScene);
    shadowBuffers_ = GetShadowBufferNodeData(gpuResourceMgr, stores_.dataStoreNameScene);

    renderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);

    samplerHandles_.cubemap =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    samplerHandles_.linear = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    samplerHandles_.nearest = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    samplerHandles_.linearMip = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");

    defaultColorPrePassHandle_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
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
        UpdatePostProcessConfiguration();
        UpdateGlobalPostProcessUbo();
    }
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
        renderNodeUtil.BindResourcesToBinder(inputResources_, *allDescriptorSets_.pipelineDescriptorSetBinder);
    }

    cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);
    if (validRenderDataStore) {
        RenderData(cmdList);
    }
    cmdList.EndRenderPass();
}

void RenderNodeDefaultMaterialDeferredShading::RenderData(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }
    const RenderHandle psoHandle = GetPsoHandle();
    if (!RenderHandleUtil::IsValid(psoHandle)) {
        return; // early out
    }
    cmdList.BindPipeline(psoHandle);

    // set 0, update camera, general data, and lighting
    // set 1, update input attachments
    UpdateSet01(cmdList);
    // set 2-3, update user bindings
    UpdateUserSets(cmdList);

    // dynamic state
    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);

    // bind all sets
    {
        RenderHandle descriptorSets[4U] = {
            allDescriptorSets_.set0->GetDescriptorSetHandle(),
            allDescriptorSets_.set1->GetDescriptorSetHandle(),
            {},
            {},
        };
        uint32_t setCount = BUILT_IN_SETS_COUNT;
        if (allDescriptorSets_.hasUserSet2 && allDescriptorSets_.pipelineDescriptorSetBinder) {
            descriptorSets[setCount] = allDescriptorSets_.pipelineDescriptorSetBinder->GetDescriptorSetHandle(setCount);
            setCount++;
        }
        if (allDescriptorSets_.hasUserSet3 && allDescriptorSets_.pipelineDescriptorSetBinder) {
            descriptorSets[setCount] = allDescriptorSets_.pipelineDescriptorSetBinder->GetDescriptorSetHandle(setCount);
            setCount++;
        }
        cmdList.BindDescriptorSets(0u, { descriptorSets, setCount });
    }

    // push constants
    if (pipelineLayout_.pushConstant.byteSize > 0) {
        const float fWidth = static_cast<float>(renderPass_.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass_.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
            ppLocalConfig_.variables.factor };
        cmdList.PushConstant(pipelineLayout_.pushConstant, arrayviewU8(pc).data());
    }

    cmdList.Draw(3u, 1u, 0u, 0u);
}

void RenderNodeDefaultMaterialDeferredShading::UpdateSet01(IRenderCommandList& cmdList)
{
    auto& binder0 = *allDescriptorSets_.set0;
    auto& binder1 = *allDescriptorSets_.set1;
    {
        uint32_t bindingIndex = 0;
        // global
        binder0.BindBuffer(bindingIndex++, BindableBuffer { ubos_.postProcess.GetHandle() });
        binder0.BindBuffer(
            bindingIndex++, BindableBuffer { ubos_.postProcess.GetHandle(), sizeof(GlobalPostProcessStruct) });

        // scene and camera global
        binder0.BindBuffer(bindingIndex++, sceneBuffers_.camera, 0u);
        binder0.BindBuffer(bindingIndex++, cameraBuffers_.generalData, 0u);

        const RenderHandle radianceCubemap = currentScene_.cameraEnvRadianceHandle;
        const RenderHandle colorPrePass = RenderHandleUtil::IsValid(currentScene_.prePassColorTarget)
                                              ? currentScene_.prePassColorTarget
                                              : defaultColorPrePassHandle_;

        binder0.BindBuffer(bindingIndex++, cameraBuffers_.environment, 0u);
        binder0.BindBuffer(bindingIndex++, cameraBuffers_.fog, 0u);
        binder0.BindBuffer(bindingIndex++, cameraBuffers_.light, 0u);
        binder0.BindBuffer(bindingIndex++, cameraBuffers_.postProcess, 0u);
        binder0.BindBuffer(bindingIndex++, cameraBuffers_.lightCluster, 0u);

        // use immutable samplers for all set 0 samplers
        BindableImage bi;
        bi.handle = colorPrePass;
        bi.samplerHandle = samplerHandles_.linearMip;
        binder0.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
        bi.handle = shadowBuffers_.vsmColorHandle;
        bi.samplerHandle = shadowBuffers_.vsmSamplerHandle;
        binder0.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
        bi.handle = shadowBuffers_.pcfDepthHandle;
        bi.samplerHandle = shadowBuffers_.pcfSamplerHandle;
        binder0.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
        bi.handle = radianceCubemap;
        bi.samplerHandle = samplerHandles_.cubemap;
        binder0.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
    }
    {
        const auto& renderPassDesc = renderPass_.renderPassDesc;
        const auto& subpassDesc = renderPass_.subpassDesc;
        for (uint32_t bindingIdx = 0; bindingIdx < subpassDesc.inputAttachmentCount; ++bindingIdx) {
            binder1.BindImage(
                bindingIdx, renderPassDesc.attachmentHandles[subpassDesc.inputAttachmentIndices[bindingIdx]]);
        }
    }

    const RenderHandle handles[] { binder0.GetDescriptorSetHandle(), binder1.GetDescriptorSetHandle() };
    const DescriptorSetLayoutBindingResources resources[] { binder0.GetDescriptorSetLayoutBindingResources(),
        binder1.GetDescriptorSetLayoutBindingResources() };
    cmdList.UpdateDescriptorSets(handles, resources);
}

void RenderNodeDefaultMaterialDeferredShading::UpdateUserSets(IRenderCommandList& cmdList)
{
    if (allDescriptorSets_.hasUserSet2 && allDescriptorSets_.pipelineDescriptorSetBinder) {
        uint32_t set = BUILT_IN_SETS_COUNT;
        const auto descHandle = allDescriptorSets_.pipelineDescriptorSetBinder->GetDescriptorSetHandle(set);
        const auto bindings =
            allDescriptorSets_.pipelineDescriptorSetBinder->GetDescriptorSetLayoutBindingResources(set);
        if (RenderHandleUtil::IsValid(descHandle) && (!bindings.bindings.empty())) {
            cmdList.UpdateDescriptorSet(descHandle, bindings);
        }
    }
    if (allDescriptorSets_.hasUserSet3 && allDescriptorSets_.pipelineDescriptorSetBinder) {
        uint32_t set = BUILT_IN_SETS_COUNT + 1u;
        const auto descHandle = allDescriptorSets_.pipelineDescriptorSetBinder->GetDescriptorSetHandle(set);
        const auto bindings =
            allDescriptorSets_.pipelineDescriptorSetBinder->GetDescriptorSetLayoutBindingResources(set);
        if (RenderHandleUtil::IsValid(descHandle) && (!bindings.bindings.empty())) {
            cmdList.UpdateDescriptorSet(descHandle, bindings);
        }
    }
}

void RenderNodeDefaultMaterialDeferredShading::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
        renderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    }

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
    currentScene_.cameraShaderFlags = currentScene_.camera.shaderFlags;
    // remove fog explicitly if render node graph input and/or default render slot usage states so
    if (jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DISABLE_FOG_BIT) {
        currentScene_.cameraShaderFlags &= (~RenderCamera::ShaderFlagBits::CAMERA_SHADER_FOG_BIT);
    }
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
                    specializationFlags[constantId] = currentRenderPPConfiguration_.flags.x;
                } else if (ref.id == 1u) {
                    specializationFlags[constantId] = 0;
                } else if (ref.id == 2u) {
                    specializationFlags[constantId] = currentScene_.lightingFlags;
                } else if (ref.id == 4u) {
                    specializationFlags[constantId] = currentScene_.cameraShaderFlags;
                }
            }
        }

        const ShaderSpecializationConstantDataView specialization { allShaderData_.defaultSpecilizationConstants,
            specializationFlags };
        allShaderData_.psoHandle = renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(
            allShaderData_.shaderHandle, allShaderData_.stateHandle, allShaderData_.plHandle, {}, specialization,
            { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
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
        const ShaderSpecializationConstantView& sscv =
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

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    const RenderHandle plDefHandle =
        shaderMgr.GetPipelineLayoutHandle("3dpipelinelayouts://core3d_dm_fullscreen_deferred_shading.shaderpl");
    const PipelineLayout plDef = shaderMgr.GetPipelineLayout(plDefHandle);
    const RenderHandle plShaderHandle = shaderMgr.GetPipelineLayoutHandleByShaderHandle(allShaderData_.shaderHandle);
    pipelineLayout_ = shaderMgr.GetPipelineLayout(plShaderHandle);

    const IShaderManager::CompatibilityFlags compatibilityFlags =
        shaderMgr.GetCompatibilityFlags(plDefHandle, plShaderHandle);
    if (compatibilityFlags != 0) {
        valid_ = true;
    } else {
        CORE_LOG_W("RN: %s incompatible pipeline layout for given shader", renderNodeContextMgr_->GetName().data());
    }

    // currently we allocate just in case based on both layouts to make sure that we have enough descriptors
    // we create pipeline descriptor set binder for convenience for user sets
    DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
    // double the sets for pipeline descriptor binder usage
    for (auto& dcRef : dc.counts) {
        dcRef.count *= 2u;
    }
    // add built-in set from the first set (if user set was missing something)
    dc.counts.reserve(dc.counts.size() + plDef.descriptorSetLayouts[0U].bindings.size());
    for (const auto& bindingRef : plDef.descriptorSetLayouts[0U].bindings) {
        dc.counts.push_back(DescriptorCounts::TypedCount { bindingRef.descriptorType, bindingRef.descriptorCount });
    }
    descriptorSetMgr.ResetAndReserve(dc);

    {
        // set 0 descriptors are fixed at the moment
        constexpr uint32_t set { 0u };
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, plDef);
        allDescriptorSets_.set0 =
            descriptorSetMgr.CreateDescriptorSetBinder(descriptorSetHandle, plDef.descriptorSetLayouts[set].bindings);
    }
    {
        // input attachment count is allowed to change, so we need to create the descriptor set based on shader
        constexpr uint32_t set { 1u };
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, pipelineLayout_);
        allDescriptorSets_.set1 = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetHandle, pipelineLayout_.descriptorSetLayouts[set].bindings);
    }

    // pipeline descriptor set binder for user sets
    allDescriptorSets_.pipelineDescriptorSetBinder = renderNodeUtil.CreatePipelineDescriptorSetBinder(pipelineLayout_);
    if (allDescriptorSets_.pipelineDescriptorSetBinder) {
        renderNodeUtil.BindResourcesToBinder(inputResources_, *allDescriptorSets_.pipelineDescriptorSetBinder);
#if (CORE3D_VALIDATION_ENABLED == 1)
        auto CheckBindingValidity = [](const auto builtInBindings, const auto& resources) {
            for (const auto& res : resources) {
                if (res.set < builtInBindings) {
                    return false;
                }
            }
            return true;
        };
        bool valid = CheckBindingValidity(BUILT_IN_SETS_COUNT, inputResources_.buffers);
        valid = valid && CheckBindingValidity(BUILT_IN_SETS_COUNT, inputResources_.images);
        valid = valid && CheckBindingValidity(BUILT_IN_SETS_COUNT, inputResources_.samplers);
        if (!valid) {
            CORE_LOG_W("RN: %s does not support user bindings for sets <= %u", renderNodeContextMgr_->GetName().data(),
                BUILT_IN_SETS_COUNT);
        }
        allDescriptorSets_.hasUserSet2 = !(
            allDescriptorSets_.pipelineDescriptorSetBinder->GetDescriptorSetLayoutBindingResources(BUILT_IN_SETS_COUNT)
                .bindings.empty());
        allDescriptorSets_.hasUserSet3 = !(allDescriptorSets_.pipelineDescriptorSetBinder
                                               ->GetDescriptorSetLayoutBindingResources(BUILT_IN_SETS_COUNT + 1u)
                                               .bindings.empty());
#endif
    }
}

void RenderNodeDefaultMaterialDeferredShading::GetSceneUniformBuffers(const string_view us)
{
    sceneBuffers_ = RenderNodeSceneUtil::GetSceneBufferHandles(*renderNodeContextMgr_, stores_.dataStoreNameScene);

    string camName;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        camName = jsonInputs_.customCameraName;
    }
    cameraBuffers_ =
        RenderNodeSceneUtil::GetSceneCameraBufferHandles(*renderNodeContextMgr_, stores_.dataStoreNameScene, camName);
}

void RenderNodeDefaultMaterialDeferredShading::UpdatePostProcessConfiguration()
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName); ds) {
            if (jsonInputs_.renderDataStore.typeName == RENDER_DATA_STORE_POST_PROCESS_NAME) {
                auto* const dataStore = static_cast<IRenderDataStorePostProcess const*>(ds);
                ppLocalConfig_ = dataStore->Get(jsonInputs_.renderDataStore.configurationName, jsonInputs_.ppName);
            }
        }
        if (const IRenderDataStorePod* ds =
                static_cast<const IRenderDataStorePod*>(dsMgr.GetRenderDataStore(RENDER_DATA_STORE_POD_NAME));
            ds) {
            auto const dataView = ds->Get(jsonInputs_.renderDataStore.configurationName);
            if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                ppGlobalConfig_ = *((const PostProcessConfiguration*)dataView.data());
            }
        }
    }
}

void RenderNodeDefaultMaterialDeferredShading::UpdateGlobalPostProcessUbo()
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const RenderPostProcessConfiguration rppc =
        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(ppGlobalConfig_);
    CORE_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    CORE_STATIC_ASSERT(sizeof(LocalPostProcessStruct) ==
                       sizeof(IRenderDataStorePostProcess::PostProcess::Variables::customPropertyData));
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.postProcess.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(GlobalPostProcessStruct) + sizeof(LocalPostProcessStruct);
        // global data
        CloneData(data, size_t(dataEnd - data), &rppc, sizeof(GlobalPostProcessStruct));
        // local data
        data += sizeof(GlobalPostProcessStruct);
        CloneData(
            data, size_t(dataEnd - data), ppLocalConfig_.variables.customPropertyData, sizeof(LocalPostProcessStruct));
        gpuResourceMgr.UnmapBuffer(ubos_.postProcess.GetHandle());
    }
}

void RenderNodeDefaultMaterialDeferredShading::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    jsonInputs_.ppName = parserUtil.GetStringValue(jsonVal, "postProcess");

    jsonInputs_.nodeFlags = static_cast<uint32_t>(parserUtil.GetUintValue(jsonVal, "nodeFlags"));
    if (jsonInputs_.nodeFlags == ~0u) {
        jsonInputs_.nodeFlags = 0;
    }

    const string renderSlot = parserUtil.GetStringValue(jsonVal, "renderSlot");
    jsonInputs_.renderSlotId = renderNodeContextMgr_->GetShaderManager().GetRenderSlotId(renderSlot);

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    shader_ = shaderMgr.GetShaderHandle(shaderName);

    EvaluateFogBits();

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    jsonInputs_.hasChangeableRenderPassHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.renderPass);
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);
}

void RenderNodeDefaultMaterialDeferredShading::EvaluateFogBits()
{
    // if no explicit bits set we check default render slot usages
    if ((jsonInputs_.nodeFlags & (RENDER_SCENE_ENABLE_FOG_BIT | RENDER_SCENE_DISABLE_FOG_BIT)) == 0) {
        jsonInputs_.nodeFlags |= RenderSceneFlagBits::RENDER_SCENE_ENABLE_FOG_BIT;
    }
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
