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

#include "render_node_mip_chain_post_process.h"

#include <algorithm>

#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/math/mathf.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/shaders/common/render_post_process_structs_common.h>

#include "datastore/render_data_store_pod.h"
#include "datastore/render_data_store_post_process.h"
#include "default_engine_constants.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t GLOBAL_POST_PROCESS_SET { 0u };
constexpr uint32_t LOCAL_POST_PROCESS_SET { 1u };
constexpr string_view RENDER_DATA_STORE_POD_NAME { RenderDataStorePod::TYPE_NAME };
constexpr string_view INPUT = "input";
constexpr string_view OUTPUT = "output";

constexpr string_view POST_PROCESS_BASE_PIPELINE_LAYOUT { "renderpipelinelayouts://post_process_common.shaderpl" };

constexpr uint32_t MAX_MIP_COUNT { 16u };
constexpr uint32_t MAX_PASS_PER_LEVEL_COUNT { 1u };
constexpr uint32_t MAX_LOCAL_BINDER_COUNT = MAX_MIP_COUNT * MAX_PASS_PER_LEVEL_COUNT;

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

inline BindableImage GetBindableImage(const RenderHandle& res)
{
    return BindableImage { res, PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS, ImageLayout::CORE_IMAGE_LAYOUT_UNDEFINED, {} };
}
} // namespace

void RenderNodeMipChainPostProcess::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    valid_ = true;
    ubos_.postProcess =
        CreatePostProcessDataUniformBuffer(renderNodeContextMgr.GetGpuResourceManager(), ubos_.postProcess);

    ParseRenderNodeInputs();
    UpdateImageData();
    ProcessPostProcessConfiguration();

    if (RenderHandleUtil::GetHandleType(pipelineData_.shader) != RenderHandleType::SHADER_STATE_OBJECT) {
        PLUGIN_LOG_E("CORE_RN_MIP_CHAIN_POST_PROCESS needs a valid shader handle");
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const RenderHandleType handleType = RenderHandleUtil::GetHandleType(pipelineData_.shader);
    if (handleType == RenderHandleType::SHADER_STATE_OBJECT) {
        pipelineData_.graphics = true;
    } else if (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        pipelineData_.graphics = false;
        pipelineData_.threadGroupSize = shaderMgr.GetReflectionThreadGroupSize(pipelineData_.shader);
        if (dispatchResources_.customInputBuffers.empty() && dispatchResources_.customInputImages.empty()) {
            valid_ = false;
            PLUGIN_LOG_W("CORE_RN_MIP_CHAIN_POST_PROCESS: dispatchResources (GPU buffer or GPU image) needed");
        }
    }

    {
        if (!((handleType == RenderHandleType::SHADER_STATE_OBJECT) ||
                (handleType == RenderHandleType::COMPUTE_SHADER_STATE_OBJECT))) {
            PLUGIN_LOG_E("RN:%s needs a valid shader handle", renderNodeContextMgr_->GetName().data());
        }
        pipelineData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayoutHandle(pipelineData_.shader);
        pipelineData_.pipelineLayoutData = shaderMgr.GetPipelineLayout(pipelineData_.pipelineLayout);
        if (pipelineData_.graphics) {
            const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(pipelineData_.shader);
            pipelineData_.pso = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(pipelineData_.shader,
                graphicsState, pipelineData_.pipelineLayoutData, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        } else {
            pipelineData_.pso = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(
                pipelineData_.shader, pipelineData_.pipelineLayout, {});
        }

        // NOTE: cannot be compared with shaderMgr.GetCompatibilityFlags due to missing pipeline layout handle
        const RenderHandle basePlHandle = shaderMgr.GetPipelineLayoutHandle(POST_PROCESS_BASE_PIPELINE_LAYOUT);
        const IShaderManager::CompatibilityFlags compatibilityFlags =
            shaderMgr.GetCompatibilityFlags(basePlHandle, pipelineData_.pipelineLayout);
        if ((compatibilityFlags & IShaderManager::CompatibilityFlagBits::COMPATIBLE_BIT) == 0) {
            PLUGIN_LOG_E("RN:%s uncompatible pipeline layout to %s", renderNodeContextMgr_->GetName().data(),
                POST_PROCESS_BASE_PIPELINE_LAYOUT.data());
        }
    }

    {
        DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineData_.pipelineLayoutData);
        // NOTE: hard-coded mip chain
        dc.counts.push_back({ CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MAX_LOCAL_BINDER_COUNT });
        dc.counts.push_back({ CORE_DESCRIPTOR_TYPE_SAMPLER, MAX_LOCAL_BINDER_COUNT });
        const DescriptorCounts copyDc = renderCopy_.GetDescriptorCounts();
        for (const auto& ref : copyDc.counts) {
            dc.counts.push_back(ref);
        }
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
    }

    {
        binders_.clear();
        binders_.resize(MAX_LOCAL_BINDER_COUNT);

        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
        {
            const auto& bindings =
                pipelineData_.pipelineLayoutData.descriptorSetLayouts[GLOBAL_POST_PROCESS_SET].bindings;
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            globalSet0_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
        const auto& bindings = pipelineData_.pipelineLayoutData.descriptorSetLayouts[LOCAL_POST_PROCESS_SET].bindings;
        for (uint32_t idx = 0; idx < MAX_LOCAL_BINDER_COUNT; ++idx) {
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            binders_[idx] = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
    }

    renderCopy_.Init(renderNodeContextMgr, {});
    RegisterOutputs(builtInVariables_.output);
}

void RenderNodeMipChainPostProcess::PreExecuteFrame()
{
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    UpdateImageData();
    ProcessPostProcessConfiguration();
    if (pipelineData_.graphics && GetRequiresPreCopy()) {
        RenderCopy::CopyInfo copyInfo { GetBindableImage(builtInVariables_.input),
            GetBindableImage(builtInVariables_.output), {} };
        renderCopy_.PreExecute(*renderNodeContextMgr_, copyInfo);
    }
    RegisterOutputs(builtInVariables_.output);
}

void RenderNodeMipChainPostProcess::ExecuteFrame(IRenderCommandList& cmdList)
{
    if ((!ppLocalConfig_.variables.enabled) &&
        (jsonInputs_.defaultOutputImage != DefaultOutputImage::INPUT_OUTPUT_COPY)) {
        return;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    }
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    }
    if (jsonInputs_.hasChangeableDispatchHandles) {
        dispatchResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.dispatchResources);
    }
    if (useAutoBindSet0_) {
        UpdateGlobalPostProcessUbo();
    }

    if (pipelineData_.graphics) {
        RenderGraphics(cmdList);
    } else {
        RenderCompute(cmdList);
    }
}

IRenderNode::ExecuteFlags RenderNodeMipChainPostProcess::GetExecuteFlags() const
{
    if ((!ppLocalConfig_.variables.enabled) &&
        (jsonInputs_.defaultOutputImage != DefaultOutputImage::INPUT_OUTPUT_COPY)) {
        return ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
    return ExecuteFlagBits::EXECUTE_FLAG_BITS_DEFAULT;
}

void RenderNodeMipChainPostProcess::ProcessPostProcessConfiguration()
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName)) {
            if (jsonInputs_.renderDataStore.typeName == RenderDataStorePostProcess::TYPE_NAME) {
                auto* const dataStore = static_cast<IRenderDataStorePostProcess const*>(ds);
                ppLocalConfig_ = dataStore->Get(jsonInputs_.renderDataStore.configurationName, jsonInputs_.ppName);
            }
        }
        if (const IRenderDataStorePod* ds =
                static_cast<const IRenderDataStorePod*>(dsMgr.GetRenderDataStore(RENDER_DATA_STORE_POD_NAME))) {
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

void RenderNodeMipChainPostProcess::UpdateGlobalPostProcessUbo()
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

// constants for RenderNodeMipChainPostProcess::RenderData
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

bool RenderNodeMipChainPostProcess::GetRequiresPreCopy() const
{
    // check for first copy from another image
    if (builtInVariables_.input != builtInVariables_.output) {
        return true;
    } else {
        return false;
    }
}

RenderPass RenderNodeMipChainPostProcess::GetBaseRenderPass()
{
    RenderPass renderPass = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    RenderHandle imageHandle = renderPass.renderPassDesc.attachmentHandles[0];
    // find the target image if not provided with the render pass
    if (!RenderHandleUtil::IsValid(imageHandle)) {
        if ((!inputResources_.customOutputImages.empty()))
            imageHandle = inputResources_.customOutputImages[0].handle;
    }
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachmentHandles[0] = imageHandle;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;
    return renderPass;
}

void RenderNodeMipChainPostProcess::UpdateGlobalSet(IRenderCommandList& cmdList)
{
    auto& binder = *globalSet0_;
    binder.ClearBindings();
    uint32_t binding = 0u;
    binder.BindBuffer(binding++, BindableBuffer { ubos_.postProcess.GetHandle() });
    binder.BindBuffer(binding++, BindableBuffer { ubos_.postProcess.GetHandle(), sizeof(GlobalPostProcessStruct) });
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

void RenderNodeMipChainPostProcess::RenderGraphics(IRenderCommandList& cmdList)
{
    RenderPass renderPass = GetBaseRenderPass();

    const RenderHandle imageHandle = renderPass.renderPassDesc.attachmentHandles[0];
    if (!RenderHandleUtil::IsValid(imageHandle)) {
        return; // early out
    }
    const GpuImageDesc imageDesc = renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(imageHandle);
    if (imageDesc.mipCount <= 1U) {
        return; // early out
    }
    renderPass.renderPassDesc.renderArea = { 0, 0, imageDesc.width, imageDesc.height };
    // check for first copy from another image
    if (builtInVariables_.input != builtInVariables_.output) {
        renderCopy_.Execute(*renderNodeContextMgr_, cmdList);
    }

    if constexpr (USE_CUSTOM_BARRIERS) {
        cmdList.BeginDisableAutomaticBarrierPoints();
    }

    UpdateGlobalSet(cmdList);

    ImageSubresourceRange imageSubresourceRange { CORE_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    for (uint32_t idx = 1U; idx < imageDesc.mipCount; ++idx) {
        const uint32_t renderPassMipLevel = jsonInputs_.upRamp ? (imageDesc.mipCount - idx - 1) : idx;
        const uint32_t inputMipLevel = jsonInputs_.upRamp ? (imageDesc.mipCount - idx) : (idx - 1);

        const uint32_t width = std::max(1u, imageDesc.width >> renderPassMipLevel);
        const uint32_t height = std::max(1u, imageDesc.height >> renderPassMipLevel);
        const float fWidth = static_cast<float>(width);
        const float fHeight = static_cast<float>(height);

        renderPass.renderPassDesc.renderArea = { 0, 0, width, height };
        renderPass.renderPassDesc.attachments[0].mipLevel = renderPassMipLevel;

        if constexpr (USE_CUSTOM_BARRIERS) {
            imageSubresourceRange.baseMipLevel = renderPassMipLevel;
            cmdList.CustomImageBarrier(imageHandle, SRC_UNDEFINED, COL_ATTACHMENT, imageSubresourceRange);

            imageSubresourceRange.baseMipLevel = inputMipLevel;
            if (inputMipLevel == 0) {
                cmdList.CustomImageBarrier(imageHandle, SHDR_READ, imageSubresourceRange);
            } else {
                cmdList.CustomImageBarrier(imageHandle, COL_ATTACHMENT, SHDR_READ, imageSubresourceRange);
            }

            cmdList.AddCustomBarrierPoint();
        }

        // update local descriptor set
        {
            // hard-coded
            auto& binder = *binders_[idx];
            uint32_t binding = 0;
            binder.BindSampler(binding++, BindableSampler { samplerHandle_ });
            binder.BindImage(binding++, BindableImage { imageHandle, inputMipLevel });
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
#if (RENDER_VALIDATION_ENABLED == 1)
            if (!binder.GetDescriptorSetLayoutBindingValidity()) {
                PLUGIN_LOG_W("RN: %s, bindings missing", renderNodeContextMgr_->GetName().data());
            }
#endif
        }

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

        cmdList.SetDynamicStateViewport(ViewportDesc { 0.0f, 0.0f, fWidth, fHeight, 0.0f, 0.0f });
        cmdList.SetDynamicStateScissor(ScissorDesc { 0, 0, width, height });

        cmdList.BindPipeline(pipelineData_.pso);

        // bind all sets
        {
            RenderHandle sets[2U] {};
            sets[0U] = globalSet0_->GetDescriptorSetHandle();
            sets[1U] = binders_[idx]->GetDescriptorSetHandle();
            cmdList.BindDescriptorSets(0u, sets);
        }

        // push constants
        if (pipelineData_.pipelineLayoutData.pushConstant.byteSize > 0) {
            const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight },
                ppLocalConfig_.variables.factor };
            cmdList.PushConstantData(pipelineData_.pipelineLayoutData.pushConstant, arrayviewU8(pc));
        }

        cmdList.Draw(3U, 1U, 0U, 0U);
        cmdList.EndRenderPass();
    }

    if constexpr (USE_CUSTOM_BARRIERS) {
        if (imageDesc.mipCount > 1U) {
            // transition the final mip level
            imageSubresourceRange.baseMipLevel = imageDesc.mipCount - 1;
            cmdList.CustomImageBarrier(imageHandle, FINAL_SRC, FINAL_DST, imageSubresourceRange);
        }
        cmdList.AddCustomBarrierPoint();
        cmdList.EndDisableAutomaticBarrierPoints();
    }
}

void RenderNodeMipChainPostProcess::RenderCompute(IRenderCommandList& cmdList)
{
    // NOTE: not yet supported
}

void RenderNodeMipChainPostProcess::BindDefaultResources(
    const uint32_t set, const DescriptorSetLayoutBindingResources& bindings)
{
    // NOTE: not yet supported
}

void RenderNodeMipChainPostProcess::RegisterOutputs(const RenderHandle output)
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

void RenderNodeMipChainPostProcess::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    pipelineData_.shader = shaderMgr.GetShaderHandle(shaderName);
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.dispatchResources = parserUtil.GetInputResources(jsonVal, "dispatchResources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");
    jsonInputs_.renderDataStoreSpecialization =
        parserUtil.GetRenderDataStore(jsonVal, "renderDataStoreShaderSpecialization");
    const auto ramp = parserUtil.GetStringValue(jsonVal, "ramp");
    jsonInputs_.upRamp = ramp == "up";
    jsonInputs_.ppName = parserUtil.GetStringValue(jsonVal, "postProcess");

#if (RENDER_VALIDATION_ENABLED == 1)
    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_W("RENDER_VALIDATION: RN %s renderDataStore::dataStoreName missing.",
            renderNodeContextMgr_->GetName().data());
    }
    if (jsonInputs_.ppName.empty()) {
        PLUGIN_LOG_W("RENDER_VALIDATION: RN %s postProcess name missing.", renderNodeContextMgr_->GetName().data());
    }
#endif

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
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

    // pick a sampler from the input resources which matches the layout location set=1, binding=0
    if (!inputResources_.samplers.empty()) {
        if (auto pos = std::find_if(inputResources_.samplers.cbegin(), inputResources_.samplers.cend(),
                [](const RenderNodeResource& sampler) {
                    return (sampler.set == 1U) && (sampler.binding == 0U) &&
                           RenderHandleUtil::IsValid(sampler.handle) &&
                           RenderHandleUtil::GetHandleType(sampler.handle) == RenderHandleType::GPU_SAMPLER;
                });
            pos != inputResources_.samplers.cend()) {
            samplerHandle_ = pos->handle;
        }
    }
}

void RenderNodeMipChainPostProcess::UpdateImageData()
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
    if (!RenderHandleUtil::IsValid(samplerHandle_)) {
        builtInVariables_.defSampler =
            gpuResourceMgr.GetSamplerHandle(DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
        if (!RenderHandleUtil::IsValid(builtInVariables_.defSampler)) {
            samplerHandle_ = builtInVariables_.defSampler;
        }
    }
    if (jsonInputs_.inputIdx < inputResources_.customInputImages.size()) {
        builtInVariables_.input = inputResources_.customInputImages[jsonInputs_.inputIdx].handle;
    }
    if (jsonInputs_.outputIdx < inputResources_.customOutputImages.size()) {
        builtInVariables_.output = inputResources_.customOutputImages[jsonInputs_.outputIdx].handle;
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeMipChainPostProcess::Create()
{
    return new RenderNodeMipChainPostProcess();
}

void RenderNodeMipChainPostProcess::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeMipChainPostProcess*>(instance);
}
RENDER_END_NAMESPACE()
