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

#include "render_bloom.h"

#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
}

void RenderBloom::Init(IRenderNodeContextManager& renderNodeContextMgr, const BloomInfo& bloomInfo)
{
    bloomInfo_ = bloomInfo;

    // NOTE: target counts etc. should probably be resized based on configuration
    CreatePsos(renderNodeContextMgr);

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    samplerHandle_ = gpuResourceMgr.Create(samplerHandle_,
        GpuSamplerDesc {
            Filter::CORE_FILTER_LINEAR,                                  // magFilter
            Filter::CORE_FILTER_LINEAR,                                  // minFilter
            Filter::CORE_FILTER_LINEAR,                                  // mipMapMode
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
        });
}

void RenderBloom::PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const BloomInfo& bloomInfo,
    const PostProcessConfiguration& ppConfig)
{
    bloomInfo_ = bloomInfo;

    const GpuImageDesc& imgDesc =
        renderNodeContextMgr.GetGpuResourceManager().GetImageDescriptor(bloomInfo_.input.handle);
    uint32_t sizeDenom = 1u;
    if (ppConfig.bloomConfiguration.bloomQualityType == BloomConfiguration::QUALITY_TYPE_LOW) {
        sizeDenom = 2u;
    }
    CreateTargets(renderNodeContextMgr, Math::UVec2(imgDesc.width, imgDesc.height) / sizeDenom);
}

void RenderBloom::Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
    const PostProcessConfiguration& ppConfig)
{
    bloomEnabled_ = false;
    BloomConfiguration bloomConfiguration;
    if (ppConfig.enableFlags & PostProcessConfiguration::ENABLE_BLOOM_BIT) {
        bloomConfiguration.thresholdHard = ppConfig.bloomConfiguration.thresholdHard;
        bloomConfiguration.thresholdSoft = ppConfig.bloomConfiguration.thresholdSoft;
        bloomConfiguration.amountCoefficient = ppConfig.bloomConfiguration.amountCoefficient;
        bloomConfiguration.dirtMaskCoefficient = ppConfig.bloomConfiguration.dirtMaskCoefficient;

        bloomEnabled_ = true;
    }

    const auto bloomQualityType = ppConfig.bloomConfiguration.bloomQualityType;
    PLUGIN_ASSERT(bloomQualityType < CORE_BLOOM_QUALITY_COUNT);
    if (bloomInfo_.useCompute) {
        psos_.downscale = psos_.downscaleHandlesCompute[bloomQualityType].regular;
        psos_.downscaleAndThreshold = psos_.downscaleHandlesCompute[bloomQualityType].threshold;
    } else {
        psos_.downscale = psos_.downscaleHandles[bloomQualityType].regular;
        psos_.downscaleAndThreshold = psos_.downscaleHandles[bloomQualityType].threshold;
    }

    if (!bloomEnabled_) {
        bloomConfiguration.amountCoefficient = 0.0f;
    }

    bloomParameters_ = Math::Vec4(
        // .x = thresholdHard, luma values below this won't bloom
        bloomConfiguration.thresholdHard,
        // .y = thresholdSoft, luma values from this value to hard threshold will reduce bloom input from 1.0 -> 0.0
        // i.e. this creates softer threshold for bloom
        bloomConfiguration.thresholdSoft,
        // .z = amountCoefficient, will multiply the colors from the bloom textures when combined with original color
        // target
        bloomConfiguration.amountCoefficient,
        // .w = -will multiply the dirt mask effect
        bloomConfiguration.dirtMaskCoefficient);

    const bool validBinders = binders_.globalSet0.get() != nullptr;
    if (validBinders) {
        if (bloomInfo_.useCompute) {
            ComputeBloom(renderNodeContextMgr, cmdList);
        } else {
            GraphicsBloom(renderNodeContextMgr, cmdList);
        }
    }
}

DescriptorCounts RenderBloom::GetDescriptorCounts() const
{
    // NOTE: when added support for various bloom target counts, might need to be calculated for max
    return DescriptorCounts { {
        { CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32u },
        { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32u },
        { CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32u },
        { CORE_DESCRIPTOR_TYPE_SAMPLER, 24u },
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u },
    } };
}

RenderHandle RenderBloom::GetFinalTarget() const
{
    if (RenderHandleUtil::IsValid(bloomInfo_.output.handle)) {
        return bloomInfo_.output.handle;
    } else {
        // output tex1 on compute and tex2 on graphics
        return bloomInfo_.useCompute ? (targets_.tex1[0u].GetHandle()) : (targets_.tex2[0u].GetHandle());
    }
}

void RenderBloom::UpdateGlobalSet(IRenderCommandList& cmdList)
{
    auto& binder = *binders_.globalSet0;
    binder.ClearBindings();
    uint32_t binding = 0u;
    binder.BindBuffer(binding++, bloomInfo_.globalUbo, 0);
    binder.BindBuffer(binding++, bloomInfo_.globalUbo, sizeof(GlobalPostProcessStruct));
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

void RenderBloom::ComputeBloom(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList)
{
    constexpr PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT,
        sizeof(LocalPostProcessPushConstantStruct) };

    UpdateGlobalSet(cmdList);
    if (bloomEnabled_) {
        ComputeDownscaleAndThreshold(pc, cmdList);
        ComputeDownscale(pc, cmdList);
        ComputeUpscale(pc, cmdList);
    }
    // needs to be done even when bloom is disabled if node is in use
    if (RenderHandleUtil::IsValid(bloomInfo_.output.handle)) {
        ComputeCombine(pc, cmdList);
    }
}

void RenderBloom::ComputeDownscaleAndThreshold(const PushConstant& pc, IRenderCommandList& cmdList)
{
    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();
    {
        auto& binder = *binders_.downscaleAndThreshold;
        sets[1u] = binder.GetDescriptorSetHandle();
        binder.ClearBindings();

        uint32_t binding = 0;
        binder.BindImage(binding++, { targets_.tex1[0].GetHandle() });
        binder.BindImage(binding++, { bloomInfo_.input });
        binder.BindSampler(binding++, { samplerHandle_.GetHandle() });

        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }

    cmdList.BindPipeline(psos_.downscaleAndThreshold);
    const ShaderThreadGroup tgs = psos_.downscaleAndThresholdTGS;

    // bind all sets
    cmdList.BindDescriptorSets(0, sets);

    const auto targetSize = targets_.tex1Size[0];

    LocalPostProcessPushConstantStruct uPc;
    uPc.factor = bloomParameters_;
    uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
        1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));

    cmdList.PushConstantData(pc, arrayviewU8(uPc));

    cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
}

void RenderBloom::ComputeDownscale(const PushConstant& pc, IRenderCommandList& cmdList)
{
    cmdList.BindPipeline(psos_.downscale);
    const ShaderThreadGroup tgs = psos_.downscaleTGS;

    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();
    for (size_t i = 1; i < targets_.tex1.size(); ++i) {
        {
            auto& binder = *binders_.downscale[i];
            sets[1u] = binder.GetDescriptorSetHandle();
            binder.ClearBindings();

            uint32_t binding = 0;
            binder.BindImage(binding++, { targets_.tex1[i].GetHandle() });
            binder.BindImage(binding++, { targets_.tex1[i - 1].GetHandle() });
            binder.BindSampler(binding++, { samplerHandle_.GetHandle() });

            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        }
        cmdList.BindDescriptorSets(0u, sets);

        const auto targetSize = targets_.tex1Size[i];

        LocalPostProcessPushConstantStruct uPc;
        uPc.factor = bloomParameters_;
        uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
            1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));
        cmdList.PushConstantData(pc, arrayviewU8(uPc));

        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderBloom::ComputeUpscale(const PushConstant& pc, IRenderCommandList& cmdList)
{
    cmdList.BindPipeline(psos_.upscale);
    const ShaderThreadGroup tgs = psos_.upscaleTGS;

    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();

    for (size_t i = targets_.tex1.size() - 1; i != 0; --i) {
        {
            auto& binder = *binders_.upscale[i];
            sets[1u] = binder.GetDescriptorSetHandle();
            binder.ClearBindings();

            binder.BindImage(0u, { targets_.tex1[i - 1].GetHandle() });
            binder.BindImage(1u, { targets_.tex1[i].GetHandle() });
            binder.BindSampler(2u, { samplerHandle_.GetHandle() });

            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        }
        cmdList.BindDescriptorSets(0u, sets);

        const auto targetSize = targets_.tex1Size[i - 1];

        LocalPostProcessPushConstantStruct uPc;
        uPc.factor = bloomParameters_;
        uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
            1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));
        cmdList.PushConstantData(pc, arrayviewU8(uPc));

        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderBloom::ComputeCombine(const PushConstant& pc, IRenderCommandList& cmdList)
{
    cmdList.BindPipeline(psos_.combine);
    const ShaderThreadGroup tgs = psos_.combineTGS;

    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();
    {
        auto& binder = *binders_.combine;
        sets[1u] = binder.GetDescriptorSetHandle();
        binder.ClearBindings();
        // bind resources to set 1
        uint32_t binding = 0;
        binder.BindImage(binding++, { bloomInfo_.output });
        binder.BindImage(binding++, { bloomInfo_.input });
        binder.BindImage(binding++, { targets_.tex1[0].GetHandle() });
        binder.BindSampler(binding++, { samplerHandle_.GetHandle() });

        // update the descriptor set bindings for set 1
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }

    cmdList.BindDescriptorSets(0u, sets);

    const auto targetSize = baseSize_;

    LocalPostProcessPushConstantStruct uPc;
    uPc.factor = bloomParameters_;
    uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
        1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));
    cmdList.PushConstantData(pc, arrayviewU8(uPc));

    cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
}

void RenderBloom::GraphicsBloom(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList)
{
    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;

    RenderPassSubpassDesc& subpassDesc = renderPass.subpassDesc;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.colorAttachmentIndices[0] = 0;

    constexpr PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT,
        sizeof(LocalPostProcessPushConstantStruct) };

    UpdateGlobalSet(cmdList);
    if (bloomEnabled_) {
        RenderDownscaleAndThreshold(renderPass, pc, cmdList);
        RenderDownscale(renderPass, pc, cmdList);
        RenderUpscale(renderPass, pc, cmdList);
    }
    // combine (needs to be done even when bloom is disabled if node is in use
    if (RenderHandleUtil::IsValid(bloomInfo_.output.handle)) {
        RenderCombine(renderPass, pc, cmdList);
    }
}

void RenderBloom::RenderDownscaleAndThreshold(
    RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList)
{
    const auto targetSize = targets_.tex1Size[0];
    const ViewportDesc viewportDesc { 0, 0, static_cast<float>(targetSize.x), static_cast<float>(targetSize.y) };
    const ScissorDesc scissorDesc = { 0, 0, targetSize.x, targetSize.y };

    renderPass.renderPassDesc.attachmentHandles[0] = targets_.tex1[0].GetHandle();
    renderPass.renderPassDesc.renderArea = { 0, 0, targetSize.x, targetSize.y };
    cmdList.BeginRenderPass(renderPass.renderPassDesc, 0, renderPass.subpassDesc);

    cmdList.SetDynamicStateViewport(viewportDesc);
    cmdList.SetDynamicStateScissor(scissorDesc);
    cmdList.BindPipeline(psos_.downscaleAndThreshold);

    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();
    {
        auto& binder = *binders_.downscaleAndThreshold;
        sets[1u] = binder.GetDescriptorSetHandle();
        binder.ClearBindings();

        binder.BindImage(0u, { bloomInfo_.input });
        binder.BindSampler(1u, { samplerHandle_.GetHandle() });
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }
    cmdList.BindDescriptorSets(0u, sets);

    LocalPostProcessPushConstantStruct uPc;
    uPc.factor = bloomParameters_;
    uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
        1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));

    cmdList.PushConstantData(pc, arrayviewU8(uPc));
    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderBloom::RenderDownscale(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList)
{
    LocalPostProcessPushConstantStruct uPc;
    uPc.factor = bloomParameters_;

    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();
    for (size_t idx = 1; idx < targets_.tex1.size(); ++idx) {
        const auto targetSize = targets_.tex1Size[idx];
        const ViewportDesc viewportDesc { 0, 0, static_cast<float>(targetSize.x), static_cast<float>(targetSize.y) };
        const ScissorDesc scissorDesc = { 0, 0, targetSize.x, targetSize.y };

        renderPass.renderPassDesc.attachmentHandles[0] = targets_.tex1[idx].GetHandle();
        renderPass.renderPassDesc.renderArea = { 0, 0, targetSize.x, targetSize.y };
        cmdList.BeginRenderPass(renderPass.renderPassDesc, 0, renderPass.subpassDesc);

        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);

        cmdList.BindPipeline(psos_.downscale);

        {
            auto& binder = *binders_.downscale[idx];
            sets[1u] = binder.GetDescriptorSetHandle();
            binder.ClearBindings();
            binder.BindImage(0u, { targets_.tex1[idx - 1].GetHandle() });
            binder.BindSampler(1u, { samplerHandle_.GetHandle() });
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        }
        cmdList.BindDescriptorSets(0u, sets);

        uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
            1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }
}

void RenderBloom::RenderUpscale(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList)
{
    RenderPass renderPassUpscale = renderPass;
    renderPassUpscale.subpassDesc.inputAttachmentCount = 1;
    renderPassUpscale.subpassDesc.inputAttachmentIndices[0] = 0;
    renderPassUpscale.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPassUpscale.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;

    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();
    PLUGIN_ASSERT(targets_.tex1.size() == targets_.tex2.size());
    RenderHandle input;
    if (targets_.tex1.size() >= 1) {
        input = targets_.tex1[targets_.tex1.size() - 1].GetHandle();
    }
    for (size_t idx = targets_.tex1.size() - 1; idx != 0; --idx) {
        const auto targetSize = targets_.tex1Size[idx - 1];
        const ViewportDesc viewportDesc { 0, 0, static_cast<float>(targetSize.x), static_cast<float>(targetSize.y) };
        const ScissorDesc scissorDesc = { 0, 0, targetSize.x, targetSize.y };

        // tex2 as output
        renderPassUpscale.renderPassDesc.attachmentHandles[0] = targets_.tex2[idx - 1].GetHandle();
        renderPassUpscale.renderPassDesc.renderArea = { 0, 0, targetSize.x, targetSize.y };
        cmdList.BeginRenderPass(renderPassUpscale.renderPassDesc, 0, renderPassUpscale.subpassDesc);

        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);

        cmdList.BindPipeline(psos_.upscale);

        {
            auto& binder = *binders_.upscale[idx];
            sets[1u] = binder.GetDescriptorSetHandle();
            binder.ClearBindings();

            uint32_t binding = 0;
            binder.BindImage(binding++, { input });
            binder.BindImage(binding++, { targets_.tex1[idx - 1].GetHandle() });
            binder.BindSampler(binding++, { samplerHandle_.GetHandle() });
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        }
        cmdList.BindDescriptorSets(0u, sets);
        LocalPostProcessPushConstantStruct uPc;
        uPc.factor = bloomParameters_;
        uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
            1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();

        // next pass input
        input = renderPassUpscale.renderPassDesc.attachmentHandles[0];
    }
}

void RenderBloom::RenderCombine(RenderPass& renderPass, const PushConstant& pc, IRenderCommandList& cmdList)
{
    const auto targetSize = baseSize_;

    renderPass.renderPassDesc.attachmentHandles[0] = bloomInfo_.output.handle;
    renderPass.renderPassDesc.renderArea = { 0, 0, targetSize.x, targetSize.y };
    cmdList.BeginRenderPass(renderPass.renderPassDesc, 0, renderPass.subpassDesc);

    cmdList.SetDynamicStateViewport(baseViewportDesc_);
    cmdList.SetDynamicStateScissor(baseScissorDesc_);

    cmdList.BindPipeline(psos_.combine);

    RenderHandle sets[2u] {};
    sets[0u] = binders_.globalSet0->GetDescriptorSetHandle();
    {
        auto& binder = *binders_.combine;
        sets[1u] = binder.GetDescriptorSetHandle();
        binder.ClearBindings();

        uint32_t binding = 0;
        binder.BindImage(binding++, { bloomInfo_.input });
        // tex2 handle has the final result
        binder.BindImage(binding++, { targets_.tex2[0].GetHandle() });
        binder.BindSampler(binding++, { samplerHandle_.GetHandle() });

        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }
    cmdList.BindDescriptorSets(0u, sets);

    LocalPostProcessPushConstantStruct uPc;
    uPc.factor = bloomParameters_;
    uPc.viewportSizeInvSize = Math::Vec4(static_cast<float>(targetSize.x), static_cast<float>(targetSize.y),
        1.0f / static_cast<float>(targetSize.x), 1.0f / static_cast<float>(targetSize.y));

    cmdList.PushConstantData(pc, arrayviewU8(uPc));
    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderBloom::CreateTargets(IRenderNodeContextManager& renderNodeContextMgr, const Math::UVec2 baseSize)
{
    if (baseSize.x != baseSize_.x || baseSize.y != baseSize_.y) {
        baseSize_ = baseSize;

        format_ = Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32;
        ImageUsageFlags usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT |
                                     CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        if (bloomInfo_.useCompute) {
            format_ = Format::BASE_FORMAT_R16G16B16A16_SFLOAT; // used due to GLES
            usageFlags = CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
        } else {
            baseViewportDesc_ = { 0.0f, 0.0f, static_cast<float>(baseSize.x), static_cast<float>(baseSize.y), 0.0f,
                1.0f };
            baseScissorDesc_ = { 0, 0, baseSize.x, baseSize.y };
        }

        // create target image
        const Math::UVec2 startTargetSize = baseSize_;
        GpuImageDesc desc {
            ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            format_,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            usageFlags,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS |
                EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS,
            startTargetSize.x,
            startTargetSize.y,
            1u,
            1u,
            1u,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };

        auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
#if (RENDER_VALIDATION_ENABLED == 1)
        const string_view nodeName = renderNodeContextMgr.GetName();
#endif
        for (size_t idx = 0; idx < targets_.tex1.size(); ++idx) {
            // every bloom target is half the size of the original/ previous bloom target
            desc.width /= 2u;
            desc.height /= 2u;
            desc.width = (desc.width >= 1u) ? desc.width : 1u;
            desc.height = (desc.height >= 1u) ? desc.height : 1u;
            targets_.tex1Size[idx] = Math::UVec2(desc.width, desc.height);
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_Bloom_" + to_string(idx);
            targets_.tex1[idx] = gpuResourceMgr.Create(baseTargetName + "_A", desc);
            if (!bloomInfo_.useCompute) {
                targets_.tex2[idx] = gpuResourceMgr.Create(baseTargetName + "_B", desc);
            }
#else
            targets_.tex1[idx] = gpuResourceMgr.Create(targets_.tex1[idx], desc);
            if (!bloomInfo_.useCompute) {
                targets_.tex2[idx] = gpuResourceMgr.Create(targets_.tex2[idx], desc);
            }
#endif
        }
    }
}

void RenderBloom::CreatePsos(IRenderNodeContextManager& renderNodeContextMgr)
{
    if (bloomInfo_.useCompute) {
        CreateComputePsos(renderNodeContextMgr);
    } else {
        CreateRenderPsos(renderNodeContextMgr);
    }
}

void RenderBloom::CreateComputePsos(IRenderNodeContextManager& renderNodeContextMgr)
{
    const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    INodeContextPsoManager& psoMgr = renderNodeContextMgr.GetPsoManager();
    INodeContextDescriptorSetManager& dSetMgr = renderNodeContextMgr.GetDescriptorSetManager();

    constexpr BASE_NS::pair<BloomConfiguration::BloomQualityType, uint32_t> configurations[] = {
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_LOW, RenderBloom::CORE_BLOOM_QUALITY_LOW },
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_NORMAL, RenderBloom::CORE_BLOOM_QUALITY_NORMAL },
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_HIGH, RenderBloom::CORE_BLOOM_QUALITY_HIGH }
    };
    for (const auto& configuration : configurations) {
        {
            auto shader = shaderMgr.GetShaderHandle("rendershaders://computeshader/bloom_downscale.shader");
            const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);
            ShaderSpecializationConstantView specializations = shaderMgr.GetReflectionSpecialization(shader);
            const ShaderSpecializationConstantDataView specDataView {
                { specializations.constants.data(), specializations.constants.size() },
                { &configuration.second, 1u },
            };

            psos_.downscaleHandlesCompute[configuration.first].regular =
                psoMgr.GetComputePsoHandle(shader, pl, specDataView);
        }
        {
            auto shader = shaderMgr.GetShaderHandle("rendershaders://computeshader/bloom_downscale_threshold.shader");
            const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

            ShaderSpecializationConstantView specializations = shaderMgr.GetReflectionSpecialization(shader);
            const ShaderSpecializationConstantDataView specDataView {
                { specializations.constants.data(), specializations.constants.size() },
                { &configuration.second, 1u },
            };
            psos_.downscaleHandlesCompute[configuration.first].threshold =
                psoMgr.GetComputePsoHandle(shader, pl, specDataView);
        }
    }

    constexpr uint32_t globalSet = 0u;
    constexpr uint32_t localSetIdx = 1u;
    // the first one creates the global set as well
    {
        const RenderHandle shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/bloom_downscale_threshold.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
        psos_.downscaleAndThreshold = psoMgr.GetComputePsoHandle(shaderHandle, pl, {});
        psos_.downscaleAndThresholdTGS = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);

        const auto& gBinds = pl.descriptorSetLayouts[globalSet].bindings;
        binders_.globalSet0 = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(gBinds), gBinds);

        const auto& lBinds = pl.descriptorSetLayouts[localSetIdx].bindings;
        binders_.downscaleAndThreshold = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(lBinds), lBinds);
    }
    {
        const RenderHandle shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/bloom_downscale.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
        psos_.downscale = psoMgr.GetComputePsoHandle(shaderHandle, pl, {});
        psos_.downscaleTGS = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);

        PLUGIN_ASSERT(binders_.downscale.size() >= TARGET_COUNT);
        const auto& binds = pl.descriptorSetLayouts[localSetIdx].bindings;
        for (uint32_t idx = 0; idx < TARGET_COUNT; ++idx) {
            binders_.downscale[idx] = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(binds), binds);
        }
    }
    {
        const RenderHandle shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/bloom_upscale.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
        psos_.upscale = psoMgr.GetComputePsoHandle(shaderHandle, pl, {});
        psos_.upscaleTGS = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);

        PLUGIN_ASSERT(binders_.upscale.size() >= TARGET_COUNT);
        const auto& binds = pl.descriptorSetLayouts[localSetIdx].bindings;
        for (uint32_t idx = 0; idx < TARGET_COUNT; ++idx) {
            binders_.upscale[idx] = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(binds), binds);
        }
    }
    {
        const RenderHandle shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/bloom_combine.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
        psos_.combine = psoMgr.GetComputePsoHandle(shaderHandle, pl, {});
        psos_.combineTGS = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);

        const auto& binds = pl.descriptorSetLayouts[localSetIdx].bindings;
        binders_.combine = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(binds), binds);
    }
}

std::pair<RenderHandle, const PipelineLayout&> RenderBloom::CreateAndReflectRenderPso(
    IRenderNodeContextManager& renderNodeContextMgr, const string_view shader, const RenderPass& renderPass)
{
    const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    const RenderHandle shaderHandle = shaderMgr.GetShaderHandle(shader.data());
    const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderHandle);
    const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shaderHandle);

    auto& psoMgr = renderNodeContextMgr.GetPsoManager();
    const RenderHandle pso = psoMgr.GetGraphicsPsoHandle(
        shaderHandle, graphicsStateHandle, pl, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    return { pso, pl };
}

void RenderBloom::CreateRenderPsos(IRenderNodeContextManager& renderNodeContextMgr)
{
    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.attachmentHandles[0] = bloomInfo_.input.handle;
    renderPass.renderPassDesc.renderArea = { 0, 0, baseSize_.x, baseSize_.y };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;

    RenderPassSubpassDesc subpassDesc = renderPass.subpassDesc;
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.colorAttachmentIndices[0] = 0;

    constexpr BASE_NS::pair<BloomConfiguration::BloomQualityType, uint32_t> configurations[] = {
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_LOW, RenderBloom::CORE_BLOOM_QUALITY_LOW },
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_NORMAL, RenderBloom::CORE_BLOOM_QUALITY_NORMAL },
        { BloomConfiguration::BloomQualityType::QUALITY_TYPE_HIGH, RenderBloom::CORE_BLOOM_QUALITY_HIGH }
    };

    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr.GetShaderManager();
    INodeContextPsoManager& psoMgr = renderNodeContextMgr.GetPsoManager();

    for (const auto& configuration : configurations) {
        {
            auto shader = shaderMgr.GetShaderHandle("rendershaders://shader/bloom_downscale.shader");
            const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);
            ShaderSpecializationConstantView specializations = shaderMgr.GetReflectionSpecialization(shader);
            const ShaderSpecializationConstantDataView specDataView {
                { specializations.constants.data(), specializations.constants.size() },
                { &configuration.second, 1u },
            };
            const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(shader);
            psos_.downscaleHandles[configuration.first].regular = psoMgr.GetGraphicsPsoHandle(
                shader, graphicsState, pl, {}, specDataView, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }

        {
            auto shader = shaderMgr.GetShaderHandle("rendershaders://shader/bloom_downscale_threshold.shader");
            const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);
            ShaderSpecializationConstantView specializations = shaderMgr.GetReflectionSpecialization(shader);
            const ShaderSpecializationConstantDataView specDataView {
                { specializations.constants.data(), specializations.constants.size() },
                { &configuration.second, 1u },
            };
            const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(shader);
            psos_.downscaleHandles[configuration.first].threshold = psoMgr.GetGraphicsPsoHandle(
                shader, graphicsState, pl, {}, specDataView, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
    }

    INodeContextDescriptorSetManager& dSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
    constexpr uint32_t globalSet = 0u;
    constexpr uint32_t localSet = 1u;
    // the first one creates the global set as well
    {
        const auto [pso, pipelineLayout] = CreateAndReflectRenderPso(
            renderNodeContextMgr, "rendershaders://shader/bloom_downscale_threshold.shader", renderPass);
        psos_.downscaleAndThreshold = pso;

        const auto& gBinds = pipelineLayout.descriptorSetLayouts[globalSet].bindings;
        binders_.globalSet0 = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(gBinds), gBinds);

        const auto& lBinds = pipelineLayout.descriptorSetLayouts[localSet].bindings;
        binders_.downscaleAndThreshold = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(lBinds), lBinds);
    }
    {
        const auto [pso, pipelineLayout] = CreateAndReflectRenderPso(
            renderNodeContextMgr, "rendershaders://shader/bloom_downscale.shader", renderPass);
        psos_.downscale = pso;
        const auto& binds = pipelineLayout.descriptorSetLayouts[localSet].bindings;
        for (uint32_t idx = 0; idx < TARGET_COUNT; ++idx) {
            binders_.downscale[idx] = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(binds), binds);
        }
    }
    {
        const auto [pso, pipelineLayout] =
            CreateAndReflectRenderPso(renderNodeContextMgr, "rendershaders://shader/bloom_upscale.shader", renderPass);
        psos_.upscale = pso;
        const auto& binds = pipelineLayout.descriptorSetLayouts[localSet].bindings;
        for (uint32_t idx = 0; idx < TARGET_COUNT; ++idx) {
            binders_.upscale[idx] = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(binds), binds);
        }
    }
    {
        const auto [pso, pipelineLayout] =
            CreateAndReflectRenderPso(renderNodeContextMgr, "rendershaders://shader/bloom_combine.shader", renderPass);
        psos_.combine = pso;
        const auto& binds = pipelineLayout.descriptorSetLayouts[localSet].bindings;
        binders_.combine = dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(binds), binds);
    }
}
RENDER_END_NAMESPACE()
