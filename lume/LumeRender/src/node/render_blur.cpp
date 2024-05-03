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

#include "render_blur.h"

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
#include <render/nodecontext/intf_render_node_util.h>
#include <render/shaders/common/render_blur_common.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "render/shaders/common/render_post_process_structs_common.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t MAX_MIP_COUNT { 16u };
constexpr uint32_t MAX_PASS_PER_LEVEL_COUNT { 3u };
constexpr bool GAUSSIAN_TYPE { true };
} // namespace

void RenderBlur::Init(IRenderNodeContextManager& renderNodeContextMgr, const BlurInfo& blurInfo)
{
    blurInfo_ = blurInfo;
    {
        const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr.GetShaderManager();
        renderData_ = {};
        renderData_.shader = shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_blur.shader");
        renderData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayout(renderData_.shader);
    }
    {
        imageData_ = {};
        imageData_.mipImage = blurInfo.blurTarget.handle;
        samplerHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetSamplerHandle(
            DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
    }
    {
        constexpr uint32_t globalSet = 0u;
        constexpr uint32_t localSet = 1u;

        constexpr uint32_t maxBinderCount = MAX_MIP_COUNT * MAX_PASS_PER_LEVEL_COUNT;
        binders_.clear();
        binders_.resize(maxBinderCount);

        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
        {
            const auto& bindings = renderData_.pipelineLayout.descriptorSetLayouts[globalSet].bindings;
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            globalSet0_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
        const auto& bindings = renderData_.pipelineLayout.descriptorSetLayouts[localSet].bindings;
        for (uint32_t idx = 0; idx < maxBinderCount; ++idx) {
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            binders_[idx] = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
    }
}

void RenderBlur::PreExecute(
    IRenderNodeContextManager& renderNodeContextMgr, const BlurInfo& blurInfo, const PostProcessConfiguration& ppConfig)
{
    blurInfo_ = blurInfo;
    imageData_.mipImage = blurInfo.blurTarget.handle;
    globalUbo_ = blurInfo.globalUbo;

    const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    const GpuImageDesc imageDesc = gpuResourceMgr.GetImageDescriptor(imageData_.mipImage);
    imageData_.mipCount = imageDesc.mipCount;
    imageData_.format = imageDesc.format;
    imageData_.size = { imageDesc.width, imageDesc.height };
    if (GAUSSIAN_TYPE) {
        CreateTargets(renderNodeContextMgr, imageData_.size);
    }
}

void RenderBlur::Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
    const PostProcessConfiguration& ppConfig)
{
    if (!RenderHandleUtil::IsGpuImage(imageData_.mipImage)) {
        return;
    }

    UpdateGlobalSet(cmdList);

    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.renderArea = { 0, 0, imageData_.size.x, imageData_.size.y };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachmentHandles[0] = imageData_.mipImage;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;

    if (!RenderHandleUtil::IsValid(renderData_.psoScale)) {
        const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
        const ShaderSpecializationConstantView sscv = shaderMgr.GetReflectionSpecialization(renderData_.shader);
        const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(renderData_.shader);
        {
            const uint32_t specializationFlags[] = { blurInfo_.scaleType };
            const ShaderSpecializationConstantDataView specDataView { sscv.constants, specializationFlags };
            renderData_.psoScale =
                renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(renderData_.shader, graphicsState,
                    renderData_.pipelineLayout, {}, specDataView, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
        {
            const uint32_t specializationFlags[] = { blurInfo_.blurType };
            const ShaderSpecializationConstantDataView specDataView { sscv.constants, specializationFlags };
            renderData_.psoBlur =
                renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(renderData_.shader, graphicsState,
                    renderData_.pipelineLayout, {}, specDataView, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
    }

    if (GAUSSIAN_TYPE) {
        RenderGaussian(renderNodeContextMgr, cmdList, renderPass, ppConfig);
    } else {
        RenderData(renderNodeContextMgr, cmdList, renderPass, ppConfig);
    }
}

void RenderBlur::UpdateGlobalSet(IRenderCommandList& cmdList)
{
    auto& binder = *globalSet0_;
    binder.ClearBindings();
    uint32_t binding = 0u;
    binder.BindBuffer(binding++, globalUbo_, 0);
    binder.BindBuffer(binding++, globalUbo_, sizeof(GlobalPostProcessStruct));
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

DescriptorCounts RenderBlur::GetDescriptorCounts() const
{
    // expected high max mip count
    return DescriptorCounts { {
        { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_MIP_COUNT },
        { CORE_DESCRIPTOR_TYPE_SAMPLER, MAX_MIP_COUNT },
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u * MAX_MIP_COUNT },
    } };
}

// constants for RenderBlur::RenderData
namespace {
constexpr bool USE_CUSTOM_BARRIERS = true;

constexpr ImageResourceBarrier SRC_UNDEFINED { 0, CORE_PIPELINE_STAGE_TOP_OF_PIPE_BIT, CORE_IMAGE_LAYOUT_UNDEFINED };
constexpr ImageResourceBarrier COL_ATTACHMENT { CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
constexpr ImageResourceBarrier SHDR_READ { CORE_ACCESS_SHADER_READ_BIT, CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
// transition the final mip level to read only as well
constexpr ImageResourceBarrier FINAL_SRC { CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
    CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
constexpr ImageResourceBarrier FINAL_DST { CORE_ACCESS_SHADER_READ_BIT,
    CORE_PIPELINE_STAGE_VERTEX_SHADER_BIT, // first possible shader read stage
    CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
} // namespace

void RenderBlur::RenderData(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
    const RenderPass& renderPassBase, const PostProcessConfiguration& ppConfig)
{
    RenderPass renderPass = renderPassBase;
    const GpuImageDesc imageDesc = renderNodeContextMgr.GetGpuResourceManager().GetImageDescriptor(imageData_.mipImage);

    if (USE_CUSTOM_BARRIERS) {
        cmdList.BeginDisableAutomaticBarrierPoints();
    }

    RenderHandle sets[2u] {};
    sets[0] = globalSet0_->GetDescriptorSetHandle();

    const uint32_t blurCount = Math::min(ppConfig.blurConfiguration.maxMipLevel, imageData_.mipCount);
    // NOTE: for smoother results, first downscale -> then horiz / vert -> downscale and so on
    ImageSubresourceRange imageSubresourceRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    for (uint32_t idx = 1; idx < blurCount; ++idx) {
        const uint32_t renderPassMipLevel = blurInfo_.upScale ? (blurCount - idx - 1) : idx;
        const uint32_t inputMipLevel = blurInfo_.upScale ? (blurCount - idx) : (idx - 1);

        const uint32_t currWidth = Math::max(1u, imageDesc.width >> renderPassMipLevel);
        const uint32_t currHeight = Math::max(1u, imageDesc.height >> renderPassMipLevel);
        const float fCurrWidth = static_cast<float>(currWidth);
        const float fCurrHeight = static_cast<float>(currHeight);

        renderPass.renderPassDesc.renderArea = { 0, 0, currWidth, currHeight };
        renderPass.renderPassDesc.attachments[0].mipLevel = renderPassMipLevel;

        if (USE_CUSTOM_BARRIERS) {
            imageSubresourceRange.baseMipLevel = renderPassMipLevel;
            cmdList.CustomImageBarrier(imageData_.mipImage, SRC_UNDEFINED, COL_ATTACHMENT, imageSubresourceRange);
            imageSubresourceRange.baseMipLevel = inputMipLevel;
            if (inputMipLevel == 0) {
                cmdList.CustomImageBarrier(imageData_.mipImage, SHDR_READ, imageSubresourceRange);
            } else {
                cmdList.CustomImageBarrier(imageData_.mipImage, COL_ATTACHMENT, SHDR_READ, imageSubresourceRange);
            }
            cmdList.AddCustomBarrierPoint();
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

        cmdList.SetDynamicStateViewport(ViewportDesc { 0.0f, 0.0f, fCurrWidth, fCurrHeight, 0.0f, 0.0f });
        cmdList.SetDynamicStateScissor(ScissorDesc { 0, 0, currWidth, currHeight });

        cmdList.BindPipeline(renderData_.psoScale);

        {
            auto& binder = *binders_[idx];
            sets[1u] = binder.GetDescriptorSetHandle();
            binder.ClearBindings();
            binder.BindSampler(0, samplerHandle_);
            binder.BindImage(1, { imageData_.mipImage, inputMipLevel });
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        }
        cmdList.BindDescriptorSets(0u, sets);

        const LocalPostProcessPushConstantStruct pc {
            { fCurrWidth, fCurrHeight, 1.0f / (fCurrWidth), 1.0f / (fCurrHeight) }, { 1.0f, 0.0f, 0.0f, 0.0f }
        };
        cmdList.PushConstant(renderData_.pipelineLayout.pushConstant, reinterpret_cast<const uint8_t*>(&pc));

        cmdList.Draw(3u, 1u, 0u, 0u);
        cmdList.EndRenderPass();
    }

    if (USE_CUSTOM_BARRIERS) {
        if (imageData_.mipCount > 1u) {
            // transition the final used mip level
            if (blurCount > 0) {
                const ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, blurCount - 1, 1, 0,
                    PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                cmdList.CustomImageBarrier(imageData_.mipImage, FINAL_SRC, FINAL_DST, imgRange);
            }
            if (blurCount < imageData_.mipCount) {
                // transition the final levels which might be in undefined state
                const ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, blurCount,
                    imageData_.mipCount - blurCount, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                cmdList.CustomImageBarrier(imageData_.mipImage, SRC_UNDEFINED, FINAL_DST, imgRange);
            }
        }
        cmdList.AddCustomBarrierPoint();
        cmdList.EndDisableAutomaticBarrierPoints();
    }
}

namespace {
void DownscaleBarrier(IRenderCommandList& cmdList, const RenderHandle image, const uint32_t mipLevel)
{
    ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    imgRange.baseMipLevel = mipLevel;
    cmdList.CustomImageBarrier(image, SRC_UNDEFINED, COL_ATTACHMENT, imgRange);
    const uint32_t inputMipLevel = mipLevel - 1u;
    imgRange.baseMipLevel = inputMipLevel;
    if (inputMipLevel == 0) {
        cmdList.CustomImageBarrier(image, SHDR_READ, imgRange);
    } else {
        cmdList.CustomImageBarrier(image, COL_ATTACHMENT, SHDR_READ, imgRange);
    }
    cmdList.AddCustomBarrierPoint();
}

void BlurHorizontalBarrier(
    IRenderCommandList& cmdList, const RenderHandle realImage, const uint32_t mipLevel, const RenderHandle tmpImage)
{
    ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    imgRange.baseMipLevel = mipLevel;
    cmdList.CustomImageBarrier(realImage, COL_ATTACHMENT, SHDR_READ, imgRange);
    imgRange.baseMipLevel = mipLevel - 1;
    cmdList.CustomImageBarrier(tmpImage, SRC_UNDEFINED, COL_ATTACHMENT, imgRange);
    cmdList.AddCustomBarrierPoint();
}

void BlurVerticalBarrier(
    IRenderCommandList& cmdList, const RenderHandle realImage, const uint32_t mipLevel, const RenderHandle tmpImage)
{
    ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    imgRange.baseMipLevel = mipLevel;
    cmdList.CustomImageBarrier(realImage, SHDR_READ, COL_ATTACHMENT, imgRange);
    imgRange.baseMipLevel = mipLevel - 1;
    cmdList.CustomImageBarrier(tmpImage, COL_ATTACHMENT, SHDR_READ, imgRange);
    cmdList.AddCustomBarrierPoint();
}

struct ConstDrawInput {
    IRenderCommandList& cmdList;
    const RenderPass& renderPass;
    const PushConstant& pushConstant;
    const LocalPostProcessPushConstantStruct& pc;
    RenderHandle sampler;
};
void BlurPass(const ConstDrawInput& di, IDescriptorSetBinder& binder, IDescriptorSetBinder& globalBinder,
    const RenderHandle psoHandle, const RenderHandle image, const uint32_t inputMipLevel)
{
    di.cmdList.BeginRenderPass(
        di.renderPass.renderPassDesc, di.renderPass.subpassStartIndex, di.renderPass.subpassDesc);
    di.cmdList.BindPipeline(psoHandle);

    RenderHandle sets[2u] {};
    sets[0] = globalBinder.GetDescriptorSetHandle();
    {
        binder.ClearBindings();
        sets[1u] = binder.GetDescriptorSetHandle();
        binder.BindSampler(0, di.sampler);
        binder.BindImage(1u, { image, inputMipLevel });
        di.cmdList.UpdateDescriptorSet(
            binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }
    di.cmdList.BindDescriptorSets(0, sets);

    di.cmdList.PushConstant(di.pushConstant, reinterpret_cast<const uint8_t*>(&di.pc));
    di.cmdList.Draw(3u, 1u, 0u, 0u);
    di.cmdList.EndRenderPass();
}
} // namespace

void RenderBlur::RenderGaussian(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
    const RenderPass& renderPassBase, const PostProcessConfiguration& ppConfig)
{
    RenderPass renderPass = renderPassBase;
    if (USE_CUSTOM_BARRIERS) {
        cmdList.BeginDisableAutomaticBarrierPoints();
    }

    // with every mip, first we do a downscale
    // then a single horizontal and a single vertical blur
    LocalPostProcessPushConstantStruct pc { { 1.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f } };
    uint32_t descIdx = 0;
    const uint32_t blurCount = Math::min(ppConfig.blurConfiguration.maxMipLevel, imageData_.mipCount);
    const ConstDrawInput di { cmdList, renderPass, renderData_.pipelineLayout.pushConstant, pc, samplerHandle_ };
    for (uint32_t idx = 1; idx < blurCount; ++idx) {
        const uint32_t mip = idx;

        const Math::UVec2 size = { Math::max(1u, imageData_.size.x >> mip), Math::max(1u, imageData_.size.y >> mip) };
        const Math::Vec2 fSize = { static_cast<float>(size.x), static_cast<float>(size.y) };
        const Math::Vec4 texSizeInvTexSize { fSize.x * 1.0f, fSize.y * 1.0f, 1.0f / fSize.x, 1.0f / fSize.y };
        pc = { texSizeInvTexSize, { 1.0f, 0.0f, 0.0f, 0.0f } };

        renderPass.renderPassDesc.renderArea = { 0, 0, size.x, size.y };
        renderPass.renderPassDesc.attachments[0].mipLevel = mip;

        cmdList.SetDynamicStateViewport({ 0.0f, 0.0f, fSize.x, fSize.y, 0.0f, 1.0f });
        cmdList.SetDynamicStateScissor({ 0, 0, size.x, size.y });

        // downscale
        if (USE_CUSTOM_BARRIERS) {
            DownscaleBarrier(cmdList, imageData_.mipImage, mip);
        }
        BlurPass(di, *binders_[descIdx++], *globalSet0_, renderData_.psoScale, imageData_.mipImage, mip - 1u);

        // horizontal (from real image to temp)
        if (USE_CUSTOM_BARRIERS) {
            BlurHorizontalBarrier(cmdList, imageData_.mipImage, mip, tempTarget_.tex.GetHandle());
        }

        renderPass.renderPassDesc.attachmentHandles[0] = tempTarget_.tex.GetHandle();
        renderPass.renderPassDesc.attachments[0].mipLevel = mip - 1u;
        BlurPass(di, *binders_[descIdx++], *globalSet0_, renderData_.psoBlur, imageData_.mipImage, mip);

        // vertical
        if (USE_CUSTOM_BARRIERS) {
            BlurVerticalBarrier(cmdList, imageData_.mipImage, mip, tempTarget_.tex.GetHandle());
        }

        renderPass.renderPassDesc.attachmentHandles[0] = imageData_.mipImage;
        renderPass.renderPassDesc.attachments[0].mipLevel = mip;
        pc.factor = { 0.0f, 1.0f, 0.0f, 0.0f };
        BlurPass(di, *binders_[descIdx++], *globalSet0_, renderData_.psoBlur, tempTarget_.tex.GetHandle(), mip - 1);
    }

    if (USE_CUSTOM_BARRIERS) {
        if (imageData_.mipCount > 1u) {
            // transition the final used mip level
            if (blurCount > 0) {
                const ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, blurCount - 1, 1, 0,
                    PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                cmdList.CustomImageBarrier(imageData_.mipImage, FINAL_SRC, FINAL_DST, imgRange);
            }
            if (blurCount < imageData_.mipCount) {
                // transition the final levels which might be in undefined state
                const ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, blurCount,
                    imageData_.mipCount - blurCount, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                cmdList.CustomImageBarrier(imageData_.mipImage, SRC_UNDEFINED, FINAL_DST, imgRange);
            }
        }
        cmdList.AddCustomBarrierPoint();
        cmdList.EndDisableAutomaticBarrierPoints();
    }
}

void RenderBlur::CreateTargets(IRenderNodeContextManager& renderNodeContextMgr, const Math::UVec2 baseSize)
{
    Math::UVec2 texSize = baseSize / 2u;
    texSize.x = Math::max(1u, texSize.x);
    texSize.y = Math::max(1u, texSize.y);
    if (texSize.x != tempTarget_.texSize.x || texSize.y != tempTarget_.texSize.y) {
        tempTarget_.texSize = texSize;
        tempTarget_.format = imageData_.format;

        constexpr ImageUsageFlags usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                               ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT |
                                               ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;

        const GpuImageDesc desc {
            ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            tempTarget_.format,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            usageFlags,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS,
            tempTarget_.texSize.x,
            tempTarget_.texSize.y,
            1u,
            Math::max(1u, (imageData_.mipCount - 1u)),
            1u,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };
#if (RENDER_VALIDATION_ENABLED == 1)
        tempTarget_.tex =
            renderNodeContextMgr.GetGpuResourceManager().Create(renderNodeContextMgr.GetName() + "_BLUR_TARGET", desc);
#else
        tempTarget_.tex = renderNodeContextMgr.GetGpuResourceManager().Create(tempTarget_.tex, desc);
#endif
    }
}
RENDER_END_NAMESPACE()
