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

#include "render_node_combined_post_process.h"

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
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
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_layout_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t UBO_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };
constexpr uint32_t MAX_POST_PROCESS_EFFECT_COUNT { 8 };

constexpr string_view INPUT_COLOR = "color";
constexpr string_view INPUT_DEPTH = "depth";
constexpr string_view INPUT_VELOCITY = "velocity";
constexpr string_view INPUT_HISTORY = "history";
constexpr string_view INPUT_HISTORY_NEXT = "history_next";

constexpr string_view COMBINED_SHADER_NAME = "rendershaders://shader/fullscreen_combined_post_process.shader";
constexpr string_view FXAA_SHADER_NAME = "rendershaders://shader/fullscreen_fxaa.shader";
constexpr string_view TAA_SHADER_NAME = "rendershaders://shader/fullscreen_taa.shader";
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
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == UBO_OFFSET_ALIGNMENT);
    PLUGIN_STATIC_ASSERT(sizeof(LocalPostProcessStruct) == UBO_OFFSET_ALIGNMENT);
    return gpuResourceMgr.Create(handle,
        GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, UBO_OFFSET_ALIGNMENT * MAX_POST_PROCESS_EFFECT_COUNT });
}
} // namespace

void RenderNodeCombinedPostProcess::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();
    UpdateImageData();

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    sampler_ = gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
    ubos_.postProcess = CreatePostProcessDataUniformBuffer(gpuResourceMgr, ubos_.postProcess);

    InitCreateShaderResources();

    if ((!RenderHandleUtil::IsValid(images_.input)) || (!RenderHandleUtil::IsValid(images_.output))) {
        validInputs_ = false;
        PLUGIN_LOG_E("RN: %s, expects custom input and output image", renderNodeContextMgr_->GetName().data());
    }

    if (!renderDataStore_.dataStoreName.empty()) {
        configurationNames_.renderDataStoreName = renderDataStore_.dataStoreName;
        configurationNames_.postProcessConfigurationName = renderDataStore_.configurationName;
    } else {
        PLUGIN_LOG_V("RenderNodeCombinedPostProcess: render data store configuration not set in render node graph");
    }

    {
        DescriptorCounts dc = renderBloom_.GetDescriptorCounts();
        const DescriptorCounts blurDc = renderBlur_.GetDescriptorCounts();
        const DescriptorCounts plDc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
        const DescriptorCounts fxaaDc = renderNodeUtil.GetDescriptorCounts(fxaaData_.pipelineLayout);
        const DescriptorCounts taaDc = renderNodeUtil.GetDescriptorCounts(taaData_.pipelineLayout);
        const DescriptorCounts blitDc = renderNodeUtil.GetDescriptorCounts(copyPipelineLayout_);
        dc.counts.reserve(dc.counts.size() + blurDc.counts.size() + plDc.counts.size() + fxaaDc.counts.size() +
                          taaDc.counts.size() + blitDc.counts.size());
        auto EmplaceDescriptorSets = [](const auto& inputDc, auto& dc) {
            for (const auto& ref : inputDc.counts) {
                dc.counts.emplace_back(ref);
            }
        };
        EmplaceDescriptorSets(blurDc, dc);
        EmplaceDescriptorSets(plDc, dc);
        EmplaceDescriptorSets(fxaaDc, dc);
        EmplaceDescriptorSets(taaDc, dc);
        EmplaceDescriptorSets(blitDc, dc);
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
    }
    // bloom does not do combine, and does not render to output with this combined post process
    ProcessPostProcessConfiguration();
    const RenderBloom::BloomInfo bloomInfo { images_.input, {}, ubos_.postProcess.GetHandle(),
        ppConfig_.bloomConfiguration.useCompute };
    renderBloom_.Init(renderNodeContextMgr, bloomInfo);
    RenderBlur::BlurInfo blurInfo { images_.output, ubos_.postProcess.GetHandle(), false };
    renderBlur_.Init(renderNodeContextMgr, blurInfo);

    InitCreateBinders();
}

void RenderNodeCombinedPostProcess::PreExecuteFrame()
{
    if (!validInputs_) {
        return;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    UpdateImageData();

    if (validInputsForTaa_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_TAA_BIT)) {
        images_.input = images_.historyNext;
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if ((ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_TAA_BIT) &&
        (!validInputsForTaa_)) {
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "_taa_depth_vel",
            "RENDER_VALIDATION: Default TAA post process needs output depth, velocity, and history");
    }
#endif

    // process this frame's enabled post processes here
    // bloom might need target updates, no further processing is needed
    ProcessPostProcessConfiguration();
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_BLOOM_BIT) {
        const RenderBloom::BloomInfo bloomInfo { images_.input, {}, ubos_.postProcess.GetHandle(),
            ppConfig_.bloomConfiguration.useCompute };
        renderBloom_.PreExecute(*renderNodeContextMgr_, bloomInfo, ppConfig_);
    }
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_BLUR_BIT) {
        RenderBlur::BlurInfo blurInfo { images_.output, ubos_.postProcess.GetHandle(), false };
        renderBlur_.PreExecute(*renderNodeContextMgr_, blurInfo, ppConfig_);
    }
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_FXAA_BIT) {
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        auto fxaaInputDesc = gpuResourceMgr.GetImageDescriptor(images_.output);
        if ((fxaaTargetSize_.x != fxaaInputDesc.width) || (fxaaTargetSize_.y != fxaaInputDesc.height)) {
            fxaaInputDesc.usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                       ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
            // don't want this multisampled even if the final output would be.
            fxaaInputDesc.sampleCountFlags = SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT;
            intermediateImage_ = gpuResourceMgr.Create(intermediateImage_, fxaaInputDesc);
            fxaaTargetSize_.x = Math::max(1.0f, static_cast<float>(fxaaInputDesc.width));
            fxaaTargetSize_.y = Math::max(1.0f, static_cast<float>(fxaaInputDesc.height));
            fxaaTargetSize_.z = 1.f / fxaaTargetSize_.x;
            fxaaTargetSize_.w = 1.f / fxaaTargetSize_.y;
        }
    }
}

void RenderNodeCombinedPostProcess::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!validInputs_) {
        return;
    }
    binders_.globalSetIndex = 0;

    UpdatePostProcessData(ppConfig_);

    if (validInputsForTaa_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_TAA_BIT)) {
        ExecuteTAA(cmdList);
    }

    // ppConfig_ is already up-to-date from PreExecuteFrame
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_BLOOM_BIT) {
        renderBloom_.Execute(*renderNodeContextMgr_, cmdList, ppConfig_);
    }

    // post process
    const bool fxaaEnabled =
        ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_FXAA_BIT;
    const RenderHandle postprocOutput = fxaaEnabled ? intermediateImage_.GetHandle() : images_.output;
    const bool postProcessNeeded = (ppConfig_.enableFlags != 0);
    // NOTE: same input/output not currently supported
    const bool sameInputOutput = (images_.input == postprocOutput);
#if (RENDER_VALIDATION_ENABLED == 1)
    if (postProcessNeeded && sameInputOutput) {
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName().data(),
            "%s: combined post process shader not supported for same input/output ATM.",
            renderNodeContextMgr_->GetName().data());
    }
#endif
    if (postProcessNeeded && (!sameInputOutput)) {
        ExecuteCombine(cmdList);
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_FXAA_BIT) {
        ExecuteFXAA(cmdList);
    }

    // post blur
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_BLUR_BIT) {
        // NOTE: add ppConfig
        renderBlur_.Execute(*renderNodeContextMgr_, cmdList, ppConfig_);
    }

    // if all flags are zero and output is different than input
    // we need to execute a copy
    if ((!postProcessNeeded) && (!sameInputOutput)) {
        ExecuteBlit(cmdList);
    }
}

void RenderNodeCombinedPostProcess::ExecuteCombine(IRenderCommandList& cmdList)
{
    const bool fxaaEnabled =
        ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_FXAA_BIT;
    const RenderHandle postprocOutput = fxaaEnabled ? intermediateImage_.GetHandle() : images_.output;

    const bool postProcessNeeded = (ppConfig_.enableFlags != 0);
    // NOTE: same input/output not currently supported
    const bool sameInputOutput = (images_.input == postprocOutput);

    if (postProcessNeeded && (!sameInputOutput)) {
        const RenderHandle bloomImage =
            (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_BLOOM_BIT)
                ? renderBloom_.GetFinalTarget()
                : images_.black;

        renderPass_ = CreateRenderPass(renderNodeContextMgr_->GetGpuResourceManager(), postprocOutput);

        CheckForPsoSpecilization();

        cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);
        cmdList.BindPipeline(psoHandle_);

        RenderHandle sets[2u] {};
        {
            const RenderHandle ubo = ubos_.postProcess.GetHandle();
            auto& binder = *binders_.globalSet0[binders_.globalSetIndex++];
            sets[0u] = binder.GetDescriptorSetHandle();
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding, ubo, 0);
            binder.BindBuffer(++binding, ubo, ubos_.combinedIndex * UBO_OFFSET_ALIGNMENT);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        }
        {
            auto& binder = *binders_.mainBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindImage(binding++, images_.input, sampler_);
            binder.BindImage(binding++, bloomImage, sampler_);
            binder.BindImage(binding++, images_.dirtMask, sampler_);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            sets[1u] = binder.GetDescriptorSetHandle();
        }
        cmdList.BindDescriptorSets(0u, sets);

        // dynamic state
        cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass_));
        cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass_));

        if (pipelineLayout_.pushConstant.byteSize > 0) {
            const float fWidth = static_cast<float>(renderPass_.renderPassDesc.renderArea.extentWidth);
            const float fHeight = static_cast<float>(renderPass_.renderPassDesc.renderArea.extentHeight);
            const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
            cmdList.PushConstant(pipelineLayout_.pushConstant, arrayviewU8(pc).data());
        }

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }
}

namespace {
inline constexpr float GetFxaaSharpness(const FxaaConfiguration::Sharpness sharpness)
{
    if (sharpness == FxaaConfiguration::Sharpness::SOFT) {
        return 2.0f;
    } else if (sharpness == FxaaConfiguration::Sharpness::MEDIUM) {
        return 4.0f;
    } else { // SHARP
        return 8.0f;
    }
}

inline constexpr Math::Vec2 GetFxaaThreshold(const FxaaConfiguration::Quality quality)
{
    if (quality == FxaaConfiguration::Quality::LOW) {
        return { 0.333f, 0.0833f };
    } else if (quality == FxaaConfiguration::Quality::HIGH) {
        return { 0.063f, 0.0625f };
    } else { // MEDIUM
        return { 0.125f, 0.0312f };
    }
}
} // namespace

void RenderNodeCombinedPostProcess::ExecuteFXAA(IRenderCommandList& cmdList)
{
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_FXAA_BIT) {
        auto renderPass = CreateRenderPass(renderNodeContextMgr_->GetGpuResourceManager(), images_.output);
        if (!RenderHandleUtil::IsValid(fxaaData_.pso)) {
            auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
            const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
            const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(fxaaData_.shader);
            constexpr DynamicStateFlags dynamicStateFlags = CORE_DYNAMIC_STATE_VIEWPORT | CORE_DYNAMIC_STATE_SCISSOR;
            fxaaData_.pso =
                psoMgr.GetGraphicsPsoHandle(fxaaData_.shader, gfxHandle, fxaaData_.pl, {}, {}, dynamicStateFlags);
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(fxaaData_.pso);

        RenderHandle sets[2u] {};
        {
            const RenderHandle ubo = ubos_.postProcess.GetHandle();
            auto& binder = *binders_.globalSet0[binders_.globalSetIndex++];
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding, ubo, 0u);
            binder.BindBuffer(++binding, ubo, ubos_.fxaaIndex * UBO_OFFSET_ALIGNMENT);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            sets[0u] = binder.GetDescriptorSetHandle();
        }
        {
            auto& binder = *binders_.fxaaBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindImage(binding, intermediateImage_.GetHandle());
            binder.BindSampler(++binding, sampler_);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            sets[1u] = binder.GetDescriptorSetHandle();
        }
        cmdList.BindDescriptorSets(0u, sets);

        {
            const float edgeSharpness = GetFxaaSharpness(ppConfig_.fxaaConfiguration.sharpness);
            const Math::Vec2 edgeThreshold = GetFxaaThreshold(ppConfig_.fxaaConfiguration.quality);

            const LocalPostProcessPushConstantStruct pc { fxaaTargetSize_,
                Math::Vec4 { edgeSharpness, edgeThreshold.x, edgeThreshold.y, 0.f } };
            cmdList.PushConstant(fxaaData_.pipelineLayout.pushConstant, arrayviewU8(pc).data());
        }

        // dynamic state
        cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
        cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }
}

namespace {
inline constexpr Math::Vec4 GetTaaFactor(const TaaConfiguration& config)
{
    return { static_cast<float>(config.quality), static_cast<float>(config.sharpness), 0.0f, 0.0f };
}
} // namespace

void RenderNodeCombinedPostProcess::ExecuteTAA(IRenderCommandList& cmdList)
{
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagbits::ENABLE_TAA_BIT) {
        auto renderPass = CreateRenderPass(renderNodeContextMgr_->GetGpuResourceManager(), images_.historyNext);
        if (!RenderHandleUtil::IsValid(taaData_.pso)) {
            const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
            auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
            const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(taaData_.shader);
            constexpr DynamicStateFlags dynamicStateFlags = CORE_DYNAMIC_STATE_VIEWPORT | CORE_DYNAMIC_STATE_SCISSOR;
            taaData_.pso =
                psoMgr.GetGraphicsPsoHandle(taaData_.shader, gfxHandle, taaData_.pl, {}, {}, dynamicStateFlags);
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(taaData_.pso);

        RenderHandle sets[2u] {};
        {
            const RenderHandle ubo = ubos_.postProcess.GetHandle();
            auto& binder = *binders_.globalSet0[binders_.globalSetIndex++];
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding++, ubo, 0u);
            binder.BindBuffer(binding++, ubo, ubos_.taaIndex * UBO_OFFSET_ALIGNMENT);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            sets[0u] = binder.GetDescriptorSetHandle();
        }
        {
            auto& binder = *binders_.taaBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindImage(binding, images_.depth);
            binder.BindImage(++binding, images_.globalInput);
            binder.BindImage(++binding, images_.velocity);
            binder.BindImage(++binding, images_.history);
            binder.BindSampler(++binding, sampler_);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            sets[1u] = binder.GetDescriptorSetHandle();
        }
        cmdList.BindDescriptorSets(0u, sets);

        if (taaData_.pipelineLayout.pushConstant.byteSize > 0) {
            const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
            const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
            const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                GetTaaFactor(ppConfig_.taaConfiguration) };
            cmdList.PushConstant(taaData_.pipelineLayout.pushConstant, arrayviewU8(pc).data());
        }

        // dynamic state
        cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
        cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }
}

void RenderNodeCombinedPostProcess::ExecuteBlit(IRenderCommandList& cmdList)
{
    // extra blit from input to ouput
    if (RenderHandleUtil::IsGpuImage(images_.input) && RenderHandleUtil::IsGpuImage(images_.output)) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName().data(),
            "RENDER_PERFORMANCE_VALIDATION: Extra blit from input to output (RN: %s)",
            renderNodeContextMgr_->GetName().data());
#endif
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

        auto renderPass = CreateRenderPass(gpuResourceMgr, images_.output);
        if (!RenderHandleUtil::IsValid(copyPsoHandle_)) {
            auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
            const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
            const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(copyShader_);
            const DynamicStateFlags dynamicStateFlags =
                DynamicStateFlagBits::CORE_DYNAMIC_STATE_VIEWPORT | DynamicStateFlagBits::CORE_DYNAMIC_STATE_SCISSOR;
            copyPsoHandle_ = psoMgr.GetGraphicsPsoHandle(
                copyShader_, graphicsStateHandle, copyPipelineLayout_, {}, {}, dynamicStateFlags);
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(copyPsoHandle_);

        {
            auto& binder = *binders_.copyBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindSampler(binding, sampler_);
            binder.BindImage(++binding, images_.input);
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

void RenderNodeCombinedPostProcess::CheckForPsoSpecilization()
{
    if (!RenderHandleUtil::IsValid(psoHandle_)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(combinedShader_);
        const DynamicStateFlags dynamicStateFlags =
            DynamicStateFlagBits::CORE_DYNAMIC_STATE_VIEWPORT | DynamicStateFlagBits::CORE_DYNAMIC_STATE_SCISSOR;
        psoHandle_ = psoMgr.GetGraphicsPsoHandle(
            combinedShader_, graphicsStateHandle, pipelineLayout_, {}, {}, dynamicStateFlags);
    }
}

void RenderNodeCombinedPostProcess::UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const RenderPostProcessConfiguration rppc =
        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(postProcessConfiguration);
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.postProcess.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(RenderPostProcessConfiguration);
        if (!CloneData(data, size_t(dataEnd - data), &rppc, sizeof(RenderPostProcessConfiguration))) {
            PLUGIN_LOG_E("post process ubo copying failed.");
        }
        gpuResourceMgr.UnmapBuffer(ubos_.postProcess.GetHandle());
    }
}

void RenderNodeCombinedPostProcess::ProcessPostProcessConfiguration()
{
    if (!configurationNames_.renderDataStoreName.empty()) {
        auto& dataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        auto const dataStore = static_cast<IRenderDataStorePod const*>(
            dataStoreMgr.GetRenderDataStore(configurationNames_.renderDataStoreName));
        if (dataStore) {
            auto const dataView = dataStore->Get(configurationNames_.postProcessConfigurationName);
            if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                ppConfig_ = *((const PostProcessConfiguration*)dataView.data());
                if (ppConfig_.bloomConfiguration.dirtMaskImage) {
                    images_.dirtMask = ppConfig_.bloomConfiguration.dirtMaskImage.GetHandle();
                } else {
                    images_.dirtMask = images_.black;
                }
            }
        }
    }
}

void RenderNodeCombinedPostProcess::InitCreateShaderResources()
{
    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();

    psoHandle_ = {};
    fxaaData_.pso = {};
    taaData_.pso = {};

    combinedShader_ = shaderMgr.GetShaderHandle(COMBINED_SHADER_NAME);

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

    copyShader_ = shaderMgr.GetShaderHandle(COPY_SHADER_NAME);
    copyPipelineLayout_ = renderNodeUtil.CreatePipelineLayout(copyShader_);

    pipelineLayout_ = renderNodeUtil.CreatePipelineLayout(combinedShader_);
}

void RenderNodeCombinedPostProcess::InitCreateBinders()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    {
        constexpr uint32_t globalSetIdx = 0u;
        constexpr uint32_t localSetIdx = 1u;

        for (uint32_t idx = 0; idx < AllBinders::GLOBAL_SET0_COUNT; ++idx) {
            binders_.globalSet0[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetMgr.CreateDescriptorSet(globalSetIdx, pipelineLayout_),
                pipelineLayout_.descriptorSetLayouts[globalSetIdx].bindings);
        }

        binders_.mainBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, pipelineLayout_),
            pipelineLayout_.descriptorSetLayouts[localSetIdx].bindings);
        binders_.fxaaBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, fxaaData_.pipelineLayout),
            fxaaData_.pipelineLayout.descriptorSetLayouts[localSetIdx].bindings);
        binders_.taaBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, taaData_.pipelineLayout),
            taaData_.pipelineLayout.descriptorSetLayouts[localSetIdx].bindings);
    }
    binders_.copyBinder =
        descriptorSetMgr.CreateDescriptorSetBinder(descriptorSetMgr.CreateDescriptorSet(0u, copyPipelineLayout_),
            copyPipelineLayout_.descriptorSetLayouts[0u].bindings);
}

void RenderNodeCombinedPostProcess::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    renderDataStore_ = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

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

void RenderNodeCombinedPostProcess::UpdateImageData()
{
    if (!RenderHandleUtil::IsValid(images_.black)) {
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        images_.black = gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
        images_.white = gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE_WHITE);
    }
    PLUGIN_ASSERT(inputResources_.customInputImages.size() > 0);
    images_.globalInput = inputResources_.customInputImages[0].handle;
    images_.globalOutput = inputResources_.customOutputImages[0].handle;
    if (jsonInputs_.depthIndex < inputResources_.customInputImages.size()) {
        images_.depth = inputResources_.customInputImages[jsonInputs_.depthIndex].handle;
    }
    if (jsonInputs_.velocityIndex < inputResources_.customInputImages.size()) {
        images_.velocity = inputResources_.customInputImages[jsonInputs_.velocityIndex].handle;
    }
    if (jsonInputs_.historyIndex < inputResources_.customInputImages.size()) {
        images_.history = inputResources_.customInputImages[jsonInputs_.historyIndex].handle;
    }
    if (jsonInputs_.historyNextIndex < inputResources_.customInputImages.size()) {
        images_.historyNext = inputResources_.customInputImages[jsonInputs_.historyNextIndex].handle;
    }

    if (!RenderHandleUtil::IsValid(images_.depth)) {
        images_.depth = images_.white;
    }
    if (!RenderHandleUtil::IsValid(images_.velocity)) {
        images_.velocity = images_.black;
    }
    if (RenderHandleUtil::IsValid(images_.history) && RenderHandleUtil::IsValid(images_.historyNext)) {
        const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const GpuImageDesc& depthDesc = gpuResourceMgr.GetImageDescriptor(images_.depth);
        const GpuImageDesc& velDesc = gpuResourceMgr.GetImageDescriptor(images_.velocity);
        if ((depthDesc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT) &&
            (velDesc.usageFlags & CORE_IMAGE_USAGE_SAMPLED_BIT)) {
            validInputsForTaa_ = true;
        } else {
            validInputsForTaa_ = false;
        }
    } else {
        validInputsForTaa_ = false;
    }
    if (!RenderHandleUtil::IsValid(images_.velocity)) {
        images_.velocity = images_.black;
    }
    if (!RenderHandleUtil::IsValid(images_.dirtMask)) {
        images_.dirtMask = images_.black;
    }

    images_.input = images_.globalInput;
    images_.output = images_.globalOutput;
}

// for plugin / factory interface
IRenderNode* RenderNodeCombinedPostProcess::Create()
{
    return new RenderNodeCombinedPostProcess();
}

void RenderNodeCombinedPostProcess::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeCombinedPostProcess*>(instance);
}
RENDER_END_NAMESPACE()
