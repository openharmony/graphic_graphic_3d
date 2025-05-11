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

#include "render_post_process_motion_blur_node.h"

#include <base/containers/fixed_string.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>
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
#include "render_post_process_motion_blur.h"
#include "util/log.h"

// shaders
#include <render/shaders/common/render_post_process_structs_common.h>

using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

CORE_BEGIN_NAMESPACE()
DATA_TYPE_METADATA(RenderPostProcessMotionBlurNode::NodeInputs, MEMBER_PROPERTY(input, "input", 0),
    MEMBER_PROPERTY(velocity, "velocity", 0), MEMBER_PROPERTY(depth, "depth", 0))
DATA_TYPE_METADATA(RenderPostProcessMotionBlurNode::NodeOutputs, MEMBER_PROPERTY(output, "output", 0))
CORE_END_NAMESPACE()

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t CORE_SPREAD_TYPE_NEIGHBORHOOD { 0U };
constexpr uint32_t CORE_SPREAD_TYPE_HORIZONTAL { 1U };
constexpr uint32_t CORE_SPREAD_TYPE_VERTICAL { 2U };
constexpr uint32_t VELOCITY_TILE_SIZE { 8U };

constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

constexpr uint32_t MAX_PASS_COUNT { 3u };

constexpr int32_t BUFFER_SIZE_IN_BYTES = sizeof(BASE_NS::Math::Vec4);
constexpr GpuBufferDesc BUFFER_DESC { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, 0U };

RenderHandleReference CreateGpuBuffers(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandleReference& handle)
{
    // Create buffer for the shader data.
    GpuBufferDesc desc = BUFFER_DESC;
    desc.byteSize = BUFFER_SIZE_IN_BYTES;
    return gpuResourceMgr.Create(handle, desc);
}

void UpdateBuffer(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderHandle& handle,
    const BASE_NS::Math::Vec4& shaderParameters)
{
    if (void* data = gpuResourceMgr.MapBuffer(handle); data) {
        CloneData(data, sizeof(BASE_NS::Math::Vec4), &shaderParameters, sizeof(BASE_NS::Math::Vec4));
        gpuResourceMgr.UnmapBuffer(handle);
    }
}
} // namespace

RenderPostProcessMotionBlurNode::RenderPostProcessMotionBlurNode()
    : inputProperties_(
          &nodeInputsData, array_view(PropertyType::DataType<RenderPostProcessMotionBlurNode::NodeInputs>::properties)),
      outputProperties_(&nodeOutputsData,
          array_view(PropertyType::DataType<RenderPostProcessMotionBlurNode::NodeOutputs>::properties))

{}

IPropertyHandle* RenderPostProcessMotionBlurNode::GetRenderInputProperties()
{
    return inputProperties_.GetData();
}

IPropertyHandle* RenderPostProcessMotionBlurNode::GetRenderOutputProperties()
{
    return outputProperties_.GetData();
}

void RenderPostProcessMotionBlurNode::SetRenderAreaRequest(const RenderAreaRequest& renderAreaRequest)
{
    useRequestedRenderArea_ = true;
    renderAreaRequest_ = renderAreaRequest;
}

IRenderNode::ExecuteFlags RenderPostProcessMotionBlurNode::GetExecuteFlags() const
{
    if (effectProperties_.enabled) {
        return 0;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderPostProcessMotionBlurNode::Init(
    const IRenderPostProcess::Ptr& postProcess, IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    postProcess_ = postProcess;

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

    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    samplerHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetSamplerHandle(
        DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_LINEAR_CLAMP);
    samplerNearestHandle_ = renderNodeContextMgr.GetGpuResourceManager().GetSamplerHandle(
        DefaultEngineGpuResourceConstants::CORE_DEFAULT_SAMPLER_NEAREST_CLAMP);
    gpuBuffer_ = CreateGpuBuffers(gpuResourceMgr, gpuBuffer_);

    valid_ = true;
}

void RenderPostProcessMotionBlurNode::PreExecute()
{
    if (valid_ && postProcess_) {
        const array_view<const uint8_t> propertyView = postProcess_->GetData();
        // this node is directly dependant
        PLUGIN_ASSERT(propertyView.size_bytes() == sizeof(RenderPostProcessMotionBlurNode::EffectProperties));
        if (propertyView.size_bytes() == sizeof(RenderPostProcessMotionBlurNode::EffectProperties)) {
            effectProperties_ = (const RenderPostProcessMotionBlurNode::EffectProperties&)(*propertyView.data());
            if ((effectProperties_.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::MEDIUM) ||
                (effectProperties_.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::HIGH)) {
                const uint32_t compSizeX = (effectProperties_.size.x + VELOCITY_TILE_SIZE - 1U) / VELOCITY_TILE_SIZE;
                const uint32_t compSizeY = (effectProperties_.size.y + VELOCITY_TILE_SIZE - 1U) / VELOCITY_TILE_SIZE;
                if ((!tileVelocityImages_[0U]) || (tileImageSize_.x != compSizeX) || (tileImageSize_.y != compSizeY)) {
                    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
                    GpuImageDesc desc = renderNodeContextMgr_->GetGpuResourceManager().GetImageDescriptor(
                        nodeInputsData.velocity.handle);
                    desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS |
                                               CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
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
    } else {
        effectProperties_.enabled = false;
    }
}

void RenderPostProcessMotionBlurNode::Execute(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsGpuImage(nodeOutputsData.output.handle)) {
        return;
    }

    CheckDescriptorSetNeed();

    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "RenderMotionBlur", DefaultDebugConstants::CORE_DEFAULT_DEBUG_COLOR);

    const RenderHandle velocity = nodeInputsData.velocity.handle;
    RenderHandle tileVelocity = nodeInputsData.velocity.handle;
    if ((effectProperties_.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::MEDIUM) ||
        (effectProperties_.motionBlurConfiguration.quality == MotionBlurConfiguration::Quality::HIGH)) {
        ExecuteTileVelocity(cmdList);
        const RenderHandle tv = GetTileVelocityForMotionBlur();
        tileVelocity = RenderHandleUtil::IsValid(tv) ? tv : velocity;
    }

    const auto& renderData = renderData_;

    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.renderArea = { 0, 0, effectProperties_.size.x, effectProperties_.size.y };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachmentHandles[0] = nodeOutputsData.output.handle;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
    cmdList.BindPipeline(renderData.pso);

    shaderParameters_ = GetFactorMotionBlur();
    UpdateBuffer(renderNodeContextMgr_->GetGpuResourceManager(), gpuBuffer_.GetHandle(), shaderParameters_);

    {
        auto& binder = *binders_.localSet0;
        binder.ClearBindings();
        uint32_t binding = 0u;
        binder.BindImage(binding++, nodeInputsData.input.handle, samplerHandle_);
        binder.BindImage(binding++, nodeInputsData.depth.handle, samplerNearestHandle_);
        binder.BindImage(binding++, velocity, samplerHandle_);
        binder.BindImage(binding++, tileVelocity, samplerHandle_);
        binder.BindBuffer(binding++, gpuBuffer_.GetHandle(), 0u);

        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0u, binder.GetDescriptorSetHandle());
    }

    if (renderData.pipelineLayout.pushConstant.byteSize > 0) {
        const auto fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
        const auto fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
        const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };
        cmdList.PushConstantData(renderData_.pipelineLayout.pushConstant, arrayviewU8(pc));
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass));
    cmdList.SetDynamicStateScissor(renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass));

    cmdList.Draw(3u, 1u, 0u, 0u);
    cmdList.EndRenderPass();
}

void RenderPostProcessMotionBlurNode::ExecuteTileVelocity(IRenderCommandList& cmdList)
{
    if ((!RenderHandleUtil::IsGpuImage(nodeOutputsData.output.handle)) || (!tileVelocityImages_[0U])) {
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

    const ViewportDesc viewport = renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultViewport(renderPass);
    const ScissorDesc scissor = renderNodeContextMgr_->GetRenderNodeUtil().CreateDefaultScissor(renderPass);

    const auto fWidth = static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth);
    const auto fHeight = static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight);
    const LocalPostProcessPushConstantStruct pc { { fWidth, fHeight, 1.0f / fWidth, 1.0f / fHeight }, {} };

    // tile max pass
    {
        const auto& renderData = renderTileMaxData_;
        renderPass.renderPassDesc.attachmentHandles[0] = vel0;

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);
        cmdList.BindPipeline(renderData.pso);

        {
            auto& binder = *binders_.localTileMaxSet0;
            binder.ClearBindings();
            binder.BindImage(0U, nodeInputsData.velocity.handle, samplerHandle_);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0u, binder.GetDescriptorSetHandle());
        }

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
                auto& binder = *binders_.localTileNeighborhoodSet0[0U];
                binder.ClearBindings();
                binder.BindImage(0U, vel0, samplerHandle_);
                cmdList.UpdateDescriptorSet(
                    binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
                cmdList.BindDescriptorSet(0u, binder.GetDescriptorSetHandle());
            }

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
                auto& binder = *binders_.localTileNeighborhoodSet0[1U];
                binder.ClearBindings();
                binder.BindImage(0U, vel1, samplerHandle_);
                cmdList.UpdateDescriptorSet(
                    binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
                cmdList.BindDescriptorSet(0u, binder.GetDescriptorSetHandle());
            }

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

BASE_NS::Math::Vec4 RenderPostProcessMotionBlurNode::GetFactorMotionBlur() const
{
    return {
        static_cast<float>(effectProperties_.motionBlurConfiguration.sharpness),
        static_cast<float>(effectProperties_.motionBlurConfiguration.quality),
        effectProperties_.motionBlurConfiguration.alpha,
        effectProperties_.motionBlurConfiguration.velocityCoefficient,
    };
}

RenderHandle RenderPostProcessMotionBlurNode::GetTileVelocityForMotionBlur() const
{
    return renderTileNeighborData_.doublePass ? tileVelocityImages_[0U].GetHandle()
                                              : tileVelocityImages_[1U].GetHandle();
}

DescriptorCounts RenderPostProcessMotionBlurNode::GetRenderDescriptorCounts() const
{
    // expected high max mip count
    return DescriptorCounts { {
        { CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_PASS_COUNT },
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u * MAX_PASS_COUNT },
    } };
}

void RenderPostProcessMotionBlurNode::CheckDescriptorSetNeed()
{
    if (!binders_.localSet0) {
        constexpr uint32_t localSet = 0u;
        INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        {
            const auto& bindings = renderData_.pipelineLayout.descriptorSetLayouts[localSet].bindings;
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            binders_.localSet0 = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
        {
            const auto& bindings = renderTileMaxData_.pipelineLayout.descriptorSetLayouts[localSet].bindings;
            const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
            binders_.localTileMaxSet0 = descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
        }
        {
            const auto& bindings = renderTileNeighborData_.pipelineLayout.descriptorSetLayouts[localSet].bindings;
            {
                const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
                binders_.localTileNeighborhoodSet0[0U] =
                    descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
            }
            {
                const RenderHandle descHandle = descriptorSetMgr.CreateDescriptorSet(bindings);
                binders_.localTileNeighborhoodSet0[1U] =
                    descriptorSetMgr.CreateDescriptorSetBinder(descHandle, bindings);
            }
        }
    }
}

RENDER_END_NAMESPACE()
