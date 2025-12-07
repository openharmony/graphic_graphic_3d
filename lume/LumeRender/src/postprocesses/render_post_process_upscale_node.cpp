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

#include "render_post_process_upscale_node.h"

#include <base/containers/unique_ptr.h>
#include <base/math/matrix_util.h>
#include <core/log.h>
#include <core/property/property_handle_util.h>
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
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RENDER_NS::UpscaleConfiguration, MEMBER_PROPERTY(ratio, "Upscale Ratio", 0),
    MEMBER_PROPERTY(smoothScale, "Gradient smooth scale", 0),
    MEMBER_PROPERTY(structureSensitivity, "Structure sensitivity", 0),
    MEMBER_PROPERTY(edgeSharpness, "Edge sharpness", 0))

DATA_TYPE_METADATA(RenderPostProcessUpscaleNode::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0),
    MEMBER_PROPERTY(upscaleConfiguration, "Configuration", 0))

DATA_TYPE_METADATA(RenderPostProcessUpscaleNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0))
DATA_TYPE_METADATA(RenderPostProcessUpscaleNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr string_view UPSCALE_GRADIENT_SHADER_NAME = "rendershaders://shader/tfs_upscaler_gradients.shader";
constexpr string_view UPSCALE_STRUCTURE_TENSOR_SHADER_NAME =
    "rendershaders://shader/tfs_upscaler_structure_tensor.shader";
constexpr string_view UPSCALE_FINAL_SHADER_NAME = "rendershaders://shader/tfs_upscaler_tensor_field.shader";
} // namespace

RenderPostProcessUpscaleNode::RenderPostProcessUpscaleNode()
    : properties_(&propertiesData, PropertyType::DataType<EffectProperties>::MetaDataFromType()),
      inputProperties_(&nodeInputsData, PropertyType::DataType<NodeInputs>::MetaDataFromType()),
      outputProperties_(&nodeOutputsData, PropertyType::DataType<NodeOutputs>::MetaDataFromType())
{}

IPropertyHandle* RenderPostProcessUpscaleNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessUpscaleNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

DescriptorCounts RenderPostProcessUpscaleNode::GetRenderDescriptorCounts() const
{
    return DescriptorCounts { {
        { CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3u },
        { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 4u },
        { CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0u },
        { CORE_DESCRIPTOR_TYPE_SAMPLER, 6u },
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0u },
    } };
}

void RenderPostProcessUpscaleNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

void RenderPostProcessUpscaleNode::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    renderCopyOutput_.Init(renderNodeContextMgr);
    psos_ = {};

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

    samplerNearestHandle_ = gpuResourceMgr.Create(samplerNearestHandle_,
        GpuSamplerDesc { Filter::CORE_FILTER_NEAREST, Filter::CORE_FILTER_NEAREST, Filter::CORE_FILTER_NEAREST,
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE });

    binders_.gradientPass.reset();
    binders_.structureTensorPass.reset();
    binders_.finalUpscalePass.reset();

    valid_ = true;
}

void RenderPostProcessUpscaleNode::PreExecuteFrame()
{
    if (valid_) {
        const array_view<const uint8_t> propertyView = GetData();
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessUpscaleNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessUpscaleNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessUpscaleNode::EffectProperties&)(*propertyView.data());
        }

        const GpuImageDesc& imgDesc =
            renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(nodeInputsData.input.handle);

        CreateTargets(Math::UVec2(imgDesc.width, imgDesc.height));
    } else {
        effectProperties_.enabled = false;
    }
}

IRenderNode::ExecuteFlags RenderPostProcessUpscaleNode::GetExecuteFlags() const
{
    if (effectProperties_.enabled) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessUpscaleNode::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }

    CreatePsos();
    CheckDescriptorSetNeed();

    if (!RenderHandleUtil::IsValid(nodeOutputsData.output.handle)) {
        return;
    }

    RENDER_DEBUG_MARKER_SCOPE(cmdList, "TensorFieldUpscaler");

    {
        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "GradientPass", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        RenderGradientPass(cmdList);
    }

    {
        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "StructureTensorPass", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        RenderStructureTensorPass(cmdList);
    }

    {
        RENDER_DEBUG_MARKER_COL_SCOPE(
            cmdList, "TensorFieldUpscalePass", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        TensorFieldUpscalePass(cmdList);
    }
}

void RenderPostProcessUpscaleNode::RenderGradientPass(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(nodeInputsData.input.handle)) {
        CORE_LOG_E("RenderGradientPass inputs invalid!");
        return;
    }

    auto& binder = binders_.gradientPass;
    if (!binder) {
        return;
    }

    auto renderPass = CreateRenderPass(targets_.gradientTexture.GetHandle(), targets_.inputResolution);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.gradientPass);

    binder->ClearBindings();
    binder->BindImage(0u, { nodeInputsData.input.handle });
    binder->BindSampler(1u, { samplerNearestHandle_.GetHandle() });

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    if (shaderData_.gradientPass.pipelineLayoutData.pushConstant.byteSize > 0U) {
        const auto& inputSize = targets_.inputResolution;
        const auto& outputSize = targets_.outputResolution;

        UpscalerPushConstant uPc;
        uPc.inputSize =
            Math::Vec4(float(inputSize.x), float(inputSize.y), 1.0f / float(inputSize.x), 1.0f / float(inputSize.y));
        uPc.outputSize = Math::Vec4(
            float(outputSize.x), float(outputSize.y), 1.0f / float(outputSize.x), 1.0f / float(outputSize.y));
        uPc.smoothScale = effectProperties_.upscaleConfiguration.smoothScale;
        uPc.structureSensitivity = effectProperties_.upscaleConfiguration.structureSensitivity;
        uPc.edgeSharpness = effectProperties_.upscaleConfiguration.edgeSharpness;

        cmdList.PushConstantData(shaderData_.finalUpscalePass.pipelineLayoutData.pushConstant, arrayviewU8(uPc));

        cmdList.PushConstantData(shaderData_.gradientPass.pipelineLayoutData.pushConstant, arrayviewU8(uPc));
    }

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessUpscaleNode::RenderStructureTensorPass(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(nodeInputsData.input.handle) ||
        !RenderHandleUtil::IsValid(targets_.gradientTexture.GetHandle())) {
        CORE_LOG_E("RenderStructureTensorPass inputs invalid!");
        return;
    }

    auto& binder = binders_.structureTensorPass;
    if (!binder) {
        return;
    }

    auto renderPass = CreateRenderPass(targets_.tensorDataTexture.GetHandle(), targets_.inputResolution);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.structureTensorPass);

    binder->ClearBindings();
    binder->BindImage(0u, { targets_.gradientTexture.GetHandle() });
    binder->BindSampler(1u, { samplerNearestHandle_.GetHandle() });

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    if (shaderData_.structureTensorPass.pipelineLayoutData.pushConstant.byteSize > 0U) {
        const auto& inputSize = targets_.inputResolution;
        const auto& outputSize = targets_.outputResolution;

        UpscalerPushConstant uPc;
        uPc.inputSize =
            Math::Vec4(float(inputSize.x), float(inputSize.y), 1.0f / float(inputSize.x), 1.0f / float(inputSize.y));
        uPc.outputSize = Math::Vec4(
            float(outputSize.x), float(outputSize.y), 1.0f / float(outputSize.x), 1.0f / float(outputSize.y));
        uPc.smoothScale = effectProperties_.upscaleConfiguration.smoothScale;
        uPc.structureSensitivity = effectProperties_.upscaleConfiguration.structureSensitivity;
        uPc.edgeSharpness = effectProperties_.upscaleConfiguration.edgeSharpness;

        cmdList.PushConstantData(shaderData_.finalUpscalePass.pipelineLayoutData.pushConstant, arrayviewU8(uPc));

        cmdList.PushConstantData(shaderData_.structureTensorPass.pipelineLayoutData.pushConstant, arrayviewU8(uPc));
    }

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessUpscaleNode::TensorFieldUpscalePass(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(nodeInputsData.input.handle) ||
        !RenderHandleUtil::IsValid(targets_.tensorDataTexture.GetHandle())) {
        CORE_LOG_E("TensorFieldUpscalePass inputs invalid!");
        return;
    }

    auto& binder = binders_.finalUpscalePass;
    if (!binder) {
        return;
    }

    auto renderPass = CreateRenderPass(nodeOutputsData.output.handle, targets_.outputResolution);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.finalUpscalePass);

    binder->ClearBindings();
    binder->BindImage(0u, { nodeInputsData.input.handle });
    binder->BindImage(1u, { targets_.tensorDataTexture.GetHandle() });
    binder->BindSampler(2u, { samplerNearestHandle_.GetHandle() });

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0u, binder->GetDescriptorSetHandle());

    if (shaderData_.finalUpscalePass.pipelineLayoutData.pushConstant.byteSize > 0U) {
        const auto& inputSize = targets_.inputResolution;
        const auto& outputSize = targets_.outputResolution;

        UpscalerPushConstant uPc;
        uPc.inputSize =
            Math::Vec4(float(inputSize.x), float(inputSize.y), 1.0f / float(inputSize.x), 1.0f / float(inputSize.y));
        uPc.outputSize = Math::Vec4(
            float(outputSize.x), float(outputSize.y), 1.0f / float(outputSize.x), 1.0f / float(outputSize.y));
        uPc.smoothScale = effectProperties_.upscaleConfiguration.smoothScale;
        uPc.structureSensitivity = effectProperties_.upscaleConfiguration.structureSensitivity;
        uPc.edgeSharpness = effectProperties_.upscaleConfiguration.edgeSharpness;

        cmdList.PushConstantData(shaderData_.finalUpscalePass.pipelineLayoutData.pushConstant, arrayviewU8(uPc));
    }

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

RenderPass RenderPostProcessUpscaleNode::CreateRenderPass(
    const RenderHandle output, const BASE_NS::Math::UVec2& resolution) const
{
    RenderPass rp;
    rp.renderPassDesc.attachmentCount = 1u;
    rp.renderPassDesc.attachmentHandles[0u] = output;
    rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    rp.renderPassDesc.renderArea = { 0, 0, resolution.x, resolution.y };

    rp.renderPassDesc.subpassCount = 1u;
    rp.subpassDesc.colorAttachmentCount = 1u;
    rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
    rp.subpassStartIndex = 0u;
    return rp;
}

void RenderPostProcessUpscaleNode::CreateTargets(const BASE_NS::Math::UVec2 baseSize)
{
    if (baseSize.x != baseSize_.x || baseSize.y != baseSize_.y) {
        baseSize_ = baseSize;

        const float ratio = effectProperties_.upscaleConfiguration.ratio;

        targets_.inputResolution = baseSize;
        targets_.outputResolution = Math::UVec2(static_cast<uint32_t>(Math::round((float)baseSize.x * ratio)),
            static_cast<uint32_t>(Math::round((float)baseSize.y * ratio)));

        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
#if (RENDER_VALIDATION_ENABLED == 1)
        const string_view nodeName = renderNodeContextMgr_->GetName();
#endif

        // gradient texture
        const GpuImageDesc gradientDesc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R16G16_SFLOAT, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, targets_.inputResolution.x,
            targets_.inputResolution.y, 1u, 1u, 1u, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

        // structure tensor components
        const GpuImageDesc tensorDataTextureDesc { ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, Format::BASE_FORMAT_R16G16B16A16_SFLOAT,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, targets_.inputResolution.x,
            targets_.inputResolution.y, 1u, 1u, 1u, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

#if (RENDER_VALIDATION_ENABLED == 1)
        const auto gradientName = nodeName + "_gradient";
        const auto tensorDataName = nodeName + "_tensor_data";
        const auto outputName = nodeName + "_upscale_output";

        targets_.gradientTexture = gpuResourceMgr.Create(gradientName, gradientDesc);
        targets_.tensorDataTexture = gpuResourceMgr.Create(tensorDataName, tensorDataTextureDesc);
#else
        targets_.gradientTexture = gpuResourceMgr.Create(gradientDesc);
        targets_.tensorDataTexture = gpuResourceMgr.Create(tensorDataTextureDesc);
#endif
    }
}

void RenderPostProcessUpscaleNode::CreatePsos()
{
    if (RenderHandleUtil::IsValid(psos_.gradientPass) && RenderHandleUtil::IsValid(psos_.structureTensorPass) &&
        RenderHandleUtil::IsValid(psos_.finalUpscalePass)) {
        return;
    }

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    INodeContextPsoManager& psoMgr = renderNodeContextMgr_->GetPsoManager();

    if (!RenderHandleUtil::IsValid(shaderData_.gradientPass.shader)) {
        shaderData_.gradientPass = shaderMgr.GetShaderDataByShaderName(UPSCALE_GRADIENT_SHADER_NAME);
    }
    if (!RenderHandleUtil::IsValid(shaderData_.structureTensorPass.shader)) {
        shaderData_.structureTensorPass = shaderMgr.GetShaderDataByShaderName(UPSCALE_STRUCTURE_TENSOR_SHADER_NAME);
    }
    if (!RenderHandleUtil::IsValid(shaderData_.finalUpscalePass.shader)) {
        shaderData_.finalUpscalePass = shaderMgr.GetShaderDataByShaderName(UPSCALE_FINAL_SHADER_NAME);
    }

    if (!RenderHandleUtil::IsValid(psos_.gradientPass)) {
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.gradientPass.shader);
        psos_.gradientPass = psoMgr.GetGraphicsPsoHandle(shaderData_.gradientPass.shader, gfxHandle,
            shaderData_.gradientPass.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    if (!RenderHandleUtil::IsValid(psos_.structureTensorPass)) {
        const RenderHandle gfxHandle =
            shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.structureTensorPass.shader);
        psos_.structureTensorPass = psoMgr.GetGraphicsPsoHandle(shaderData_.structureTensorPass.shader, gfxHandle,
            shaderData_.structureTensorPass.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    if (!RenderHandleUtil::IsValid(psos_.finalUpscalePass)) {
        const RenderHandle gfxHandle =
            shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.finalUpscalePass.shader);
        psos_.finalUpscalePass = psoMgr.GetGraphicsPsoHandle(shaderData_.finalUpscalePass.shader, gfxHandle,
            shaderData_.finalUpscalePass.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }
}

void RenderPostProcessUpscaleNode::CheckDescriptorSetNeed()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    if (!binders_.gradientPass) {
        binders_.gradientPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.gradientPass.pipelineLayoutData),
            shaderData_.gradientPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.structureTensorPass) {
        binders_.structureTensorPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.structureTensorPass.pipelineLayoutData),
            shaderData_.structureTensorPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.finalUpscalePass) {
        binders_.finalUpscalePass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.finalUpscalePass.pipelineLayoutData),
            shaderData_.finalUpscalePass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }
}

RENDER_END_NAMESPACE()