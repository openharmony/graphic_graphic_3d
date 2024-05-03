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

#include "render_node_compute_generic.h"

#include <base/math/mathf.h>
#include <base/math/vector.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {
struct DispatchResources {
    RenderHandle buffer {};
    RenderHandle image {};
};

DispatchResources GetDispatchResources(const RenderNodeHandles::InputResources& ir)
{
    DispatchResources dr;
    if (!ir.customInputBuffers.empty()) {
        dr.buffer = ir.customInputBuffers[0].handle;
    }
    if (!ir.customInputImages.empty()) {
        dr.image = ir.customInputImages[0].handle;
    }
    return dr;
}
} // namespace

void RenderNodeComputeGeneric::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    useDataStoreShaderSpecialization_ = !jsonInputs_.renderDataStoreSpecialization.dataStoreName.empty();

    auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    const auto& renderNodeUtil = renderNodeContextMgr.GetRenderNodeUtil();
    if (RenderHandleUtil::GetHandleType(shader_) != RenderHandleType::COMPUTE_SHADER_STATE_OBJECT) {
        PLUGIN_LOG_E("RenderNodeComputeGeneric needs a valid compute shader handle");
    }
    pipelineLayout_ = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(shader_);
    threadGroupSize_ = shaderMgr.GetReflectionThreadGroupSize(shader_);

    if (dispatchResources_.customInputBuffers.empty() && dispatchResources_.customInputImages.empty()) {
        PLUGIN_LOG_W("RenderNodeComputeGeneric: dispatchResources (GPU buffer or GPU image) needed");
    }

    if (useDataStoreShaderSpecialization_) {
        const ShaderSpecializationConstantView sscv = shaderMgr.GetReflectionSpecialization(shader_);
        shaderSpecializationData_.constants.resize(sscv.constants.size());
        shaderSpecializationData_.data.resize(sscv.constants.size());
        for (size_t idx = 0; idx < shaderSpecializationData_.constants.size(); ++idx) {
            shaderSpecializationData_.constants[idx] = sscv.constants[idx];
            shaderSpecializationData_.data[idx] = ~0u;
        }
        useDataStoreShaderSpecialization_ = !sscv.constants.empty();
    }
    psoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(shader_, pipelineLayout_, {});

    {
        const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
    }

    pipelineDescriptorSetBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(pipelineLayout_);
    renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);

    useDataStorePushConstant_ = (pipelineLayout_.pushConstant.byteSize > 0) &&
                                (!jsonInputs_.renderDataStore.dataStoreName.empty()) &&
                                (!jsonInputs_.renderDataStore.configurationName.empty());
}

void RenderNodeComputeGeneric::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeComputeGeneric::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!RenderHandleUtil::IsValid(shader_)) {
        return; // invalid shader
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
        renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
    }
    if (jsonInputs_.hasChangeableDispatchHandles) {
        dispatchResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.dispatchResources);
    }
    const DispatchResources dr = GetDispatchResources(dispatchResources_);
    if ((!RenderHandleUtil::IsValid(dr.buffer)) && (!RenderHandleUtil::IsValid(dr.image))) {
#if (RENDER_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "_no_dr",
            "RENDER_VALIDATION: RN: %s, no valid dispatch resource", renderNodeContextMgr_->GetName().data());
#endif
        return; // no way to evaluate dispatch size
    }
    const uint32_t firstSetIndex = pipelineDescriptorSetBinder_->GetFirstSet();
    {
        const auto setIndices = pipelineDescriptorSetBinder_->GetSetIndices();
        for (auto refIndex : setIndices) {
            const auto descHandle = pipelineDescriptorSetBinder_->GetDescriptorSetHandle(refIndex);
            const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(refIndex);
            cmdList.UpdateDescriptorSet(descHandle, bindings);
        }
#if (RENDER_VALIDATION_ENABLED == 1)
        if (!pipelineDescriptorSetBinder_->GetPipelineDescriptorSetLayoutBindingValidity()) {
            PLUGIN_LOG_ONCE_E(renderNodeContextMgr_->GetName() + "_bindings_missing",
                "RENDER_VALIDATION: RenderNodeComputeGeneric: bindings missing (RN: %s)",
                renderNodeContextMgr_->GetName().data());
        }
#endif
    }

    const RenderHandle psoHandle = GetPsoHandle(*renderNodeContextMgr_);
    cmdList.BindPipeline(psoHandle);

    // bind all sets
    {
        const auto descHandles = pipelineDescriptorSetBinder_->GetDescriptorSetHandles();
        cmdList.BindDescriptorSets(firstSetIndex, descHandles);
    }

    // push constants
    if (useDataStorePushConstant_) {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto dataStore = static_cast<IRenderDataStorePod const*>(
            renderDataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName.c_str()));
        if (dataStore) {
            const auto dataView = dataStore->Get(jsonInputs_.renderDataStore.configurationName);
            if (!dataView.empty()) {
                cmdList.PushConstant(pipelineLayout_.pushConstant, dataView.data());
            }
        }
    }

    if (RenderHandleUtil::IsValid(dr.buffer)) {
        cmdList.DispatchIndirect(dr.buffer, 0);
    } else if (RenderHandleUtil::IsValid(dr.image)) {
        const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const GpuImageDesc desc = gpuResourceMgr.GetImageDescriptor(dr.image);
        const Math::UVec3 targetSize = { desc.width, desc.height, desc.depth };
        cmdList.Dispatch((targetSize.x + threadGroupSize_.x - 1u) / threadGroupSize_.x,
            (targetSize.y + threadGroupSize_.y - 1u) / threadGroupSize_.y,
            (targetSize.z + threadGroupSize_.z - 1u) / threadGroupSize_.z);
    }
}

RenderHandle RenderNodeComputeGeneric::GetPsoHandle(IRenderNodeContextManager& renderNodeContextMgr)
{
    if (useDataStoreShaderSpecialization_) {
        const auto& renderDataStoreMgr = renderNodeContextMgr.GetRenderDataStoreManager();
        const auto dataStore = static_cast<IRenderDataStorePod const*>(
            renderDataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStoreSpecialization.dataStoreName.c_str()));
        if (dataStore) {
            const auto dataView = dataStore->Get(jsonInputs_.renderDataStoreSpecialization.configurationName);
            if (dataView.data() && (dataView.size_bytes() == sizeof(ShaderSpecializationRenderPod))) {
                const auto* spec = reinterpret_cast<const ShaderSpecializationRenderPod*>(dataView.data());
                bool valuesChanged = false;
                const auto specializationCount = Math::min(
                    ShaderSpecializationRenderPod::MAX_SPECIALIZATION_CONSTANT_COUNT,
                    Math::min((uint32_t)shaderSpecializationData_.constants.size(), spec->specializationConstantCount));
                const auto constantsView = array_view(shaderSpecializationData_.constants.data(), specializationCount);
                for (const auto& ref : constantsView) {
                    const uint32_t constantId = ref.offset / sizeof(uint32_t);
                    const uint32_t specId = ref.id;
                    if (specId < ShaderSpecializationRenderPod::MAX_SPECIALIZATION_CONSTANT_COUNT) {
                        if (shaderSpecializationData_.data[constantId] != spec->specializationFlags[specId].value) {
                            shaderSpecializationData_.data[constantId] = spec->specializationFlags[specId].value;
                            valuesChanged = true;
                        }
                    }
                }
                if (valuesChanged) {
                    const ShaderSpecializationConstantDataView specialization {
                        constantsView,
                        { shaderSpecializationData_.data.data(), specializationCount },
                    };
                    psoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(
                        shader_, pipelineLayout_, specialization);
                }
            } else {
#if (RENDER_VALIDATION_ENABLED == 1)
                const string logName = "RenderNodeComputeGeneric_ShaderSpecialization" +
                                       string(jsonInputs_.renderDataStoreSpecialization.configurationName);
                PLUGIN_LOG_ONCE_E(logName.c_str(),
                    "RENDER_VALIDATION: RenderNodeComputeGeneric shader specilization render data store size mismatch, "
                    "name: %s, size:%u, podsize%u",
                    jsonInputs_.renderDataStoreSpecialization.configurationName.c_str(),
                    static_cast<uint32_t>(sizeof(ShaderSpecializationRenderPod)),
                    static_cast<uint32_t>(dataView.size_bytes()));
#endif
            }
        }
    }
    return psoHandle_;
}

void RenderNodeComputeGeneric::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.dispatchResources = parserUtil.GetInputResources(jsonVal, "dispatchResources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");
    jsonInputs_.renderDataStoreSpecialization =
        parserUtil.GetRenderDataStore(jsonVal, "renderDataStoreShaderSpecialization");

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    shader_ = shaderMgr.GetShaderHandle(shaderName);

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
    dispatchResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.dispatchResources);
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);
    jsonInputs_.hasChangeableDispatchHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.dispatchResources);
}

// for plugin / factory interface
IRenderNode* RenderNodeComputeGeneric::Create()
{
    return new RenderNodeComputeGeneric();
}

void RenderNodeComputeGeneric::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeComputeGeneric*>(instance);
}
RENDER_END_NAMESPACE()
