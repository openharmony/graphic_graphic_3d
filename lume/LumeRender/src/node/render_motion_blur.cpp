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

#include "render_motion_blur.h"

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
// needs to match motion blur shaders
constexpr uint32_t CORE_SPREAD_TYPE_NEIGHBORHOOD { 0U };
constexpr uint32_t CORE_SPREAD_TYPE_HORIZONTAL { 1U };
constexpr uint32_t CORE_SPREAD_TYPE_VERTICAL { 2U };
constexpr uint32_t VELOCITY_TILE_SIZE { 8U };

constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t MAX_PASS_COUNT { 3u };
} // namespace

void RenderMotionBlur::Init(IRenderNodeContextManager& renderNodeContextMgr, const MotionBlurInfo& motionBlurInfo)
{
    motionBlurInfo_ = motionBlurInfo;
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr.GetShaderManager();
    {
        renderData_ = {};
        renderData_.shader = shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_motion_blur.shader");
        renderData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayout(renderData_.shader);
        const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(renderData_.shader);
        renderData_.pso = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(renderData_.shader, graphicsState,
            renderData_.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }
    {
        renderTileMaxData_ = {};
        renderTileMaxData_.shader =
            shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_motion_blur_tile_max.shader");
        renderTileMaxData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayout(renderTileMaxData_.shader);
        const RenderHandle graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(renderTileMaxData_.shader);
        renderTileMaxData_.pso = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(renderTileMaxData_.shader,
            graphicsState, renderTileMaxData_.pipelineLayout, {}, {}, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }
    {
        renderTileNeighborData_ = {};
        renderTileNeighborData_.shader =
            shaderMgr.GetShaderHandle("rendershaders://shader/fullscreen_motion_blur_tile_neighborhood.shader");
        renderTileNeighborData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayout(renderTileNeighborData_.shader);
        const RenderHandle graphicsState =
            shaderMgr.GetGraphicsStateHandleByShaderHandle(renderTileNeighborData_.shader);
        const ShaderSpecializationConstantView specView =
            shaderMgr.GetReflectionSpecialization(renderTileNeighborData_.shader);
        {
            const uint32_t specFlags[] = { CORE_SPREAD_TYPE_NEIGHBORHOOD };
            const ShaderSpecializationConstantDataView specDataView { specView.constants, specFlags };
            renderTileNeighborData_.psoNeighborhood = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(
                renderTileNeighborData_.shader, graphicsState, renderTileNeighborData_.pipelineLayout, {}, specDataView,
                { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
        {
            const uint32_t specFlags[] = { CORE_SPREAD_TYPE_HORIZONTAL };
            const ShaderSpecializationConstantDataView specDataView { specView.constants, specFlags };
            renderTileNeighborData_.psoHorizontal = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(
                renderTileNeighborData_.shader, graphicsState, renderTileNeighborData_.pipelineLayout, {}, specDataView,
                { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
        {
            const uint32_t specFlags[] = { CORE_SPREAD_TYPE_VERTICAL };
            const ShaderSpecializationConstantDataView specDataView { specView.constants, specFlags };
            renderTileNeighborData_.psoVertical = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(
                renderTileNeighborData_.shader, graphicsState, renderTileNeighborData_.pipelineLayout, {}, specDataView,
                { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
        }
    }
    samplerHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetSamplerHandle(
        DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
    samplerNearestHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetSamplerHandle(
        DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_NEAREST_CLAMP);
    {
        constexpr uint32_t globalSet = 0u;
        constexpr uint32_t localSet = 1u;

        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
        {
            const auto& bindings = renderData_.pipelineLayout.descriptorSetLayouts[globalSet].bindings;
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            globalSet0_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
        {
            const auto& bindings = renderData_.pipelineLayout.descriptorSetLayouts[localSet].bindings;
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            localSet1_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
        {
            const auto& bindings = renderTileMaxData_.pipelineLayout.descriptorSetLayouts[localSet].bindings;
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            localTileMaxSet1_ = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
        {
            const auto& bindings = renderTileNeighborData_.pipelineLayout.descriptorSetLayouts[localSet].bindings;
            {
                const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
                localTileNeighborhoodSet1_[0U] = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
            }
            {
                const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
                localTileNeighborhoodSet1_[1U] = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
            }
        }
    }
}

void RenderMotionBlur::PreExecute(IRenderNodeContextManager& renderNodeContextMgr, const MotionBlurInfo& blurInfo,
    const PostProcessConfiguration& ppConfig)
{
    motionBlurInfo_ = blurInfo;

    if ((ppConfig.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::MEDIUM) ||
        (ppConfig.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::HIGH)) {
        const uint32_t compSizeX = (motionBlurInfo_.size.x + VELOCITY_TILE_SIZE - 1U) / VELOCITY_TILE_SIZE;
        const uint32_t compSizeY = (motionBlurInfo_.size.y + VELOCITY_TILE_SIZE - 1U) / VELOCITY_TILE_SIZE;
        if ((!tileVelocityImages_[0U]) || (tileImageSize_.x != compSizeX) || (tileImageSize_.y != compSizeY)) {
            IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
            GpuImageDesc desc = renderNodeContextMgr.GetGpuResourceManager().GetImageDescriptor(blurInfo.velocity);
            desc.engineCreationFlags =
                CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
            desc.usageFlags = CORE_IMAGE_USAGE_SAMPLED_BIT | CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            desc.mipCount = 1U;
            desc.layerCount = 1U;
            desc.width = compSizeX;
            desc.height = compSizeY;
            tileVelocityImages_[0U] = gpuResourceMgr.Create(tileVelocityImages_[0U], desc);
            tileVelocityImages_[1U] = gpuResourceMgr.Create(tileVelocityImages_[1U], desc);

            tileImageSize_ = { compSizeX, compSizeY };
        }
    }
}

void RenderMotionBlur::Execute(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
    const MotionBlurInfo& blurInfo, const PostProcessConfiguration& ppConfig)
{
    if (!RenderHandleUtil::IsGpuImage(blurInfo.output)) {
        return;
    }

    // Update global descriptor set 0 for both passes
    UpdateDescriptorSet0(renderNodeContextMgr, cmdList, blurInfo, ppConfig);

    const RenderHandle velocity = blurInfo.velocity;
    RenderHandle tileVelocity = blurInfo.velocity;
    if ((ppConfig.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::MEDIUM) ||
        (ppConfig.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::HIGH)) {
        ExecuteTileVelocity(renderNodeContextMgr, cmdList, blurInfo, ppConfig);
        const RenderHandle tv = GetTileVelocityForMotionBlur();
        tileVelocity = RenderHandleUtil::IsValid(tv) ? tv : velocity;
    }

    const auto& renderData = renderData_;

    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.renderArea = { 0, 0, motionBlurInfo_.size.x, motionBlurInfo_.size.y };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachmentHandles[0] = blurInfo.output;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(renderData.pso);

    RenderHandle sets[2u] {};
    {
        auto& binder = *globalSet0_;
        sets[0u] = binder.GetDescriptorSetHandle();
    }
    {
        auto& binder = *localSet1_;
        binder.ClearBindings();
        uint32_t binding = 0u;
        binder.BindImage(binding++, blurInfo.input, samplerHandle_);
        binder.BindImage(binding++, blurInfo.depth, samplerNearestHandle_);
        binder.BindImage(binding++, velocity, samplerHandle_);
        binder.BindImage(binding++, tileVelocity, samplerHandle_);
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        sets[1u] = binder.GetDescriptorSetHandle();
    }
    cmdList.BindDescriptorSets(0u, sets);

    if (renderData.pipelineLayout.pushConstant.byteSize > 0) {
        const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
        cmdList.PushConstantData(renderData_.pipelineLayout.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr.GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr.GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderMotionBlur::ExecuteTileVelocity(IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList,
    const MotionBlurInfo& blurInfo, const PostProcessConfiguration& ppConfig)
{
    if ((!RenderHandleUtil::IsGpuImage(blurInfo.output)) || (!tileVelocityImages_[0U])) {
        return;
    }

    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.renderArea = { 0, 0, tileImageSize_.x, tileImageSize_.y };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;

    const RenderHandle vel0 = tileVelocityImages_[0U].GetHandle();
    const RenderHandle vel1 = tileVelocityImages_[1U].GetHandle();

    const ViewportDesc viewport = renderNodeContextMgr.GetRenderNodeUtil().CreateDefaultViewport(renderPass);
    const ScissorDesc scissor = renderNodeContextMgr.GetRenderNodeUtil().CreateDefaultScissor(renderPass);

    const float fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
    const float fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
    const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };

    RenderHandle sets[2u] {};
    {
        auto& binder = *globalSet0_;
        sets[0u] = binder.GetDescriptorSetHandle();
    }

    // tile max pass
    {
        const auto& renderData = renderTileMaxData_;
        renderPass.renderPassDesc.attachmentHandles[0] = vel0;

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(renderData.pso);

        {
            auto& binder = *localTileMaxSet1_;
            binder.ClearBindings();
            binder.BindImage(0U, blurInfo.velocity, samplerHandle_);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            sets[1u] = binder.GetDescriptorSetHandle();
        }
        cmdList.BindDescriptorSets(0u, sets);

        if (renderData.pipelineLayout.pushConstant.byteSize > 0) {
            cmdList.PushConstant(renderData.pipelineLayout.pushConstant, arrayviewU8(pc).data());
        }

        // dynamic state
        cmdList.SetDynamicStateViewport(viewport);
        cmdList.SetDynamicStateScissor(scissor);

        cmdList.Draw(3U, 1U, 0U, 0U);
        cmdList.EndRenderPass();
    }
    // tile neighborhood pass
    {
        const auto& renderData = renderTileNeighborData_;
        {
            const RenderHandle pso =
                renderTileNeighborData_.doublePass ? renderData.psoHorizontal : renderData.psoNeighborhood;

            renderPass.renderPassDesc.attachmentHandles[0] = vel1;

            cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
            cmdList.BindPipeline(pso);

            {
                auto& binder = *localTileNeighborhoodSet1_[0U];
                binder.ClearBindings();
                binder.BindImage(0U, vel0, samplerHandle_);
                cmdList.UpdateDescriptorSet(
                    binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
                sets[1u] = binder.GetDescriptorSetHandle();
            }
            cmdList.BindDescriptorSets(0u, sets);

            if (renderData.pipelineLayout.pushConstant.byteSize > 0) {
                cmdList.PushConstant(renderData.pipelineLayout.pushConstant, arrayviewU8(pc).data());
            }

            // dynamic state
            cmdList.SetDynamicStateViewport(viewport);
            cmdList.SetDynamicStateScissor(scissor);

            cmdList.Draw(3U, 1U, 0U, 0U);
            cmdList.EndRenderPass();
        }
        if (renderTileNeighborData_.doublePass) {
            const RenderHandle pso = renderData.psoVertical;

            renderPass.renderPassDesc.attachmentHandles[0] = vel0;

            cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
            cmdList.BindPipeline(pso);

            {
                auto& binder = *localTileNeighborhoodSet1_[1U];
                binder.ClearBindings();
                binder.BindImage(0U, vel1, samplerHandle_);
                cmdList.UpdateDescriptorSet(
                    binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
                sets[1u] = binder.GetDescriptorSetHandle();
            }
            cmdList.BindDescriptorSets(0u, sets);

            if (renderData.pipelineLayout.pushConstant.byteSize > 0) {
                cmdList.PushConstant(renderData.pipelineLayout.pushConstant, arrayviewU8(pc).data());
            }

            // dynamic state
            cmdList.SetDynamicStateViewport(viewport);
            cmdList.SetDynamicStateScissor(scissor);

            cmdList.Draw(3U, 1U, 0U, 0U);
            cmdList.EndRenderPass();
        }
    }
}

void RenderMotionBlur::UpdateDescriptorSet0(IRenderNodeContextManager& renderNodeContextMgr,
    IRenderCommandList& cmdList, const MotionBlurInfo& blurInfo, const PostProcessConfiguration& ppConfig)
{
    const RenderHandle ubo = blurInfo.globalUbo;
    auto& binder = *globalSet0_;
    binder.ClearBindings();
    uint32_t binding = 0u;
    binder.BindBuffer(binding++, ubo, 0u);
    binder.BindBuffer(binding++, ubo, blurInfo.globalUboByteOffset, sizeof(GlobalPostProcessStruct));
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

RenderHandle RenderMotionBlur::GetTileVelocityForMotionBlur() const
{
    return renderTileNeighborData_.doublePass ? tileVelocityImages_[0U].GetHandle()
                                              : tileVelocityImages_[1U].GetHandle();
}

DescriptorCounts RenderMotionBlur::GetDescriptorCounts() const
{
    // expected high max mip count
    return DescriptorCounts { {
        { CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_PASS_COUNT },
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u * MAX_PASS_COUNT },
    } };
}

RENDER_END_NAMESPACE()
