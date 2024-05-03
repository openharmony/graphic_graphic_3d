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

#include "render_node_morph.h"

#include <algorithm>

#include <3d/render/intf_render_data_store_morph.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>

namespace {
#include "3d/shaders/common/morph_target_structs.h"

constexpr const uint32_t SET_WEIGHTS = 0u;
constexpr const uint32_t SET_INPUTS = 1u;
constexpr const uint32_t SET_OUTPUTS = 2u;

constexpr const uint32_t BUFFER_ALIGN = 0x100; // on Nvidia = 0x20, on Mali and Intel = 0x10, SBO on Mali = 0x100

inline size_t Align(size_t value, size_t align)
{
    if (align == 0U) {
        return value;
    }

    return ((value + align - 1U) / align) * align;
}
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

void RenderNodeMorph::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    auto& shaderMgr = renderNodeContextMgr.GetShaderManager();

    {
        const RenderHandle shaderHandle = shaderMgr.GetShaderHandle("3dshaders://computeshader/core3d_dm_morph.shader");
        threadGroupSize_ = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);
        pipelineLayout_ = shaderMgr.GetReflectionPipelineLayout(shaderHandle);

        auto& psoMgr = renderNodeContextMgr.GetPsoManager();
        psoHandle_ = psoMgr.GetComputePsoHandle(shaderHandle, pipelineLayout_, {});
    }
}

void RenderNodeMorph::PreExecuteFrame()
{
    hasExecuteData_ = false;

    // re-create needed gpu resources
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* morphDataStore =
        static_cast<IRenderDataStoreMorph*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMorph));
    if (!morphDataStore) {
        return;
    }

    const auto submeshes = morphDataStore->GetSubmeshes();
    if (submeshes.empty()) {
        return;
    }

    hasExecuteData_ = true;
    if (maxObjectCount_ < submeshes.size()) {
        maxObjectCount_ = static_cast<uint32_t>(submeshes.size() + submeshes.size() / 2u);

        auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        const DescriptorCounts dc { { // weight/indexset for all prims + number of inputs and outputs
            { CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u + (2u + 3u) * maxObjectCount_ } } };
        descriptorSetMgr.ResetAndReserve(dc);
        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(SET_WEIGHTS, pipelineLayout_);
            allDescriptorSets_.params = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, pipelineLayout_.descriptorSetLayouts[SET_WEIGHTS].bindings);
        }
        {
            allDescriptorSets_.inputs.resize(maxObjectCount_);
            for (uint32_t idx = 0; idx < maxObjectCount_; ++idx) {
                const RenderHandle descriptorSetHandle =
                    descriptorSetMgr.CreateDescriptorSet(SET_INPUTS, pipelineLayout_);
                allDescriptorSets_.inputs[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                    descriptorSetHandle, pipelineLayout_.descriptorSetLayouts[SET_INPUTS].bindings);
            }
        }
        {
            allDescriptorSets_.outputs.resize(maxObjectCount_);
            for (uint32_t idx = 0; idx < maxObjectCount_; ++idx) {
                const RenderHandle descriptorSetHandle =
                    descriptorSetMgr.CreateDescriptorSet(SET_OUTPUTS, pipelineLayout_);
                allDescriptorSets_.outputs[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                    descriptorSetHandle, pipelineLayout_.descriptorSetLayouts[SET_OUTPUTS].bindings);
            }
        }
    }

    uint32_t structCount = 0u;
    for (const auto& submesh : submeshes) {
        structCount += (static_cast<uint32_t>(submesh.activeTargets.size()) + 3u) / 4u;
    }
    if (maxStructCount_ < structCount) {
        maxStructCount_ = structCount + structCount / 2u;
        const uint32_t sizeOfBuffer = maxStructCount_ * sizeof(::MorphTargetInfoStruct);
        if (bufferSize_ < sizeOfBuffer) {
            bufferSize_ = static_cast<uint32_t>(Align(sizeOfBuffer, BUFFER_ALIGN));
            auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
            morphTargetBufferHandle_ = gpuResourceMgr.Create(morphTargetBufferHandle_,
                GpuBufferDesc { CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                    (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                    CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, bufferSize_ });
        }
    }
}

IRenderNode::ExecuteFlags RenderNodeMorph::GetExecuteFlags() const
{
    if (hasExecuteData_) {
        return 0U;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderNodeMorph::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* morphDataStore =
        static_cast<IRenderDataStoreMorph*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMorph));
    if (!morphDataStore) {
        return;
    }
    array_view<const RenderDataMorph::Submesh> submeshes = morphDataStore->GetSubmeshes();
    if (submeshes.empty()) {
        return;
    }

    const uint32_t maxSubmeshCount = std::min((uint32_t)submeshes.size(), maxObjectCount_);
    submeshes = array_view(submeshes.data(), maxSubmeshCount);

    UpdateWeightsAndTargets(submeshes);

    ComputeMorphs(cmdList, submeshes);
}

void RenderNodeMorph::UpdateWeightsAndTargets(array_view<const RenderDataMorph::Submesh> submeshes)
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    auto morphData =
        reinterpret_cast<::MorphTargetInfoStruct*>(gpuResourceMgr.MapBuffer(morphTargetBufferHandle_.GetHandle()));
    if (morphData) {
        uint32_t offset = 0;
        for (const RenderDataMorph::Submesh& submesh : submeshes) {
            const auto& activeTargets = submesh.activeTargets;
            const auto blockSize = (static_cast<uint32_t>(activeTargets.size()) + 3u) / 4u;
            // Assert that the maximum active morph target count is not reached.
            CORE_ASSERT((offset + blockSize) < bufferSize_);
            if ((offset + blockSize) < bufferSize_) {
                for (size_t i = 0; i < activeTargets.size(); i++) {
                    morphData[offset + i / 4u].target[i % 4u] = activeTargets[i].id;
                    morphData[offset + i / 4u].weight[i % 4u] = activeTargets[i].weight;
                }
            }
            offset += blockSize;
        }
        // Could there be some way to mark the modified range of buffer dirty?
        gpuResourceMgr.UnmapBuffer(morphTargetBufferHandle_.GetHandle());
    }
}

void RenderNodeMorph::ComputeMorphs(IRenderCommandList& cmdList, array_view<const RenderDataMorph::Submesh> submeshes)
{
    cmdList.BindPipeline(psoHandle_);
    // set 0
    {
        auto& binder = *allDescriptorSets_.params;
        binder.BindBuffer(0u, morphTargetBufferHandle_.GetHandle(), 0u);
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());

        cmdList.BindDescriptorSet(SET_WEIGHTS, binder.GetDescriptorSetHandle());
    }
    uint32_t offset = 0;
    uint32_t outputIdx = 0u;
    uint32_t inputIdx = 0u;
    for (const RenderDataMorph::Submesh& submesh : submeshes) {
        const auto blockSize = (static_cast<uint32_t>(submesh.activeTargets.size()) + 3u) / 4u;
        if ((offset + blockSize) < bufferSize_) {
            // Bind inputs = target position (set 1)
            auto& inputBinder = *allDescriptorSets_.inputs[inputIdx++];

            const auto indexOffset = submesh.morphTargetBuffer.bufferOffset;
            const auto indexSize = static_cast<uint32_t>(
                Align((submesh.vertexCount * submesh.morphTargetCount * static_cast<uint32_t>(sizeof(uint32_t))),
                    BUFFER_ALIGN));

            inputBinder.BindBuffer(0u, submesh.morphTargetBuffer.bufferHandle.GetHandle(), indexOffset, indexSize);

            const auto vertexOffset = indexOffset + indexSize;
            const auto vertexSize = submesh.morphTargetBuffer.byteSize - indexSize;
            inputBinder.BindBuffer(1u, submesh.morphTargetBuffer.bufferHandle.GetHandle(), vertexOffset, vertexSize);
            cmdList.UpdateDescriptorSet(
                inputBinder.GetDescriptorSetHandle(), inputBinder.GetDescriptorSetLayoutBindingResources());

            // Bind outputs = pos/nor/tangent buffers (set 2)
            auto& outputBinder = *allDescriptorSets_.outputs[outputIdx++];
            {
                outputBinder.BindBuffer(0u, submesh.vertexBuffers[0u].bufferHandle.GetHandle(),
                    submesh.vertexBuffers[0u].bufferOffset,
                    submesh.vertexBuffers[0u].byteSize); // position
                outputBinder.BindBuffer(1u, submesh.vertexBuffers[1u].bufferHandle.GetHandle(),
                    submesh.vertexBuffers[1u].bufferOffset,
                    submesh.vertexBuffers[1u].byteSize); // normal
                outputBinder.BindBuffer(2u, submesh.vertexBuffers[2u].bufferHandle.GetHandle(),
                    submesh.vertexBuffers[2u].bufferOffset,
                    submesh.vertexBuffers[2u].byteSize); // tangent

                cmdList.UpdateDescriptorSet(
                    outputBinder.GetDescriptorSetHandle(), outputBinder.GetDescriptorSetLayoutBindingResources());
            }
            const RenderHandle sets[] = { inputBinder.GetDescriptorSetHandle(), outputBinder.GetDescriptorSetHandle() };
            cmdList.BindDescriptorSets(SET_INPUTS, sets);

            const ::MorphObjectPushConstantStruct pushData { offset, submesh.vertexCount, submesh.morphTargetCount,
                static_cast<uint32_t>(submesh.activeTargets.size()) };
            cmdList.PushConstant(pipelineLayout_.pushConstant, reinterpret_cast<const uint8_t*>(&pushData));
            cmdList.Dispatch((submesh.vertexCount + threadGroupSize_.x - 1) / threadGroupSize_.x, 1, 1);
        }
        offset += blockSize;
    }
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeMorph::Create()
{
    return new RenderNodeMorph();
}

void RenderNodeMorph::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeMorph*>(instance);
}
CORE3D_END_NAMESPACE()
