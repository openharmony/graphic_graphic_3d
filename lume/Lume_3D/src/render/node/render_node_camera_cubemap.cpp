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

#include "render_node_camera_cubemap.h"

#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/math/mathf.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace RENDER_NS;

CORE3D_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t GLOBAL_POST_PROCESS_SET { 0u };
constexpr uint32_t LOCAL_POST_PROCESS_SET { 1u };
constexpr uint32_t MAX_MIP_COUNT { 16U };
constexpr string_view RENDER_DATA_STORE_POD_NAME { "RenderDataStorePod" };
constexpr string_view RENDER_DATA_STORE_POST_PROCESS_NAME { "RenderDataStorePostProcess" };

constexpr string_view CORE_DEFAULT_GPU_IMAGE_BLACK { "CORE_DEFAULT_GPU_IMAGE" };
constexpr string_view CORE_DEFAULT_GPU_IMAGE_WHITE { "CORE_DEFAULT_GPU_IMAGE_WHITE" };

#if (CORE3D_VALIDATION_ENABLED == 1)
constexpr string_view POST_PROCESS_BASE_PIPELINE_LAYOUT { "renderpipelinelayouts://post_process_common.shaderpl" };
#endif
constexpr string_view POST_PROCESS_CAMERA_BASE_PIPELINE_LAYOUT { "3dpipelinelayouts://core3d_post_process.shaderpl" };

RenderNodeCameraCubemap::ShadowBuffers GetShadowBufferNodeData(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view sceneName)
{
    RenderNodeCameraCubemap::ShadowBuffers sb;
    sb.vsmSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_VSM_SHADOW_SAMPLER);
    sb.pcfSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_PCF_SHADOW_SAMPLER);

    sb.pcfDepthHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_DEPTH_BUFFER_NAME);
    sb.vsmColorHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_VSM_COLOR_BUFFER_NAME);
    if (!RenderHandleUtil::IsValid(sb.pcfDepthHandle)) {
        sb.pcfDepthHandle = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_WHITE);
    }
    if (!RenderHandleUtil::IsValid(sb.vsmColorHandle)) {
        sb.vsmColorHandle = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_BLACK);
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

RenderNodeCameraCubemap::DefaultImagesAndSamplers GetDefaultImagesAndSamplers(
    const IRenderNodeGpuResourceManager& gpuResourceMgr)
{
    RenderNodeCameraCubemap::DefaultImagesAndSamplers dias;
    dias.cubemapSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    dias.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    dias.nearestHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    dias.linearMipHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");
    dias.colorPrePassHandle = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_BLACK);
    dias.skyBoxRadianceCubemapHandle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP);
    return dias;
}
} // namespace

void RenderNodeCameraCubemap::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    valid_ = true;
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    ubos_.postProcess = CreatePostProcessDataUniformBuffer(gpuResourceMgr, ubos_.postProcess);

    ParseRenderNodeInputs();
    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
    currentScene_ = {};

    shadowBuffers_ = GetShadowBufferNodeData(gpuResourceMgr, stores_.dataStoreNameScene);
    defaultImagesAndSamplers_ = GetDefaultImagesAndSamplers(gpuResourceMgr);

    UpdateImageData();
    ProcessPostProcessConfiguration();
    GetSceneUniformBuffers(stores_.dataStoreNameScene);
    if (!RenderHandleUtil::IsValid(shader_)) {
        shader_ = ppLocalConfig_.shader.GetHandle();
    }

    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(shader_);
    const RenderHandle plHandle = shaderMgr.GetReflectionPipelineLayoutHandle(shader_);
    pipelineLayout_ = shaderMgr.GetPipelineLayout(plHandle);
    if (handleType == RenderHandleType::SHADER_STATE_OBJECT) {
        const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(shader_);
        psoHandle_ = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(
            shader_, graphicsState, pipelineLayout_, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    } else {
        CORE_LOG_E("RN:%s needs a valid shader handle", renderNodeContextMgr_->GetName().data());
    }
    {
        // NOTE: cannot be compared with shaderMgr.GetCompatibilityFlags due to missing pipeline layout handle
        const RenderHandle baseCamPlHandle =
            shaderMgr.GetPipelineLayoutHandle(POST_PROCESS_CAMERA_BASE_PIPELINE_LAYOUT);
        // when validation enabled compare to render post process pipeline layout as well
#if (CORE3D_VALIDATION_ENABLED == 1)
        {
            const RenderHandle basePlHandle = shaderMgr.GetPipelineLayoutHandle(POST_PROCESS_BASE_PIPELINE_LAYOUT);
            const IShaderManager::CompatibilityFlags compatibilityFlags =
                shaderMgr.GetCompatibilityFlags(baseCamPlHandle, basePlHandle);
            if ((compatibilityFlags & IShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT) == 0) {
                CORE_LOG_E("RN:%s uncompatible render vs 3D pipeline layout (%s)",
                    renderNodeContextMgr_->GetName().data(), POST_PROCESS_CAMERA_BASE_PIPELINE_LAYOUT.data());
            }
        }
#endif
        const IShaderManager::CompatibilityFlags compatibilityFlags =
            shaderMgr.GetCompatibilityFlags(baseCamPlHandle, plHandle);
        if ((compatibilityFlags & IShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT) == 0) {
            CORE_LOG_E("RN:%s uncompatible pipeline layout to %s", renderNodeContextMgr_->GetName().data(),
                POST_PROCESS_CAMERA_BASE_PIPELINE_LAYOUT.data());
        }
    }
    InitCreateBinders();

    RegisterOutputs();
}

void RenderNodeCameraCubemap::PreExecuteFrame()
{
    {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto* dataStoreScene = static_cast<IRenderDataStoreDefaultScene*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
        const auto* dataStoreCamera = static_cast<IRenderDataStoreDefaultCamera*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
        const auto* dataStoreLight = static_cast<IRenderDataStoreDefaultLight*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));
        const bool validRenderDataStore = dataStoreScene && dataStoreCamera && dataStoreLight;
        if (validRenderDataStore) {
            UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);
        }
    }

    UpdateImageData();
    ProcessPostProcessConfiguration();
    RegisterOutputs();
}

void RenderNodeCameraCubemap::ExecuteFrame(IRenderCommandList& cmdList)
{
    if ((!ppLocalConfig_.variables.enabled)) {
        return;
    }

    if (ppLocalConfig_.variables.enabled && valid_ && RenderHandleUtil::IsValid(builtInVariables_.output)) {
        ExecuteSinglePostProcess(cmdList);
    }
}

namespace {
constexpr ImageResourceBarrier SRC_UNDEFINED { 0, CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT, CORE_IMAGE_LAYOUT_UNDEFINED };
constexpr ImageResourceBarrier COL_ATTACHMENT { CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
constexpr ImageResourceBarrier SHDR_READ { CORE_ACCESS_SHADER_READ_BIT, CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
} // namespace

void RenderNodeCameraCubemap::ExecuteSinglePostProcess(IRenderCommandList& cmdList)
{
    Math::UVec2 currSize = builtInVariables_.outputSize;

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1U;
    renderPass.renderPassDesc.attachmentHandles[0U] = builtInVariables_.output;
    renderPass.renderPassDesc.attachments[0U].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0U].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachments[0U].layer = 0U;
    renderPass.renderPassDesc.renderArea = { 0U, 0U, currSize.x, currSize.y };
    renderPass.renderPassDesc.subpassCount = 1U;
    renderPass.subpassDesc.viewMask = 0x3f;
    renderPass.subpassDesc.colorAttachmentCount = 1U;
    renderPass.subpassDesc.colorAttachmentIndices[0U] = 0U;

    if ((renderPass.renderPassDesc.attachmentCount == 0) ||
        !RenderHandleUtil::IsValid(renderPass.renderPassDesc.attachmentHandles[0])) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_ONCE_W("rp_cm_missing_" + renderNodeContextMgr_->GetName(), "RN: %s, invalid attachment",
            renderNodeContextMgr_->GetName().data());
#endif
        return;
    }

    UpdateGlobalPostProcessUbo();
    {
        // handle automatic set 0 bindings
        UpdateSet0(cmdList);
        const auto bindings = globalSet0_->GetDescriptorSetLayoutBindingResources();
        cmdList.UpdateDescriptorSet(globalSet0_->GetDescriptorSetHandle(), bindings);
    }
    for (uint32_t mipIdx = 0; mipIdx < builtInVariables_.outputMipCount; ++mipIdx) {
        // handle automatic set 1 bindings
        UpdateSet1(cmdList, mipIdx);
        const auto bindings = localSets_[mipIdx]->GetDescriptorSetLayoutBindingResources();
        cmdList.UpdateDescriptorSet(localSets_[mipIdx]->GetDescriptorSetHandle(), bindings);
    }

    cmdList.BeginDisableAutomaticBarrierPoints();

    // change all the layouts accordingly
    {
        ImageSubresourceRange imageSubresourceRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
        cmdList.CustomImageBarrier(builtInVariables_.output, SRC_UNDEFINED, COL_ATTACHMENT, imageSubresourceRange);
        cmdList.AddCustomBarrierPoint();
    }

    for (uint32_t mipIdx = 0; mipIdx < builtInVariables_.outputMipCount; ++mipIdx) {
        renderPass.renderPassDesc.attachments[0U].mipLevel = mipIdx;
        if (mipIdx != 0U) {
            currSize.x = Math::max(1U, currSize.x / 2U);
            currSize.y = Math::max(1U, currSize.y / 2U);
        }
        renderPass.renderPassDesc.renderArea = { 0U, 0U, currSize.x, currSize.y };

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

        cmdList.BindPipeline(psoHandle_);
        // bind all sets
        {
            RenderHandle sets[2U] { globalSet0_->GetDescriptorSetHandle(),
                localSets_[mipIdx]->GetDescriptorSetHandle() };
            cmdList.BindDescriptorSets(0u, sets);
        }

        // dynamic state
        const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);
        // push constants
        if (pipelineLayout_.pushConstant.byteSize > 0) {
            const float fWidth = static_cast<float>(currSize.x);
            const float fHeight = static_cast<float>(currSize.y);
            const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                ppLocalConfig_.variables.factor };
            cmdList.PushConstant(pipelineLayout_.pushConstant, arrayviewU8(pc).data());
        }

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }

    // change all the layouts accordingly
    {
        ImageSubresourceRange imageSubresourceRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
        cmdList.CustomImageBarrier(builtInVariables_.output, COL_ATTACHMENT, SHDR_READ, imageSubresourceRange);
        cmdList.AddCustomBarrierPoint();
    }

    cmdList.EndDisableAutomaticBarrierPoints();
}

void RenderNodeCameraCubemap::RegisterOutputs()
{
    const RenderHandle output = builtInVariables_.output;
    IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
    RenderHandle registerOutput;
    if (ppLocalConfig_.variables.enabled) {
        if (RenderHandleUtil::IsValid(output)) {
            registerOutput = output;
        }
    }
    if (!RenderHandleUtil::IsValid(registerOutput)) {
        registerOutput = defaultImagesAndSamplers_.skyBoxRadianceCubemapHandle;
    }
    shrMgr.RegisterRenderNodeOutput("output", registerOutput);
}

void RenderNodeCameraCubemap::UpdateSet0(IRenderCommandList& cmdList)
{
    const RenderHandle radianceCubemap = (currentScene_.camera.environment.radianceCubemap)
                                             ? currentScene_.camera.environment.radianceCubemap.GetHandle()
                                             : defaultImagesAndSamplers_.skyBoxRadianceCubemapHandle;
    const RenderHandle colorPrePass = RenderHandleUtil::IsValid(currentScene_.prePassColorTarget)
                                          ? currentScene_.prePassColorTarget
                                          : defaultImagesAndSamplers_.colorPrePassHandle;

    auto& binder = *globalSet0_;
    uint32_t bindingIndex = 0;
    // global
    binder.BindBuffer(bindingIndex++, BindableBuffer { ubos_.postProcess.GetHandle() });
    binder.BindBuffer(
        bindingIndex++, BindableBuffer { ubos_.postProcess.GetHandle(), sizeof(GlobalPostProcessStruct) });

    // scene and camera global
    binder.BindBuffer(bindingIndex++, { sceneBuffers_.camera });
    binder.BindBuffer(bindingIndex++, { cameraBuffers_.generalData });

    binder.BindBuffer(bindingIndex++, { cameraBuffers_.environment });
    binder.BindBuffer(bindingIndex++, { cameraBuffers_.fog });
    binder.BindBuffer(bindingIndex++, { cameraBuffers_.light });
    binder.BindBuffer(bindingIndex++, { cameraBuffers_.postProcess });
    binder.BindBuffer(bindingIndex++, { cameraBuffers_.lightCluster });

    // scene and camera global images
    BindableImage bi;
    bi.handle = colorPrePass;
    bi.samplerHandle = defaultImagesAndSamplers_.linearMipHandle;
    binder.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
    bi.handle = shadowBuffers_.vsmColorHandle;
    bi.samplerHandle = shadowBuffers_.vsmSamplerHandle;
    binder.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
    bi.handle = shadowBuffers_.pcfDepthHandle;
    bi.samplerHandle = shadowBuffers_.pcfSamplerHandle;
    binder.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
    bi.handle = radianceCubemap;
    bi.samplerHandle = defaultImagesAndSamplers_.cubemapSamplerHandle;
    binder.BindImage(bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);

    // NOTE: UpdateDescriptorSets is done when called
}

void RenderNodeCameraCubemap::UpdateSet1(IRenderCommandList& cmdList, const uint32_t idx)
{
    // bind all radiance cubemaps
    auto& binder = *localSets_[idx];
    // local
    const auto res = binder.GetDescriptorSetLayoutBindingResources();
    const auto bindings = res.bindings;
    const uint32_t maxCount =
        Math::min(DefaultMaterialCameraConstants::MAX_ENVIRONMENT_COUNT, static_cast<uint32_t>(bindings.size()));
    BindableImage bi;
    for (uint32_t resIdx = 0U; resIdx < maxCount; ++resIdx) {
        // filled with valid handles
        bi.handle = currentScene_.cameraEnvRadianceHandles[resIdx];
        bi.samplerHandle = defaultImagesAndSamplers_.cubemapSamplerHandle;
        binder.BindImage(resIdx, bi, {});
    }
}

void RenderNodeCameraCubemap::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    const auto scene = dataStoreScene.GetScene();
    uint32_t cameraIdx = scene.cameraIndex;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraId);
        currentScene_.cameraName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraName);
        currentScene_.cameraName = jsonInputs_.customCameraName;
    }

    if (const auto cameras = dataStoreCamera.GetCameras(); cameraIdx < (uint32_t)cameras.size()) {
        // store current frame camera
        currentScene_.camera = cameras[cameraIdx];
    }
    // fetch all the radiance cubemaps
    for (uint32_t idx = 0; idx < DefaultMaterialCameraConstants::MAX_ENVIRONMENT_COUNT; ++idx) {
        if (idx < currentScene_.camera.environmentCount) {
            const auto& env = dataStoreCamera.GetEnvironment(currentScene_.camera.environmentIds[idx]);
            currentScene_.cameraEnvRadianceHandles[idx] = (env.radianceCubemap)
                                                              ? env.radianceCubemap.GetHandle()
                                                              : defaultImagesAndSamplers_.skyBoxRadianceCubemapHandle;

        } else {
            currentScene_.cameraEnvRadianceHandles[idx] = defaultImagesAndSamplers_.skyBoxRadianceCubemapHandle;
        }
    }

    const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight.GetLightCounts();
    currentScene_.hasShadow = ((lightCounts.dirShadow > 0) || (lightCounts.spotShadow > 0)) ? true : false;
    currentScene_.shadowTypes = dataStoreLight.GetShadowTypes();
    currentScene_.lightingFlags = dataStoreLight.GetLightingFlags();
}

void RenderNodeCameraCubemap::ProcessPostProcessConfiguration()
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
    } else if (jsonInputs_.ppName.empty()) {
        // if trying to just use shader without post processing we enable running by default
        ppLocalConfig_.variables.enabled = true;
    }
}

void RenderNodeCameraCubemap::UpdateGlobalPostProcessUbo()
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

void RenderNodeCameraCubemap::GetSceneUniformBuffers(const string_view us)
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

void RenderNodeCameraCubemap::InitCreateBinders()
{
    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    {
        DescriptorCounts dc0 = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
        const DescriptorCounts dc1 =
            renderNodeUtil.GetDescriptorCounts(pipelineLayout_.descriptorSetLayouts[LOCAL_POST_PROCESS_SET].bindings);
        for (uint32_t layerIdx = 0; layerIdx < MAX_MIP_COUNT; ++layerIdx) {
            dc0.counts.insert(dc0.counts.end(), dc1.counts.begin(), dc1.counts.end());
        }
        descriptorSetMgr.ResetAndReserve(dc0);
    }

    localSets_.clear();
    localSets_.resize(MAX_MIP_COUNT);
    {
        const auto& bindings = pipelineLayout_.descriptorSetLayouts[GLOBAL_POST_PROCESS_SET].bindings;
        const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
        globalSet0_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
    }
    {
        const auto& bindings = pipelineLayout_.descriptorSetLayouts[LOCAL_POST_PROCESS_SET].bindings;
        for (uint32_t idx = 0; idx < MAX_MIP_COUNT; ++idx) {
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            localSets_[idx] = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
    }

    if ((!RenderHandleUtil::IsValid(shader_)) || (!RenderHandleUtil::IsValid(psoHandle_))) {
        valid_ = false;
    }
}

void RenderNodeCameraCubemap::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");

    jsonInputs_.ppName = parserUtil.GetStringValue(jsonVal, "postProcess");

#if (CORE3D_VALIDATION_ENABLED == 1)
    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        CORE_LOG_W("CORE3D_VALIDATION: RN %s renderDataStore::dataStoreName missing.",
            renderNodeContextMgr_->GetName().data());
    }
    if (jsonInputs_.ppName.empty()) {
        CORE_LOG_W("CORE3D_VALIDATION: RN %s postProcess name missing.", renderNodeContextMgr_->GetName().data());
    }
#endif

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    if (!shaderName.empty()) {
        shader_ = shaderMgr.GetShaderHandle(shaderName);
    }
}

void RenderNodeCameraCubemap::UpdateImageData()
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBuffer)) {
        builtInVariables_.defBuffer = ubos_.postProcess.GetHandle();
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBlackImage)) {
        builtInVariables_.defBlackImage = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_BLACK);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defSampler)) {
        builtInVariables_.defSampler = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    }
    if (jsonInputs_.outputIdx < inputResources_.customOutputImages.size()) {
        builtInVariables_.output = inputResources_.customOutputImages[jsonInputs_.outputIdx].handle;
    }
    builtInVariables_.outputSize = Math::UVec2(1U, 1U);
    builtInVariables_.outputMipCount = 1U;
    if (!RenderHandleUtil::IsValid(builtInVariables_.output)) {
        builtInVariables_.output = gpuResourceMgr.GetImageHandle(
            stores_.dataStoreNameScene + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME +
            "RADIANCE_CUBEMAP_" + currentScene_.camera.name);
    }
    if (RenderHandleUtil::IsValid(builtInVariables_.output)) {
        const auto& desc = gpuResourceMgr.GetImageDescriptor(builtInVariables_.output);
        builtInVariables_.outputSize = Math::UVec2(desc.width, desc.height);
        builtInVariables_.outputMipCount = desc.mipCount;
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeCameraCubemap::Create()
{
    return new RenderNodeCameraCubemap();
}

void RenderNodeCameraCubemap::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCameraCubemap*>(instance);
}
CORE3D_END_NAMESPACE()
