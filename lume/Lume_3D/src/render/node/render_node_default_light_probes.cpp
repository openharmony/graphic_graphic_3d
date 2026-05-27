/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "render_node_default_light_probes.h"

#include <algorithm>

#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/math/matrix_util.h>
#include <base/math/vector.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
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
#include "util/log.h"

#if (CORE3D_DEV_ENABLED == 1)
#include "render/datastore/render_data_store_default_material.h"
#endif

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
#include <render/shaders/common/render_post_process_structs_common.h>
}  // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
static constexpr uint32_t FIXED_CUSTOM_SET3{3u};

void BindVertextBufferAndDraw(IRenderCommandList& cmdList, const RenderSubmesh& currSubmesh)
{
    // vertex buffers and draw
    if (currSubmesh.buffers.vertexBufferCount > 0U) {
        cmdList.BindVertexBuffers({currSubmesh.buffers.vertexBuffers, currSubmesh.buffers.vertexBufferCount});
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

RenderDataDefaultMaterial::SubmeshMaterialFlags GetSubmeshMaterialFlags(
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const bool instanced, const bool hasShadows)
{
    // create a new copy and modify if needed (force instancing on and off)
    RenderDataDefaultMaterial::SubmeshMaterialFlags materialFlags = submeshMaterialFlags;
    if (!hasShadows) {  // remove shadow if not in scene
        materialFlags.renderMaterialFlags &= (~RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_RECEIVER_BIT);
        materialFlags.renderHash = dataStoreMaterial.GenerateRenderHash(materialFlags);
    }
    return materialFlags;
}

}  // namespace

void RenderNodeDefaultLightProbes::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
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

    // reset
    currentScene_ = {};
    allShaderData_ = {};

    if ((jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DIRECT_POST_PROCESS_BIT) &&
        jsonInputs_.renderDataStore.dataStoreName.empty()) {
        PLUGIN_LOG_V("%s: render data store post process configuration not set in render node graph",
            renderNodeContextMgr_->GetName().data());
    }
    rngRenderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    CreateDefaultShaderData();

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    defaultSamplers_.cubemapHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    defaultSamplers_.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    defaultSamplers_.nearestHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    defaultSamplers_.linearMipHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");
    defaultColorPrePassHandle_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
}

void RenderNodeDefaultLightProbes::ExecuteFrame(IRenderCommandList& cmdList)
{
    RenderCamera& cam = currentScene_.camData.camera;

    uint32_t probesToBake = static_cast<uint32_t>(cam.lightProbeBakingData.perProbeData.size());
    if (probesToBake == 0u) {
        return;
    }

    constexpr uint32_t maxProbesInAtlas = CORE_MAX_NUM_LIGHT_PROBE_BAKES_PER_ITERATION;
    if (probesToBake > maxProbesInAtlas) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        PLUGIN_LOG_E("RenderNodeDefaultLightProbes: Too many probes to bake, max is %d per frame but received %d",
            static_cast<int>(maxProbesInAtlas),
            static_cast<int>(probesToBake));
        probesToBake = maxProbesInAtlas;
#endif
    }

    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    const bool validRenderDataStore = dataStoreScene && dataStoreMaterial && dataStoreCamera && dataStoreLight;

    // upload
    uint8_t* const mappedData = reinterpret_cast<uint8_t*>(
        renderNodeContextMgr_->GetGpuResourceManager().MapBufferMemory(probeViewMatricesUbo_.GetHandle()));

    if (mappedData) {
        const uint32_t byteSize = static_cast<uint32_t>(sizeof(BASE_NS::Math::Mat4X4) * viewMatrices_.size());
        const uint32_t fullBufferByteSize =
            static_cast<uint32_t>(sizeof(BASE_NS::Math::Mat4X4) * CORE_MAX_NUM_LIGHT_PROBE_BAKES_PER_ITERATION * 6u);
        CloneData(mappedData, byteSize, viewMatrices_.data(), byteSize);
        ClearToValue(mappedData + byteSize, fullBufferByteSize - byteSize, 0, fullBufferByteSize - byteSize);
        renderNodeContextMgr_->GetGpuResourceManager().UnmapBuffer(probeViewMatricesUbo_.GetHandle());
    }

#if (CORE3D_VALIDATION_ENABLED == 1)
    RENDER_DEBUG_MARKER_COL_SCOPE(
        cmdList, "3DLightProbeMaterial" + jsonInputs_.renderSlotName, DefaultDebugConstants::DEFAULT_DEBUG_COLOR);
#else
    RENDER_DEBUG_MARKER_COL_SCOPE(cmdList, "3DLightProbeMaterial", DefaultDebugConstants::DEFAULT_DEBUG_COLOR);
#endif

    for (uint32_t probeIdx = 0u; probeIdx < probesToBake; ++probeIdx) {
        for (uint32_t faceIdx = 0u; faceIdx < 6u; ++faceIdx) {
            const auto& renderArea = cam.lightProbeBakingData.perProbeData[probeIdx].probeAtlasRenderAreas[faceIdx];

            renderPass_.renderPassDesc.renderArea.offsetX = renderArea.offsetX;
            renderPass_.renderPassDesc.renderArea.offsetY = renderArea.offsetY;
            renderPass_.renderPassDesc.renderArea.extentWidth = renderArea.extentWidth;
            renderPass_.renderPassDesc.renderArea.extentHeight = renderArea.extentHeight;

            currentScene_.viewportDesc.x = static_cast<float>(renderArea.offsetX);
            currentScene_.viewportDesc.y = static_cast<float>(renderArea.offsetY);
            currentScene_.viewportDesc.width = static_cast<float>(renderArea.extentWidth);
            currentScene_.viewportDesc.height = static_cast<float>(renderArea.extentHeight);

            currentScene_.scissorDesc.offsetX = renderArea.offsetX;
            currentScene_.scissorDesc.offsetY = renderArea.offsetY;
            currentScene_.scissorDesc.extentWidth = renderArea.extentWidth;
            currentScene_.scissorDesc.extentHeight = renderArea.extentHeight;

            cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);

            if (validRenderDataStore) {
                const auto cameras = dataStoreCamera->GetCameras();
                const auto& camData = currentScene_.camData;

                const bool hasShaders = allShaderData_.slotHasShaders;
                const bool hasCamera = !cameras.empty() && (camData.cameraIdx < (uint32_t)cameras.size());

                ProcessSlotSubmeshes(*dataStoreCamera, *dataStoreMaterial);
                const bool hasSubmeshes = !sortedSlotSubmeshes_.empty();
                if (hasShaders && hasCamera && hasSubmeshes) {
                    UpdatePostProcessConfiguration();
                    uint32_t viewMatrixIndex = probeIdx * 6u + faceIdx;
                    RenderSubmeshesProbes(cmdList, *dataStoreMaterial, *dataStoreCamera, viewMatrixIndex);
                }
            }

            cmdList.EndRenderPass();
        }
    }
}

void RenderNodeDefaultLightProbes::ResetRenderSlotData(const uint32_t shaderRenderSlotId, const bool multiView)
{
    // can be reset to multi-view usage or reset back to default usage
    if (shaderRenderSlotId != jsonInputs_.shaderRenderSlotId) {
        jsonInputs_.shaderRenderSlotId = shaderRenderSlotId;
        jsonInputs_.explicitShader = jsonInputs_.initialExplicitShader || multiView;
        // reset
        CreateDefaultShaderData();
    }
}

void RenderNodeDefaultLightProbes::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
        rngRenderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    }
    // get default RNG based render pass setup
    renderPass_ = rngRenderPass_;

    const auto scene = dataStoreScene.GetScene();
    currentScene_.camData = RenderNodeSceneUtil::GetSceneCameraData(
        dataStoreScene, dataStoreCamera, jsonInputs_.customCameraId, jsonInputs_.customCameraName);
    const auto& cam = currentScene_.camData.camera;
    const bool legacyCamera =
        currentScene_.camData.flags & SceneRenderCameraDataFlagBits::SCENE_CAMERA_DATA_FLAG_LEGACY_MAIN_BIT;

    const auto camHandles = RenderNodeSceneUtil::GetSceneCameraImageHandles(
        *renderNodeContextMgr_, stores_.dataStoreNameScene, cam.name, cam);
    currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;

    if (!cam.prePassColorTargetName.empty()) {
        currentScene_.prePassColorTarget = renderNodeContextMgr_->GetGpuResourceManager().GetImageHandle(
            currentScene_.camData.camera.prePassColorTargetName);
    }

    // renderpass needs to be valid (created in init)
    if (!legacyCamera) {
        // uses camera based loadOp clear override if given
        const bool isNamedCamera =
            (currentScene_.camData.flags == SceneRenderCameraDataFlagBits::SCENE_CAMERA_DATA_FLAG_NAMED_BIT);
        RenderNodeSceneUtil::UpdateRenderPassFromCustomCamera(cam, isNamedCamera, renderPass_);
    } else {
        RenderNodeSceneUtil::UpdateRenderPassFromCamera(cam, renderPass_);
    }
    currentScene_.viewportDesc = RenderNodeSceneUtil::CreateViewportFromCamera(cam);
    currentScene_.scissorDesc = RenderNodeSceneUtil::CreateScissorFromCamera(cam);

    const IRenderDataStoreDefaultLight::LightCounts lightCounts = dataStoreLight.GetLightCounts();
    currentScene_.hasShadow = (lightCounts.shadowCount > 0) ? true : false;
    currentScene_.shadowTypes = dataStoreLight.GetShadowTypes();
    currentScene_.lightingFlags = dataStoreLight.GetLightingFlags();
    currentScene_.cameraShaderFlags = cam.shaderFlags;
    // remove fog explicitly if render node graph input and/or default render slot usage states so
    if (jsonInputs_.nodeFlags & RenderSceneFlagBits::RENDER_SCENE_DISABLE_FOG_BIT) {
        currentScene_.cameraShaderFlags &= (~RenderCamera::ShaderFlagBits::CAMERA_SHADER_FOG_BIT);
    }
    if ((currentScene_.camData.camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_OUTPUT_VELOCITY_NORMAL_BIT) ==
        RenderCamera::CameraFlagBits::CAMERA_FLAG_OUTPUT_VELOCITY_NORMAL_BIT) {
        currentScene_.cameraShaderFlags |= RenderCamera::ShaderFlagBits::CAMERA_SHADER_VELOCITY_OUT_BIT;
    }
    if ((currentScene_.camData.camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_LIGHT_PROBE_BAKE_BIT) ==
        RenderCamera::CameraFlagBits::CAMERA_FLAG_LIGHT_PROBE_BAKE_BIT) {
        currentScene_.cameraShaderFlags |= RenderCamera::ShaderFlagBits::CAMERA_SHADER_LIGHT_PROBE_BAKE_BIT;
    }

    RenderNodeSceneUtil::GetMultiViewCameraIndices(dataStoreCamera, cam, currentScene_.mvCameraIndices);
    // add multi-view flags if needed
    if (renderPass_.subpassDesc.viewMask > 1U) {
        ResetRenderSlotData(jsonInputs_.shaderRenderSlotMultiviewId, true);
    } else {
        ResetRenderSlotData(jsonInputs_.shaderRenderSlotBaseId, false);
    }
}

void RenderNodeDefaultLightProbes::RenderSubmeshesProbes(IRenderCommandList& cmdList,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const IRenderDataStoreDefaultCamera& dataStoreCamera,
    const uint32_t viewMatrixIndex)
{
    // re-fetch global descriptor sets every frame
    const RenderNodeSceneUtil::FrameGlobalDescriptorSets fgds =
        RenderNodeSceneUtil::GetFrameGlobalDescriptorSets(*renderNodeContextMgr_,
            stores_,
            cameraName_,
            RenderNodeSceneUtil::FrameGlobalDescriptorSetFlagBits::GLOBAL_SET_ALL);
    if (!fgds.valid) {
        return;  // cannot continue
    }

    // dynamic state
    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);
    if (fsrEnabled_) {
        cmdList.SetDynamicStateFragmentShadingRate({1u, 1u},
            FragmentShadingRateCombinerOps{
                CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE, CORE_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE});
    }

    // first two sets are only bound for the first submesh (inside for loop)
    // no bindless, we need to update images per object
    PipelineInfo pipelineInfo;
    uint32_t currMaterialIndex = ~0u;
    bool initialBindDone = false;  // cannot be checked from the idx

    const auto& submeshMaterialFlags = dataStoreMaterial.GetSubmeshMaterialFlags();
    const auto& submeshes = dataStoreMaterial.GetSubmeshes();
    const auto& customResourceHandles = dataStoreMaterial.GetCustomResourceHandles();
    const uint64_t camLayerMask = currentScene_.camData.camera.layerMask;
    const uint64_t camScene = currentScene_.camData.camera.sceneId;

    for (auto ssp : sortedSlotSubmeshes_) {
        ssp.shaderHandle = allShaderData_.defaultShaderHandle;
        const uint32_t submeshIndex = ssp.submeshIndex;
        const auto& currSubmesh = submeshes[submeshIndex];
        if (currSubmesh.layers.sceneId != camScene) {
            continue;
        }
        // sorted slot submeshes should already have removed layers if default sorting was used
        if (((camLayerMask & currSubmesh.layers.layerMask) == 0U) ||
            ((jsonInputs_.nodeFlags & RENDER_SCENE_DISCARD_MATERIAL_BIT) &&
                (submeshMaterialFlags[submeshIndex].extraMaterialRenderingFlags &
                    RenderExtraRenderingFlagBits::RENDER_EXTRA_RENDERING_DISCARD_BIT))) {
            continue;
        }
        const auto materialSubmeshFlags = GetSubmeshMaterialFlags(submeshMaterialFlags[submeshIndex],
            dataStoreMaterial,
            (currSubmesh.drawCommand.instanceCount > 1U),
            currentScene_.hasShadow);
        const RenderSubmeshFlags submeshFlags = currSubmesh.submeshFlags | jsonInputs_.nodeSubmeshExtraFlags;

        BindPipeline(cmdList, ssp, materialSubmeshFlags, submeshFlags, currSubmesh.buffers.inputAssembly, pipelineInfo);

        // bind first set only the first time
        if (!initialBindDone) {
            cmdList.BindDescriptorSet(0, fgds.set0);
        }

        currMaterialIndex = BindSet1And2(cmdList, currSubmesh, submeshFlags, initialBindDone, fgds, currMaterialIndex);
        initialBindDone = true;

        // set3 has e.g. the view matrices
        UpdateAndBindSet3Probes(cmdList, customResourceHandles[currSubmesh.indices.materialIndex]);

        // push constant for probe-face view matrix indexing
        struct {
            uint32_t lightProbeDataIndex;
            uint32_t viewMatrixIndex;
        } pushConstantData;
        pushConstantData.lightProbeDataIndex = submeshIndex;
        pushConstantData.viewMatrixIndex = viewMatrixIndex;
        PushConstant pc{
            ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT | ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT,
            sizeof(pushConstantData)};
        cmdList.PushConstantData(pc, arrayviewU8(pushConstantData));
        BindVertextBufferAndDraw(cmdList, currSubmesh);
    }
}

void RenderNodeDefaultLightProbes::PreExecuteFrame()
{
    RenderNodeDefaultMaterialRenderSlot::PreExecuteFrame();

    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    const bool validRenderDataStore = dataStoreScene && dataStoreMaterial && dataStoreCamera && dataStoreLight;
    if (validRenderDataStore) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);
    } else {
        PLUGIN_LOG_E("invalid render data stores in RenderNodeDefaultLightProbes");
    }

    static const Math::Vec3 cubeFaceDirections[6] = {
        {1.0f, 0.0f, 0.0f},   // +x
        {-1.0f, 0.0f, 0.0f},  // -x
        {0.0f, 1.0f, 0.0f},   // +y
        {0.0f, -1.0f, 0.0f},  // -y
        {0.0f, 0.0f, 1.0f},   // +z
        {0.0f, 0.0f, -1.0f}   // -z
    };

    static const Math::Vec3 cubeFaceUps[6] = {
        {0.0f, -1.0f, 0.0f},  // +x
        {0.0f, -1.0f, 0.0f},  // -x
        {0.0f, 0.0f, 1.0f},   // +y
        {0.0f, 0.0f, -1.0f},  // -y
        {0.0f, -1.0f, 0.0f},  // +z
        {0.0f, -1.0f, 0.0f}   // -z
    };

    RenderCamera& cam = currentScene_.camData.camera;

    viewMatrices_.clear();

    // collect view matrices
    for (uint32_t probeIdx = 0; probeIdx < cam.lightProbeBakingData.perProbeData.size(); probeIdx++) {
        const RenderCamera::LightProbeBakingData::PerProbeData& bakeData =
            cam.lightProbeBakingData.perProbeData[probeIdx];
        for (size_t face = 0; face < 6u; face++) {
            // view matrix for this cubemap face
            const Math::Vec3 target = bakeData.probePosition + cubeFaceDirections[face];
            viewMatrices_.push_back(Math::LookAtRh(bakeData.probePosition, target, cubeFaceUps[face]));
        }
    }

    // upload to GPU and update UBO
    if (!probeViewMatricesUbo_) {
        GpuBufferDesc desc;
        desc.usageFlags = BufferUsageFlagBits::CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT |
                                   MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
        desc.engineCreationFlags = EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_CREATE_IMMEDIATE |
                                   EngineBufferCreationFlagBits::CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER;

        desc.byteSize =
            static_cast<uint32_t>(sizeof(BASE_NS::Math::Mat4X4) * CORE_MAX_NUM_LIGHT_PROBE_BAKES_PER_ITERATION * 6u);
        probeViewMatricesUbo_ = renderNodeContextMgr_->GetGpuResourceManager().Create(desc);
    }
}

bool RenderNodeDefaultLightProbes::UpdateAndBindSet3Probes(
    IRenderCommandList& cmdList, const RenderDataDefaultMaterial::CustomResourceData& customResourceData)
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    // with default pipeline layout with default bindings
    RenderHandle currPlHandle = allShaderData_.defaultPlSet3
                                    ? allShaderData_.defaultPlHandle
                                    : shaderMgr.GetPipelineLayoutHandleByShaderHandle(customResourceData.shaderHandle);
    if (!RenderHandleUtil::IsValid(currPlHandle)) {
        currPlHandle = shaderMgr.GetReflectionPipelineLayoutHandle(customResourceData.shaderHandle);
    }
    const PipelineLayout& plRef = shaderMgr.GetPipelineLayout(currPlHandle);
    if (plRef.descriptorSetLayouts[FIXED_CUSTOM_SET3].bindings.empty()) {
        return false;  // early out
    }
    static_assert(FIXED_CUSTOM_SET3 <
                  BASE_NS::extent_v<decltype(BASE_NS::remove_reference_t<decltype(plRef)>::descriptorSetLayouts)>);
    const auto& descBindings = plRef.descriptorSetLayouts[FIXED_CUSTOM_SET3].bindings;
    const RenderHandle descSetHandle = descriptorSetMgr.CreateOneFrameDescriptorSet(descBindings);
    if (!RenderHandleUtil::IsValid(descSetHandle)) {
        return false;
    }
    IDescriptorSetBinder::Ptr binderPtr = descriptorSetMgr.CreateDescriptorSetBinder(descSetHandle, descBindings);
    if (!binderPtr) {
        return false;
    }
    auto& binder = *binderPtr;
    uint32_t idx = 0;
    for (; idx < customResourceData.resourceHandleCount; ++idx) {
        PLUGIN_ASSERT(idx < descBindings.size());
        const RenderHandle& currRes = customResourceData.resourceHandles[idx];
        if (gpuResourceMgr.IsGpuBuffer(currRes)) {
            binder.BindBuffer(idx, currRes, 0);
        } else if (gpuResourceMgr.IsGpuImage(currRes)) {
            if (descBindings[idx].descriptorType == DescriptorType::CORE_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                binder.BindImage(idx, currRes, defaultSamplers_.linearMipHandle);
            } else {
                binder.BindImage(idx, currRes);
            }
        } else if (gpuResourceMgr.IsGpuSampler(currRes)) {
            binder.BindSampler(idx, currRes);
        }
    }

    // bind view matrix buffer
    {
        BindableBuffer vmBuffer;
        vmBuffer.byteSize =
            static_cast<uint32_t>(sizeof(BASE_NS::Math::Mat4X4) * CORE_MAX_NUM_LIGHT_PROBE_BAKES_PER_ITERATION * 6u);
        vmBuffer.handle = probeViewMatricesUbo_.GetHandle();
        vmBuffer.byteOffset = 0;
        binder.BindBuffer(4u, vmBuffer);
    }

    // user generated setup, we check for validity of all bindings in the descriptor set
    if (!binder.GetDescriptorSetLayoutBindingValidity()) {
        return false;
    }
    cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
    cmdList.BindDescriptorSet(FIXED_CUSTOM_SET3, binder.GetDescriptorSetHandle());
    return true;
}

void RenderNodeDefaultLightProbes::CreateDefaultShaderData()
{
    allShaderData_ = {};

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();

    // get the default material shader and default shader state

    const IShaderManager::RenderSlotData shaderRsd = shaderMgr.GetRenderSlotData(jsonInputs_.shaderRenderSlotId);

    const auto bakeShader =
        shaderMgr.GetShaderDataByShaderName("3dshaders://shader/core3d_dm_fw_light_probe_bake.shader");

    allShaderData_.defaultShaderHandle = bakeShader.shader;
    allShaderData_.defaultStateHandle = shaderRsd.graphicsState.GetHandle();
    // get the pl and vid defaults
    if (bindlessEnabled_) {
        PLUGIN_LOG_E("RenderNodeDefaultLightProbes::CreateDefaultShaderData: bindless rendering is currently not "
                     "supported with light probes!");
        return;
    }
    allShaderData_.defaultPlHandle = bakeShader.pipelineLayout;
    allShaderData_.defaultPipelineLayout = shaderMgr.GetPipelineLayout(allShaderData_.defaultPlHandle);
    allShaderData_.defaultTmpPipelineLayout = allShaderData_.defaultPipelineLayout;
    allShaderData_.defaultVidHandle = (shaderRsd.vertexInputDeclaration)
                                          ? shaderRsd.vertexInputDeclaration.GetHandle()
                                          : shaderMgr.GetVertexInputDeclarationHandle(
                                                DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_FORWARD);
    if (!allShaderData_.defaultPipelineLayout.descriptorSetLayouts[FIXED_CUSTOM_SET3].bindings.empty()) {
        allShaderData_.defaultPlSet3 = true;
    }

    if (shaderMgr.IsShader(allShaderData_.defaultShaderHandle)) {
        allShaderData_.slotHasShaders = true;
        const ShaderSpecializationConstantView& sscv =
            shaderMgr.GetReflectionSpecialization(allShaderData_.defaultShaderHandle);
        allShaderData_.defaultSpecilizationConstants.resize(sscv.constants.size());
        for (uint32_t idx = 0; idx < (uint32_t)allShaderData_.defaultSpecilizationConstants.size(); ++idx) {
            allShaderData_.defaultSpecilizationConstants[idx] = sscv.constants[idx];
        }
        specializationData_.maxSpecializationCount =
            Math::min(static_cast<uint32_t>(allShaderData_.defaultSpecilizationConstants.size()),
                SpecializationData::MAX_FLAG_COUNT);
    } else {
        PLUGIN_LOG_I("RenderNode: %s, no default shaders for render slot id %u",
            renderNodeContextMgr_->GetName().data(),
            jsonInputs_.shaderRenderSlotId);
    }
    if (jsonInputs_.shaderRenderSlotId != jsonInputs_.stateRenderSlotId) {
        const IShaderManager::RenderSlotData stateRsd = shaderMgr.GetRenderSlotData(jsonInputs_.stateRenderSlotId);
        if (stateRsd.graphicsState) {
            allShaderData_.defaultStateHandle = stateRsd.graphicsState.GetHandle();
        } else {
            PLUGIN_LOG_I("RenderNode: %s, no default state for render slot id %u",
                renderNodeContextMgr_->GetName().data(),
                jsonInputs_.stateRenderSlotId);
        }
    }
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultLightProbes::Create()
{
    return new RenderNodeDefaultLightProbes();
}

void RenderNodeDefaultLightProbes::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultLightProbes*>(instance);
}

CORE3D_END_NAMESPACE()
