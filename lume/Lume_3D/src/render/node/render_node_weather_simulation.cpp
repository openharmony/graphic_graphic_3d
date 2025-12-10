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

#include "render_node_weather_simulation.h"

#include <cinttypes>

#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/intf_render_node_scene_util.h>
#include <base/math/mathf.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store.h>
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
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "3d/shaders/common/water_ripple_common.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

void RenderNodeWeatherSimulation ::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    ParseRenderNodeInputs();

    auto& shaderMgr = renderNodeContextMgr.GetShaderManager();
    const auto& renderNodeUtil = renderNodeContextMgr.GetRenderNodeUtil();
    auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

    if (!shaderMgr.IsValid(shader_) || !shaderMgr.IsValid(initShader_)) {
        CORE_LOG_E("RenderNodeRippleSimulation needs a valid shader handle");
    }

    const uint32_t set = 0U;
    initPipelineLayout_ = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(initShader_);
    pipelineLayout_ = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(shader_);

    DescriptorCounts updateDescriptorSet = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
    DescriptorCounts initDescriptorSet = renderNodeUtil.GetDescriptorCounts(initPipelineLayout_);

    vector<DescriptorCounts> descriptorCounts;
    descriptorCounts.reserve(MAX_WATER_PLANES * 2U);
    for (uint32_t i = 0; i < MAX_WATER_PLANES; i++) {
        descriptorCounts.push_back(updateDescriptorSet);
        descriptorCounts.push_back(initDescriptorSet);
    }
    descriptorSetMgr.ResetAndReserve(descriptorCounts);

    // simulate shader
    {
        psoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(shader_, pipelineLayout_, {});
        // create pipeline descriptor set
        for (uint32_t i = 0; i < MAX_WATER_PLANES; i++) {
            RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, pipelineLayout_);
            updateDescriptorSetBinder_[i] = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, pipelineLayout_.descriptorSetLayouts[set].bindings);
        }
    }

    // initialization shader
    {
        initPsoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(initShader_, initPipelineLayout_, {});
        // create pipeline descriptor set
        for (uint32_t i = 0; i < MAX_WATER_PLANES; i++) {
            RenderHandle descriptorSetHandle = descriptorSetMgr.CreateDescriptorSet(set, initPipelineLayout_);

            initDescriptorSetBinder_[i] = descriptorSetMgr.CreateDescriptorSetBinder(
                descriptorSetHandle, initPipelineLayout_.descriptorSetLayouts[set].bindings);
        }
    }

    defaultMaterialSam_ = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
}

void RenderNodeWeatherSimulation ::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeWeatherSimulation::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();

    dataStoreWeather_ =
        static_cast<RenderDataStoreWeather*>(renderDataStoreMgr.GetRenderDataStore("RenderDataStoreWeather"));
    const IRenderDataStoreDefaultScene* dataStoreScene = static_cast<CORE3D_NS::IRenderDataStoreDefaultScene*>(
        renderDataStoreMgr.GetRenderDataStore("RenderDataStoreDefaultScene"));
    if ((!dataStoreWeather_) || (!dataStoreScene)) {
        return;
    }

    if (!RenderHandleUtil::IsValid(shader_)) {
        return; // invalid shader
    }

    const auto& effectData = dataStoreWeather_->GetWaterEffectData();
    if (effectData.empty()) {
        return; // No water planes to simulate
    }

    const auto scene = dataStoreScene->GetScene();
    const float deltaTime = scene.deltaTime / 1000.0f;
    uint32_t planeIndex = 0u;

    // Loop through each water effect plane to process its simulation
    for (const auto& waterPlaneData : effectData) {
        // Retrieve the specific textures, args buffer, and ping-pong index for this plane
        const RenderHandle& current_rippleTexture0 = waterPlaneData.texture0;
        const RenderHandle& current_rippleTexture1 = waterPlaneData.texture1;
        const RenderHandle& current_rippleInputArgsBuffer = waterPlaneData.argsBuffer;
        const uint32_t current_pingPongIdx = waterPlaneData.curIdx;

        if (!RenderHandleUtil::IsValid(current_rippleTexture0) || !RenderHandleUtil::IsValid(current_rippleTexture1) ||
            !RenderHandleUtil::IsValid(current_rippleInputArgsBuffer)) {
            CORE_LOG_W("RenderNodeWeatherSimulation: Invalid render handles for water plane %" PRIu64
                       ". Skipping simulation for this plane.",
                waterPlaneData.id);
            continue;
        }
        if (!waterPlaneData.isInitialized) {
            InitializeRippleBuffers(cmdList, waterPlaneData, planeIndex);
        }
        cmdList.BindPipeline(psoHandle_);
        const auto textureImageDesc = gpuResourceMgr.GetImageDescriptor(current_rippleTexture0);
        if (textureImageDesc.width <= 1u || textureImageDesc.height <= 1u) {
            CORE_LOG_W("RenderNodeWeatherSimulation: Invalid texture dimensions (%ux%u) for water plane %" PRIu64
                       ". Skipping simulation.",
                textureImageDesc.width, textureImageDesc.height, waterPlaneData.id);
            continue;
        }
        // --- Bindings for THIS plane's simulation ---
        {
            auto& pipelineDescriptorSetBinder = updateDescriptorSetBinder_[planeIndex];
            pipelineDescriptorSetBinder->ClearBindings();

            {
                BindableBuffer bindable;
                bindable.handle = current_rippleInputArgsBuffer;
                bindable.byteOffset = 0;
                bindable.byteSize = sizeof(DefaultWaterRippleDataStruct);
                pipelineDescriptorSetBinder->BindBuffer(0, bindable);
            }

            {
                BindableImage bindable;
                bindable.handle = (current_pingPongIdx == 0) ? current_rippleTexture0 : current_rippleTexture1;
                pipelineDescriptorSetBinder->BindImage(1, bindable);
            }

            {
                BindableImage bindable;
                bindable.handle = (current_pingPongIdx == 0) ? current_rippleTexture1 : current_rippleTexture0;
                pipelineDescriptorSetBinder->BindImage(2, bindable);
            }

            const auto descHandle = updateDescriptorSetBinder_[planeIndex]->GetDescriptorSetHandle();
            const auto bindings = updateDescriptorSetBinder_[planeIndex]->GetDescriptorSetLayoutBindingResources();
            cmdList.UpdateDescriptorSet(descHandle, bindings);
        }
        {
            const auto descHandles = updateDescriptorSetBinder_[planeIndex]->GetDescriptorSetHandle();
            cmdList.BindDescriptorSet(0, descHandles);
        }

        // Push constants for THIS plane
        BASE_NS::Math::Vec4 pc { deltaTime, 0, waterPlaneData.planeOffset.x, waterPlaneData.planeOffset.y };
        cmdList.PushConstant(pipelineLayout_.pushConstant, arrayviewU8(pc).data());

        // Dispatch compute shader for THIS plane
        const uint32_t tgcX = (textureImageDesc.width + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;
        const uint32_t tgcY = (textureImageDesc.height + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;
        cmdList.Dispatch(tgcX, tgcY, 1);

        planeIndex++;
    }
}

void RenderNodeWeatherSimulation::InitializeRippleBuffers(RENDER_NS::IRenderCommandList& cmdList,
    const RenderDataStoreWeather::WaterEffectData& waterPlaneData, uint32_t slot)
{
    const RenderHandle& current_rippleTexture0 = waterPlaneData.texture0;
    const RenderHandle& current_rippleTexture1 = waterPlaneData.texture1;
    const RenderHandle& rippleInputArgsBuffer = waterPlaneData.argsBuffer;

    if (!RenderHandleUtil::IsValid(current_rippleTexture0) || !RenderHandleUtil::IsValid(current_rippleTexture1) ||
        !RenderHandleUtil::IsValid(rippleInputArgsBuffer)) {
        CORE_LOG_W("RenderNodeWeatherSimulation::InitializeSpecificPlaneRippleBuffers: Invalid render handles for "
                   "water plane %" PRIu64 ". Cannot initialize.",
            waterPlaneData.id);
        return;
    }

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(current_rippleTexture0);

    cmdList.BindPipeline(initPsoHandle_);

    {
        auto& pipelineDescriptorSetBinder = initDescriptorSetBinder_[slot];
        pipelineDescriptorSetBinder->ClearBindings();

        {
            BindableBuffer bindable;
            bindable.handle = rippleInputArgsBuffer;
            bindable.byteOffset = 0;
            bindable.byteSize = sizeof(DefaultWaterRippleDataStruct);
            pipelineDescriptorSetBinder->BindBuffer(0, bindable);
        }

        {
            BindableImage bindable;
            bindable.handle = current_rippleTexture0;
            pipelineDescriptorSetBinder->BindImage(1, bindable);
        }

        // Bind the second ripple texture as a storage image for initialization
        {
            BindableImage bindable;
            bindable.handle = current_rippleTexture1;
            pipelineDescriptorSetBinder->BindImage(2, bindable);
        }

        const auto descHandle = initDescriptorSetBinder_[slot]->GetDescriptorSetHandle();
        const auto bindings = initDescriptorSetBinder_[slot]->GetDescriptorSetLayoutBindingResources();
        cmdList.UpdateDescriptorSet(descHandle, bindings);

        {
            const auto descHandles = initDescriptorSetBinder_[slot]->GetDescriptorSetHandle();
            cmdList.BindDescriptorSet(0, descHandles);
        }

        // Dispatch compute shader to initialize textures for this plane
        if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
            const Math::UVec3 targetSize = { TextureImageDesc.width, TextureImageDesc.height, 1 };

            const uint32_t tgcX = (targetSize.x + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;
            const uint32_t tgcY = (targetSize.y + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;

            cmdList.Dispatch(tgcX, tgcY, 1);
        } else {
            CORE_LOG_W("RenderNodeRippleSimulation: dispatchResources needed");
        }

        // Add a barrier immediately after initializing this plane's textures.
        {
            constexpr GeneralBarrier src { AccessFlagBits::CORE_ACCESS_SHADER_WRITE_BIT,
                PipelineStageFlagBits::CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
            constexpr GeneralBarrier dst { AccessFlagBits::CORE_ACCESS_INDIRECT_COMMAND_READ_BIT |
                                               AccessFlagBits::CORE_ACCESS_SHADER_WRITE_BIT,
                PipelineStageFlagBits::CORE_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                    PipelineStageFlagBits::CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

            cmdList.CustomMemoryBarrier(src, dst);
            cmdList.AddCustomBarrierPoint();
        }
    }
}

void RenderNodeWeatherSimulation ::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.resources = parserUtil.GetInputResources(jsonVal, "resources");
    jsonInputs_.dispatchResources = parserUtil.GetInputResources(jsonVal, "dispatchResources");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");
    jsonInputs_.renderDataStoreSpecialization =
        parserUtil.GetRenderDataStore(jsonVal, "renderDataStoreShaderSpecialization");

    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    // simulate shader
    {
        const auto shaderName = "3dshaders://computeshader/water_ripple_simulation.shader";
        shader_ = shaderMgr.GetShaderHandle(shaderName);
    }

    // init shader
    {
        initShader_ = shaderMgr.GetShaderHandle("3dshaders://computeshader/water_ripple_initialize.shader");
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    jsonInputs_.hasChangeableResourceHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.resources);
    jsonInputs_.hasChangeableDispatchHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.dispatchResources);
}

// for plugin / factory interface
IRenderNode* RenderNodeWeatherSimulation ::Create()
{
    return new RenderNodeWeatherSimulation();
}

void RenderNodeWeatherSimulation::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeWeatherSimulation*>(instance);
}
CORE3D_END_NAMESPACE()
