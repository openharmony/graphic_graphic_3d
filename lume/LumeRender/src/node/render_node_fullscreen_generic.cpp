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

#include "render_node_fullscreen_generic.h"

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

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
void RenderNodeFullscreenGeneric::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    pipelineData_ = {};
    ParseRenderNodeInputs();

    useDataStoreShaderSpecialization_ = !jsonInputs_.renderDataStoreSpecialization.dataStoreName.empty();

    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    if (RenderHandleUtil::GetHandleType(pipelineData_.shader) != RenderHandleType::SHADER_STATE_OBJECT) {
        PLUGIN_LOG_E("RenderNodeFullscreenGeneric needs a valid shader handle");
    }

    if (useDataStoreShaderSpecialization_) {
        const ShaderSpecializationConstantView sscv = shaderMgr.GetReflectionSpecialization(pipelineData_.shader);
        shaderSpecializationData_.constants.resize(sscv.constants.size());
        shaderSpecializationData_.data.resize(sscv.constants.size());
        for (size_t idx = 0; idx < shaderSpecializationData_.constants.size(); ++idx) {
            shaderSpecializationData_.constants[idx] = sscv.constants[idx];
            shaderSpecializationData_.data[idx] = ~0u;
        }
        useDataStoreShaderSpecialization_ = !sscv.constants.empty();
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    pipelineData_.pipelineLayout = shaderMgr.GetPipelineLayoutHandleByShaderHandle(pipelineData_.shader);
    if (!RenderHandleUtil::IsValid(pipelineData_.pipelineLayout)) {
        pipelineData_.pipelineLayout = shaderMgr.GetReflectionPipelineLayoutHandle(pipelineData_.shader);
    }
    pipelineData_.graphicsState = shaderMgr.GetGraphicsStateHandleByShaderHandle(pipelineData_.shader);
    pipelineData_.pipelineLayoutData = shaderMgr.GetPipelineLayout(pipelineData_.pipelineLayout);
    pipelineData_.pso = GetPsoHandle();

    {
        const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineData_.pipelineLayoutData);
        renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
    }

    pipelineDescriptorSetBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(pipelineData_.pipelineLayoutData);
    renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);

    useDataStorePushConstant_ = (pipelineData_.pipelineLayoutData.pushConstant.byteSize > 0) &&
                                (!jsonInputs_.renderDataStore.dataStoreName.empty()) &&
                                (!jsonInputs_.renderDataStore.configurationName.empty());
}

void RenderNodeFullscreenGeneric::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeFullscreenGeneric::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (!RENDER_NS::RenderHandleUtil::IsValid(pipelineData_.pso)) {
        return;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    }
    if (jsonInputs_.hasChangeableResourceHandles) {
        inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);
        renderNodeUtil.BindResourcesToBinder(inputResources_, *pipelineDescriptorSetBinder_);
    }

    const RenderPass renderPass = renderNodeUtil.CreateRenderPass(inputRenderPass_);
    const ViewportDesc viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass);
    const ScissorDesc scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass);

    const auto setIndices = pipelineDescriptorSetBinder_->GetSetIndices();
    const uint32_t firstSetIndex = pipelineDescriptorSetBinder_->GetFirstSet();
    for (auto refIndex : setIndices) {
        const auto descHandle = pipelineDescriptorSetBinder_->GetDescriptorSetHandle(refIndex);
        const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(refIndex);
        cmdList.UpdateDescriptorSet(descHandle, bindings);
    }
#if (RENDER_VALIDATION_ENABLED == 1)
    if (!pipelineDescriptorSetBinder_->GetPipelineDescriptorSetLayoutBindingValidity()) {
        PLUGIN_LOG_ONCE_E(renderNodeContextMgr_->GetName() + "_RCL_UpdateDescriptorSet_invalid_",
            "RENDER_VALIDATION: RenderNodeFullscreenGeneric: bindings missing (RN: %s)",
            renderNodeContextMgr_->GetName().data());
    }
#endif

    cmdList.BeginRenderPass(renderPass.renderPassDesc, renderPass.subpassStartIndex, renderPass.subpassDesc);

    const RenderHandle psoHandle = GetPsoHandle();
    cmdList.BindPipeline(psoHandle);

    // bind all sets from the first on-wards
    cmdList.BindDescriptorSets(firstSetIndex, pipelineDescriptorSetBinder_->GetDescriptorSetHandles());

    // dynamic state
    cmdList.SetDynamicStateViewport(viewportDesc);
    cmdList.SetDynamicStateScissor(scissorDesc);
    if (renderPass.subpassDesc.fragmentShadingRateAttachmentCount > 0) {
        cmdList.SetDynamicStateFragmentShadingRate(
            { 1u, 1u }, FragmentShadingRateCombinerOps { CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE,
                            CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE });
    }

    // push constants
    if (useDataStorePushConstant_) {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto dataStore = static_cast<IRenderDataStorePod const*>(
            renderDataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStore.dataStoreName));
        if (dataStore) {
            const auto dataView = dataStore->Get(jsonInputs_.renderDataStore.configurationName);
            if (!dataView.empty()) {
                cmdList.PushConstant(pipelineData_.pipelineLayoutData.pushConstant, dataView.data());
            }
        }
    }

    cmdList.Draw(3u, 1u, 0u, 0u); // vertex count, instance count, first vertex, first instance
    cmdList.EndRenderPass();
}

RenderHandle RenderNodeFullscreenGeneric::GetPsoHandle()
{
    // controlled by count
    constexpr DynamicStateEnum dynamicStates[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR,
        CORE_DYNAMIC_STATE_ENUM_FRAGMENT_SHADING_RATE };
    if (useDataStoreShaderSpecialization_) {
        const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
        const auto dataStore = static_cast<IRenderDataStorePod const*>(
            renderDataStoreMgr.GetRenderDataStore(jsonInputs_.renderDataStoreSpecialization.dataStoreName));
        if (dataStore) {
            const auto dataView = dataStore->Get(jsonInputs_.renderDataStoreSpecialization.configurationName);
            if (dataView.data() && (dataView.size_bytes() == sizeof(ShaderSpecializationRenderPod))) {
                const auto* spec = reinterpret_cast<const ShaderSpecializationRenderPod*>(dataView.data());
                bool valuesChanged = false;
                const uint32_t specializationCount = Math::min(
                    Math::min(spec->specializationConstantCount, (uint32_t)shaderSpecializationData_.constants.size()),
                    ShaderSpecializationRenderPod::MAX_SPECIALIZATION_CONSTANT_COUNT);
                for (uint32_t idx = 0; idx < specializationCount; ++idx) {
                    const auto& ref = shaderSpecializationData_.constants[idx];
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
                        { shaderSpecializationData_.constants.data(), specializationCount },
                        { shaderSpecializationData_.data.data(), specializationCount }
                    };
                    const uint32_t dynamicStateCount =
                        (inputRenderPass_.fragmentShadingRateAttachmentIndex != ~0u) ? 3u : 2u;
                    pipelineData_.pso = renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(
                        pipelineData_.shader, pipelineData_.graphicsState, pipelineData_.pipelineLayout, {},
                        specialization, { dynamicStates, dynamicStateCount });
                }
            } else {
#if (RENDER_VALIDATION_ENABLED == 1)
                const string logName = "RenderNodeFullscreenGeneric_ShaderSpecialization" +
                                       string(jsonInputs_.renderDataStoreSpecialization.configurationName);
                PLUGIN_LOG_ONCE_E(logName.c_str(),
                    "RENDER_VALIDATION: RenderNodeFullscreenGeneric shader specilization render data store size "
                    "mismatch, name: %s, size:%u, podsize%u",
                    jsonInputs_.renderDataStoreSpecialization.configurationName.c_str(),
                    static_cast<uint32_t>(sizeof(ShaderSpecializationRenderPod)),
                    static_cast<uint32_t>(dataView.size_bytes()));
#endif
            }
        }
    } else if (!RenderHandleUtil::IsValid(pipelineData_.pso)) {
        const uint32_t dynamicStateCount = (inputRenderPass_.fragmentShadingRateAttachmentIndex != ~0u) ? 3u : 2u;
        pipelineData_.pso = renderNodeContextMgr_->GetPsoManager().GetGraphicsPsoHandle(pipelineData_.shader,
            pipelineData_.graphicsState, pipelineData_.pipelineLayout, {}, {}, { dynamicStates, dynamicStateCount });
    }
    return pipelineData_.pso;
}

void RenderNodeFullscreenGeneric::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");
    jsonInputs_.renderDataStoreSpecialization =
        parserUtil.GetRenderDataStore(jsonVal, "renderDataStoreShaderSpecialization");

    const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    pipelineData_.shader = shaderMgr.GetShaderHandle(shaderName);

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    inputResources_ = renderNodeUtil.CreateInputResources(jsonInputs_.resources);

    jsonInputs_.hasChangeableRenderPassHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.renderPass);
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);
}

// for plugin / factory interface
IRenderNode* RenderNodeFullscreenGeneric::Create()
{
    return new RenderNodeFullscreenGeneric();
}

void RenderNodeFullscreenGeneric::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeFullscreenGeneric*>(instance);
}
RENDER_END_NAMESPACE()
