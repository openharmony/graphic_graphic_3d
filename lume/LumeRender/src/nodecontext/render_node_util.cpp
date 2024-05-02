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

#include "render_node_util.h"

#include <algorithm>

#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "datastore/render_data_store_post_process.h"
#include "device/gpu_resource_handle_util.h"
#include "nodecontext/pipeline_descriptor_set_binder.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
RenderHandle GetRoutedResource(const IRenderNodeGpuResourceManager& gpuResourceMgr,
    const IRenderNodeGraphShareManager& rngShareMgr, const RenderDataConstants::RenderDataFixedString& name,
    const RenderDataConstants::RenderDataFixedString& nodeName,
    const RenderNodeGraphResourceLocationType resourceLocation, const uint32_t resourceIndex,
    const RenderHandleType handleType)
{
    RenderHandle handle;
    switch (resourceLocation) {
        case (RenderNodeGraphResourceLocationType::DEFAULT):
            if (handleType == RenderHandleType::GPU_BUFFER) {
                handle = gpuResourceMgr.GetBufferHandle(name);
            } else if (handleType == RenderHandleType::GPU_IMAGE) {
                handle = gpuResourceMgr.GetImageHandle(name);
            } else {
                handle = gpuResourceMgr.GetSamplerHandle(name);
            }
            break;
        case (RenderNodeGraphResourceLocationType::FROM_RENDER_GRAPH_INPUT):
            handle = rngShareMgr.GetRenderNodeGraphInput(resourceIndex);
            break;
        case (RenderNodeGraphResourceLocationType::FROM_RENDER_GRAPH_OUTPUT):
            handle = rngShareMgr.GetRenderNodeGraphOutput(resourceIndex);
            break;
        case (RenderNodeGraphResourceLocationType::FROM_PREVIOUS_RENDER_NODE_OUTPUT):
            handle = rngShareMgr.GetRegisteredPrevRenderNodeOutput(resourceIndex);
            break;
        case (RenderNodeGraphResourceLocationType::FROM_NAMED_RENDER_NODE_OUTPUT):
            if (!name.empty()) {
                handle = rngShareMgr.GetRegisteredRenderNodeOutput(nodeName, name);
            } else {
                handle = rngShareMgr.GetRegisteredRenderNodeOutput(nodeName, resourceIndex);
            }
            break;
        case (RenderNodeGraphResourceLocationType::FROM_PREVIOUS_RENDER_NODE_GRAPH_OUTPUT):
            if (!name.empty()) {
                handle = rngShareMgr.GetNamedPrevRenderNodeGraphOutput(name);
            } else {
                handle = rngShareMgr.GetPrevRenderNodeGraphOutput(resourceIndex);
            }
            break;
        default:
            break;
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!RenderHandleUtil::IsValid(handle)) {
        PLUGIN_LOG_ONCE_W(string(nodeName + name),
            "RENDER_VALIDATION: named render node GPU resource handle not found (name:%s) (type:%u)", name.c_str(),
            static_cast<uint32_t>(handleType));
    }
#endif
    return handle;
}

void SetupRenderNodeResourceHandles(const IRenderNodeGpuResourceManager& gpuResourceMgr,
    const IRenderNodeGraphShareManager& rngShareMgr, const RenderNodeGraphInputs::InputResources& resources,
    RenderNodeHandles::InputResources& res)
{
    const auto setHandles = [](const IRenderNodeGpuResourceManager& gpuResourceMgr,
                                const IRenderNodeGraphShareManager& rngShareMgr, const RenderHandleType handleType,
                                const auto& input, auto& output) {
        output.reserve(input.size());
        for (const auto& ref : input) {
            const RenderHandle handle = GetRoutedResource(gpuResourceMgr, rngShareMgr, ref.name, ref.nodeName,
                ref.resourceLocation, ref.resourceIndex, handleType);
            output.push_back(RenderNodeResource { ref.set, ref.binding, handle, {}, ref.mip, ref.layer });
        }
    };
    const auto setImageHandles = [](const IRenderNodeGpuResourceManager& gpuResourceMgr,
                                     const IRenderNodeGraphShareManager& rngShareMgr,
                                     vector<RenderNodeResource>& optionalSamplers, auto& input, auto& output) {
        output.reserve(input.size());
        for (const auto& ref : input) {
            const RenderHandle handle = GetRoutedResource(gpuResourceMgr, rngShareMgr, ref.name, ref.nodeName,
                ref.resourceLocation, ref.resourceIndex, RenderHandleType::GPU_IMAGE);
            RenderHandle secondHandle;
            for (auto sampIter = optionalSamplers.begin(); sampIter != optionalSamplers.end();) {
                if ((sampIter->set == ref.set) && (sampIter->binding == ref.binding)) {
                    secondHandle = sampIter->handle;
                    optionalSamplers.erase(sampIter);
                } else {
                    sampIter++;
                }
            }
            output.push_back(RenderNodeResource { ref.set, ref.binding, handle, secondHandle, ref.mip, ref.layer });
        }
    };

    setHandles(gpuResourceMgr, rngShareMgr, RenderHandleType::GPU_BUFFER, resources.buffers, res.buffers);
    setHandles(gpuResourceMgr, rngShareMgr, RenderHandleType::GPU_SAMPLER, resources.samplers, res.samplers);
    // loops through samplers for possible combined image sampler (this method is usually only called in
    // RenderNodeInit())
    // If found, moves the sampler handle from res.samplers to res.images.secondHandle
    setImageHandles(gpuResourceMgr, rngShareMgr, res.samplers, resources.images, res.images);

    setHandles(gpuResourceMgr, rngShareMgr, RenderHandleType::GPU_BUFFER, resources.customInputBuffers,
        res.customInputBuffers);
    setHandles(gpuResourceMgr, rngShareMgr, RenderHandleType::GPU_BUFFER, resources.customOutputBuffers,
        res.customOutputBuffers);

    vector<RenderNodeResource> sams;
    setImageHandles(gpuResourceMgr, rngShareMgr, sams, resources.customInputImages, res.customInputImages);
    setImageHandles(gpuResourceMgr, rngShareMgr, sams, resources.customOutputImages, res.customOutputImages);
}
} // namespace

RenderNodeUtil::RenderNodeUtil(IRenderNodeContextManager& renderNodeContextMgr)
    : renderNodeContextMgr_(renderNodeContextMgr)
{}

RenderNodeHandles::InputRenderPass RenderNodeUtil::CreateInputRenderPass(
    const RenderNodeGraphInputs::InputRenderPass& renderPass) const
{
    RenderNodeHandles::InputRenderPass rp;
    const auto& gpuResourceMgr = renderNodeContextMgr_.GetGpuResourceManager();
    const auto& rngShareMgr = renderNodeContextMgr_.GetRenderNodeGraphShareManager();
    if (!renderPass.attachments.empty()) {
        rp.attachments.reserve(renderPass.attachments.size());
        for (const auto& ref : renderPass.attachments) {
            const RenderHandle handle = GetRoutedResource(gpuResourceMgr, rngShareMgr, ref.name, ref.nodeName,
                ref.resourceLocation, ref.resourceIndex, RenderHandleType::GPU_IMAGE);
            rp.attachments.push_back(RenderNodeAttachment { handle, ref.loadOp, ref.storeOp, ref.stencilLoadOp,
                ref.stencilStoreOp, ref.clearValue, ref.mip, ref.layer });
        }

        rp.subpassIndex = renderPass.subpassIndex;
        rp.subpassCount = renderPass.subpassCount;
        rp.subpassContents = renderPass.subpassContents;

        rp.depthAttachmentIndex = renderPass.depthAttachmentIndex;
        rp.depthResolveAttachmentIndex = renderPass.depthResolveAttachmentIndex;
        rp.inputAttachmentIndices = renderPass.inputAttachmentIndices;
        rp.colorAttachmentIndices = renderPass.colorAttachmentIndices;
        rp.resolveAttachmentIndices = renderPass.resolveAttachmentIndices;
        rp.fragmentShadingRateAttachmentIndex = renderPass.fragmentShadingRateAttachmentIndex;
        rp.depthResolveModeFlagBit = renderPass.depthResolveModeFlagBit;
        rp.stencilResolveModeFlagBit = renderPass.stencilResolveModeFlagBit;
        rp.shadingRateTexelSize = renderPass.shadingRateTexelSize;
        rp.viewMask = renderPass.viewMask;
    }

    return rp;
}

RenderNodeHandles::InputResources RenderNodeUtil::CreateInputResources(
    const RenderNodeGraphInputs::InputResources& resources) const
{
    RenderNodeHandles::InputResources res;
    const auto& gpuResourceMgr = renderNodeContextMgr_.GetGpuResourceManager();
    const auto& rngShareMgr = renderNodeContextMgr_.GetRenderNodeGraphShareManager();
    SetupRenderNodeResourceHandles(gpuResourceMgr, rngShareMgr, resources, res);
    return res;
}

PipelineLayout RenderNodeUtil::CreatePipelineLayout(const RenderHandle& shaderHandle) const
{
    const auto& shaderMgr = renderNodeContextMgr_.GetShaderManager();
    RenderHandle plHandle = shaderMgr.GetPipelineLayoutHandleByShaderHandle(shaderHandle);
    if (RenderHandleUtil::IsValid(plHandle)) {
        return shaderMgr.GetPipelineLayout(plHandle);
    } else {
        return shaderMgr.GetReflectionPipelineLayout(shaderHandle);
    }
}

IPipelineDescriptorSetBinder::Ptr RenderNodeUtil::CreatePipelineDescriptorSetBinder(
    const PipelineLayout& pipelineLayout) const
{
    auto& descriptorSetMgr = renderNodeContextMgr_.GetDescriptorSetManager();
    return descriptorSetMgr.CreatePipelineDescriptorSetBinder(pipelineLayout);
}

void RenderNodeUtil::BindResourcesToBinder(
    const RenderNodeHandles::InputResources& resources, IPipelineDescriptorSetBinder& pipelineDescriptorSetBinder) const
{
    pipelineDescriptorSetBinder.ClearBindings();
    for (const auto& ref : resources.buffers) {
        if (RenderHandleUtil::IsValid(ref.handle)) {
            BindableBuffer bindable;
            bindable.handle = ref.handle;
            pipelineDescriptorSetBinder.BindBuffer(ref.set, ref.binding, bindable);
        }
    }
    for (const auto& ref : resources.images) {
        if (RenderHandleUtil::IsValid(ref.handle)) {
            BindableImage bindable;
            bindable.handle = ref.handle;
            bindable.samplerHandle = ref.secondHandle; // might be combined image sampler if valid handle
            bindable.mip = ref.mip;
            bindable.layer = ref.layer;
            pipelineDescriptorSetBinder.BindImage(ref.set, ref.binding, bindable);
        }
    }
    for (const auto& ref : resources.samplers) {
        if (RenderHandleUtil::IsValid(ref.handle)) {
            BindableSampler bindable;
            bindable.handle = ref.handle;
            pipelineDescriptorSetBinder.BindSampler(ref.set, ref.binding, bindable);
        }
    }
}

DescriptorCounts RenderNodeUtil::GetDescriptorCounts(const PipelineLayout& pipelineLayout) const
{
    DescriptorCounts dc;
    for (uint32_t setIdx = 0; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
        const auto& setRef = pipelineLayout.descriptorSetLayouts[setIdx];
        if (setRef.set == PipelineLayoutConstants::INVALID_INDEX) {
            continue;
        }
        dc.counts.reserve(dc.counts.size() + setRef.bindings.size());
        for (const auto& bindingRef : setRef.bindings) {
            dc.counts.push_back(DescriptorCounts::TypedCount { bindingRef.descriptorType, bindingRef.descriptorCount });
        }
    }
    return dc;
}

DescriptorCounts RenderNodeUtil::GetDescriptorCounts(const array_view<DescriptorSetLayoutBinding> bindings) const
{
    DescriptorCounts dc;
    for (const auto& bindingRef : bindings) {
        dc.counts.push_back(DescriptorCounts::TypedCount { bindingRef.descriptorType, bindingRef.descriptorCount });
    }
    return dc;
}

RenderPass RenderNodeUtil::CreateRenderPass(const RenderNodeHandles::InputRenderPass& renderPass) const
{
    RenderPass rp;
    RenderPassDesc& rpDesc = rp.renderPassDesc;

    const uint32_t attachmentCount = (uint32_t)renderPass.attachments.size();
    PLUGIN_ASSERT(attachmentCount <= PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT);
    uint32_t width = ~0u;
    uint32_t height = ~0u;
    if (attachmentCount > 0) {
        rp.renderPassDesc.attachmentCount = attachmentCount;
        for (size_t idx = 0; idx < renderPass.attachments.size(); ++idx) {
            const auto& ref = renderPass.attachments[idx];
            rpDesc.attachments[idx] = { ref.layer, ref.mip, ref.loadOp, ref.storeOp, ref.stencilLoadOp,
                ref.stencilStoreOp, ref.clearValue };
            rpDesc.attachmentHandles[idx] = ref.handle;
            if (idx == 0) { // optimization, width and height must match (will end in error later)
                const GpuImageDesc desc = renderNodeContextMgr_.GetGpuResourceManager().GetImageDescriptor(ref.handle);
                width = desc.width;
                height = desc.height;
            }
        }

        rpDesc.subpassContents = renderPass.subpassContents;
        rpDesc.subpassCount = renderPass.subpassCount;
        if (renderPass.subpassIndex >= renderPass.subpassCount) {
            PLUGIN_LOG_E("Render pass subpass idx < count (%u < %u)", renderPass.subpassIndex, renderPass.subpassCount);
        }
        if (renderPass.subpassIndex < renderPass.subpassCount) {
            rp.subpassStartIndex = renderPass.subpassIndex;
            // update the one subpass described in render node graph
            auto& spDesc = rp.subpassDesc;

            spDesc.inputAttachmentCount = (uint32_t)renderPass.inputAttachmentIndices.size();
            spDesc.colorAttachmentCount = (uint32_t)renderPass.colorAttachmentIndices.size();
            spDesc.resolveAttachmentCount = (uint32_t)renderPass.resolveAttachmentIndices.size();

            spDesc.depthAttachmentCount = (renderPass.depthAttachmentIndex != ~0u) ? 1u : 0u;
            spDesc.depthAttachmentIndex = (spDesc.depthAttachmentCount > 0) ? renderPass.depthAttachmentIndex : ~0u;
            spDesc.depthResolveAttachmentCount = (renderPass.depthResolveAttachmentIndex != ~0u) ? 1u : 0u;
            spDesc.depthResolveAttachmentIndex =
                (spDesc.depthResolveAttachmentCount > 0) ? renderPass.depthResolveAttachmentIndex : ~0u;
            spDesc.depthResolveModeFlagBit = renderPass.depthResolveModeFlagBit;
            spDesc.stencilResolveModeFlagBit = renderPass.stencilResolveModeFlagBit;

            spDesc.fragmentShadingRateAttachmentCount =
                (renderPass.fragmentShadingRateAttachmentIndex != ~0u) ? 1u : 0u;
            spDesc.fragmentShadingRateAttachmentIndex =
                (spDesc.fragmentShadingRateAttachmentCount > 0) ? renderPass.fragmentShadingRateAttachmentIndex : ~0u;
            spDesc.shadingRateTexelSize = renderPass.shadingRateTexelSize;
            spDesc.viewMask = renderPass.viewMask;

            for (uint32_t idx = 0; idx < spDesc.inputAttachmentCount; ++idx) {
                spDesc.inputAttachmentIndices[idx] = renderPass.inputAttachmentIndices[idx];
            }
            for (uint32_t idx = 0; idx < spDesc.colorAttachmentCount; ++idx) {
                spDesc.colorAttachmentIndices[idx] = renderPass.colorAttachmentIndices[idx];
            }
            for (uint32_t idx = 0; idx < spDesc.resolveAttachmentCount; ++idx) {
                spDesc.resolveAttachmentIndices[idx] = renderPass.resolveAttachmentIndices[idx];
            }
        }
    }
    rpDesc.renderArea = { 0, 0, width, height };

    return rp;
}

ViewportDesc RenderNodeUtil::CreateDefaultViewport(const RenderPass& renderPass) const
{
    return ViewportDesc {
        0.0f,
        0.0f,
        static_cast<float>(renderPass.renderPassDesc.renderArea.extentWidth),
        static_cast<float>(renderPass.renderPassDesc.renderArea.extentHeight),
        0.0f,
        1.0f,
    };
}

ScissorDesc RenderNodeUtil::CreateDefaultScissor(const RenderPass& renderPass) const
{
    return ScissorDesc {
        0,
        0,
        renderPass.renderPassDesc.renderArea.extentWidth,
        renderPass.renderPassDesc.renderArea.extentHeight,
    };
}

RenderPostProcessConfiguration RenderNodeUtil::GetRenderPostProcessConfiguration(
    const PostProcessConfiguration& input) const
{
    RenderPostProcessConfiguration output;
    std::fill(output.factors, output.factors + PostProcessConfiguration::INDEX_FACTOR_COUNT,
        Math::Vec4 { 0.0f, 0.0f, 0.0f, 0.0f });

    output.flags = { input.enableFlags, 0, 0, 0 };
    output.renderTimings = renderNodeContextMgr_.GetRenderNodeGraphData().renderingConfiguration.renderTimings;
    output.factors[PostProcessConfiguration::INDEX_TONEMAP] = PostProcessConversionHelper::GetFactorTonemap(input);
    output.factors[PostProcessConfiguration::INDEX_VIGNETTE] = PostProcessConversionHelper::GetFactorVignette(input);
    output.factors[PostProcessConfiguration::INDEX_DITHER] = PostProcessConversionHelper::GetFactorDither(input);
    output.factors[PostProcessConfiguration::INDEX_COLOR_CONVERSION] =
        PostProcessConversionHelper::GetFactorColorConversion(input);
    output.factors[PostProcessConfiguration::INDEX_COLOR_FRINGE] = PostProcessConversionHelper::GetFactorFringe(input);

    output.factors[PostProcessConfiguration::INDEX_BLUR] = PostProcessConversionHelper::GetFactorBlur(input);
    output.factors[PostProcessConfiguration::INDEX_BLOOM] = PostProcessConversionHelper::GetFactorBloom(input);
    output.factors[PostProcessConfiguration::INDEX_FXAA] = PostProcessConversionHelper::GetFactorFxaa(input);
    output.factors[PostProcessConfiguration::INDEX_TAA] = PostProcessConversionHelper::GetFactorTaa(input);
    output.factors[PostProcessConfiguration::INDEX_DOF] = PostProcessConversionHelper::GetFactorDof(input);
    output.factors[PostProcessConfiguration::INDEX_MOTION_BLUR] =
        PostProcessConversionHelper::GetFactorMotionBlur(input);

    BASE_NS::CloneData(output.userFactors, sizeof(output.userFactors), input.userFactors, sizeof(input.userFactors));

    return output;
}

RenderPostProcessConfiguration RenderNodeUtil::GetRenderPostProcessConfiguration(
    const IRenderDataStorePostProcess::GlobalFactors& globalFactors) const
{
    RenderPostProcessConfiguration output;

    output.flags = { globalFactors.enableFlags, 0, 0, 0 };
    output.renderTimings = renderNodeContextMgr_.GetRenderNodeGraphData().renderingConfiguration.renderTimings;

    BASE_NS::CloneData(output.factors, sizeof(output.factors), globalFactors.factors, sizeof(globalFactors.factors));
    BASE_NS::CloneData(
        output.userFactors, sizeof(output.userFactors), globalFactors.userFactors, sizeof(globalFactors.userFactors));

    return output;
}

bool RenderNodeUtil::HasChangeableResources(const RenderNodeGraphInputs::InputRenderPass& renderPass) const
{
    bool changeable = false;
    for (const auto& ref : renderPass.attachments) {
        if (ref.resourceLocation != RenderNodeGraphResourceLocationType::DEFAULT) {
            changeable = true;
        }
    }
    return changeable;
}

bool RenderNodeUtil::HasChangeableResources(const RenderNodeGraphInputs::InputResources& resources) const
{
    auto HasChangingResources = [](const auto& vec) {
        bool changeable = false;
        for (const auto& ref : vec) {
            if (ref.resourceLocation != RenderNodeGraphResourceLocationType::DEFAULT) {
                changeable = true;
            }
        }
        return changeable;
    };
    bool changeable = false;
    changeable = changeable || HasChangingResources(resources.buffers);
    changeable = changeable || HasChangingResources(resources.images);
    changeable = changeable || HasChangingResources(resources.samplers);
    changeable = changeable || HasChangingResources(resources.customInputBuffers);
    changeable = changeable || HasChangingResources(resources.customOutputBuffers);
    changeable = changeable || HasChangingResources(resources.customInputImages);
    changeable = changeable || HasChangingResources(resources.customOutputImages);
    return changeable;
}
RENDER_END_NAMESPACE()
