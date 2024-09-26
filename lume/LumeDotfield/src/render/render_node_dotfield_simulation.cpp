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

#include "render_node_dotfield_simulation.h"
#include <array>
#include <3d/implementation_uids.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <base/containers/array_view.h>
#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>
#include "render_data_store_default_dotfield.h"

namespace {
#include "app/shaders/common/dotfield_common.h"
#include "app/shaders/common/dotfield_struct_common.h"
} // namespace
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace Dotfield {
constexpr string_view DEFAULT_DOTFIELD_RENDER_DATA_STORE_NAME_POST_FIX = "RenderDataStoreDefaultDotfield";

namespace {
void CreateBinders(const IRenderNodeShaderManager& shaderMgr, INodeContextDescriptorSetManager& descriptorSetMgr,
    RenderNodeDotfieldSimulation::Binders& binders)
{
    {
        constexpr uint32_t prevBuffersSet1 = 0;
        constexpr uint32_t currBuffersset2 = 1;

        auto createDescriptorSet = [](INodeContextDescriptorSetManager& descriptorSetMgr, PipelineLayout& pl,
                                       uint32_t bufferSetIndex, IDescriptorSetBinder::Ptr& setBinder) {
            const RenderHandle setDescHandle = descriptorSetMgr.CreateDescriptorSet(bufferSetIndex, pl);
            setBinder = descriptorSetMgr.CreateDescriptorSetBinder(
                setDescHandle, pl.descriptorSetLayouts[bufferSetIndex].bindings);
        };

        auto createDescriptorSets = [&createDescriptorSet](INodeContextDescriptorSetManager& descriptorSetMgr,
                                        PipelineLayout& pl, uint32_t bufferSetIndex,
                                        vector<IDescriptorSetBinder::Ptr>& setBinders) {
            for (auto& binder : setBinders) {
                createDescriptorSet(descriptorSetMgr, pl, bufferSetIndex, binder);
            }
        };
        { // simulate
            auto& currBinders = binders.simulate;
            currBinders.prevBuffersSet1.resize(DOTFIELD_EFFECT_MAX_COUNT);
            currBinders.currBuffersSet2.resize(DOTFIELD_EFFECT_MAX_COUNT);
            currBinders.pl = shaderMgr.GetPipelineLayout(
                shaderMgr.GetPipelineLayoutHandle("dotfieldpipelinelayouts://dotfield_simulation.shaderpl"));
            PipelineLayout& pl = currBinders.pl;
            CORE_ASSERT(pl.pushConstant.byteSize == sizeof(DotfieldSimulationPushConstantStruct));

            createDescriptorSets(descriptorSetMgr, pl, prevBuffersSet1, currBinders.prevBuffersSet1);
            createDescriptorSets(descriptorSetMgr, pl, currBuffersset2, currBinders.currBuffersSet2);
        }
    }
}
} // namespace

void RenderNodeDotfieldSimulation::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    {
        // (sim(currFrame + prevFrame) * effectcount;
        constexpr uint32_t dataBufferCountPerFrame = (2u + 2u) * DOTFIELD_EFFECT_MAX_COUNT;
        const DescriptorCounts dc { {
            { CORE_DESCRIPTOR_TYPE_STORAGE_BUFFER, dataBufferCountPerFrame },
        } };
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
    }

    specializationToPsoData_.clear();

    shaderHandle_ = renderNodeContextMgr.GetShaderManager().GetShaderHandle(
        "dotfieldshaders://computeshader/dotfield_simulation.shader");
    CreateBinders(renderNodeContextMgr.GetShaderManager(), renderNodeContextMgr.GetDescriptorSetManager(), binders_);

    if (auto* renderNodeSceneUtil = CORE_NS::GetInstance<CORE3D_NS::IRenderNodeSceneUtil>(
            *(renderNodeContextMgr_->GetRenderContext().GetInterface<IClassRegister>()),
            CORE3D_NS::UID_RENDER_NODE_SCENE_UTIL);
        renderNodeSceneUtil) {
        const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
        stores_ = renderNodeSceneUtil->GetSceneRenderDataStores(
            renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
    }
}

void RenderNodeDotfieldSimulation::PreExecuteFrame() {}

void RenderNodeDotfieldSimulation::ExecuteFrame(IRenderCommandList& cmdList)
{
    ComputeSimulate(*renderNodeContextMgr_, cmdList);
}

void RenderNodeDotfieldSimulation::ComputeSimulate(
    IRenderNodeContextManager& renderNodeContextMgr, IRenderCommandList& cmdList)
{
    const RenderDataStoreDefaultDotfield* dataStore = static_cast<RenderDataStoreDefaultDotfield*>(
        renderNodeContextMgr.GetRenderDataStoreManager().GetRenderDataStore(
            stores_.dataStoreNamePrefix + DEFAULT_DOTFIELD_RENDER_DATA_STORE_NAME_POST_FIX.data()));
    if (!dataStore) {
        return;
    }

    const auto& dotfieldPrimitives = dataStore->GetDotfieldPrimitives();
    if (dotfieldPrimitives.empty()) {
        return;
    }
    const auto& bufferData = dataStore->GetBufferData();

    const uint32_t currFrameIndex = bufferData.currFrameIndex;
    const uint32_t prevFrameIndex = 1 - currFrameIndex;
    const uint32_t primitiveCount = static_cast<uint32_t>(dotfieldPrimitives.size());

    const auto& currBinders = binders_.simulate;

    RenderHandle pso;
    for (uint32_t idx = 0; idx < primitiveCount; ++idx) {
        const auto& dotfieldPrimitive = dotfieldPrimitives[idx];
        const RenderDataDefaultDotfield::BufferData::Buffer& effect = bufferData.buffers[idx];

        PsoData psoData = GetPsoData(renderNodeContextMgr, currBinders.pl, shaderHandle_, firstFrame_);
        if (pso != psoData.psoHandle) {
            pso = psoData.psoHandle;
            cmdList.BindPipeline(psoData.psoHandle);
        }

        {
            auto& prevBinder = *currBinders.prevBuffersSet1[idx];
            prevBinder.BindBuffer(0, effect.dataBuffer[prevFrameIndex].GetHandle(), 0);
            cmdList.UpdateDescriptorSet(
                prevBinder.GetDescriptorSetHandle(), prevBinder.GetDescriptorSetLayoutBindingResources());

            auto& currBinder = *currBinders.currBuffersSet2[idx];
            currBinder.BindBuffer(0, effect.dataBuffer[currFrameIndex].GetHandle(), 0);
            cmdList.UpdateDescriptorSet(
                currBinder.GetDescriptorSetHandle(), currBinder.GetDescriptorSetLayoutBindingResources());

            const RenderHandle descHandles[2] = { prevBinder.GetDescriptorSetHandle(),
                currBinder.GetDescriptorSetHandle() };
            cmdList.BindDescriptorSets(0, { descHandles, 2u });
        }
        const DotfieldSimulationPushConstantStruct pc = { { dotfieldPrimitive.size.x, dotfieldPrimitive.size.y, 0, 0 },
            { dotfieldPrimitive.touch.x, dotfieldPrimitive.touch.y, dotfieldPrimitive.touchRadius, 0.f },
            { dotfieldPrimitive.touchDirection.x, dotfieldPrimitive.touchDirection.y,
                dotfieldPrimitive.touchDirection.z, 0.f } };
        cmdList.PushConstant(psoData.pushConstant, reinterpret_cast<const uint8_t*>(&pc));

        const uint32_t tgcX = (dotfieldPrimitive.size.x * dotfieldPrimitive.size.y + (DOTFIELD_SIMULATION_TGS - 1)) /
                              DOTFIELD_SIMULATION_TGS;
        cmdList.Dispatch(tgcX, 1, 1);
        {
            constexpr GeneralBarrier src { AccessFlagBits::CORE_ACCESS_SHADER_WRITE_BIT,
                PipelineStageFlagBits::CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
            constexpr GeneralBarrier dst { AccessFlagBits::CORE_ACCESS_INDIRECT_COMMAND_READ_BIT,
                PipelineStageFlagBits::CORE_PIPELINE_STAGE_DRAW_INDIRECT_BIT };

            cmdList.CustomMemoryBarrier(src, dst);
            cmdList.AddCustomBarrierPoint();
        }
    }
    firstFrame_ = false;
}

RenderNodeDotfieldSimulation::PsoData RenderNodeDotfieldSimulation::GetPsoData(
    IRenderNodeContextManager& renderNodeContextMgr, const PipelineLayout& pl, const RenderHandle& shaderHandle,
    bool firstFrame)
{
    uint32_t constantData = firstFrame ? 1u : 0u;

    if (const auto iter = specializationToPsoData_.find(constantData); iter != specializationToPsoData_.cend()) {
        return iter->second;
    } else { // new
        const ShaderSpecializationConstantView sscv =
            renderNodeContextMgr.GetShaderManager().GetReflectionSpecialization(shaderHandle);
        const ShaderSpecializationConstantDataView specDataView {
            sscv.constants,
            { &constantData, 1 },
        };
        RenderHandle psoHandle =
            renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(shaderHandle, pl, specDataView);
        PsoData psoData = { psoHandle, pl.pushConstant };
        specializationToPsoData_[constantData] = psoData;
        return psoData;
    }
}

IRenderNode* RenderNodeDotfieldSimulation::Create()
{
    return new RenderNodeDotfieldSimulation();
}

void RenderNodeDotfieldSimulation::Destroy(IRenderNode* instance)
{
    delete (RenderNodeDotfieldSimulation*)instance;
}
} // namespace Dotfield
