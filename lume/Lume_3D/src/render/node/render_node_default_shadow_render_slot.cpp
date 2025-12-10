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

#include "render_node_default_shadow_render_slot.h"

#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/containers/vector.h>
#include <base/math/mathf.h>
#include <base/math/matrix_util.h>
#include <base/math/vector_util.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/resource_handle.h>

#include "render/default_constants.h"
#include "render/render_node_scene_util.h"

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };

static constexpr uint32_t UBO_OFFSET_ALIGNMENT { 256u };
static constexpr uint32_t MAX_SHADOW_ATLAS_WIDTH { 8192u };

inline uint64_t HashShaderAndSubmesh(
    const uint64_t shaderDataHash, const uint32_t renderHash, const GraphicsState::InputAssembly& ia)
{
    const uint64_t iaHash = uint32_t(ia.enablePrimitiveRestart) | (ia.primitiveTopology << 1U);
    uint64_t hash = (iaHash << 32) | (uint64_t)renderHash;
    HashCombine(hash, shaderDataHash);
    return hash;
}

GpuImageDesc GetDepthBufferDesc(const RenderNodeDefaultShadowRenderSlot::ShadowBuffers& shadowBuffers)
{
    constexpr ImageUsageFlags usage = ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                      ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    constexpr MemoryPropertyFlags memPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    return GpuImageDesc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
        Format::BASE_FORMAT_D16_UNORM, ImageTiling::CORE_IMAGE_TILING_OPTIMAL, usage, memPropertyFlags, 0,
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, shadowBuffers.width,
        shadowBuffers.height, 1, 1, 1, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };
}

GpuImageDesc GetColorBufferDesc(const IRenderNodeGpuResourceManager& gpuResourceMgr,
    const RenderNodeDefaultShadowRenderSlot::ShadowBuffers& shadowBuffers)
{
    const EngineImageCreationFlags engineImageCreateFlags =
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;

    const ImageUsageFlags usage =
        ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT;
    const MemoryPropertyFlags memPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    Format format = Format::BASE_FORMAT_R16G16_UNORM;
    const auto formatProperties = gpuResourceMgr.GetFormatProperties(format);
    if ((formatProperties.optimalTilingFeatures & CORE_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0) {
        format = Format::BASE_FORMAT_R16G16_SFLOAT;
    }
    return GpuImageDesc { ImageType::CORE_IMAGE_TYPE_2D, ImageViewType::CORE_IMAGE_VIEW_TYPE_2D, format,
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL, usage, memPropertyFlags, 0, engineImageCreateFlags, shadowBuffers.width,
        shadowBuffers.height, 1u, 1u, 1u, SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT, {} };
}

inline bool IsInverseWinding(const RenderSubmeshFlags submeshFlags)
{
    return ((submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT) > 0);
}

inline RenderHandleReference CreateGeneralDataUniformBuffer(IRenderNodeGpuResourceManager& gpuResourceMgr)
{
    return gpuResourceMgr.Create(GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
        UBO_OFFSET_ALIGNMENT * DefaultMaterialLightingConstants::MAX_SHADOW_COUNT });
}

template<typename RenderDataStoreType>
RenderDataStoreType* GetRenderDataStore(
    const IRenderNodeRenderDataStoreManager& renderDataStoreManager, const string_view name)
{
    return static_cast<RenderDataStoreType*>(renderDataStoreManager.GetRenderDataStore(name));
}

inline bool UpdateResourceCount(uint32_t& oldCount, const uint32_t newValue, const uint32_t overCommitDivisor)
{
    if (oldCount < newValue) {
        oldCount = newValue + (newValue / overCommitDivisor);
        return true;
    }
    return false;
}

struct FrameGlobalDescriptorSets {
    RenderHandle set1;
    RenderHandle set2Default;
    array_view<const RenderHandle> set2;
    bool valid = false;
};

FrameGlobalDescriptorSets GetFrameGlobalDescriptorSets(
    IRenderNodeContextManager* rncm, const SceneRenderDataStores& stores)
{
    FrameGlobalDescriptorSets fgds;
    if (rncm) {
        // re-fetch global descriptor sets every frame
        const INodeContextDescriptorSetManager& dsMgr = rncm->GetDescriptorSetManager();
        const string_view us = stores.dataStoreNameScene;
        fgds.set1 = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_SET1_GLOBAL_DESCRIPTOR_SET_NAME);
        fgds.set2 = dsMgr.GetGlobalDescriptorSets(
            us + DefaultMaterialMaterialConstants::MATERIAL_RESOURCES_GLOBAL_DESCRIPTOR_SET_NAME);
        fgds.set2Default = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_DEFAULT_RESOURCE_GLOBAL_DESCRIPTOR_SET_NAME);
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (fgds.set2.empty()) {
            CORE_LOG_ONCE_W("core3d_global_descriptor_set_render_slot_issues",
                "CORE3D_VALIDATION: Global descriptor set for default material env not found");
        }
#endif
        fgds.valid = RenderHandleUtil::IsValid(fgds.set1) && RenderHandleUtil::IsValid(fgds.set2Default);
        if (!fgds.valid) {
            CORE_LOG_ONCE_E("core3d_global_descriptor_set_shadow_all_issues",
                "Global descriptor set 1/2 for default material not found (RenderNodeDefaultCameraController needed)");
        }
    }
    return fgds;
}
} // namespace

void RenderNodeDefaultShadowRenderSlot::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    // get flags
    bindlessEnabled_ = false;
    IRenderContext& renderContext = renderNodeContextMgr_->GetRenderContext();
    if (auto* renderContextClassRegister = renderContext.GetInterface<CORE_NS::IClassRegister>()) {
        if (auto* graphicsContext =
                CORE_NS::GetInstance<IGraphicsContext>(*renderContextClassRegister, UID_GRAPHICS_CONTEXT)) {
            const IGraphicsContext::CreateInfo ci = graphicsContext->GetCreateInfo();
            if (ci.createFlags & IGraphicsContext::CreateInfo::ENABLE_BINDLESS_PIPELINES_BIT) {
                bindlessEnabled_ = true;
            }
        }
    }

    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    {
        shadowBuffers_.depthName =
            stores_.dataStoreNameScene + DefaultMaterialLightingConstants::SHADOW_DEPTH_BUFFER_NAME;
        shadowBuffers_.vsmColorName =
            stores_.dataStoreNameScene + DefaultMaterialLightingConstants::SHADOW_VSM_COLOR_BUFFER_NAME;
        shadowBuffers_.depthHandle =
            gpuResourceMgr.Create(shadowBuffers_.depthName, GetDepthBufferDesc(shadowBuffers_));
        shadowBuffers_.vsmColorHandle =
            gpuResourceMgr.Create(shadowBuffers_.vsmColorName, GetColorBufferDesc(gpuResourceMgr, shadowBuffers_));
    }

    // reset
    validShadowNode_ = false;
    currentScene_ = {};
    allShaderData_ = {};
    allDescriptorSets_ = {};

    CreateDefaultShaderData();

    uboHandles_.generalData = CreateGeneralDataUniformBuffer(gpuResourceMgr);
    sceneBuffers_ = RenderNodeSceneUtil::GetSceneBufferHandles(*renderNodeContextMgr_, stores_.dataStoreNameScene);
    if (RenderHandleUtil::IsValid(sceneBuffers_.camera)) {
        validShadowNode_ = true;
    }
}

void RenderNodeDefaultShadowRenderSlot::PreExecuteFrame()
{
    shadowCount_ = 0U;
    if (!validShadowNode_) {
        return;
    }

    ProcessBuffersAndDescriptors();

    const auto& dataMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreMaterial =
        GetRenderDataStore<IRenderDataStoreDefaultMaterial>(dataMgr, stores_.dataStoreNameMaterial);
    auto* dataStoreLight = GetRenderDataStore<IRenderDataStoreDefaultLight>(dataMgr, stores_.dataStoreNameLight);
    auto* dataStoreCamera = GetRenderDataStore<IRenderDataStoreDefaultCamera>(dataMgr, stores_.dataStoreNameCamera);
    auto* dataStoreScene = GetRenderDataStore<IRenderDataStoreDefaultScene>(dataMgr, stores_.dataStoreNameScene);

    if ((!dataStoreMaterial) || (!dataStoreLight) || (!dataStoreCamera) || (!dataStoreScene)) {
        return;
    }
    const auto scene = dataStoreScene->GetScene();
    const auto lightCounts = dataStoreLight->GetLightCounts();
    shadowCount_ = lightCounts.shadowCount;

    const Math::UVec2 res = dataStoreLight->GetShadowQualityResolution();
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const IRenderDataStoreDefaultLight::ShadowTypes shadowTypes = dataStoreLight->GetShadowTypes();
    if (shadowCount_ > 0) {
        const uint32_t xWidth = Math::min(res.x * shadowCount_, MAX_SHADOW_ATLAS_WIDTH);
        const uint32_t yHeight = res.y;
        const bool xChanged = (xWidth != shadowBuffers_.width);
        const bool yChanged = (yHeight != shadowBuffers_.height);
        const bool shadowTypeChanged = (shadowTypes.shadowType != shadowBuffers_.shadowTypes.shadowType);

        if (xChanged || yChanged || shadowTypeChanged) {
            shadowBuffers_.shadowTypes = shadowTypes;
            shadowBuffers_.width = xWidth;
            shadowBuffers_.height = yHeight;

            shadowBuffers_.depthHandle =
                gpuResourceMgr.Create(shadowBuffers_.depthName, GetDepthBufferDesc(shadowBuffers_));
            if (shadowBuffers_.shadowTypes.shadowType == IRenderDataStoreDefaultLight::ShadowType::VSM) {
                shadowBuffers_.vsmColorHandle = gpuResourceMgr.Create(
                    shadowBuffers_.vsmColorName, GetColorBufferDesc(gpuResourceMgr, shadowBuffers_));
            } else {
            }
        }
    }
}

IRenderNode::ExecuteFlags RenderNodeDefaultShadowRenderSlot::GetExecuteFlags() const
{
    // NOTE: shadow buffers should not be read
    // we can leave there old data without clearing if shadow count goes to zero
    if (validShadowNode_ && (shadowCount_ > 0U)) {
        return 0U;
    } else {
        return IRenderNode::ExecuteFlagBits::EXECUTE_FLAG_BITS_DO_NOT_EXECUTE;
    }
}

void RenderNodeDefaultShadowRenderSlot::ExecuteFrame(IRenderCommandList& cmdList)
{
    if ((!validShadowNode_) || (shadowCount_ == 0U)) {
        return;
    }

    const auto& dataMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    auto* storeScene = GetRenderDataStore<IRenderDataStoreDefaultScene>(dataMgr, stores_.dataStoreNameScene);
    auto* storeMaterial = GetRenderDataStore<IRenderDataStoreDefaultMaterial>(dataMgr, stores_.dataStoreNameMaterial);
    auto* storeCamera = GetRenderDataStore<IRenderDataStoreDefaultCamera>(dataMgr, stores_.dataStoreNameCamera);
    auto* storeLight = GetRenderDataStore<IRenderDataStoreDefaultLight>(dataMgr, stores_.dataStoreNameLight);

    if (storeScene && storeMaterial && storeCamera && storeLight && allShaderData_.slotHasShaders) {
        const auto scene = storeScene->GetScene();
        const auto lightCounts = storeLight->GetLightCounts();
        const uint32_t shadowCounts = lightCounts.shadowCount;
        CORE_ASSERT(shadowCounts == shadowCount_);
        if (shadowCounts == 0) {
            return; // early out
        }

        RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "3DShadows", DefaultDebugConstants::DEFAULT_DEBUG_COLOR);

        UpdateCurrentScene(*storeScene, *storeLight);
        UpdateGeneralDataUniformBuffers(*storeLight);

        const auto cameras = storeCamera->GetCameras();
        const auto lights = storeLight->GetLights();

        // write all shadows in a single render pass
        renderPass_ = CreateRenderPass(shadowBuffers_);
        cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);

        uint32_t shadowPassIdx = 0;
        for (const auto& light : lights) {
            if ((light.lightUsageFlags & RenderLight::LIGHT_USAGE_SHADOW_LIGHT_BIT) == 0) {
                continue;
            }
#if (CORE3D_VALIDATION_ENABLED == 1)
            if (light.shadowCameraIndex >= static_cast<uint32_t>(cameras.size())) {
                const string onceName = string(renderNodeContextMgr_->GetName().data()) + "_too_many_cam";
                CORE_LOG_ONCE_W(onceName,
                    "CORE3D_VALIDATION: RN: %s, shadow cameras dropped, too many cameras in scene",
                    renderNodeContextMgr_->GetName().data());
            }
#endif
            if (light.shadowCameraIndex < static_cast<uint32_t>(cameras.size())) {
                const auto& camera = cameras[light.shadowCameraIndex];
                // sort slot data to be accessible
                ProcessSlotSubmeshes(*storeCamera, *storeMaterial, light.shadowCameraIndex);
                if (!sortedSlotSubmeshes_.empty()) {
                    RenderSubmeshes(
                        cmdList, *storeMaterial, shadowBuffers_.shadowTypes.shadowType, camera, light, shadowPassIdx);
                }
            }

            shadowPassIdx++;
        }

        cmdList.EndRenderPass();
    }
}

void RenderNodeDefaultShadowRenderSlot::RenderSubmeshes(IRenderCommandList& cmdList,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const IRenderDataStoreDefaultLight::ShadowType shadowType,
    const RenderCamera& camera, const RenderLight& light, const uint32_t shadowPassIdx)
{
    const size_t submeshCount = sortedSlotSubmeshes_.size();

    // re-fetch global descriptor sets every frame
    const FrameGlobalDescriptorSets fgds = GetFrameGlobalDescriptorSets(renderNodeContextMgr_, stores_);
    if (!fgds.valid) {
        return; // cannot continue
    }

    // dynamic state
    {
        const int32_t xOffset = static_cast<int32_t>(light.shadowIndex * currentScene_.res.x);
        ViewportDesc vd = currentScene_.viewportDesc;
        vd.x = static_cast<float>(xOffset);
        ScissorDesc sd = currentScene_.scissorDesc;
        sd.offsetX = xOffset;
        cmdList.SetDynamicStateViewport(vd);
        cmdList.SetDynamicStateScissor(sd);
    }

    // set 0, update camera
    UpdateSet0(cmdList, shadowPassIdx);

    const uint64_t camLayerMask = camera.layerMask;
    RenderHandle boundPsoHandle = {};
    uint64_t boundShaderHash = 0;
    uint32_t currMaterialIndex = ~0u;
    bool initialBindDone = false; // cannot be checked from the idx
    bool hasSet2ImageData = false;
    const auto& selectableShaders =
        (shadowType == IRenderDataStoreDefaultLight::ShadowType::VSM) ? vsmShaders_ : pcfShaders_;

    const auto& submeshMaterialFlags = dataStoreMaterial.GetSubmeshMaterialFlags();
    const auto& submeshes = dataStoreMaterial.GetSubmeshes();
    for (size_t idx = 0; idx < submeshCount; ++idx) {
        // NOTE: submesh index is used to index into already updated descriptor set 2 slot
        // if the alpha shadow version is used
        const uint32_t submeshIndex = sortedSlotSubmeshes_[idx].submeshIndex;
        const auto& currSubmesh = submeshes[submeshIndex];

        // sorted slot submeshes should already have removed layers if default sorting was used
        if ((camLayerMask & currSubmesh.layers.layerMask) == 0) {
            continue;
        }
        auto currMaterialFlags = submeshMaterialFlags[submeshIndex];
        // get shader and graphics state and start hashing
        const auto& ssp = sortedSlotSubmeshes_[idx];
        ShaderStateData ssd { ssp.shaderHandle, ssp.gfxStateHandle, 0, selectableShaders.basic,
            selectableShaders.basicState };
        ssd.hash = (ssd.shader.id << 32U) | (ssd.gfxState.id & 0xFFFFffff);
        HashCombine(ssd.hash, ((ssd.defaultShader.id << 32U) | (ssd.defaultShaderState.id & 0xFFFFffff)));
        ssd.hash = HashShaderAndSubmesh(ssd.hash, currMaterialFlags.renderDepthHash, currSubmesh.buffers.inputAssembly);
        if (ssd.hash != boundShaderHash) {
            const PsoCreationValue psoVal =
                GetSubmeshPso(ssd, currSubmesh.buffers.inputAssembly, currMaterialFlags, currSubmesh.submeshFlags);
            if (psoVal.psoHandle.id != boundPsoHandle.id) {
                boundShaderHash = ssd.hash;
                boundPsoHandle = psoVal.psoHandle;
                hasSet2ImageData = psoVal.hasImageData;
                cmdList.BindPipeline(boundPsoHandle);
            }
        }

        // bind first set only the first time
        if (!initialBindDone) {
            const RenderHandle descriptorSetHandle = allDescriptorSets_.set0[shadowPassIdx]->GetDescriptorSetHandle();
            cmdList.BindDescriptorSets(0u, { &descriptorSetHandle, 1u });
        }

        // set 1 (material, mesh matrix and skin matrices)
        const uint32_t currMatOffset =
            currSubmesh.indices.materialFrameOffset * CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT;
        const uint32_t currMeshMatrixOffset = currSubmesh.indices.meshIndex * CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT;
        uint32_t currJointMatrixOffset = 0u;
        if (currSubmesh.submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) {
            currJointMatrixOffset =
                currSubmesh.indices.skinJointIndex * static_cast<uint32_t>(sizeof(DefaultMaterialSkinStruct));
        }
        const uint32_t dynamicOffsets[] = { currMeshMatrixOffset, currJointMatrixOffset, currMatOffset, currMatOffset,
            currMatOffset };
        // set to bind, handle to resource, offsets for dynamic descs
        IRenderCommandList::BindDescriptorSetData bindSets[2U] {};
        uint32_t bindSetCount = 0U;
        bindSets[bindSetCount++] = { fgds.set1, dynamicOffsets };

        // update material descriptor set
        if (bindlessEnabled_) {
            if (!initialBindDone) {
                bindSets[bindSetCount++] = { fgds.set2Default, {} };
            }
        } else if ((!initialBindDone) ||
                   ((currMaterialIndex != currSubmesh.indices.materialIndex) && hasSet2ImageData)) {
            currMaterialIndex = currSubmesh.indices.materialIndex;
            // safety check for global material sets
            const RenderHandle set2Handle =
                (currMaterialIndex < fgds.set2.size()) ? fgds.set2[currMaterialIndex] : fgds.set2Default;
            bindSets[bindSetCount++] = { set2Handle, {} };
        }

        // bind sets 1 and possibly 2
        cmdList.BindDescriptorSets(1U, { bindSets, bindSetCount });

        initialBindDone = true;

        // vertex buffers and draw
        if (currSubmesh.buffers.vertexBufferCount > 0) {
            cmdList.BindVertexBuffers({ currSubmesh.buffers.vertexBuffers, currSubmesh.buffers.vertexBufferCount });
        }
        const auto& dc = currSubmesh.drawCommand;
        const VertexBuffer& iArgs = currSubmesh.buffers.indirectArgsBuffer;
        const bool indirectDraw = RenderHandleUtil::IsValid(iArgs.bufferHandle);
        if ((currSubmesh.buffers.indexBuffer.byteSize > 0U) &&
            RenderHandleUtil::IsValid(currSubmesh.buffers.indexBuffer.bufferHandle)) {
            cmdList.BindIndexBuffer(currSubmesh.buffers.indexBuffer);
            if (indirectDraw) {
                cmdList.DrawIndexedIndirect(
                    iArgs.bufferHandle, iArgs.bufferOffset, dc.drawCountIndirect, dc.strideIndirect);
            } else {
                cmdList.DrawIndexed(dc.indexCount, dc.instanceCount, 0, 0, 0);
            }
        } else {
            if (indirectDraw) {
                cmdList.DrawIndirect(iArgs.bufferHandle, iArgs.bufferOffset, dc.drawCountIndirect, dc.strideIndirect);
            } else {
                cmdList.Draw(dc.vertexCount, dc.instanceCount, 0, 0);
            }
        }
    }
}

void RenderNodeDefaultShadowRenderSlot::UpdateSet0(IRenderCommandList& cmdList, const uint32_t shadowPassIdx)
{
    auto& binder = *allDescriptorSets_.set0[shadowPassIdx];
    uint32_t bindingIndex = 0;
    binder.BindBuffer(bindingIndex++, sceneBuffers_.camera, 0u);
    binder.BindBuffer(bindingIndex++, uboHandles_.generalData.GetHandle(), UBO_OFFSET_ALIGNMENT * shadowPassIdx);
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
}

void RenderNodeDefaultShadowRenderSlot::CreateDefaultShaderData()
{
    allShaderData_ = {};

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    // default viewport from sizes. camera will update later (if needed)
    currentScene_.viewportDesc = renderNodeUtil.CreateDefaultViewport(renderPass_);
    currentScene_.scissorDesc = renderNodeUtil.CreateDefaultScissor(renderPass_);

    allShaderData_.defaultPlHandle =
        bindlessEnabled_
            ? shaderMgr.GetPipelineLayoutHandle(DefaultMaterialShaderConstants::PIPELINE_LAYOUT_DEPTH_BINDLESS)
            : shaderMgr.GetPipelineLayoutHandle(DefaultMaterialShaderConstants::PIPELINE_LAYOUT_DEPTH);
    allShaderData_.defaultPipelineLayout = shaderMgr.GetPipelineLayout(allShaderData_.defaultPlHandle);
    allShaderData_.defaultVidHandle =
        shaderMgr.GetVertexInputDeclarationHandle(DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_DEPTH);

    // pcf
    {
        const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(jsonInputs_.renderSlotId);
        pcfShaders_.basic = rsd.shader.GetHandle();
        pcfShaders_.basicState = rsd.graphicsState.GetHandle();
        if (shaderMgr.IsShader(pcfShaders_.basic)) {
            allShaderData_.slotHasShaders = true;
            const ShaderSpecializationConstantView& sscv = shaderMgr.GetReflectionSpecialization(pcfShaders_.basic);
            allShaderData_.defaultSpecilizationConstants.resize(sscv.constants.size());
            for (uint32_t idx = 0; idx < (uint32_t)allShaderData_.defaultSpecilizationConstants.size(); ++idx) {
                allShaderData_.defaultSpecilizationConstants[idx] = sscv.constants[idx];
            }
        }
    }
    // vsm
    {
        const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(jsonInputs_.renderSlotVsmId);
        vsmShaders_.basic = rsd.shader.GetHandle();
        vsmShaders_.basicState = rsd.graphicsState.GetHandle();
    }

    // GPU resources
    {
        IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        allShaderData_.defaultBaseColor.handle =
            gpuResourceMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_MATERIAL_BASE_COLOR);
        allShaderData_.defaultBaseColor.samplerHandle =
            gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_REPEAT");
    }
}

namespace {
// updates graphics state based on params
inline GraphicsState GetNewGraphicsState(const IRenderNodeShaderManager& shaderMgr, const RenderHandle& handle,
    const bool inverseWinding, const bool customInputAssembly, const GraphicsState::InputAssembly& ia)
{
    // we create a new graphics state based on current
    GraphicsState gfxState = shaderMgr.GetGraphicsState(handle);
    // update state
    if (inverseWinding) {
        gfxState.rasterizationState.frontFace = FrontFace::CORE_FRONT_FACE_CLOCKWISE;
    }
    if (customInputAssembly) {
        gfxState.inputAssembly = ia;
    }
    return gfxState;
}
} // namespace

RenderNodeDefaultShadowRenderSlot::PsoCreationValue RenderNodeDefaultShadowRenderSlot::CreateNewPso(
    const ShaderStateData& ssd, const GraphicsState::InputAssembly& ia,
    const ShaderSpecializationConstantDataView& specialization, const RenderSubmeshFlags submeshFlags)
{
    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    RenderHandle currShader;
    RenderHandle currPl;
    RenderHandle currVid;
    RenderHandle currState;
    // first try to find matching shader
    if (RenderHandleUtil::GetHandleType(ssd.shader) == RenderHandleType::SHADER_STATE_OBJECT) {
        currShader = ssd.shader; // force given shader
        const RenderHandle slotShader = shaderMgr.GetShaderHandle(ssd.shader, currentScene_.renderSlotId);
        if (RenderHandleUtil::IsValid(slotShader)) {
            currShader = slotShader; // override with render slot variant
        }
        // if not explicit gfx state given, check if shader has graphics state for this slot
        if (!RenderHandleUtil::IsValid(ssd.gfxState)) {
            const auto gfxStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(currShader);
            if (shaderMgr.GetRenderSlotId(gfxStateHandle) == currentScene_.renderSlotId) {
                currState = gfxStateHandle;
            }
        }
        currVid = shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(currShader);
        currPl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(currShader);
    }
    if (RenderHandleUtil::GetHandleType(ssd.gfxState) == RenderHandleType::GRAPHICS_STATE) {
        const RenderHandle slotState = shaderMgr.GetGraphicsStateHandle(ssd.gfxState, currentScene_.renderSlotId);
        if (RenderHandleUtil::IsValid(slotState)) {
            currState = slotState;
        }
    }

    // fallback to defaults if needed
    currShader = RenderHandleUtil::IsValid(currShader) ? currShader : ssd.defaultShader;
    currPl = RenderHandleUtil::IsValid(currPl) ? currPl : allShaderData_.defaultPlHandle;
    currVid = RenderHandleUtil::IsValid(currVid) ? currVid : allShaderData_.defaultVidHandle;
    currState = RenderHandleUtil::IsValid(currState) ? currState : ssd.defaultShaderState;

    auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
    RenderHandle psoHandle;
    const bool inverseWinding = IsInverseWinding(submeshFlags);
    const bool customIa = (ia.primitiveTopology != CORE_PRIMITIVE_TOPOLOGY_MAX_ENUM) || (ia.enablePrimitiveRestart);
    // ATM pipeline layout setup to shader is not forced. Use default if not an extra set.
    if (inverseWinding || customIa) {
        const GraphicsState state = GetNewGraphicsState(shaderMgr, currState, inverseWinding, customIa, ia);
        const PipelineLayout& pl = shaderMgr.GetPipelineLayout(currPl);
        const VertexInputDeclarationView vidv = shaderMgr.GetVertexInputDeclarationView(currVid);
        psoHandle = psoMgr.GetGraphicsPsoHandle(
            currShader, state, pl, vidv, specialization, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    } else {
        psoHandle = psoMgr.GetGraphicsPsoHandle(
            currShader, currState, currPl, currVid, specialization, { DYNAMIC_STATES, countof(DYNAMIC_STATES) });
    }

    const PipelineLayout& shaderPl = shaderMgr.GetReflectionPipelineLayout(currShader);
    const bool hasSet2Images = (shaderPl.descriptorSetLayouts[2U].bindings.size() > 0);

    allShaderData_.perShaderData.push_back(PerShaderData { currShader, psoHandle, currState, hasSet2Images });
    allShaderData_.shaderIdToData[ssd.hash] = (uint32_t)allShaderData_.perShaderData.size() - 1;
    return { psoHandle, hasSet2Images };
}

RenderNodeDefaultShadowRenderSlot::PsoCreationValue RenderNodeDefaultShadowRenderSlot::GetSubmeshPso(
    const ShaderStateData& ssd, const GraphicsState::InputAssembly& ia,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags, const RenderSubmeshFlags submeshFlags)
{
    if (const auto dataIter = allShaderData_.shaderIdToData.find(ssd.hash);
        dataIter != allShaderData_.shaderIdToData.cend()) {
        const auto& data = allShaderData_.perShaderData[dataIter->second];
        return { data.psoHandle, data.hasSet2Images };
    }
    // specialization for not found hash
    constexpr size_t maxSpecializationFlagCount { 8u };
    uint32_t specializationFlags[maxSpecializationFlagCount];
    const size_t maxSpecializations =
        Math::min(maxSpecializationFlagCount, allShaderData_.defaultSpecilizationConstants.size());
    for (size_t idx = 0; idx < maxSpecializations; ++idx) {
        const auto& ref = allShaderData_.defaultSpecilizationConstants[idx];
        if (ref.shaderStage != ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT) {
            continue;
        }
        const uint32_t constantId = ref.offset / sizeof(uint32_t);
        switch (ref.id) {
            case 0u:
                specializationFlags[constantId] = submeshFlags & RenderDataDefaultMaterial::RENDER_SUBMESH_DEPTH_FLAGS;
                break;
            case 1u:
                specializationFlags[constantId] =
                    submeshMaterialFlags.renderMaterialFlags & RenderDataDefaultMaterial::RENDER_MATERIAL_DEPTH_FLAGS;
                break;
            default:
                break;
        }
    }

    const ShaderSpecializationConstantDataView spec { { allShaderData_.defaultSpecilizationConstants.data(),
                                                          maxSpecializations },
        { specializationFlags, maxSpecializations } };

    return CreateNewPso(ssd, ia, spec, submeshFlags);
}

RenderPass RenderNodeDefaultShadowRenderSlot::CreateRenderPass(const ShadowBuffers& buffers)
{
    // NOTE: the depth buffer needs to be samplable (optimmally with VSM it could be discarded)
    const bool isPcf = (buffers.shadowTypes.shadowType == IRenderDataStoreDefaultLight::ShadowType::PCF);
    RenderPass renderPass;
    renderPass.renderPassDesc.renderArea = { 0, 0, buffers.width, buffers.height };
    renderPass.renderPassDesc.subpassCount = 1;
    renderPass.subpassStartIndex = 0;
    auto& subpass = renderPass.subpassDesc;
    subpass.depthAttachmentCount = 1u;
    subpass.depthAttachmentIndex = 0u;
    renderPass.renderPassDesc.attachmentCount = 1u;
    renderPass.renderPassDesc.attachmentHandles[0] = buffers.depthHandle.GetHandle();
    renderPass.renderPassDesc.attachments[0] = {
        0,
        0,
        AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR,
        AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE,
        AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE,
        AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE,
        ClearValue { ClearDepthStencilValue { 1.0f, 0u } },
    };

    if (!isPcf) {
        subpass.colorAttachmentCount = 1;
        subpass.colorAttachmentIndices[0] = 1u;
        renderPass.renderPassDesc.attachmentCount++;
        renderPass.renderPassDesc.attachmentHandles[1] = buffers.vsmColorHandle.GetHandle();
        renderPass.renderPassDesc.attachments[1] = {
            0,
            0,
            AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR,
            AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE,
            AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE,
            AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_DONT_CARE,
            ClearValue { ClearColorValue { 1.0f, 1.0f, 0.0f, 0.0f } },
        };
    }

    return renderPass;
}

void RenderNodeDefaultShadowRenderSlot::ProcessBuffersAndDescriptors()
{
    if (!allDescriptorSets_.set0[0U]) {
        auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        const DescriptorCounts dc { {
            // camera and general data
            { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2U * DefaultMaterialLightingConstants::MAX_SHADOW_COUNT },
        } };
        descriptorSetMgr.ResetAndReserve(dc);

        for (uint32_t idx = 0; idx < DefaultMaterialLightingConstants::MAX_SHADOW_COUNT; ++idx) {
            {
                constexpr uint32_t setIdx = 0u;
                const RenderHandle descriptorSetHandle =
                    descriptorSetMgr.CreateDescriptorSet(setIdx, allShaderData_.defaultPipelineLayout);
                allDescriptorSets_.set0[idx] = descriptorSetMgr.CreateDescriptorSetBinder(
                    descriptorSetHandle, allShaderData_.defaultPipelineLayout.descriptorSetLayouts[setIdx].bindings);
            }
        }
    }
}

void RenderNodeDefaultShadowRenderSlot::UpdateCurrentScene(
    const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    const auto scene = dataStoreScene.GetScene();
    currentScene_.res = dataStoreLight.GetShadowQualityResolution();
    currentScene_.viewportDesc = { 0.0f, 0.0f, static_cast<float>(currentScene_.res.x),
        static_cast<float>(currentScene_.res.y), 0.0f, 1.0f };
    currentScene_.scissorDesc = { 0, 0, currentScene_.res.x, currentScene_.res.y };
    currentScene_.sceneTimingData = { scene.sceneDeltaTime, scene.deltaTime, scene.totalTime,
        *reinterpret_cast<const float*>(&scene.frameIndex) };

    currentScene_.renderSlotId =
        (shadowBuffers_.shadowTypes.shadowType == IRenderDataStoreDefaultLight::ShadowType::VSM)
            ? jsonInputs_.renderSlotVsmId
            : jsonInputs_.renderSlotId;
}

void RenderNodeDefaultShadowRenderSlot::UpdateGeneralDataUniformBuffers(
    const IRenderDataStoreDefaultLight& dataStoreLight)
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const Math::Vec2 viewportSize = { static_cast<float>(currentScene_.viewportDesc.width),
        static_cast<float>(currentScene_.viewportDesc.height) };
    const Math::Vec4 viewportSizeInvSize = { viewportSize.x, viewportSize.y, 1.0f / viewportSize.x,
        1.0f / viewportSize.y };
    DefaultMaterialGeneralDataStruct dataStruct {
        { 0, 0, 0u, 0u }, // NOTE: shadow camera id to both
        viewportSizeInvSize,
        currentScene_.sceneTimingData,
    };
    auto* data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboHandles_.generalData.GetHandle()));
    if (!data) {
        return;
    }
    const auto* dataEnd = data + UBO_OFFSET_ALIGNMENT * DefaultMaterialLightingConstants::MAX_SHADOW_COUNT;
    const auto lights = dataStoreLight.GetLights();
    uint32_t shadowPassIndex = 0;
    for (uint32_t lightIdx = 0; lightIdx < lights.size(); ++lightIdx) {
        const auto& light = lights[lightIdx];
        if ((light.lightUsageFlags & RenderLight::LIGHT_USAGE_SHADOW_LIGHT_BIT) == 0) {
            continue;
        }
        if (light.shadowCameraIndex != ~0u) {
            dataStruct.indices = { light.shadowCameraIndex, 0u, 0u, 0u };
            auto* currData = data + UBO_OFFSET_ALIGNMENT * shadowPassIndex;
            if (!CloneData(
                    currData, size_t(dataEnd - currData), &dataStruct, sizeof(DefaultMaterialGeneralDataStruct))) {
                CORE_LOG_E("generalData ubo copying failed.");
            }
        }
        shadowPassIndex++;
    }
    gpuResourceMgr.UnmapBuffer(uboHandles_.generalData.GetHandle());
}

void RenderNodeDefaultShadowRenderSlot::ProcessSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t shadowCameraIdx)
{
    const uint32_t cameraIndex = shadowCameraIdx;
    const IRenderNodeSceneUtil::RenderSlotInfo rsi { currentScene_.renderSlotId, jsonInputs_.sortType,
        jsonInputs_.cullType, 0 };
    RenderNodeSceneUtil::GetRenderSlotSubmeshes(
        dataStoreCamera, dataStoreMaterial, cameraIndex, {}, rsi, sortedSlotSubmeshes_);
}

void RenderNodeDefaultShadowRenderSlot::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();

    jsonInputs_.sortType = parserUtil.GetRenderSlotSortType(jsonVal, "renderSlotSortType");
    jsonInputs_.cullType = parserUtil.GetRenderSlotCullType(jsonVal, "renderSlotCullType");
    jsonInputs_.nodeFlags = static_cast<uint32_t>(parserUtil.GetUintValue(jsonVal, "nodeFlags"));
    if (jsonInputs_.nodeFlags == ~0u) {
        jsonInputs_.nodeFlags = 0;
    }

    string rsSlot = parserUtil.GetStringValue(jsonVal, "renderSlot");
    string rsSlotVsm = parserUtil.GetStringValue(jsonVal, "renderSlotVsm");
    // with bindless the default render slots are switched
    if (bindlessEnabled_) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_I("Switching to bindless variants of default material render slots (node:%s)",
            renderNodeContextMgr_->GetNodeName().data());
#endif
        if (rsSlot == DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH) {
            rsSlot = DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH_BINDLESS;
        }
        if (rsSlotVsm == DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH_VSM) {
            rsSlotVsm = DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH_VSM_BINDLESS;
        }
    }
    jsonInputs_.renderSlotId = renderNodeContextMgr_->GetShaderManager().GetRenderSlotId(rsSlot);
    jsonInputs_.renderSlotVsmId = renderNodeContextMgr_->GetShaderManager().GetRenderSlotId(rsSlotVsm);

    if (jsonInputs_.renderSlotVsmId == ~0u) {
        jsonInputs_.renderSlotVsmId = jsonInputs_.renderSlotId;
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_I("RN (%s), VSM render slot not given (renderSlotVsm)", renderNodeContextMgr_->GetName().data());
#endif
    }
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultShadowRenderSlot::Create()
{
    return new RenderNodeDefaultShadowRenderSlot();
}

void RenderNodeDefaultShadowRenderSlot::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultShadowRenderSlot*>(instance);
}
CORE3D_END_NAMESPACE()
