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

#include "render_post_process_blur_node.h"

#include <base/math/mathf.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "render/shaders/common/render_post_process_structs_common.h"
#include "util/log.h"

using namespace BASE_NS;
using namespace CORE_NS;

CORE_BEGIN_NAMESPACE()
ENUM_TYPE_METADATA(RENDER_NS::BlurConfiguration::BlurType, ENUM_VALUE(TYPE_NORMAL, "Normal"),
    ENUM_VALUE(TYPE_HORIZONTAL, "Horizontal"), ENUM_VALUE(TYPE_VERTICAL, "Vertical"))

ENUM_TYPE_METADATA(RENDER_NS::BlurConfiguration::BlurQualityType, ENUM_VALUE(QUALITY_TYPE_LOW, "Low Quality"),
    ENUM_VALUE(QUALITY_TYPE_NORMAL, "Normal Quality"), ENUM_VALUE(QUALITY_TYPE_HIGH, "High Quality"))

DATA_TYPE_METADATA(RENDER_NS::BlurConfiguration, MEMBER_PROPERTY(blurType, "Type", 0),
    MEMBER_PROPERTY(blurQualityType, "Quality Type", 0), MEMBER_PROPERTY(filterSize, "Filter Size", 0),
    MEMBER_PROPERTY(maxMipLevel, "Max Mip Levels", 0))

ENUM_TYPE_METADATA(RENDER_NS::RenderPostProcessBlurNode::BlurShaderType, ENUM_VALUE(BLUR_SHADER_TYPE_RGBA, "RGBA"),
    ENUM_VALUE(BLUR_SHADER_TYPE_R, "Red"), ENUM_VALUE(BLUR_SHADER_TYPE_RG, "RG"),
    ENUM_VALUE(BLUR_SHADER_TYPE_RGB, "RGB"), ENUM_VALUE(BLUR_SHADER_TYPE_A, "Alpha"),
    ENUM_VALUE(BLUR_SHADER_TYPE_SOFT_DOWNSCALE_RGB, "Downscale RGB"),
    ENUM_VALUE(BLUR_SHADER_TYPE_DOWNSCALE_RGBA, "Downscale RGBA"),
    ENUM_VALUE(BLUR_SHADER_TYPE_DOWNSCALE_RGBA_ALPHA_WEIGHT, "Downscale RGBA Alpha Weight"),
    ENUM_VALUE(BLUR_SHADER_TYPE_RGBA_ALPHA_WEIGHT, "RGBA Alpha Weight"))

DATA_TYPE_METADATA(RENDER_NS::RenderPostProcessBlurNode::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0),
    MEMBER_PROPERTY(blurConfiguration, "Blur Configuration", 0), MEMBER_PROPERTY(blurShaderType, "Blur Shader Type", 0),
    MEMBER_PROPERTY(blurShaderScaleType, "Blur Shader Type", 0))

DATA_TYPE_METADATA(RENDER_NS::RenderPostProcessBlurNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0))

DATA_TYPE_METADATA(RENDER_NS::RenderPostProcessBlurNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr string_view SHADER_NAME { "rendershaders://shader/fullscreen_blur.shader" };
constexpr uint32_t MAX_MIP_COUNT { 16U };
constexpr uint32_t MAX_PASS_PER_LEVEL_COUNT { 3U };
constexpr uint32_t MAX_BINDER_COUNT = MAX_MIP_COUNT * MAX_PASS_PER_LEVEL_COUNT;

RenderPassDesc::RenderArea GetImageRenderArea(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle handle)
{
    const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(handle);
    return { 0U, 0U, desc.width, desc.height };
}
} // namespace

RenderPostProcessBlurNode::RenderPostProcessBlurNode()
    : properties_(&propertiesData, PropertyType::DataType<EffectProperties>::MetaDataFromType()),
      inputProperties_(&nodeInputsData, PropertyType::DataType<NodeInputs>::MetaDataFromType()),
      outputProperties_(&nodeOutputsData, PropertyType::DataType<NodeOutputs>::MetaDataFromType())
{}

IPropertyHandle* RenderPostProcessBlurNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessBlurNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

DescriptorCounts RenderPostProcessBlurNode::GetRenderDescriptorCounts() const
{
    return descriptorCounts_;
}

void RenderPostProcessBlurNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

void RenderPostProcessBlurNode::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    // clear
    pipelineData_ = {};
    binders_.clear();

    renderNodeContextMgr_ = &renderNodeContextMgr;

    // default inputs
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    defaultInput_.handle = gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
    defaultInput_.samplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);

    // load shaders
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    pipelineData_.gsd = shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(SHADER_NAME));

    // get needed descriptor set counts
    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    descriptorCounts_ = renderNodeUtil.GetDescriptorCounts(pipelineData_.gsd.pipelineLayoutData);
    for (auto& ref : descriptorCounts_.counts) {
        ref.count *= MAX_BINDER_COUNT;
    }

    valid_ = true;
}

void RenderPostProcessBlurNode::PreExecuteFrame()
{
    if (valid_) {
        const array_view<const uint8_t> propertyView = GetData();
        // this node is directly dependant
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessBlurNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessBlurNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessBlurNode::EffectProperties&)(*propertyView.data());
        }
    } else {
        effectProperties_.enabled = false;
    }

    if (effectProperties_.enabled) {
        // check input and output
        const bool valid = EvaluateInOut();
        if (valid) {
            EvaluateTemporaryTargets();
            mipsBlur =
                (inputImgData_.mipCount > 1U) && ((nodeInputsData.input.handle == nodeOutputsData.output.handle) ||
                                                     ((effectProperties_.blurConfiguration.maxMipLevel != 0U) &&
                                                         (effectProperties_.blurConfiguration.filterSize == 1.0f)));
        }
        valid_ = valid && effectProperties_.enabled;
    }
}

IRenderNode::ExecuteFlags RenderPostProcessBlurNode::GetExecuteFlags() const
{
    if (effectProperties_.enabled) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessBlurNode::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }

    CheckDescriptorSetNeed();

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderBlur", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    if (binders_.empty()) {
        return;
    }

    BindableImage currOutput = nodeOutputsData.output;
    if (!RenderHandleUtil::IsValid(currOutput.handle)) {
        return;
    }
    // get the property values

    // update the output
    nodeOutputsData.output = currOutput;

    const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const RenderPassDesc::RenderArea imageArea = GetImageRenderArea(gpuResourceMgr, currOutput.handle);
    const RenderPassDesc::RenderArea renderArea = useRequestedRenderArea_ ? renderAreaRequest_.area : imageArea;
    if ((renderArea.extentWidth == 0) || (renderArea.extentHeight == 0)) {
        return;
    }

    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.renderArea = { 0, 0, imageArea.extentWidth, imageArea.extentHeight };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachmentHandles[0] = currOutput.handle;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;

    if (!RenderHandleUtil::IsValid(pipelineData_.psoScale)) {
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const ShaderSpecializationConstantView sscv = shaderMgr.GetReflectionSpecialization(pipelineData_.gsd.shader);
        {
            const uint32_t specializationFlags[] = { effectProperties_.blurShaderScaleType };
            const ShaderSpecializationConstantDataView specDataView { sscv.constants, specializationFlags };
            pipelineData_.psoScale = psoMgr.GetGraphicsPsoHandle(
                pipelineData_.gsd, specDataView, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
        {
            const uint32_t specializationFlags[] = { effectProperties_.blurShaderType };
            const ShaderSpecializationConstantDataView specDataView { sscv.constants, specializationFlags };
            pipelineData_.psoBlur = psoMgr.GetGraphicsPsoHandle(
                pipelineData_.gsd, specDataView, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
    }
    if (mipsBlur) {
        if (effectProperties_.blurConfiguration.blurType == BlurConfiguration::BlurType::TYPE_NORMAL) {
            RenderGaussian(cmdList, renderPass);
        } else {
            RenderDirectionalBlur(cmdList, renderPass);
        }
    } else {
        RenderData(cmdList, renderPass);
    }
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

void BlurPass(const ConstDrawInput& di, IDescriptorSetBinder& binder, const RenderHandle psoHandle,
    const RenderHandle image, const uint32_t inputMipLevel)
{
    di.cmdList.BeginRenderPass(
        di.renderPass.renderPassDesc, di.renderPass.subpassStartIndex, di.renderPass.subpassDesc);
    di.cmdList.BindPipeline(psoHandle);

    binder.ClearBindings();
    binder.BindSampler(0, di.sampler);
    binder.BindImage(1u, { image, inputMipLevel });
    di.cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    di.cmdList.BindDescriptorSet(0, binder.GetDescriptorSetHandle());

    di.cmdList.PushConstant(di.pushConstant, reinterpret_cast<const uint8_t*>(&di.pc));
    di.cmdList.Draw(3u, 1u, 0u, 0u);
    di.cmdList.EndRenderPass();
}
} // namespace

void RenderPostProcessBlurNode::RenderDirectionalBlur(IRenderCommandList& cmdList, const RenderPass& renderPassBase)
{
    RenderPass renderPass = renderPassBase;
    if constexpr (USE_CUSTOM_BARRIERS) {
        cmdList.BeginDisableAutomaticBarrierPoints();
    }
    const vec4 factor = (effectProperties_.blurConfiguration.blurType == BlurConfiguration::BlurType::TYPE_HORIZONTAL)
                            ? vec4 { 1.0f, 0.0f, 0.0f, 0.0f }
                            : vec4 { 0.0f, 1.0f, 0.0f, 0.0f };

    // with every mip, first we do a downscale
    // then a single horizontal/vertical blur
    LocalPostProcessPushConstantStruct pc { { 1.0f, 0.0f, 0.0f, 0.0f }, factor };
    uint32_t descIdx = 0U;
    const uint32_t blurCount = Math::min(effectProperties_.blurConfiguration.maxMipLevel, inputImgData_.mipCount);
    const ConstDrawInput di { cmdList, renderPass, pipelineData_.gsd.pipelineLayoutData.pushConstant, pc,
        defaultInput_.samplerHandle };
    ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };

    const RenderHandle handle0 = inputImgData_.rawHandle;
    const RenderHandle handle1 = tmpImgTargets_.rawHandle;
    for (uint32_t idx = 1U; idx < blurCount; ++idx) {
        const uint32_t mip = idx;

        const Math::UVec2 size = { Math::max(1u, inputImgData_.size.x >> mip),
            Math::max(1u, inputImgData_.size.y >> mip) };
        const Math::Vec2 fSize = { static_cast<float>(size.x), static_cast<float>(size.y) };
        const Math::Vec4 texSizeInvTexSize { fSize.x * 1.0f, fSize.y * 1.0f, 1.0f / fSize.x, 1.0f / fSize.y };
        pc = { texSizeInvTexSize, factor };

        renderPass.renderPassDesc.renderArea = { 0, 0, size.x, size.y };
        renderPass.renderPassDesc.attachmentHandles[0] = handle0;
        renderPass.renderPassDesc.attachments[0].mipLevel = mip;

        cmdList.SetDynamicStateViewport({ 0.0f, 0.0f, fSize.x, fSize.y, 0.0f, 1.0f });
        cmdList.SetDynamicStateScissor({ 0, 0, size.x, size.y });

        // downscale
        if constexpr (USE_CUSTOM_BARRIERS) {
            DownscaleBarrier(cmdList, handle0, mip);
        }
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoScale, handle0, mip - 1u);

        if constexpr (USE_CUSTOM_BARRIERS) {
            BlurHorizontalBarrier(cmdList, handle0, mip, handle1);
        }

        // Apply horizontal/vertical blur on main image and store in temp
        renderPass.renderPassDesc.attachmentHandles[0] = handle1;
        renderPass.renderPassDesc.attachments[0].mipLevel = mip - 1u;
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoBlur, handle0, mip);

        if constexpr (USE_CUSTOM_BARRIERS) {
            BlurVerticalBarrier(cmdList, handle0, mip, handle1);
        }

        // copy temp (mip-1) to main (mip)
        renderPass.renderPassDesc.attachmentHandles[0] = handle0;
        renderPass.renderPassDesc.attachments[0].mipLevel = mip;
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoScale, handle1, mip - 1u);
    }

    if constexpr (USE_CUSTOM_BARRIERS) {
        if (inputImgData_.mipCount > 1U) {
            // transition the final used mip level
            if (blurCount > 0) {
                imgRange.baseMipLevel = blurCount - 1;
                imgRange.levelCount = 1;
                cmdList.CustomImageBarrier(handle0, FINAL_SRC, FINAL_DST, imgRange);
            }
            if (blurCount < inputImgData_.mipCount) {
                // transition the final levels which might be in undefined state
                imgRange.baseMipLevel = blurCount;
                imgRange.levelCount = inputImgData_.mipCount - blurCount;
                cmdList.CustomImageBarrier(handle0, SRC_UNDEFINED, FINAL_DST, imgRange);
            }
        }
        cmdList.AddCustomBarrierPoint();
        cmdList.EndDisableAutomaticBarrierPoints();
    }
}

void RenderPostProcessBlurNode::RenderGaussian(IRenderCommandList& cmdList, const RenderPass& renderPassBase)
{
    RenderPass renderPass = renderPassBase;
    if constexpr (USE_CUSTOM_BARRIERS) {
        cmdList.BeginDisableAutomaticBarrierPoints();
    }

    // with every mip, first we do a downscale
    // then a single horizontal and a single vertical blur
    LocalPostProcessPushConstantStruct pc { { 1.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f } };
    uint32_t descIdx = 0U;
    const uint32_t blurCount = Math::min(effectProperties_.blurConfiguration.maxMipLevel, inputImgData_.mipCount);
    const ConstDrawInput di { cmdList, renderPass, pipelineData_.gsd.pipelineLayoutData.pushConstant, pc,
        defaultInput_.samplerHandle };
    const RenderHandle handle0 = inputImgData_.rawHandle;
    const RenderHandle handle1 = tmpImgTargets_.rawHandle;
    for (uint32_t idx = 1U; idx < blurCount; ++idx) {
        const uint32_t mip = idx;

        const Math::UVec2 size = { Math::max(1u, inputImgData_.size.x >> mip),
            Math::max(1u, inputImgData_.size.y >> mip) };
        const Math::Vec2 fSize = { static_cast<float>(size.x), static_cast<float>(size.y) };
        const Math::Vec4 texSizeInvTexSize { fSize.x * 1.0f, fSize.y * 1.0f, 1.0f / fSize.x, 1.0f / fSize.y };
        pc = { texSizeInvTexSize, { 1.0f, 0.0f, 0.0f, 0.0f } };

        renderPass.renderPassDesc.renderArea = { 0, 0, size.x, size.y };
        renderPass.renderPassDesc.attachmentHandles[0] = handle0;
        renderPass.renderPassDesc.attachments[0].mipLevel = mip;

        cmdList.SetDynamicStateViewport({ 0.0f, 0.0f, fSize.x, fSize.y, 0.0f, 1.0f });
        cmdList.SetDynamicStateScissor({ 0, 0, size.x, size.y });

        // downscale
        if constexpr (USE_CUSTOM_BARRIERS) {
            DownscaleBarrier(cmdList, handle0, mip);
        }
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoScale, handle0, mip - 1u);

        // horizontal (from real image to temp)
        if constexpr (USE_CUSTOM_BARRIERS) {
            BlurHorizontalBarrier(cmdList, handle0, mip, handle1);
        }

        renderPass.renderPassDesc.attachmentHandles[0] = handle1;
        renderPass.renderPassDesc.attachments[0].mipLevel = mip - 1u;
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoBlur, handle0, mip);

        // vertical
        if constexpr (USE_CUSTOM_BARRIERS) {
            BlurVerticalBarrier(cmdList, handle0, mip, handle1);
        }

        renderPass.renderPassDesc.attachmentHandles[0] = handle0;
        renderPass.renderPassDesc.attachments[0].mipLevel = mip;
        pc.factor = { 0.0f, 1.0f, 0.0f, 0.0f };
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoBlur, handle1, mip - 1);
    }

    if constexpr (USE_CUSTOM_BARRIERS) {
        if (inputImgData_.mipCount > 1u) {
            // transition the final used mip level
            if (blurCount > 0) {
                const ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, blurCount - 1, 1, 0,
                    PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                cmdList.CustomImageBarrier(handle0, FINAL_SRC, FINAL_DST, imgRange);
            }
            if (blurCount < inputImgData_.mipCount) {
                // transition the final levels which might be in undefined state
                const ImageSubresourceRange imgRange { CORE_IMAGE_ASPECT_COLOR_BIT, blurCount,
                    inputImgData_.mipCount - blurCount, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
                cmdList.CustomImageBarrier(handle0, SRC_UNDEFINED, FINAL_DST, imgRange);
            }
        }
        cmdList.AddCustomBarrierPoint();
        cmdList.EndDisableAutomaticBarrierPoints();
    }
}

void RenderPostProcessBlurNode::RenderData(IRenderCommandList& cmdList, const RenderPass& renderPassBase)
{
    if ((!RenderHandleUtil::IsValid(tmpImgTargets_.rawHandle)) ||
        (!RenderHandleUtil::IsValid(tmpImgTargets_.rawHandleExt))) {
        return;
    }

    RenderPass renderPass = renderPassBase;

    // with every mip, first we do a downscale
    // then a single horizontal and a single vertical blur
    LocalPostProcessPushConstantStruct pc { { 1.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.0f } };
    uint32_t descIdx = 0U;
    const uint32_t blurCount =
        Math::min(static_cast<uint32_t>(effectProperties_.blurConfiguration.filterSize), MAX_MIP_COUNT);
    const ConstDrawInput di { cmdList, renderPass, pipelineData_.gsd.pipelineLayoutData.pushConstant, pc,
        defaultInput_.samplerHandle };
    const RenderHandle inputHandle = inputImgData_.rawHandle;
    const RenderHandle handle0 = tmpImgTargets_.rawHandle;
    const RenderHandle handle1 = tmpImgTargets_.rawHandleExt;
    // first downscale -> then ping pong
    {
        const Math::UVec2 size = { inputImgData_.size.x / 2U, inputImgData_.size.y / 2U };
        const Math::Vec2 fSize = { static_cast<float>(size.x), static_cast<float>(size.y) };
        const Math::Vec4 texSizeInvTexSize { fSize.x * 1.0f, fSize.y * 1.0f, 1.0f / fSize.x, 1.0f / fSize.y };
        pc = { texSizeInvTexSize, { 1.0f, 0.0f, 0.0f, 0.0f } };

        renderPass.renderPassDesc.renderArea = { 0, 0, size.x, size.y };

        cmdList.SetDynamicStateViewport({ 0.0f, 0.0f, fSize.x, fSize.y, 0.0f, 1.0f });
        cmdList.SetDynamicStateScissor({ 0, 0, size.x, size.y });

        renderPass.renderPassDesc.attachmentHandles[0] = handle0;
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoScale, inputHandle, 0U);
    }

    // output will be handle0
    const Math::UVec2 size = { inputImgData_.size.x / 2U, inputImgData_.size.y / 2U };
    const Math::Vec2 fSize = { static_cast<float>(size.x), static_cast<float>(size.y) };
    const Math::Vec4 texSizeInvTexSize { fSize.x * 1.0f, fSize.y * 1.0f, 1.0f / fSize.x, 1.0f / fSize.y };
    pc = { texSizeInvTexSize, { 1.0f, 0.0f, 0.0f, 0.0f } };
    renderPass.renderPassDesc.renderArea = { 0, 0, size.x, size.y };

    cmdList.SetDynamicStateViewport({ 0.0f, 0.0f, fSize.x, fSize.y, 0.0f, 1.0f });
    cmdList.SetDynamicStateScissor({ 0, 0, size.x, size.y });

    for (uint32_t idx = 0U; idx < blurCount; ++idx) {
        // horizontal (from real image to temp)
        pc.factor = { 1.0f, 0.0f, 0.0f, 0.0f };
        renderPass.renderPassDesc.attachmentHandles[0] = handle1;
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoBlur, handle0, 0U);

        // vertical
        renderPass.renderPassDesc.attachmentHandles[0] = handle0;
        pc.factor = { 0.0f, 1.0f, 0.0f, 0.0f };
        BlurPass(di, *binders_[descIdx++], pipelineData_.psoBlur, handle1, 0U);
    }

    // update output
    nodeOutputsData.output.handle = handle0;
}

bool RenderPostProcessBlurNode::EvaluateInOut()
{
    inputImgData_.rawHandle = nodeInputsData.input.handle;
    if (RenderHandleUtil::IsValid(nodeInputsData.input.handle)) {
        nodeOutputsData.output.handle = RenderHandleUtil::IsValid(nodeOutputsData.output.handle)
                                            ? nodeOutputsData.output.handle
                                            : nodeInputsData.input.handle;
        return true;
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    CORE_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "_inout_issue",
        "RENDER_VALIDATION: render post process input / output issue");
#endif
    return false;
}

void RenderPostProcessBlurNode::CheckDescriptorSetNeed()
{
    if (binders_.empty()) {
        constexpr uint32_t set { 0U };
        binders_.clear();
        binders_.resize(MAX_BINDER_COUNT);

        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        const auto& bindings = pipelineData_.gsd.pipelineLayoutData.descriptorSetLayouts[set].bindings;
        for (uint32_t idx = 0; idx < MAX_BINDER_COUNT; ++idx) {
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            binders_[idx] = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
    }
}

void RenderPostProcessBlurNode::EvaluateTemporaryTargets()
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const GpuImageDesc inDesc = gpuResourceMgr.GetImageDescriptor(nodeInputsData.input.handle);
    inputImgData_.size = { inDesc.width, inDesc.height };
    inputImgData_.mipCount = inDesc.mipCount;
    Math::UVec2 texSize = inputImgData_.size / 2U;
    texSize.x = Math::max(1u, texSize.x);
    texSize.y = Math::max(1u, texSize.y);
    if (texSize.x != tmpImgTargets_.size.x || texSize.y != tmpImgTargets_.size.y) {
        const bool needsTwoTargets =
            ((inDesc.usageFlags & ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT) == 0) ||
            (inputImgData_.mipCount <= 1U);
        tmpImgTargets_.size = texSize;
        constexpr ImageUsageFlags usageFlags = ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                               ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT |
                                               ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        const GpuImageDesc desc {
            ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            inDesc.format,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            usageFlags,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            0U,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS,
            texSize.x,
            texSize.y,
            1U,
            Math::max(1U, (inDesc.mipCount - 1U)),
            1u,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };
#if (RENDER_VALIDATION_ENABLED == 1)
        const auto additionalName = BASE_NS::to_string(reinterpret_cast<uintptr_t>(reinterpret_cast<void*>(this)));
        tmpImgTargets_.handle =
            gpuResourceMgr.Create(renderNodeContextMgr_->GetName() + "_BLUR0_" + additionalName, desc);
        if (needsTwoTargets) {
            tmpImgTargets_.handleExt =
                gpuResourceMgr.Create(renderNodeContextMgr_->GetName() + "_BLUR1" + additionalName, desc);
        }
#else
        tmpImgTargets_.handle = renderNodeContextMgr_->GetGpuResourceManager().Create(tmpImgTargets_.handle, desc);
        if (needsTwoTargets) {
            tmpImgTargets_.handleExt =
                renderNodeContextMgr_->GetGpuResourceManager().Create(tmpImgTargets_.handleExt, desc);
        }
#endif
        if (!needsTwoTargets) {
            tmpImgTargets_.handleExt = {};
        }
        tmpImgTargets_.rawHandle = tmpImgTargets_.handle.GetHandle();
        tmpImgTargets_.rawHandleExt = tmpImgTargets_.handleExt.GetHandle();
        tmpImgTargets_.mipCount = desc.mipCount;
    }
}
RENDER_END_NAMESPACE()
