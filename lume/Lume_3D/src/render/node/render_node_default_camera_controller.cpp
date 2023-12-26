/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "render_node_default_camera_controller.h"

#if (CORE3D_VALIDATION_ENABLED == 1)
#include <cinttypes>
#include <string>
#endif

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/containers/string.h>
#include <base/math/matrix_util.h>
#include <core/log.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/render_data_structures.h>

namespace {
#include <render/shaders/common/render_post_process_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr float CUBE_MAP_LOD_COEFF { 8.0f };
constexpr string_view POD_DATA_STORE_NAME { "RenderDataStorePod" };

void ValidateRenderCamera(RenderCamera& camera)
{
    if (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
        if (camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_MSAA_BIT) {
            camera.flags = camera.flags & (~RenderCamera::CameraFlagBits::CAMERA_FLAG_MSAA_BIT);
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_I("valid_r_c_" + to_string(camera.id),
                "MSAA flag with deferred pipeline dropped (cam id %" PRIu64 ")", camera.id);
#endif
        }
    }
}

struct CreatedTargetHandles {
    RenderHandle color;
    RenderHandle depth;
    RenderHandle colorResolve;
    RenderHandle colorMsaa;
    RenderHandle depthMsaa;
    RenderHandle baseColor;
    RenderHandle velNor;
    RenderHandle material;
};

CreatedTargetHandles FillCreatedTargets(const RenderNodeDefaultCameraController::CreatedTargets& targets)
{
    return CreatedTargetHandles {
        targets.color.GetHandle(),
        targets.depth.GetHandle(),
        targets.colorResolve.GetHandle(),
        targets.colorMsaa.GetHandle(),
        targets.depthMsaa.GetHandle(),
        targets.baseColor.GetHandle(),
        targets.velocityNormal.GetHandle(),
        targets.material.GetHandle(),
    };
}

inline constexpr Math::UVec2 GetPacked64(const uint64_t value)
{
    return { static_cast<uint32_t>(value >> 32) & 0xFFFFffff, static_cast<uint32_t>(value & 0xFFFFffff) };
}

inline constexpr Format GetInitialHDRColorFormat(const Format format)
{
    return (format == Format::BASE_FORMAT_UNDEFINED) ? Format::BASE_FORMAT_R16G16B16A16_SFLOAT : format;
}

inline constexpr Format GetInitialHDRDepthFormat(const Format format)
{
    return (format == Format::BASE_FORMAT_UNDEFINED) ? Format::BASE_FORMAT_D32_SFLOAT : format;
}

Format GetValidColorFormat(const IRenderNodeGpuResourceManager& gpuResourceMgr, const Format format)
{
    Format outFormat = format;
    const auto formatProperties = gpuResourceMgr.GetFormatProperties(format);
    if (((formatProperties.optimalTilingFeatures & CORE_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0) ||
        ((formatProperties.optimalTilingFeatures & CORE_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)) {
        outFormat = Format::BASE_FORMAT_R8G8B8A8_SRGB;
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_W("CORE_VALIDATION: not supported camera color format %u, using BASE_FORMAT_R8G8B8A8_SRGB", outFormat);
#endif
    }
    return outFormat;
}

Format GetValidDepthFormat(const IRenderNodeGpuResourceManager& gpuResourceMgr, const Format format)
{
    Format outFormat = format;
    const auto formatProperties = gpuResourceMgr.GetFormatProperties(format);
    if ((formatProperties.optimalTilingFeatures & CORE_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) == 0) {
        outFormat = Format::BASE_FORMAT_D32_SFLOAT;
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_W("CORE_VALIDATION: not supported camera depth format %u, using BASE_FORMAT_D32_SFLOAT", outFormat);
#endif
    }
    return outFormat;
}

void CreateBaseColorTarget(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const string_view customCameraRngId, RenderNodeDefaultCameraController::CreatedTargets& targets)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    CORE_LOG_I("CORE3D_VALIDATION: creating camera base color target %s", customCameraRngId.data());
#endif
    Format format = Format::BASE_FORMAT_R8G8B8A8_SRGB;
    uint32_t mipCount = 1u;
    if (camera.flags & RenderCamera::CAMERA_FLAG_COLOR_PRE_PASS_BIT) {
        format = Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32;
        mipCount = 6u; // NOTE: hard-coded pre pass mip count
    }
    const GpuImageDesc desc {
        ImageType::CORE_IMAGE_TYPE_2D,
        ImageViewType::CORE_IMAGE_VIEW_TYPE_2D,
        format,
        ImageTiling::CORE_IMAGE_TILING_OPTIMAL,
        ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT |
            ImageUsageFlagBits::CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        0,
        EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS,
        camera.renderResolution.x,
        camera.renderResolution.y,
        1u,
        mipCount,
        1u,
        SampleCountFlagBits::CORE_SAMPLE_COUNT_1_BIT,
        ComponentMapping {},
    };
    targets.color =
        gpuResourceMgr.Create(DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + customCameraRngId, desc);
}

void CreateVelocityTarget(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
    const bool createVelocity = (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) ||
                                (camera.renderPipelineType == RenderCamera::RenderPipelineType::FORWARD);
    if (createVelocity) {
        GpuImageDesc desc = targetDesc;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
        desc.format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (camera.flags & RenderCamera::CAMERA_FLAG_OUTPUT_VELOCITY_NORMAL_BIT) {
            desc.usageFlags |= CORE_IMAGE_USAGE_SAMPLED_BIT;
        } else {
            desc.usageFlags |= CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            desc.memoryPropertyFlags |= CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        }
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.velocityNormal = gpuResourceMgr.Create(
            DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "VEL_NOR" + customCameraRngId, desc);
#else
        createdTargets.velocityNormal = gpuResourceMgr.Create(createdTargets.velocityNormal, desc);
#endif
    }
}

void CreateDeferredTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
    if (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
        GpuImageDesc desc = targetDesc;
        desc.engineCreationFlags =
            CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;

        // at the moment, baseColor/albedo and material are always treated as input attachments
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                          CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

        // create base color
        desc.format = Format::BASE_FORMAT_R8G8B8A8_SRGB;
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.baseColor = gpuResourceMgr.Create(
            DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "BC_" + customCameraRngId, desc);
#else
        createdTargets.baseColor = gpuResourceMgr.Create(createdTargets.baseColor, desc);
#endif
        // create material
        desc.format = Format::BASE_FORMAT_R8G8B8A8_UNORM;
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.material = gpuResourceMgr.Create(
            DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "MAT_" + customCameraRngId, desc);
#else
        createdTargets.material = gpuResourceMgr.Create(createdTargets.material, desc);
#endif
    }
}

void CreateHistoryTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup, const Format colorFormat,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
    if (camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_HISTORY_BIT) {
        GpuImageDesc desc = targetDesc;
        desc.format = colorFormat;
        // sampled from neighbours, cannot be transient
        desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
                          CORE_IMAGE_USAGE_SAMPLED_BIT;
        desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        desc.engineCreationFlags = CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.history[0u] = gpuResourceMgr.Create(
            DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "HIS0_" + customCameraRngId, desc);
        createdTargets.history[1u] = gpuResourceMgr.Create(
            DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "HIS1_" + customCameraRngId, desc);
#else
        createdTargets.history[0u] = gpuResourceMgr.Create(createdTargets.history[0u], desc);
        createdTargets.history[1u] = gpuResourceMgr.Create(createdTargets.history[1u], desc);
#endif
    }
}

void CreateColorTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    CORE_LOG_I("CORE3D_VALIDATION: creating camera color targets %s", customCameraRngId.data());
#endif
    // This (re-)creates all the needed color targets
    const bool enableHdr = (camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD);
    Format colorFormat = targetDesc.format;
    if (enableHdr || (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT)) {
        Format hdrFormat = (cameraResourceSetup.hdrColorFormat == BASE_FORMAT_UNDEFINED)
                               ? BASE_FORMAT_R16G16B16A16_SFLOAT
                               : cameraResourceSetup.hdrColorFormat;
        // correct queries only available in Vulkan
        if (cameraResourceSetup.backendType == DeviceBackendType::VULKAN) {
            hdrFormat = GetValidColorFormat(gpuResourceMgr, hdrFormat);
        }
        GpuImageDesc desc = targetDesc;
        // only when HDR pipeline enabled we can enable render resolution
        if (enableHdr) {
            desc.width = cameraResourceSetup.renResolution.x;
            desc.height = cameraResourceSetup.renResolution.y;
        }
        colorFormat = enableHdr ? hdrFormat : targetDesc.format;
        desc.format = colorFormat;
        desc.engineCreationFlags =
            CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;

        if (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) {
            desc.sampleCountFlags = CORE_SAMPLE_COUNT_4_BIT;
            desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            desc.memoryPropertyFlags =
                CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

#if (CORE3D_VALIDATION_ENABLED == 1)
            createdTargets.colorMsaa = gpuResourceMgr.Create(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "MSAA_" + customCameraRngId, desc);
#else
            createdTargets.colorMsaa = gpuResourceMgr.Create(createdTargets.colorMsaa, desc);
#endif
        }

        if (enableHdr) {
            desc.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
            desc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT;
            desc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

#if (CORE3D_VALIDATION_ENABLED == 1)
            createdTargets.colorResolve = gpuResourceMgr.Create(
                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "RESL_" + customCameraRngId, desc);
#else
            createdTargets.colorResolve = gpuResourceMgr.Create(createdTargets.colorResolve, desc);
#endif
        }
    }

    CreateVelocityTarget(gpuResourceMgr, camera, targetDesc, customCameraRngId, cameraResourceSetup, createdTargets);
    CreateDeferredTargets(gpuResourceMgr, camera, targetDesc, customCameraRngId, cameraResourceSetup, createdTargets);

    CreateHistoryTargets(
        gpuResourceMgr, camera, targetDesc, customCameraRngId, cameraResourceSetup, colorFormat, createdTargets);
}

void CreateDepthTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    CORE_LOG_I("CORE3D_VALIDATION: creating camera depth targets %s", customCameraRngId.data());
#endif
    // this (re-)creates all the needed depth targets
    // we support cameras without depth targets (default depth is created if no msaa)
    const bool enableHdr = (camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD);
    Format hdrFormat = (cameraResourceSetup.hdrDepthFormat == BASE_FORMAT_UNDEFINED)
                           ? BASE_FORMAT_D32_SFLOAT
                           : cameraResourceSetup.hdrDepthFormat;
    // correct queries only available in Vulkan
    if (cameraResourceSetup.backendType == DeviceBackendType::VULKAN) {
        hdrFormat = GetValidDepthFormat(gpuResourceMgr, hdrFormat);
    }
    GpuImageDesc depthDesc = targetDesc;
    // only when HDR pipeline enabled we can enable render resolution
    if (enableHdr) {
        depthDesc.width = cameraResourceSetup.renResolution.x;
        depthDesc.height = cameraResourceSetup.renResolution.y;
    }
    depthDesc.format = enableHdr ? hdrFormat : targetDesc.format;
    depthDesc.engineCreationFlags =
        CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS | CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
    depthDesc.usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    depthDesc.memoryPropertyFlags = CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    if (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) {
        // always transient
        depthDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        depthDesc.memoryPropertyFlags |= CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        depthDesc.sampleCountFlags = CORE_SAMPLE_COUNT_4_BIT;
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.depthMsaa = gpuResourceMgr.Create(
            DefaultMaterialCameraConstants::CAMERA_DEPTH_PREFIX_NAME + "MSAA_" + customCameraRngId, depthDesc);
#else
        createdTargets.depthMsaa = gpuResourceMgr.Create(createdTargets.depthMsaa, depthDesc);
#endif
    } else {
        depthDesc.usageFlags |= CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
        if (camera.flags & RenderCamera::CAMERA_FLAG_OUTPUT_DEPTH_BIT) {
            depthDesc.usageFlags |= CORE_IMAGE_USAGE_SAMPLED_BIT;
        } else {
            depthDesc.usageFlags |= CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
            depthDesc.memoryPropertyFlags |= CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        }
        depthDesc.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
        createdTargets.depth = gpuResourceMgr.Create(
            DefaultMaterialCameraConstants::CAMERA_DEPTH_PREFIX_NAME + customCameraRngId, depthDesc);
    }
}

bool ColorTargetsRecreationNeeded(const RenderCamera& camera,
    const RenderNodeDefaultCameraController::CameraResourceSetup& camRes, const GpuImageDesc& desc)
{
    constexpr RenderCamera::Flags importantFlags { RenderCamera::CAMERA_FLAG_HISTORY_BIT |
                                                   RenderCamera::CAMERA_FLAG_OUTPUT_DEPTH_BIT |
                                                   RenderCamera::CAMERA_FLAG_OUTPUT_VELOCITY_NORMAL_BIT };
    const RenderCamera::Flags newFlags = camera.flags & importantFlags;
    const RenderCamera::Flags oldFlags = camRes.camFlags & importantFlags;
    const bool creationFlagsChanged = newFlags != oldFlags;
    const bool pipelineChanged = (camera.renderPipelineType != camRes.pipelineType);
    const bool formatResChanged = ((desc.format != camRes.colorFormat) || (desc.width != camRes.outResolution.x) ||
                                   (desc.height != camRes.outResolution.y));

    bool changed = false;
    if (creationFlagsChanged || pipelineChanged || formatResChanged) {
        changed = true;
    }
    const Format currColorFormat = GetInitialHDRColorFormat(camera.colorTargetCustomization[0].format);
    if ((camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) &&
        (currColorFormat != camRes.hdrColorFormat)) {
        changed = true;
    }
    return changed;
}

inline bool DepthTargetsRecreationNeeded(const RenderCamera& camera,
    const RenderNodeDefaultCameraController::CameraResourceSetup& camRes, const GpuImageDesc& desc)
{
    bool changed = false;
    const Format currDepthFormat = GetInitialHDRDepthFormat(camera.depthTargetCustomization.format);
    const bool formatResChanged = ((desc.format != camRes.depthFormat) || (desc.width != camRes.outResolution.x) ||
                                   (desc.height != camRes.outResolution.y));
    if (formatResChanged) {
        changed = true;
    }
    if ((camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) &&
        (currDepthFormat != camRes.hdrDepthFormat)) {
        changed = true;
    }
    return changed;
}
} // namespace

void RenderNodeDefaultCameraController::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;
    ParseRenderNodeInputs();

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    currentScene_ = {};
    currentScene_.customCamRngName = jsonInputs_.customCameraName;

    // id checked first for custom camera
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        currentScene_.customCameraId = jsonInputs_.customCameraId;
    } else if (!jsonInputs_.customCameraName.empty()) {
        currentScene_.customCameraName = jsonInputs_.customCameraName;
    }

    camRes_.backendType = renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderBackend;

    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    if (dataStoreScene && dataStoreCamera && dataStoreLight) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);
        CreateResources();
        RegisterOutputs();
        CreateBuffers();
    }
}

void RenderNodeDefaultCameraController::PreExecuteFrame()
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    if (dataStoreScene && dataStoreCamera && dataStoreLight) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);
        CreateResources();
        RegisterOutputs();
    }
}

void RenderNodeDefaultCameraController::ExecuteFrame(IRenderCommandList& cmdList)
{
    UpdateBuffers();
}

void RenderNodeDefaultCameraController::UpdateCurrentScene(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const IRenderDataStoreDefaultLight& dataStoreLight)
{
    const auto scene = dataStoreScene.GetScene();
    uint32_t cameraIdx = scene.cameraIndex;
    if (currentScene_.customCameraId != INVALID_CAM_ID) {
        cameraIdx = dataStoreCamera.GetCameraIndex(currentScene_.customCameraId);
    } else if (!(currentScene_.customCameraName.empty())) {
        cameraIdx = dataStoreCamera.GetCameraIndex(currentScene_.customCameraName);
    }

    if (const auto cameras = dataStoreCamera.GetCameras(); cameraIdx < (uint32_t)cameras.size()) {
        // store current frame camera
        currentScene_.camera = cameras[cameraIdx];
    }

    currentScene_.cameraIdx = cameraIdx;
    currentScene_.sceneTimingData = { scene.sceneDeltaTime, scene.deltaTime, scene.totalTime,
        *reinterpret_cast<const float*>(&scene.frameIndex) };

    ValidateRenderCamera(currentScene_.camera);

    UpdatePostProcessConfiguration();
}

void RenderNodeDefaultCameraController::RegisterOutputs()
{
    if ((currentScene_.customCameraId == INVALID_CAM_ID) && currentScene_.customCameraName.empty()) {
        return;
    }

    const CreatedTargetHandles cth = FillCreatedTargets(createdTargets_);
    const RenderHandle colorTarget = RenderHandleUtil::IsValid(camRes_.colorTarget) ? camRes_.colorTarget : cth.color;
    const RenderHandle depthTarget = RenderHandleUtil::IsValid(camRes_.depthTarget) ? camRes_.depthTarget : cth.depth;

    IRenderNodeGraphShareManager& shrMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
    shrMgr.RegisterRenderNodeGraphOutputs({ &colorTarget, 1u });
    // color is output always, depth usually (not with MSAA if not forced)
    const RenderHandle regDepth = RenderHandleUtil::IsValid(cth.depth) ? cth.depth : depthTarget;
    const RenderHandle regColor = RenderHandleUtil::IsValid(cth.colorResolve) ? cth.colorResolve : colorTarget;
    if (RenderHandleUtil::IsValid(regDepth)) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH, regDepth);
    }
    shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR, regColor);
    if (RenderHandleUtil::IsValid(cth.velNor)) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_VELOCITY_NORMAL, cth.velNor);
    }
    // NOTE: HDR does not have float intermediate target ATM
    if (currentScene_.camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_BASE_COLOR, cth.baseColor);
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_MATERIAL, cth.material);
    } else if (currentScene_.camera.flags & (RenderCamera::CAMERA_FLAG_MSAA_BIT)) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH_MSAA, cth.depthMsaa);
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_MSAA, cth.colorMsaa);
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_RESOLVE,
            RenderHandleUtil::IsValid(cth.colorResolve) ? cth.colorResolve : colorTarget);
    }

    // output history as final target
    if (currentScene_.camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_HISTORY_BIT) {
        const uint32_t currIndex = camRes_.historyFlipFrame;
        const uint32_t nextIndex = (currIndex + 1u) % 2u;
        shrMgr.RegisterRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY, createdTargets_.history[currIndex].GetHandle());
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY_NEXT,
            createdTargets_.history[nextIndex].GetHandle());
        camRes_.historyFlipFrame = nextIndex;
    }
}

void RenderNodeDefaultCameraController::CreateResources()
{
    if ((currentScene_.customCameraId == INVALID_CAM_ID) && currentScene_.customCameraName.empty()) {
        return;
    }

    // NOTE: with CAMERA_FLAG_MAIN_BIT we support rendering to cameras without targets
    // if no depth is given, we create a transient depth
    CreateResourceBaseTargets();

    IRenderNodeGpuResourceManager& gpuResMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto& camera = currentScene_.camera;
    const bool validDepthHandle = RenderHandleUtil::IsValid(camRes_.depthTarget);
    const bool isHdr = (camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD);
    const bool isMsaa = (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT);
    const bool isDeferred = (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED);
    if ((!validDepthHandle) || isMsaa || isHdr || isDeferred) {
        const GpuImageDesc colorDesc = RenderHandleUtil::IsValid(camRes_.colorTarget)
                                           ? gpuResMgr.GetImageDescriptor(camRes_.colorTarget)
                                           : gpuResMgr.GetImageDescriptor(createdTargets_.color.GetHandle());
        const bool colorTargetChanged = ColorTargetsRecreationNeeded(camera, camRes_, colorDesc);

        GpuImageDesc depthDesc;
        if (!validDepthHandle) {
            depthDesc = colorDesc;
            depthDesc.format = BASE_FORMAT_D32_SFLOAT;
        } else {
            depthDesc = gpuResMgr.GetImageDescriptor(camRes_.depthTarget);
        }
        bool depthTargetChanged = DepthTargetsRecreationNeeded(camera, camRes_, depthDesc);
        // depth size needs to match color
        if ((colorDesc.width != depthDesc.width) || (colorDesc.height != depthDesc.height)) {
            depthTargetChanged = true;
            depthDesc.width = colorDesc.width;
            depthDesc.height = colorDesc.height;
        }

        camRes_.outResolution.x = colorDesc.width;
        camRes_.outResolution.y = colorDesc.height;
        camRes_.renResolution = currentScene_.camera.renderResolution;
        camRes_.colorFormat = colorDesc.format;
        camRes_.depthFormat = depthDesc.format;
        camRes_.hdrColorFormat = GetInitialHDRColorFormat(camera.colorTargetCustomization[0].format);
        camRes_.hdrDepthFormat = GetInitialHDRDepthFormat(camera.depthTargetCustomization.format);
        camRes_.camFlags = camera.flags;
        camRes_.pipelineType = camera.renderPipelineType;

        // all decisions are done based on color and depth targets
        if (colorTargetChanged) {
            CreateColorTargets(gpuResMgr, camera, colorDesc, currentScene_.customCamRngName, camRes_, createdTargets_);
        }
        if (depthTargetChanged) {
            CreateDepthTargets(gpuResMgr, camera, depthDesc, currentScene_.customCamRngName, camRes_, createdTargets_);
        }
    }
}

void RenderNodeDefaultCameraController::CreateResourceBaseTargets()
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto& camera = currentScene_.camera;
    camRes_.colorTarget = camera.colorTargets[0].GetHandle();
    camRes_.depthTarget = camera.depthTarget.GetHandle();
    if ((camera.flags & RenderCamera::CAMERA_FLAG_MAIN_BIT) && (!RenderHandleUtil::IsValid(camRes_.colorTarget))) {
        camRes_.colorTarget = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_BACKBUFFER");
        camRes_.depthTarget = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_BACKBUFFER_DEPTH");
    }
    if (!RenderHandleUtil::IsValid(camRes_.colorTarget)) {
        if (createdTargets_.color) {
            const GpuImageDesc colorDesc = gpuResourceMgr.GetImageDescriptor(createdTargets_.color.GetHandle());
            if ((colorDesc.width != camera.renderResolution.x) || (colorDesc.height != camera.renderResolution.y)) {
                CreateBaseColorTarget(gpuResourceMgr, camera, currentScene_.customCamRngName, createdTargets_);
            }
        } else {
            CreateBaseColorTarget(gpuResourceMgr, camera, currentScene_.customCamRngName, createdTargets_);
        }
    }
}

void RenderNodeDefaultCameraController::CreateBuffers()
{
    auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    string camName;
    if (currentScene_.customCameraId != INVALID_CAM_ID) {
        camName = to_string(currentScene_.customCameraId);
    } else if (!(currentScene_.customCameraName.empty())) {
        camName = currentScene_.customCameraName;
    }
    uboHandles_.generalData =
        gpuResourceMgr.Create(DefaultMaterialCameraConstants::CAMERA_GENERAL_BUFFER_PREFIX_NAME + camName,
            GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                static_cast<uint32_t>(sizeof(DefaultMaterialGeneralDataStruct)) });
    uboHandles_.environment =
        gpuResourceMgr.Create(DefaultMaterialCameraConstants::CAMERA_ENVIRONMENT_BUFFER_PREFIX_NAME + camName,
            GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                static_cast<uint32_t>(sizeof(DefaultMaterialEnvironmentStruct)) });
    uboHandles_.fog = gpuResourceMgr.Create(DefaultMaterialCameraConstants::CAMERA_FOG_BUFFER_PREFIX_NAME + camName,
        GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, static_cast<uint32_t>(sizeof(DefaultMaterialFogStruct)) });
    uboHandles_.postProcess = gpuResourceMgr.Create(
        DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_BUFFER_PREFIX_NAME + camName,
        GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, static_cast<uint32_t>(sizeof(GlobalPostProcessStruct)) });
}

void RenderNodeDefaultCameraController::UpdateBuffers()
{
    UpdateGeneralUniformBuffer();
    UpdateEnvironmentUniformBuffer();
    UpdateFogUniformBuffer();
    UpdatePostProcessUniformBuffer();
}

void RenderNodeDefaultCameraController::UpdateGeneralUniformBuffer()
{
    const auto& camera = currentScene_.camera;
    const Math::Vec2 viewportSize = { static_cast<float>(camera.renderResolution.x),
        static_cast<float>(camera.renderResolution.y) };
    DefaultMaterialGeneralDataStruct dataStruct {
        { currentScene_.cameraIdx, 0u, 0u, 0u },
        { viewportSize.x, viewportSize.y, 1.0f / viewportSize.x, 1.0f / viewportSize.y },
        currentScene_.sceneTimingData,
    };

    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboHandles_.generalData.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(DefaultMaterialGeneralDataStruct);
        if (!CloneData(data, size_t(dataEnd - data), &dataStruct, sizeof(DefaultMaterialGeneralDataStruct))) {
            CORE_LOG_E("generalData ubo copying failed.");
        }
        gpuResourceMgr.UnmapBuffer(uboHandles_.generalData.GetHandle());
    }
}

void RenderNodeDefaultCameraController::UpdatePostProcessUniformBuffer()
{
    CORE_STATIC_ASSERT(sizeof(GlobalPostProcessStruct) == sizeof(RenderPostProcessConfiguration));
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboHandles_.postProcess.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(GlobalPostProcessStruct);
        if (!CloneData(data, size_t(dataEnd - data), &currentRenderPPConfiguration_, sizeof(GlobalPostProcessStruct))) {
            CORE_LOG_E("post process ubo copying failed.");
        }
        gpuResourceMgr.UnmapBuffer(uboHandles_.postProcess.GetHandle());
    }
}

void RenderNodeDefaultCameraController::UpdateEnvironmentUniformBuffer()
{
    BASE_NS::Math::Vec4
        defaultSHIndirectCoefficients[9u] {}; // nine vectors which are used in spherical harmonics calculations
    defaultSHIndirectCoefficients[0] = { 1.0f, 1.0f, 1.0f, 1.0f };

    const auto& camera = currentScene_.camera;
    const RenderCamera::Environment& env = camera.environment;
    const float radianceCubemapLodCoeff =
        (env.radianceCubemapMipCount != 0)
            ? Math::min(CUBE_MAP_LOD_COEFF, static_cast<float>(env.radianceCubemapMipCount))
            : CUBE_MAP_LOD_COEFF;
    DefaultMaterialEnvironmentStruct envStruct {
        Math::Vec4(
            (Math::Vec3(env.indirectSpecularFactor) * env.indirectSpecularFactor.w), env.indirectSpecularFactor.w),
        Math::Vec4(Math::Vec3(env.indirectDiffuseFactor) * env.indirectDiffuseFactor.w, env.indirectDiffuseFactor.w),
        Math::Vec4(Math::Vec3(env.envMapFactor) * env.envMapFactor.w, env.envMapFactor.w),
        Math::Vec4(radianceCubemapLodCoeff, env.envMapLodLevel, 0.0f, 0.0f),
        env.additionalFactor,
        Math::Mat4Cast(env.rotation),
        Math::UVec4(
            static_cast<uint32_t>(env.layerMask >> 32u), static_cast<uint32_t>(env.layerMask & 0xFFFFffff), 0, 0),
        {},
    };
    constexpr size_t countOfSh = countof(envStruct.shIndirectCoefficients);
    if (env.radianceCubemap) {
        for (size_t idx = 0; idx < countOfSh; ++idx) {
            envStruct.shIndirectCoefficients[idx] = env.shIndirectCoefficients[idx];
        }
    } else {
        for (size_t idx = 0; idx < countOfSh; ++idx) {
            envStruct.shIndirectCoefficients[idx] = defaultSHIndirectCoefficients[idx];
        }
    }

    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboHandles_.environment.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(DefaultMaterialEnvironmentStruct);
        if (!CloneData(data, size_t(dataEnd - data), &envStruct, sizeof(DefaultMaterialEnvironmentStruct))) {
            CORE_LOG_E("environment ubo copying failed.");
        }
        gpuResourceMgr.UnmapBuffer(uboHandles_.environment.GetHandle());
    }
}

void RenderNodeDefaultCameraController::UpdateFogUniformBuffer()
{
    const auto& camera = currentScene_.camera;
    const RenderCamera::Fog& fog = camera.fog;
    const Math::UVec2 id = GetPacked64(fog.id);
    const Math::UVec2 layer = GetPacked64(fog.id);
    const DefaultMaterialFogStruct fogStruct {
        Math::UVec4 { id.x, id.y, layer.x, layer.y },
        fog.firstLayer,
        fog.secondLayer,
        fog.baseFactors,
        fog.inscatteringColor,
        fog.envMapFactor,
        fog.additionalFactor,
    };
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboHandles_.fog.GetHandle())); data) {
        const auto* dataEnd = data + sizeof(DefaultMaterialFogStruct);
        if (!CloneData(data, size_t(dataEnd - data), &fogStruct, sizeof(DefaultMaterialFogStruct))) {
            CORE_LOG_E("fog ubo copying failed.");
        }
        gpuResourceMgr.UnmapBuffer(uboHandles_.fog.GetHandle());
    }
}

void RenderNodeDefaultCameraController::UpdatePostProcessConfiguration()
{
    const auto& dataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    auto* dataStore = static_cast<IRenderDataStorePod*>(dataStoreMgr.GetRenderDataStore(POD_DATA_STORE_NAME));
    if (!dataStore) {
        return;
    }
    if (dataStore) {
        auto const dataView = dataStore->Get(currentScene_.camera.postProcessName);
        if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
            const PostProcessConfiguration* data = (const PostProcessConfiguration*)dataView.data();
            currentRenderPPConfiguration_ =
                renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(*data);
        }
    }
}

void RenderNodeDefaultCameraController::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultCameraController::Create()
{
    return new RenderNodeDefaultCameraController();
}

void RenderNodeDefaultCameraController::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultCameraController*>(instance);
}
CORE3D_END_NAMESPACE()
