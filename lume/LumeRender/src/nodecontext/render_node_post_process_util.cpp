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

#include "render_node_post_process_util.h"

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "datastore/render_data_store_pod.h"
#include "datastore/render_data_store_post_process.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_layout_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t UBO_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };
constexpr uint32_t MAX_POST_PROCESS_EFFECT_COUNT { 8 };
constexpr uint32_t POST_PROCESS_UBO_BYTE_SIZE { sizeof(GlobalPostProcessStruct) +
                                                sizeof(LocalPostProcessStruct) * MAX_POST_PROCESS_EFFECT_COUNT };

constexpr string_view INPUT_COLOR = "color";
constexpr string_view INPUT_DEPTH = "depth";
constexpr string_view INPUT_VELOCITY = "velocity";
constexpr string_view INPUT_HISTORY = "history";
constexpr string_view INPUT_HISTORY_NEXT = "history_next";

constexpr string_view COMBINED_SHADER_NAME = "rendershaders://shader/fullscreen_combined_post_process.shader";
constexpr string_view FXAA_SHADER_NAME = "rendershaders://shader/fullscreen_fxaa.shader";
constexpr string_view TAA_SHADER_NAME = "rendershaders://shader/fullscreen_taa.shader";
constexpr string_view DOF_BLUR_SHADER_NAME = "rendershaders://shader/depth_of_field_blur.shader";
constexpr string_view DOF_SHADER_NAME = "rendershaders://shader/depth_of_field.shader";
constexpr string_view COPY_SHADER_NAME = "rendershaders://shader/fullscreen_copy.shader";

RenderPass CreateRenderPass(const IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle input)
{
    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(input);
    RenderPass rp;
    rp.renderPassDesc.attachmentCount = 1u;
    rp.renderPassDesc.attachmentHandles[0u] = input;
    rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    rp.renderPassDesc.renderArea = { 0, 0, desc.width, desc.height };

    rp.renderPassDesc.subpassCount = 1u;
    rp.subpassDesc.colorAttachmentCount = 1u;
    rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
    rp.subpassStartIndex = 0u;
    return rp;
}

RenderHandleReference CreatePostProcessDataUniformBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    PLUGIN_STATIC_ASSERT(sizeof(LocalPostProcessStruct) == UBO_OFFSET_ALIGNMENT);
    return gpuResourceMgr.Create(
        handle, GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, POST_PROCESS_UBO_BYTE_SIZE });
}

void FillTmpImageDesc(GpuImageDesc& desc)
{
    desc.imageType = CORE_IMAGE_TYPE_2D;
    desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS |
                               EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
    desc.usageFlags =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    // don't want this multisampled even if the final output would be.
    desc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT;
    desc.layerCount = 1U;
    desc.mipCount = 1U;
}

inline bool IsValidHandle(const BindableImage& img)
{
    return RenderHandleUtil::IsValid(img.handle);
}
} // namespace

void RenderNodePostProcessUtil::Init(
    IRenderNodeContextManager& renderNodeContextMgr, const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    postProcessInfo_ = postProcessInfo;
    ParseRenderNodeInputs();
    UpdateImageData();

    deviceBackendType_ = renderNodeContextMgr_->GetRenderContext().GetDevice().GetBackendType();

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    samplers_.nearest =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_NEAREST_CLAMP);
    samplers_.linear =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
    samplers_.mipLinear =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP);
    ubos_.postProcess = CreatePostProcessDataUniformBuffer(gpuResourceMgr, ubos_.postProcess);

    InitCreateShaderResources();

    if ((!IsValidHandle(images_.input)) || (!IsValidHandle(images_.output))) {
        validInputs_ = false;
        PLUGIN_LOG_E("RN: %s, expects custom input and output image", renderNodeContextMgr_->GetName().data());
    }

    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_W("RenderNodeCombinedPostProcess: render data store configuration not set in render node graph");
    }
    if (jsonInputs_.renderDataStore.typeName != RenderDataStorePod::TYPE_NAME) {
        PLUGIN_LOG_E("RenderNodeCombinedPostProcess: render data store type name not supported (%s != %s)",
            jsonInputs_.renderDataStore.typeName.data(), RenderDataStorePod::TYPE_NAME);
        validInputs_ = false;
    }

    {
        vector<DescriptorCounts> descriptorCounts;
        descriptorCounts.reserve(16U);
        descriptorCounts.push_back(renderBloom_.GetDescriptorCounts());
        descriptorCounts.push_back(renderBlur_.GetDescriptorCounts());
        descriptorCounts.push_back(renderBloom_.GetDescriptorCounts());
        descriptorCounts.push_back(renderMotionBlur_.GetDescriptorCounts());
        descriptorCounts.push_back(renderCopy_.GetDescriptorCounts());
        descriptorCounts.push_back(renderCopyLayer_.GetDescriptorCounts());
        descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(combineData_.pipelineLayout));
        descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(fxaaData_.pipelineLayout));
        descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(taaData_.pipelineLayout));
        descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(dofBlurData_.pipelineLayout));
        descriptorCounts.push_back(renderNearBlur_.GetDescriptorCounts());
        descriptorCounts.push_back(renderFarBlur_.GetDescriptorCounts());
        descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(dofData_.pipelineLayout));
        descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(copyData_.pipelineLayout));
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(descriptorCounts);
    }
    // bloom does not do combine, and does not render to output with this combined post process
    ProcessPostProcessConfiguration();
    {
        const RenderBloom::BloomInfo bloomInfo { images_.input, {}, ubos_.postProcess.GetHandle(),
            ppConfig_.bloomConfiguration.useCompute };
        renderBloom_.Init(renderNodeContextMgr, bloomInfo);
    }
    {
        RenderBlur::BlurInfo blurInfo { images_.output, ubos_.postProcess.GetHandle(), false,
            CORE_BLUR_TYPE_DOWNSCALE_RGBA, CORE_BLUR_TYPE_RGBA_DOF };
        renderBlur_.Init(renderNodeContextMgr, blurInfo);
        renderNearBlur_.Init(renderNodeContextMgr, blurInfo);
        renderFarBlur_.Init(renderNodeContextMgr, blurInfo);
    }
    {
        RenderMotionBlur::MotionBlurInfo motionBlurInfo {};
        renderMotionBlur_.Init(renderNodeContextMgr, motionBlurInfo);
    }
    renderCopy_.Init(renderNodeContextMgr, {});
    if (deviceBackendType_ != DeviceBackendType::VULKAN) {
        // prepare for possible layer copy
        renderCopyLayer_.Init(renderNodeContextMgr, {});
    }

    InitCreateBinders();

    if ((!jsonInputs_.resources.customOutputImages.empty()) && (!inputResources_.customOutputImages.empty()) &&
        (RenderHandleUtil::IsValid(inputResources_.customOutputImages[0].handle))) {
        IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        shrMgr.RegisterRenderNodeOutput(
            jsonInputs_.resources.customOutputImages[0u].name, inputResources_.customOutputImages[0u].handle);
    }
}

void RenderNodePostProcessUtil::PreExecute(const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo)
{
    postProcessInfo_ = postProcessInfo;
    if (!validInputs_) {
        return;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    UpdateImageData();

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const GpuImageDesc inputDesc = gpuResourceMgr.GetImageDescriptor(images_.input.handle);
    const GpuImageDesc outputDesc = gpuResourceMgr.GetImageDescriptor(images_.output.handle);
    outputSize_ = { outputDesc.width, outputDesc.height };
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT) &&
        (!validInputsForTaa_)) {
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "_taa_depth_vel",
            "RENDER_VALIDATION: Default TAA post process needs output depth, velocity, and history.");
    }
    if ((ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DOF_BIT) &&
        (!validInputsForDof_)) {
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "_dof_depth",
            "RENDER_VALIDATION: Default DOF post process needs output depth.");
    }
    if ((ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_MOTION_BLUR_BIT) &&
        (!validInputsForMb_)) {
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "_mb_vel",
            "RENDER_VALIDATION: Default motion blur post process needs output (samplable) velocity.");
    }
#endif

    // process this frame's enabled post processes here
    // bloom might need target updates, no further processing is needed
    ProcessPostProcessConfiguration();

    // post process
    const bool fxaaEnabled =
        ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT;
    const bool motionBlurEnabled =
        validInputsForMb_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_MOTION_BLUR_BIT);
    const bool dofEnabled =
        validInputsForDof_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DOF_BIT);

    ti_.idx = 0;
    const bool basicImagesNeeded = fxaaEnabled || motionBlurEnabled || dofEnabled;
    const bool mipImageNeeded = dofEnabled;
    const auto nearMips = static_cast<uint32_t>(Math::ceilToInt(ppConfig_.dofConfiguration.nearBlur));
    const auto farMips = static_cast<uint32_t>(Math::ceilToInt(ppConfig_.dofConfiguration.farBlur));
    const bool mipCountChanged = (nearMips != ti_.mipCount[0U]) || (farMips != ti_.mipCount[1U]);
    const bool sizeChanged = (ti_.targetSize.x != static_cast<float>(outputSize_.x)) ||
                             (ti_.targetSize.y != static_cast<float>(outputSize_.y));
    if (sizeChanged) {
        ti_.targetSize.x = Math::max(1.0f, static_cast<float>(outputSize_.x));
        ti_.targetSize.y = Math::max(1.0f, static_cast<float>(outputSize_.y));
        ti_.targetSize.z = 1.f / ti_.targetSize.x;
        ti_.targetSize.w = 1.f / ti_.targetSize.y;
    }
    if (basicImagesNeeded) {
        if ((!ti_.images[0U]) || sizeChanged) {
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_I("RENDER_VALIDATION: post process temporary images re-created (size:%ux%u)", outputSize_.x,
                outputSize_.y);
#endif
            GpuImageDesc tmpDesc = outputDesc;
            FillTmpImageDesc(tmpDesc);
            ti_.images[0U] = gpuResourceMgr.Create(ti_.images[0U], tmpDesc);
            ti_.images[1U] = gpuResourceMgr.Create(ti_.images[1U], tmpDesc);
            ti_.imageCount = 2U;
        }
    } else {
        ti_.images[0U] = {};
        ti_.images[1U] = {};
    }
    if (mipImageNeeded) {
        if ((!ti_.mipImages[0U]) || sizeChanged || mipCountChanged) {
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_I("RENDER_VALIDATION: post process temporary mip image re-created (size:%ux%u)", outputSize_.x,
                outputSize_.y);
#endif
            GpuImageDesc tmpDesc = outputDesc;
            FillTmpImageDesc(tmpDesc);
            tmpDesc.mipCount = nearMips;
            ti_.mipImages[0] = gpuResourceMgr.Create(ti_.mipImages[0U], tmpDesc);
            tmpDesc.mipCount = farMips;
            ti_.mipImages[1] = gpuResourceMgr.Create(ti_.mipImages[1U], tmpDesc);
            ti_.mipImageCount = 2U;
            ti_.mipCount[0U] = nearMips;
            ti_.mipCount[1U] = farMips;
        }
    } else {
        ti_.mipImages[0U] = {};
        ti_.mipImages[1U] = {};
    }
    if ((!basicImagesNeeded) && (!mipImageNeeded)) {
        ti_ = {};
    }

    BindableImage postTaaBloomInput = images_.input;
    // prepare for possible layer copy
    if ((deviceBackendType_ != DeviceBackendType::VULKAN) &&
        (images_.input.layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)) {
        if (sizeChanged || (!ti_.layerCopyImage)) {
            GpuImageDesc tmpDesc = inputDesc; // NOTE: input desc
            FillTmpImageDesc(tmpDesc);
            ti_.layerCopyImage = gpuResourceMgr.Create(ti_.layerCopyImage, tmpDesc);
        }

        BindableImage layerCopyOutput;
        layerCopyOutput.handle = ti_.layerCopyImage.GetHandle();
        RenderCopy::CopyInfo layerCopyInfo;
        layerCopyInfo.input = images_.input;
        layerCopyInfo.output = layerCopyOutput;
        layerCopyInfo.copyType = RenderCopy::CopyType::LAYER_COPY;
        renderCopyLayer_.PreExecute(*renderNodeContextMgr_, layerCopyInfo);

        // postTaa bloom input
        postTaaBloomInput = layerCopyOutput;
    }

    if (validInputsForTaa_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT)) {
        postTaaBloomInput = images_.historyNext;
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT) {
        const RenderBloom::BloomInfo bloomInfo { postTaaBloomInput, {}, ubos_.postProcess.GetHandle(),
            ppConfig_.bloomConfiguration.useCompute };
        renderBloom_.PreExecute(*renderNodeContextMgr_, bloomInfo, ppConfig_);
    }

    // needs to evaluate what is the final effect, where we will use the final target
    // the final target (output) might a swapchain which cannot be sampled
    framePostProcessInOut_.clear();
    // after bloom or TAA
    BindableImage input = postTaaBloomInput;
    const bool postProcessNeeded = (ppConfig_.enableFlags != 0);
    const bool sameInputOutput = (images_.input.handle == images_.output.handle);
    // combine
    if (postProcessNeeded && (!sameInputOutput)) {
        const BindableImage output =
            (basicImagesNeeded || mipImageNeeded) ? GetIntermediateImage(input.handle) : images_.output;
        framePostProcessInOut_.push_back({ input, output });
        input = framePostProcessInOut_.back().output;
    }
    if (fxaaEnabled) {
        framePostProcessInOut_.push_back({ input, GetIntermediateImage(input.handle) });
        input = framePostProcessInOut_.back().output;
    }
    if (motionBlurEnabled) {
        framePostProcessInOut_.push_back({ input, GetIntermediateImage(input.handle) });
        input = framePostProcessInOut_.back().output;
    }
    if (dofEnabled) {
        framePostProcessInOut_.push_back({ input, GetIntermediateImage(input.handle) });
        input = framePostProcessInOut_.back().output;
    }
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT) {
        framePostProcessInOut_.push_back({ input, input });
        input = framePostProcessInOut_.back().output;
    }
    // finalize
    if (!framePostProcessInOut_.empty()) {
        framePostProcessInOut_.back().output = images_.output;
    }

    uint32_t ppIdx = 0U;
    if (postProcessNeeded && (!sameInputOutput)) {
        ppIdx++;
    }
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT) {
        ppIdx++;
    }
    if (motionBlurEnabled) {
        const auto& inOut = framePostProcessInOut_[ppIdx++];
        const RenderMotionBlur::MotionBlurInfo info { inOut.input.handle, inOut.output.handle, images_.velocity.handle,
            images_.depth.handle, ubos_.postProcess.GetHandle(),
            sizeof(GlobalPostProcessStruct) + PP_MB_IDX * UBO_OFFSET_ALIGNMENT, outputSize_ };
        renderMotionBlur_.PreExecute(*renderNodeContextMgr_, info, ppConfig_);
    }

    if (dofEnabled) {
        auto nearMip = GetMipImage(input.handle);
        RenderBlur::BlurInfo blurInfo { nearMip, ubos_.postProcess.GetHandle(), false, CORE_BLUR_TYPE_DOWNSCALE_RGBA,
            CORE_BLUR_TYPE_RGBA_DOF };
        renderNearBlur_.PreExecute(*renderNodeContextMgr_, blurInfo, ppConfig_);

        blurInfo.blurTarget = GetMipImage(nearMip.handle);
        renderFarBlur_.PreExecute(*renderNodeContextMgr_, blurInfo, ppConfig_);
        ppIdx++;
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT) {
        const auto& inOut = framePostProcessInOut_[ppIdx++];
        RenderBlur::BlurInfo blurInfo { inOut.output, ubos_.postProcess.GetHandle(), false };
        renderBlur_.PreExecute(*renderNodeContextMgr_, blurInfo, ppConfig_);
    }

    PLUGIN_ASSERT(ppIdx == framePostProcessInOut_.size());

    if ((!jsonInputs_.resources.customOutputImages.empty()) && (!inputResources_.customOutputImages.empty()) &&
        (RenderHandleUtil::IsValid(inputResources_.customOutputImages[0].handle))) {
        IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        shrMgr.RegisterRenderNodeOutput(
            jsonInputs_.resources.customOutputImages[0u].name, inputResources_.customOutputImages[0u].handle);
    }
}

void RenderNodePostProcessUtil::Execute(IRenderCommandList& cmdList)
{
    if (!validInputs_) {
        return;
    }
    UpdatePostProcessData(ppConfig_);

    // prepare for possible layer copy
    if ((deviceBackendType_ != DeviceBackendType::VULKAN) &&
        (images_.input.layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)) {
        renderCopyLayer_.Execute(*renderNodeContextMgr_, cmdList);
    }

    if (validInputsForTaa_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT)) {
        ExecuteTAA(cmdList, { images_.input, images_.historyNext });
    }

    // ppConfig_ is already up-to-date from PreExecuteFrame
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT) {
        renderBloom_.Execute(*renderNodeContextMgr_, cmdList, ppConfig_);
    }

    // post process
    const bool fxaaEnabled =
        ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT;
    const bool motionBlurEnabled =
        validInputsForMb_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_MOTION_BLUR_BIT);
    const bool dofEnabled =
        validInputsForDof_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DOF_BIT);

    // after bloom or TAA
    const bool postProcessNeeded = (ppConfig_.enableFlags != 0);
    const bool sameInputOutput = (images_.input.handle == images_.output.handle);

#if (RENDER_VALIDATION_ENABLED == 1)
    if (postProcessNeeded && sameInputOutput) {
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName().data(),
            "%s: combined post process shader not supported for same input/output ATM.",
            renderNodeContextMgr_->GetName().data());
    }
#endif
    uint32_t ppIdx = 0U;
    if (postProcessNeeded && (!sameInputOutput)) {
        ExecuteCombine(cmdList, framePostProcessInOut_[ppIdx++]);
    }

    if (fxaaEnabled) {
        ExecuteFXAA(cmdList, framePostProcessInOut_[ppIdx++]);
    }

    if (motionBlurEnabled) {
        const auto& inOut = framePostProcessInOut_[ppIdx++];
        RenderMotionBlur::MotionBlurInfo info { inOut.input.handle, inOut.output.handle, images_.velocity.handle,
            images_.depth.handle, ubos_.postProcess.GetHandle(),
            sizeof(GlobalPostProcessStruct) + PP_MB_IDX * UBO_OFFSET_ALIGNMENT, outputSize_ };
        renderMotionBlur_.Execute(*renderNodeContextMgr_, cmdList, info, ppConfig_);
    }
    if (dofEnabled) {
        const auto& inOut = framePostProcessInOut_[ppIdx++];
        ExecuteDof(cmdList, inOut);
    }

    // post blur
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT) {
        ppIdx++;
        // NOTE: add ppConfig
        renderBlur_.Execute(*renderNodeContextMgr_, cmdList, ppConfig_);
    }

    PLUGIN_ASSERT(ppIdx == framePostProcessInOut_.size());

    // if all flags are zero and output is different than input
    // we need to execute a copy
    if ((!postProcessNeeded) && (!sameInputOutput)) {
        ExecuteBlit(cmdList, { images_.input, images_.output });
    }
}

void RenderNodePostProcessUtil::ExecuteCombine(IRenderCommandList& cmdList, const InputOutput& inOut)
{
    const RenderHandle bloomImage =
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT)
            ? renderBloom_.GetFinalTarget()
            : aimg_.black;

    auto& effect = combineData_;
    const RenderPass renderPass = CreateRenderPass(renderNodeContextMgr_->GetGpuResourceManager(), inOut.output.handle);
    if (!RenderHandleUtil::IsValid(effect.pso)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(effect.shader);
        effect.pso = psoMgr.GetGraphicsPsoHandle(effect.shader, graphicsStateHandle, effect.pipelineLayout, {}, {},
            { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(effect.pso);

    {
        RenderHandle sets[2U] {};
        DescriptorSetLayoutBindingResources resources[2U] {};
        {
            const RenderHandle ubo = ubos_.postProcess.GetHandle();
            PLUGIN_ASSERT(binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_COMBINED_IDX]);
            auto& binder = *binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_COMBINED_IDX];
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding, ubo, 0);
            binder.BindBuffer(++binding, ubo, sizeof(GlobalPostProcessStruct) + PP_COMBINED_IDX * UBO_OFFSET_ALIGNMENT);
            sets[0U] = binder.GetDescriptorSetHandle();
            resources[0U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        {
            auto& binder = *binders_.combineBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            BindableImage mainInput = inOut.input;
            mainInput.samplerHandle = samplers_.linear;
            binder.BindImage(binding++, mainInput);
            binder.BindImage(binding++, bloomImage, samplers_.linear);
            binder.BindImage(binding++, images_.dirtMask, samplers_.linear);
            sets[1U] = binder.GetDescriptorSetHandle();
            resources[1U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        cmdList.UpdateDescriptorSets(sets, resources);
        cmdList.BindDescriptorSets(0u, sets);
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    if (effect.pipelineLayout.pushConstant.byteSize > 0) {
        const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
        cmdList.PushConstantData(effect.pipelineLayout.pushConstant, arrayviewU8(pc));
    }

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderNodePostProcessUtil::ExecuteFXAA(IRenderCommandList& cmdList, const InputOutput& inOut)
{
    auto renderPass = CreateRenderPass(renderNodeContextMgr_->GetGpuResourceManager(), inOut.output.handle);
    if (!RenderHandleUtil::IsValid(fxaaData_.pso)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(fxaaData_.shader);
        fxaaData_.pso = psoMgr.GetGraphicsPsoHandle(
            fxaaData_.shader, gfxHandle, fxaaData_.pl, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(fxaaData_.pso);

    {
        RenderHandle sets[2U] {};
        DescriptorSetLayoutBindingResources resources[2U] {};
        {
            const RenderHandle ubo = ubos_.postProcess.GetHandle();
            PLUGIN_ASSERT(binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_FXAA_IDX]);
            auto& binder = *binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_FXAA_IDX];
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding, ubo, 0u);
            binder.BindBuffer(++binding, ubo, sizeof(GlobalPostProcessStruct) + PP_FXAA_IDX * UBO_OFFSET_ALIGNMENT);
            sets[0U] = binder.GetDescriptorSetHandle();
            resources[0U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        {
            auto& binder = *binders_.fxaaBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindImage(binding, inOut.input);
            binder.BindSampler(++binding, samplers_.linear);
            sets[1U] = binder.GetDescriptorSetHandle();
            resources[1U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        cmdList.UpdateDescriptorSets(sets, resources);
        cmdList.BindDescriptorSets(0u, sets);
    }

    {
        const LocalPostProcessPushConstantStruct pc { ti_.targetSize,
            PostProcessConversionHelper::GetFactorFxaa(ppConfig_) };
        cmdList.PushConstantData(fxaaData_.pipelineLayout.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderNodePostProcessUtil::ExecuteTAA(IRenderCommandList& cmdList, const InputOutput& inOut)
{
    auto renderPass = CreateRenderPass(renderNodeContextMgr_->GetGpuResourceManager(), inOut.output.handle);
    if (!RenderHandleUtil::IsValid(taaData_.pso)) {
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(taaData_.shader);
        taaData_.pso = psoMgr.GetGraphicsPsoHandle(
            taaData_.shader, gfxHandle, taaData_.pl, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(taaData_.pso);

    {
        RenderHandle sets[2U] {};
        DescriptorSetLayoutBindingResources resources[2U] {};
        {
            const RenderHandle ubo = ubos_.postProcess.GetHandle();
            PLUGIN_ASSERT(binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_TAA_IDX]);
            auto& binder = *binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_TAA_IDX];
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding++, ubo, 0u);
            binder.BindBuffer(binding++, ubo, sizeof(GlobalPostProcessStruct) + PP_TAA_IDX * UBO_OFFSET_ALIGNMENT);
            sets[0U] = binder.GetDescriptorSetHandle();
            resources[0U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        {
            auto& binder = *binders_.taaBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindImage(binding, images_.depth.handle);
            binder.BindImage(++binding, inOut.input.handle);
            binder.BindImage(++binding, images_.velocity.handle);
            binder.BindImage(++binding, images_.history.handle);
            binder.BindSampler(++binding, samplers_.linear);
            sets[1U] = binder.GetDescriptorSetHandle();
            resources[1U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        cmdList.UpdateDescriptorSets(sets, resources);
        cmdList.BindDescriptorSets(0U, sets);
    }

    if (taaData_.pipelineLayout.pushConstant.byteSize > 0) {
        const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
            PostProcessConversionHelper::GetFactorTaa(ppConfig_) };
        cmdList.PushConstantData(taaData_.pipelineLayout.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderNodePostProcessUtil::ExecuteDofBlur(IRenderCommandList& cmdList, const InputOutput& inOut)
{
    RenderPass rp;
    {
        const GpuImageDesc desc =
            renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(ti_.mipImages[0U].GetHandle());
        rp.renderPassDesc.attachmentCount = 2u;
        rp.renderPassDesc.attachmentHandles[0u] = ti_.mipImages[0U].GetHandle();
        rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
        rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        rp.renderPassDesc.attachmentHandles[1u] = ti_.mipImages[1U].GetHandle();
        rp.renderPassDesc.attachments[1u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
        rp.renderPassDesc.attachments[1u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        rp.renderPassDesc.renderArea = { 0, 0, desc.width, desc.height };

        rp.renderPassDesc.subpassCount = 1u;
        rp.subpassDesc.colorAttachmentCount = 2u;
        rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
        rp.subpassDesc.colorAttachmentIndices[1u] = 1u;
        rp.subpassStartIndex = 0u;
    }
    if (!RenderHandleUtil::IsValid(dofBlurData_.pso)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(dofBlurData_.shader);
        dofBlurData_.pso = psoMgr.GetGraphicsPsoHandle(
            dofBlurData_.shader, gfxHandle, dofBlurData_.pl, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }
    cmdList.BeginRenderPass(rp.renderPassDesc, rp.subpassStartIndex, rp.subpassDesc);
    cmdList.BindPipeline(dofBlurData_.pso);

    {
        RenderHandle sets[2U] {};
        DescriptorSetLayoutBindingResources resources[2U] {};
        {
            // NOTE: this updated descriptor set is used in dof
            const RenderHandle ubo = ubos_.postProcess.GetHandle();
            PLUGIN_ASSERT(binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_DOF_IDX]);
            auto& binder = *binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_DOF_IDX];
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding, ubo, 0u);
            binder.BindBuffer(++binding, ubo, sizeof(GlobalPostProcessStruct) + PP_DOF_IDX * UBO_OFFSET_ALIGNMENT);
            sets[0U] = binder.GetDescriptorSetHandle();
            resources[0U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        {
            auto& binder = *binders_.dofBlurBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            BindableImage input = inOut.input;
            input.samplerHandle = samplers_.mipLinear;
            binder.BindImage(binding, input);
            BindableImage depth = images_.depth;
            depth.samplerHandle = samplers_.nearest;
            binder.BindImage(++binding, depth);
            sets[1U] = binder.GetDescriptorSetHandle();
            resources[1U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        cmdList.UpdateDescriptorSets(sets, resources);
        cmdList.BindDescriptorSets(0U, sets);
    }

    {
        const float fWidth = static_cast<float>(rp.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(rp.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
        cmdList.PushConstantData(dofBlurData_.pipelineLayout.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(rp));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(rp));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
    const auto maxMipLevel = ppConfig_.blurConfiguration.maxMipLevel;
    ppConfig_.blurConfiguration.maxMipLevel = static_cast<uint32_t>(Math::round(ppConfig_.dofConfiguration.nearBlur));
    renderNearBlur_.Execute(*renderNodeContextMgr_, cmdList, ppConfig_);
    ppConfig_.blurConfiguration.maxMipLevel = static_cast<uint32_t>(Math::round(ppConfig_.dofConfiguration.farBlur));
    renderFarBlur_.Execute(*renderNodeContextMgr_, cmdList, ppConfig_);
    ppConfig_.blurConfiguration.maxMipLevel = maxMipLevel;
}

void RenderNodePostProcessUtil::ExecuteDof(IRenderCommandList& cmdList, const InputOutput& inOut)
{
    // NOTE: updates set 0 for dof
    ExecuteDofBlur(cmdList, inOut);

    auto renderPass = CreateRenderPass(renderNodeContextMgr_->GetGpuResourceManager(), inOut.output.handle);
    if (!RenderHandleUtil::IsValid(dofData_.pso)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(dofData_.shader);
        dofData_.pso = psoMgr.GetGraphicsPsoHandle(
            dofData_.shader, gfxHandle, dofData_.pl, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(dofData_.pso);

    RenderHandle sets[2u] {};
    {
        // NOTE: descriptor set updated by DOF blur
        PLUGIN_ASSERT(binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_DOF_IDX]);
        auto& binder = *binders_.globalSet0[POST_PROCESS_UBO_INDICES::PP_DOF_IDX];
        sets[0u] = binder.GetDescriptorSetHandle();
    }
    {
        auto& binder = *binders_.dofBinder;
        binder.ClearBindings();
        uint32_t binding = 0u;
        BindableImage input = inOut.input;
        input.samplerHandle = samplers_.mipLinear;
        binder.BindImage(binding, input);
        binder.BindImage(++binding, ti_.mipImages[0U].GetHandle(), samplers_.mipLinear);
        binder.BindImage(++binding, ti_.mipImages[1U].GetHandle(), samplers_.mipLinear);
        BindableImage depth = images_.depth;
        depth.samplerHandle = samplers_.nearest;
        binder.BindImage(++binding, depth);

        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        sets[1u] = binder.GetDescriptorSetHandle();
    }
    cmdList.BindDescriptorSets(0u, sets);

    {
        const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
        cmdList.PushConstantData(dofData_.pipelineLayout.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderNodePostProcessUtil::ExecuteBlit(IRenderCommandList& cmdList, const InputOutput& inOut)
{
    // extra blit from input to ouput
    if (RenderHandleUtil::IsGpuImage(inOut.input.handle) && RenderHandleUtil::IsGpuImage(inOut.output.handle)) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName().data(),
            "RENDER_PERFORMANCE_VALIDATION: Extra blit from input to output (RN: %s)",
            renderNodeContextMgr_->GetName().data());
#endif
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

        RenderPass renderPass = CreateRenderPass(gpuResourceMgr, inOut.output.handle);
        auto& effect = copyData_;
        if (!RenderHandleUtil::IsValid(effect.pso)) {
            auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
            const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
            const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(effect.shader);
            effect.pso = psoMgr.GetGraphicsPsoHandle(effect.shader, graphicsStateHandle, effect.pipelineLayout, {}, {},
                { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(effect.pso);

        {
            auto& binder = *binders_.copyBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindSampler(binding, samplers_.linear);
            binder.BindImage(++binding, inOut.input);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0u, binder.GetDescriptorSetHandle());
        }

        // dynamic state
        const ViewportDesc viewportDesc = renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }
}

void RenderNodePostProcessUtil::UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const RenderPostProcessConfiguration rppc =
        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(postProcessConfiguration);
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    if (auto* const data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.postProcess.GetHandle())); data) {
        const auto* dataEnd = data + POST_PROCESS_UBO_BYTE_SIZE;
        if (!CloneData(data, size_t(dataEnd - data), &rppc, sizeof(RenderPostProcessConfiguration))) {
            PLUGIN_LOG_E("post process ubo copying failed.");
        }
        {
            auto* localData = data + (sizeof(RenderPostProcessConfiguration) + PP_TAA_IDX * UBO_OFFSET_ALIGNMENT);
            auto factor = PostProcessConversionHelper::GetFactorTaa(postProcessConfiguration);
            CloneData(localData, size_t(dataEnd - localData), &factor, sizeof(factor));
        }
        {
            auto* localData = data + (sizeof(RenderPostProcessConfiguration) + PP_BLOOM_IDX * UBO_OFFSET_ALIGNMENT);
            auto factor = PostProcessConversionHelper::GetFactorBloom(postProcessConfiguration);
            CloneData(localData, size_t(dataEnd - localData), &factor, sizeof(factor));
        }
        {
            auto* localData = data + (sizeof(RenderPostProcessConfiguration) + PP_BLUR_IDX * UBO_OFFSET_ALIGNMENT);
            auto factor = PostProcessConversionHelper::GetFactorBlur(postProcessConfiguration);
            CloneData(localData, size_t(dataEnd - localData), &factor, sizeof(factor));
        }
        {
            auto* localData = data + (sizeof(RenderPostProcessConfiguration) + PP_FXAA_IDX * UBO_OFFSET_ALIGNMENT);
            auto factor = PostProcessConversionHelper::GetFactorFxaa(postProcessConfiguration);
            CloneData(localData, size_t(dataEnd - localData), &factor, sizeof(factor));
        }
        {
            auto* localData = data + (sizeof(RenderPostProcessConfiguration) + PP_DOF_IDX * UBO_OFFSET_ALIGNMENT);
            auto factors = PostProcessConversionHelper::GetFactorDof(postProcessConfiguration);
            CloneData(localData, size_t(dataEnd - localData), &factors, sizeof(factors));
            localData += sizeof(factors);
            factors = PostProcessConversionHelper::GetFactorDof2(postProcessConfiguration);
            CloneData(localData, size_t(dataEnd - localData), &factors, sizeof(factors));
        }
        {
            auto* localData = data + (sizeof(RenderPostProcessConfiguration) + PP_MB_IDX * UBO_OFFSET_ALIGNMENT);
            auto factor = PostProcessConversionHelper::GetFactorMotionBlur(postProcessConfiguration);
            CloneData(localData, size_t(dataEnd - localData), &factor, sizeof(factor));
        }

        gpuResourceMgr.UnmapBuffer(ubos_.postProcess.GetHandle());
    }
}

void RenderNodePostProcessUtil::ProcessPostProcessConfiguration()
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName); ds) {
            if (jsonInputs_.renderDataStore.typeName == RenderDataStorePod::TYPE_NAME) {
                auto const dataStore = static_cast<IRenderDataStorePod const*>(ds);
                auto const dataView = dataStore->Get(jsonInputs_.renderDataStore.configurationName);
                if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                    ppConfig_ = *((const PostProcessConfiguration*)dataView.data());
                    if (RenderHandleUtil::IsValid(ppConfig_.bloomConfiguration.dirtMaskImage)) {
                        images_.dirtMask = ppConfig_.bloomConfiguration.dirtMaskImage;
                    } else {
                        images_.dirtMask = aimg_.black;
                    }
                }
            }
        }
    }
}

void RenderNodePostProcessUtil::InitCreateShaderResources()
{
    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    combineData_ = {};
    copyData_ = {};
    fxaaData_ = {};
    taaData_ = {};
    dofData_ = {};
    dofBlurData_ = {};

    combineData_.shader = shaderMgr.GetShaderHandle(COMBINED_SHADER_NAME);
    combineData_.pl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(combineData_.shader);
    if (!RenderHandleUtil::IsValid(combineData_.pl)) {
        combineData_.pl = shaderMgr.GetReflectionPipelineLayoutHandle(combineData_.shader);
    }
    combineData_.pipelineLayout = shaderMgr.GetPipelineLayout(combineData_.pl);
    copyData_.shader = shaderMgr.GetShaderHandle(COPY_SHADER_NAME);
    copyData_.pl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(copyData_.shader);
    if (!RenderHandleUtil::IsValid(copyData_.pl)) {
        copyData_.pl = shaderMgr.GetReflectionPipelineLayoutHandle(copyData_.shader);
    }
    copyData_.pipelineLayout = shaderMgr.GetPipelineLayout(copyData_.pl);

    fxaaData_.shader = shaderMgr.GetShaderHandle(FXAA_SHADER_NAME);
    fxaaData_.pl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(fxaaData_.shader);
    if (!RenderHandleUtil::IsValid(fxaaData_.pl)) {
        fxaaData_.pl = shaderMgr.GetReflectionPipelineLayoutHandle(fxaaData_.shader);
    }
    fxaaData_.pipelineLayout = shaderMgr.GetPipelineLayout(fxaaData_.pl);
    taaData_.shader = shaderMgr.GetShaderHandle(TAA_SHADER_NAME);
    taaData_.pl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(taaData_.shader);
    if (!RenderHandleUtil::IsValid(taaData_.pl)) {
        taaData_.pl = shaderMgr.GetReflectionPipelineLayoutHandle(taaData_.shader);
    }
    taaData_.pipelineLayout = shaderMgr.GetPipelineLayout(taaData_.pl);

    dofData_.shader = shaderMgr.GetShaderHandle(DOF_SHADER_NAME);
    dofData_.pl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(dofData_.shader);
    if (!RenderHandleUtil::IsValid(dofData_.pl)) {
        dofData_.pl = shaderMgr.GetReflectionPipelineLayoutHandle(dofData_.shader);
    }
    dofData_.pipelineLayout = shaderMgr.GetPipelineLayout(dofData_.pl);

    dofBlurData_.shader = shaderMgr.GetShaderHandle(DOF_BLUR_SHADER_NAME);
    dofBlurData_.pl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(dofBlurData_.shader);
    if (!RenderHandleUtil::IsValid(dofBlurData_.pl)) {
        dofBlurData_.pl = shaderMgr.GetReflectionPipelineLayoutHandle(dofBlurData_.shader);
    }
    dofBlurData_.pipelineLayout = shaderMgr.GetPipelineLayout(dofBlurData_.pl);
}

void RenderNodePostProcessUtil::InitCreateBinders()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    {
        constexpr uint32_t globalSetIdx = 0u;
        constexpr uint32_t localSetIdx = 1u;

        for (uint32_t idx = 0; idx < POST_PROCESS_UBO_INDICES::PP_COUNT_IDX; ++idx) {
            binders_.globalSet0[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetMgr.CreateDescriptorSet(globalSetIdx, combineData_.pipelineLayout),
                combineData_.pipelineLayout.descriptorSetLayouts[globalSetIdx].bindings);
        }

        binders_.combineBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, combineData_.pipelineLayout),
            combineData_.pipelineLayout.descriptorSetLayouts[localSetIdx].bindings);
        binders_.fxaaBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, fxaaData_.pipelineLayout),
            fxaaData_.pipelineLayout.descriptorSetLayouts[localSetIdx].bindings);
        binders_.taaBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, taaData_.pipelineLayout),
            taaData_.pipelineLayout.descriptorSetLayouts[localSetIdx].bindings);
        binders_.dofBlurBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, dofBlurData_.pipelineLayout),
            dofBlurData_.pipelineLayout.descriptorSetLayouts[localSetIdx].bindings);
        binders_.dofBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, dofData_.pipelineLayout),
            dofData_.pipelineLayout.descriptorSetLayouts[localSetIdx].bindings);
    }
    binders_.copyBinder =
        descriptorSetMgr.CreateDescriptorSetBinder(descriptorSetMgr.CreateDescriptorSet(0u, copyData_.pipelineLayout),
            copyData_.pipelineLayout.descriptorSetLayouts[0u].bindings);
}

void RenderNodePostProcessUtil::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);

    // process custom inputs
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customInputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customInputImages[idx];
        if (ref.usageName == INPUT_COLOR) {
            jsonInputs_.colorIndex = idx;
        } else if (ref.usageName == INPUT_DEPTH) {
            jsonInputs_.depthIndex = idx;
        } else if (ref.usageName == INPUT_VELOCITY) {
            jsonInputs_.velocityIndex = idx;
        } else if (ref.usageName == INPUT_HISTORY) {
            jsonInputs_.historyIndex = idx;
        } else if (ref.usageName == INPUT_HISTORY_NEXT) {
            jsonInputs_.historyNextIndex = idx;
        }
    }
}

void RenderNodePostProcessUtil::UpdateImageData()
{
    if ((!RenderHandleUtil::IsValid(aimg_.black)) || (!RenderHandleUtil::IsValid(aimg_.white))) {
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        aimg_.black = gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
        aimg_.white = gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE_WHITE);
    }

    if (postProcessInfo_.parseRenderNodeInputs) {
        if (inputResources_.customInputImages.empty() || inputResources_.customOutputImages.empty()) {
            return; // early out
        }

        images_.input.handle = inputResources_.customInputImages[0].handle;
        images_.output.handle = inputResources_.customOutputImages[0].handle;
        if (jsonInputs_.depthIndex < inputResources_.customInputImages.size()) {
            images_.depth.handle = inputResources_.customInputImages[jsonInputs_.depthIndex].handle;
        }
        if (jsonInputs_.velocityIndex < inputResources_.customInputImages.size()) {
            images_.velocity.handle = inputResources_.customInputImages[jsonInputs_.velocityIndex].handle;
        }
        if (jsonInputs_.historyIndex < inputResources_.customInputImages.size()) {
            images_.history.handle = inputResources_.customInputImages[jsonInputs_.historyIndex].handle;
        }
        if (jsonInputs_.historyNextIndex < inputResources_.customInputImages.size()) {
            images_.historyNext.handle = inputResources_.customInputImages[jsonInputs_.historyNextIndex].handle;
        }
    } else {
        images_.input = postProcessInfo_.imageData.input;
        images_.output = postProcessInfo_.imageData.output;
        images_.depth = postProcessInfo_.imageData.depth;
        images_.velocity = postProcessInfo_.imageData.velocity;
        images_.history = postProcessInfo_.imageData.history;
        images_.historyNext = postProcessInfo_.imageData.historyNext;
    }

    validInputsForTaa_ = false;
    validInputsForDof_ = false;
    validInputsForMb_ = false;
    bool validDepth = false;
    bool validHistory = false;
    bool validVelocity = false;
    if (IsValidHandle(images_.depth) && (images_.depth.handle != aimg_.white)) {
        const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const GpuImageDesc& desc = gpuResourceMgr.GetImageDescriptor(images_.depth.handle);
        if (desc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT) {
            validDepth = true;
        }
    } else {
        images_.depth.handle = aimg_.white; // default depth
    }
    if (IsValidHandle(images_.velocity) && (images_.velocity.handle != aimg_.black)) {
        const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const GpuImageDesc& desc = gpuResourceMgr.GetImageDescriptor(images_.velocity.handle);
        if (desc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT) {
            validVelocity = true;
        }
    } else {
        images_.velocity.handle = aimg_.black; // default velocity
    }
    if (IsValidHandle(images_.history) && IsValidHandle(images_.historyNext)) {
        const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const GpuImageDesc& velDesc = gpuResourceMgr.GetImageDescriptor(images_.velocity.handle);
        if (velDesc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT) {
            validHistory = true;
        }
    }
    if (!RenderHandleUtil::IsValid(images_.dirtMask)) {
        images_.dirtMask = aimg_.black; // default dirt mask
    }

    validInputsForDof_ = validDepth;
    validInputsForTaa_ = validDepth && validHistory; // TAA can be used without velocities
    validInputsForMb_ = validVelocity;
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void RenderNodePostProcessUtilImpl::Init(
    IRenderNodeContextManager& renderNodeContextMgr, const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo)
{
    rn_.Init(renderNodeContextMgr, postProcessInfo);
}

void RenderNodePostProcessUtilImpl::PreExecute(const IRenderNodePostProcessUtil::PostProcessInfo& postProcessInfo)
{
    rn_.PreExecute(postProcessInfo);
}

void RenderNodePostProcessUtilImpl::Execute(IRenderCommandList& cmdList)
{
    rn_.Execute(cmdList);
}

const IInterface* RenderNodePostProcessUtilImpl::GetInterface(const Uid& uid) const
{
    if ((uid == IRenderNodePostProcessUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* RenderNodePostProcessUtilImpl::GetInterface(const Uid& uid)
{
    if ((uid == IRenderNodePostProcessUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderNodePostProcessUtilImpl::Ref()
{
    refCount_++;
}

void RenderNodePostProcessUtilImpl::Unref()
{
    if (--refCount_ == 0) {
        delete this;
    }
}
RENDER_END_NAMESPACE()
