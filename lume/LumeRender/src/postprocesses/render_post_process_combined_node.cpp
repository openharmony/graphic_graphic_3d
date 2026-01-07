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

#include "render_post_process_combined_node.h"

#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>
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
#include <render/property/property_types.h>

#include "datastore/render_data_store_pod.h"
#include "datastore/render_data_store_post_process.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;

CORE_BEGIN_NAMESPACE()
ENUM_TYPE_METADATA(RENDER_NS::PostProcessConfiguration::PostProcessEnableFlagBits,
    ENUM_VALUE(ENABLE_TONEMAP_BIT, "Tonemap"), ENUM_VALUE(ENABLE_VIGNETTE_BIT, "Vignette"),
    ENUM_VALUE(ENABLE_DITHER_BIT, "Dither"), ENUM_VALUE(ENABLE_COLOR_CONVERSION_BIT, "Color Conversion"),
    ENUM_VALUE(ENABLE_COLOR_FRINGE_BIT, "Color Fringe"))

ENUM_TYPE_METADATA(RENDER_NS::TonemapConfiguration::TonemapType, ENUM_VALUE(TONEMAP_ACES, "Aces"),
    ENUM_VALUE(TONEMAP_ACES_2020, "Aces 2020"), ENUM_VALUE(TONEMAP_FILMIC, "Filmic"),
    ENUM_VALUE(TONEMAP_PBR_NEUTRAL, "Neutral"))

ENUM_TYPE_METADATA(RENDER_NS::DitherConfiguration::DitherType, ENUM_VALUE(INTERLEAVED_NOISE, "Interleaved Noise"),
    ENUM_VALUE(TRIANGLE_NOISE, "Triangle Noise"), ENUM_VALUE(TRIANGLE_NOISE_RGB, "Triangle Noise RGB"))

ENUM_TYPE_METADATA(RENDER_NS::ColorConversionConfiguration::ConversionFunctionType,
    ENUM_VALUE(CONVERSION_LINEAR, "Linear"), ENUM_VALUE(CONVERSION_LINEAR_TO_SRGB, "Linear to sRGB"),
    ENUM_VALUE(CONVERSION_MULTIPLY_WITH_ALPHA, "Multiply With Alpha"),)

DATA_TYPE_METADATA(RENDER_NS::TonemapConfiguration, MEMBER_PROPERTY(tonemapType, "Tonemap Type", 0),
    MEMBER_PROPERTY(exposure, "Exposure", 0))

DATA_TYPE_METADATA(RENDER_NS::VignetteConfiguration, MEMBER_PROPERTY(coefficient, "Coefficient", 0),
    MEMBER_PROPERTY(power, "Power", 0))

DATA_TYPE_METADATA(RENDER_NS::ColorFringeConfiguration, MEMBER_PROPERTY(distanceCoefficient, "Distance Coefficient", 0))

DATA_TYPE_METADATA(RENDER_NS::DitherConfiguration, MEMBER_PROPERTY(ditherType, "Type", 0),
    MEMBER_PROPERTY(amountCoefficient, "Amount", 0))

DATA_TYPE_METADATA(RENDER_NS::ColorConversionConfiguration,
    BITFIELD_MEMBER_PROPERTY(conversionFunctionType, "Conversion Function", PropertyFlags::IS_BITFIELD,
        RENDER_NS::ColorConversionConfiguration::ConversionFunctionType))

DATA_TYPE_METADATA(RENDER_NS::PostProcessConfiguration,
    BITFIELD_MEMBER_PROPERTY(enableFlags, "Enable Flags", PropertyFlags::IS_BITFIELD,
        RENDER_NS::PostProcessConfiguration::PostProcessEnableFlagBits),
    MEMBER_PROPERTY(tonemapConfiguration, "Tonemap Configuration", 0),
    MEMBER_PROPERTY(vignetteConfiguration, "Vignette Configuration", 0),
    MEMBER_PROPERTY(ditherConfiguration, "Dither Configuration", 0),
    MEMBER_PROPERTY(colorConversionConfiguration, "Color Conversion Configuration", 0),
    MEMBER_PROPERTY(colorFringeConfiguration, "Color Fringe Configuration", 0))

DATA_TYPE_METADATA(RENDER_NS::RenderPostProcessCombinedNode::EffectProperties, MEMBER_PROPERTY(enabled, "Enabled", 0),
    MEMBER_PROPERTY(glOptimizedLayerCopyEnabled, "GL Optimized Layer Copy Enabled", 0),
    MEMBER_PROPERTY(postProcessConfiguration, "Postprocess Configuration", 0))

DATA_TYPE_METADATA(RENDER_NS::RenderPostProcessCombinedNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0),
    MEMBER_PROPERTY(bloomFinalTarget, "bloomFinalTarget", 0), MEMBER_PROPERTY(dirtMask, "dirtMask", 0))

DATA_TYPE_METADATA(RENDER_NS::RenderPostProcessCombinedNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr string_view COMBINED_SHADER_NAME = "rendershaders://shader/fullscreen_combined_post_process.shader";
constexpr string_view COMBINED_LAYER_SHADER_NAME =
    "rendershaders://shader/fullscreen_combined_post_process_layer.shader";
} // namespace

RenderPostProcessCombinedNode::RenderPostProcessCombinedNode()
    : properties_(&propertiesData, PropertyType::DataType<EffectProperties>::MetaDataFromType()),
      inputProperties_(&nodeInputsData, PropertyType::DataType<NodeInputs>::MetaDataFromType()),
      outputProperties_(&nodeOutputsData, PropertyType::DataType<NodeOutputs>::MetaDataFromType())
{}

IPropertyHandle* RenderPostProcessCombinedNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessCombinedNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

void RenderPostProcessCombinedNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

IRenderNode::ExecuteFlags RenderPostProcessCombinedNode::GetExecuteFlags() const
{
    if (effectProperties_.enabled) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessCombinedNode::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    samplers_.nearest =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_NEAREST_CLAMP);
    samplers_.linear =
        gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    combineData_.sd = shaderMgr.GetShaderDataByShaderName(COMBINED_SHADER_NAME);
    combineData_.pso = {};
    combineDataLayer_.sd = shaderMgr.GetShaderDataByShaderName(COMBINED_LAYER_SHADER_NAME);
    combineDataLayer_.pso = {};

    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    auto combineDataDescriptors = renderNodeUtil.GetDescriptorCounts(combineData_.sd.pipelineLayoutData);
    auto combineLayerDescriptors = renderNodeUtil.GetDescriptorCounts(combineDataLayer_.sd.pipelineLayoutData);
    descriptorCounts_.counts.clear();
    descriptorCounts_.counts.reserve(combineDataDescriptors.counts.size() + combineLayerDescriptors.counts.size());
    descriptorCounts_.counts.append(combineDataDescriptors.counts.cbegin(), combineDataDescriptors.counts.cend());
    descriptorCounts_.counts.append(combineLayerDescriptors.counts.cbegin(), combineLayerDescriptors.counts.cend());

    binders_.set0.reset();
    binders_.combineBinder.reset();
    binders_.combineLayerBinder.reset();

    valid_ = true;
}

void RenderPostProcessCombinedNode::PreExecuteFrame()
{
    if (valid_) {
        const array_view<const uint8_t> propertyView = GetData();
        // this node is directly dependant
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessCombinedNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessCombinedNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessCombinedNode::EffectProperties&)(*propertyView.data());
        }
        // allocate UBO data
        IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        reservedUboHandle_ = gpuResourceMgr.ReserveGpuBuffer(sizeof(RenderPostProcessConfiguration));
    } else {
        effectProperties_.enabled = false;
    }
}

RenderPass RenderPostProcessCombinedNode::CreateRenderPass(const RenderHandle input)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
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

void RenderPostProcessCombinedNode::ExecuteFrame(IRenderCommandList& cmdList)
{
    CheckDescriptorSetNeed();

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderCombine", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    UpdatePostProcessData(effectProperties_.postProcessConfiguration);

    auto& effect = effectProperties_.glOptimizedLayerCopyEnabled ? combineDataLayer_ : combineData_;
    const RenderPass renderPass = CreateRenderPass(nodeOutputsData.output.handle);
    if (!RenderHandleUtil::IsValid(effect.pso)) {
        auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
        const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(effect.sd.shader);
        effect.pso = psoMgr.GetGraphicsPsoHandle(effect.sd.shader, graphicsStateHandle, effect.sd.pipelineLayout, {},
            {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(effect.pso);

    {
        RenderHandle sets[2U] {};
        DescriptorSetLayoutBindingResources resources[2U] {};
        {
            auto& binder = *binders_.set0;
            binder.ClearBindings();
            uint32_t binding = 0u;
            binder.BindBuffer(binding, acquiredGpuBufferData_.handle, acquiredGpuBufferData_.bindingByteOffset);
            sets[0U] = binder.GetDescriptorSetHandle();
            resources[0U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        {
            auto& binder = *binders_.combineBinder;
            binder.ClearBindings();
            uint32_t binding = 0u;
            BindableImage mainInput = nodeInputsData.input;
            mainInput.samplerHandle = samplers_.linear;
            binder.BindImage(binding++, mainInput);
            if (!RenderHandleUtil::IsValid(nodeInputsData.bloomFinalTarget.handle)) {
                auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
                nodeInputsData.bloomFinalTarget.handle =
                    gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
            }
            if (!RenderHandleUtil::IsValid(nodeInputsData.dirtMask.handle)) {
                auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
                nodeInputsData.dirtMask.handle =
                    gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
            }
            binder.BindImage(binding++, nodeInputsData.bloomFinalTarget.handle, samplers_.linear);
            binder.BindImage(binding++, nodeInputsData.dirtMask.handle, samplers_.linear);
            sets[1U] = binder.GetDescriptorSetHandle();
            resources[1U] = binder.GetDescriptorSetLayoutBindingResources();
        }
        cmdList.UpdateDescriptorSets(sets, resources);
        cmdList.BindDescriptorSets(0u, sets);
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    if (effect.sd.pipelineLayoutData.pushConstant.byteSize > 0U) {
        const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const float layer = effectProperties_.glOptimizedLayerCopyEnabled ? float(nodeInputsData.input.layer) : 0.0f;
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
            { layer, 0.0f, 0.0f, 0.0f } };
        cmdList.PushConstantData(effect.sd.pipelineLayoutData.pushConstant, arrayviewU8(pc));
    }

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessCombinedNode::UpdatePostProcessData(const PostProcessConfiguration& postProcessConfiguration)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    acquiredGpuBufferData_ = gpuResourceMgr.AcquireGpubuffer(reservedUboHandle_);

    const RenderPostProcessConfiguration rppc =
        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(postProcessConfiguration);
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    if (acquiredGpuBufferData_.data) {
        const auto& mgbd = acquiredGpuBufferData_;
        if (!CloneData(mgbd.data, size_t(mgbd.byteSize), &rppc, sizeof(RenderPostProcessConfiguration))) {
            PLUGIN_LOG_E("post process ubo copying failed.");
        }
    }
}

void RenderPostProcessCombinedNode::CheckDescriptorSetNeed()
{
    if (!binders_.set0) {
        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        constexpr uint32_t globalSetIdx = 0u;
        constexpr uint32_t localSetIdx = 1u;

        binders_.set0 = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(globalSetIdx, combineData_.sd.pipelineLayoutData),
            combineData_.sd.pipelineLayoutData.descriptorSetLayouts[globalSetIdx].bindings);

        binders_.combineBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, combineData_.sd.pipelineLayoutData),
            combineData_.sd.pipelineLayoutData.descriptorSetLayouts[localSetIdx].bindings);
        binders_.combineLayerBinder = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(localSetIdx, combineDataLayer_.sd.pipelineLayoutData),
            combineDataLayer_.sd.pipelineLayoutData.descriptorSetLayouts[localSetIdx].bindings);
    }
}

DescriptorCounts RenderPostProcessCombinedNode::GetRenderDescriptorCounts() const
{
    return descriptorCounts_;
}

RENDER_END_NAMESPACE()
