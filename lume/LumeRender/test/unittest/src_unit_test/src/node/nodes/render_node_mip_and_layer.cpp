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

#include "node/nodes/render_node_mip_and_layer.h"

#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()

namespace {
static constexpr string_view GRAPHICS_IMAGE_NAME { "GraphicsImage" };
static constexpr string_view COMPUTE_IMAGE_NAME { "ComputeImage" };

struct PushDataStruct {
    Math::Vec4 texSizeInvSize { 1.0f, 1.0f, 1.0f, 1.0f };
    Math::Vec4 color { 0.0f, 0.0f, 0.0f, 0.0f };
};
} // namespace

void RenderNodeMipAndLayer::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    {
        const DescriptorCounts dc { {
            { CORE_DESCRIPTOR_TYPE_STORAGE_IMAGE, 2U },
            { CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 4U },
        } };
        descriptorSetMgr.ResetAndReserve(dc);
    }

    {
        auto& dat = graphicsData_;

        const RenderHandle shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://shader/RenderNodeMipAndLayerGraphicsTest.shader");
        const ShaderSpecializationConstantDataView specialization { {}, {} };
        constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT,
            CORE_DYNAMIC_STATE_ENUM_SCISSOR };
        const RenderHandle graphicsState =
            renderNodeContextMgr_->GetShaderManager().GetGraphicsStateHandleByShaderHandle(shaderHandle);
        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        dat.pipelineLayout = renderNodeUtil.CreatePipelineLayout(shaderHandle);
        const auto& inputLayout = shaderMgr.GetReflectionVertexInputDeclaration(shaderHandle);
        dat.psoHandle = renderNodeContextMgr.GetPsoManager().GetGraphicsPsoHandle(shaderHandle, graphicsState,
            dat.pipelineLayout, inputLayout, specialization, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });

        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(0U, dat.pipelineLayout);
            dat.descriptorSet0 = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, dat.pipelineLayout.descriptorSetLayouts[0U].bindings);
        }
        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(0U, dat.pipelineLayout);
            dat.descriptorSet1 = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, dat.pipelineLayout.descriptorSetLayouts[0U].bindings);
        }

        dat.image = gpuResourceMgr.GetImageHandle(GRAPHICS_IMAGE_NAME);
        const auto desc = gpuResourceMgr.GetImageDescriptor(dat.image);
        dat.imageSize = { desc.width, desc.height };
    }
    {
        auto& dat = computeData_;

        auto shaderHandle =
            shaderMgr.GetShaderHandle("rendershaders://computeshader/RenderNodeMipAndLayerComputeTest.shader");
        const ShaderSpecializationConstantDataView specialization { {}, {} };
        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        dat.pipelineLayout = renderNodeUtil.CreatePipelineLayout(shaderHandle);
        dat.psoHandle =
            renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(shaderHandle, dat.pipelineLayout, specialization);

        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(0U, dat.pipelineLayout);
            dat.descriptorSet0 = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, dat.pipelineLayout.descriptorSetLayouts[0U].bindings);
        }
        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(0U, dat.pipelineLayout);
            dat.descriptorSet1 = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, dat.pipelineLayout.descriptorSetLayouts[0U].bindings);
        }

        dat.image = gpuResourceMgr.GetImageHandle(COMPUTE_IMAGE_NAME);
        const auto desc = gpuResourceMgr.GetImageDescriptor(dat.image);
        dat.imageSize = { desc.width, desc.height };
    }

    defaultImage_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    defaultSampler_ = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
}

void RenderNodeMipAndLayer::PreExecuteFrame() {}

void RenderNodeMipAndLayer::ExecuteFrame(IRenderCommandList& cmdList)
{
    static bool yesNo = true;
    if (yesNo) {
        ExecuteGraphics(cmdList);
        ExecuteCompute(cmdList);
    } else {
        ExecuteCompute(cmdList);
        ExecuteGraphics(cmdList);
    }
    yesNo = !yesNo;
}

void RenderNodeMipAndLayer::ExecuteGraphics(IRenderCommandList& cmdList)
{
    auto& dat = graphicsData_;

    RenderPass renderPass;
    renderPass.renderPassDesc.attachmentCount = 1;
    renderPass.renderPassDesc.renderArea = { 0, 0, dat.imageSize.x, dat.imageSize.y };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.renderPassDesc.attachments[0].loadOp = CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
    renderPass.renderPassDesc.attachments[0].storeOp = CORE_ATTACHMENT_STORE_OP_STORE;
    renderPass.renderPassDesc.attachmentHandles[0] = dat.image;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.colorAttachmentCount = 1;
    subpass.colorAttachmentIndices[0] = 0;

    uint32_t mipLevel = 0;
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    // first pass
    {
        const uint32_t currWidth = Math::max(1u, dat.imageSize.x >> mipLevel);
        const uint32_t currHeight = Math::max(1u, dat.imageSize.y >> mipLevel);
        const float fCurrWidth = static_cast<float>(currWidth);
        const float fCurrHeight = static_cast<float>(currHeight);

        renderPass.renderPassDesc.renderArea = { 0, 0, currWidth, currHeight };
        renderPass.renderPassDesc.attachments[0].mipLevel = mipLevel;

        const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

        cmdList.BindPipeline(dat.psoHandle);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);
        {
            auto& binder = *dat.descriptorSet0;
            BindableImage bi;
            bi.handle = defaultImage_;
            bi.samplerHandle = defaultSampler_;
            binder.BindImage(0, bi);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0U, binder.GetDescriptorSetHandle());
        }
        if (dat.pipelineLayout.pushConstant.byteSize > 0) {
            PushDataStruct pds;
            pds.texSizeInvSize = { fCurrWidth, fCurrHeight, 1.0f / fCurrWidth, 1.0f / fCurrHeight };
            pds.color = { 1.0f, 0.0, 0.0, 0.501f };
            cmdList.PushConstantData(dat.pipelineLayout.pushConstant, arrayviewU8(pds));
        }
        cmdList.Draw(3u, 1u, 0u, 0u);

        cmdList.EndRenderPass();
    }
    mipLevel++;
    // second pass
    {
        const uint32_t currWidth = Math::max(1u, dat.imageSize.x >> mipLevel);
        const uint32_t currHeight = Math::max(1u, dat.imageSize.y >> mipLevel);
        const float fCurrWidth = static_cast<float>(currWidth);
        const float fCurrHeight = static_cast<float>(currHeight);

        renderPass.renderPassDesc.renderArea = { 0, 0, currWidth, currHeight };
        renderPass.renderPassDesc.attachments[0].mipLevel = mipLevel;

        const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
        const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);

        cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

        cmdList.BindPipeline(dat.psoHandle);
        cmdList.SetDynamicStateViewport(viewportDesc);
        cmdList.SetDynamicStateScissor(scissorDesc);
        {
            auto& binder = *dat.descriptorSet1;
            BindableImage bi;
            bi.handle = dat.image;
            bi.mip = 0U; // NOTE
            bi.samplerHandle = defaultSampler_;
            binder.BindImage(0, bi);
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0U, binder.GetDescriptorSetHandle());
        }
        if (dat.pipelineLayout.pushConstant.byteSize > 0) {
            PushDataStruct pds;
            pds.texSizeInvSize = { fCurrWidth, fCurrHeight, 1.0f / fCurrWidth, 1.0f / fCurrHeight };
            pds.color = { 0.0f, 1.0, 0.0, 0.501f };
            cmdList.PushConstantData(dat.pipelineLayout.pushConstant, arrayviewU8(pds));
        }
        cmdList.Draw(3u, 1u, 0u, 0u);

        cmdList.EndRenderPass();
    }
}

void RenderNodeMipAndLayer::ExecuteCompute(IRenderCommandList& cmdList)
{
    constexpr uint32_t threadGroupSize { 8U };

    uint32_t mipLevel = 0;
    auto& dat = computeData_;
    // first pass
    {
        const uint32_t currWidth = Math::max(1u, dat.imageSize.x >> mipLevel);
        const uint32_t currHeight = Math::max(1u, dat.imageSize.y >> mipLevel);
        const float fCurrWidth = static_cast<float>(currWidth);
        const float fCurrHeight = static_cast<float>(currHeight);

        cmdList.BindPipeline(dat.psoHandle);
        {
            auto& binder = *dat.descriptorSet0;
            {
                BindableImage bi;
                bi.handle = dat.image;
                bi.mip = 0U;
                binder.BindImage(0U, bi);
            }
            {
                BindableImage bi;
                bi.handle = defaultImage_;
                bi.samplerHandle = defaultSampler_;
                binder.BindImage(1U, bi);
            }
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0U, binder.GetDescriptorSetHandle());
        }
        if (dat.pipelineLayout.pushConstant.byteSize > 0) {
            PushDataStruct pds;
            pds.texSizeInvSize = { fCurrWidth, fCurrHeight, 1.0f / fCurrWidth, 1.0f / fCurrHeight };
            pds.color = { 1.0f, 0.0, 0.0, 0.501f };
            cmdList.PushConstantData(dat.pipelineLayout.pushConstant, arrayviewU8(pds));
        }
        cmdList.Dispatch((currWidth + threadGroupSize - 1u) / threadGroupSize,
            (currHeight + threadGroupSize - 1u) / threadGroupSize, 1U);
    }
    mipLevel++;
    // second pass
    {
        const uint32_t currWidth = Math::max(1u, dat.imageSize.x >> mipLevel);
        const uint32_t currHeight = Math::max(1u, dat.imageSize.y >> mipLevel);
        const float fCurrWidth = static_cast<float>(currWidth);
        const float fCurrHeight = static_cast<float>(currHeight);

        cmdList.BindPipeline(dat.psoHandle);
        {
            auto& binder = *dat.descriptorSet1;
            {
                BindableImage bi;
                bi.handle = dat.image;
                bi.mip = 1U;
                binder.BindImage(0U, bi);
            }
            {
                BindableImage bi;
                bi.handle = dat.image;
                bi.mip = 0U;
                bi.samplerHandle = defaultSampler_;
                binder.BindImage(1U, bi);
            }
            cmdList.UpdateDescriptorSet(
                binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
            cmdList.BindDescriptorSet(0U, binder.GetDescriptorSetHandle());
        }
        if (dat.pipelineLayout.pushConstant.byteSize > 0) {
            PushDataStruct pds;
            pds.texSizeInvSize = { fCurrWidth, fCurrHeight, 1.0f / fCurrWidth, 1.0f / fCurrHeight };
            pds.color = { 0.0f, 1.0, 0.0, 0.501f };
            cmdList.PushConstantData(dat.pipelineLayout.pushConstant, arrayviewU8(pds));
        }
        cmdList.Dispatch((currWidth + threadGroupSize - 1u) / threadGroupSize,
            (currHeight + threadGroupSize - 1u) / threadGroupSize, 1U);
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeMipAndLayer::Create()
{
    return new RenderNodeMipAndLayer();
}

void RenderNodeMipAndLayer::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeMipAndLayer*>(instance);
}
RENDER_END_NAMESPACE()