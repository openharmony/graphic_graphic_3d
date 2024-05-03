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

#include "render_node_default_shadows_blur.h"

#include <algorithm>

#include <3d/render/intf_render_data_store_default_light.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_layout_desc.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/render_data_structures.h>
#include <render/resource_handle.h>

// shaders
#include <render/shaders/common/render_blur_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;
namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr uint32_t MAX_LOOP_COUNT { 2u };
} // namespace

void RenderNodeDefaultShadowsBlur::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    const auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    shadowColorBufferHandle_ = gpuResourceMgr.GetImageHandle(
        stores_.dataStoreNameScene + DefaultMaterialLightingConstants::SHADOW_VSM_COLOR_BUFFER_NAME);
    samplerHandle_ = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    bufferHandle_ = gpuResourceMgr.GetBufferHandle("CORE_DEFAULT_GPU_BUFFER");

    const auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    shaderData_ = {};
    {
        shaderData_.shaderHandle = shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_blur.shader");
        const auto& reflPipelineLayout = shaderMgr.GetReflectionPipelineLayout(shaderData_.shaderHandle);
        shaderData_.pushConstant = reflPipelineLayout.pushConstant;
    }

    CreateDescriptorSets();
}

void RenderNodeDefaultShadowsBlur::PreExecuteFrame()
{
    shadowCount_ = 0U;

    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));
    if (dataStoreLight) {
        const auto lightCounts = dataStoreLight->GetLightCounts();
        shadowTypes_ = dataStoreLight->GetShadowTypes();
        if (shadowTypes_.shadowType == IRenderDataStoreDefaultLight::ShadowType::VSM) {
            if ((lightCounts.dirShadow > 0) || (lightCounts.spotShadow > 0)) {
                // only run with VSM
                shadowCount_ = lightCounts.dirShadow + lightCounts.spotShadow;

                auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
                GpuImageDesc shadowImageDesc = gpuResourceMgr.GetImageDescriptor(shadowColorBufferHandle_);
                if ((shadowImageDesc.width != temporaryImage_.width) ||
                    (shadowImageDesc.height != temporaryImage_.height) ||
                    (shadowImageDesc.format != temporaryImage_.format) ||
                    (shadowImageDesc.sampleCountFlags != temporaryImage_.sampleCountFlags)) {
                    shadowImageDesc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
                    shadowImageDesc.layerCount = 1u;
#if (CORE3D_VALIDATION_ENABLED == 1)
                    const string name = renderNodeContextMgr_->GetName() + "_ShadowTemporaryImage";
                    temporaryImage_.imageHandle = gpuResourceMgr.Create(name, shadowImageDesc);
#else
                    temporaryImage_.imageHandle = gpuResourceMgr.Create(temporaryImage_.imageHandle, shadowImageDesc);
#endif
                    temporaryImage_.width = shadowImageDesc.width;
                    temporaryImage_.height = shadowImageDesc.height;
                    temporaryImage_.format = shadowImageDesc.format;
                    temporaryImage_.sampleCountFlags = shadowImageDesc.sampleCountFlags;
                }
            }
        }
    }
}

IRenderNode::ExecuteFlags RenderNodeDefaultShadowsBlur::GetExecuteFlags() const
{
    // this node does only blurring for VSM and can be left out easily
    if (shadowCount_ > 0U) {
        return 0U;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderNodeDefaultShadowsBlur::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));
    if (dataStoreLight) {
        if (shadowTypes_.shadowType != IRenderDataStoreDefaultLight::ShadowType::VSM) {
            return; // early out
        }
        // NOTE: try separating/overlapping different shadows
        // first horizontal for all then vertical for all (-> less syncs)
        const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight->GetLightCounts();
        if ((lightCounts.dirShadow > 0) || (lightCounts.spotShadow > 0)) {
            ExplicitInputBarrier(cmdList, shadowColorBufferHandle_);
        }
        if (lightCounts.dirShadow > 0) {
            ProcessSingleShadow(cmdList, 0u, shadowColorBufferHandle_, temporaryImage_);
        }
        if (lightCounts.spotShadow > 0) {
            ProcessSingleShadow(cmdList, 1u, shadowColorBufferHandle_, temporaryImage_);
        }
    }
}

void RenderNodeDefaultShadowsBlur::ProcessSingleShadow(IRenderCommandList& cmdList, const uint32_t drawIdx,
    const RenderHandle imageHandle, const TemporaryImage& tempImage)
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (gpuResourceMgr.IsGpuImage(imageHandle) && gpuResourceMgr.IsGpuImage(tempImage.imageHandle.GetHandle())) {
        RenderPass renderPass;
        renderPass.renderPassDesc.attachmentCount = 1u;
        renderPass.renderPassDesc.renderArea = { 0, 0, tempImage.width, tempImage.height };
        renderPass.renderPassDesc.subpassCount = 1u;
        renderPass.renderPassDesc.attachments[0].layer = drawIdx;
        renderPass.subpassStartIndex = 0;
        auto& subpass = renderPass.subpassDesc;
        subpass.colorAttachmentCount = 1u;
        subpass.colorAttachmentIndices[0] = 0;

        renderPass.renderPassDesc.attachments[0] = { 0, 0, AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE,
            AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE, AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE,
            AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE, ClearValue { ClearColorValue {} } };

        if (!RenderHandleUtil::IsValid(shaderData_.psoHandle)) {
            const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
            const RenderHandle graphicsStateHandle =
                shaderMgr.GetGraphicsStateHandleByShaderHandle(shaderData_.shaderHandle);
            const auto& reflPipelineLayout = shaderMgr.GetReflectionPipelineLayout(shaderData_.shaderHandle);
            const ShaderSpecializationConstantView sscv =
                shaderMgr.GetReflectionSpecialization(shaderData_.shaderHandle);
            const VertexInputDeclarationView vidv =
                shaderMgr.GetReflectionVertexInputDeclaration(shaderData_.shaderHandle);
            const uint32_t specializationFlags[] = { CORE_BLUR_TYPE_RG };
            CORE_ASSERT(sscv.constants.size() == countof(specializationFlags));
            const ShaderSpecializationConstantDataView specDataView { sscv.constants, specializationFlags };
            auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
            shaderData_.psoHandle = psoMgr.GetGraphicsPsoHandle(shaderData_.shaderHandle, graphicsStateHandle,
                reflPipelineLayout, vidv, specDataView, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }

        const float fWidth = static_cast<float>(tempImage.width);
        const float fHeight = static_cast<float>(tempImage.height);
        const Math::Vec4 texSizeInvTexSize = { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight };

        const ViewportDesc viewport { 0.0f, 0.0f, fWidth, fHeight, 0.0f, 1.0f };
        const ScissorDesc scissor { 0, 0, tempImage.width, tempImage.height };

        RenderData(cmdList, renderPass, viewport, scissor, imageHandle, tempImage.imageHandle.GetHandle(), drawIdx,
            texSizeInvTexSize);
    }
}

void RenderNodeDefaultShadowsBlur::RenderData(IRenderCommandList& cmdList, const RenderPass& renderPassBase,
    const ViewportDesc& viewport, const ScissorDesc& scissor, const RenderHandle inputHandle,
    const RenderHandle outputHandle, const uint32_t drawIdx, const Math::Vec4 texSizeInvTexSize)
{
    RenderPass renderPass = renderPassBase;
    uint32_t loopCount = 0;
    if (shadowTypes_.shadowSmoothness == IRenderDataStoreDefaultLight::ShadowSmoothness::NORMAL) {
        loopCount = 1U;
    } else if (shadowTypes_.shadowSmoothness == IRenderDataStoreDefaultLight::ShadowSmoothness::SOFT) {
        loopCount = 2U;
    }

    RenderHandle sets[2u] {};
    {
        auto& binder = *allDescriptorSets_.globalSet;
        sets[0u] = binder.GetDescriptorSetHandle();
        binder.ClearBindings();
        uint32_t binding = 0u;
        binder.BindBuffer(binding++, bufferHandle_, 0u);
        binder.BindBuffer(binding++, bufferHandle_, 0u);
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    }

    for (uint32_t idx = 0; idx < loopCount; ++idx) {
        const uint32_t descriptorSetIndex = (drawIdx * loopCount) + idx;
        // horizontal
        renderPass.renderPassDesc.attachmentHandles[0] = outputHandle;
        RenderBlur(cmdList, renderPass, viewport, scissor, allDescriptorSets_.set1Horizontal[descriptorSetIndex],
            texSizeInvTexSize, { 1.0f, 0.0f, 0.0f, 0.0f }, inputHandle);
        ExplicitOutputBarrier(cmdList, inputHandle);
        // vertical
        renderPass.renderPassDesc.attachmentHandles[0] = inputHandle;
        RenderBlur(cmdList, renderPass, viewport, scissor, allDescriptorSets_.set1Vertical[descriptorSetIndex],
            texSizeInvTexSize, { 0.0f, 1.0f, 0.0f, 0.0f }, outputHandle);
    }
}

void RenderNodeDefaultShadowsBlur::RenderBlur(IRenderCommandList& cmdList, const RenderPass& renderPass,
    const ViewportDesc& viewport, const ScissorDesc& scissor, const IDescriptorSetBinder::Ptr& binder,
    const Math::Vec4& texSizeInvTexSize, const Math::Vec4& dir, const RenderHandle imageHandle)
{
    RenderHandle sets[2u] {};
    sets[0u] = allDescriptorSets_.globalSet->GetDescriptorSetHandle();

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.SetDynamicStateViewport(viewport);
    cmdList.SetDynamicStateScissor(scissor);
    cmdList.BindPipeline(shaderData_.psoHandle);

    {
        auto& bind = *binder;
        sets[1u] = bind.GetDescriptorSetHandle();
        bind.ClearBindings();
        bind.BindSampler(0, samplerHandle_);
        bind.BindImage(1, imageHandle);

        cmdList.UpdateDescriptorSet(bind.GetDescriptorSetHandle(), bind.GetDescriptorSetLayoutBindingResources());
    }
    cmdList.BindDescriptorSets(0u, sets);

    const LocalPostProcessPushConstantStruct pc { texSizeInvTexSize, dir };
    cmdList.PushConstant(shaderData_.pushConstant, reinterpret_cast<const uint8_t*>(&pc));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderNodeDefaultShadowsBlur::ExplicitInputBarrier(IRenderCommandList& cmdList, const RenderHandle handle)
{
    const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        PipelineStageFlagBits::CORE_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        ImageLayout::CORE_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    const ImageSubresourceRange range { CORE_IMAGE_ASPECT_COLOR_BIT, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    cmdList.CustomImageBarrier(handle, dst, range);

    cmdList.AddCustomBarrierPoint();
}

void RenderNodeDefaultShadowsBlur::ExplicitOutputBarrier(IRenderCommandList& cmdList, const RenderHandle handle)
{
    const ImageResourceBarrier dst { AccessFlagBits::CORE_ACCESS_SHADER_READ_BIT,
        PipelineStageFlagBits::CORE_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        ImageLayout::CORE_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    const ImageSubresourceRange range { CORE_IMAGE_ASPECT_COLOR_BIT, 0,
        PipelineStateConstants::GPU_IMAGE_ALL_MIP_LEVELS, 0, PipelineStateConstants::GPU_IMAGE_ALL_LAYERS };
    cmdList.CustomImageBarrier(handle, dst, range);

    cmdList.AddCustomBarrierPoint();
}

void RenderNodeDefaultShadowsBlur::CreateDescriptorSets()
{
    auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    const uint32_t descriptorSetCount = DefaultMaterialLightingConstants::MAX_SHADOW_COUNT * MAX_LOOP_COUNT;
    {
        const DescriptorCounts dc { {
            { CORE_DESCRIPTOR_TYPE_SAMPLER, descriptorSetCount },
            { CORE_DESCRIPTOR_TYPE_SAMPLED_IMAGE, descriptorSetCount },
            { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u }, // global set with two ubos
        } };
        descriptorSetMgr.ResetAndReserve(dc);
    }

    const auto& reflPipelineLayout =
        renderNodeContextMgr_->GetShaderManager().GetReflectionPipelineLayout(shaderData_.shaderHandle);
    const uint32_t descSetCount = descriptorSetCount / 2u;
    allDescriptorSets_.set1Horizontal.resize(descSetCount);
    allDescriptorSets_.set1Vertical.resize(descSetCount);
    constexpr uint32_t globalSet = 0u;
    constexpr uint32_t localSet = 1u;
    {
        const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(globalSet, reflPipelineLayout);
        allDescriptorSets_.globalSet = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetHandle, reflPipelineLayout.descriptorSetLayouts[globalSet].bindings);
    }
    for (uint32_t idx = 0; idx < descSetCount; ++idx) {
        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(localSet, reflPipelineLayout);
            allDescriptorSets_.set1Horizontal[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, reflPipelineLayout.descriptorSetLayouts[localSet].bindings);
        }
        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(localSet, reflPipelineLayout);
            allDescriptorSets_.set1Vertical[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, reflPipelineLayout.descriptorSetLayouts[localSet].bindings);
        }
    }
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultShadowsBlur::Create()
{
    return new RenderNodeDefaultShadowsBlur();
}

void RenderNodeDefaultShadowsBlur::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultShadowsBlur*>(instance);
}
CORE3D_END_NAMESPACE()
