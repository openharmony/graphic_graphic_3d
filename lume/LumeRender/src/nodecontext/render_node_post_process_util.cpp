/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <core/property/property_handle_util.h>
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
#include <render/property/property_types.h>

#include "datastore/render_data_store_pod.h"
#include "datastore/render_data_store_post_process.h"
#include "default_engine_constants.h"
#include "device/gpu_resource_handle_util.h"
#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "postprocesses/render_post_process_bloom_node.h"
#include "postprocesses/render_post_process_blur_node.h"
#include "postprocesses/render_post_process_combined_node.h"
#include "postprocesses/render_post_process_dof_node.h"
#include "postprocesses/render_post_process_flare_node.h"
#include "postprocesses/render_post_process_fxaa_node.h"
#include "postprocesses/render_post_process_motion_blur_node.h"
#include "postprocesses/render_post_process_taa_node.h"
#include "postprocesses/render_post_process_upscale_node.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_layout_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr uint32_t GL_LAYER_CANNOT_OPT_FLAGS = PostProcessConfiguration::ENABLE_BLOOM_BIT |
                                               PostProcessConfiguration::ENABLE_TAA_BIT |
                                               PostProcessConfiguration::ENABLE_DOF_BIT;

constexpr string_view INPUT_COLOR = "color";
constexpr string_view INPUT_DEPTH = "depth";
constexpr string_view INPUT_VELOCITY = "velocity_normal";
constexpr string_view INPUT_HISTORY = "history";
constexpr string_view INPUT_HISTORY_NEXT = "history_next";

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
    CreatePostProcessInterfaces();
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

    // call post process interface inits before descriptor set allocation
    {
        if (ppLensFlareInterface_.postProcessNode) {
            ppLensFlareInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderUpscaleInterface_.postProcessNode) {
            ppRenderUpscaleInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderBlurInterface_.postProcessNode) {
            ppRenderBlurInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderTaaInterface_.postProcessNode) {
            ppRenderTaaInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderFxaaInterface_.postProcessNode) {
            ppRenderFxaaInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderDofInterface_.postProcessNode) {
            ppRenderDofInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderMotionBlurInterface_.postProcessNode) {
            ppRenderMotionBlurInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderBloomInterface_.postProcessNode) {
            ppRenderBloomInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
        if (ppRenderCombinedInterface_.postProcessNode) {
            ppRenderCombinedInterface_.postProcessNode->InitNode(*renderNodeContextMgr_);
        }
    }
    {
        vector<DescriptorCounts> descriptorCounts;
        descriptorCounts.reserve(16U);
        // pre-set custom descriptor sets
        descriptorCounts.push_back(DescriptorCounts { {
            { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 8u },
            { CORE_DESCRIPTOR_TYPE_SAMPLER, 8u },
            { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 8u },
        } });
        descriptorCounts.push_back(renderCopyLayer_.GetRenderDescriptorCounts());
        descriptorCounts.push_back(renderNodeUtil.GetDescriptorCounts(copyData_.sd.pipelineLayoutData));
        // interfaces
        if (ppRenderBlurInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderBlurInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        if (ppRenderTaaInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderTaaInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        if (ppRenderFxaaInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderFxaaInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        if (ppRenderDofInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderDofInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        if (ppRenderMotionBlurInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderMotionBlurInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        if (ppRenderBloomInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderBloomInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        if (ppRenderCombinedInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderCombinedInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        if (ppRenderUpscaleInterface_.postProcessNode) {
            descriptorCounts.push_back(ppRenderUpscaleInterface_.postProcessNode->GetRenderDescriptorCounts());
        }
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(descriptorCounts);
    }

    ProcessPostProcessConfiguration();

    if (deviceBackendType_ != DeviceBackendType::VULKAN) {
        // prepare for possible layer copy
        renderCopyLayer_.Init(renderNodeContextMgr);
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
    const bool upscaleEnabled =
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_UPSCALE_BIT) &&
        ppConfig_.upscaleConfiguration.ratio != 1.0f;
    const bool fxaaEnabled =
        ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT;
    const bool motionBlurEnabled =
        validInputsForMb_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_MOTION_BLUR_BIT);
    const bool dofEnabled =
        validInputsForDof_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DOF_BIT);

    ti_.idx = 0;
    const bool basicImagesNeeded = upscaleEnabled || fxaaEnabled || motionBlurEnabled || dofEnabled;
    const bool sizeChanged = (outputDesc.width != outputSize_.x) || (outputDesc.height != outputSize_.y);
    if (sizeChanged) {
        outputSize_ = { Math::max(1U, outputDesc.width), Math::max(1U, outputDesc.height) };
        ti_.targetSize.x = static_cast<float>(outputSize_.x);
        ti_.targetSize.y = static_cast<float>(outputSize_.y);
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
            if (upscaleEnabled) {
                // make sure the upscaler outputs the correct format for bloom and tonemapping
                tmpDesc = inputDesc;
                tmpDesc.width = outputDesc.width;
                tmpDesc.height = outputDesc.height;
            }
            FillTmpImageDesc(tmpDesc);
            ti_.images[0U] = gpuResourceMgr.Create(ti_.images[0U], tmpDesc);
            ti_.images[1U] = gpuResourceMgr.Create(ti_.images[1U], tmpDesc);
            ti_.imageCount = 2U;
        }
    } else {
        ti_.images[0U] = {};
        ti_.images[1U] = {};
    }

    BindableImage postTaaInput = images_.input;

    // prepare for possible layer copy
    glOptimizedLayerCopyEnabled_ = false;
    if ((deviceBackendType_ != DeviceBackendType::VULKAN) &&
        (images_.input.layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS)) {
        if ((ppConfig_.enableFlags & GL_LAYER_CANNOT_OPT_FLAGS) == 0U) {
            // optimize with combine
            glOptimizedLayerCopyEnabled_ = true;
        } else {
            if (sizeChanged || (!ti_.layerCopyImage)) {
                GpuImageDesc tmpDesc = inputDesc; // NOTE: input desc
                FillTmpImageDesc(tmpDesc);
                ti_.layerCopyImage = gpuResourceMgr.Create(ti_.layerCopyImage, tmpDesc);
            }

            BindableImage layerCopyOutput;
            layerCopyOutput.handle = ti_.layerCopyImage.GetHandle();
            renderCopyLayer_.PreExecute();

            postTaaInput = layerCopyOutput;
        }
    }

    // post-processing chain is TAA -> upscale -> lens flare -> bloom -> combined -> fxaa -> motion blur -> dof -> blur

    BindableImage postTaaOutput = postTaaInput;
    if (validInputsForTaa_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT)) {
        postTaaOutput = images_.historyNext;
        if (ppRenderTaaInterface_.postProcessNode) {
            // copy properties to new property post process
            auto& ppNode = static_cast<RenderPostProcessTaaNode&>(*ppRenderTaaInterface_.postProcessNode);
            ppNode.propertiesData.enabled = true;
            ppNode.propertiesData.taaConfiguration = ppConfig_.taaConfiguration;

            ppNode.nodeInputsData.input = postTaaInput;
            ppNode.nodeOutputsData.output = postTaaOutput;
            ppNode.nodeInputsData.depth = images_.depth;
            ppNode.nodeInputsData.velocity = images_.velocity;
            ppNode.nodeInputsData.history = images_.history;
            ppNode.PreExecuteFrame();
        }
    }

    framePostProcessInOut_.clear();
    BindableImage currentInput = postTaaOutput;
    const bool postProcessNeeded = (ppConfig_.enableFlags != 0);
    const bool sameInputOutput = (currentInput.handle == images_.output.handle);

    if (upscaleEnabled) {
        framePostProcessInOut_.push_back({ currentInput, GetIntermediateImage(currentInput.handle) });
        currentInput = framePostProcessInOut_.back().output;
    }

    const BindableImage postUpscaleInput = currentInput;

    if (postProcessNeeded && (!sameInputOutput)) {
        const BindableImage output = (fxaaEnabled || motionBlurEnabled || dofEnabled)
                                         ? GetIntermediateImage(currentInput.handle)
                                         : images_.output;
        framePostProcessInOut_.push_back({ currentInput, output });
        currentInput = framePostProcessInOut_.back().output;
    }

    if (fxaaEnabled) {
        framePostProcessInOut_.push_back({ currentInput, GetIntermediateImage(currentInput.handle) });
        currentInput = framePostProcessInOut_.back().output;
    }

    if (motionBlurEnabled) {
        framePostProcessInOut_.push_back({ currentInput, GetIntermediateImage(currentInput.handle) });
        currentInput = framePostProcessInOut_.back().output;
    }

    if (dofEnabled) {
        framePostProcessInOut_.push_back({ currentInput, GetIntermediateImage(currentInput.handle) });
        currentInput = framePostProcessInOut_.back().output;
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT) {
        framePostProcessInOut_.push_back({ currentInput, currentInput });
        currentInput = framePostProcessInOut_.back().output;
    }

    // finalize
    if (!framePostProcessInOut_.empty()) {
        framePostProcessInOut_.back().output = images_.output;
    }

    uint32_t ppIdx = 0U;

    if (upscaleEnabled) {
        if (ppRenderUpscaleInterface_.postProcessNode) {
            auto& pp = static_cast<RenderPostProcessUpscaleNode&>(*ppRenderUpscaleInterface_.postProcessNode);
            pp.propertiesData.enabled = true;
            pp.propertiesData.upscaleConfiguration.ratio = ppConfig_.upscaleConfiguration.ratio;
            pp.propertiesData.upscaleConfiguration.smoothScale = ppConfig_.upscaleConfiguration.smoothScale;
            pp.propertiesData.upscaleConfiguration.structureSensitivity =
                ppConfig_.upscaleConfiguration.structureSensitivity;
            pp.propertiesData.upscaleConfiguration.edgeSharpness = ppConfig_.upscaleConfiguration.edgeSharpness;

            RenderPostProcessUpscaleNode& ppNode =
                static_cast<RenderPostProcessUpscaleNode&>(*ppRenderUpscaleInterface_.postProcessNode);
            ppNode.nodeInputsData.input = framePostProcessInOut_[ppIdx].input;
            ppNode.nodeOutputsData.output = framePostProcessInOut_[ppIdx].output;
            ppNode.PreExecuteFrame();
        }
        ppIdx++;
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_LENS_FLARE_BIT) {
        if (ppLensFlareInterface_.postProcessNode) {
            // copy properties to new property post process
            auto& ppNode = static_cast<RenderPostProcessFlareNode&>(*ppLensFlareInterface_.postProcessNode);
            ppNode.propertiesData.enabled = true;
            ppNode.propertiesData.flarePos = ppConfig_.lensFlareConfiguration.flarePosition;
            ppNode.propertiesData.intensity = ppConfig_.lensFlareConfiguration.intensity;

            ppNode.nodeInputsData.input = postUpscaleInput;
            ppNode.PreExecuteFrame();
        }
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT) {
        if (ppRenderBloomInterface_.postProcessNode) {
            // copy properties to new property post process
            auto& ppNode = static_cast<RenderPostProcessBloomNode&>(*ppRenderBloomInterface_.postProcessNode);
            ppNode.propertiesData.enabled = true;
            ppNode.propertiesData.bloomConfiguration = ppConfig_.bloomConfiguration;

            ppNode.nodeInputsData.input = postUpscaleInput;
            ppNode.PreExecuteFrame();
        }
    }

    if (postProcessNeeded && (!sameInputOutput)) {
        if (ppRenderCombinedInterface_.postProcessNode) {
            auto& ppNode = static_cast<RenderPostProcessCombinedNode&>(*ppRenderCombinedInterface_.postProcessNode);
            ppNode.propertiesData.enabled = true;
            ppNode.propertiesData.postProcessConfiguration = ppConfig_;
            ppNode.propertiesData.glOptimizedLayerCopyEnabled = glOptimizedLayerCopyEnabled_;

            ppNode.nodeInputsData.input = framePostProcessInOut_[ppIdx].input;
            ppNode.nodeOutputsData.output = framePostProcessInOut_[ppIdx].output;
            ppNode.nodeInputsData.dirtMask.handle = images_.dirtMask;
            RenderHandle bloomImage = aimg_.black;
            if ((ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT) &&
                ppRenderBloomInterface_.postProcessNode) {
                bloomImage = static_cast<RenderPostProcessBloomNode*>(ppRenderBloomInterface_.postProcessNode.get())
                                 ->GetFinalTarget();
            }
            ppNode.nodeInputsData.bloomFinalTarget.handle = bloomImage;
            ppNode.PreExecuteFrame();
        }
        ppIdx++;
    }

    if (fxaaEnabled) {
        if (ppRenderFxaaInterface_.postProcessNode) {
            // copy properties to new property post process
            auto& pp = static_cast<RenderPostProcessFxaaNode&>(*ppRenderFxaaInterface_.postProcessNode);
            pp.propertiesData.enabled = true;
            pp.propertiesData.fxaaConfiguration = ppConfig_.fxaaConfiguration;
            pp.propertiesData.targetSize = ti_.targetSize;

            RenderPostProcessFxaaNode& ppNode =
                static_cast<RenderPostProcessFxaaNode&>(*ppRenderFxaaInterface_.postProcessNode);
            ppNode.nodeInputsData.input = framePostProcessInOut_[ppIdx].input;
            ppNode.nodeOutputsData.output = framePostProcessInOut_[ppIdx].output;
            ppNode.PreExecuteFrame();
        }
        ppIdx++;
    }

    if (motionBlurEnabled) {
        if (ppRenderMotionBlurInterface_.postProcessNode) {
            auto& pp = static_cast<RenderPostProcessMotionBlurNode&>(*ppRenderMotionBlurInterface_.postProcessNode);
            pp.propertiesData.enabled = true;
            pp.propertiesData.motionBlurConfiguration = ppConfig_.motionBlurConfiguration;
            pp.propertiesData.size = outputSize_;

            RenderPostProcessMotionBlurNode& ppNode =
                static_cast<RenderPostProcessMotionBlurNode&>(*ppRenderMotionBlurInterface_.postProcessNode);
            ppNode.nodeInputsData.input = framePostProcessInOut_[ppIdx].input;
            ppNode.nodeOutputsData.output = framePostProcessInOut_[ppIdx].output;
            ppNode.nodeInputsData.velocity = images_.velocity;
            ppNode.nodeInputsData.depth = images_.depth;

            ppNode.PreExecuteFrame();
        }
        ppIdx++;
    }

    if (dofEnabled) {
        if (ppRenderDofInterface_.postProcessNode) {
            // copy properties to new property post process
            auto& ppNode = static_cast<RenderPostProcessDofNode&>(*ppRenderDofInterface_.postProcessNode);
            ppNode.propertiesData.enabled = true;
            ppNode.propertiesData.dofConfiguration = ppConfig_.dofConfiguration;

            ppNode.nodeInputsData.input = framePostProcessInOut_[ppIdx].input;
            ppNode.nodeOutputsData.output = framePostProcessInOut_[ppIdx].output;
            ppNode.nodeInputsData.depth = images_.depth;

            ppNode.PreExecuteFrame();
        }
        ppIdx++;
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT) {
        const auto& inOut = framePostProcessInOut_[ppIdx];
        if (ppRenderBlurInterface_.postProcessNode) {
            // copy properties to new property post process
            auto& ppNode = static_cast<RenderPostProcessBlurNode&>(*ppRenderBlurInterface_.postProcessNode);
            ppNode.propertiesData.enabled = true;
            ppNode.propertiesData.blurConfiguration = ppConfig_.blurConfiguration;

            ppNode.nodeInputsData.input = inOut.output;
            ppNode.PreExecuteFrame();
        }
        ppIdx++;
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

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderPostProcess", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    // prepare for possible layer copy if not using optimized paths for layers
    BindableImage currentInput = images_.input;
    if ((deviceBackendType_ != DeviceBackendType::VULKAN) &&
        (images_.input.layer != PipelineStateConstants::GPU_IMAGE_ALL_LAYERS) && (!glOptimizedLayerCopyEnabled_)) {
        BindableImage layerCopyOutput;
        layerCopyOutput.handle = ti_.layerCopyImage.GetHandle();
        IRenderNodeCopyUtil::CopyInfo layerCopyInfo;
        layerCopyInfo.input = images_.input;
        layerCopyInfo.output = layerCopyOutput;
        layerCopyInfo.copyType = IRenderNodeCopyUtil::CopyType::LAYER_COPY;
        renderCopyLayer_.Execute(cmdList, layerCopyInfo);
        currentInput = layerCopyOutput;
    }

    if (validInputsForTaa_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_TAA_BIT)) {
        if (ppRenderTaaInterface_.postProcessNode) {
            // inputs set in pre-execute
            ppRenderTaaInterface_.postProcessNode->ExecuteFrame(cmdList);
            currentInput = images_.historyNext;
        }
    }

    const bool upscaleEnabled =
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_UPSCALE_BIT) &&
        ppConfig_.upscaleConfiguration.ratio != 1.0f;
    const bool fxaaEnabled =
        ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_FXAA_BIT;
    const bool motionBlurEnabled =
        validInputsForMb_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_MOTION_BLUR_BIT);
    const bool dofEnabled =
        validInputsForDof_ &&
        (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_DOF_BIT);

    const bool postProcessNeeded = (ppConfig_.enableFlags != 0);
    const bool sameInputOutput = (currentInput.handle == images_.output.handle);
    uint32_t ppIdx = 0U;

#if (RENDER_VALIDATION_ENABLED == 1)
    if (postProcessNeeded && sameInputOutput) {
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName().data(),
            "%s: combined post process shader not supported for same input/output ATM.",
            renderNodeContextMgr_->GetName().data());
    }
#endif

    if (upscaleEnabled) {
        if (ppRenderUpscaleInterface_.postProcessNode) {
            ppRenderUpscaleInterface_.postProcessNode->ExecuteFrame(cmdList);
        }
        ppIdx++;
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_LENS_FLARE_BIT) {
        if (ppLensFlareInterface_.postProcessNode) {
            // inputs set in pre-execute
            ppLensFlareInterface_.postProcessNode->ExecuteFrame(cmdList);
        }
    }

    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLOOM_BIT) {
        if (ppRenderBloomInterface_.postProcessNode) {
            // inputs set in pre-execute
            ppRenderBloomInterface_.postProcessNode->ExecuteFrame(cmdList);
        }
    }

    if (postProcessNeeded && (!sameInputOutput)) {
        if (ppRenderCombinedInterface_.postProcessNode) {
            ppRenderCombinedInterface_.postProcessNode->ExecuteFrame(cmdList);
            ppIdx++;
        }
    }

    if (fxaaEnabled) {
        if (ppRenderFxaaInterface_.postProcessNode) {
            ppRenderFxaaInterface_.postProcessNode->ExecuteFrame(cmdList);
        }
        ppIdx++;
    }

    if (motionBlurEnabled) {
        if (ppRenderMotionBlurInterface_.postProcessNode) {
            ppRenderMotionBlurInterface_.postProcessNode->ExecuteFrame(cmdList);
        }
        ppIdx++;
    }

    if (dofEnabled) {
        if (ppRenderDofInterface_.postProcessNode) {
            ppRenderDofInterface_.postProcessNode->ExecuteFrame(cmdList);
        }
        ppIdx++;
    }

    // post blur
    if (ppConfig_.enableFlags & PostProcessConfiguration::PostProcessEnableFlagBits::ENABLE_BLUR_BIT) {
        if (ppRenderBlurInterface_.postProcessNode) {
            ppRenderBlurInterface_.postProcessNode->ExecuteFrame(cmdList);
        }
        ppIdx++;
    }

    PLUGIN_ASSERT(ppIdx == framePostProcessInOut_.size());

    // if all flags are zero and output is different than input
    // we need to execute a copy
    if ((!postProcessNeeded) && (!sameInputOutput)) {
        ExecuteBlit(cmdList, { images_.input, images_.output });
    }
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
        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderBlit", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

        RenderPass renderPass = CreateRenderPass(gpuResourceMgr, inOut.output.handle);
        auto& effect = copyData_;
        if (!RenderHandleUtil::IsValid(effect.pso)) {
            auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
            const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
            const RenderHandle graphicsStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(effect.sd.shader);
            effect.pso = psoMgr.GetGraphicsPsoHandle(effect.sd.shader, graphicsStateHandle, effect.sd.pipelineLayout,
                {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
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

void RenderNodePostProcessUtil::ProcessPostProcessConfiguration()
{
    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        auto& dsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        if (const IRenderDataStore* ds = dsMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName); ds) {
            if (jsonInputs_.renderDataStore.typeName == RenderDataStorePod::TYPE_NAME) {
                auto const dataStore = static_cast<const IRenderDataStorePod*>(ds);
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
    copyData_ = {};

    copyData_.sd = shaderMgr.GetShaderDataByShaderName(COPY_SHADER_NAME);
}

void RenderNodePostProcessUtil::InitCreateBinders()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    binders_.copyBinder = descriptorSetMgr.CreateDescriptorSetBinder(
        descriptorSetMgr.CreateDescriptorSet(0u, copyData_.sd.pipelineLayoutData),
        copyData_.sd.pipelineLayoutData.descriptorSetLayouts[0u].bindings);
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
        images_.inputUpscaled = postProcessInfo_.imageData.inputUpscaled;
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

    validInputsForTaa_ = validDepth && validHistory; // TAA can be used without velocities
    validInputsForDof_ = validDepth;
    validInputsForMb_ = validVelocity;
}

void RenderNodePostProcessUtil::CreatePostProcessInterfaces()
{
    auto* renderClassFactory = renderNodeContextMgr_->GetRenderContext().GetInterface<IClassFactory>();
    if (renderClassFactory) {
        auto CreatePostProcessInterface = [&](const auto uid, auto& ppNode) {
            ppNode = CreateInstance<IRenderPostProcessNode>(*renderClassFactory, uid);
        };

        CreatePostProcessInterface(RenderPostProcessFlareNode::UID, ppLensFlareInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessUpscaleNode::UID, ppRenderUpscaleInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessBlurNode::UID, ppRenderBlurInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessBloomNode::UID, ppRenderBloomInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessTaaNode::UID, ppRenderTaaInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessFxaaNode::UID, ppRenderFxaaInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessDofNode::UID, ppRenderDofInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessMotionBlurNode::UID, ppRenderMotionBlurInterface_.postProcessNode);

        CreatePostProcessInterface(RenderPostProcessCombinedNode::UID, ppRenderCombinedInterface_.postProcessNode);
    }
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
