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

#include "render_post_process_ssao_node.h"

#include <base/containers/unique_ptr.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/property/property_types.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RenderPostProcessSsaoNode::EffectProperties, MEMBER_PROPERTY(enabled, "enabled", 0),
    MEMBER_PROPERTY(radius, "radius", 0), MEMBER_PROPERTY(intensity, "intensity", 0), MEMBER_PROPERTY(bias, "bias", 0),
    MEMBER_PROPERTY(contrast, "contrast", 0))
DATA_TYPE_METADATA(RenderPostProcessSsaoNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0),
    MEMBER_PROPERTY(depth, "depth", 0), MEMBER_PROPERTY(velocity, "velocity", 0), MEMBER_PROPERTY(camera, "camera", 0))
DATA_TYPE_METADATA(RenderPostProcessSsaoNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))

CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr string_view DOWNSCALE_SHADER_NAME { "rendershaders://shader/ssao/ssao_downscale.shader" };
constexpr string_view SSAO_SHADER_NAME { "rendershaders://shader/ssao/ssao.shader" };
constexpr string_view BLUR_SHADER_NAME { "rendershaders://shader/ssao/ssao_bilateral_blend.shader" };
constexpr string_view AREA_NOISE_SHADER_NAME { "rendershaders://shader/ssao/ssao_blur.shader" };

constexpr float SSAO_RENDER_RATIO = 0.5f;

constexpr int32_t SHADER_PARAM_COUNT { 2 };
constexpr int32_t BUFFER_SIZE_IN_BYTES = SHADER_PARAM_COUNT * 4 * sizeof(float);

constexpr GpuBufferDesc UBO_DESC { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0U };

RenderHandleReference CreateGpuBuffers(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    GpuBufferDesc desc = UBO_DESC;
    desc.byteSize = BUFFER_SIZE_IN_BYTES;
    return gpuResourceMgr.Create(handle, desc);
}

void UpdateBuffer(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle& handle,
    const RenderPostProcessSsaoNode::ShaderParameters& shaderParameters)
{
    if (void* data = gpuResourceMgr.MapBuffer(handle); data) {
        CloneData(data, sizeof(RenderPostProcessSsaoNode::ShaderParameters), &shaderParameters,
            sizeof(RenderPostProcessSsaoNode::ShaderParameters));
        gpuResourceMgr.UnmapBuffer(handle);
    }
}
} // namespace

RenderPostProcessSsaoNode::RenderPostProcessSsaoNode()
    : properties_(&propertiesData_, PropertyType::DataType<EffectProperties>::MetaDataFromType()),
      inputProperties_(&nodeInputsData_, PropertyType::DataType<NodeInputs>::MetaDataFromType()),
      outputProperties_(&nodeOutputsData_, PropertyType::DataType<NodeOutputs>::MetaDataFromType())
{}

RenderPostProcessSsaoNode::~RenderPostProcessSsaoNode() = default;

IPropertyHandle* RenderPostProcessSsaoNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessSsaoNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

DescriptorCounts RenderPostProcessSsaoNode::GetRenderDescriptorCounts() const
{
    return descriptorCounts_;
}

void RenderPostProcessSsaoNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

void RenderPostProcessSsaoNode::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    // clear
    psos_ = {};
    shaderData_ = {};
    binders_ = {};

    renderNodeContextMgr_ = &renderNodeContextMgr;

    // default inputs
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    defaultInput_.handle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    defaultInput_.samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");

    defaultLinearSampler_.samplerHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");

    gpuBuffer_ = CreateGpuBuffers(gpuResourceMgr, gpuBuffer_);

    // load shaders
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    shaderData_.downscalePass =
        shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(DOWNSCALE_SHADER_NAME));
    shaderData_.ssaoPass = shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(SSAO_SHADER_NAME));
    shaderData_.blurPass = shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(BLUR_SHADER_NAME));
    shaderData_.areaNoisePass =
        shaderMgr.GetGraphicsShaderDataByShaderHandle(shaderMgr.GetShaderHandle(AREA_NOISE_SHADER_NAME));

    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();

    const DescriptorCounts downscaleCounts =
        renderNodeUtil.GetDescriptorCounts(shaderData_.downscalePass.pipelineLayoutData);
    const DescriptorCounts ssaoCounts = renderNodeUtil.GetDescriptorCounts(shaderData_.ssaoPass.pipelineLayoutData);
    const DescriptorCounts blurCounts = renderNodeUtil.GetDescriptorCounts(shaderData_.blurPass.pipelineLayoutData);
    const DescriptorCounts areaNoiseCounts =
        renderNodeUtil.GetDescriptorCounts(shaderData_.areaNoisePass.pipelineLayoutData);

    descriptorCounts_.counts.clear();
    descriptorCounts_.counts.reserve(downscaleCounts.counts.size() + ssaoCounts.counts.size() +
                                     blurCounts.counts.size() + areaNoiseCounts.counts.size());
    descriptorCounts_.counts.append(downscaleCounts.counts.cbegin(), downscaleCounts.counts.cend());
    descriptorCounts_.counts.append(ssaoCounts.counts.cbegin(), ssaoCounts.counts.cend());
    descriptorCounts_.counts.append(blurCounts.counts.cbegin(), blurCounts.counts.cend());
    descriptorCounts_.counts.append(areaNoiseCounts.counts.cbegin(), areaNoiseCounts.counts.cend());

    valid_ = true;
}

void RenderPostProcessSsaoNode::PreExecuteFrame()
{
    if (!valid_) {
        return;
    }
    enabled_ = propertiesData_.enabled;

    if (enabled_) {
        // check input and output
        EvaluateOutputImageCreation();

        cameraUbo_ = nodeInputsData_.camera.handle;
        if (!RenderHandleUtil::IsValid(cameraUbo_)) {
            CORE_LOG_E("RenderPostProcessSsaoNode: Could not find camera UBO, SSAO will be disabled!");
            valid_ = false;
            return;
        }
        if (RenderHandleUtil::IsValid(nodeInputsData_.input.handle)) {
            const GpuImageDesc& imgDesc =
                renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(nodeInputsData_.input.handle);
            CreateTargets(Math::UVec2(imgDesc.width, imgDesc.height));
        }

        if (RenderHandleUtil::IsValid(nodeInputsData_.velocity.handle)) {
            velocityNormalIsValid_ = true;
            currVelocityNormalInput_ = nodeInputsData_.velocity;
        } else {
            velocityNormalIsValid_ = false;
            currVelocityNormalInput_.handle = defaultInput_.handle;

            CORE_LOG_ONCE_W("RenderPostProcessSsaoNodeVelocityMissing",
                "RenderPostProcessSsaoNode: No velocityNormal texture found, using screen-space approximation!");
        }
    }
}

IRenderNode::ExecuteFlags RenderPostProcessSsaoNode::GetExecuteFlags() const
{
    return (enabled_ && valid_) ? IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DEFAULT
                                : IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
}

void RenderPostProcessSsaoNode::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }

    /**
     *  1. Downscale depth and color to half resolution.
     *  2. Compute SSAO on the half resolution inputs.
     *  3. Blur the SSAO texture.
     *  4. Use depth-guided bilateral upscampling to blend with the original input.
     */

    CreatePsos();
    CheckDescriptorSetNeed();

    if (!RenderHandleUtil::IsValid(currOutput_.handle)) {
        return;
    }

    currInput_ = RenderHandleUtil::IsValid(nodeInputsData_.input.handle) ? nodeInputsData_.input : defaultInput_;
    if (!RenderHandleUtil::IsValid(currInput_.samplerHandle)) {
        currInput_.samplerHandle = defaultInput_.samplerHandle;
    }

    currDepthInput_ = nodeInputsData_.depth;
    if (!RenderHandleUtil::IsValid(currDepthInput_.samplerHandle)) {
        currDepthInput_.samplerHandle = defaultInput_.samplerHandle;
    }

    if (!RenderHandleUtil::IsValid(currVelocityNormalInput_.samplerHandle)) {
        currVelocityNormalInput_.samplerHandle = defaultInput_.samplerHandle;
    }

    currOutput_.handle = RenderHandleUtil::IsValid(nodeOutputsData_.output.handle)
                             ? nodeOutputsData_.output.handle
                             : ownOutputImageData_.handle.GetHandle();

    // update shader parameters
    shaderParameters_.resX = static_cast<float>(targets_.ssaoResolution.x);
    shaderParameters_.resY = static_cast<float>(targets_.ssaoResolution.y);
    shaderParameters_.time =
        renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderTimings.z * 1.0f;
    shaderParameters_.hasValidNormalTex = velocityNormalIsValid_ ? 1.0f : 0.0f;

    shaderParameters_.radius = propertiesData_.radius;
    shaderParameters_.bias = propertiesData_.bias;
    shaderParameters_.intensity = propertiesData_.intensity;
    shaderParameters_.contrast = propertiesData_.contrast;

    UpdateBuffer(renderNodeContextMgr_->GetGpuResourceManager(), gpuBuffer_.GetHandle(), shaderParameters_);

    RenderDownscalePass(cmdList);
    RenderSSAOPass(cmdList);
    RenderAreaNoisePass(cmdList);
    RenderBlurPass(cmdList);
}

void RenderPostProcessSsaoNode::RenderDownscalePass(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(currInput_.handle) || !RenderHandleUtil::IsValid(currDepthInput_.handle)) {
        CORE_LOG_E("RenderDownscalePass inputs invalid!");
        return;
    }

    auto& binder = binders_.downscalePass;
    if (!binder) {
        return;
    }

    RenderPass rp;
    rp.renderPassDesc.attachmentCount = 2u;
    rp.renderPassDesc.attachmentHandles[0u] = targets_.halfResDepth.GetHandle();
    rp.renderPassDesc.attachmentHandles[1u] = targets_.halfResColor.GetHandle();
    rp.renderPassDesc.attachments[0u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp.renderPassDesc.attachments[0u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    rp.renderPassDesc.attachments[1u].loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    rp.renderPassDesc.attachments[1u].storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
    rp.renderPassDesc.renderArea = { 0, 0, targets_.ssaoResolution.x, targets_.ssaoResolution.y };

    rp.renderPassDesc.subpassCount = 1u;
    rp.subpassDesc.colorAttachmentCount = 2u;
    rp.subpassDesc.colorAttachmentIndices[0u] = 0u;
    rp.subpassDesc.colorAttachmentIndices[1u] = 1u;
    rp.subpassStartIndex = 0u;

    cmdList.BeginRenderPass(rp.renderPassDesc, rp.subpassStartIndex, rp.subpassDesc);
    cmdList.BindPipeline(psos_.downscalePass);

    binder->ClearBindings();
    uint32_t binding = 0U;
    binder->BindImage(binding++, currInput_);
    binder->BindImage(binding++, currDepthInput_);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(rp));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(rp));

    cmdList.Draw(3U, 1U, 0U, 0U);
    cmdList.EndRenderPass();
}

void RenderPostProcessSsaoNode::RenderSSAOPass(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(targets_.halfResDepth.GetHandle()) ||
        !RenderHandleUtil::IsValid(targets_.halfResColor.GetHandle())) {
        CORE_LOG_E("RenderSSAOPass inputs invalid!");
        return;
    }

    auto& binder = binders_.ssaoPass;
    if (!binder) {
        return;
    }

    const auto renderPass = CreateRenderPass(targets_.ssaoTexture.GetHandle(), targets_.ssaoResolution);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.ssaoPass);

    binder->ClearBindings();
    uint32_t binding = 0u;
    binder->BindBuffer(binding++, gpuBuffer_.GetHandle(), 0u);

    BindableImage halfResColor;
    halfResColor.handle = targets_.halfResColor.GetHandle();
    halfResColor.samplerHandle = defaultInput_.samplerHandle;

    BindableImage halfResDepth;
    halfResDepth.handle = targets_.halfResDepth.GetHandle();
    halfResDepth.samplerHandle = defaultInput_.samplerHandle;

    binder->BindImage(binding++, halfResColor);
    binder->BindImage(binding++, halfResDepth);
    binder->BindBuffer(binding++, cameraUbo_, 0);
    binder->BindImage(binding++, currVelocityNormalInput_);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessSsaoNode::RenderBlurPass(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(targets_.ssaoTexture.GetHandle()) ||
        !RenderHandleUtil::IsValid(currDepthInput_.handle)) {
        CORE_LOG_E("RenderBlurPass inputs invalid!");
        return;
    }

    auto& binder = binders_.blurPass;
    if (!binder) {
        return;
    }

    const auto renderPass = CreateRenderPass(currOutput_.handle, targets_.outputResolution);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.blurPass);

    binder->ClearBindings();
    uint32_t binding = 0U;

    BindableImage ssaoInput;
    ssaoInput.handle = targets_.ssaoAreaNoiseTexture.GetHandle();
    ssaoInput.samplerHandle = defaultLinearSampler_.samplerHandle;

    binder->BindImage(binding++, ssaoInput);
    binder->BindImage(binding++, currInput_);
    binder->BindImage(binding++, currDepthInput_);
    binder->BindBuffer(binding++, cameraUbo_, 0);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3U, 1U, 0U, 0U);
    cmdList.EndRenderPass();
}

void RenderPostProcessSsaoNode::RenderAreaNoisePass(RENDER_NS::IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(targets_.ssaoTexture.GetHandle()) ||
        !RenderHandleUtil::IsValid(targets_.ssaoAreaNoiseTexture.GetHandle())) {
        CORE_LOG_E("RenderAreaNoisePass inputs invalid!");
        return;
    }

    auto& binder = binders_.areaNoisePass;
    if (!binder) {
        return;
    }

    const auto renderPass = CreateRenderPass(targets_.ssaoAreaNoiseTexture.GetHandle(), targets_.ssaoResolution);

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(psos_.areaNoisePass);

    binder->ClearBindings();
    uint32_t binding = 0U;

    BindableImage ssaoInput;
    ssaoInput.handle = targets_.ssaoTexture.GetHandle();
    ssaoInput.samplerHandle = defaultLinearSampler_.samplerHandle;

    binder->BindImage(binding++, ssaoInput);

    cmdList.UpdateDescriptorSet(binder->GetDescriptorSetHandle(), binder->GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(0U, binder->GetDescriptorSetHandle());

    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3U, 1U, 0U, 0U);
    cmdList.EndRenderPass();
}

RenderPass RenderPostProcessSsaoNode::CreateRenderPass(
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

void RenderPostProcessSsaoNode::CreateTargets(const BASE_NS::Math::UVec2 baseSize)
{
    if (baseSize.x != baseSize_.x || baseSize.y != baseSize_.y) {
        baseSize_ = baseSize;

        targets_.inputResolution = baseSize;
        targets_.outputResolution = baseSize;
        targets_.ssaoResolution = Math::UVec2(
            uint32_t(float(baseSize.x) * SSAO_RENDER_RATIO), uint32_t(float(baseSize.y) * SSAO_RENDER_RATIO));

        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

        const GpuImageDesc halfDepthDesc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R32_SFLOAT, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, targets_.ssaoResolution.x,
            targets_.ssaoResolution.y, 1u, 1u, 1u, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

        const GpuImageDesc halfColorDesc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R8G8B8A8_SRGB, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, targets_.ssaoResolution.x,
            targets_.ssaoResolution.y, 1u, 1u, 1u, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

        const GpuImageDesc ssaoDesc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
            Format::BASE_FORMAT_R8_UNORM, ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
            CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
            MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0,
            EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, targets_.ssaoResolution.x,
            targets_.ssaoResolution.y, 1u, 1u, 1u, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };

        targets_.halfResDepth = gpuResourceMgr.Create(halfDepthDesc);
        targets_.halfResColor = gpuResourceMgr.Create(halfColorDesc);
        targets_.ssaoTexture = gpuResourceMgr.Create(ssaoDesc);
        targets_.ssaoAreaNoiseTexture = gpuResourceMgr.Create(ssaoDesc);
    }
}

void RenderPostProcessSsaoNode::CreatePsos()
{
    if (RenderHandleUtil::IsValid(psos_.downscalePass) && RenderHandleUtil::IsValid(psos_.ssaoPass) &&
        RenderHandleUtil::IsValid(psos_.blurPass)) {
        return;
    }

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    INodeContextPsoManager& psoMgr = renderNodeContextMgr_->GetPsoManager();

    if (!RenderHandleUtil::IsValid(psos_.downscalePass)) {
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.downscalePass.shader);
        psos_.downscalePass = psoMgr.GetGraphicsPsoHandle(shaderData_.downscalePass.shader, gfxHandle,
            shaderData_.downscalePass.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    if (!RenderHandleUtil::IsValid(psos_.ssaoPass)) {
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.ssaoPass.shader);
        psos_.ssaoPass = psoMgr.GetGraphicsPsoHandle(shaderData_.ssaoPass.shader, gfxHandle,
            shaderData_.ssaoPass.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    if (!RenderHandleUtil::IsValid(psos_.areaNoisePass)) {
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.areaNoisePass.shader);
        psos_.areaNoisePass = psoMgr.GetGraphicsPsoHandle(shaderData_.areaNoisePass.shader, gfxHandle,
            shaderData_.areaNoisePass.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    if (!RenderHandleUtil::IsValid(psos_.blurPass)) {
        const RenderHandle gfxHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.blurPass.shader);
        psos_.blurPass = psoMgr.GetGraphicsPsoHandle(shaderData_.blurPass.shader, gfxHandle,
            shaderData_.blurPass.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }
}

void RenderPostProcessSsaoNode::CheckDescriptorSetNeed()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    if (!binders_.downscalePass) {
        binders_.downscalePass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.downscalePass.pipelineLayoutData),
            shaderData_.downscalePass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.ssaoPass) {
        binders_.ssaoPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.ssaoPass.pipelineLayoutData),
            shaderData_.ssaoPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.areaNoisePass) {
        binders_.areaNoisePass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.areaNoisePass.pipelineLayoutData),
            shaderData_.areaNoisePass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }

    if (!binders_.blurPass) {
        binders_.blurPass = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetMgr.CreateDescriptorSet(0, shaderData_.blurPass.pipelineLayoutData),
            shaderData_.blurPass.pipelineLayoutData.descriptorSetLayouts[0].bindings);
    }
}

void RenderPostProcessSsaoNode::EvaluateOutputImageCreation()
{
    if (RenderHandleUtil::IsValid(nodeOutputsData_.output.handle)) {
        currOutput_ = nodeOutputsData_.output;
    } else {
        IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        Math::UVec2 size { 1U, 1U };
        if (useRequestedRenderArea_) {
            const RenderPassDesc::RenderArea& area = renderAreaRequest_.area;
            size = { area.offsetX + area.extentWidth, area.offsetY + area.extentHeight };
        }
        if ((size.x != ownOutputImageData_.width) || (size.y != ownOutputImageData_.height)) {
            GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(currInput_.handle);
            if (desc.format == BASE_FORMAT_UNDEFINED) {
                desc.format = BASE_FORMAT_R8G8B8A8_SRGB;
            }
            desc.width = size.x;
            desc.height = size.y;
            desc.usageFlags |= ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                               ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
            desc.engineCreationFlags |=
                EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS |
                EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            ownOutputImageData_.handle = gpuResourceMgr.Create(desc);
            ownOutputImageData_.width = size.x;
            ownOutputImageData_.height = size.y;
        }
    }
}

RENDER_END_NAMESPACE()