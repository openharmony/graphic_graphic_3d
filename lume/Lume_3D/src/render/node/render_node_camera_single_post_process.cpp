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

#include "render_node_camera_single_post_process.h"

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
constexpr string_view RENDER_DATA_STORE_POD_NAME { "RenderDataStorePod" };
constexpr string_view RENDER_DATA_STORE_POST_PROCESS_NAME { "RenderDataStorePostProcess" };

constexpr string_view CORE_DEFAULT_GPU_IMAGE_BLACK { "CORE_DEFAULT_GPU_IMAGE" };
constexpr string_view CORE_DEFAULT_GPU_IMAGE_WHITE { "CORE_DEFAULT_GPU_IMAGE_WHITE" };

constexpr string_view INPUT = "input";
constexpr string_view OUTPUT = "output";
#if (CORE3D_VALIDATION_ENABLED == 1)
constexpr string_view POST_PROCESS_BASE_PIPELINE_LAYOUT { "renderpipelinelayouts://post_process_common.shaderpl" };
#endif
constexpr string_view POST_PROCESS_CAMERA_BASE_PIPELINE_LAYOUT { "3dpipelinelayouts://core3d_post_process.shaderpl" };

#if (CORE3D_VALIDATION_ENABLED == 1)
uint32_t GetPostProcessFlag(const string_view ppName)
{
    // get built-in flags
    if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_FXAA]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT;
    } else if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_TAA]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT;
    } else if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_BLOOM]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT;
    } else if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_BLUR]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT;
    } else {
        return 0;
    }
}
#endif

RenderNodeCameraSinglePostProcess::ShadowBuffers GetShadowBufferNodeData(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view sceneName)
{
    RenderNodeCameraSinglePostProcess::ShadowBuffers sb;
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

bool NeedsAutoBindingSet0(const RenderNodeHandles::InputResources& inputRes)
{
    uint32_t set0Bindings = 0;
    for (const auto& res : inputRes.buffers) {
        if (res.set == GLOBAL_POST_PROCESS_SET) {
            set0Bindings++;
        }
    }
    return (set0Bindings == 0);
}

RenderNodeCameraSinglePostProcess::DefaultImagesAndSamplers GetDefaultImagesAndSamplers(
    const IRenderNodeGpuResourceManager& gpuResourceMgr)
{
    RenderNodeCameraSinglePostProcess::DefaultImagesAndSamplers dias;
    dias.cubemapHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    dias.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    dias.nearestHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    dias.linearMipHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");
    dias.colorPrePassHandle = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_BLACK);
    return dias;
}

struct DispatchResources {
    RenderHandle buffer {};
    RenderHandle image {};
};

DispatchResources GetDispatchResources(const RenderNodeHandles::InputResources& ir)
{
    DispatchResources dr;
    if (!ir.customInputBuffers.empty()) {
        dr.buffer = ir.customInputBuffers[0].handle;
    }
    if (!ir.customInputImages.empty()) {
        dr.image = ir.customInputImages[0].handle;
    }
    return dr;
}
} // namespace

void RenderNodeCameraSinglePostProcess::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
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
        graphics_ = true;
        const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(shader_);
        psoHandle_ = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(
            shader_, graphicsState, pipelineLayout_, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    } else if (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        graphics_ = false;
        psoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(shader_, pipelineLayout_, {});
        threadGroupSize_ = shaderMgr.GetReflectionThreadGroupSize(shader_);
        if (dispatchResources_.customInputBuffers.empty() && dispatchResources_.customInputImages.empty()) {
            valid_ = false;
            CORE_LOG_W("RenderNodeCameraSinglePostProcess: dispatchResources (GPU buffer or GPU image) needed");
        }
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

#if (CORE3D_VALIDATION_ENABLED == 1)
    // 3d does not operate on render built-in post processes
    const uint32_t postProcessFlag = GetPostProcessFlag(jsonInputs_.ppName);
    if (postProcessFlag != 0) {
        valid_ = false;
        CORE_LOG_W("RN:%s does not execute render built-in post processes.", renderNodeContextMgr_->GetName().data());
    }
#endif
    InitCreateBinders();

    renderCopy_.Init(renderNodeContextMgr, {});

    RegisterOutputs();
}

void RenderNodeCameraSinglePostProcess::PreExecuteFrame()
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

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    UpdateImageData();
    ProcessPostProcessConfiguration();
    RegisterOutputs();
}

void RenderNodeCameraSinglePostProcess::ExecuteFrame(IRenderCommandList& cmdList)
{
    if ((!ppLocalConfig_.variables.enabled) &&
        (jsonInputs_.defaultOutputImage != DefaultOutputImage::INPUT_OUTPUT_COPY)) {
        return;
    }

    if (ppLocalConfig_.variables.enabled && valid_) {
        ExecuteSinglePostProcess(cmdList);
    } else if (jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT_OUTPUT_COPY) {
        renderCopy_.Execute(*renderNodeContextMgr_, cmdList);
    }
}

void RenderNodeCameraSinglePostProcess::ExecuteSinglePostProcess(IRenderCommandList& cmdList)
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    }
    if (jsonInputs_.hasChangeableResourceHandles) {
        // input resources updated in preExecuteFrame
        renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
    }
    if (jsonInputs_.hasChangeableDispatchHandles) {
        dispatchResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.dispatchResources);
    }
    if (useAutoBindSet0_) {
        UpdateGlobalPostProcessUbo();
    }

    RenderPass renderPass;
    DispatchResources dispatchResources;
    if (graphics_) {
        renderPass = renderNodeUtil.CreateRenderPass(inputRenderPass_);
        if ((renderPass.renderPassDesc.attachmentCount == 0) ||
            !RenderHandleUtil::IsValid(renderPass.renderPassDesc.attachmentHandles[0])) {
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_W("rp_missing_" + renderNodeContextMgr_->GetName(), "RN: %s, invalid attachment",
                renderNodeContextMgr_->GetName().data());
#endif
            return;
        }
    } else {
        dispatchResources = GetDispatchResources(dispatchResources_);
        if ((!RenderHandleUtil::IsValid(dispatchResources.buffer)) &&
            (!RenderHandleUtil::IsValid(dispatchResources.image))) {
            return; // no way to evaluate dispatch size
        }
    }

    const bool invalidBindings = (!pipelineDescriptorSetBinder_->GetPipelineDescriptorSetLayoutBindingValidity());
    const auto setIndices = pipelineDescriptorSetBinder_->GetSetIndices();
    const uint32_t firstSetIndex = pipelineDescriptorSetBinder_->GetFirstSet();
    for (const auto refIndex : setIndices) {
        const auto descHandle = pipelineDescriptorSetBinder_->GetDescriptorSetHandle(refIndex);
        // handle automatic set 0 bindings
        if ((refIndex == 0) && useAutoBindSet0_) {
            UpdateSet0(cmdList);
        } else if (invalidBindings) {
            const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(refIndex);
            BindDefaultResources(refIndex, bindings);
        }
        const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(refIndex);
        cmdList.UpdateDescriptorSet(descHandle, bindings);
    }
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (!pipelineDescriptorSetBinder_->GetPipelineDescriptorSetLayoutBindingValidity()) {
        CORE_LOG_W("RN: %s, bindings missing", renderNodeContextMgr_->GetName().data());
    }
#endif

    if (graphics_) {
        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    }

    cmdList.BindPipeline(psoHandle_);

    // bind all sets
    cmdList.BindDescriptorSets(firstSetIndex, pipelineDescriptorSetBinder_->GetDescriptorSetHandles());

    if (graphics_) {
        // dynamic state
        const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);
        // push constants
        if (pipelineLayout_.pushConstant.byteSize > 0) {
            const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
            const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
            const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                ppLocalConfig_.variables.factor };
            cmdList.PushConstant(pipelineLayout_.pushConstant, arrayviewU8(pc).data());
        }

        cmdList.Draw(3u, 1u, 0u, 0u); // vertex count, instance count, first vertex, first instance
        cmdList.EndRenderPass();
    } else {
        if (RenderHandleUtil::IsValid(dispatchResources.buffer)) {
            cmdList.DispatchIndirect(dispatchResources.buffer, 0);
        } else if (RenderHandleUtil::IsValid(dispatchResources.image)) {
            const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
            const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(dispatchResources.image);
            const Math::UVec3 targetSize = { desc.width, desc.height, desc.depth };
            if (pipelineLayout_.pushConstant.byteSize > 0) {
                const float fWidth = static_cast<float>(targetSize.x);
                const float fHeight = static_cast<float>(targetSize.y);
                const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                    ppLocalConfig_.variables.factor };
                cmdList.PushConstant(pipelineLayout_.pushConstant, arrayviewU8(pc).data());
            }

            cmdList.Dispatch((targetSize.x + threadGroupSize_.x - 1u) / threadGroupSize_.x,
                (targetSize.y + threadGroupSize_.y - 1u) / threadGroupSize_.y,
                (targetSize.z + threadGroupSize_.z - 1u) / threadGroupSize_.z);
        }
    }
}

void RenderNodeCameraSinglePostProcess::RegisterOutputs()
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
        if (((jsonInputs_.defaultOutputImage == DefaultOutputImage::OUTPUT) ||
                (jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT_OUTPUT_COPY)) &&
            RenderHandleUtil::IsValid(output)) {
            registerOutput = output;
        } else if ((jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT) &&
                   RenderHandleUtil::IsValid(builtInVariables_.input)) {
            registerOutput = builtInVariables_.input;
        } else if (jsonInputs_.defaultOutputImage == DefaultOutputImage::WHITE) {
            registerOutput = builtInVariables_.defWhiteImage;
        } else {
            registerOutput = builtInVariables_.defBlackImage;
        }
    }
    shrMgr.RegisterRenderNodeOutput("output", registerOutput);
}

void RenderNodeCameraSinglePostProcess::UpdateSet0(IRenderCommandList& cmdList)
{
    const RenderHandle radianceCubemap = currentScene_.cameraEnvRadianceHandle;
    const RenderHandle colorPrePass = RenderHandleUtil::IsValid(currentScene_.prePassColorTarget)
                                          ? currentScene_.prePassColorTarget
                                          : defaultImagesAndSamplers_.colorPrePassHandle;

    auto& binder = *pipelineDescriptorSetBinder_;
    uint32_t bindingIndex = 0;
    // global
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, BindableBuffer { ubos_.postProcess.GetHandle() });
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++,
        BindableBuffer { ubos_.postProcess.GetHandle(), sizeof(GlobalPostProcessStruct) });

    // scene and camera global
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, { sceneBuffers_.camera });
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, { cameraBuffers_.generalData });

    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, { cameraBuffers_.environment });
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, { cameraBuffers_.fog });
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, { cameraBuffers_.light });
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, { cameraBuffers_.postProcess });
    binder.BindBuffer(GLOBAL_POST_PROCESS_SET, bindingIndex++, { cameraBuffers_.lightCluster });

    // scene and camera global images
    BindableImage bi;
    bi.handle = colorPrePass;
    bi.samplerHandle = defaultImagesAndSamplers_.linearMipHandle;
    binder.BindImage(GLOBAL_POST_PROCESS_SET, bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
    bi.handle = shadowBuffers_.vsmColorHandle;
    bi.samplerHandle = shadowBuffers_.vsmSamplerHandle;
    binder.BindImage(GLOBAL_POST_PROCESS_SET, bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
    bi.handle = shadowBuffers_.pcfDepthHandle;
    bi.samplerHandle = shadowBuffers_.pcfSamplerHandle;
    binder.BindImage(GLOBAL_POST_PROCESS_SET, bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);
    bi.handle = radianceCubemap;
    bi.samplerHandle = defaultImagesAndSamplers_.cubemapHandle;
    binder.BindImage(GLOBAL_POST_PROCESS_SET, bindingIndex++, bi, CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT);

    // NOTE: UpdateDescriptorSets is done when called
}

void RenderNodeCameraSinglePostProcess::BindDefaultResources(
    const uint32_t set, const DescriptorSetLayoutBindingResources& bindings)
{
    if (pipelineDescriptorSetBinder_) {
        auto& binder = *pipelineDescriptorSetBinder_;
        for (const auto& ref : bindings.buffers) {
            if (!RenderHandleUtil::IsValid(ref.resource.handle)) {
                binder.BindBuffer(set, ref.binding.binding, BindableBuffer { builtInVariables_.defBuffer });
            }
        }
        for (const auto& ref : bindings.images) {
            if (!RenderHandleUtil::IsValid(ref.resource.handle)) {
                BindableImage bi;
                bi.handle = builtInVariables_.defBlackImage;
                bi.samplerHandle = builtInVariables_.defSampler;
                binder.BindImage(set, ref.binding.binding, bi);
            }
        }
        for (const auto& ref : bindings.samplers) {
            if (!RenderHandleUtil::IsValid(ref.resource.handle)) {
                binder.BindSampler(set, ref.binding.binding, BindableSampler { builtInVariables_.defSampler });
            }
        }
    }
}

void RenderNodeCameraSinglePostProcess::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    const auto scene = dataStoreScene.GetScene();
    uint32_t cameraIdx = scene.cameraIndex;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraName);
    }

    if (const auto cameras = dataStoreCamera.GetCameras(); cameraIdx < (uint32_t)cameras.size()) {
        // store current frame camera
        currentScene_.camera = cameras[cameraIdx];
    }
    const auto camHandles = RenderNodeSceneUtil::GetSceneCameraImageHandles(
        *renderNodeContextMgr_, stores_.dataStoreNameScene, currentScene_.camera.name, currentScene_.camera);
    currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;

    const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight.GetLightCounts();
    currentScene_.hasShadow = ((lightCounts.dirShadow > 0) || (lightCounts.spotShadow > 0)) ? true : false;
    currentScene_.shadowTypes = dataStoreLight.GetShadowTypes();
    currentScene_.lightingFlags = dataStoreLight.GetLightingFlags();
}

void RenderNodeCameraSinglePostProcess::ProcessPostProcessConfiguration()
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

void RenderNodeCameraSinglePostProcess::UpdateGlobalPostProcessUbo()
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

void RenderNodeCameraSinglePostProcess::GetSceneUniformBuffers(const string_view us)
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

void RenderNodeCameraSinglePostProcess::InitCreateBinders()
{
    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    {
        DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
        if (jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT_OUTPUT_COPY) {
            const DescriptorCounts copyDc = renderCopy_.GetDescriptorCounts();
            for (const auto& ref : copyDc.counts) {
                dc.counts.push_back(ref);
            }
        }
        descriptorSetMgr.ResetAndReserve(dc);
    }

    pipelineDescriptorSetBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(pipelineLayout_);
    if (pipelineDescriptorSetBinder_) {
        renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
        if (NeedsAutoBindingSet0(inputResources_)) {
            useAutoBindSet0_ = true;
        }
    } else {
        valid_ = false;
    }
    if ((!RenderHandleUtil::IsValid(shader_)) || (!RenderHandleUtil::IsValid(psoHandle_))) {
        valid_ = false;
    }
}

void RenderNodeCameraSinglePostProcess::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.dispatchResources = parserUtil.GetInputResources(jsonVal, "dispatchResources");
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
    const auto defaultOutput = parserUtil.GetStringValue(jsonVal, "defaultOutputImage");
    if (!defaultOutput.empty()) {
        if (defaultOutput == "output") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::OUTPUT;
        } else if (defaultOutput == "input_output_copy") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::INPUT_OUTPUT_COPY;
        } else if (defaultOutput == "input") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::INPUT;
        } else if (defaultOutput == "black") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::BLACK;
        } else if (defaultOutput == "white") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::WHITE;
        } else {
            CORE_LOG_W(
                "RenderNodeCameraSinglePostProcess default output image not supported (%s)", defaultOutput.c_str());
        }
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    dispatchResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.dispatchResources);

    jsonInputs_.hasChangeableRenderPassHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.renderPass);
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);
    jsonInputs_.hasChangeableDispatchHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.dispatchResources);

    // process custom resources
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customInputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customInputImages[idx];
        if (ref.usageName == INPUT) {
            jsonInputs_.inputIdx = idx;
        }
    }
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customOutputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customOutputImages[idx];
        if (ref.usageName == OUTPUT) {
            jsonInputs_.outputIdx = idx;
        }
    }
}

void RenderNodeCameraSinglePostProcess::UpdateImageData()
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBuffer)) {
        builtInVariables_.defBuffer = ubos_.postProcess.GetHandle();
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBlackImage)) {
        builtInVariables_.defBlackImage = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_BLACK);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defWhiteImage)) {
        builtInVariables_.defWhiteImage = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_WHITE);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defSampler)) {
        builtInVariables_.defSampler = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    }
    if (jsonInputs_.inputIdx < inputResources_.customInputImages.size()) {
        builtInVariables_.input = inputResources_.customInputImages[jsonInputs_.inputIdx].handle;
    }
    if (jsonInputs_.outputIdx < inputResources_.customOutputImages.size()) {
        builtInVariables_.output = inputResources_.customOutputImages[jsonInputs_.outputIdx].handle;
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeCameraSinglePostProcess::Create()
{
    return new RenderNodeCameraSinglePostProcess();
}

void RenderNodeCameraSinglePostProcess::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCameraSinglePostProcess*>(instance);
}
CORE3D_END_NAMESPACE()
