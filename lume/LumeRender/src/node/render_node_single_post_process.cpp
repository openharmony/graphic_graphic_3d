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

#include "render_node_single_post_process.h"

#include <base/math/mathf.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_post_process.h>
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

#include "datastore/render_data_store_pod.h"
#include "datastore/render_data_store_post_process.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t GLOBAL_POST_PROCESS_SET { 0u };
constexpr string_view RENDER_DATA_STORE_POD_NAME { RenderDataStorePod::TYPE_NAME };

constexpr string_view INPUT = "input";
constexpr string_view OUTPUT = "output";

constexpr string_view POST_PROCESS_BASE_PIPELINE_LAYOUT { "renderpipelinelayouts://post_process_common.shaderpl" };

struct DispatchResources {
    RenderHandle buffer {};
    RenderHandle image {};
};

DispatchResources GetDispatchResources(const RenderNodeHandles::InputResources& ir)
{
    DispatchResources dr;
    if (!ir.customInputBuffers.empty()) {
        dr.buffer = ir.customInputBuffers[0].handle;
    }
    if (!ir.customInputImages.empty()) {
        dr.image = ir.customInputImages[0].handle;
    }
    return dr;
}

RenderHandleReference CreatePostProcessDataUniformBuffer(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    PLUGIN_STATIC_ASSERT(
        sizeof(LocalPostProcessStruct) == PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE);
    return gpuResourceMgr.Create(
        handle, GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                    sizeof(GlobalPostProcessStruct) + sizeof(LocalPostProcessStruct) });
}

uint32_t GetPostProcessFlag(const string_view ppName)
{
    if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_FXAA]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT;
    } else if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_TAA]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT;
    } else if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_BLOOM]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT;
    } else if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_BLUR]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT;
    } else if (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_DOF]) {
        return PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DOF_BIT;
    } else {
        return 0;
    }
}

bool NeedsShader(const string_view ppName)
{
    if ((ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_BLOOM]) ||
        (ppName == PostProcessConstants::POST_PROCESS_NAMES[PostProcessConstants::RENDER_BLUR])) {
        return false;
    } else {
        return true;
    }
}

bool NeedsAutoBindingSet0(const RenderNodeHandles::InputResources& inputRes)
{
    uint32_t set0Bindings = 0;
    for (const auto& res : inputRes.buffers) {
        if (res.set == GLOBAL_POST_PROCESS_SET) {
            set0Bindings++;
        }
    }
    return (set0Bindings == 0);
}

inline BindableImage GetBindableImage(const RenderHandle& res)
{
    return BindableImage { res, PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS, ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED, {} };
}
} // namespace

void RenderNodeSinglePostProcess::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    valid_ = true;
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    ubos_.postProcess = CreatePostProcessDataUniformBuffer(gpuResourceMgr, ubos_.postProcess);

    ParseRenderNodeInputs();
    UpdateImageData();
    ProcessPostProcessConfiguration();
    if (!RenderHandleUtil::IsValid(shader_)) {
        shader_ = ppLocalConfig_.shader.GetHandle();
    }
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(shader_);
    if (handleType == RenderHandleType::SHADER_STATE_OBJECT) {
        graphics_ = true;
    } else if (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        graphics_ = false;
        threadGroupSize_ = shaderMgr.GetReflectionThreadGroupSize(shader_);
        if (dispatchResources_.customInputBuffers.empty() && dispatchResources_.customInputImages.empty()) {
            valid_ = false;
            PLUGIN_LOG_W("RenderNodeSinglePostProcess: dispatchResources (GPU buffer or GPU image) needed");
        }
    }

    pipelineLayout_ = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(shader_);

    const bool needsShader = NeedsShader(jsonInputs_.ppName);
    if (needsShader) {
        if (!((handleType == RenderHandleType::SHADER_STATE_OBJECT) ||
                (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT))) {
            PLUGIN_LOG_E("RN:%s needs a valid shader handle", renderNodeContextMgr_->GetName().data());
        }
        const RenderHandle plHandle = shaderMgr.GetReflectionPipelineLayoutHandle(shader_);
        pipelineLayout_ = shaderMgr.GetPipelineLayout(plHandle);
        if (graphics_) {
            const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(shader_);
            psoHandle_ = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(
                shader_, graphicsState, pipelineLayout_, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        } else {
            psoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(shader_, pipelineLayout_, {});
        }

        // NOTE: cannot be compared with shaderMgr.GetCompatibilityFlags due to missing pipeline layout handle
        const RenderHandle basePlHandle = shaderMgr.GetPipelineLayoutHandle(POST_PROCESS_BASE_PIPELINE_LAYOUT);
        const IShaderManager::CompatibilityFlags compatibilityFlags =
            shaderMgr.GetCompatibilityFlags(basePlHandle, plHandle);
        if ((compatibilityFlags & IShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT) == 0) {
            PLUGIN_LOG_E("RN:%s uncompatible pipeline layout to %s", renderNodeContextMgr_->GetName().data(),
                POST_PROCESS_BASE_PIPELINE_LAYOUT.data());
        }
    }
    InitCreateBinders();

    if ((!RenderHandleUtil::IsValid(shader_)) || (!RenderHandleUtil::IsValid(psoHandle_))) {
        valid_ = false;
    }

    // special handling for bloom and blur
    RenderHandle output = builtInVariables_.output;
    if ((builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLOOM_BIT) &&
        ppLocalConfig_.variables.enabled) {
        const RenderBloom::BloomInfo bloomInfo { GetBindableImage(builtInVariables_.input),
            GetBindableImage(builtInVariables_.output), ubos_.postProcess.GetHandle(),
            ppGlobalConfig_.bloomConfiguration.useCompute };
        renderBloom_.Init(renderNodeContextMgr, bloomInfo);
        output = renderBloom_.GetFinalTarget();
    } else if ((builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLUR_BIT) &&
               ppLocalConfig_.variables.enabled) {
        RenderBlur::BlurInfo blurInfo { GetBindableImage(builtInVariables_.output), ubos_.postProcess.GetHandle(),
            false };
        renderBlur_.Init(renderNodeContextMgr, blurInfo);
    }
    renderCopy_.Init(renderNodeContextMgr, {});
    RegisterOutputs(output);
}

void RenderNodeSinglePostProcess::PreExecuteFrame()
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    UpdateImageData();

    ProcessPostProcessConfiguration();

    // special handling for bloom and blur
    RenderHandle output = builtInVariables_.output;
    if ((builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLOOM_BIT) &&
        ppLocalConfig_.variables.enabled) {
        const RenderBloom::BloomInfo bloomInfo { GetBindableImage(builtInVariables_.input),
            GetBindableImage(builtInVariables_.output), ubos_.postProcess.GetHandle(),
            ppGlobalConfig_.bloomConfiguration.useCompute };
        renderBloom_.PreExecute(*renderNodeContextMgr_, bloomInfo, ppGlobalConfig_);
        output = renderBloom_.GetFinalTarget();
    } else if ((builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLUR_BIT) &&
               ppLocalConfig_.variables.enabled) {
        RenderBlur::BlurInfo blurInfo { GetBindableImage(builtInVariables_.input), ubos_.postProcess.GetHandle(),
            false };
        renderBlur_.PreExecute(*renderNodeContextMgr_, blurInfo, ppGlobalConfig_);
    }
    {
        RenderCopy::CopyInfo copyInfo { GetBindableImage(builtInVariables_.input),
            GetBindableImage(builtInVariables_.output), {} };
        renderCopy_.PreExecute(*renderNodeContextMgr_, copyInfo);
    }
    RegisterOutputs(output);
}

void RenderNodeSinglePostProcess::ExecuteFrame(IRenderCommandList& cmdList)
{
    if ((!ppLocalConfig_.variables.enabled) &&
        (jsonInputs_.defaultOutputImage != DefaultOutputImage::INPUT_OUTPUT_COPY)) {
        return;
    }

    if (ppLocalConfig_.variables.enabled) {
        // ppConfig_ is already up-to-date from PreExecuteFrame
        if ((builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLOOM_BIT) &&
            ppLocalConfig_.variables.enabled) {
            renderBloom_.Execute(*renderNodeContextMgr_, cmdList, ppGlobalConfig_);
            return;
        } else if ((builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLUR_BIT) &&
                   ppLocalConfig_.variables.enabled) {
            renderBlur_.Execute(*renderNodeContextMgr_, cmdList, ppGlobalConfig_);
            return;
        }
        if (valid_) {
            ExecuteSinglePostProcess(cmdList);
        }
    } else if (jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT_OUTPUT_COPY) {
        renderCopy_.Execute(*renderNodeContextMgr_, cmdList);
    }
}

void RenderNodeSinglePostProcess::ExecuteSinglePostProcess(IRenderCommandList& cmdList)
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    }
    if (jsonInputs_.hasChangeableResourceHandles) {
        // input resources updated in preExecuteFrame
        renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
    }
    if (jsonInputs_.hasChangeableDispatchHandles) {
        dispatchResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.dispatchResources);
    }
    if (useAutoBindSet0_) {
        UpdateGlobalPostProcessUbo();
    }

    RenderPass renderPass;
    DispatchResources dispatchResources;
    if (graphics_) {
        renderPass = renderNodeUtil.CreateRenderPass(inputRenderPass_);
        if ((renderPass.renderPassDesc.attachmentCount == 0) ||
            !RenderHandleUtil::IsValid(renderPass.renderPassDesc.attachmentHandles[0])) {
#if (RENDER_VALIDATION_ENABLED == 1)
            PLUGIN_LOG_ONCE_W("rp_missing_" + renderNodeContextMgr_->GetName(),
                "RENDER_VALIDATION: RN: %s, invalid attachment", renderNodeContextMgr_->GetName().data());
#endif
            return;
        }
    } else {
        dispatchResources = GetDispatchResources(dispatchResources_);
        if ((!RenderHandleUtil::IsValid(dispatchResources.buffer)) &&
            (!RenderHandleUtil::IsValid(dispatchResources.image))) {
            return; // no way to evaluate dispatch size
        }
    }

    const bool invalidBindings = (!pipelineDescriptorSetBinder_->GetPipelineDescriptorSetLayoutBindingValidity());
    const auto setIndices = pipelineDescriptorSetBinder_->GetSetIndices();
    const uint32_t firstSetIndex = pipelineDescriptorSetBinder_->GetFirstSet();
    for (const auto refIndex : setIndices) {
        const auto descHandle = pipelineDescriptorSetBinder_->GetDescriptorSetHandle(refIndex);
        // handle automatic set 0 bindings
        if ((refIndex == 0) && useAutoBindSet0_) {
            auto& binder = *pipelineDescriptorSetBinder_;
            binder.BindBuffer(GLOBAL_POST_PROCESS_SET, 0u, BindableBuffer { ubos_.postProcess.GetHandle() });
            binder.BindBuffer(GLOBAL_POST_PROCESS_SET, 1u,
                BindableBuffer { ubos_.postProcess.GetHandle(), sizeof(GlobalPostProcessStruct) });
        } else if (invalidBindings) {
            const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(refIndex);
            BindDefaultResources(refIndex, bindings);
        }
        const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(refIndex);
        cmdList.UpdateDescriptorSet(descHandle, bindings);
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!pipelineDescriptorSetBinder_->GetPipelineDescriptorSetLayoutBindingValidity()) {
        PLUGIN_LOG_W("RN: %s, bindings missing", renderNodeContextMgr_->GetName().data());
    }
#endif

    if (graphics_) {
        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    }

    cmdList.BindPipeline(psoHandle_);

    // bind all sets
    cmdList.BindDescriptorSets(firstSetIndex, pipelineDescriptorSetBinder_->GetDescriptorSetHandles());

    if (graphics_) {
        // dynamic state
        const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);

        if (pipelineLayout_.pushConstant.byteSize > 0) {
            const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
            const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
            const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                ppLocalConfig_.variables.factor };
            cmdList.PushConstantData(pipelineLayout_.pushConstant, arrayviewU8(pc));
        }

        cmdList.Draw(3u, 1u, 0u, 0u); // vertex count, instance count, first vertex, first instance
        cmdList.EndRenderPass();
    } else {
        if (RenderHandleUtil::IsValid(dispatchResources.buffer)) {
            cmdList.DispatchIndirect(dispatchResources.buffer, 0);
        } else if (RenderHandleUtil::IsValid(dispatchResources.image)) {
            const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
            const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(dispatchResources.image);
            const Math::UVec3 targetSize = { desc.width, desc.height, desc.depth };
            if (pipelineLayout_.pushConstant.byteSize > 0) {
                const float fWidth = static_cast<float>(targetSize.x);
                const float fHeight = static_cast<float>(targetSize.y);
                const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                    ppLocalConfig_.variables.factor };
                cmdList.PushConstantData(pipelineLayout_.pushConstant, arrayviewU8(pc));
            }

            cmdList.Dispatch((targetSize.x + threadGroupSize_.x - 1u) / threadGroupSize_.x,
                (targetSize.y + threadGroupSize_.y - 1u) / threadGroupSize_.y,
                (targetSize.z + threadGroupSize_.z - 1u) / threadGroupSize_.z);
        }
    }
}

void RenderNodeSinglePostProcess::RegisterOutputs(const RenderHandle output)
{
    IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
    RenderHandle registerOutput;
    if (ppLocalConfig_.variables.enabled) {
        if (RenderHandleUtil::IsValid(output)) {
            registerOutput = output;
        }
    }
    if (!RenderHandleUtil::IsValid(registerOutput)) {
        if (((jsonInputs_.defaultOutputImage == DefaultOutputImage::OUTPUT) ||
                (jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT_OUTPUT_COPY)) &&
            RenderHandleUtil::IsValid(output)) {
            registerOutput = output;
        } else if ((jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT) &&
                   RenderHandleUtil::IsValid(builtInVariables_.input)) {
            registerOutput = builtInVariables_.input;
        } else if (jsonInputs_.defaultOutputImage == DefaultOutputImage::WHITE) {
            registerOutput = builtInVariables_.defWhiteImage;
        } else {
            registerOutput = builtInVariables_.defBlackImage;
        }
    }
    shrMgr.RegisterRenderNodeOutput("output", registerOutput);
}

void RenderNodeSinglePostProcess::BindDefaultResources(
    const uint32_t set, const DescriptorSetLayoutBindingResources& bindings)
{
    if (pipelineDescriptorSetBinder_) {
        auto& binder = *pipelineDescriptorSetBinder_;
        for (const auto& ref : bindings.buffers) {
            if (!RenderHandleUtil::IsValid(ref.resource.handle)) {
                binder.BindBuffer(set, ref.binding.binding, BindableBuffer { builtInVariables_.defBuffer });
            }
        }
        for (const auto& ref : bindings.images) {
            if (!RenderHandleUtil::IsValid(ref.resource.handle)) {
                BindableImage bi;
                bi.handle = builtInVariables_.defBlackImage;
                bi.samplerHandle = builtInVariables_.defSampler;
                binder.BindImage(set, ref.binding.binding, bi);
            }
        }
        for (const auto& ref : bindings.samplers) {
            if (!RenderHandleUtil::IsValid(ref.resource.handle)) {
                binder.BindSampler(set, ref.binding.binding, BindableSampler { builtInVariables_.defSampler });
            }
        }
    }
}

void RenderNodeSinglePostProcess::ProcessPostProcessConfiguration()
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName); ds) {
            if (jsonInputs_.renderDataStore.typeName == RenderDataStorePostProcess::TYPE_NAME) {
                auto* const dataStore = static_cast<IRenderDataStorePostProcess const*>(ds);
                ppLocalConfig_ = dataStore->Get(jsonInputs_.renderDataStore.configurationName, jsonInputs_.ppName);
            }
        }
        if (const IRenderDataStorePod* ds =
                static_cast<const IRenderDataStorePod*>(dsMgr.GetRenderDataStore(RENDER_DATA_STORE_POD_NAME));
            ds) {
            auto const dataView = ds->Get(jsonInputs_.renderDataStore.configurationName);
            if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                ppGlobalConfig_ = *((const PostProcessConfiguration*)dataView.data());
            }
        }
    } else if (jsonInputs_.ppName.empty()) {
        // if trying to just use shader without post processing we enable running by default
        ppLocalConfig_.variables.enabled = true;
    }
}

void RenderNodeSinglePostProcess::UpdateGlobalPostProcessUbo()
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const RenderPostProcessConfiguration rppc =
        renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(ppGlobalConfig_);
    PLUGIN_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    PLUGIN_STATIC_ASSERT(sizeof(LocalPostProcessStruct) ==
                         sizeof(IRenderDataStorePostProcess::PostProcess::Variables::customPropertyData));
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(ubos_.postProcess.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(GlobalPostProcessStruct) + sizeof(LocalPostProcessStruct);
        // global data
        CloneData(data, size_t(dataEnd - data), &rppc, sizeof(GlobalPostProcessStruct));
        // local data
        data += sizeof(GlobalPostProcessStruct);
        CloneData(
            data, size_t(dataEnd - data), ppLocalConfig_.variables.customPropertyData, sizeof(LocalPostProcessStruct));
        gpuResourceMgr.UnmapBuffer(ubos_.postProcess.GetHandle());
    }
}

void RenderNodeSinglePostProcess::InitCreateBinders()
{
    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    DescriptorCounts dc;
    if (builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLOOM_BIT) {
        dc = renderBloom_.GetDescriptorCounts();
    } else if (builtInVariables_.postProcessFlag & PostProcessConfiguration::ENABLE_BLUR_BIT) {
        dc = renderBlur_.GetDescriptorCounts();
    } else {
        dc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
    }
    if (jsonInputs_.defaultOutputImage == DefaultOutputImage::INPUT_OUTPUT_COPY) {
        const DescriptorCounts copyDc = renderCopy_.GetDescriptorCounts();
        for (const auto& ref : copyDc.counts) {
            dc.counts.push_back(ref);
        }
    }
    descriptorSetMgr.ResetAndReserve(dc);

    pipelineDescriptorSetBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(pipelineLayout_);
    if (pipelineDescriptorSetBinder_) {
        renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
        if (NeedsAutoBindingSet0(inputResources_)) {
            useAutoBindSet0_ = true;
        }
    } else {
        valid_ = false;
    }
}

void RenderNodeSinglePostProcess::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.dispatchResources = parserUtil.GetInputResources(jsonVal, "dispatchResources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    jsonInputs_.ppName = parserUtil.GetStringValue(jsonVal, "postProcess");
    builtInVariables_.postProcessFlag = GetPostProcessFlag(jsonInputs_.ppName);

#if (RENDER_VALIDATION_ENABLED == 1)
    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_W("RENDER_VALIDATION: RN %s renderDataStore::dataStoreName missing.",
            renderNodeContextMgr_->GetName().data());
    }
    if (jsonInputs_.ppName.empty()) {
        PLUGIN_LOG_W("RENDER_VALIDATION: RN %s postProcess name missing.", renderNodeContextMgr_->GetName().data());
    }
#endif

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    if (!shaderName.empty()) {
        shader_ = shaderMgr.GetShaderHandle(shaderName);
    }
    const auto defaultOutput = parserUtil.GetStringValue(jsonVal, "defaultOutputImage");
    if (!defaultOutput.empty()) {
        if (defaultOutput == "output") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::OUTPUT;
        } else if (defaultOutput == "input_output_copy") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::INPUT_OUTPUT_COPY;
        } else if (defaultOutput == "input") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::INPUT;
        } else if (defaultOutput == "black") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::BLACK;
        } else if (defaultOutput == "white") {
            jsonInputs_.defaultOutputImage = DefaultOutputImage::WHITE;
        } else {
            PLUGIN_LOG_W("RenderNodeSinglePostProcess default output image not supported (%s)", defaultOutput.c_str());
        }
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    dispatchResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.dispatchResources);

    jsonInputs_.hasChangeableRenderPassHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.renderPass);
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);
    jsonInputs_.hasChangeableDispatchHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.dispatchResources);

    // process custom resources
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customInputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customInputImages[idx];
        if (ref.usageName == INPUT) {
            jsonInputs_.inputIdx = idx;
        }
    }
    for (uint32_t idx = 0; idx < static_cast<uint32_t>(jsonInputs_.resources.customOutputImages.size()); ++idx) {
        const auto& ref = jsonInputs_.resources.customOutputImages[idx];
        if (ref.usageName == OUTPUT) {
            jsonInputs_.outputIdx = idx;
        }
    }
}

void RenderNodeSinglePostProcess::UpdateImageData()
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBuffer)) {
        builtInVariables_.defBuffer = ubos_.postProcess.GetHandle();
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defBlackImage)) {
        builtInVariables_.defBlackImage =
            gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defWhiteImage)) {
        builtInVariables_.defWhiteImage =
            gpuResourceMgr.GetImageHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_GPU_IMAGE_WHITE);
    }
    if (!RenderHandleUtil::IsValid(builtInVariables_.defSampler)) {
        builtInVariables_.defSampler =
            gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
    }
    if (jsonInputs_.inputIdx < inputResources_.customInputImages.size()) {
        builtInVariables_.input = inputResources_.customInputImages[jsonInputs_.inputIdx].handle;
    }
    if (jsonInputs_.outputIdx < inputResources_.customOutputImages.size()) {
        builtInVariables_.output = inputResources_.customOutputImages[jsonInputs_.outputIdx].handle;
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeSinglePostProcess::Create()
{
    return new RenderNodeSinglePostProcess();
}

void RenderNodeSinglePostProcess::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeSinglePostProcess*>(instance);
}
RENDER_END_NAMESPACE()
