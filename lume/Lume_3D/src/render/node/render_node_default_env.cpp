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

#include "render_node_default_env.h"

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

RenderHandleReference CreateDefaultSampler(IRenderNodeGpuResourceManager& gpuResourceMgr)
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

struct InputEnvironmentDataHandles {
    RenderHandle cubeHandle;
    RenderHandle texHandle;
    float lodLevel { 0.0f };
};

InputEnvironmentDataHandles GetEnvironmentDataHandles(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera::Environment& renderEnvironment)
{
    InputEnvironmentDataHandles iedh;
    iedh.texHandle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_BASE_COLOR);
    iedh.cubeHandle = gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_SKYBOX_CUBEMAP);

    if (renderEnvironment.envMap) {
        const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(renderEnvironment.envMap.GetHandle());
        if ((renderEnvironment.backgroundType == RenderCamera::Environment::BG_TYPE_IMAGE) ||
            (renderEnvironment.backgroundType == RenderCamera::Environment::BG_TYPE_EQUIRECTANGULAR)) {
            if (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_2D) {
                iedh.texHandle = renderEnvironment.envMap.GetHandle();
            } else {
                CORE_LOG_E("invalid environment map, type does not match background type");
            }
        } else if (renderEnvironment.backgroundType == RenderCamera::Environment::BG_TYPE_CUBEMAP) {
            iedh.cubeHandle = renderEnvironment.envMap.GetHandle();
            if (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_CUBE) {
                iedh.cubeHandle = renderEnvironment.envMap.GetHandle();
            } else {
                CORE_LOG_E("invalid environment map, type does not match background type");
            }
        }
        iedh.lodLevel = renderEnvironment.envMapLodLevel;
    }
    return iedh;
}
} // namespace

void RenderNodeDefaultEnv::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    currentScene_ = {};
    currentBgType_ = { RenderCamera::Environment::BG_TYPE_NONE };

    if ((jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) &&
        jsonInputs_.renderDataStore.dataStoreName.empty()) {
        CORE_LOG_V("%s: render data store post process configuration not set in render node graph",
            renderNodeContextMgr_->GetName().data());
    }

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    createdHandles_.sampler = CreateDefaultSampler(gpuResourceMgr);
    renderPass_ = renderNodeContextMgr.GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    bufferHandles_.defaultBuffer = gpuResourceMgr.GetBufferHandle("CORE_DEFAULT_GPU_BUFFER");
    GetCameraUniformBuffers();

    const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    // default pipeline layout
    {
        const RenderHandle plHandle =
            shaderMgr.GetPipelineLayoutHandle(DefaultMaterialShaderConstants::PIPELINE_LAYOUT_ENV);
        defaultPipelineLayout_ = shaderMgr.GetPipelineLayout(plHandle);
    }
    shaderHandle_ = shaderMgr.GetShaderHandle(DefaultMaterialShaderConstants::ENV_SHADER_NAME);

    CreateDescriptorSets();
}

void RenderNodeDefaultEnv::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeDefaultEnv::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    if (dataStoreLight && dataStoreCamera && dataStoreScene) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera);
    }

    cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);

    if (currentScene_.camera.environment.backgroundType != RenderCamera::Environment::BG_TYPE_NONE) {
        if (currentScene_.camera.layerMask & currentScene_.camera.environment.layerMask) {
            UpdatePostProcessConfiguration();
            RenderData(cmdList);
        }
    }

    cmdList.EndRenderPass();
}

void RenderNodeDefaultEnv::RenderData(IRenderCommandList& cmdList)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);

    const RenderCamera::Environment& renderEnv = currentScene_.camera.environment;
    const RenderHandle shaderHandle = renderEnv.shader ? renderEnv.shader.GetHandle() : shaderHandle_;
    // check for pso changes
    if ((renderEnv.backgroundType != currentBgType_) || (shaderHandle.id != shaderHandle_.id)) {
        currentBgType_ = currentScene_.camera.environment.backgroundType;
        psoHandle_ = GetPso(shaderHandle, currentBgType_, currentRenderPPConfiguration_);
    }

    cmdList.BindPipeline(psoHandle_);

    // set 0, ubos
    {
        auto& binder = *allDescriptorSets_.set0;
        uint32_t bindingIndex = 0;
        binder.BindBuffer(bindingIndex++, bufferHandles_.camera, 0u);
        binder.BindBuffer(bindingIndex++, bufferHandles_.generalData, 0u);
        binder.BindBuffer(bindingIndex++, bufferHandles_.environment, 0u);
        binder.BindBuffer(bindingIndex++, bufferHandles_.fog, 0u);
        binder.BindBuffer(bindingIndex++, bufferHandles_.postProcess, 0u);
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }

    const auto envDataHandles = GetEnvironmentDataHandles(gpuResourceMgr, currentScene_.camera.environment);
    // set 1, bind combined image samplers
    {
        auto& binder = *allDescriptorSets_.set1;
        uint32_t bindingIndex = 0;
        binder.BindImage(bindingIndex++, envDataHandles.texHandle, createdHandles_.sampler.GetHandle());
        binder.BindImage(bindingIndex++, envDataHandles.cubeHandle, createdHandles_.sampler.GetHandle());
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }

    // bind build-in sets
    const RenderHandle descSets[] = { allDescriptorSets_.set0->GetDescriptorSetHandle(),
        allDescriptorSets_.set1->GetDescriptorSetHandle() };
    cmdList.BindDescriptorSets(0u, descSets);

    // custom set 2 resources
    bool validDraw = true;
    if (renderEnv.customResourceHandles[0]) {
        validDraw = UpdateAndBindCustomSet(cmdList, renderEnv);
    }

    if (validDraw) {
        cmdList.Draw(3u, 1u, 0u, 0u);
    }
}

bool RenderNodeDefaultEnv::UpdateAndBindCustomSet(
    IRenderCommandList& cmdList, const RenderCamera::Environment& renderEnv)
{
    constexpr uint32_t fixedSet = 2u;
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    RenderHandle currPlHandle = shaderMgr.GetPipelineLayoutHandleByShaderHandle(renderEnv.shader.GetHandle());
    const PipelineLayout& plRef = shaderMgr.GetPipelineLayout(currPlHandle);
    const auto& descBindings = plRef.descriptorSetLayouts[fixedSet].bindings;
    const RenderHandle descSetHandle = descriptorSetMgr.CreateOneFrameDescriptorSet(descBindings);
    bool valid = false;
    uint32_t validResCount = 0;
    for (uint32_t idx = 0; idx < RenderSceneDataConstants::MAX_ENV_CUSTOM_RESOURCE_COUNT; ++idx) {
        if (renderEnv.customResourceHandles[idx]) {
            validResCount++;
        } else {
            break;
        }
    }
    const array_view<const RenderHandleReference> customResourceHandles(renderEnv.customResourceHandles, validResCount);
    if (RenderHandleUtil::IsValid(descSetHandle) && (descBindings.size() == customResourceHandles.size())) {
        IDescriptorSetBinder::Ptr binderPtr = descriptorSetMgr.CreateDescriptorSetBinder(descSetHandle, descBindings);
        if (binderPtr) {
            auto& binder = *binderPtr;
            for (uint32_t idx = 0; idx < static_cast<uint32_t>(customResourceHandles.size()); ++idx) {
                const RenderHandle currRes = customResourceHandles[idx].GetHandle();
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
                cmdList.BindDescriptorSet(fixedSet, binder.GetDescriptorSetHandle());
                valid = true;
            }
        }
    }
    if (!valid) {
        CORE_LOG_ONCE_E("default_env_custom_res_issue",
            "invalid bindings with custom shader descriptor set 2 (render node: %s)",
            renderNodeContextMgr_->GetName().data());
    }
    return valid;
}

void RenderNodeDefaultEnv::UpdateCurrentScene(
    const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultCamera& dataStoreCamera)
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
}

RenderHandle RenderNodeDefaultEnv::GetPso(const RenderHandle shaderHandle,
    const RenderCamera::Environment::BackgroundType bgType,
    const RenderPostProcessConfiguration& renderPostProcessConfiguration)
{
    if (RenderHandleUtil::IsValid(shaderHandle)) {
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const ShaderSpecilizationConstantView sscv = shaderMgr.GetReflectionSpecialization(shaderHandle);
        vector<uint32_t> flags(sscv.constants.size());

        constexpr uint32_t defaultEnvType = 0u;
        constexpr uint32_t postProcessFlags = 1u;

        for (const auto& ref : sscv.constants) {
            const uint32_t constantId = ref.offset / sizeof(uint32_t);

            if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT) {
                switch (ref.id) {
                    case defaultEnvType:
                        flags[constantId] = (uint32_t)bgType;
                        break;
                    case postProcessFlags:
                        flags[constantId] = renderPostProcessConfiguration.flags.x;
                        break;
                    default:
                        break;
                }
            }
        }

        const ShaderSpecializationConstantDataView specialization { sscv.constants, flags };
        const DynamicStateFlags dynamicStateFlags =
            DynamicStateFlagBits::CORE_DYNAMIC_STATE_VIEWPORT | DynamicStateFlagBits::CORE_DYNAMIC_STATE_SCISSOR;
        const RenderHandle plHandle = shaderMgr.GetPipelineLayoutHandleByShaderHandle(shaderHandle);
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle);
        psoHandle_ = renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(
            shaderHandle, gfxHandle, plHandle, {}, specialization, dynamicStateFlags);
    }
    return psoHandle_;
}

void RenderNodeDefaultEnv::CreateDescriptorSets()
{
    auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    {
        // automatic calculation
        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(defaultPipelineLayout_);
        descriptorSetMgr.ResetAndReserve(dc);
    }

    CORE_ASSERT(defaultPipelineLayout_.descriptorSetCount == 2u);
    {
        const uint32_t set = 0u;
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, defaultPipelineLayout_);
        allDescriptorSets_.set0 = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetHandle, defaultPipelineLayout_.descriptorSetLayouts[set].bindings);
    }
    {
        const uint32_t set = 1u;
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, defaultPipelineLayout_);
        allDescriptorSets_.set1 = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetHandle, defaultPipelineLayout_.descriptorSetLayouts[set].bindings);
    }
}

void RenderNodeDefaultEnv::UpdatePostProcessConfiguration()
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

void RenderNodeDefaultEnv::GetCameraUniformBuffers()
{
    string camName;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        camName = jsonInputs_.customCameraName;
    }
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    bufferHandles_.camera = gpuResourceMgr.GetBufferHandle(
        stores_.dataStoreNameScene + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);
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
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.camera);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.generalData);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.environment);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.fog);
    checkValidity(bufferHandles_.defaultBuffer, valid, bufferHandles_.postProcess);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all camera buffers found.", renderNodeContextMgr_->GetName().data());
    }
}

void RenderNodeDefaultEnv::ParseRenderNodeInputs()
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

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultEnv::Create()
{
    return new RenderNodeDefaultEnv();
}

void RenderNodeDefaultEnv::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultEnv*>(instance);
}
CORE3D_END_NAMESPACE()
