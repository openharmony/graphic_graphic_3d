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

#include "render_node_default_depth_render_slot.h"

#include <algorithm>

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/math/matrix_util.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/resource_handle.h>

#include "render/render_node_scene_util.h"

#if (CORE3D_DEV_ENABLED == 1)
#include "render/datastore/render_data_store_default_material.h"
#endif

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr DynamicStateEnum DYNAMIC_STATES[] = { CORE_DYNAMIC_STATE_ENUM_VIEWPORT, CORE_DYNAMIC_STATE_ENUM_SCISSOR };
constexpr uint32_t UBO_BIND_OFFSET_ALIGNMENT { PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };

inline uint64_t HashShaderAndSubmesh(const uint64_t shaderDataHash, const uint32_t renderHash)
{
    uint64_t hash = (uint64_t)renderHash;
    HashCombine(hash, shaderDataHash);
    return hash;
}

inline bool IsInverseWinding(const RenderSubmeshFlags submeshFlags, const RenderSceneFlags sceneRenderingFlags,
    const RenderCamera::Flags cameraRenderingFlags)
{
    const bool flipWinding = (sceneRenderingFlags & RENDER_SCENE_FLIP_WINDING_BIT) |
                             (cameraRenderingFlags & RenderCamera::CAMERA_FLAG_INVERSE_WINDING_BIT);
    const bool isNegative = flipWinding
                                ? !((submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT) > 0)
                                : ((submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT) > 0);
    return isNegative;
}
} // namespace

void RenderNodeDefaultDepthRenderSlot::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    // reset
    currentScene_ = {};
    allShaderData_ = {};
    objectCounts_ = {};

    rngRenderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    CreateDefaultShaderData();

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    buffers_.defaultBuffer = gpuResourceMgr.GetBufferHandle("CORE_DEFAULT_GPU_BUFFER");
    GetSceneUniformBuffers(stores_.dataStoreNameScene);
    GetCameraUniformBuffers(stores_.dataStoreNameScene);
}

void RenderNodeDefaultDepthRenderSlot::PreExecuteFrame()
{
    // re-create needed gpu resources
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const IRenderDataStoreDefaultMaterial* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
    // we need to have all buffers created (reason to have at least 1u)
    if (dataStoreMaterial) {
        const auto dsOc = dataStoreMaterial->GetSlotObjectCounts(jsonInputs_.renderSlotId);
        const ObjectCounts oc {
            Math::max(dsOc.meshCount, 1u),
            Math::max(dsOc.submeshCount, 1u),
            Math::max(dsOc.skinCount, 1u),
        };
        ProcessBuffersAndDescriptors(oc);
    } else if (objectCounts_.maxSlotMeshCount == 0) {
        ProcessBuffersAndDescriptors({ 1u, 1u, 1u });
    }
}

void RenderNodeDefaultDepthRenderSlot::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreMaterial = static_cast<IRenderDataStoreDefaultMaterial*>(
        renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameMaterial));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));

    const bool validRenderDataStore = dataStoreScene && dataStoreMaterial && dataStoreCamera;
    if (validRenderDataStore) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera);
    } else {
        CORE_LOG_E("invalid render data stores in RenderNodeDefaultDepthRenderSlot");
    }

    cmdList.BeginRenderPass(renderPass_.renderPassDesc, renderPass_.subpassStartIndex, renderPass_.subpassDesc);

    if (validRenderDataStore) {
        const auto cameras = dataStoreCamera->GetCameras();
        const auto scene = dataStoreScene->GetScene();

        const bool hasShaders = allShaderData_.slotHasShaders;
        const bool hasCamera =
            (!cameras.empty() && (currentScene_.cameraIdx < (uint32_t)cameras.size())) ? true : false;

        ProcessSlotSubmeshes(*dataStoreCamera, *dataStoreMaterial);
        const bool hasSubmeshes = (!sortedSlotSubmeshes_.empty());
        if (hasShaders && hasCamera && hasSubmeshes) {
            RenderSubmeshes(cmdList, *dataStoreMaterial, *dataStoreCamera);
        }
    }

    cmdList.EndRenderPass();
}

void RenderNodeDefaultDepthRenderSlot::RenderSubmeshes(IRenderCommandList& cmdList,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const IRenderDataStoreDefaultCamera& dataStoreCamera)
{
    const size_t submeshCount = sortedSlotSubmeshes_.size();
    CORE_ASSERT(submeshCount <= objectCounts_.maxSlotSubmeshCount);

    // dynamic state
    cmdList.SetDynamicStateViewport(currentScene_.viewportDesc);
    cmdList.SetDynamicStateScissor(currentScene_.scissorDesc);

    // set 0, update camera, general data
    // set 1, update mesh matrices, material data, and skinning data (uses dynamic offset)
    UpdateSet01(cmdList);

    // first two sets are only bound for the first submesh (inside for loop)
    // no bindless, we need to update images per object
    RenderHandle boundPsoHandle = {};
    uint64_t boundShaderHash = 0;
    bool initialBindDone = false; // cannot be checked from the idx

    const auto& submeshMaterialFlags = dataStoreMaterial.GetSubmeshMaterialFlags();
    const auto& submeshes = dataStoreMaterial.GetSubmeshes();
    const uint64_t camLayerMask = currentScene_.camera.layerMask;
    for (size_t idx = 0; idx < submeshCount; ++idx) {
        const auto& ssp = sortedSlotSubmeshes_[idx];
        const uint32_t submeshIndex = ssp.submeshIndex;
        const auto& currSubmesh = submeshes[submeshIndex];
        const RenderSubmeshFlags submeshFlags = currSubmesh.submeshFlags | jsonInputs_.nodeSubmeshExtraFlags;
        auto currMaterialFlags = submeshMaterialFlags[submeshIndex];

        // sorted slot submeshes should already have removed layers if default sorting was used
        if (((camLayerMask & currSubmesh.layerMask) == 0) ||
            ((jsonInputs_.nodeFlags & RENDER_SCENE_DISCARD_MATERIAL_BIT) &&
                (currMaterialFlags.extraMaterialRenderingFlags &
                    RenderExtraRenderingFlagBits::RENDER_EXTRA_RENDERING_DISCARD_BIT))) {
            continue;
        }

        const uint32_t renderHash = currMaterialFlags.renderHash;
        // get shader and graphics state and start hashing
        ShaderStateData ssd { ssp.shaderHandle, ssp.gfxStateHandle, 0 };
        ssd.hash = (ssd.shader.id << 32U) | (ssd.gfxState.id & 0xFFFFffff);
        // current shader state is fetched for build-in and custom shaders (decision is made later)
        ssd.hash = HashShaderAndSubmesh(ssd.hash, renderHash);
        if (ssd.hash != boundShaderHash) {
            const RenderHandle pso = GetSubmeshPso(ssd, currMaterialFlags, submeshFlags);
            if (pso.id != boundPsoHandle.id) {
                boundShaderHash = ssd.hash;
                boundPsoHandle = pso;
                cmdList.BindPipeline(boundPsoHandle);
            }
        }

        // bind first set only the first time
        if (!initialBindDone) {
            RenderHandle setResourceHandles[2u] { allDescriptorSets_.set01[0u]->GetDescriptorSetHandle(),
                allDescriptorSets_.set01[1u]->GetDescriptorSetHandle() };
            cmdList.BindDescriptorSets(0, { setResourceHandles, 2u });
        }

        // set 1 (mesh matrix, skin matrices, material, material user data)
        const uint32_t currMeshMatrixOffset = currSubmesh.meshIndex * UBO_BIND_OFFSET_ALIGNMENT;
        uint32_t currJointMatrixOffset = 0u;
        if (submeshFlags & RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT) {
            currJointMatrixOffset =
                currSubmesh.skinJointIndex * static_cast<uint32_t>(sizeof(DefaultMaterialSkinStruct));
        }
        const uint32_t dynamicOffsets[] = { currMeshMatrixOffset, currJointMatrixOffset };
        // set to bind, handle to resource, offsets for dynamic descs
        cmdList.BindDescriptorSet(1u, allDescriptorSets_.set01[1u]->GetDescriptorSetHandle(), dynamicOffsets);

        initialBindDone = true;

        // vertex buffers and draw
        if (currSubmesh.vertexBufferCount > 0) {
            VertexBuffer vbs[RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT];
            for (uint32_t vbIdx = 0; vbIdx < currSubmesh.vertexBufferCount; ++vbIdx) {
                vbs[vbIdx] = ConvertVertexBuffer(currSubmesh.vertexBuffers[vbIdx]);
            }
            cmdList.BindVertexBuffers({ vbs, currSubmesh.vertexBufferCount });
        }
        const auto& dc = currSubmesh.drawCommand;
        const RenderVertexBuffer& iArgs = currSubmesh.indirectArgsBuffer;
        const bool indirectDraw = iArgs.bufferHandle ? true : false;
        if (currSubmesh.indexBuffer.byteSize > 0U) {
            cmdList.BindIndexBuffer(ConvertIndexBuffer(currSubmesh.indexBuffer));
            if (indirectDraw) {
                cmdList.DrawIndexedIndirect(iArgs.bufferHandle.GetHandle(), iArgs.bufferOffset, 1u, 0u);
            } else {
                cmdList.DrawIndexed(dc.indexCount, dc.instanceCount, 0, 0, 0);
            }
        } else {
            if (indirectDraw) {
                cmdList.DrawIndirect(iArgs.bufferHandle.GetHandle(), iArgs.bufferOffset, 1u, 0u);
            } else {
                cmdList.Draw(dc.vertexCount, dc.instanceCount, 0, 0);
            }
        }
    }
}

void RenderNodeDefaultDepthRenderSlot::UpdateSet01(IRenderCommandList& cmdList)
{
    auto& binder0 = *allDescriptorSets_.set01[0u];
    auto& binder1 = *allDescriptorSets_.set01[1u];
    {
        uint32_t bindingIndex = 0;
        binder0.BindBuffer(bindingIndex++, buffers_.camera, 0u);
        binder0.BindBuffer(bindingIndex++, buffers_.generalData, 0u);
    }
    {
        // NOTE: should be PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE or current size
        constexpr uint32_t skinSize = sizeof(DefaultMaterialSkinStruct);

        uint32_t bindingIdx = 0u;
        binder1.BindBuffer(bindingIdx++, buffers_.mesh, 0u, PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE);
        binder1.BindBuffer(bindingIdx++, buffers_.skinJoint, 0u, skinSize);
    }
    const RenderHandle handles[] { binder0.GetDescriptorSetHandle(), binder1.GetDescriptorSetHandle() };
    const DescriptorSetLayoutBindingResources resources[] { binder0.GetDescriptorSetLayoutBindingResources(),
        binder1.GetDescriptorSetLayoutBindingResources() };
    cmdList.UpdateDescriptorSets(handles, resources);
}

RenderHandle RenderNodeDefaultDepthRenderSlot::GetSubmeshPso(const ShaderStateData& ssd,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags, const RenderSubmeshFlags submeshFlags)
{
    if (const auto dataIter = allShaderData_.shaderIdToData.find(ssd.hash);
        dataIter != allShaderData_.shaderIdToData.cend()) {
        const auto& ref = allShaderData_.perShaderData[dataIter->second];
        return ref.psoHandle;
    }
    return CreateNewPso(ssd, submeshMaterialFlags, submeshFlags);
}

void RenderNodeDefaultDepthRenderSlot::UpdateCurrentScene(
    const IRenderDataStoreDefaultScene& dataStoreScene, const IRenderDataStoreDefaultCamera& dataStoreCamera)
{
    if (jsonInputs_.hasChangeableRenderPassHandles) {
        const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
        inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
        rngRenderPass_ = renderNodeContextMgr_->GetRenderNodeUtil().CreateRenderPass(inputRenderPass_);
    }
    // get default RNG based render pass setup
    renderPass_ = rngRenderPass_;

    const auto scene = dataStoreScene.GetScene();
    bool hasCustomCamera = false;
    bool isNamedCamera = false; // NOTE: legacy support will be removed
    uint32_t cameraIdx = scene.cameraIndex;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraId);
        hasCustomCamera = true;
    } else if (!(jsonInputs_.customCameraName.empty())) {
        cameraIdx = dataStoreCamera.GetCameraIndex(jsonInputs_.customCameraName);
        hasCustomCamera = true;
        isNamedCamera = true;
    }

    if (const auto cameras = dataStoreCamera.GetCameras(); cameraIdx < (uint32_t)cameras.size()) {
        // store current frame camera
        currentScene_.camera = cameras[cameraIdx];
    }

    // renderpass needs to be valid (created in init)
    if (hasCustomCamera) {
        // uses camera based loadOp clear override if given
        RenderNodeSceneUtil::UpdateRenderPassFromCustomCamera(currentScene_.camera, isNamedCamera, renderPass_);
    } else {
        RenderNodeSceneUtil::UpdateRenderPassFromCamera(currentScene_.camera, renderPass_);
    }
    currentScene_.viewportDesc = RenderNodeSceneUtil::CreateViewportFromCamera(currentScene_.camera);
    currentScene_.scissorDesc = RenderNodeSceneUtil::CreateScissorFromCamera(currentScene_.camera);
    currentScene_.cameraIdx = cameraIdx;
}

void RenderNodeDefaultDepthRenderSlot::CreateDefaultShaderData()
{
    allShaderData_ = {};

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    allShaderData_.defaultPlHandle =
        shaderMgr.GetPipelineLayoutHandle(DefaultMaterialShaderConstants::PIPELINE_LAYOUT_DEPTH);
    allShaderData_.defaultPipelineLayout = shaderMgr.GetPipelineLayout(allShaderData_.defaultPlHandle);
    allShaderData_.defaultVidHandle =
        shaderMgr.GetVertexInputDeclarationHandle(DefaultMaterialShaderConstants::VERTEX_INPUT_DECLARATION_DEPTH);

    {
        // get the default material shader and default shader state
        const IShaderManager::RenderSlotData shaderRsd = shaderMgr.GetRenderSlotData(jsonInputs_.shaderRenderSlotId);
        allShaderData_.defaultShaderHandle = shaderRsd.shader.GetHandle();
        allShaderData_.defaultStateHandle = shaderRsd.graphicsState.GetHandle();
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
            CORE_LOG_I("RenderNode: %s, no default shaders for render slot id %u",
                renderNodeContextMgr_->GetName().data(), jsonInputs_.shaderRenderSlotId);
        }
        if (jsonInputs_.shaderRenderSlotId != jsonInputs_.stateRenderSlotId) {
            const IShaderManager::RenderSlotData stateRsd = shaderMgr.GetRenderSlotData(jsonInputs_.stateRenderSlotId);
            if (stateRsd.graphicsState) {
                allShaderData_.defaultStateHandle = stateRsd.graphicsState.GetHandle();
            } else {
                CORE_LOG_I("RenderNode: %s, no default state for render slot id %u",
                    renderNodeContextMgr_->GetName().data(), jsonInputs_.stateRenderSlotId);
            }
        }
    }
}

void RenderNodeDefaultDepthRenderSlot::GetSceneUniformBuffers(const string_view us)
{
    const auto& gpuMgr = renderNodeContextMgr_->GetGpuResourceManager();
    buffers_.camera = gpuMgr.GetBufferHandle(us + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);
    buffers_.mesh = gpuMgr.GetBufferHandle(us + DefaultMaterialMaterialConstants::MESH_DATA_BUFFER_NAME);
    buffers_.skinJoint = gpuMgr.GetBufferHandle(us + DefaultMaterialMaterialConstants::SKIN_DATA_BUFFER_NAME);

    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(buffers_.defaultBuffer, valid, buffers_.camera);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.mesh);
    checkValidity(buffers_.defaultBuffer, valid, buffers_.skinJoint);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all scene buffers not found.", renderNodeContextMgr_->GetName().data());
    }
}

void RenderNodeDefaultDepthRenderSlot::GetCameraUniformBuffers(const string_view us)
{
    string camName;
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(jsonInputs_.customCameraId);
    } else if (!(jsonInputs_.customCameraName.empty())) {
        camName = jsonInputs_.customCameraName;
    }
    const auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    buffers_.generalData = gpuResourceMgr.GetBufferHandle(
        us + DefaultMaterialCameraConstants::CAMERA_GENERAL_BUFFER_PREFIX_NAME + camName);

    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(buffers_.defaultBuffer, valid, buffers_.generalData);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all camera buffers found.", renderNodeContextMgr_->GetName().data());
    }
}

RenderHandle RenderNodeDefaultDepthRenderSlot::CreateNewPso(const ShaderStateData& ssd,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags, const RenderSubmeshFlags submeshFlags)
{
    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    // NOTE: The easiest route would be to input shader and graphics state to material component
    RenderHandle currShader;
    RenderHandle currPl;
    RenderHandle currVid;
    RenderHandle currState;
    // first try to find matching shader
    if (RenderHandleUtil::GetHandleType(ssd.shader) == RenderHandleType::SHADER_STATE_OBJECT) {
        // we force the given shader if explicit shader render slot is not given
        if (!jsonInputs_.explicitShader) {
            currShader = ssd.shader;
        }
        const RenderHandle slotShader = shaderMgr.GetShaderHandle(ssd.shader, jsonInputs_.shaderRenderSlotId);
        if (RenderHandleUtil::IsValid(slotShader)) {
            currShader = slotShader; // override with render slot variant
        }
        // if not explicit gfx state given, check if shader has graphics state for this slot
        if (!RenderHandleUtil::IsValid(ssd.gfxState)) {
            const auto gfxStateHandle = shaderMgr.GetGraphicsStateHandleByShaderHandle(currShader);
            if (shaderMgr.GetRenderSlotId(gfxStateHandle) == jsonInputs_.stateRenderSlotId) {
                currState = gfxStateHandle;
            }
        }
        currVid = shaderMgr.GetVertexInputDeclarationHandleByShaderHandle(currShader);
        currPl = shaderMgr.GetPipelineLayoutHandleByShaderHandle(currShader);
    }
    if (RenderHandleUtil::GetHandleType(ssd.gfxState) == RenderHandleType::GRAPHICS_STATE) {
        const RenderHandle slotState = shaderMgr.GetGraphicsStateHandle(ssd.gfxState, jsonInputs_.stateRenderSlotId);
        if (RenderHandleUtil::IsValid(slotState)) {
            currState = slotState;
        }
    }

    // NOTE: the pipeline layout compatibility should be checked

    // fallback to defaults if needed
    currShader = RenderHandleUtil::IsValid(currShader) ? currShader : allShaderData_.defaultShaderHandle;
    currPl = RenderHandleUtil::IsValid(currPl) ? currPl : allShaderData_.defaultPlHandle;
    currVid = RenderHandleUtil::IsValid(currVid) ? currVid : allShaderData_.defaultVidHandle;
    currState = RenderHandleUtil::IsValid(currState) ? currState : allShaderData_.defaultStateHandle;

    auto& psoMgr = renderNodeContextMgr_->GetPsoManager();
    RenderHandle psoHandle;
    constexpr array_view<const DynamicStateEnum> dynamicStatesView { DYNAMIC_STATES, countof(DYNAMIC_STATES) };
    if (IsInverseWinding(submeshFlags, jsonInputs_.nodeFlags, currentScene_.camera.flags)) {
        // we create a new graphics state with inverse winding
        GraphicsState gfxState = shaderMgr.GetGraphicsState(currState);
        gfxState.rasterizationState.frontFace = FrontFace::CORE_FRONT_FACE_CLOCKWISE;
        const auto spec = GetShaderSpecializationView(gfxState, submeshMaterialFlags, submeshFlags);
        psoHandle = psoMgr.GetGraphicsPsoHandle(currShader, gfxState, shaderMgr.GetPipelineLayout(currPl),
            shaderMgr.GetVertexInputDeclarationView(currVid), spec, dynamicStatesView);
    } else {
        const GraphicsState& gfxState = shaderMgr.GetGraphicsState(currState);
        const auto spec = GetShaderSpecializationView(gfxState, submeshMaterialFlags, submeshFlags);
        psoHandle = psoMgr.GetGraphicsPsoHandle(currShader, currState, currPl, currVid, spec, dynamicStatesView);
    }

    allShaderData_.perShaderData.push_back(PerShaderData { currShader, psoHandle, currState });
    allShaderData_.shaderIdToData[ssd.hash] = (uint32_t)allShaderData_.perShaderData.size() - 1;
    return psoHandle;
}

ShaderSpecializationConstantDataView RenderNodeDefaultDepthRenderSlot::GetShaderSpecializationView(
    const RENDER_NS::GraphicsState& gfxState,
    const RenderDataDefaultMaterial::SubmeshMaterialFlags& submeshMaterialFlags, const RenderSubmeshFlags submeshFlags)
{
    for (uint32_t idx = 0; idx < specializationData_.maxSpecializationCount; ++idx) {
        const auto& ref = allShaderData_.defaultSpecilizationConstants[idx];
        const uint32_t constantId = ref.offset / sizeof(uint32_t);

        if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_VERTEX_BIT) {
            if (ref.id == 0u) {
                specializationData_.flags[constantId] = submeshFlags;
            } else if (ref.id == 1u) {
                specializationData_.flags[constantId] = submeshMaterialFlags.renderMaterialFlags;
            }
        } else if (ref.shaderStage == ShaderStageFlagBits::CORE_SHADER_STAGE_FRAGMENT_BIT) {
            if (ref.id == 0u) {
                specializationData_.flags[constantId] = static_cast<uint32_t>(submeshMaterialFlags.materialType);
            }
        }
    }

    return { { allShaderData_.defaultSpecilizationConstants.data(), specializationData_.maxSpecializationCount },
        { specializationData_.flags, specializationData_.maxSpecializationCount } };
}

void RenderNodeDefaultDepthRenderSlot::ProcessBuffersAndDescriptors(const ObjectCounts& objectCounts)
{
    constexpr uint32_t overEstimate { 16u };
    bool updateDescriptorSets = false;
    if (objectCounts_.maxSlotMeshCount < objectCounts.maxSlotMeshCount) {
        updateDescriptorSets = true;
        objectCounts_.maxSlotMeshCount = objectCounts.maxSlotMeshCount + (objectCounts.maxSlotMeshCount / overEstimate);
    }
    if (objectCounts_.maxSlotSkinCount < objectCounts.maxSlotSkinCount) {
        updateDescriptorSets = true;
        objectCounts_.maxSlotSkinCount = objectCounts.maxSlotSkinCount + (objectCounts.maxSlotSkinCount / overEstimate);
    }
    if (objectCounts_.maxSlotSubmeshCount < objectCounts.maxSlotSubmeshCount) {
        updateDescriptorSets = true;
        objectCounts_.maxSlotSubmeshCount =
            objectCounts.maxSlotSubmeshCount + (objectCounts.maxSlotSubmeshCount / overEstimate);
    }

    if (updateDescriptorSets) {
        ResetAndUpdateDescriptorSets();
    }
}

void RenderNodeDefaultDepthRenderSlot::ResetAndUpdateDescriptorSets()
{
    INodeContextDescriptorSetManager& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
    // known and calculated (could run through descriptorsets and calculate automatically as well)
    const DescriptorCounts dc { {
        // camera + general data
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 2u },
        // mesh and skin data
        { CORE_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 2u },
    } };
    descriptorSetMgr.ResetAndReserve(dc);

    for (uint32_t setIdx = 0; setIdx < AllDescriptorSets::SINGLE_SET_COUNT; ++setIdx) {
        const RenderHandle descriptorSetHandle =
            descriptorSetMgr.CreateDescriptorSet(setIdx, allShaderData_.defaultPipelineLayout);
        allDescriptorSets_.set01[setIdx] = descriptorSetMgr.CreateDescriptorSetBinder(
            descriptorSetHandle, allShaderData_.defaultPipelineLayout.descriptorSetLayouts[setIdx].bindings);
    }
}

void RenderNodeDefaultDepthRenderSlot::ProcessSlotSubmeshes(
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultMaterial& dataStoreMaterial)
{
    // currentScene has been updated prior, has the correct camera (scene camera or custom camera)
    const IRenderNodeSceneUtil::RenderSlotInfo rsi { jsonInputs_.renderSlotId, jsonInputs_.sortType,
        jsonInputs_.cullType, jsonInputs_.nodeMaterialDiscardFlags };
    RenderNodeSceneUtil::GetRenderSlotSubmeshes(
        dataStoreCamera, dataStoreMaterial, currentScene_.cameraIdx, rsi, sortedSlotSubmeshes_);
}

void RenderNodeDefaultDepthRenderSlot::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.renderPass = parserUtil.GetInputRenderPass(jsonVal, "renderPass");
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
    jsonInputs_.renderDataStore = parserUtil.GetRenderDataStore(jsonVal, "renderDataStore");

    jsonInputs_.sortType = parserUtil.GetRenderSlotSortType(jsonVal, "renderSlotSortType");
    jsonInputs_.cullType = parserUtil.GetRenderSlotCullType(jsonVal, "renderSlotCullType");
    jsonInputs_.nodeFlags = static_cast<uint32_t>(parserUtil.GetUintValue(jsonVal, "nodeFlags"));
    if (jsonInputs_.nodeFlags == ~0u) {
        jsonInputs_.nodeFlags = 0;
    }
    jsonInputs_.nodeMaterialDiscardFlags =
        static_cast<uint32_t>(parserUtil.GetUintValue(jsonVal, "nodeMaterialDiscardFlags"));
    if (jsonInputs_.nodeMaterialDiscardFlags == ~0u) {
        jsonInputs_.nodeMaterialDiscardFlags = 0;
    }
    // automatic default material velocity named target based parsing to add velocity calculations bit
    for (const auto& ref : jsonInputs_.renderPass.attachments) {
        if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_VELOCITY_NORMAL) {
            jsonInputs_.nodeSubmeshExtraFlags |= RenderSubmeshFlagBits::RENDER_SUBMESH_VELOCITY_BIT;
        }
    }

    const auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();
    const string renderSlot = parserUtil.GetStringValue(jsonVal, "renderSlot");
    jsonInputs_.renderSlotId = shaderMgr.GetRenderSlotId(renderSlot);
    jsonInputs_.shaderRenderSlotId = jsonInputs_.renderSlotId;
    jsonInputs_.stateRenderSlotId = jsonInputs_.renderSlotId;
    const string shaderRenderSlot = parserUtil.GetStringValue(jsonVal, "shaderRenderSlot");
    if (!shaderRenderSlot.empty()) {
        const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(shaderRenderSlot);
        if (renderSlotId != ~0U) {
            jsonInputs_.shaderRenderSlotId = renderSlotId;
            jsonInputs_.explicitShader = true;
        }
    }
    const string stateRenderSlot = parserUtil.GetStringValue(jsonVal, "stateRenderSlot");
    if (!stateRenderSlot.empty()) {
        const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(stateRenderSlot);
        jsonInputs_.stateRenderSlotId = (renderSlotId != ~0U) ? renderSlotId : jsonInputs_.renderSlotId;
    }

    const auto& renderNodeUtil = renderNodeContextMgr_->GetRenderNodeUtil();
    inputRenderPass_ = renderNodeUtil.CreateInputRenderPass(jsonInputs_.renderPass);
    jsonInputs_.hasChangeableRenderPassHandles = renderNodeUtil.HasChangeableResources(jsonInputs_.renderPass);
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultDepthRenderSlot::Create()
{
    return new RenderNodeDefaultDepthRenderSlot();
}

void RenderNodeDefaultDepthRenderSlot::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultDepthRenderSlot*>(instance);
}
CORE3D_END_NAMESPACE()
