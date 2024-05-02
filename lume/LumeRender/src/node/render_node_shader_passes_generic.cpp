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

#include "render_node_shader_passes_generic.h"

#include <base/math/mathf.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
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
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "datastore/render_data_store_shader_passes.h"
#include "device/shader_pipeline_binder.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };
constexpr uint32_t OVERESTIMATE_COUNT { 4U };

constexpr uint32_t Align(uint32_t value, uint32_t align)
{
    if (value == 0) {
        return 0;
    }
    return ((value + align) / align) * align;
}

RenderPass ConvertToLowLevelRenderPass(const RenderPassWithHandleReference& renderPassInput)
{
    RenderPass rp;
    rp.subpassDesc = renderPassInput.subpassDesc;
    rp.subpassStartIndex = renderPassInput.subpassStartIndex;
    const uint32_t attachmentCount = Math::min(
        PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT, renderPassInput.renderPassDesc.attachmentCount);
    rp.renderPassDesc.attachmentCount = attachmentCount;
    CloneData(rp.renderPassDesc.attachments, sizeof(rp.renderPassDesc.attachments),
        renderPassInput.renderPassDesc.attachments, sizeof(renderPassInput.renderPassDesc.attachments));
    rp.renderPassDesc.renderArea = renderPassInput.renderPassDesc.renderArea;
    rp.renderPassDesc.subpassCount = renderPassInput.renderPassDesc.subpassCount;
    for (uint32_t idx = 0U; idx < attachmentCount; ++idx) {
        rp.renderPassDesc.attachmentHandles[idx] = renderPassInput.renderPassDesc.attachmentHandles[idx].GetHandle();
    }
    return rp;
}
} // namespace

void RenderNodeShaderPassesGeneric::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    valid_ = true;
    uboData_ = {};

    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    if (!jsonInputs_.renderDataStore.dataStoreName.empty()) {
        if (jsonInputs_.renderDataStore.typeName != RenderDataStoreShaderPasses::TYPE_NAME) {
            PLUGIN_LOG_W("RENDER_VALIDATION: RenderNodeShaderPassesGeneric only supports render data store with "
                "typename RenderDataStoreShaderPasses (typename: %s)",
                jsonInputs_.renderDataStore.typeName.c_str());
            valid_ = false;
        }
    }
    if (jsonInputs_.renderDataStore.dataStoreName.empty()) {
        // use default if not given
        jsonInputs_.renderDataStore.dataStoreName = RenderDataStoreShaderPasses::TYPE_NAME;
        jsonInputs_.renderDataStore.typeName = RenderDataStoreShaderPasses::TYPE_NAME;
    }
}

void RenderNodeShaderPassesGeneric::PreExecuteFrame()
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    dsShaderPasses_ = static_cast<RenderDataStoreShaderPasses*>(
        renderDataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName));
    if (!dsShaderPasses_) {
        return;
    }

    // process amount of data needed for UBO
    uint32_t uboByteSize = 0U;
    uboByteSize += dsShaderPasses_->GetRenderPropertyBindingInfo().alignedByteSize;
    uboByteSize += dsShaderPasses_->GetComputePropertyBindingInfo().alignedByteSize;
    if (uboByteSize > uboData_.byteSize) {
        uboData_.byteSize = uboByteSize + (OFFSET_ALIGNMENT * OVERESTIMATE_COUNT);
        uboData_.byteSize = static_cast<uint32_t>(Align(uboData_.byteSize, OFFSET_ALIGNMENT));

        IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        uboData_.handle = gpuResourceMgr.Create(
            uboData_.handle, GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, uboData_.byteSize });
    }
}

void RenderNodeShaderPassesGeneric::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!valid_) {
        return;
    }
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    dsShaderPasses_ = static_cast<RenderDataStoreShaderPasses*>(
        renderDataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName));
    if (!dsShaderPasses_) {
        return;
    }

    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (uboData_.handle) {
        uboData_.mapData = static_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboData_.handle.GetHandle()));
        uboData_.currentOffset = 0U;
    }

    ProcessExecuteData();
    ExecuteFrameGraphics(cmdList);
    ExecuteFrameCompute(cmdList);

    if (uboData_.handle) {
        gpuResourceMgr.UnmapBuffer(uboData_.handle.GetHandle());
    }
    uboData_.mapData = nullptr;
    uboData_.currentOffset = 0U;
}

void RenderNodeShaderPassesGeneric::ProcessExecuteData()
{
    if (!dsShaderPasses_) {
        return;
    }

    // NOTE: does not process descriptors, uses one frame descriptors at the moment
}

void RenderNodeShaderPassesGeneric::ExecuteFrameGraphics(IRenderCommandList& cmdList)
{
    if (!dsShaderPasses_) {
        return;
    }

    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    const IRenderNodeUtil& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();

    const auto rpData = dsShaderPasses_->GetRenderData();
    for (const auto& ref : rpData) {
        if (ValidRenderPass(ref.renderPass)) {
            const RenderPass renderPass = ConvertToLowLevelRenderPass(ref.renderPass);
            cmdList.BeginRenderPass(renderPass.renderPassDesc, { &renderPass.subpassDesc, 1U });

            const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
            const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);
            cmdList.SetDynamicStateViewport(viewportDesc);
            cmdList.SetDynamicStateScissor(scissorDesc);
            if (ref.renderPass.subpassDesc.fragmentShadingRateAttachmentCount > 0) {
                cmdList.SetDynamicStateFragmentShadingRate(
                    { 1u, 1u },
                    FragmentShadingRateCombinerOps { 
                        CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE,
                        CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE });
            }

            for (const auto& shaderRef : ref.shaderBinders) {
                if (!shaderRef) {
                    continue;
                }
                const auto& sRef = (const ShaderPipelineBinder&)(*shaderRef.get());
                IPipelineDescriptorSetBinder* binder = sRef.GetPipelineDescriptorSetBinder();
                if (!binder) {
                    continue;
                }
                const PipelineLayout& pl = sRef.GetPipelineLayout();
                const RenderHandle psoHandle = GetPsoHandleGraphics(renderPass, sRef.GetShaderHandle().GetHandle(), pl);
                cmdList.BindPipeline(psoHandle);

                // copy custom property data
                {
                    IShaderPipelineBinder::PropertyBindingView cpbv = shaderRef->GetPropertyBindingView();
                    if (!cpbv.data.empty()) {
                        const uint32_t byteOffset = WriteLocalUboData(cpbv.data);
                        BindableBuffer bRes;
                        bRes.handle = uboData_.handle.GetHandle();
                        bRes.byteOffset = byteOffset;
                        binder->BindBuffer(cpbv.set, cpbv.binding, bRes);
                    }
                }

                // create single frame descriptor sets
                RenderHandle descriptorSetHandles[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] {};
                uint32_t setCount = 0U;
                for (uint32_t setIdx = 0U; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
                    const auto& currSet = pl.descriptorSetLayouts[setIdx];
                    if (currSet.set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
                        descriptorSetHandles[setIdx] = descriptorSetMgr.CreateOneFrameDescriptorSet(currSet.bindings);
                        cmdList.UpdateDescriptorSet(
                            descriptorSetHandles[setIdx], binder->GetDescriptorSetLayoutBindingResources(currSet.set));
                        setCount++;
                    }
                }
                cmdList.BindDescriptorSets(binder->GetFirstSet(), { descriptorSetHandles, setCount });

                // vertex buffers and draw
                const array_view<const VertexBufferWithHandleReference> vb = sRef.GetVertexBuffers();
                const IndexBufferWithHandleReference ib = sRef.GetIndexBuffer();
                const IShaderPipelineBinder::DrawCommand dc = sRef.GetDrawCommand();
                if (!vb.empty()) {
                    VertexBuffer vbs[PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
                    const uint32_t vbCount =
                        Math::min(PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT, static_cast<uint32_t>(vb.size()));
                    for (uint32_t vbIdx = 0; vbIdx < vbCount; ++vbIdx) {
                        const auto& vr = vb[vbIdx];
                        vbs[vbIdx] = { vr.bufferHandle.GetHandle(), vr.bufferOffset, vr.byteSize };
                    }
                    cmdList.BindVertexBuffers({ vbs, vbCount });
                }
                const RenderHandle iaHandle = dc.argsHandle.GetHandle();
                const bool indirectDraw = RenderHandleUtil::IsValid(iaHandle);
                if (ib.bufferHandle) {
                    cmdList.BindIndexBuffer(
                        { ib.bufferHandle.GetHandle(), ib.bufferOffset, ib.byteSize, ib.indexType });
                    if (indirectDraw) {
                        cmdList.DrawIndexedIndirect(iaHandle, dc.argsOffset, 1U, 0U);
                    } else {
                        cmdList.DrawIndexed(dc.indexCount, dc.instanceCount, 0U, 0U, 0U);
                    }
                } else {
                    if (indirectDraw) {
                        cmdList.DrawIndirect(iaHandle, dc.argsOffset, 1U, 0U);
                    } else {
                        cmdList.Draw(dc.vertexCount, dc.instanceCount, 0, 0);
                    }
                }
            }
            cmdList.EndRenderPass();
        }
    }
}

void RenderNodeShaderPassesGeneric::ExecuteFrameCompute(IRenderCommandList& cmdList)
{
    if (!dsShaderPasses_) {
        return;
    }

    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    const auto cData = dsShaderPasses_->GetComputeData();
    for (const auto& ref : cData) {
        for (const auto& shaderRef : ref.shaderBinders) {
            if (!shaderRef) {
                continue;
            }
            const auto& sRef = (const ShaderPipelineBinder &)(*shaderRef.get());
            IPipelineDescriptorSetBinder* binder = sRef.GetPipelineDescriptorSetBinder();
            if (!binder) {
                continue;
            }

            const RenderHandle shaderHandle = sRef.GetShaderHandle().GetHandle();
            const PipelineLayout& pl = sRef.GetPipelineLayout();
            const RenderHandle psoHandle = GetPsoHandleCompute(shaderHandle, pl);
            cmdList.BindPipeline(psoHandle);

            // copy custom property data
            {
                IShaderPipelineBinder::PropertyBindingView cpbv = shaderRef->GetPropertyBindingView();
                if (!cpbv.data.empty()) {
                    const uint32_t byteOffset = WriteLocalUboData(cpbv.data);
                    BindableBuffer bRes;
                    bRes.handle = uboData_.handle.GetHandle();
                    bRes.byteOffset = byteOffset;
                    binder->BindBuffer(cpbv.set, cpbv.binding, bRes);
                }
            }

            // create single frame descriptor sets
            RenderHandle descriptorSetHandles[PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT] {};
            uint32_t setCount = 0U;
            for (uint32_t setIdx = 0U; setIdx < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT; ++setIdx) {
                const auto& currSet = pl.descriptorSetLayouts[setIdx];
                if (currSet.set < PipelineLayoutConstants::MAX_DESCRIPTOR_SET_COUNT) {
                    descriptorSetHandles[setIdx] = descriptorSetMgr.CreateOneFrameDescriptorSet(currSet.bindings);
                    cmdList.UpdateDescriptorSet(
                        descriptorSetHandles[setIdx], binder->GetDescriptorSetLayoutBindingResources(currSet.set));
                    setCount++;
                }
            }
            cmdList.BindDescriptorSets(binder->GetFirstSet(), { descriptorSetHandles, setCount });

            const IShaderPipelineBinder::DispatchCommand dc = sRef.GetDispatchCommand();
            const RenderHandle dcHandle = dc.handle.GetHandle();
            const RenderHandleType handleType = RenderHandleUtil::GetHandleType(dcHandle);
            if (handleType == RenderHandleType::GPU_BUFFER) {
                cmdList.DispatchIndirect(dcHandle, dc.argsOffset);
            } else if (handleType == RenderHandleType::GPU_IMAGE) {
                const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
                const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(dcHandle);
                const Math::UVec3 targetSize = { desc.width, desc.height, desc.depth };
                const ShaderThreadGroup tgs = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);
                cmdList.Dispatch((targetSize.x + tgs.x - 1u) / tgs.x, (targetSize.y + tgs.y - 1u) / tgs.y,
                    (targetSize.z + tgs.z - 1u) / tgs.z);
            } else {
                cmdList.Dispatch(dc.threadGroupCount.x, dc.threadGroupCount.y, dc.threadGroupCount.z);
            }
        }
    }
}

uint32_t RenderNodeShaderPassesGeneric::WriteLocalUboData(const array_view<const uint8_t> uboData)
{
    uint32_t retOffset = 0U;
    if (uboData_.mapData && (!uboData.empty())) {
        retOffset = uboData_.currentOffset;
        PLUGIN_ASSERT((uboData_.currentOffset + uboData.size_bytes()) < uboData_.byteSize);
        if ((uboData_.currentOffset + uboData.size_bytes()) < uboData_.byteSize) {
            uint8_t* currData = uboData_.mapData + uboData_.currentOffset;
            const uint8_t* dataPtrEnd = uboData_.mapData + uboData_.byteSize;
            const uint32_t writeDataByteSize = static_cast<uint32_t>(uboData.size_bytes());
            CloneData(currData, size_t(dataPtrEnd - currData), uboData.data(), uboData.size_bytes());

            uboData_.currentOffset = Align(uboData_.currentOffset + writeDataByteSize, OFFSET_ALIGNMENT);
        }
    }
    return retOffset;
}

RenderHandle RenderNodeShaderPassesGeneric::GetPsoHandleGraphics(
    const RenderPass& renderPass, const RenderHandle& shader, const PipelineLayout& pipelineLayout)
{
    // controlled by count
    constexpr DynamicStateEnum dynamicStates[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR,
        CORE_DYNAMIC_STATE_ENUM_FRAGMENT_SHADING_RATE };
    const uint32_t dynamicStateCount = (renderPass.subpassDesc.fragmentShadingRateAttachmentIndex != ~0u) ? 3u : 2u;
    const RenderHandle gfxStateHandle =
        renderNodeContextMgr_->GetShaderManager().GetGraphicsStateHandleByShaderHandle(shader);
    return renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(
        shader, gfxStateHandle, pipelineLayout, {}, {}, { dynamicStates, dynamicStateCount });
}

RenderHandle RenderNodeShaderPassesGeneric::GetPsoHandleCompute(
    const RenderHandle& shader, const PipelineLayout& pipelineLayout)
{
    return renderNodeContextMgr_->GetPsoManager().GetComputePsoHandle(shader, pipelineLayout, {});
}

void RenderNodeShaderPassesGeneric::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");
}

bool RenderNodeShaderPassesGeneric::ValidRenderPass(const RenderPassWithHandleReference& renderPass)
{
    if ((renderPass.renderPassDesc.attachmentCount > 0U) && renderPass.renderPassDesc.attachmentHandles[0U]) {
        return true;
    } else {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "_invalid_rp", "Invalid render pass in node: %s",
            renderNodeContextMgr_->GetNodeName().data());
#endif
        return false;
    }
}

// for plugin / factory interface
IRenderNode* RenderNodeShaderPassesGeneric::Create()
{
    return new RenderNodeShaderPassesGeneric();
}

void RenderNodeShaderPassesGeneric::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeShaderPassesGeneric*>(instance);
}
RENDER_END_NAMESPACE()