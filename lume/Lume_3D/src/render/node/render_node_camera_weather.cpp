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

#include "render_node_camera_weather.h"

#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "render/datastore/render_data_store_weather.h"
#include "render/shaders/common/render_post_process_structs_common.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

constexpr bool ENABLE_ADDITIONAL { false };

CORE3D_BEGIN_NAMESPACE()
struct RenderNodeCameraWeather::Settings {
    BASE_NS::Math::Vec4 params[7];
    BASE_NS::Math::Vec4 padding[16 - 7];
};

namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr Math::Vec4 DEBUG_MARKER_COLOR { 0.5f, 0.75f, 0.75f, 1.0f };

template<typename T>
RenderHandleReference CreateCloudDataUniformBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    CORE_STATIC_ASSERT(sizeof(T) == PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE);
    return gpuResourceMgr.Create(
        handle, GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, sizeof(T) });
}
} // namespace

void RenderNodeCameraWeather::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    if (renderNodeSceneUtil_ = CORE_NS::GetInstance<IRenderNodeSceneUtil>(
            *(renderNodeContextMgr_->GetRenderContext().GetInterface<IClassRegister>()), UID_RENDER_NODE_SCENE_UTIL);
        renderNodeSceneUtil_) {
        stores_ = renderNodeSceneUtil_->GetSceneRenderDataStores(
            renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
        dsWeatherName_ = renderNodeSceneUtil_->GetSceneRenderDataStore(stores_, RenderDataStoreWeather::TYPE_NAME);

        GetSceneUniformBuffers(stores_.dataStoreNameScene);
    }
    currentScene_ = {};

    valid_ = true;
    clear_ = true;
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();

    GpuSamplerDesc desc {};
    desc.minFilter = Filter::CORE_FILTER_LINEAR;
    desc.mipMapMode = Filter::CORE_FILTER_NEAREST;
    desc.magFilter = Filter::CORE_FILTER_LINEAR;
    desc.enableAnisotropy = false;
    desc.maxAnisotropy = 1;
    desc.minLod = 0;
    desc.maxLod = 0;
    desc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    desc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_REPEAT;
    volumeSampler_ = gpuResourceMgr.Create(desc);

    desc.minFilter = Filter::CORE_FILTER_LINEAR;
    desc.mipMapMode = Filter::CORE_FILTER_NEAREST;
    desc.magFilter = Filter::CORE_FILTER_LINEAR;
    desc.enableAnisotropy = false;
    desc.maxAnisotropy = 1;
    desc.minLod = 0;
    desc.maxLod = 0;
    desc.borderColor = Render::BorderColor::CORE_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
    desc.addressModeU = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    desc.addressModeV = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    desc.addressModeW = SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    weatherSampler_ = gpuResourceMgr.Create(desc);

    builtInVariables_.defSampler = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");

    skyTex_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");

    if (renderNodeSceneUtil_) {
        const auto camHandles = renderNodeSceneUtil_->GetSceneCameraImageHandles(
            *renderNodeContextMgr_, stores_.dataStoreNameScene, currentScene_.camera.name, currentScene_.camera);
        currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;
    }

    vector<DescriptorCounts> descriptorCounts;
    Load("3dshaders://shader/clouds/cloud_volumes.shader", descriptorCounts, cloudVolumeShaderData_, DYNAMIC_STATES, 1);

    Load("3dshaders://shader/clouds/upsample.shader", descriptorCounts, upsampleData1_, DYNAMIC_STATES, 1);
    Load("3dshaders://shader/clouds/upsample2.shader", descriptorCounts, upsampleData2_, DYNAMIC_STATES, 1);

    renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(descriptorCounts);

    cloudVolumeBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(cloudVolumeShaderData_.pipelineLayoutData);
    upsample1Binder = renderNodeUtil.CreatePipelineDescriptorSetBinder(upsampleData1_.pipelineLayoutData);
    upsample2Binder = renderNodeUtil.CreatePipelineDescriptorSetBinder(upsampleData2_.pipelineLayoutData);

    isDefaultImageInUse_ = false;

    // get camera ubo
    {
        cameraUboHandle_ = gpuResourceMgr.GetBufferHandle(
            stores_.dataStoreNameScene + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);
    }

    uniformBuffer_ = CreateCloudDataUniformBuffer<Settings>(gpuResourceMgr, uniformBuffer_);
}

struct RenderNodeCameraWeather::GlobalFrameData {
    float time { 0.0f };
};

void RenderNodeCameraWeather::PreExecuteFrame()
{
    {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto* dataStoreScene = static_cast<IRenderDataStoreDefaultScene*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
        const auto* dataStoreCamera = static_cast<IRenderDataStoreDefaultCamera*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
        const auto* dataStoreLight = static_cast<IRenderDataStoreDefaultLight*>(
            renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));
        const auto* dataStoreWeather =
            static_cast<RenderDataStoreWeather*>(renderDataStoreMgr.GetRenderDataStore(dsWeatherName_));
        const bool validRenderDataStore = dataStoreScene && dataStoreCamera && dataStoreLight && dataStoreWeather;
        if (validRenderDataStore) {
            UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);

            settings_ = dataStoreWeather->GetWeatherSettings();
            downscale_ = (int32_t)settings_.cloudRenderingType;
        }
    }
}

void RenderNodeCameraWeather::ExecuteFrame(IRenderCommandList& cmdList)
{
    lowFrequencyTex_ = settings_.lowFrequencyImage;
    highFrequencyTex_ = settings_.highFrequencyImage;
    curlnoiseTex_ = settings_.curlNoiseImage;
    cirrusTex_ = settings_.cirrusImage;
    weatherMapTex_ = settings_.weatherMapImage;

    cloudTexture_ = renderNodeContextMgr_->GetGpuResourceManager().GetImageHandle(
        DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD");
    cloudTexturePrev_ = renderNodeContextMgr_->GetGpuResourceManager().GetImageHandle(
        DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "CLOUD_PREV");

    [this]() {
        auto& b = cloudVolumeBinder_;
        uint32_t binding = 0;
        b->BindSampler(0, binding++, { builtInVariables_.defSampler });
        b->BindImage(0, binding++, { skyTex_ });
        b->BindSampler(0, binding++, { volumeSampler_.GetHandle() });
        b->BindImage(0, binding++, { lowFrequencyTex_ });
        b->BindImage(0, binding++, { highFrequencyTex_ });
        b->BindImage(0, binding++, { curlnoiseTex_ });
        b->BindBuffer(0, binding++, { uniformBuffer_.GetHandle() });
        b->BindSampler(0, binding++, { weatherSampler_.GetHandle() });
        b->BindImage(0, binding++, { cirrusTex_ });
    }();

    if constexpr (ENABLE_ADDITIONAL) {
        [this]() {
            auto& b = upsample2Binder;
            uint32_t binding = 0;
            b->BindSampler(0, binding++, { builtInVariables_.defSampler });
            b->BindBuffer(0, binding++, { uniformBuffer_.GetHandle() });
            b->BindBuffer(0, binding++, { cameraUboHandle_ });
        }();

        [this]() {
            auto& b = upsample1Binder;
            uint32_t binding = 0;
            b->BindSampler(0, binding++, { builtInVariables_.defSampler });
            b->BindBuffer(0, binding++, { uniformBuffer_.GetHandle() });
            b->BindBuffer(0, binding++, { cameraUboHandle_ });
        }();
    }

    const auto sHash = BASE_NS::FNV1a32Hash(reinterpret_cast<const uint8_t*>(&settings_), sizeof(settings_));
    if (uniformHash_ != sHash) {
        uniformHash_ = sHash;
        Settings uniformSettings;
        uniformSettings.params[0U].x = settings_.coverage;
        uniformSettings.params[0U].y = settings_.precipitation;
        uniformSettings.params[0U].z = settings_.cloudType;
        uniformSettings.params[0U].w = settings_.domain[0U];

        uniformSettings.params[1U].x = currentScene_.camera.zNear;
        uniformSettings.params[1U].y = currentScene_.camera.zFar;
        uniformSettings.params[1U].z = settings_.silverIntensity;
        uniformSettings.params[1U].w = settings_.silverSpread;

        uniformSettings.params[2U].x = settings_.sunPos[0U];
        uniformSettings.params[2U].y = settings_.sunPos[1U];
        uniformSettings.params[2U].z = settings_.sunPos[2U];
        uniformSettings.params[2U].w = settings_.domain[1U];

        uniformSettings.params[3U].x = settings_.atmosphereColor[0U];
        uniformSettings.params[3U].y = settings_.atmosphereColor[1U];
        uniformSettings.params[3U].z = settings_.atmosphereColor[2U];
        uniformSettings.params[3U].w = settings_.domain[2U];

        uniformSettings.params[4U].x = settings_.anvil_bias;
        uniformSettings.params[4U].y = settings_.brightness;
        uniformSettings.params[4U].z = settings_.eccintricity;
        uniformSettings.params[4U].w = settings_.domain[3U];

        uniformSettings.params[5U].x = settings_.ambient;
        uniformSettings.params[5U].y = settings_.direct;
        uniformSettings.params[5U].z = settings_.lightStep;
        uniformSettings.params[5U].w = settings_.scatteringDist;

        uniformSettings.params[6U].w = settings_.domain[4U];

        UpdateBufferData<Settings>(
            renderNodeContextMgr_->GetGpuResourceManager(), uniformBuffer_.GetHandle(), uniformSettings);
    }

    GlobalFrameData data {};
    data.time = renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderTimings.z;
    cameraUboHandle_ = renderNodeContextMgr_->GetGpuResourceManager().GetBufferHandle(
        stores_.dataStoreNameScene + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);

    {
        // NOTE: Need to be update every frame if cannot be sure (descriptor set manager tries to optimize not needed
        // bindings)
        cmdList.UpdateDescriptorSet(cloudVolumeBinder_->GetDescriptorSetHandle(0),
            cloudVolumeBinder_->GetDescriptorSetLayoutBindingResources(0));
        if constexpr (ENABLE_ADDITIONAL) {
            cmdList.UpdateDescriptorSet(
                upsample1Binder->GetDescriptorSetHandle(0), upsample1Binder->GetDescriptorSetLayoutBindingResources(0));
            cmdList.UpdateDescriptorSet(
                upsample2Binder->GetDescriptorSetHandle(0), upsample2Binder->GetDescriptorSetLayoutBindingResources(0));
        }
    }

    if (settings_.cloudRenderingType == RenderDataStoreWeather::CloudRenderingType::REPROJECTED) {
        ExecuteReprojected(cmdList, data);
    } else if (settings_.cloudRenderingType == RenderDataStoreWeather::CloudRenderingType::DOWNSCALED) {
        ExecuteDownscaled(cmdList, data);
    } else {
        ExecuteFull(cmdList, data);
    }

    // change index per frame
    targetIndex_ = (targetIndex_ + 1) % 2U;
}

void RenderNodeCameraWeather::ExecuteReprojected(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data)
{
    if (clear_) {
        clear_ = false;
        ExplicitPass p;
        p.size.x = currentScene_.camera.renderResolution.x;
        p.size.y = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.attachmentCount = 2U;
        p.p.renderPassDesc.attachments[0U].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        p.p.renderPassDesc.attachments[0U].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        p.p.renderPassDesc.attachments[0U].clearValue = ClearValue(0, 0, 0, 0);
        p.p.renderPassDesc.attachments[1U].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        p.p.renderPassDesc.attachments[1U].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        p.p.renderPassDesc.attachments[1U].clearValue = ClearValue(0, 0, 0, 0);
        p.p.renderPassDesc.attachmentHandles[0U] = cloudTexture_;
        p.p.renderPassDesc.attachmentHandles[1U] = cloudTexturePrev_;
        p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x;
        p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.renderArea.offsetX = 0;
        p.p.renderPassDesc.renderArea.offsetY = 0;
        p.p.renderPassDesc.subpassCount = 1;
        p.p.subpassDesc.colorAttachmentCount = 2u;
        p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
        p.p.subpassDesc.colorAttachmentIndices[1u] = 1u;
        p.p.subpassStartIndex = 0u;

        // clear the accumulation buffers on the first frame
        ExecuteClear(cmdList, p, data);
    }

    RenderHandle output = ((targetIndex_ % 2U) == 0) ? cloudTexture_ : cloudTexturePrev_;

    ExplicitPass p;
    p.size.x = currentScene_.camera.renderResolution.x >> downscale_;
    p.size.y = currentScene_.camera.renderResolution.y >> downscale_;
    p.p.renderPassDesc.attachmentCount = 1;
    p.p.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
    p.p.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    p.p.renderPassDesc.attachments[0].clearValue = ClearValue(0, 0, 0, 0);
    p.p.renderPassDesc.attachmentHandles[0] = output;
    p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x >> downscale_;
    p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y >> downscale_;
    p.p.renderPassDesc.renderArea.offsetX = 0;
    p.p.renderPassDesc.renderArea.offsetY = 0;
    p.p.renderPassDesc.subpassCount = 1;
    p.p.subpassDesc.colorAttachmentCount = 1u;
    p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
    p.p.subpassStartIndex = 0u;

    // generate limited information represeting clouds, ambient,direct,blend,transmission
    ExecuteGenerateClouds(cmdList, p, data);

    if constexpr (ENABLE_ADDITIONAL) {
        p.size.x = currentScene_.camera.renderResolution.x;
        p.size.y = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.attachmentCount = 1;
        p.p.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
        p.p.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        p.p.renderPassDesc.attachments[0].clearValue = ClearValue(0, 0, 0, 0);
        p.p.renderPassDesc.attachmentHandles[0] = cloudTexture_;
        p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x;
        p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.renderArea.offsetX = 0;
        p.p.renderPassDesc.renderArea.offsetY = 0;
        p.p.renderPassDesc.subpassCount = 1;
        p.p.subpassDesc.colorAttachmentCount = 1u;
        p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
        p.p.subpassDesc.subpassFlags = 0;
        p.p.subpassStartIndex = 0u;

        // NOTE: the reprojection should happen in translucent pass with depth test enabled
        // this would fix most of the resolution artifacts
        // probably 1 step upsample should be enough

        // accumulates the results with results reproject from last frame, the results generated by the previous
        // pass are 1/16th of frame, round robinned.
        ExecuteUpsampleAndReproject(cmdList, cloudTexture_, cloudTexturePrev_, p, data);

        p.size.x = currentScene_.camera.renderResolution.x;
        p.size.y = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.attachmentCount = 1;
        p.p.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD;
        p.p.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        p.p.renderPassDesc.attachments[0].clearValue = ClearValue(0, 0, 0, 0);
        p.p.renderPassDesc.attachmentHandles[0] = cloudTexture_;
        p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x;
        p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.renderArea.offsetX = 0;
        p.p.renderPassDesc.renderArea.offsetY = 0;
        p.p.renderPassDesc.subpassCount = 1;
        p.p.subpassDesc.colorAttachmentCount = 1u;
        p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
        p.p.subpassDesc.subpassFlags = CORE_SUBPASS_MERGE_BIT;
        p.p.subpassStartIndex = 0u;

        // colorize the clouds
        ExecuteUpsampleAndPostProcess(cmdList, cloudTexturePrev_, p, data);
    }
}

void RenderNodeCameraWeather::ExecuteDownscaled(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data)
{
    ExplicitPass p;
    p.size.x = currentScene_.camera.renderResolution.x >> downscale_;
    p.size.y = currentScene_.camera.renderResolution.y >> downscale_;
    p.p.renderPassDesc.attachmentCount = 1;
    p.p.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
    p.p.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    p.p.renderPassDesc.attachments[0].clearValue = ClearValue(0, 0, 0, 0);
    p.p.renderPassDesc.attachmentHandles[0] = cloudTexture_;
    p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x >> 1;
    p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y >> 1;
    p.p.renderPassDesc.renderArea.offsetX = 0;
    p.p.renderPassDesc.renderArea.offsetY = 0;
    p.p.renderPassDesc.subpassCount = 1;
    p.p.subpassDesc.colorAttachmentCount = 1u;
    p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
    p.p.subpassStartIndex = 0u;

    // generate limited information represeting clouds, ambient, direct, blend, transmission
    ExecuteGenerateClouds(cmdList, p, data);

    if constexpr (ENABLE_ADDITIONAL) {
        p.size.x = currentScene_.camera.renderResolution.x;
        p.size.y = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.attachmentCount = 1;
        p.p.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD;
        p.p.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        p.p.renderPassDesc.attachments[0].clearValue = ClearValue(0, 0, 0, 0);
        p.p.renderPassDesc.attachmentHandles[0] = cloudTexture_;
        p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x;
        p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.renderArea.offsetX = 0;
        p.p.renderPassDesc.renderArea.offsetY = 0;
        p.p.renderPassDesc.subpassCount = 1;
        p.p.subpassDesc.colorAttachmentCount = 1u;
        p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
        p.p.subpassStartIndex = 0u;

        // colorize the clouds
        ExecuteUpsampleAndPostProcess(cmdList, cloudTexture_, p, data);
    }
}

void RenderNodeCameraWeather::ExecuteFull(RENDER_NS::IRenderCommandList& cmdList, const GlobalFrameData& data)
{
    ExplicitPass p;
    p.size.x = currentScene_.camera.renderResolution.x;
    p.size.y = currentScene_.camera.renderResolution.y;
    p.p.renderPassDesc.attachmentCount = 1;
    p.p.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
    p.p.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    p.p.renderPassDesc.attachments[0].clearValue = ClearValue(0, 0, 0, 0);
    p.p.renderPassDesc.attachmentHandles[0] = cloudTexture_;
    p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x;
    p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y;
    p.p.renderPassDesc.renderArea.offsetX = 0;
    p.p.renderPassDesc.renderArea.offsetY = 0;
    p.p.renderPassDesc.subpassCount = 1;
    p.p.subpassDesc.colorAttachmentCount = 1u;
    p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
    p.p.subpassDesc.subpassFlags = CORE_SUBPASS_MERGE_BIT;
    p.p.subpassStartIndex = 0u;

    // generate limited information represeting clouds, ambient, direct, blend, transmission
    ExecuteGenerateClouds(cmdList, p, data);

    if constexpr (ENABLE_ADDITIONAL) {
        p.size.x = currentScene_.camera.renderResolution.x;
        p.size.y = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.attachmentCount = 1;
        p.p.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_LOAD;
        p.p.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        p.p.renderPassDesc.attachments[0].clearValue = ClearValue(0, 0, 0, 0);
        p.p.renderPassDesc.attachmentHandles[0] = cloudTexture_;
        p.p.renderPassDesc.renderArea.extentWidth = currentScene_.camera.renderResolution.x;
        p.p.renderPassDesc.renderArea.extentHeight = currentScene_.camera.renderResolution.y;
        p.p.renderPassDesc.renderArea.offsetX = 0;
        p.p.renderPassDesc.renderArea.offsetY = 0;
        p.p.renderPassDesc.subpassCount = 1;
        p.p.subpassDesc.colorAttachmentCount = 1u;
        p.p.subpassDesc.colorAttachmentIndices[0u] = 0u;
        p.p.subpassDesc.subpassFlags = CORE_SUBPASS_MERGE_BIT;
        p.p.subpassStartIndex = 0u;

        // colorize the clouds
        ExecuteUpsampleAndPostProcess(cmdList, cloudTexture_, p, data);
    }
}

void RenderNodeCameraWeather::ExecuteUpsampleAndReproject(RENDER_NS::IRenderCommandList& cmdList,
    RENDER_NS::RenderHandle input, RENDER_NS::RenderHandle prevInput, const ExplicitPass& p, const GlobalFrameData& d)
{
    // NOTE: not in use at the moment
    CORE_ASSERT(false);

    const auto& settings = settings_;
    GraphicsShaderData& s = upsampleData2_;
    auto& b = upsample2Binder;
    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "clouds-upsample", DEBUG_MARKER_COLOR);

    auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    uint32_t binding = 0U;
    // current cloud texture (RGBA: direct, ambient, blend, transmission)
    b->BindImage(1, binding++, { input });
    // accumulation texture used in previous frame
    b->BindImage(1, binding++, { prevInput });
    // used to derive camera matrices
    b->BindBuffer(1, binding++, { cameraUboHandle_ });
    cmdList.UpdateDescriptorSet(b->GetDescriptorSetHandle(1), b->GetDescriptorSetLayoutBindingResources(1));
    b->PrintPipelineDescriptorSetLayoutBindingValidation();

    RenderPass renderPass = CreateRenderPass(p.p.renderPassDesc.attachmentHandles[0]);
    const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);

    const float fEffectWidth = viewportDesc.width;
    const float fEffectHeight = viewportDesc.height;
    const Math::Vec4 texSizeInvTexSize { fEffectWidth, fEffectHeight, 1.0f / fEffectWidth, 1.0f / fEffectHeight };
    const Math::UVec2 size { static_cast<uint32_t>(fEffectWidth), static_cast<uint32_t>(fEffectHeight) };

    BlurShaderPushConstant pcV {};

    // pcV.view = d.view;
    pcV.dir[0U].x = d.time;
    pcV.dir[0U].y = float(targetIndex_) + .5f;
    pcV.dir[0U].z = settings.density;
    pcV.dir[0U].w = settings.altitude;

    pcV.dir[1U].x = settings.ambientColor[0U];
    pcV.dir[1U].y = settings.ambientColor[1U];
    pcV.dir[1U].z = settings.ambientColor[2U];
    pcV.dir[1U].w = settings.windDir[0U];

    pcV.dir[2U].x = settings.sunColor[0U];
    pcV.dir[2U].y = settings.sunColor[1U];
    pcV.dir[2U].z = settings.sunColor[2U];
    pcV.dir[2U].w = settings.windDir[1U];

    pcV.dir[3U].w = settings.windDir[2U];
    const BlurParams<BlurShaderPushConstant, ExplicitPass> vBlur { s, *b, { pcV }, p };
    RenderFullscreenTriangle(0, cmdList, vBlur);
}

void RenderNodeCameraWeather::ExecuteUpsampleAndPostProcess(RENDER_NS::IRenderCommandList& cmdList,
    RENDER_NS::RenderHandle input, const ExplicitPass& p, const GlobalFrameData& d)
{
    // NOTE: not in use at the moment
    CORE_ASSERT(false);

    const auto& settings = settings_;
    GraphicsShaderData& s = upsampleData1_;
    auto& b = upsample1Binder;
    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "clouds-postprocess", DEBUG_MARKER_COLOR);

    auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    // current accumulated/reprjected results, (RGBA: direct, ambient, blend, transmission)
    uint32_t binding = 0U;
    b->BindImage(1, binding++, { input });
    // used to derive camera matrices
    b->BindBuffer(1, binding++, { cameraUboHandle_ });
    cmdList.UpdateDescriptorSet(b->GetDescriptorSetHandle(1), b->GetDescriptorSetLayoutBindingResources(1));

    RenderPass renderPass = CreateRenderPass(p.p.renderPassDesc.attachmentHandles[0]);
    const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);

    const float fEffectWidth = viewportDesc.width;
    const float fEffectHeight = viewportDesc.height;
    const Math::Vec4 texSizeInvTexSize { fEffectWidth, fEffectHeight, 1.0f / fEffectWidth, 1.0f / fEffectHeight };
    const Math::UVec2 size { static_cast<uint32_t>(fEffectWidth), static_cast<uint32_t>(fEffectHeight) };

    BlurShaderPushConstant pcV {};

    // pcV.view = d.view;
    pcV.dir[0U].x = d.time;
    pcV.dir[0U].y = settings.windSpeed;
    pcV.dir[0U].z = settings.density;
    pcV.dir[0U].w = settings.altitude;

    pcV.dir[1U].x = settings.ambientColor[0U];
    pcV.dir[1U].y = settings.ambientColor[1U];
    pcV.dir[1U].z = settings.ambientColor[2U];
    pcV.dir[1U].w = settings.windDir[0U];

    pcV.dir[2U].x = settings.sunColor[0U];
    pcV.dir[2U].y = settings.sunColor[1U];
    pcV.dir[2U].z = settings.sunColor[2U];
    pcV.dir[2U].w = settings.windDir[1U];

    pcV.dir[3U].w = settings.windDir[2U];

    const BlurParams<BlurShaderPushConstant, ExplicitPass> vBlur { s, *b, { pcV }, p };
    RenderFullscreenTriangle(0, cmdList, vBlur);
}

void RenderNodeCameraWeather::ExecuteGenerateClouds(
    IRenderCommandList& cmdList, const ExplicitPass& p, const GlobalFrameData& d)
{
    const auto& settings = settings_;

    GraphicsShaderData& s = cloudVolumeShaderData_;
    auto& b = cloudVolumeBinder_;

    uint32_t binding = 0;
    // used to derive camera matrices
    b->BindBuffer(1, binding++, { cameraUboHandle_ });
    // weather map ( R = coverage %, G = precipation, B = cloud type)
    b->BindImage(1, binding++, { weatherMapTex_ });
    cmdList.UpdateDescriptorSet(b->GetDescriptorSetHandle(1), b->GetDescriptorSetLayoutBindingResources(1));

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "clouds-generate", DEBUG_MARKER_COLOR);

    BlurShaderPushConstant pcV {};

    pcV.dir[0U].x = d.time;
    pcV.dir[0U].y = settings.windSpeed;
    pcV.dir[0U].z = settings.density;
    pcV.dir[0U].w = settings.altitude;

    pcV.dir[1U].x = settings.ambientColor[0U];
    pcV.dir[1U].y = settings.ambientColor[1U];
    pcV.dir[1U].z = settings.ambientColor[2U];
    pcV.dir[1U].w = settings.windDir[0U];

    pcV.dir[2U].x = settings.sunColor[0U];
    pcV.dir[2U].y = settings.sunColor[1U];
    pcV.dir[2U].z = settings.sunColor[2U];
    pcV.dir[2U].w = settings.windDir[1U];

    pcV.dir[3U].w = settings.windDir[2U];

    const BlurParams<BlurShaderPushConstant, ExplicitPass> vBlur { s, *b, { pcV }, p };
    RenderFullscreenTriangle(0, cmdList, vBlur);
}

void RenderNodeCameraWeather::ExecuteClear(IRenderCommandList& cmdList, const ExplicitPass& p, const GlobalFrameData& d)
{
    RenderPass pass = p;
    cmdList.BeginRenderPass(pass.renderPassDesc, pass.subpassStartIndex, pass.subpassDesc);
    cmdList.EndRenderPass();
}

template<typename T, class U>
void RenderNodeCameraWeather::RenderFullscreenTriangle(
    int firstSet, IRenderCommandList& commandList, const BlurParams<T, U>& args)
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(args.data.p);
    const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(args.data.p);
    commandList.BeginRenderPass(args.data.p.renderPassDesc, args.data.p.subpassStartIndex, args.data.p.subpassDesc);
    commandList.BindPipeline(args.shader.pso);
    commandList.BindDescriptorSets(uint32_t(firstSet), args.binder.GetDescriptorSetHandles());
    commandList.SetDynamicStateViewport(viewportDesc);
    commandList.SetDynamicStateScissor(scissorDesc);
    commandList.PushConstantData(args.shader.pipelineLayoutData.pushConstant, args.pc);
    commandList.Draw(3u, 1u, 0u, 0u); // draw one triangle
    commandList.EndRenderPass();
}

void RenderNodeCameraWeather::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
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
    const auto camHandles = renderNodeSceneUtil_->GetSceneCameraImageHandles(
        *renderNodeContextMgr_, stores_.dataStoreNameScene, currentScene_.camera.name, currentScene_.camera);
    currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;

    const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight.GetLightCounts();
    currentScene_.hasShadow = ((lightCounts.dirShadow > 0) || (lightCounts.spotShadow > 0)) ? true : false;
    currentScene_.shadowTypes = dataStoreLight.GetShadowTypes();
    currentScene_.lightingFlags = dataStoreLight.GetLightingFlags();
}

void RenderNodeCameraWeather::GetSceneUniformBuffers(const string_view us)
{
    if (renderNodeSceneUtil_) {
        sceneBuffers_ = renderNodeSceneUtil_->GetSceneBufferHandles(*renderNodeContextMgr_, stores_.dataStoreNameScene);
    }

    string camName;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        camName = jsonInputs_.customCameraName;
    }
    if (renderNodeSceneUtil_) {
        cameraBuffers_ = renderNodeSceneUtil_->GetSceneCameraBufferHandles(
            *renderNodeContextMgr_, stores_.dataStoreNameScene, camName);
    }
}

void RenderNodeCameraWeather::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
}

// for plugin / factory interface
IRenderNode* RenderNodeCameraWeather::Create()
{
    return new RenderNodeCameraWeather();
}

void RenderNodeCameraWeather::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCameraWeather*>(instance);
}
CORE3D_END_NAMESPACE()
