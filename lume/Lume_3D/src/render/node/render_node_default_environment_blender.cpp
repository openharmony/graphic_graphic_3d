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

#include "render_node_default_environment_blender.h"

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

#include "render/default_constants.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace RENDER_NS;

CORE3D_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t MAX_MIP_COUNT { 16U };
constexpr string_view CORE_DEFAULT_GPU_IMAGE_BLACK { "CORE_DEFAULT_GPU_IMAGE" };
constexpr string_view DEFAULT_CUBEMAP_BLENDER_SHADER { "3dshaders://shader/core3d_dm_camera_cubemap_blender.shader" };

constexpr GpuImageDesc CUBEMAP_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_CUBE,
    BASE_FORMAT_B10G11R11_UFLOAT_PACK32, CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT, CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    CORE_IMAGE_CREATE_CUBE_COMPATIBLE_BIT, CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 256U, 256U, 1U, 9U, 6U,
    CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };

RenderNodeDefaultEnvironmentBlender::DefaultImagesAndSamplers GetDefaultImagesAndSamplers(
    const IRenderNodeGpuResourceManager& gpuResourceMgr)
{
    RenderNodeDefaultEnvironmentBlender::DefaultImagesAndSamplers dias;
    dias.cubemapSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    dias.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    dias.nearestHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    dias.linearMipHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");
    dias.skyBoxRadianceCubemapHandle =
        gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP);
    return dias;
}

RenderHandleReference CreateRadianceCubemap(IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view name)
{
    return gpuResourceMgr.Create(name, CUBEMAP_DEFAULT_DESC);
}
} // namespace

void RenderNodeDefaultEnvironmentBlender::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    valid_ = true;

    ParseRenderNodeInputs();
    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    UpdateImageData();
}

void RenderNodeDefaultEnvironmentBlender::PreExecuteFrame()
{
    hasBlendEnvironments_ = false;

    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    valid_ = (dataStoreScene && dataStoreCamera);
    if (!valid_) {
        return;
    }

    if (!dataStoreCamera->HasBlendEnvironments()) {
        // need to clear targets
        envTargetData_ = {};
        return; // early out
    }

    hasBlendEnvironments_ = true;

    // create possible targets
    const auto& renderEnvironments = dataStoreCamera->GetEnvironments();
    for (uint32_t idx = 0U; idx < static_cast<uint32_t>(renderEnvironments.size()); ++idx) {
        const auto& currEnv = renderEnvironments[idx];
        if (currEnv.multiEnvCount != 0U) {
            if (!currEnv.radianceCubemap) {
                bool targetFound = false;
                // create target if not available and mark for frame usage
                for (auto& targetRef : envTargetData_) {
                    if (targetRef.id == currEnv.id) {
                        targetFound = true;
                        break;
                    }
                }
                if (!targetFound) {
                    const string name = string(stores_.dataStoreNameScene) +
                                        DefaultMaterialSceneConstants::ENVIRONMENT_RADIANCE_CUBEMAP_PREFIX_NAME +
                                        to_hex(currEnv.id);
                    envTargetData_.push_back({ currEnv.id,
                        CreateRadianceCubemap(renderNodeContextMgr_->GetGpuResourceManager(), name.c_str()), true });
                }
            }
        }
    }
}

IRenderNode::ExecuteFlags RenderNodeDefaultEnvironmentBlender::GetExecuteFlags() const
{
    if (valid_ && hasBlendEnvironments_) {
        return 0U;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderNodeDefaultEnvironmentBlender::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    if (valid_ && dataStoreCamera) {
        CORE_ASSERT(dataStoreCamera->HasBlendEnvironments());
        UpdateImageData();
        InitializeShaderData();

        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "3DEnvCubemapBlender", DefaultDebugConstants::DEFAULT_DEBUG_COLOR);
        const auto renderEnvironments = dataStoreCamera->GetEnvironments();

        environmentUbo_ = renderNodeContextMgr_->GetGpuResourceManager().GetBufferHandle(
            stores_.dataStoreNameScene.c_str() + DefaultMaterialSceneConstants::SCENE_ENVIRONMENT_DATA_BUFFER_NAME);
        if (RenderHandleUtil::IsValid(environmentUbo_)) {
            for (uint32_t idx = 0U; idx < static_cast<uint32_t>(renderEnvironments.size()); ++idx) {
                const auto& ref = renderEnvironments[idx];
                if (ref.multiEnvCount != 0U) {
                    ExecuteSingleEnvironment(cmdList, renderEnvironments, idx);
                }
            }
        }
    }

    // clear targets which are not used this frame
    for (auto iter = envTargetData_.begin(); iter != envTargetData_.end();) {
        if (iter->usedThisFrame) {
            ++iter;
        } else {
            envTargetData_.erase(iter);
        }
    }
}

namespace {
constexpr ImageResourceBarrier SRC_UNDEFINED { 0, CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT, CORE_IMAGE_LAYOUT_UNDEFINED };
constexpr ImageResourceBarrier COL_ATTACHMENT { CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
constexpr ImageResourceBarrier SHDR_READ { CORE_ACCESS_SHADER_READ_BIT, CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
} // namespace

RenderHandle RenderNodeDefaultEnvironmentBlender::GetEnvironmentTargetHandle(
    const RenderCamera::Environment& environment)
{
    RenderHandle target;
    if (environment.radianceCubemap) {
        // there's a target available which we should use
        target = environment.radianceCubemap.GetHandle();
#if (CORE3D_VALIDATION_ENABLED == 1)
        const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const GpuImageDesc& desc = gpuResourceMgr.GetImageDescriptor(target);
        if ((desc.usageFlags & ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0U) {
            CORE_LOG_ONCE_W("target_usage_cubemap_color_attachment_" + to_string(target.id),
                "CORE3D_VALIDATION: Environment blending radiance cubemap target has invalid flags");
        }
#endif
    } else {
        // mark for frame usage
        for (auto& targetRef : envTargetData_) {
            if (targetRef.id == environment.id) {
                targetRef.usedThisFrame = true;
                target = targetRef.handle.GetHandle();
                break;
            }
        }
        if (!RenderHandleUtil::IsValid(target)) {
            CORE_LOG_W("Target handle issues with cubemap blender");
        }
    }
    return target;
}

void RenderNodeDefaultEnvironmentBlender::FillBlendEnvironments(
    const array_view<const RenderCamera::Environment> environments, const uint32_t envIdx,
    array_view<RenderHandle>& blendEnvs) const
{
    const auto& env = environments[envIdx];
    // fetch all the radiance cubemaps
    uint32_t mvEnvIdx = 0U;
    for (uint32_t idx = 0; idx < env.multiEnvCount; ++idx) {
        if (mvEnvIdx >= blendEnvs.size()) {
            break;
        }
        if (idx < env.multiEnvCount) {
            for (const auto& mvEnvRef : environments) {
                if (mvEnvRef.id == env.multiEnvIds[idx]) {
                    blendEnvs[mvEnvIdx++] = (mvEnvRef.radianceCubemap)
                                                ? mvEnvRef.radianceCubemap.GetHandle()
                                                : defaultImagesAndSamplers_.skyBoxRadianceCubemapHandle;
                    break;
                }
            }
        }
    }
}

void RenderNodeDefaultEnvironmentBlender::ExecuteSingleEnvironment(
    IRenderCommandList& cmdList, const array_view<const RenderCamera::Environment> environments, const uint32_t envIdx)
{
    const auto& envRef = environments[envIdx];
    CORE_ASSERT(envRef.multiEnvCount > 0U);

    const RenderHandle target = GetEnvironmentTargetHandle(envRef);
    if (!RenderHandleUtil::IsValid(target)) {
        return;
    }

    RenderHandle blendEnvironments[2U] { defaultImagesAndSamplers_.skyBoxRadianceCubemapHandle,
        defaultImagesAndSamplers_.skyBoxRadianceCubemapHandle };
    array_view<RenderHandle> beView { blendEnvironments, countof(blendEnvironments) };
    FillBlendEnvironments(environments, envIdx, beView);

    const GpuImageDesc desc = renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(target);
    Math::UVec2 currSize = { desc.width, desc.height };

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1U;
    renderPass.renderPassDesc.attachmentHandles[0U] = target;
    renderPass.renderPassDesc.attachments[0U].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0U].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachments[0U].layer = 0U;
    renderPass.renderPassDesc.renderArea = { 0U, 0U, currSize.x, currSize.y };
    renderPass.renderPassDesc.subpassCount = 1U;
    renderPass.subpassDesc.viewMask = 0x3f;
    renderPass.subpassDesc.colorAttachmentCount = 1U;
    renderPass.subpassDesc.colorAttachmentIndices[0U] = 0U;

    auto& sets = allEnvSets_[envIdx];
    for (uint32_t mipIdx = 0; mipIdx < desc.mipCount; ++mipIdx) {
        // handle all descriptor set updates
        UpdateSet0(cmdList, beView, envIdx, mipIdx);
        const auto& bindings = sets.localSets[mipIdx]->GetDescriptorSetLayoutBindingResources();
        cmdList.UpdateDescriptorSet(sets.localSets[mipIdx]->GetDescriptorSetHandle(), bindings);
    }

    cmdList.BeginDisableAutomaticBarrierPoints();

    // change all the layouts accordingly
    {
        ImageSubresourceRange imageSubresourceRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
        cmdList.CustomImageBarrier(target, SRC_UNDEFINED, COL_ATTACHMENT, imageSubresourceRange);
        cmdList.AddCustomBarrierPoint();
    }

    for (uint32_t mipIdx = 0; mipIdx < desc.mipCount; ++mipIdx) {
        renderPass.renderPassDesc.attachments[0U].mipLevel = mipIdx;
        if (mipIdx != 0U) {
            currSize.x = Math::max(1U, currSize.x / 2U);
            currSize.y = Math::max(1U, currSize.y / 2U);
        }
        renderPass.renderPassDesc.renderArea = { 0U, 0U, currSize.x, currSize.y };

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

        cmdList.BindPipeline(psoHandle_);
        // bind all sets
        cmdList.BindDescriptorSet(0U, sets.localSets[mipIdx]->GetDescriptorSetHandle());

        // dynamic state
        const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);

        if (pipelineLayout_.pushConstant.byteSize > 0U) {
            // mipIdx as lodlevel
            Math::UVec4 indices { envIdx, mipIdx, 0U, 0U };
            cmdList.PushConstantData(pipelineLayout_.pushConstant, arrayviewU8(indices));
        }

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }

    // change all the layouts accordingly
    {
        ImageSubresourceRange imageSubresourceRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0,
            PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
        cmdList.CustomImageBarrier(target, COL_ATTACHMENT, SHDR_READ, imageSubresourceRange);
        cmdList.AddCustomBarrierPoint();
    }

    cmdList.EndDisableAutomaticBarrierPoints();
}

void RenderNodeDefaultEnvironmentBlender::UpdateSet0(IRenderCommandList& cmdList,
    const array_view<const RenderHandle> blendImages, const uint32_t envIdx, const uint32_t mipIdx)
{
    // bind all radiance cubemaps
    const auto& sets = allEnvSets_[envIdx];
    auto& binder = *sets.localSets[mipIdx];
    CORE_ASSERT(sets.localSets[mipIdx]);
    CORE_ASSERT(blendImages.size() == 2U);

    const uint32_t maxCount = Math::min(DefaultMaterialCameraConstants::MAX_CAMERA_MULTI_ENVIRONMENT_COUNT, 2U);
    uint32_t binding = 0U;
    binder.BindBuffer(binding++, { environmentUbo_ });
    BindableImage bi;
    // additional cubemaps from first index
    for (uint32_t resIdx = 0U; resIdx < maxCount; ++resIdx) {
        // filled with valid handles
        bi.handle = blendImages[resIdx];
        bi.samplerHandle = defaultImagesAndSamplers_.cubemapSamplerHandle;
        binder.BindImage(binding++, bi, {});
    }
}

void RenderNodeDefaultEnvironmentBlender::InitializeShaderData()
{
    if (!hasInitializedShaderData_) {
        hasInitializedShaderData_ = true;

        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        defaultImagesAndSamplers_ = GetDefaultImagesAndSamplers(gpuResourceMgr);
        const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandleType handleType = RenderHandleUtil::GetHandleType(shader_);
        const RenderHandle plHandle = shaderMgr.GetReflectionPipelineLayoutHandle(shader_);
        pipelineLayout_ = shaderMgr.GetPipelineLayout(plHandle);
        if (handleType == RenderHandleType::SHADER_STATE_OBJECT) {
            const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(shader_);
            psoHandle_ = renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(
                shader_, graphicsState, pipelineLayout_, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        } else {
            CORE_LOG_E("RN:%s needs a valid shader handle", renderNodeContextMgr_->GetName().data());
        }
        InitCreateBinders();
    }
}

void RenderNodeDefaultEnvironmentBlender::InitCreateBinders()
{
    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    {
        DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
        for (uint32_t idx = 0; idx < dc.counts.size(); ++idx) {
            dc.counts[idx].count *= MAX_MIP_COUNT * CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT;
        }
        descriptorSetMgr.ResetAndReserve(dc);
    }

    allEnvSets_.clear();
    allEnvSets_.resize(CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT);

    for (auto& setsRef : allEnvSets_) {
        setsRef.localSets.resize(MAX_MIP_COUNT);
        const auto& bindings = pipelineLayout_.descriptorSetLayouts[0U].bindings;
        for (uint32_t idx = 0; idx < MAX_MIP_COUNT; ++idx) {
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            setsRef.localSets[idx] = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
    }

    if ((!RenderHandleUtil::IsValid(shader_)) || (!RenderHandleUtil::IsValid(psoHandle_))) {
        valid_ = false;
    }
}

void RenderNodeDefaultEnvironmentBlender::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    if (!shaderName.empty()) {
        shader_ = shaderMgr.GetShaderHandle(shaderName);
    } else {
        shader_ = shaderMgr.GetShaderHandle(DEFAULT_CUBEMAP_BLENDER_SHADER);
    }
}

void RenderNodeDefaultEnvironmentBlender::UpdateImageData()
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBlackImage)) {
        builtInVariables_.defBlackImage = gpuResourceMgr.GetImageHandle(CORE_DEFAULT_GPU_IMAGE_BLACK);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defSampler)) {
        builtInVariables_.defSampler = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeDefaultEnvironmentBlender::Create()
{
    return new RenderNodeDefaultEnvironmentBlender();
}

void RenderNodeDefaultEnvironmentBlender::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultEnvironmentBlender*>(instance);
}
CORE3D_END_NAMESPACE()
