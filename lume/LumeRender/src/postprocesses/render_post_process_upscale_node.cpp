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
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

#include "default_engine_constants.h"
#include "postprocesses/render_post_process_upscale.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RenderPostProcessUpscaleNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0),
    MEMBER_PROPERTY(depth, "depth", 0), MEMBER_PROPERTY(velocity, "velocity", 0))
DATA_TYPE_METADATA(RenderPostProcessUpscaleNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()

RenderPostProcessUpscaleNode::RenderPostProcessUpscaleNode()
    : inputProperties_(
          &nodeInputsData, array_view(PropertyType::DataType<RenderPostProcessUpscaleNode::NodeInputs>::properties)),
      outputProperties_(
          &nodeOutputsData, array_view(PropertyType::DataType<RenderPostProcessUpscaleNode::NodeOutputs>::properties))

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
        { CORE_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 32u },
        { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 32u },
        { CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, 32u },
        { CORE_DESCRIPTOR_TYPE_SAMPLER, 24u },
    } };
}

void RenderPostProcessUpscaleNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

void RenderPostProcessUpscaleNode::Init(
    const IRenderPostProcess::Ptr& postProcess, IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    postProcess_ = postProcess;
    renderCopyOutput_.Init(renderNodeContextMgr);

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    samplerHandle_ = gpuResourceMgr.Create(samplerHandle_,
        GpuSamplerDesc {
            Filter::CORE_FILTER_LINEAR,                                  // magFilter
            Filter::CORE_FILTER_LINEAR,                                  // minFilter
            Filter::CORE_FILTER_LINEAR,                                  // mipMapMode
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeU
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeV
            SamplerAddressMode::CORE_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, // addressModeW
        });
    SetCameraData();

    valid_ = true;
}

void RenderPostProcessUpscaleNode::PreExecute()
{
    if (valid_ && postProcess_) {
        const array_view<const uint8_t> propertyView = postProcess_->GetData();
        // this node is directly dependant
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessUpscaleNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessUpscaleNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessUpscaleNode::EffectProperties&)(*propertyView.data());
        }
        effectProperties_.ratio = 1.5f;
        const GpuImageDesc& imgDesc =
            renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(nodeInputsData.input.handle);

        mipLevels_ = Math::max(1U, TARGET_COUNT);
        // floor(log2(Math::max(imgDesc.width, imgDesc.height)));
        CreateTargets(Math::UVec2(imgDesc.width, imgDesc.height));
        if (effectProperties_.enabled) {
            // check input and output
            EvaluateOutput();
        }
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

void RenderPostProcessUpscaleNode::Execute(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }
    // NOTE: target counts etc. should probably be resized based on configuration

    CreatePsos();

    EvaluateOutput();
    BindableImage currOutput = nodeOutputsData.output;
    if (!RenderHandleUtil::IsValid(currOutput.handle)) {
        return;
    }

    constexpr PushConstant pc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT,
        sizeof(LocalPostProcessPushConstantStruct) };
    // update the output
    nodeOutputsData.output = currOutput;
    RENDER_DEBUG_MARKER_SCOPE(cmdList, "Upscaling (LSR)");
    {
        RENDER_DEBUG_MARKER_COL_SCOPE(
            cmdList, "Compute Luminance Hierarchy", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        ComputeLuminancePyramid(pc, cmdList);
    }
    {
        RENDER_DEBUG_MARKER_COL_SCOPE(
            cmdList, "Dilate and reconstruct", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        ComputeReconstructAndDilate(pc, cmdList);
    }
    {
        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "Depth clip", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        ComputeDepthClip(pc, cmdList);
    }
    {
        constexpr PushConstant lockPassPc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT,
            sizeof(LockPassPushConstant) };
        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "Create Locks", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        ComputeCreateLocks(lockPassPc, cmdList);
    }
    {
        constexpr PushConstant accumulatePassPc { ShaderStageFlagBits::CORE_SHADER_STAGE_COMPUTE_BIT,
            sizeof(AccumulatePassPushConstant) };
        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "Accumulate", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        ComputeAccumulate(accumulatePassPc, cmdList);
    }

    if (targets_.rcas_enabled) {
        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RCAS", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);
        ComputeRcas(pc, cmdList);
    }

    // Toggle between history and current motion vector texture
    targets_.motionVectorIdx ^= 1;
    targets_.historyBufferIdx ^= 1;
    IRenderNodeCopyUtil::CopyInfo copyInfo;
    BindableImage finalColor;

    targets_.rcas_enabled ? finalColor.handle = targets_.rcas_final.GetHandle()
                          : finalColor.handle = targets_.finalColor.GetHandle();
    copyInfo.input = finalColor;
    copyInfo.output = nodeOutputsData.output;
    renderCopyOutput_.Execute(cmdList, copyInfo);
}

void RenderPostProcessUpscaleNode::SetCameraData()
{
    // change this
    // Temporary workaround to send camera data from 3D to Render
    RenderHandle cameraUbo = renderNodeContextMgr_->GetGpuResourceManager().GetBufferHandle(
        "RenderDataStoreDefaultSceneCORE3D_DM_CAM_DATA_BUF");
    targets_.cameraUbo = cameraUbo;
}

void RenderPostProcessUpscaleNode::ComputeLuminancePyramid(const PushConstant& pc, IRenderCommandList& cmdList)
{
    // bind downscale pso
    cmdList.BindPipeline(psos_.luminanceDownscale);
    const ShaderThreadGroup tgs = psos_.luminanceDownscaleTGS;

    if (!RenderHandleUtil::IsValid(nodeInputsData.input.handle)) {
        return;
    }
    //-----------------------------------------------------------
    // Pass #1: generate luminance texture mip 0 by sampling the original input
    //-----------------------------------------------------------
    {
        auto& binder = binders_.luminanceDownscale;
        binder->ClearBindings();

        binder->BindImage(0U, { targets_.tex1[0].GetHandle() });
        binder->BindImage(1U, { nodeInputsData.input.handle });
        binder->BindSampler(2U, { samplerHandle_.GetHandle() });

        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

        const auto targetSize = targets_.tex1Size[0];
        LocalPostProcessPushConstantStruct uPc;
        uPc.viewportSizeInvSize = Math::Vec4(
            float(targetSize.x), float(targetSize.y), 1.0f / float(targetSize.x), 1.0f / float(targetSize.y));

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }

    // bind hierarchy pso
    cmdList.BindPipeline(psos_.luminancePyramid);
    const ShaderThreadGroup PyramidTgs = psos_.luminancePyramidTGS;
    ////-----------------------------------------------------------
    //// Pass #2..N: generate each subsequent mip from the previous
    ////-----------------------------------------------------------
    for (size_t i = 1; i < mipLevels_; ++i) {
        {
            auto& binder = binders_.luminancePyramid[i];
            const RenderHandle setHandle = binder->GetDescriptorSetHandle();
            binder->ClearBindings();

            binder->BindImage(0U, { targets_.tex1[i].GetHandle() });
            binder->BindImage(1U, { targets_.tex1[i - 1].GetHandle() });
            binder->BindSampler(2U, { samplerHandle_.GetHandle() });

            cmdList.UpdateDescriptorSet(
                binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0U, setHandle);
        }

        const auto targetSize = targets_.tex1Size[i];

        LocalPostProcessPushConstantStruct uPc;
        uPc.viewportSizeInvSize = Math::Vec4(
            float(targetSize.x), float(targetSize.y), 1.0f / float(targetSize.x), 1.0f / float(targetSize.y));
        cmdList.PushConstantData(pc, arrayviewU8(uPc));

        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderPostProcessUpscaleNode::ComputeReconstructAndDilate(
    const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList)
{
    cmdList.BindPipeline(psos_.reconstructAndDilate);
    const ShaderThreadGroup& tgs = psos_.reconstructAndDilateTGS;

    const uint32_t motionVecIdx = targets_.motionVectorIdx;

    auto& binder = binders_.reconstructAndDilate;
    if (!binder)
        return;
    {
        binder->ClearBindings();

        //  Bindings
        //  - 0: uPreviousDepth (R32_UINT)
        //  - 1: DilatedDepth  (R16_UINT)
        //  - 2: sampler
        //  - 3: DilatedMotion (R16G16F)
        //  - 4: LockInputLuma (R16F)
        //  - 5: Depth         (sampler2D)
        //  - 6: Velocity
        //  - 7: Color
        binder->BindImage(0U, { targets_.estPrevDepth.GetHandle() });
        binder->BindImage(1U, { targets_.dilatedDepth.GetHandle() });
        binder->BindSampler(2U, { samplerHandle_.GetHandle() });
        binder->BindImage(3U, { targets_.dilatedMotionVectors[motionVecIdx].GetHandle() });
        binder->BindImage(4U, { targets_.lockInputLuma.GetHandle() });
        binder->BindImage(5U, { nodeInputsData.depth.handle });
        binder->BindImage(6U, { nodeInputsData.velocity.handle });
        binder->BindImage(7U, { nodeInputsData.input.handle });

        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

        const auto targetSize = targets_.renderResolution;
        LocalPostProcessPushConstantStruct uPc;
        uPc.viewportSizeInvSize = Math::Vec4(
            float(targetSize.x), float(targetSize.y), 1.0f / float(targetSize.x), 1.0f / float(targetSize.y));
        cmdList.PushConstantData(pc, arrayviewU8(uPc));

        // Dispatch
        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderPostProcessUpscaleNode::ComputeDebug(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList)
{
    // bind pso
    cmdList.BindPipeline(psos_.debugPass);
    const ShaderThreadGroup tgs = psos_.debugPassTGS;

    auto& binder = binders_.debugPass;
    if (!binder) {
        return;
    }
    if (!RenderHandleUtil::IsValid(nodeInputsData.input.handle)) {
        return;
    }
    {
        binder->ClearBindings();

        binder->BindImage(0U, { targets_.estPrevDepth.GetHandle() });
        binder->BindImage(1U, { targets_.debugImage.GetHandle() });
        binder->BindSampler(2U, { samplerHandle_.GetHandle() });

        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

        const auto targetSize = targets_.renderResolution;

        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderPostProcessUpscaleNode::ComputeDepthClip(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList)
{
    // bind pso
    cmdList.BindPipeline(psos_.depthClipPass);
    const ShaderThreadGroup tgs = psos_.depthClipPassTGS;

    const uint32_t curMotionVecIdx = targets_.motionVectorIdx;
    const uint32_t prevMotionVecIdx = curMotionVecIdx ^ 1;

    auto& binder = binders_.depthClipPass;
    if (!binder) {
        return;
    }
    {
        binder->ClearBindings();

        //  Bindings
        //  - 0: estPrevDepth (R32_UINT)
        //  - 1: DilatedDepth  (R16_UINT)
        //  - 2: sampler
        //  - 3: DilatedMotion (R16G16F)
        //  - 5: Input Color (sampler2D)
        //  - 6: Depth
        //  - 7: Velocity
        //  - 7: Adjusted Color output (R16G16B16A16F)
        //  - 8: Dilated reactive masks output (R16G16F)
        binder->BindImage(0U, { targets_.estPrevDepth.GetHandle() });
        binder->BindImage(1U, { targets_.dilatedDepth.GetHandle() });
        binder->BindSampler(2U, { samplerHandle_.GetHandle() });
        binder->BindImage(3U, { targets_.dilatedMotionVectors[curMotionVecIdx].GetHandle() });
        binder->BindImage(4U, { targets_.dilatedMotionVectors[prevMotionVecIdx].GetHandle() });

        binder->BindImage(5U, { nodeInputsData.input.handle });
        binder->BindImage(6U, { nodeInputsData.velocity.handle });
        binder->BindImage(7U, { targets_.adjustedColorBuffer.GetHandle() });
        binder->BindImage(8U, { targets_.dilatedReactiveMask.GetHandle() });

        // Bind camera UBO
        binder->BindBuffer(12U, targets_.cameraUbo, 0);

        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

        const auto targetSize = targets_.renderResolution;
        LocalPostProcessPushConstantStruct uPc;
        uPc.viewportSizeInvSize = Math::Vec4(
            float(targetSize.x), float(targetSize.y), 1.0f / float(targetSize.x), 1.0f / float(targetSize.y));
        cmdList.PushConstantData(pc, arrayviewU8(uPc));

        // Dispatch
        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderPostProcessUpscaleNode::ComputeCreateLocks(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList)
{
    // bind pso
    cmdList.BindPipeline(psos_.locksPass);
    const ShaderThreadGroup tgs = psos_.locksPassTGS;

    auto& binder = binders_.locksPass;
    if (!binder) {
        return;
    }
    if (!RenderHandleUtil::IsValid(nodeInputsData.input.handle)) {
        return;
    }
    {
        binder->ClearBindings();

        binder->BindImage(0U, { targets_.lockInputLuma.GetHandle() });
        binder->BindImage(1U, { targets_.newLocksMask.GetHandle() });
        binder->BindImage(3U, { targets_.estPrevDepth.GetHandle() });
        binder->BindSampler(2U, { samplerHandle_.GetHandle() });

        // Bind camera UBO
        binder->BindBuffer(12U, targets_.cameraUbo, 0);

        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

        const auto renderSize = targets_.renderResolution;
        const auto displaySize = targets_.displayResolution;

        LockPassPushConstant uPc;
        uPc.renderSizeInvSize = Math::Vec4(
            float(renderSize.x), float(renderSize.y), 1.0f / float(renderSize.x), 1.0f / float(renderSize.y));
        uPc.displaySizeInvSize = Math::Vec4(
            float(displaySize.x), float(displaySize.y), 1.0f / float(displaySize.x), 1.0f / float(displaySize.y));

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Dispatch((renderSize.x + tgs.x - 1) / tgs.x, (renderSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderPostProcessUpscaleNode::ComputeAccumulate(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList)
{
    // bind pso
    cmdList.BindPipeline(psos_.accumulatePass);
    const ShaderThreadGroup tgs = psos_.accumulatePassTGS;

    auto& binder = binders_.accumulatePass;
    if (!binder) {
        return;
    }

    uint32_t readHistoryIdx = targets_.historyBufferIdx ^ 1;
    uint32_t writeHistoryIdx = targets_.historyBufferIdx;
    {
        binder->ClearBindings();
        // Bindings
        // - 0: Adjusted Color + Depth Clip (rgba16f)
        // - 1: Dilated Reactive Mask (rg16f)
        // - 2: Sampler
        // - 3: New Locks Mask (r8ui)
        // - 4: Dilated Motion Vectors (rg16f)

        // Input History Textures (Previous Frame Data - Display Resolution)
        // - 5: History Color + Reactive Alpha (rgba16f)
        // - 6: History Lock Status (rg16f)
        // - 7: History Luma (rgba16f)

        // Output History Textures (Current Frame Data - Display Resolution)
        // - 8: History Color + Reactive Alpha (rgba16f)
        // - 9: History Lock Status (rg16f)
        // - 10: History Luma (rgba16f)

        // Final Output Texture
        // - 11: Output Color (rgba16f) - Final Result [WRITE]
        // - 13: Luminance pyramid last mip (avg exposure)
        binder->BindImage(0U, { targets_.adjustedColorBuffer.GetHandle() });
        binder->BindImage(1U, { targets_.dilatedReactiveMask.GetHandle() });
        binder->BindSampler(2U, { samplerHandle_.GetHandle() });

        binder->BindImage(3U, { targets_.newLocksMask.GetHandle() });
        binder->BindImage(4U, { targets_.dilatedMotionVectors[targets_.motionVectorIdx].GetHandle() });
        // (Sampled Image)

        // --- History Buffers (Display Resolution) ---
        binder->BindImage(5U, { targets_.historyColorAndReactive[readHistoryIdx].GetHandle() });
        binder->BindImage(6U, { targets_.historyLockStatus[readHistoryIdx].GetHandle() });
        binder->BindImage(7U, { targets_.historyLuma[readHistoryIdx].GetHandle() });

        // Write current frame's history (Storage Image)
        binder->BindImage(8U, { targets_.historyColorAndReactive[writeHistoryIdx].GetHandle() });
        binder->BindImage(9U, { targets_.historyLockStatus[writeHistoryIdx].GetHandle() });
        binder->BindImage(10U, { targets_.historyLuma[writeHistoryIdx].GetHandle() });

        binder->BindImage(11U, { targets_.finalColor.GetHandle() });

        // Bind camera UBO
        binder->BindBuffer(12U, targets_.cameraUbo, 0);

        // Bind luminance pyramid texture level 4
        binder->BindImage(13U, targets_.tex1[mipLevels_ - 1].GetHandle());

        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

        const auto renderSize = targets_.renderResolution;
        const auto displaySize = targets_.displayResolution;
        AccumulatePassPushConstant uPc;
        uPc.displaySizeInvSize = Math::Vec4(
            float(displaySize.x), float(displaySize.y), 1.0f / float(displaySize.x), 1.0f / float(displaySize.y));
        uPc.viewportSizeInvSize = Math::Vec4(
            float(renderSize.x), float(renderSize.y), 1.0f / float(renderSize.x), 1.0f / float(renderSize.y));

        uPc.frameIndex = 1;

        uPc.jitterSequenceLength = 16; // 16: length
        if (uPc.jitterSequenceLength > 0) {
            uPc.avgLanczosWeightPerFrame = 1.0f / float(uPc.jitterSequenceLength);
        } else {
            uPc.avgLanczosWeightPerFrame = 1.0f;
        }
        uPc.maxAccumulationLanczosWeight = 0.98f;

        cmdList.PushConstantData(pc, arrayviewU8(uPc));
        cmdList.Dispatch((displaySize.x + tgs.x - 1) / tgs.x, (displaySize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderPostProcessUpscaleNode::ComputeRcas(const PushConstant& pc, RENDER_NS::IRenderCommandList& cmdList)
{
    // bind pso
    cmdList.BindPipeline(psos_.rcasPass);
    const ShaderThreadGroup tgs = psos_.rcasPassTGS;

    auto& binder = binders_.rcasPass;
    if (!binder) {
        return;
    }
    {
        binder->ClearBindings();

        binder->BindImage(0U, { targets_.finalColor.GetHandle() });
        binder->BindImage(1U, { targets_.rcas_final.GetHandle() });
        binder->BindSampler(2U, { samplerHandle_.GetHandle() });

        cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

        const auto targetSize = targets_.displayResolution;

        LocalPostProcessPushConstantStruct uPc;
        uPc.viewportSizeInvSize = Math::Vec4(
            float(targetSize.x), float(targetSize.y), 1.0f / float(targetSize.x), 1.0f / float(targetSize.y));
        // .x = exposure, .y = pre exposure, .z = sharpness, .w = 0
        // Sharpness : 0 = max, 1 = -1 stop, 2 = -2 stops
        float exposure = 0.7f;
        float preExposure = 1.0f;
        float sharpness = 0.0f;
        uPc.factor = Math::Vec4(exposure, preExposure, sharpness, 0.0f);
        cmdList.PushConstantData(pc, arrayviewU8(uPc));

        cmdList.Dispatch((targetSize.x + tgs.x - 1) / tgs.x, (targetSize.y + tgs.y - 1) / tgs.y, 1);
    }
}

void RenderPostProcessUpscaleNode::CreateTargets(const BASE_NS::Math::UVec2 baseSize)
{
    if (baseSize.x != baseSize_.x || baseSize.y != baseSize_.y) {
        baseSize_ = baseSize;
        // We only store the luminance value in texture
        ImageUsageFlags usageFlags =
            CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_TRANSFER_SRC_BIT;

        targets_.renderResolution = baseSize;
        const uint32_t dSizeX = uint32_t(float(baseSize.x) * effectProperties_.ratio);
        const uint32_t dSizeY = uint32_t(float(baseSize.y) * effectProperties_.ratio);

        targets_.displayResolution = BASE_NS::Math::UVec2(dSizeX, dSizeY); // effectProperties_.ratio);

        // create target image
        const Math::UVec2 startTargetSize = baseSize;
        GpuImageDesc desc {
            ImageType::CORE_IMAGE_TYPE_2D,
            ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R16_SFLOAT,
            ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            usageFlags,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS,
            startTargetSize.x,
            startTargetSize.y,
            1u,
            1u,
            1u,
            SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
            {},
        };

        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
#if (RENDER_VALIDATION_ENABLED == 1)
        const string_view nodeName = renderNodeContextMgr_->GetName();
#endif
        for (size_t idx = 0; idx < targets_.tex1.size(); ++idx) {
            // every upscale target is half the size of the original/ previous upscale target
            desc.width /= 2U;
            desc.height /= 2U;
            desc.width = (desc.width >= 1U) ? desc.width : 1U;
            desc.height = (desc.height >= 1U) ? desc.height : 1U;
            targets_.tex1Size[idx] = Math::UVec2(desc.width, desc.height);
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_luminance_Mip" + to_string(idx);
            targets_.tex1[idx] = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.tex1[idx] = gpuResourceMgr.Create(desc);
#endif
        }

        // Dilate and reconstruct targets at render resolution
        desc.width = baseSize.x;
        desc.height = baseSize.y;
        desc.usageFlags = CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;

        // Create dilated depth
        {
            desc.format = Format::BASE_FORMAT_R16_UINT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_dilated_depth";
            targets_.dilatedDepth = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.dilatedDepth = gpuResourceMgr.Create(desc);
#endif
        }

        // Create dilated motion vectors

        desc.format = Format::BASE_FORMAT_R16G16_SFLOAT;
        for (int i = 0; i < 2U; ++i) {
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto name = nodeName + "_lsr_dilated_motion_vector_" + ((i == 0) ? "A" : "B");
            targets_.dilatedMotionVectors[i] = gpuResourceMgr.Create(name, desc);
#else
            targets_.dilatedMotionVectors[i] = gpuResourceMgr.Create(desc);
#endif
        }

        // Create lock input luma
        {
            desc.format = Format::BASE_FORMAT_R16_SFLOAT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_lock_input_luma";
            targets_.lockInputLuma = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.lockInputLuma = gpuResourceMgr.Create(desc);
#endif
        }

        // Create previous depth for the current frame
        {
            desc.format = Format::BASE_FORMAT_R32_UINT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_previous_depth";
            targets_.estPrevDepth = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.estPrevDepth = gpuResourceMgr.Create(desc);
#endif
        }
        // Debug depth map texture
        {
            desc.format = Format::BASE_FORMAT_R32_SFLOAT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_debug_depth";
            targets_.debugImage = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.debugImage = gpuResourceMgr.Create(desc);
#endif
        }
        {
            desc.format = Format::BASE_FORMAT_R16G16B16A16_SFLOAT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_adjusted_color";
            targets_.adjustedColorBuffer = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.adjustedColorBuffer = gpuResourceMgr.Create(desc);
#endif
        }
        // Dilated reactive mask texture
        {
            desc.format = Format::BASE_FORMAT_R16G16_SFLOAT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_reactive_mask";
            targets_.dilatedReactiveMask = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.dilatedReactiveMask = gpuResourceMgr.Create(desc);
#endif
        }
        // Create locks mask texture
        {
            desc.width = targets_.displayResolution.x;
            desc.height = targets_.displayResolution.y;
            desc.format = Format::BASE_FORMAT_R8_UNORM;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_new_locks_mask";
            targets_.newLocksMask = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.newLocksMask = gpuResourceMgr.Create(desc);
#endif
        }

        // ---------- Create Accumulate Pass History Buffers (DISPLAY resolution) ----------
        desc.width = targets_.displayResolution.x;
        desc.height = targets_.displayResolution.y;
        desc.usageFlags = CORE_IMAGE_USAGE_STORAGE_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;

        // Color History (RGBA16F)
        desc.format = Format::BASE_FORMAT_R16G16B16A16_SFLOAT;
        for (int i = 0; i < 2U; ++i) {
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto name = nodeName + "_lsr_history_color_" + ((i == 0) ? "A" : "B");
            targets_.historyColorAndReactive[i] = gpuResourceMgr.Create(name, desc);
#else
            targets_.historyColorAndReactive[i] = gpuResourceMgr.Create(desc);
#endif
        }

        // Lock Status History (RG16F)
        desc.format = Format::BASE_FORMAT_R16G16_SFLOAT;
        for (int i = 0; i < 2U; ++i) {
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto name = nodeName + "_lsr_history_lock_" + ((i == 0) ? "A" : "B");
            targets_.historyLockStatus[i] = gpuResourceMgr.Create(name, desc);
#else
            targets_.historyLockStatus[i] = gpuResourceMgr.Create(desc);
#endif
        }

        // Luma History (RGBA16F)
        desc.format = Format::BASE_FORMAT_R8G8B8A8_UNORM;
        for (int i = 0; i < 2U; ++i) {
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto name = nodeName + "_lsr_history_luma_" + ((i == 0) ? "A" : "B");
            targets_.historyLuma[i] = gpuResourceMgr.Create(name, desc);
#else
            targets_.historyLuma[i] = gpuResourceMgr.Create(desc);
#endif
        }

        {
            desc.format = Format::BASE_FORMAT_R16G16B16A16_SFLOAT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_final_color";
            targets_.finalColor = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.finalColor = gpuResourceMgr.Create(desc);
#endif
        }

        // Optional RCAS pass
        {
            desc.format = Format::BASE_FORMAT_R16G16B16A16_SFLOAT;
#if (RENDER_VALIDATION_ENABLED == 1)
            const auto baseTargetName = nodeName + "_lsr_rcas_out_color";
            targets_.rcas_final = gpuResourceMgr.Create(baseTargetName, desc);
#else
            targets_.rcas_final = gpuResourceMgr.Create(desc);
#endif
        }
    }
}

void RenderPostProcessUpscaleNode::CreatePsos()
{
    if (binders_.accumulatePass) {
        return;
    }

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    INodeContextPsoManager& psoMgr = renderNodeContextMgr_->GetPsoManager();
    INodeContextDescriptorSetManager& dSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    // Luminance Hierarchy pass
    {
        const RenderHandle shader =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_luminance_downscale.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.luminanceDownscale = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.luminanceDownscaleTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        const auto& bindings = pl.descriptorSetLayouts[0].bindings;
        RenderHandle dsetHandle = dSetMgr.CreateDescriptorSet(bindings);
        binders_.luminanceDownscale = dSetMgr.CreateDescriptorSetBinder(dsetHandle, bindings);
    }

    // Luminance Hierarchy pass
    {
        const RenderHandle shader =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_luminance_pyramid.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.luminancePyramid = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.luminancePyramidTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        constexpr uint32_t localSetIdx = 0U;
        const auto& binds = pl.descriptorSetLayouts[localSetIdx].bindings;

        for (uint32_t idx = 0; idx < TARGET_COUNT; ++idx) {
            binders_.luminancePyramid[idx] =
                dSetMgr.CreateDescriptorSetBinder(dSetMgr.CreateDescriptorSet(binds), binds);
        }
    }
    // Dilate and reconstruct pass
    {
        const RenderHandle shader =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_dilate_and_reconstruct.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.reconstructAndDilate = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.reconstructAndDilateTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        const auto& bindings = pl.descriptorSetLayouts[0].bindings;
        RenderHandle dsetHandle = dSetMgr.CreateDescriptorSet(bindings);
        binders_.reconstructAndDilate = dSetMgr.CreateDescriptorSetBinder(dsetHandle, bindings);
    }

    // Debug depth map pass
    {
        const RenderHandle shader = shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_debug_depth.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.debugPass = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.debugPassTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        const auto& bindings = pl.descriptorSetLayouts[0].bindings;
        RenderHandle dsetHandle = dSetMgr.CreateDescriptorSet(bindings);
        binders_.debugPass = dSetMgr.CreateDescriptorSetBinder(dsetHandle, bindings);
    }

    // Clipping pass
    {
        const RenderHandle shader = shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_depth_clip.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.depthClipPass = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.depthClipPassTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        const auto& bindings = pl.descriptorSetLayouts[0].bindings;
        RenderHandle dsetHandle = dSetMgr.CreateDescriptorSet(bindings);
        binders_.depthClipPass = dSetMgr.CreateDescriptorSetBinder(dsetHandle, bindings);
    }

    // Create new locks
    {
        const RenderHandle shader = shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_lock.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.locksPass = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.locksPassTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        const auto& bindings = pl.descriptorSetLayouts[0].bindings;
        RenderHandle dsetHandle = dSetMgr.CreateDescriptorSet(bindings);
        binders_.locksPass = dSetMgr.CreateDescriptorSetBinder(dsetHandle, bindings);
    }

    // Accumulate pass
    {
        const RenderHandle shader = shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_accumulate.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.accumulatePass = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.accumulatePassTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        const auto& bindings = pl.descriptorSetLayouts[0].bindings;
        RenderHandle dsetHandle = dSetMgr.CreateDescriptorSet(bindings);
        binders_.accumulatePass = dSetMgr.CreateDescriptorSetBinder(dsetHandle, bindings);
    }

    // Rcas pass
    {
        const RenderHandle shader = shaderMgr.GetShaderHandle("rendershaders://computeshader/lsr_rcas.shader");
        const PipelineLayout& pl = shaderMgr.GetReflectionPipelineLayout(shader);

        psos_.rcasPass = psoMgr.GetComputePsoHandle(shader, pl, {});
        psos_.rcasPassTGS = shaderMgr.GetReflectionThreadGroupSize(shader);

        const auto& bindings = pl.descriptorSetLayouts[0].bindings;
        RenderHandle dsetHandle = dSetMgr.CreateDescriptorSet(bindings);
        binders_.rcasPass = dSetMgr.CreateDescriptorSetBinder(dsetHandle, bindings);
    }
}

void RenderPostProcessUpscaleNode::EvaluateOutput()
{
    if (RenderHandleUtil::IsValid(nodeInputsData.input.handle)) {
        nodeOutputsData.output = nodeInputsData.input;
    }
}
RENDER_END_NAMESPACE()
