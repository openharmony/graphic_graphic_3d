/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
constexpr uint32_t SET_UBO = 0u;
constexpr uint32_t SET_INPUTS = 1u;
constexpr uint32_t SET_OUTPUTS = 2u;
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

    maxObjectCount_ = 256u; // supported max object count

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    // Divide by four since four targets in one struct.
    constexpr const size_t sizeOfBuffer = (CORE_MAX_MORPH_TARGET_COUNT * sizeof(MorphTargetInfoStruct)) / 4u;
    objectUniformBufferHandle_ = gpuResourceMgr.Create(GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, (uint32_t)sizeOfBuffer });

    auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    auto& descriptorSetMgr = renderNodeContextMgr.GetDescriptorSetManager();
    const DescriptorCounts dc { {
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u },                         // weight/indexset for all submeshes.
        { CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, (2u + 3u) * maxObjectCount_ } // number of inputs and outputs
    } };
    descriptorSetMgr.ResetAndReserve(dc);

    {
        const RenderHandle shaderHandle = shaderMgr.GetShaderHandle("3dshaders://computeshader/core3d_dm_morph.shader");
        threadGroupSize_ = shaderMgr.GetReflectionThreadGroupSize(shaderHandle);
        pipelineLayout_ = shaderMgr.GetReflectionPipelineLayout(shaderHandle);
        pipelineLayout_.descriptorSetLayouts[SET_UBO].bindings[0].descriptorType = CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        {
            const RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(SET_UBO, pipelineLayout_);
            allDescriptorSets_.params = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, pipelineLayout_.descriptorSetLayouts[SET_UBO].bindings);
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

        auto& psoMgr = renderNodeContextMgr.GetPsoManager();
        psoHandle_ = psoMgr.GetComputePsoHandle(shaderHandle, pipelineLayout_, {});
    }
}

void RenderNodeMorph::PreExecuteFrame()
{
    // re-create needed gpu resources
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
        reinterpret_cast<MorphTargetInfoStruct*>(gpuResourceMgr.MapBuffer(objectUniformBufferHandle_.GetHandle()));
    if (morphData) {
        uint32_t offset = 0;
        for (const RenderDataMorph::Submesh& submesh : submeshes) {
            const float* weights = submesh.morphTargetWeight;
            const uint16_t* ids = submesh.morphTargetId;
            // Assert that the maximum active morph target count is not reached.
            CORE_ASSERT((offset + submesh.activeTargetCount) < CORE_MAX_MORPH_TARGET_COUNT);
            for (size_t i = 0; i < submesh.activeTargetCount; i++) {
                morphData[offset + i / 4u].target[i % 4u] = ids[i];
                morphData[offset + i / 4u].weight[i % 4u] = weights[i];
            }
            offset += (submesh.activeTargetCount + 3u) / 4u;
        }
        // Could there be some way to mark the modified range of buffer dirty?
        gpuResourceMgr.UnmapBuffer(objectUniformBufferHandle_.GetHandle());
    }
}

void RenderNodeMorph::ComputeMorphs(IRenderCommandList& cmdList, array_view<const RenderDataMorph::Submesh> submeshes)
{
    cmdList.BindPipeline(psoHandle_);
    // set 0
    {
        auto& binder = *allDescriptorSets_.params;
        binder.BindBuffer(0u, objectUniformBufferHandle_.GetHandle(), 0u);
        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());

        cmdList.BindDescriptorSet(SET_UBO, binder.GetDescriptorSetHandle());
    }
    uint32_t offset = 0;
    uint32_t outputIdx = 0u;
    uint32_t inputIdx = 0u;
    for (const RenderDataMorph::Submesh& submesh : submeshes) {
        // Bind inputs = target position (set 1)
        auto& inputBinder = *allDescriptorSets_.inputs[inputIdx++];

        const auto indexOffset = submesh.morphTargetBuffer.bufferOffset;
        const auto indexSize =
            (((submesh.vertexCount * submesh.morphTargetCount * static_cast<uint32_t>(sizeof(uint32_t))) + 0x100) /
                0x100) *
            0x100;

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

        const MorphObjectPushConstantStruct pushData { offset, submesh.vertexCount, submesh.morphTargetCount,
            submesh.activeTargetCount };
        cmdList.PushConstant(pipelineLayout_.pushConstant, reinterpret_cast<const uint8_t*>(&pushData));
        cmdList.Dispatch((submesh.vertexCount + threadGroupSize_.x - 1) / threadGroupSize_.x, 1, 1);

        offset += (submesh.activeTargetCount + 3u) / 4u;
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
