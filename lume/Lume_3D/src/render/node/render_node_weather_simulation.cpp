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
    if (!shaderMgr.IsValid(shader_) || !shaderMgr.IsValid(initShader_)) {
        CORE_LOG_E("RenderNodeRippleSimulation needs a valid shader handle");
    }

    // simulate shader
    {
        pipelineLayout_ = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(shader_);
        psoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(shader_, pipelineLayout_, {});

        {
            const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(pipelineLayout_);
            renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
        }

        // create pipeline descriptor set
        pipelineDescriptorSetBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(pipelineLayout_);
    }

    // initialization shader
    {
        initPipelineLayout_ = renderNodeContextMgr.GetRenderNodeUtil().CreatePipelineLayout(initShader_);
        initPsoHandle_ = renderNodeContextMgr.GetPsoManager().GetComputePsoHandle(initShader_, initPipelineLayout_, {});

        {
            const DescriptorCounts dc = renderNodeUtil.GetDescriptorCounts(initPipelineLayout_);
            renderNodeContextMgr.GetDescriptorSetManager().ResetAndReserve(dc);
        }

        // create pipeline descriptor set
        initPipelineDescriptorSetBinder_ = renderNodeUtil.CreatePipelineDescriptorSetBinder(initPipelineLayout_);
    }

    defaultMaterialSam_ = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    rippleTextureHandle_ = gpuResourceMgr.GetImageHandle("RIPPLE_RENDER_NODE_TEXTURE_0");
    rippleTextureHandle1_ = gpuResourceMgr.GetImageHandle("RIPPLE_RENDER_NODE_TEXTURE_1");
    rippleInputArgsBuffer_ = gpuResourceMgr.GetBufferHandle("RIPPLE_RENDER_NODE_INPUTBUFFER");
}

void RenderNodeWeatherSimulation ::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeWeatherSimulation::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
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

    if (!areTextureInit) {
        InitializeRippleBuffers(cmdList);
        areTextureInit = true;
        return;
    }

    const auto& effectData = dataStoreWeather_->GetWaterEffectData();
    if (effectData.empty()) {
        return;
    }

    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(rippleTextureHandle_);

    const auto scene = dataStoreScene->GetScene();
    {
        auto& pipelineDescriptorSetBinder = *pipelineDescriptorSetBinder_;
        pipelineDescriptorSetBinder.ClearBindings();

        // bind the input args buffer
        {
            BindableBuffer bindable;
            bindable.handle = rippleInputArgsBuffer_;
            bindable.byteOffset = 0;
            bindable.byteSize = sizeof(DefaultWaterRippleDataStruct);
            pipelineDescriptorSetBinder.BindBuffer(0, 0, bindable);
        }

        // bind the ripple storage src texture
        {
            BindableImage bindable;
            if (pingPongIdx == 0) {
                bindable.handle = rippleTextureHandle_;
            } else {
                bindable.handle = rippleTextureHandle1_;
            }
            pipelineDescriptorSetBinder.BindImage(0, 1, bindable);
        }

        // bind the ripple storage dst texture
        {
            BindableImage bindable;
            if (pingPongIdx == 1) {
                bindable.handle = rippleTextureHandle_;
            } else {
                bindable.handle = rippleTextureHandle1_;
            }
            pipelineDescriptorSetBinder.BindImage(0, 2, bindable);
        }

        const auto descHandle = pipelineDescriptorSetBinder_->GetDescriptorSetHandle(0);
        const auto bindings = pipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(0);
        cmdList.UpdateDescriptorSet(descHandle, bindings);
    }

    cmdList.BindPipeline(psoHandle_);

    // bind all sets
    {
        const auto descHandles = pipelineDescriptorSetBinder_->GetDescriptorSetHandles();
        cmdList.BindDescriptorSets(0, descHandles);
    }

    BASE_NS::Math::Vec4 pc { scene.deltaTime / 1000.0f, 0, 0, 0 };

    // NOTE: fix the hard-coded
    // Add plane offcet to pc
    pc.z = effectData[0U].planeOffset.x;
    pc.w = effectData[0U].planeOffset.y;

    cmdList.PushConstant(pipelineLayout_.pushConstant, arrayviewU8(pc).data());

    // dispatch compute shader
    if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
        const Math::UVec3 targetSize = { TextureImageDesc.width, TextureImageDesc.height, 1 };

        const uint32_t tgcX = (targetSize.x + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;
        const uint32_t tgcY = (targetSize.y + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;

        cmdList.Dispatch(tgcX, tgcY, 1);
    } else {
        CORE_LOG_W("RenderNodeRippleSimulation: dispatchResources needed");
    }

    {
        // add barrier for memory
        constexpr GeneralBarrier src { AccessFlagBits::CORE_ACCESS_SHADER_WRITE_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT };
        constexpr GeneralBarrier dst { AccessFlagBits::CORE_ACCESS_INDIRECT_COMMAND_READ_BIT |
                                           AccessFlagBits::CORE_ACCESS_SHADER_WRITE_BIT,
            PipelineStageFlagBits::CORE_PIPELINE_STAGE_DRAW_INDIRECT_BIT |
                PipelineStageFlagBits::CORE_PIPELINE_STAGE_COMPUTE_SHADER_BIT };

        cmdList.CustomMemoryBarrier(src, dst);
        cmdList.AddCustomBarrierPoint();
    }

    // copy from 1 to 0
    if (pingPongIdx == 0U) {
        ImageCopy imageCopy;
        imageCopy.srcSubresource = { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u };
        imageCopy.srcOffset = { 0, 0, 0 };
        imageCopy.dstSubresource = { RENDER_NS::ImageAspectFlagBits::CORE_IMAGE_ASPECT_COLOR_BIT, 0, 0, 1u };
        imageCopy.dstOffset = { 0, 0, 0 };
        imageCopy.extent = { TextureImageDesc.width, TextureImageDesc.height, 1u };

        cmdList.CopyImageToImage(rippleTextureHandle1_, rippleTextureHandle_, imageCopy);
    }

    pingPongIdx = (pingPongIdx == 0U) ? 1U : 0U;
}

void RenderNodeWeatherSimulation::InitializeRippleBuffers(RENDER_NS::IRenderCommandList& cmdList)
{
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto TextureImageDesc = gpuResourceMgr.GetImageDescriptor(rippleTextureHandle_);

    {
        auto& pipelineDescriptorSetBinder = *initPipelineDescriptorSetBinder_;
        pipelineDescriptorSetBinder.ClearBindings();

        // bind the input args buffer
        {
            BindableBuffer bindable;
            bindable.handle = rippleInputArgsBuffer_;
            bindable.byteOffset = 0;
            bindable.byteSize = sizeof(DefaultWaterRippleDataStruct);
            pipelineDescriptorSetBinder.BindBuffer(0, 0, bindable);
        }

        // bind the ripple storage src texture
        {
            BindableImage bindable;
            bindable.handle = rippleTextureHandle_;
            bindable.samplerHandle = rippleTextureHandle_;
            pipelineDescriptorSetBinder.BindImage(0, 1, bindable);
        }

        // bind the ripple storage dst texture
        {
            BindableImage bindable;
            bindable.handle = rippleTextureHandle1_;
            bindable.samplerHandle = rippleTextureHandle1_;
            pipelineDescriptorSetBinder.BindImage(0, 2, bindable);
        }

        const auto descHandle = initPipelineDescriptorSetBinder_->GetDescriptorSetHandle(0);
        const auto bindings = initPipelineDescriptorSetBinder_->GetDescriptorSetLayoutBindingResources(0);
        cmdList.UpdateDescriptorSet(descHandle, bindings);
    }

    cmdList.BindPipeline(initPsoHandle_);

    // bind all sets
    {
        const auto descHandles = initPipelineDescriptorSetBinder_->GetDescriptorSetHandles();
        cmdList.BindDescriptorSets(0, descHandles);
    }

    // dispatch compute shader
    if (TextureImageDesc.width > 1u && TextureImageDesc.height > 1u) {
        const Math::UVec3 targetSize = { TextureImageDesc.width, TextureImageDesc.height, 1 };

        const uint32_t tgcX = (targetSize.x + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;
        const uint32_t tgcY = (targetSize.y + WATER_RIPPLE_TGS - 1u) / WATER_RIPPLE_TGS;

        cmdList.Dispatch(tgcX, tgcY, 1);
    } else {
        CORE_LOG_W("RenderNodeRippleSimulation: dispatchResources needed");
    }

    {
        // add barrier for memory
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
        const auto shaderName = parserUtil.GetStringValue(jsonVal, "shader");
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
