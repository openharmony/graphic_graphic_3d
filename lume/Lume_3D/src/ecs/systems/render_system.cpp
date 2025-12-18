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

#include "render_system.h"

#include <algorithm>

#if (CORE3D_VALIDATION_ENABLED == 1)
#include <cinttypes>
#include <string>
#endif

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/dynamic_environment_blender_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/fog_component.h>
#include <3d/ecs/components/graphics_state_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/post_process_configuration_component.h>
#include <3d/ecs/components/post_process_effect_component.h>
#include <3d/ecs/components/reflection_probe_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_component.h>
#include <3d/ecs/components/skin_component.h>
#include <3d/ecs/components/skin_joints_component.h>
#include <3d/ecs/components/uri_component.h>
#include <3d/ecs/components/world_matrix_component.h>
#include <3d/ecs/systems/intf_node_system.h>
#include <3d/ecs/systems/intf_render_preprocessor_system.h>
#include <3d/ecs/systems/intf_skinning_system.h>
#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/util/intf_mesh_util.h>
#include <3d/util/intf_picking.h>
#include <3d/util/intf_render_util.h>
#include <base/containers/string.h>
#include <base/math/float_packer.h>
#include <base/math/matrix_util.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/implementation_uids.h>
#include <core/intf_engine.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/perf/intf_performance_data_manager.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/property_tools/property_api_impl.inl>
#include <core/property_tools/property_macros.h>
#include <core/util/intf_frustum_util.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/intf_render_data_store_render_post_processes.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/resource_handle.h>

#include "ecs/components/previous_joint_matrices_component.h"
#include "ecs/systems/render_preprocessor_system.h"
#include "util/component_util_functions.h"
#include "util/log.h"
#include "util/mesh_util.h"
#include "util/scene_util.h"
#include "util/uri_lookup.h"

CORE_BEGIN_NAMESPACE()
DECLARE_PROPERTY_TYPE(RENDER_NS::IRenderDataStoreManager*);
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
using namespace RENDER_NS;
using namespace BASE_NS;
using namespace CORE_NS;

namespace {
static const RenderSubmeshWithHandleReference INIT {};
static constexpr uint32_t NEEDS_COLOR_PRE_PASS { 1u << 0u };
static constexpr uint64_t SHADOW_CAMERA_START_UNIQUE_ID { 100 };
#if (CORE3D_VALIDATION_ENABLED == 1)
static constexpr uint32_t MAX_BATCH_SUBMESH_COUNT { 64u };
#endif

// typename for POD data. (e.g. "PostProcess") (core/render/intf_render_data_store_pod.h)
static constexpr string_view POST_PROCESS_NAME { "PostProcess" };
static constexpr string_view POD_DATA_STORE_NAME { "RenderDataStorePod" };
static constexpr string_view PP_DATA_STORE_NAME { "RenderDataStorePostProcess" };
static constexpr string_view RPP_DATA_STORE_NAME { "RenderDataStoreRenderPostProcesses" };

// In addition to the base our renderableQuery has two required components and three optional components:
// (0) RenderMeshComponent
// (1) WorldMatrixComponent
// (2) LayerComponent (optional)
// (3) SkinComponent (optional)
// (4) JointMatrixComponent (optional)
// (5) PreviousJointMatrixComponent (optional)
// (6) NodeComponent (optional)
static constexpr const auto RQ_RMC = 0U;
static constexpr const auto RQ_WM = 1U;
static constexpr const auto RQ_L = 2U;
static constexpr const auto RQ_SM = 3U;
static constexpr const auto RQ_JM = 4U;
static constexpr const auto RQ_PJM = 5U;
static constexpr const auto RQ_N = 6U;

static constexpr const string_view STATE_OPAQUE_NAME { "3dshaderstates://core3d_dm.shadergs" };
static constexpr const string_view STATE_TRANSLUCENT_NAME { "3dshaderstates://core3d_dm.shadergs" };
static constexpr const string_view STATE_DEPTH_NAME { "3dshaderstates://core3d_dm_depth.shadergs" };

static constexpr const string_view DS_OPAQUE_NAME { "OPAQUE_FW_DS" };
static constexpr const string_view DS_TRANSLUCENT_NAME { "TRANSLUCENT_FW_DS" };
static constexpr const string_view DS_DEPTH_NAME { "DEPTH_DS" };

static const MaterialComponent DEF_MATERIAL_COMPONENT {};
static constexpr RenderDataDefaultMaterial::InputMaterialUniforms DEF_INPUT_MATERIAL_UNIFORMS {};

struct ShaderRenderSlotInfo {
    const string_view renderSlot;
    const string_view graphicsStateName;
    const string_view doubleSidedVariantName;
};

void FillShaderData(IEntityManager& em, IUriComponentManager& uriManager,
    IRenderHandleComponentManager& renderHandleMgr, const IShaderManager& shaderMgr, const ShaderRenderSlotInfo srsi,
    RenderSystem::DefaultMaterialShaderData::SingleShaderData& shaderData)
{
    const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(srsi.renderSlot);
    const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(renderSlotId);
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (!rsd.shader) {
        CORE_LOG_W(
            "CORE3D_VALIDATION: Default material render slot shader not found (slot:%s)", srsi.renderSlot.data());
    }
    if (!rsd.graphicsState) {
        CORE_LOG_W("CORE3D_VALIDATION: Default material render slot graphics state not found (slot:%s)",
            srsi.renderSlot.data());
    }
#endif

    auto uri = "3dshaders://" + srsi.renderSlot;
    auto resourceEntity = GetOrCreateEntityReference(renderHandleMgr.GetEcs(), rsd.shader);
    uriManager.Create(resourceEntity);
    uriManager.Write(resourceEntity)->uri = uri;
    shaderData.shader = BASE_NS::move(resourceEntity);

    if (rsd.graphicsState) {
        uri = "3dshaderstates://";
        uri += srsi.renderSlot;

        resourceEntity = GetOrCreateEntityReference(renderHandleMgr.GetEcs(), rsd.graphicsState);
        uriManager.Create(resourceEntity);
        uriManager.Write(resourceEntity)->uri = uri;
        shaderData.gfxState = BASE_NS::move(resourceEntity);

        // fetch double sided mode (no culling gfx state)
        auto handlDbl = shaderMgr.GetGraphicsStateHandle(srsi.graphicsStateName, srsi.doubleSidedVariantName);
        if (handlDbl) {
            resourceEntity = GetOrCreateEntityReference(renderHandleMgr.GetEcs(), handlDbl);
            uri += "_DBL";
            uriManager.Create(resourceEntity);
            uriManager.Write(resourceEntity)->uri = uri;
            shaderData.gfxStateDoubleSided = BASE_NS::move(resourceEntity);
        }
    }
}

constexpr GpuImageDesc CreateReflectionPlaneGpuImageDesc(bool depthImage)
{
    GpuImageDesc desc;
    desc.depth = 1;
    desc.format = depthImage ? Format::BASE_FORMAT_D16_UNORM : Format::BASE_FORMAT_B10G11R11_UFLOAT_PACK32;
    desc.memoryPropertyFlags =
        depthImage ? MemoryPropertyFlags(MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT |
                                         MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
                   : MemoryPropertyFlags(MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    desc.usageFlags = depthImage ? ImageUsageFlags(ImageUsageFlagBits::CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
                                                   ImageUsageFlagBits::CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
                                 : ImageUsageFlags(ImageUsageFlagBits::CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                   ImageUsageFlagBits::CORE_IMAGE_USAGE_SAMPLED_BIT);
    desc.imageType = ImageType::CORE_IMAGE_TYPE_2D;
    desc.imageTiling = ImageTiling::CORE_IMAGE_TILING_OPTIMAL;
    desc.imageViewType = ImageViewType::CORE_IMAGE_VIEW_TYPE_2D;
    desc.engineCreationFlags = EngineImageCreationFlagBits::CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS;
    return desc;
}

constexpr uint32_t GetRenderCameraFlagsFromComponentFlags(const uint32_t pipelineFlags)
{
    uint32_t flags = 0;
    if (pipelineFlags & CameraComponent::PipelineFlagBits::MSAA_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_MSAA_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::CLEAR_DEPTH_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_CLEAR_DEPTH_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::CLEAR_COLOR_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_CLEAR_COLOR_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::HISTORY_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_HISTORY_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::JITTER_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_JITTER_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::VELOCITY_OUTPUT_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_OUTPUT_VELOCITY_NORMAL_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::DEPTH_OUTPUT_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_OUTPUT_DEPTH_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::MULTI_VIEW_ONLY_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_MULTI_VIEW_ONLY_BIT;
    }
    if ((pipelineFlags & CameraComponent::PipelineFlagBits::DISALLOW_REFLECTION_BIT) == 0U) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_ALLOW_REFLECTION_BIT;
    }
    if (pipelineFlags & CameraComponent::PipelineFlagBits::CUBEMAP_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_CUBEMAP_BIT;
    }
    // NOTE: color pre pass bit is not currently enabled from here
    return flags;
}

constexpr inline RenderCamera::CameraCullType GetRenderCameraCullTypeFromComponent(
    const CameraComponent::Culling cameraCullType)
{
    RenderCamera::CameraCullType cullType(RenderCamera::CameraCullType::CAMERA_CULL_NONE);
    if (cameraCullType == CameraComponent::Culling::VIEW_FRUSTUM) {
        cullType = RenderCamera::CameraCullType::CAMERA_CULL_VIEW_FRUSTUM;
    }
    return cullType;
}

constexpr inline RenderCamera::RenderPipelineType GetRenderCameraRenderPipelineTypeFromComponent(
    const CameraComponent::RenderingPipeline cameraRenderPipelineType)
{
    RenderCamera::RenderPipelineType pipelineType(RenderCamera::RenderPipelineType::FORWARD);
    if (cameraRenderPipelineType == CameraComponent::RenderingPipeline::LIGHT_FORWARD) {
        pipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
    } else if (cameraRenderPipelineType == CameraComponent::RenderingPipeline::FORWARD) {
        pipelineType = RenderCamera::RenderPipelineType::FORWARD;
    } else if (cameraRenderPipelineType == CameraComponent::RenderingPipeline::DEFERRED) {
        pipelineType = RenderCamera::RenderPipelineType::DEFERRED;
    } else if (cameraRenderPipelineType == CameraComponent::RenderingPipeline::CUSTOM) {
        pipelineType = RenderCamera::RenderPipelineType::CUSTOM;
    }
    return pipelineType;
}

void ValidateRenderCamera(RenderCamera& camera)
{
    if (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
        if (camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_MSAA_BIT) {
            camera.flags = camera.flags & (~RenderCamera::CameraFlagBits::CAMERA_FLAG_MSAA_BIT);
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_I("valid_r_c" + to_string(camera.id),
                "MSAA flag with deferred pipeline dropped (cam id %" PRIu64 ")", camera.id);
#endif
        }
    }
}

fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> GetPostProcessName(
    const INameComponentManager* nameMgr, const string_view sceneName, const Entity& entity, bool hasEffectComponent)
{
    if (nameMgr) {
        if (ScopedHandle<const NameComponent> nameHandle = nameMgr->Read(entity);
            nameHandle && (!nameHandle->name.empty())) {
            return fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> { nameHandle->name };
        } else {
            // checks if any of the post process mgrs has valid entity for camera
            fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> ret =
                hasEffectComponent ? DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_EFFECT_PREFIX_NAME
                                   : DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_PREFIX_NAME;

            ret.append(sceneName);
            ret.append(to_hex(entity.id));
            return ret;
        }
    }
    return hasEffectComponent ? DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_EFFECT_PREFIX_NAME
                              : DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_PREFIX_NAME;
}

fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> GetPostProcessName(
    const IPostProcessComponentManager* postProcessMgr,
    const IPostProcessConfigurationComponentManager* postProcessConfigMgr,
    const IPostProcessEffectComponentManager* postProcessEffectMgr, const INameComponentManager* nameMgr,
    const string_view sceneName, const Entity& entity)
{
    const bool hasEffectComponent = (postProcessEffectMgr && postProcessEffectMgr->HasComponent(entity));
    if (hasEffectComponent || (postProcessMgr && postProcessMgr->HasComponent(entity)) ||
        (postProcessConfigMgr && postProcessConfigMgr->HasComponent(entity))) {
        return GetPostProcessName(nameMgr, sceneName, entity, hasEffectComponent);
    }
    return DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_PREFIX_NAME;
}

string GetPostProcessRenderNodeGraph(
    const IPostProcessConfigurationComponentManager* postProcessConfigMgr, const Entity& entity)
{
    if (postProcessConfigMgr) {
        if (const auto handle = postProcessConfigMgr->Read(entity); handle) {
            return handle->customRenderNodeGraphFile;
        }
    }
    return {};
}

inline fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> GetCameraName(
    INameComponentManager& nameMgr, const Entity& entity)
{
    fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> ret;
    if (ScopedHandle<const NameComponent> nameHandle = nameMgr.Read(entity);
        nameHandle && (!nameHandle->name.empty())) {
        ret.append(nameHandle->name);
    } else {
        ret.append(to_hex(entity.id));
    }
    return ret;
}

// does not update all the variables
void FillRenderCameraBaseFromCameraComponent(const IRenderHandleComponentManager& renderHandleMgr,
    const ICameraComponentManager& cameraMgr, const IGpuResourceManager& gpuResourceMgr, const CameraComponent& cc,
    RenderCamera& renderCamera, const bool checkCustomTargets)
{
    float screenPercentage;
    if (cc.renderingPipeline != CameraComponent::RenderingPipeline::LIGHT_FORWARD) {
        screenPercentage = Math::clamp(cc.screenPercentage, 0.25f, 1.0f);
    } else {
        screenPercentage = 1.0f;
    }

    renderCamera.layerMask = cc.layerMask;
    renderCamera.viewport = { cc.viewport[0u], cc.viewport[1u], cc.viewport[2u], cc.viewport[3u] };
    renderCamera.scissor = { cc.scissor[0u], cc.scissor[1u], cc.scissor[2u], cc.scissor[3u] };

    // if component has a non-zero resolution use it.
    if (cc.renderResolution[0u] && cc.renderResolution[1u]) {
        renderCamera.renderResolution = { static_cast<uint32_t>(
                                              static_cast<float>(cc.renderResolution[0u]) * screenPercentage),
            static_cast<uint32_t>(static_cast<float>(cc.renderResolution[1u]) * screenPercentage) };
    } else {
        // otherwise check if render target is known, either a custom target or default backbuffer for main camera.
        RenderHandleReference target;
        if (checkCustomTargets && !cc.customColorTargets.empty()) {
            target = renderHandleMgr.GetRenderHandleReference(cc.customColorTargets[0]);
        } else if (cc.customDepthTarget) {
            target = renderHandleMgr.GetRenderHandleReference(cc.customDepthTarget);
        } else if (cc.sceneFlags & CameraComponent::MAIN_CAMERA_BIT) {
            target = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_BACKBUFFER");
        }
        if (target) {
            const auto& imageDesc = gpuResourceMgr.GetImageDescriptor(target);
            renderCamera.renderResolution = { imageDesc.width, imageDesc.height };
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_E(to_hex(cameraMgr.GetEcs().GetId()) + "_render_system",
                "CORE3D_VALIDATION: camera render resolution resized to match target %ux%u",
                renderCamera.renderResolution.x, renderCamera.renderResolution.y);
#endif
        }
    }

    renderCamera.screenPercentage = screenPercentage;
    renderCamera.zNear = cc.zNear;
    renderCamera.zFar = cc.zFar;
    renderCamera.flags = GetRenderCameraFlagsFromComponentFlags(cc.pipelineFlags);
    renderCamera.clearDepthStencil = { cc.clearDepthValue, 0u };
    renderCamera.clearColorValues = { cc.clearColorValue.x, cc.clearColorValue.y, cc.clearColorValue.z,
        cc.clearColorValue.w };
    renderCamera.cullType = GetRenderCameraCullTypeFromComponent(cc.culling);
    renderCamera.renderPipelineType = GetRenderCameraRenderPipelineTypeFromComponent(cc.renderingPipeline);
    renderCamera.msaaSampleCountFlags = static_cast<SampleCountFlags>(cc.msaaSampleCount);
    if (cc.customRenderNodeGraph) {
        renderCamera.customRenderNodeGraph = renderHandleMgr.GetRenderHandleReference(cc.customRenderNodeGraph);
    }
    renderCamera.customRenderNodeGraphFile = cc.customRenderNodeGraphFile;
    const uint32_t maxCount = BASE_NS::Math::min(static_cast<uint32_t>(cc.colorTargetCustomization.size()),
        RenderSceneDataConstants::MAX_CAMERA_COLOR_TARGET_COUNT);
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        renderCamera.colorTargetCustomization[idx].format = cc.colorTargetCustomization[idx].format;
        renderCamera.colorTargetCustomization[idx].usageFlags = cc.colorTargetCustomization[idx].usageFlags;
    }
    renderCamera.depthTargetCustomization.format = cc.depthTargetCustomization.format;
    renderCamera.depthTargetCustomization.usageFlags = cc.depthTargetCustomization.usageFlags;

    if (checkCustomTargets && (!cc.customColorTargets.empty() || cc.customDepthTarget)) {
        renderCamera.flags |= RenderCamera::CAMERA_FLAG_CUSTOM_TARGETS_BIT;
        RenderHandleReference customColorTarget;
        if (cc.customColorTargets.size() > 0) {
            customColorTarget = renderHandleMgr.GetRenderHandleReference(cc.customColorTargets[0]);
        }
        RenderHandleReference customDepthTarget = renderHandleMgr.GetRenderHandleReference(cc.customDepthTarget);
        if (customColorTarget.GetHandleType() != RenderHandleType::GPU_IMAGE) {
            CORE_LOG_E("invalid custom render target(s) for camera (%s)", renderCamera.name.c_str());
        }
        renderCamera.depthTarget = move(customDepthTarget);
        renderCamera.colorTargets[0u] = move(customColorTarget);
    }

    const uint32_t maxMvCount = Math::min(
        RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT, static_cast<uint32_t>(cc.multiViewCameras.size()));
    renderCamera.multiViewCameraHash = 0U;
    for (uint32_t idx = 0; idx < maxMvCount; ++idx) {
        const auto& mvRef = cc.multiViewCameras[idx];
        if (auto otherCamera = cameraMgr.Read(mvRef)) {
            if ((otherCamera->sceneFlags & (CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT)) &&
                (otherCamera->pipelineFlags & CameraComponent::PipelineFlagBits::MULTI_VIEW_ONLY_BIT)) {
                renderCamera.multiViewCameraIds[renderCamera.multiViewCameraCount++] = mvRef.id;
                HashCombine(renderCamera.multiViewCameraHash, mvRef.id);
            }
        }
    }
    if ((renderCamera.multiViewCameraCount > 0U) ||
        (renderCamera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_MULTI_VIEW_ONLY_BIT)) {
        if (renderCamera.renderPipelineType == RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_I(to_string(renderCamera.id) + "camera_pipeline_mv",
                "Camera rendering pipeline cannot be LIGHT_FORWARD for multi-view (changing to FORWARD internally).");
#endif
            renderCamera.renderPipelineType = RenderCamera::RenderPipelineType::FORWARD;
        }
    }

    ValidateRenderCamera(renderCamera);
}

RenderCamera CreateColorPrePassRenderCamera(const IRenderHandleComponentManager& renderHandleMgr,
    const ICameraComponentManager& cameraMgr, const IGpuResourceManager& gpuResourceMgr, const RenderCamera& baseCamera,
    const Entity& prePassEntity, const uint64_t uniqueId)
{
    RenderCamera rc = baseCamera;
    rc.mainCameraId = baseCamera.id; // main camera for pre-pass
    // reset targets, pre-pass does not support custom targets nor uses main camera targets
    rc.depthTarget = {};
    for (uint32_t idx = 0; idx < countof(rc.colorTargets); ++idx) {
        rc.colorTargets[idx] = {};
    }
    // NOTE: LIGHT_FORWARD prevents additional HDR target creation
    rc.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
    // by default these cannot be resolved for the pre-pass camera
    // FillRenderCameraBaseFromCameraComponent handles these for actual pre-pass cameras
    rc.customRenderNodeGraphFile.clear();
    rc.customRenderNodeGraph = {};
    if (const auto prePassCameraHandle = cameraMgr.Read(prePassEntity); prePassCameraHandle) {
        FillRenderCameraBaseFromCameraComponent(
            renderHandleMgr, cameraMgr, gpuResourceMgr, *prePassCameraHandle, rc, false);
        rc.flags |= RenderCamera::CAMERA_FLAG_COLOR_PRE_PASS_BIT | RenderCamera::CAMERA_FLAG_OPAQUE_BIT |
                    RenderCamera::CAMERA_FLAG_CLEAR_DEPTH_BIT | RenderCamera::CAMERA_FLAG_CLEAR_COLOR_BIT;
        // NOTE: does not evaluate custom targets for pre-pass camera
    } else {
        // automatic reduction to half res.
        rc.renderResolution = { static_cast<uint32_t>(static_cast<float>(rc.renderResolution[0]) * 0.5f),
            static_cast<uint32_t>(static_cast<float>(rc.renderResolution[1]) * 0.5f) };
        // NOTE: should better evaluate all the flags from the main camera
        rc.flags = RenderCamera::CAMERA_FLAG_COLOR_PRE_PASS_BIT | RenderCamera::CAMERA_FLAG_OPAQUE_BIT |
                   RenderCamera::CAMERA_FLAG_CLEAR_DEPTH_BIT | RenderCamera::CAMERA_FLAG_CLEAR_COLOR_BIT;
    }
    rc.name = to_hex(uniqueId);
    rc.id = uniqueId; // unique id for main pre-pass
    rc.prePassColorTargetName = {};
    rc.postProcessName = DefaultMaterialCameraConstants::CAMERA_PRE_PASS_POST_PROCESS_PREFIX_NAME;
    return rc;
}

void WeightedPercentualBlend(BASE_NS::Math::Vec4& origin, const BASE_NS::Math::Vec4& switches)
{
    float sumSwitches = switches.x + switches.y + switches.z + switches.w;

    if (std::fabs(sumSwitches - 1.0f) < Math::EPSILON) {
        origin = switches;
    } else {
        float sumOrigin = 0.0f;

        for (uint32_t i = 0; i < 4U; ++i) {
            if (switches[i] == 0.0f) {
                sumOrigin += origin[i];
            }
        }
        if (sumOrigin == 0.0f) { // !0div
            for (uint32_t i = 0; i < 4U; ++i) {
                if (switches[i] == 0.0f) {
                    origin[i] = 0.0f;
                }
            }
        } else {
            float scalingFactor = (1.0f - sumSwitches) / sumOrigin;

            for (uint32_t i = 0; i < 4U; ++i) {
                if (switches[i] > 0.0f) {
                    origin[i] = switches[i];
                } else {
                    origin[i] *= scalingFactor;
                }
            }
        }
    }
}

void FillRenderEnvironment(const IRenderHandleComponentManager& renderHandleMgr, const uint64_t& nodeLayerMask,
    const Entity& entity, const EnvironmentComponent& component, const Entity& probeTarget,
    RenderCamera::Environment& renderEnv, const IDynamicEnvironmentBlenderComponentManager& debcMgr)
{
    renderEnv.id = entity.id;
    renderEnv.layerMask = nodeLayerMask;
    renderEnv.shader = renderHandleMgr.GetRenderHandleReference(component.shader);
    if (component.background == EnvironmentComponent::Background::NONE) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_NONE;
    } else if (component.background == EnvironmentComponent::Background::IMAGE) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_IMAGE;
    } else if (component.background == EnvironmentComponent::Background::CUBEMAP) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_CUBEMAP;
    } else if (component.background == EnvironmentComponent::Background::EQUIRECTANGULAR) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_EQUIRECTANGULAR;
    } else if (component.background == EnvironmentComponent::Background::SKY) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_SKY;
    }
    // force blender cubemap
    if (EntityUtil::IsValid(component.blendEnvironments)) {
        if (auto blendEnv = debcMgr.Read(component.blendEnvironments); blendEnv) {
            if (!blendEnv->environments.empty()) {
                renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_CUBEMAP;
            }
        }
    }
    // check for weather effects
    if (EntityUtil::IsValid(component.weather)) {
        renderEnv.flags |= RenderCamera::Environment::EnvironmentFlagBits::ENVIRONMENT_FLAG_CAMERA_WEATHER_BIT;
    }

    if (EntityUtil::IsValid(probeTarget)) {
        renderEnv.radianceCubemap = renderHandleMgr.GetRenderHandleReference(probeTarget);
        renderEnv.envMap = renderHandleMgr.GetRenderHandleReference(probeTarget);
    } else {
        renderEnv.radianceCubemap = renderHandleMgr.GetRenderHandleReference(component.radianceCubemap);
        renderEnv.envMap = renderHandleMgr.GetRenderHandleReference(component.envMap);
    }
    renderEnv.radianceCubemapMipCount = component.radianceCubemapMipCount;
    renderEnv.envMapLodLevel = component.envMapLodLevel;
    const size_t shCount =
        std::min(countof(component.irradianceCoefficients), countof(renderEnv.shIndirectCoefficients));
    for (size_t idx = 0; idx < shCount; ++idx) {
        renderEnv.shIndirectCoefficients[idx] = Math::Vec4(component.irradianceCoefficients[idx], 1.0f);
    }
    renderEnv.indirectDiffuseFactor = component.indirectDiffuseFactor;
    renderEnv.indirectSpecularFactor = component.indirectSpecularFactor;
    renderEnv.envMapFactor = component.envMapFactor;
    if (EntityUtil::IsValid(component.blendEnvironments)) {
        if (auto blendEnv = debcMgr.Read(component.blendEnvironments); blendEnv) {
            auto entry = blendEnv->entryFactor;
            auto swtch = blendEnv->switchFactor;
            WeightedPercentualBlend(entry, swtch);
            renderEnv.blendFactor = entry;
        }
    } else {
        renderEnv.blendFactor = { 1.0f, 0.0f, 0.0f, 0.0f };
    }

    renderEnv.rotation = component.environmentRotation;
}

inline constexpr Math::Vec4 LerpVec4(const Math::Vec4& vec0, const Math::Vec4& vec1, const Math::Vec4& blend)
{
    return Math::Vec4(Math::lerp(vec0.x, vec1.x, blend.x), Math::lerp(vec0.y, vec1.y, blend.y),
        Math::lerp(vec0.z, vec1.z, blend.z), Math::lerp(vec0.w, vec1.w, blend.w));
}

inline constexpr Math::Vec4 LerpVec4(const Math::Vec4& vec0, const Math::Vec4& vec1, const float blend)
{
    return Math::Vec4(Math::lerp(vec0.x, vec1.x, blend), Math::lerp(vec0.y, vec1.y, blend),
        Math::lerp(vec0.z, vec1.z, blend), Math::lerp(vec0.w, vec1.w, blend));
}

void FillCameraRenderEnvironment(
    IRenderDataStoreDefaultCamera& dsCamera, const CameraComponent& component, RenderCamera& camera)
{
    camera.environment = EntityUtil::IsValid(component.environment)
                             ? dsCamera.GetEnvironment(component.environment.id)
                             : dsCamera.GetEnvironment(RenderSceneDataConstants::INVALID_ID);
    // if dynamic cubemap is used, we create a new environment for camera id with default coefficients
    if (camera.environment.multiEnvCount != 0U) {
        camera.environment.backgroundType = RenderCamera::Environment::BG_TYPE_CUBEMAP;
    }
}

void FillMultiEnvironments(const IEnvironmentComponentManager& envMgr, RenderCamera::Environment& renderEnv)
{
    // only supported background type
    CORE_ASSERT(renderEnv.multiEnvCount != 0U);
    CORE_ASSERT(renderEnv.backgroundType == RenderCamera::Environment::BG_TYPE_CUBEMAP);
    // replace base environment values -> combine indirect diffuse values
    renderEnv.indirectDiffuseFactor = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    renderEnv.indirectSpecularFactor = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    renderEnv.envMapFactor = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    for (size_t idx = 0; idx < countof(renderEnv.shIndirectCoefficients); ++idx) {
        renderEnv.shIndirectCoefficients[idx] = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
    float blendVal = 1.0f;
    for (uint32_t envIdx = 0U; envIdx < renderEnv.multiEnvCount; ++envIdx) {
        if (const auto multiEnv = envMgr.Read(Entity { renderEnv.multiEnvIds[envIdx] }); multiEnv) {
            const auto& currEnv = *multiEnv;
            renderEnv.indirectDiffuseFactor =
                LerpVec4(renderEnv.indirectDiffuseFactor, currEnv.indirectDiffuseFactor, blendVal);
            renderEnv.indirectSpecularFactor =
                LerpVec4(renderEnv.indirectSpecularFactor, currEnv.indirectSpecularFactor, blendVal);
            renderEnv.envMapFactor = LerpVec4(renderEnv.envMapFactor, currEnv.envMapFactor, blendVal);
            for (size_t idx = 0; idx < countof(renderEnv.shIndirectCoefficients); ++idx) {
                renderEnv.shIndirectCoefficients[idx] = LerpVec4(renderEnv.shIndirectCoefficients[idx],
                    Math::Vec4(currEnv.irradianceCoefficients[idx], 1.0f), blendVal);
            }
            blendVal = renderEnv.blendFactor[envIdx];
        }
    }
}

RenderCamera::Fog GetRenderCameraFogFromComponent(const ILayerComponentManager* layerMgr,
    const IFogComponentManager* fogMgr, const RenderConfigurationComponent& renderConfigurationComponent,
    const CameraComponent& cameraComponent)
{
    RenderCamera::Fog renderFog;
    if (!(layerMgr && fogMgr)) {
        return renderFog;
    }

    auto fillRenderFog = [](const uint64_t& nodeLayerMask, const Entity& entity, const FogComponent& component,
                             RenderCamera::Fog& renderFog) {
        renderFog.id = entity.id;
        renderFog.layerMask = nodeLayerMask;

        renderFog.firstLayer = { component.density, component.heightFalloff, component.heightFogOffset, 0.0f };
        renderFog.secondLayer = { component.layerDensity, component.layerHeightFalloff, component.layerHeightFogOffset,
            0.0f };
        renderFog.baseFactors = { component.startDistance, component.cuttoffDistance, component.maxOpacity, 0.0f };
        renderFog.inscatteringColor = component.inscatteringColor;
        renderFog.envMapFactor = component.envMapFactor;
        renderFog.additionalFactor = component.additionalFactor;
    };

    const Entity cameraFogEntity = cameraComponent.fog;
    // Check if camera has a valid fog
    Entity fogEntity = cameraFogEntity;
    auto fogId = fogMgr->GetComponentId(fogEntity);
    if (fogId == IComponentManager::INVALID_COMPONENT_ID) {
        // Next try if the scene has a valid fog
        fogEntity = renderConfigurationComponent.fog;
        fogId = fogMgr->GetComponentId(fogEntity);
    }
    if (fogId != IComponentManager::INVALID_COMPONENT_ID) {
        if (auto fogDataHandle = fogMgr->Read(fogId); fogDataHandle) {
            const FogComponent& fogComponent = *fogDataHandle;
            uint64_t layerMask = LayerConstants::DEFAULT_LAYER_MASK;
            if (auto layerHandle = layerMgr->Read(fogEntity); layerHandle) {
                layerMask = layerHandle->layerMask;
            }

            fillRenderFog(layerMask, fogEntity, fogComponent, renderFog);
        }
    }

    return renderFog;
}

constexpr IRenderDataStoreDefaultLight::ShadowTypes GetRenderShadowTypes(
    const RenderConfigurationComponent& renderConfigurationComponent)
{
    IRenderDataStoreDefaultLight::ShadowTypes st { IRenderDataStoreDefaultLight::ShadowType::PCF,
        IRenderDataStoreDefaultLight::ShadowQuality::NORMAL, IRenderDataStoreDefaultLight::ShadowSmoothness::NORMAL };
    st.shadowType = (renderConfigurationComponent.shadowType == RenderConfigurationComponent::SceneShadowType::VSM)
                        ? IRenderDataStoreDefaultLight::ShadowType::VSM
                        : IRenderDataStoreDefaultLight::ShadowType::PCF;
    if (renderConfigurationComponent.shadowQuality == RenderConfigurationComponent::SceneShadowQuality::LOW) {
        st.shadowQuality = IRenderDataStoreDefaultLight::ShadowQuality::LOW;
    } else if (renderConfigurationComponent.shadowQuality == RenderConfigurationComponent::SceneShadowQuality::HIGH) {
        st.shadowQuality = IRenderDataStoreDefaultLight::ShadowQuality::HIGH;
    } else if (renderConfigurationComponent.shadowQuality == RenderConfigurationComponent::SceneShadowQuality::ULTRA) {
        st.shadowQuality = IRenderDataStoreDefaultLight::ShadowQuality::ULTRA;
    }
    if (renderConfigurationComponent.shadowSmoothness == RenderConfigurationComponent::SceneShadowSmoothness::HARD) {
        st.shadowSmoothness = IRenderDataStoreDefaultLight::ShadowSmoothness::HARD;
    } else if (renderConfigurationComponent.shadowSmoothness ==
               RenderConfigurationComponent::SceneShadowSmoothness::SOFT) {
        st.shadowSmoothness = IRenderDataStoreDefaultLight::ShadowSmoothness::SOFT;
    }
    return st;
}

IRenderDataStorePostProcess::PostProcess::Variables FillPostProcessConfigurationVars(
    const PostProcessConfigurationComponent::PostProcessEffect& pp)
{
    IRenderDataStorePostProcess::PostProcess::Variables vars;
    vars.userFactorIndex = pp.globalUserFactorIndex;
    // NOTE: shader cannot be changed here
    vars.factor = pp.factor;
    vars.enabled = pp.enabled;
    vars.flags = pp.flags;
    if (pp.customProperties) {
        array_view<const uint8_t> customData =
            array_view(static_cast<const uint8_t*>(pp.customProperties->RLock()), pp.customProperties->Size());
        const size_t copyByteSize = Math::min(countof(vars.customPropertyData), customData.size_bytes());
        CloneData(vars.customPropertyData, countof(vars.customPropertyData), customData.data(), copyByteSize);
        pp.customProperties->RUnlock();
    }
    return vars;
}

PROPERTY_LIST(IRenderSystem::Properties, ComponentMetadata, MEMBER_PROPERTY(dataStoreMaterial, "dataStoreMaterial", 0),
    MEMBER_PROPERTY(dataStoreCamera, "dataStoreCamera", 0), MEMBER_PROPERTY(dataStoreLight, "dataStoreLight", 0),
    MEMBER_PROPERTY(dataStoreScene, "dataStoreScene", 0), MEMBER_PROPERTY(dataStoreMorph, "dataStoreMorph", 0),
    MEMBER_PROPERTY(dataStorePrefix, "", 0))

// Extended sign: returns -1, 0 or 1 based on sign of a
float Sgn(float a)
{
    if (a > 0.0f) {
        return 1.0f;
    }

    if (a < 0.0f) {
        return -1.0f;
    }

    return 0.0f;
}

struct ReflectionPlaneTargetUpdate {
    bool recreated { false };
    uint32_t mipCount { 1u };
    uint32_t renderTargetResolution[2U] {};
    EntityReference colorRenderTarget;
    EntityReference depthRenderTarget;
};

Entity UpdateReflectionPlaneMaterial(IRenderMeshComponentManager& renderMeshMgr, IMeshComponentManager& meshMgr,
    IMaterialComponentManager& materialMgr, const Entity& entity, const float screenPercentage,
    const ReflectionPlaneTargetUpdate& rptu)
{
    // update material
    const auto rmcHandle = renderMeshMgr.Read(entity);
    if (!rmcHandle) {
        return {};
    }
    const auto meshHandle = meshMgr.Read(rmcHandle->mesh);
    if (!meshHandle) {
        return {};
    }
    if (meshHandle->submeshes.empty()) {
        return {};
    }
    if (auto matHandle = materialMgr.Write(meshHandle->submeshes[0].material)) {
        // NOTE: CLEARCOAT_ROUGHNESS cannot be used due to material flags bit is enabled for lighting
        matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].factor = {
            static_cast<float>(Math::min(rptu.mipCount, rptu.mipCount - 1U)),
            screenPercentage,
            static_cast<float>(rptu.renderTargetResolution[0u]),
            static_cast<float>(rptu.renderTargetResolution[1u]),
        };
        matHandle->textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].image = rptu.colorRenderTarget;
        return meshHandle->submeshes[0].material;
    }
    return {};
}

void ProcessReflectionTargetSize(const PlanarReflectionComponent& rc, const RenderCamera& cam, Math::UVec2& targetRes)
{
    targetRes.x = Math::max(
        targetRes.x, static_cast<uint32_t>(static_cast<float>(cam.renderResolution[0U]) * rc.screenPercentage));
    targetRes.y = Math::max(
        targetRes.y, static_cast<uint32_t>(static_cast<float>(cam.renderResolution[1U]) * rc.screenPercentage));
}

ReflectionPlaneTargetUpdate UpdatePlaneReflectionTargetResolution(IGpuResourceManager& gpuResourceMgr,
    IRenderHandleComponentManager& gpuHandleMgr, const RenderCamera& sceneCamera, const Entity& entity,
    const Math::UVec2 targetRes, const uint32_t reflectionMaxMipBlur, const PlanarReflectionComponent& reflComp)
{
    ReflectionPlaneTargetUpdate rptu { false, 1U,
        { reflComp.renderTargetResolution[0], reflComp.renderTargetResolution[1] }, reflComp.colorRenderTarget,
        reflComp.depthRenderTarget };

    const RenderHandle colorRenderTarget = gpuHandleMgr.GetRenderHandle(rptu.colorRenderTarget);
    const RenderHandle depthRenderTarget = gpuHandleMgr.GetRenderHandle(rptu.depthRenderTarget);
    // get current mip count
    rptu.mipCount = RenderHandleUtil::IsValid(colorRenderTarget)
                        ? gpuResourceMgr.GetImageDescriptor(gpuResourceMgr.Get(colorRenderTarget)).mipCount
                        : 1U;
    // will resize based on frame max size
    if ((!RenderHandleUtil::IsValid(colorRenderTarget)) || (!RenderHandleUtil::IsValid(depthRenderTarget)) ||
        (targetRes.x != rptu.renderTargetResolution[0]) || (targetRes.y != rptu.renderTargetResolution[1])) {
        auto reCreateGpuImage = [](IGpuResourceManager& gpuResourceMgr, uint64_t id, RenderHandle handle,
                                    uint32_t newWidth, uint32_t newHeight, uint baseMipCount, bool depthImage) {
            GpuImageDesc desc = RenderHandleUtil::IsValid(handle)
                                    ? gpuResourceMgr.GetImageDescriptor(gpuResourceMgr.Get(handle))
                                    : CreateReflectionPlaneGpuImageDesc(depthImage);
            desc.mipCount = baseMipCount;
            desc.width = newWidth;
            desc.height = newHeight;
            if (RenderHandleUtil::IsValid(handle)) {
                return gpuResourceMgr.Create(gpuResourceMgr.Get(handle), desc);
            } else {
                return gpuResourceMgr.Create(desc);
            }
        };
        if (!EntityUtil::IsValid(rptu.colorRenderTarget)) {
            rptu.colorRenderTarget = gpuHandleMgr.GetEcs().GetEntityManager().CreateReferenceCounted();
            gpuHandleMgr.Create(rptu.colorRenderTarget);
        }
        rptu.mipCount = Math::min(reflectionMaxMipBlur,
            static_cast<uint32_t>(std::log2f(static_cast<float>(std::max(targetRes.x, targetRes.y)))) + 1u);
        gpuHandleMgr.Write(rptu.colorRenderTarget)->reference = reCreateGpuImage(
            gpuResourceMgr, entity.id, colorRenderTarget, targetRes.x, targetRes.y, rptu.mipCount, false);

        if (!EntityUtil::IsValid(rptu.depthRenderTarget)) {
            rptu.depthRenderTarget = gpuHandleMgr.GetEcs().GetEntityManager().CreateReferenceCounted();
            gpuHandleMgr.Create(rptu.depthRenderTarget);
        }
        gpuHandleMgr.Write(rptu.depthRenderTarget)->reference =
            reCreateGpuImage(gpuResourceMgr, entity.id, depthRenderTarget, targetRes.x, targetRes.y, 1u, true);

        rptu.renderTargetResolution[0] = targetRes.x;
        rptu.renderTargetResolution[1] = targetRes.y;
        rptu.recreated = true;
    }

    return rptu;
}

// Given position and normal of the plane, calculates plane in camera space.
inline Math::Vec4 CalculateCameraSpaceClipPlane(
    const Math::Mat4X4& view, Math::Vec3 pos, Math::Vec3 normal, float sideSign)
{
    const Math::Vec3 offsetPos = pos;
    const Math::Vec3 cpos = Math::MultiplyPoint3X4(view, offsetPos);
    const Math::Vec3 cnormal = Math::Normalize(Math::MultiplyVector(view, normal)) * sideSign;

    return Math::Vec4(cnormal.x, cnormal.y, cnormal.z, -Math::Dot(cpos, cnormal));
}

// See http://aras-p.info/texts/obliqueortho.html
inline void CalculateObliqueProjectionMatrix(Math::Mat4X4& projection, const Math::Vec4& plane)
{
    const Math::Mat4X4 inverseProjection = Inverse(projection);

    const Math::Vec4 q = inverseProjection * Math::Vec4(Sgn(plane.x), Sgn(plane.y), 1.0f, 1.0f);

    // https://terathon.com/lengyel/Lengyel-Oblique.pdf page 7 Figure 4b shows what we want to achieve.
    // The scale of 'c' plane controls the angle between near and far planes. Lengyel's algorithm minimizes this angle
    // to optimize depth precision by making the far plane pass through the original frustum's corner point Q.

    // IMPORTANT TRADE-OFF: This optimizes depth precision but it is more prone to clip geometry from specific angles.

    // In https://terathon.com/blog/oblique-clipping.html the equation is "plane * (2.0f / Math::Dot(plane, q))"
    // The factor 2.0f creates a small angle between the 2 planes and it is clipping geometry close to the far plane.
    // Using a smaller factor of 0.1 (larger angle) it prevents geometry clipping close to the far plane.

    // Oblique projection requires the clipping plane to face toward the camera (planeDotQ < 0).
    // When planeDotQ approaches zero or becomes positive, we clamp the scaling factor to ensure the plane always clips
    // in the correct direction.
    const float planeDotQ = Math::Dot(plane, q);
    const float clampedScale = Math::min(0.1f / Math::abs(planeDotQ), 0.1f);
    const Math::Vec4 c = plane * -clampedScale;

    projection.data[2u] = c.x;
    projection.data[6u] = c.y;
    projection.data[10u] = c.z;
    projection.data[14u] = c.w;
}

// Calculate reflection matrix from given matrix.
Math::Mat4X4 CalculateReflectionMatrix(const Math::Vec4& plane)
{
    Math::Mat4X4 result(1.0f);

    result.data[0u] = (1.0f - 2.0f * plane[0u] * plane[0u]);
    result.data[4u] = (-2.0f * plane[0u] * plane[1u]);
    result.data[8u] = (-2.0f * plane[0u] * plane[2u]);
    result.data[12u] = (-2.0f * plane[3u] * plane[0u]);

    result.data[1u] = (-2.0f * plane[1u] * plane[0u]);
    result.data[5u] = (1.0f - 2.0f * plane[1u] * plane[1u]);
    result.data[9u] = (-2.0f * plane[1u] * plane[2u]);
    result.data[13u] = (-2.0f * plane[3u] * plane[1u]);

    result.data[2u] = (-2.0f * plane[2u] * plane[0u]);
    result.data[6u] = (-2.0f * plane[2u] * plane[1u]);
    result.data[10u] = (1.0f - 2.0f * plane[2u] * plane[2u]);
    result.data[14u] = (-2.0f * plane[3u] * plane[2u]);

    result.data[3u] = 0.0f;
    result.data[7u] = 0.0f;
    result.data[11u] = 0.0f;
    result.data[15u] = 1.0f;

    return result;
}

inline void LogBatchValidation(const MeshComponent& mesh)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (mesh.submeshes.size() > MAX_BATCH_SUBMESH_COUNT) {
        CORE_LOG_ONCE_E("submesh_counts_batch",
            "CORE3D_VALIDATION: GPU instancing batches not supported for submeshes with count %u > %u ",
            static_cast<uint32_t>(mesh.submeshes.size()), MAX_BATCH_SUBMESH_COUNT);
    }
#endif
}

inline void DestroyBatchData(BASE_NS::unordered_map<CORE_NS::Entity, RenderSystem::BatchDataVector>& batches)
{
    // NOTE: we destroy batch entity if its elements were not used in this frame
    for (auto iter = batches.begin(); iter != batches.end();) {
        if (iter->second.empty()) {
            iter = batches.erase(iter);
        } else {
            iter->second.clear();
            ++iter;
        }
    }
}

inline void ProcessCameraAddMultiViewHash(const RenderCamera& cam, unordered_map<uint64_t, uint64_t>& childToParent)
{
    for (uint32_t idx = 0; idx < cam.multiViewCameraCount; ++idx) {
        childToParent.insert_or_assign(cam.multiViewCameraIds[idx], cam.id);
    }
}

void CalculateFinalSceneBoundingSphere(
    const RenderBoundingSphere& renderBoundingSphere, Math::Vec3& center, float& radius)
{
    if (renderBoundingSphere.radius == 0.0f) {
        // basically no objects when radius is zero
        center = Math::Vec3(0.0f, 0.0f, 0.0f);
        radius = 0.0f;
    } else {
        const auto boundingSpherePosition = renderBoundingSphere.center;
        const float boundingSphereRadius = renderBoundingSphere.radius;

        // Compensate jitter and adjust scene bounding sphere only if change in bounds is meaningful.
        if (radius > 0.0f) {
            // Calculate distance to new bounding sphere origin from current sphere.
            const float pointDistance = Math::Magnitude(boundingSpherePosition - center);
            // Calculate distance to edge of new bounding sphere from current sphere origin.
            const float sphereEdgeDistance = pointDistance + boundingSphereRadius;

            // Calculate step size for adjustment, use 10% granularity from current bounds.
            constexpr float granularityPct = 0.10f;
            const float granularity = radius * granularityPct;

            // Calculate required change of size, in order to fit new sphere inside current bounds.
            const float radDifference = sphereEdgeDistance - radius;
            const float posDifference = Math::Magnitude(boundingSpherePosition - center);
            // We need to adjust only if the change is bigger than the step size.
            if ((Math::abs(radDifference) > granularity) || (posDifference > granularity)) {
                // Calculate how many steps we need to change and in to which direction.
                const float radAmount = ceil((boundingSphereRadius - radius) / granularity);
                const int32_t posAmount = (int32_t)ceil(posDifference / granularity);
                if ((radAmount != 0.f) || (posAmount != 0)) {
                    // Update size and position of the bounds.
                    center = boundingSpherePosition;
                    radius = radius + (radAmount * granularity);
                }
            }
        } else {
            // No existing bounds, start with new values.
            radius = boundingSphereRadius;
            center = boundingSpherePosition;
        }
    }
}

struct CameraOrdering {
    uint64_t id { RenderSceneDataConstants::INVALID_ID };
    uint64_t mainId { RenderSceneDataConstants::INVALID_ID };
    size_t renderCameraIdx { 0 };
};

vector<CameraOrdering> SortCameras(const array_view<const RenderCamera> renderCameras, const bool prepassRequired)
{
    vector<CameraOrdering> baseCameras;
    vector<CameraOrdering> depCameras;
    baseCameras.reserve(renderCameras.size());
    depCameras.reserve(renderCameras.size());
    size_t mainCamIdx = size_t(~0);
    // ignore shadow and multi-view only cameras
    constexpr uint32_t ignoreFlags { RenderCamera::CAMERA_FLAG_SHADOW_BIT |
                                     RenderCamera::CAMERA_FLAG_MULTI_VIEW_ONLY_BIT };
    for (size_t camIdx = 0; camIdx < renderCameras.size(); ++camIdx) {
        const auto& cam = renderCameras[camIdx];
        if ((cam.flags & ignoreFlags)) {
            continue;
        }
        if (cam.flags & RenderCamera::CAMERA_FLAG_MAIN_BIT) {
            mainCamIdx = camIdx;
        } else if (cam.mainCameraId == RenderSceneDataConstants::INVALID_ID) {
            baseCameras.push_back({ cam.id, cam.mainCameraId, camIdx });
        } else if (!(cam.flags & RenderCamera::CAMERA_FLAG_COLOR_PRE_PASS_BIT) || prepassRequired) {
            // do not add pre-pass camera if render processing does not need it
            depCameras.push_back({ cam.id, cam.mainCameraId, camIdx });
        }
    }
    // main camera needs to be the last
    if (mainCamIdx < renderCameras.size()) {
        const auto& cam = renderCameras[mainCamIdx];
        baseCameras.push_back({ cam.id, cam.mainCameraId, mainCamIdx });
    }
    // insert dependency cameras to correct positions
    for (const auto& depCam : depCameras) {
        const auto pos = std::find_if(baseCameras.cbegin(), baseCameras.cend(),
            [mainId = depCam.mainId](const CameraOrdering& base) { return base.id == mainId; });
        if (pos != baseCameras.cend()) {
            baseCameras.insert(pos, depCam);
        }
    }
    // now cameras are in correct order if the dependencies were correct in RenderCameras
    return baseCameras;
}

void RemoveMaterialProperties(IRenderDataStoreDefaultMaterial& dsMaterial,
    const IMaterialComponentManager& materialManager, array_view<const Entity> removedMaterials)
{
    // destroy rendering side decoupled material data
    for (const auto& entRef : removedMaterials) {
        dsMaterial.DestroyMaterialData(entRef.id);
    }
}
#if (CORE3D_VALIDATION_ENABLED == 1)
void ValidateInputColor(const Entity material, const MaterialComponent& matComp)
{
    if (matComp.type < MaterialComponent::Type::CUSTOM) {
        const auto& base = matComp.textures[MaterialComponent::TextureIndex::BASE_COLOR];
        if ((base.factor.x > 1.0f) || (base.factor.y > 1.0f) || (base.factor.z > 1.0f) || (base.factor.w > 1.0f)) {
            CORE_LOG_ONCE_I("ValidateInputColor_expect_base_colorfactor",
                "CORE3D_VALIDATION: Non custom material type expects base color factor to be <= 1.0f.");
        }
        const auto& mat = matComp.textures[MaterialComponent::TextureIndex::MATERIAL];
        if ((mat.factor.y > 1.0f) || (mat.factor.z > 1.0f)) {
            CORE_LOG_ONCE_I("ValidateInputColor_expect_roughness_metallic_factor",
                "CORE3D_VALIDATION: Non custom material type expects roughness and metallic to be <= 1.0f.");
        }
    }
}
#endif

inline void GetRenderHandleReferences(const IRenderHandleComponentManager& renderHandleMgr,
    const array_view<const EntityReference> inputs, array_view<RenderHandleReference>& outputs)
{
    for (size_t idx = 0; idx < outputs.size(); ++idx) {
        outputs[idx] = renderHandleMgr.GetRenderHandleReference(inputs[idx]);
    }
}

constexpr uint32_t RenderMaterialLightingFlagsFromMaterialFlags(const MaterialComponent::LightingFlags materialFlags)
{
    uint32_t rmf = 0;
    if (materialFlags & MaterialComponent::LightingFlagBits::SHADOW_RECEIVER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_RECEIVER_BIT;
    }
    if (materialFlags & MaterialComponent::LightingFlagBits::SHADOW_CASTER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHADOW_CASTER_BIT;
    }
    if (materialFlags & MaterialComponent::LightingFlagBits::PUNCTUAL_LIGHT_RECEIVER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT;
    }
    if (materialFlags & MaterialComponent::LightingFlagBits::INDIRECT_LIGHT_RECEIVER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT;
    }
    if (materialFlags & MaterialComponent::LightingFlagBits::INDIRECT_IRRADIANCE_LIGHT_RECEIVER_BIT) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_INDIRECT_LIGHT_RECEIVER_IRRADIANCE_BIT;
    }
    return rmf;
}

constexpr uint32_t RenderSubmeshFlagsFromMeshFlags(const MeshComponent::Submesh::Flags flags)
{
    uint32_t rmf = 0;
    if (flags & MeshComponent::Submesh::FlagBits::TANGENTS_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_TANGENTS_BIT;
    }
    if (flags & MeshComponent::Submesh::FlagBits::VERTEX_COLORS_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_VERTEX_COLORS_BIT;
    }
    if (flags & MeshComponent::Submesh::FlagBits::SKIN_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT;
    }
    if (flags & MeshComponent::Submesh::FlagBits::SECOND_TEXCOORD_BIT) {
        rmf |= RenderSubmeshFlagBits::RENDER_SUBMESH_SECOND_TEXCOORD_BIT;
    }
    return rmf;
}

RenderDataDefaultMaterial::InputMaterialUniforms InputMaterialUniformsFromMaterialComponent(
    const Entity material, const MaterialComponent& matDesc)
{
    RenderDataDefaultMaterial::InputMaterialUniforms mu = DEF_INPUT_MATERIAL_UNIFORMS;

#if (CORE3D_VALIDATION_ENABLED == 1)
    ValidateInputColor(material, matDesc);
#endif

    uint32_t transformBits = 0u;
    constexpr const uint32_t texCount = Math::min(static_cast<uint32_t>(MaterialComponent::TextureIndex::TEXTURE_COUNT),
        RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT);
    for (uint32_t idx = 0u; idx < texCount; ++idx) {
        const auto& tex = matDesc.textures[idx];
        auto& texRef = mu.textureData[idx];
        texRef.factor = tex.factor;
        texRef.translation = tex.transform.translation;
        texRef.rotation = tex.transform.rotation;
        texRef.scale = tex.transform.scale;
        const bool hasTransform = (texRef.translation.x != 0.0f) || (texRef.translation.y != 0.0f) ||
                                  (texRef.rotation != 0.0f) || (texRef.scale.x != 1.0f) || (texRef.scale.y != 1.0f);
        transformBits |= static_cast<uint32_t>(hasTransform) << idx;
    }
    {
        // NOTE: premultiplied alpha, applied here and therefore the baseColor factor is special
        const auto& tex = matDesc.textures[MaterialComponent::TextureIndex::BASE_COLOR];
        const float alpha = tex.factor.w;
        const Math::Vec4 baseColor = {
            tex.factor.x * alpha,
            tex.factor.y * alpha,
            tex.factor.z * alpha,
            alpha,
        };

        constexpr uint32_t index = 0u;
        mu.textureData[index].factor = baseColor;
    }
    mu.alphaCutoff = matDesc.alphaCutoff;
    mu.texCoordSetBits = matDesc.useTexcoordSetBit;
    mu.texTransformSetBits = transformBits;
    mu.id = material.id;
    return mu;
}

inline RenderDataDefaultMaterial::MaterialHandlesWithHandleReference GetMaterialHandles(
    const MaterialComponent& materialDesc, const IRenderHandleComponentManager& gpuManager)
{
    RenderDataDefaultMaterial::MaterialHandlesWithHandleReference materialHandles;
    auto imageIt = std::begin(materialHandles.images);
    auto samplerIt = std::begin(materialHandles.samplers);
    for (const MaterialComponent::TextureInfo& info : materialDesc.textures) {
        *imageIt++ = gpuManager.GetRenderHandleReference(info.image);
        *samplerIt++ = gpuManager.GetRenderHandleReference(info.sampler);
    }
    return materialHandles;
}

uint32_t RenderMaterialFlagsFromMaterialValues(const MaterialComponent& matComp,
    const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference& handles, const uint32_t hasTransformBit)
{
    uint32_t rmf = 0;
    // enable built-in specialization for default materials
    CORE_ASSERT(matComp.type <= MaterialComponent::Type::OCCLUSION);
    if (matComp.type < MaterialComponent::Type::CUSTOM) {
        if (handles.images[MaterialComponent::TextureIndex::NORMAL] ||
            handles.images[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL]) {
            // need to check for tangents as well with submesh
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_NORMAL_MAP_BIT;
        }
        if (matComp.textures[MaterialComponent::TextureIndex::CLEARCOAT].factor.x > 0.0f) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_CLEAR_COAT_BIT;
        }
        if ((matComp.textures[MaterialComponent::TextureIndex::SHEEN].factor.x > 0.0f) ||
            (matComp.textures[MaterialComponent::TextureIndex::SHEEN].factor.y > 0.0f) ||
            (matComp.textures[MaterialComponent::TextureIndex::SHEEN].factor.z > 0.0f)) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHEEN_BIT;
        }
        if (matComp.textures[MaterialComponent::TextureIndex::SPECULAR].factor != Math::Vec4(1.f, 1.f, 1.f, 1.f) ||
            handles.images[MaterialComponent::TextureIndex::SPECULAR]) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SPECULAR_BIT;
        }
        if (matComp.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x > 0.0f) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_TRANSMISSION_BIT;
        }
    }
    rmf |= (hasTransformBit > 0U) ? RenderMaterialFlagBits::RENDER_MATERIAL_TEXTURE_TRANSFORM_BIT : 0U;
    // NOTE: built-in shaders write 1.0 to alpha always when discard is enabled
    rmf |= (matComp.alphaCutoff < 1.0f) ? RenderMaterialFlagBits::RENDER_MATERIAL_SHADER_DISCARD_BIT : 0U;
    // NOTE: GPU instancing specialization needs to be enabled during rendering
    return rmf;
}

inline constexpr RenderMaterialFlags RenderMaterialFlagsFromMaterialValues(
    MaterialComponent::ExtraRenderingFlags extraFlags)
{
    RenderMaterialFlags rmf = 0;
    rmf |= (extraFlags & MaterialComponent::ExtraRenderingFlagBits::CAMERA_EFFECT)
               ? (RenderMaterialFlags)RenderMaterialFlagBits::RENDER_MATERIAL_CAMERA_EFFECT_BIT
               : 0U;
    // enable render time GPU instancing evaluation
    rmf |= (extraFlags & MaterialComponent::ExtraRenderingFlagBits::ALLOW_GPU_INSTANCING_BIT)
               ? RENDER_MATERIAL_GPU_INSTANCING_BIT
               : 0U;
    rmf |= (extraFlags & MaterialComponent::ExtraRenderingFlagBits::SPECULAR_FACTOR_TEXTURE)
               ? RenderMaterialFlagBits::RENDER_MATERIAL_SPECULAR_FACTOR_TEXTURE_BIT
               : 0;
    rmf |= (extraFlags & MaterialComponent::ExtraRenderingFlagBits::SPECULAR_COLOR_TEXTURE)
               ? RenderMaterialFlagBits::RENDER_MATERIAL_SPECULAR_COLOR_TEXTURE_BIT
               : 0;
    return rmf;
}

void SetupSubmeshBuffers(const IRenderHandleComponentManager& renderHandleManager,
    const MeshComponent::Submesh& submesh, RenderSubmeshDataWithHandleReference& renderSubmesh)
{
    CORE_STATIC_ASSERT(
        MeshComponent::Submesh::BUFFER_COUNT <= RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
    // calculate real vertex buffer count and fill "safety" handles for default material
    // no default shader variants without joints etc.
    // NOTE: optimize for minimal GetRenderHandleReference calls
    // often the same vertex buffer is used.
    Entity prevEntity = {};

    for (size_t idx = 0; idx < countof(submesh.bufferAccess); ++idx) {
        const auto& acc = submesh.bufferAccess[idx];
        auto& vb = renderSubmesh.buffers.vertexBuffers[idx];
        if (EntityUtil::IsValid(prevEntity) && (prevEntity == acc.buffer)) {
            vb.bufferHandle = renderSubmesh.buffers.vertexBuffers[idx - 1].bufferHandle;
            vb.bufferOffset = acc.offset;
            vb.byteSize = acc.byteSize;
        } else if (acc.buffer) {
            vb.bufferHandle = renderHandleManager.GetRenderHandleReference(acc.buffer);
            vb.bufferOffset = acc.offset;
            vb.byteSize = acc.byteSize;

            // store the previous entity
            prevEntity = acc.buffer;
        } else {
            vb.bufferHandle = renderSubmesh.buffers.vertexBuffers[0].bufferHandle; // expecting safety binding
            vb.bufferOffset = 0;
            vb.byteSize = 0;
        }
    }

    // NOTE: we will get max amount of vertex buffers if there is at least one
    renderSubmesh.buffers.vertexBufferCount =
        submesh.bufferAccess[0U].buffer ? static_cast<uint32_t>(countof(submesh.bufferAccess)) : 0U;

    if (submesh.indexBuffer.buffer) {
        renderSubmesh.buffers.indexBuffer.bufferHandle =
            renderHandleManager.GetRenderHandleReference(submesh.indexBuffer.buffer);
        renderSubmesh.buffers.indexBuffer.bufferOffset = submesh.indexBuffer.offset;
        renderSubmesh.buffers.indexBuffer.byteSize = submesh.indexBuffer.byteSize;
        renderSubmesh.buffers.indexBuffer.indexType = submesh.indexBuffer.indexType;
    }
    if (submesh.indirectArgsBuffer.buffer) {
        renderSubmesh.buffers.indirectArgsBuffer.bufferHandle =
            renderHandleManager.GetRenderHandleReference(submesh.indirectArgsBuffer.buffer);
        renderSubmesh.buffers.indirectArgsBuffer.bufferOffset = submesh.indirectArgsBuffer.offset;
        renderSubmesh.buffers.indirectArgsBuffer.byteSize = submesh.indirectArgsBuffer.byteSize;
    }
    renderSubmesh.buffers.inputAssembly = submesh.inputAssembly;
}
} // namespace

RenderSystem::RenderSystem(IEcs& ecs)
    : ecs_(ecs), nodeMgr_(GetManager<INodeComponentManager>(ecs)),
      renderMeshMgr_(GetManager<IRenderMeshComponentManager>(ecs)),
      worldMatrixMgr_(GetManager<IWorldMatrixComponentManager>(ecs)),
      renderConfigMgr_(GetManager<IRenderConfigurationComponentManager>(ecs)),
      cameraMgr_(GetManager<ICameraComponentManager>(ecs)), lightMgr_(GetManager<ILightComponentManager>(ecs)),
      planarReflectionMgr_(GetManager<IPlanarReflectionComponentManager>(ecs)),
      materialMgr_(GetManager<IMaterialComponentManager>(ecs)), meshMgr_(GetManager<IMeshComponentManager>(ecs)),
      uriMgr_(GetManager<IUriComponentManager>(ecs)), nameMgr_(GetManager<INameComponentManager>(ecs)),
      environmentMgr_(GetManager<IEnvironmentComponentManager>(ecs)), fogMgr_(GetManager<IFogComponentManager>(ecs)),
      gpuHandleMgr_(GetManager<IRenderHandleComponentManager>(ecs)), layerMgr_(GetManager<ILayerComponentManager>(ecs)),
      dynamicEnvBlendMgr_(GetManager<IDynamicEnvironmentBlenderComponentManager>(ecs)),
      skinMgr_(GetManager<ISkinComponentManager>(ecs)),
      jointMatricesMgr_(GetManager<IJointMatricesComponentManager>(ecs)),
      prevJointMatricesMgr_(GetManager<IPreviousJointMatricesComponentManager>(ecs)),
      postProcessMgr_(GetManager<IPostProcessComponentManager>(ecs)),
      postProcessConfigMgr_(GetManager<IPostProcessConfigurationComponentManager>(ecs)),
      postProcessEffectMgr_(GetManager<IPostProcessEffectComponentManager>(ecs)),
      graphicsStateMgr_(GetManager<IGraphicsStateComponentManager>(ecs)),
      RENDER_SYSTEM_PROPERTIES(&properties_, array_view(ComponentMetadata))
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>()) {
        frustumUtil_ = GetInstance<IFrustumUtil>(UID_FRUSTUM_UTIL);
        if (auto* engineClassRegister = engine->GetInterface<IClassRegister>()) {
            renderContext_ = GetInstance<IRenderContext>(*engineClassRegister, UID_RENDER_CONTEXT);
        }
    }
    if (renderContext_) {
        if (auto* renderClassRegister = renderContext_->GetInterface<IClassRegister>()) {
            picking_ = GetInstance<IPicking>(*renderClassRegister, UID_PICKING);
            graphicsContext_ = GetInstance<IGraphicsContext>(*renderClassRegister, UID_GRAPHICS_CONTEXT);
        }
        if (graphicsContext_) {
            renderUtil_ = &graphicsContext_->GetRenderUtil();
        }
        shaderMgr_ = &renderContext_->GetDevice().GetShaderManager();
        gpuResourceMgr_ = &renderContext_->GetDevice().GetGpuResourceManager();
    }
}

RenderSystem::~RenderSystem()
{
    DestroyRenderDataStores();
}

void RenderSystem::SetActive(bool state)
{
    active_ = state;
}

bool RenderSystem::IsActive() const
{
    return active_;
}

string_view RenderSystem::GetName() const
{
    return CORE3D_NS::GetName(this);
}

Uid RenderSystem::GetUid() const
{
    return UID;
}

IPropertyHandle* RenderSystem::GetProperties()
{
    return RENDER_SYSTEM_PROPERTIES.GetData();
}

const IPropertyHandle* RenderSystem::GetProperties() const
{
    return RENDER_SYSTEM_PROPERTIES.GetData();
}

void RenderSystem::SetProperties(const IPropertyHandle& data)
{
    if (data.Owner() != &RENDER_SYSTEM_PROPERTIES) {
        return;
    }
    if (const auto in = ScopedHandle<const IRenderSystem::Properties>(&data); in) {
        properties_.dataStoreScene = in->dataStoreScene;
        properties_.dataStoreCamera = in->dataStoreCamera;
        properties_.dataStoreLight = in->dataStoreLight;
        properties_.dataStoreMaterial = in->dataStoreMaterial;
        properties_.dataStoreMorph = in->dataStoreMorph;
        properties_.dataStorePrefix = in->dataStorePrefix;
        if (renderContext_) {
            SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
        }
    }
}

void RenderSystem::SetDataStorePointers(IRenderDataStoreManager& manager)
{
    // get data stores
    dsScene_ = refcnt_ptr<IRenderDataStoreDefaultScene>(manager.GetRenderDataStore(properties_.dataStoreScene));
    dsCamera_ = refcnt_ptr<IRenderDataStoreDefaultCamera>(manager.GetRenderDataStore(properties_.dataStoreCamera));
    dsLight_ = refcnt_ptr<IRenderDataStoreDefaultLight>(manager.GetRenderDataStore(properties_.dataStoreLight));
    dsMaterial_ =
        refcnt_ptr<IRenderDataStoreDefaultMaterial>(manager.GetRenderDataStore(properties_.dataStoreMaterial));
    dsRenderPostProcesses_ = refcnt_ptr<IRenderDataStoreRenderPostProcesses>(manager.Create(
        IRenderDataStoreRenderPostProcesses::UID, (properties_.dataStorePrefix + RPP_DATA_STORE_NAME).data()));
}

const IEcs& RenderSystem::GetECS() const
{
    return ecs_;
}

void RenderSystem::Initialize()
{
    if (graphicsContext_ && renderContext_) {
        renderPreprocessorSystem_ = GetSystem<IRenderPreprocessorSystem>(ecs_);
        if (renderPreprocessorSystem_) {
            const auto in =
                ScopedHandle<IRenderPreprocessorSystem::Properties>(renderPreprocessorSystem_->GetProperties());
            properties_.dataStoreScene = in->dataStoreScene;
            properties_.dataStoreCamera = in->dataStoreCamera;
            properties_.dataStoreLight = in->dataStoreLight;
            properties_.dataStoreMaterial = in->dataStoreMaterial;
            properties_.dataStoreMorph = in->dataStoreMorph;
            properties_.dataStorePrefix = in->dataStorePrefix;
        } else {
            CORE_LOG_E("DEPRECATED USAGE: RenderPreprocessorSystem not found. Add system to system graph.");
        }

        SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
    }
    if (renderContext_ && uriMgr_ && gpuHandleMgr_) {
        // fetch default shaders and graphics states
        auto& entityMgr = ecs_.GetEntityManager();
        FillShaderData(entityMgr, *uriMgr_, *gpuHandleMgr_, *shaderMgr_,
            { DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE, STATE_OPAQUE_NAME, DS_OPAQUE_NAME },
            dmShaderData_.opaque);
        FillShaderData(entityMgr, *uriMgr_, *gpuHandleMgr_, *shaderMgr_,
            { DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT, STATE_TRANSLUCENT_NAME,
                DS_TRANSLUCENT_NAME },
            dmShaderData_.blend);
        FillShaderData(entityMgr, *uriMgr_, *gpuHandleMgr_, *shaderMgr_,
            { DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH, STATE_DEPTH_NAME, DS_DEPTH_NAME },
            dmShaderData_.depth);
    }
    {
        const auto opaqueRenderSlot =
            shaderMgr_->GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE);
        const auto opaqueSlotData = shaderMgr_->GetRenderSlotData(opaqueRenderSlot);

        auto graphicsState = shaderMgr_->GetGraphicsState(opaqueSlotData.graphicsState);
        // only change needed compared to opaque slot is to disable color writes.
        graphicsState.colorBlendState.colorAttachments->colorWriteMask = 0U;

        const auto stateHash = shaderMgr_->HashGraphicsState(graphicsState, opaqueSlotData.renderSlotId);
        auto gsRenderHandleRef = shaderMgr_->GetGraphicsStateHandleByHash(stateHash);
        if (!gsRenderHandleRef) {
            const auto path = "3dshaderstates://" + to_hex(stateHash);
            const IShaderManager::GraphicsStateCreateInfo createInfo { path, graphicsState };
            const IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo {
                DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE, {}, {}, {}, 0U, false
            };
            gsRenderHandleRef = shaderMgr_->CreateGraphicsState(createInfo, variantCreateInfo);
        }
        if (gsRenderHandleRef) {
            dmShaderData_.gfxStateOcclusionMaterial = GetOrCreateEntityReference(ecs_, gsRenderHandleRef);
        }
    }
    {
        const auto environmentRenderSlot =
            shaderMgr_->GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_ENVIRONMENT);
        const auto environmentSlotData = shaderMgr_->GetRenderSlotData(environmentRenderSlot);

        auto graphicsState = shaderMgr_->GetGraphicsState(environmentSlotData.graphicsState);
        graphicsState.colorBlendState.colorAttachmentCount = 1U;
        // compared to environment slot blending is required.
        for (auto& outColor : graphicsState.colorBlendState.colorAttachments) {
            outColor.enableBlend = true;
            outColor.dstColorBlendFactor = BlendFactor::CORE_BLEND_FACTOR_ONE;
            outColor.dstAlphaBlendFactor = BlendFactor::CORE_BLEND_FACTOR_ONE;
            outColor.srcColorBlendFactor = BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
            outColor.srcAlphaBlendFactor = BlendFactor::CORE_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        }

        const auto stateHash = shaderMgr_->HashGraphicsState(graphicsState, environmentSlotData.renderSlotId);
        auto gsRenderHandleRef = shaderMgr_->GetGraphicsStateHandleByHash(stateHash);
        if (!gsRenderHandleRef) {
            const auto path = "3dshaderstates://" + to_hex(stateHash);
            const IShaderManager::GraphicsStateCreateInfo createInfo { path, graphicsState };
            const IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo {
                DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE, {}, {}, {}, 0U, false
            };
            gsRenderHandleRef = shaderMgr_->CreateGraphicsState(createInfo, variantCreateInfo);
        }
        if (gsRenderHandleRef) {
            dmShaderData_.gfxStateOcclusionEnvironment = GetOrCreateEntityReference(ecs_, gsRenderHandleRef);
        }
    }
    {
        const ComponentQuery::Operation operations[] = {
            { *nodeMgr_, ComponentQuery::Operation::REQUIRE },
            { *worldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *layerMgr_, ComponentQuery::Operation::OPTIONAL },
        };
        lightQuery_.SetEcsListenersEnabled(true);
        lightQuery_.SetupQuery(*lightMgr_, operations);
    }
    {
        const ComponentQuery::Operation operations[] = {
            { *worldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *layerMgr_, ComponentQuery::Operation::OPTIONAL },
            { *skinMgr_, ComponentQuery::Operation::OPTIONAL },
            { *jointMatricesMgr_, ComponentQuery::Operation::OPTIONAL },
            { *prevJointMatricesMgr_, ComponentQuery::Operation::OPTIONAL },
            { *nodeMgr_, ComponentQuery::Operation::OPTIONAL },
        };
        renderableQuery_.SetEcsListenersEnabled(true);
        renderableQuery_.SetupQuery(*renderMeshMgr_, operations, true);
    }
    {
        const ComponentQuery::Operation operations[] = {
            { *worldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *nodeMgr_, ComponentQuery::Operation::REQUIRE },
            { *renderMeshMgr_, ComponentQuery::Operation::REQUIRE },
        };
        reflectionsQuery_.SetEcsListenersEnabled(true);
        reflectionsQuery_.SetupQuery(*planarReflectionMgr_, operations);
    }
    {
        const ComponentQuery::Operation operations[] = {
            { *worldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *nodeMgr_, ComponentQuery::Operation::OPTIONAL },
            { *postProcessEffectMgr_, ComponentQuery::Operation::OPTIONAL },
        };
        cameraQuery_.SetEcsListenersEnabled(true);
        cameraQuery_.SetupQuery(*cameraMgr_, operations);
    }
    if (renderContext_) {
        IRenderDataStoreManager& rdsMgr = renderContext_->GetRenderDataStoreManager();
        if (auto dsPod = refcnt_ptr<IRenderDataStorePod>(rdsMgr.GetRenderDataStore(POD_DATA_STORE_NAME))) {
            auto podData = dsPod->Get(DefaultMaterialCameraConstants::CAMERA_REFLECTION_POST_PROCESS_PREFIX_NAME);
            if (!podData.empty()) {
                const auto& config = reinterpret_cast<const PostProcessConfiguration&>(podData[0U]);
                reflectionMaxMipBlur_ = Math::min(
                    config.blurConfiguration.maxMipLevel, DefaultMaterialCameraConstants::REFLECTION_PLANE_MIP_COUNT);
            }
        }
    }
    ecs_.AddListener(*planarReflectionMgr_, *this);
    ecs_.AddListener(*materialMgr_, *this);
    ecs_.AddListener(*meshMgr_, *this);
    ecs_.AddListener(*graphicsStateMgr_, *this);
}

bool RenderSystem::Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime)
{
    renderProcessing_.frameProcessed = false;
    if (!active_) {
        return false;
    }

    const auto renderConfigurationGen = renderConfigMgr_->GetGenerationCounter();
    const auto cameraGen = cameraMgr_->GetGenerationCounter();
    const auto lightGen = lightMgr_->GetGenerationCounter();
    const auto planarReflectionGen = planarReflectionMgr_->GetGenerationCounter();
    const auto environmentGen = environmentMgr_->GetGenerationCounter();
    const auto fogGen = fogMgr_->GetGenerationCounter();
    const auto postprocessGen = postProcessMgr_->GetGenerationCounter();
    const auto postprocessConfigurationGen = postProcessConfigMgr_->GetGenerationCounter();
    const auto postprocessEffectGen = postProcessEffectMgr_->GetGenerationCounter();
    const auto jointGen = jointMatricesMgr_->GetGenerationCounter();
    const auto layerGen = layerMgr_->GetGenerationCounter();
    const auto nodeGen = nodeMgr_->GetGenerationCounter();
    const auto renderMeshGen = renderMeshMgr_->GetGenerationCounter();
    const auto worldMatrixGen = worldMatrixMgr_->GetGenerationCounter();
    if (!frameRenderingQueued && (renderConfigurationGeneration_ == renderConfigurationGen) &&
        (cameraGeneration_ == cameraGen) && (lightGeneration_ == lightGen) &&
        (planarReflectionGeneration_ == planarReflectionGen) && (environmentGeneration_ == environmentGen) &&
        (fogGeneration_ == fogGen) && (postprocessGeneration_ == postprocessGen) &&
        (postprocessConfigurationGeneration_ == postprocessConfigurationGen) &&
        (postprocessEffectGeneration_ == postprocessEffectGen) && (jointGeneration_ == jointGen) &&
        (layerGeneration_ == layerGen) && (nodeGeneration_ == nodeGen) && (renderMeshGeneration_ == renderMeshGen) &&
        (worldMatrixGeneration_ == worldMatrixGen)) {
        return false;
    }

    renderConfigurationGeneration_ = renderConfigurationGen;
    cameraGeneration_ = cameraGen;
    lightGeneration_ = lightGen;
    planarReflectionGeneration_ = planarReflectionGen;
    environmentGeneration_ = environmentGen;
    fogGeneration_ = fogGen;
    postprocessGeneration_ = postprocessGen;
    postprocessConfigurationGeneration_ = postprocessConfigurationGen;
    postprocessEffectGeneration_ = postprocessEffectGen;
    jointGeneration_ = jointGen;
    layerGeneration_ = layerGen;
    nodeGeneration_ = nodeGen;
    renderMeshGeneration_ = renderMeshGen;
    worldMatrixGeneration_ = worldMatrixGen;

    totalTime_ = totalTime;
    deltaTime_ = deltaTime;

    if (dsMaterial_ && dsCamera_ && dsLight_ && dsScene_) {
        dsMaterial_->Clear();
        dsCamera_->Clear();
        dsLight_->Clear();
        dsScene_->Clear();
        FetchFullScene();
    } else {
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_ONCE_W("rs_data_stores_not_found", "CORE3D_VALIDATION: render system render data stores not found");
#endif
    }
    frameIndex_++;

    renderProcessing_.frameProcessed = true;
    return true;
}

void RenderSystem::Uninitialize()
{
    ecs_.RemoveListener(*planarReflectionMgr_, *this);
    ecs_.RemoveListener(*materialMgr_, *this);
    ecs_.RemoveListener(*meshMgr_, *this);
    ecs_.RemoveListener(*graphicsStateMgr_, *this);

    lightQuery_.SetEcsListenersEnabled(false);
    renderableQuery_.SetEcsListenersEnabled(false);
    reflectionsQuery_.SetEcsListenersEnabled(false);
    cameraQuery_.SetEcsListenersEnabled(false);
}

void RenderSystem::OnComponentEvent(
    EventType type, const IComponentManager& componentManager, array_view<const Entity> entities)
{
    if (componentManager.GetUid() == IMaterialComponentManager::UID) {
        if ((type == EventType::CREATED) || (type == EventType::MODIFIED)) {
            materialModifiedEvents_.append(entities.cbegin(), entities.cend());
        } else if (type == EventType::DESTROYED) {
            materialDestroyedEvents_.append(entities.cbegin(), entities.cend());
        }
        materialGeneration_ = componentManager.GetGenerationCounter();
    } else if (componentManager.GetUid() == IMeshComponentManager::UID) {
        if ((type == EventType::CREATED) || (type == EventType::MODIFIED)) {
            meshModifiedEvents_.append(entities.cbegin(), entities.cend());
        } else if (type == EventType::DESTROYED) {
            meshDestroyedEvents_.append(entities.cbegin(), entities.cend());
        }
        meshGeneration_ = componentManager.GetGenerationCounter();
    } else if (componentManager.GetUid() == IGraphicsStateComponentManager::UID) {
        if ((type == EventType::CREATED) || (type == EventType::MODIFIED)) {
            graphicsStateModifiedEvents_.append(entities.cbegin(), entities.cend());
        }
    } else if (componentManager.GetUid() == IPlanarReflectionComponentManager::UID) {
        if (type == EventType::CREATED) {
            for (const auto& entity : entities) {
                IComponentManager::ComponentId meshId = IComponentManager::INVALID_COMPONENT_ID;
                if (auto renderMeshHandle = renderMeshMgr_->Read(entity)) {
                    meshId = meshMgr_->GetComponentId(renderMeshHandle->mesh);
                }
                if (meshId == IComponentManager::INVALID_COMPONENT_ID) {
#if CORE3D_VALIDATION_ENABLED
                    CORE_LOG_ONCE_E(
                        "planar" + to_hex(entity.id), "CORE3D_VALIDATION: PlanarReflectionComponent missing mesh");
#endif
                    continue;
                }
                Entity material;
                if (auto meshHandle = meshMgr_->Read(meshId); meshHandle && !meshHandle->submeshes.empty()) {
                    material = meshHandle->submeshes[0U].material;
                }
                if (!EntityUtil::IsValid(material)) {
                    material = ecs_.GetEntityManager().Create();
                    reflectionPlanes_.insert({ entity, ReflectionPlaneData { material } });
                    if (auto meshWriteHandle = meshMgr_->Write(meshId)) {
                        meshWriteHandle->submeshes[0U].material = material;
                    }
                }
                auto materialHandle = materialMgr_->Write(material);
                if (!materialHandle) {
                    materialMgr_->Create(material);
                    materialHandle = materialMgr_->Write(material);
                    // With default roughness value of 1 reflection is barely visible. Lower the roughness so that by
                    // default the effect is more visible.
                    if (materialHandle) {
                        materialHandle->textures[MaterialComponent::TextureIndex::MATERIAL].factor.y = 0.2f;
                    }
                }
                if (materialHandle) {
                    materialModifiedEvents_.push_back(material);
                    if (!materialHandle->materialShader.shader) {
                        auto* uriCM = GetManager<IUriComponentManager>(ecs_);
                        constexpr const string_view uri = "3dshaders://shader/core3d_dm_fw_reflection_plane.shader";
                        auto shaderEntity = LookupResourceByUri(uri, *uriCM, *gpuHandleMgr_);
                        if (!EntityUtil::IsValid(shaderEntity)) {
                            shaderEntity = ecs_.GetEntityManager().Create();
                            gpuHandleMgr_->Create(shaderEntity);
                            gpuHandleMgr_->Write(shaderEntity)->reference = shaderMgr_->GetShaderHandle(uri);
                            uriCM->Create(shaderEntity);
                            uriCM->Write(shaderEntity)->uri = uri;
                        }
                        materialHandle->materialShader.shader =
                            ecs_.GetEntityManager().GetReferenceCounted(shaderEntity);
                    }
                    materialHandle->extraRenderingFlags = MaterialComponent::ExtraRenderingFlagBits::DISCARD_BIT;
                }
            }
        } else if (type == EventType::DESTROYED) {
            for (const auto& entity : entities) {
                if (auto pos = reflectionPlanes_.find(entity); pos != reflectionPlanes_.cend()) {
                    if (EntityUtil::IsValid(pos->second.material)) {
                        ecs_.GetEntityManager().Destroy(pos->second.material);
                    }
                    reflectionPlanes_.erase(pos);
                }
            }
        }
    }
}

RenderConfigurationComponent RenderSystem::GetRenderConfigurationComponent()
{
    for (IComponentManager::ComponentId i = 0; i < renderConfigMgr_->GetComponentCount(); i++) {
        const Entity id = renderConfigMgr_->GetEntity(i);
        if (nodeMgr_->Get(id).effectivelyEnabled) {
            return renderConfigMgr_->Get(i);
        }
    }
    return {};
}

Entity RenderSystem::ProcessScene(const RenderConfigurationComponent& sc)
{
    Entity cameraEntity { INVALID_ENTITY };
    // Grab active main camera.
    const auto cameraCount = cameraMgr_->GetComponentCount();
#if (CORE3D_VALIDATION_ENABLED == 1)
    bool hasActiveMainCamera = false;
#endif
    for (IComponentManager::ComponentId id = 0; id < cameraCount; ++id) {
        if (auto handle = cameraMgr_->Read(id); handle) {
            if ((handle->sceneFlags & CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT) &&
                (handle->sceneFlags & CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT)) {
                cameraEntity = cameraMgr_->GetEntity(id);
#if (CORE3D_VALIDATION_ENABLED == 1)
                hasActiveMainCamera = true;
#endif
                break;
            }
        }
    }
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (!hasActiveMainCamera) {
        CORE_LOG_ONCE_I(
            "RenderSystem::ProcessScene_no_active_main_cams", "CORE3D_VALIDATION: Main cameras are not active");
    }
#endif

    dsLight_->SetShadowTypes(GetRenderShadowTypes(sc), 0u);

    // NOTE: removed code for "No main camera set, grab 1st one (if any)."

    return cameraEntity;
}

void RenderSystem::EvaluateRenderDataStoreOutput()
{
    const auto info = dsMaterial_->GetRenderFrameObjectInfo();
    // update built-in pipeline modifications for default materials
    if (info.renderMaterialFlags & RenderMaterialFlagBits::RENDER_MATERIAL_TRANSMISSION_BIT) {
        // NOTE: should be alpha blend and not double sided, should be set by material author
        // automatically done by e.g. gltf2 importer
        renderProcessing_.frameFlags |= NEEDS_COLOR_PRE_PASS; // when allowing prepass on demand
    }

    CalculateFinalSceneBoundingSphere(
        info.shadowCasterBoundingSphere, sceneBoundingSpherePosition_, sceneBoundingSphereRadius_);
}

void RenderSystem::ProcessRenderables()
{
    renderableQuery_.Execute();

    IComponentManager::ComponentId jointId = IComponentManager::INVALID_COMPONENT_ID;
    IComponentManager::ComponentId prevJointId = IComponentManager::INVALID_COMPONENT_ID;
    const auto queryResults = renderableQuery_.GetResults();
    for (const auto& row : queryResults) {
        jointId = IComponentManager::INVALID_COMPONENT_ID;
        prevJointId = IComponentManager::INVALID_COMPONENT_ID;

        const auto& entity = row.entity;
        if (auto rmcHandle = renderMeshMgr_->Read(row.components[RQ_RMC])) {
            uint32_t sceneId = 0U;
            bool enabled = false; // not going to rendering if there's no node (could go..)
            if (row.IsValidComponentId(RQ_N)) {
                if (auto nodeHandle = nodeMgr_->Read(row.components[RQ_N]); nodeHandle) {
                    sceneId = nodeHandle->sceneId;
                    enabled = nodeHandle->effectivelyEnabled;
                }
            }
            if (!enabled) {
                continue;
            }
            RenderMeshBatchData renderMeshBatch;
            if (EntityUtil::IsValid(rmcHandle->renderMeshBatch)) {
                if (auto batchRenderMeshComponent = renderMeshMgr_->Read(rmcHandle->renderMeshBatch);
                    batchRenderMeshComponent) {
                    renderMeshBatch.renderMeshId = rmcHandle->renderMeshBatch.id;
                    renderMeshBatch.meshId = batchRenderMeshComponent->mesh.id;
                }
            }

            const WorldMatrixComponent& world = worldMatrixMgr_->Get(row.components[RQ_WM]);
            const uint64_t layerMask = !row.IsValidComponentId(RQ_L) ? LayerConstants::DEFAULT_LAYER_MASK
                                                                     : layerMgr_->Read(row.components[RQ_L])->layerMask;

            // this is a batch of same material, so the material uniform data is duplicated
            RenderMeshData rmd { world.matrix, world.matrix, world.prevMatrix, entity.id, rmcHandle->mesh.id, layerMask,
                sceneId };
            std::copy(std::begin(rmcHandle->customData), std::end(rmcHandle->customData), std::begin(rmd.customData));
            // Optional skin, cannot change based on submesh)
            RenderMeshSkinData rmsd;
            if (row.IsValidComponentId(RQ_SM) && row.IsValidComponentId(RQ_JM) && row.IsValidComponentId(RQ_PJM)) {
                jointId = row.components[RQ_JM];
                prevJointId = row.components[RQ_PJM];
                if (auto skin = skinMgr_->Read(row.components[RQ_SM])) {
                    rmsd.id = skin->skinRoot.id;
                } else {
                    static_assert(RenderSceneDataConstants::INVALID_INDEX == INVALID_ENTITY);
                    rmsd.id = RenderSceneDataConstants::INVALID_INDEX;
                }
                auto const jointMatricesData = jointMatricesMgr_->Read(jointId);
                auto const prevJointMatricesData = prevJointMatricesMgr_->Read(prevJointId);
                const SkinProcessData spd { &(*jointMatricesData), &(*prevJointMatricesData) };

                CORE_ASSERT(spd.prevJointMatricesComponent);
                rmsd.skinJointMatrices = array_view<Math::Mat4X4 const>(
                    spd.jointMatricesComponent->jointMatrices, spd.jointMatricesComponent->count);
                rmsd.prevSkinJointMatrices = array_view<Math::Mat4X4 const>(
                    spd.prevJointMatricesComponent->jointMatrices, spd.prevJointMatricesComponent->count);
                rmsd.aabb.minAabb = spd.jointMatricesComponent->jointsAabbMin;
                rmsd.aabb.maxAabb = spd.jointMatricesComponent->jointsAabbMax;
            }

            dsMaterial_->AddFrameRenderMeshData(rmd, rmsd, renderMeshBatch);
        }
    }

    // force submission
    dsMaterial_->SubmitFrameMeshData();
}

void RenderSystem::ProcessEnvironments(const RenderConfigurationComponent& renderConfig)
{
    if (!(environmentMgr_ && layerMgr_ && gpuHandleMgr_)) {
        return;
    }

    const RenderHandleReference graphicsState =
        (dsMaterial_->GetRenderFrameObjectInfo().renderMaterialFlags & RENDER_MATERIAL_OCCLUSION_BIT)
            ? gpuHandleMgr_->GetRenderHandleReference(dmShaderData_.gfxStateOcclusionEnvironment)
            : RenderHandleReference {};

    const auto envCount = environmentMgr_->GetComponentCount();
    for (IComponentManager::ComponentId id = 0; id < envCount; ++id) {
        if (ScopedHandle<const EnvironmentComponent> handle = environmentMgr_->Read(id); handle) {
            const EnvironmentComponent& component = *handle;

            const Entity envEntity = environmentMgr_->GetEntity(id);
            uint64_t layerMask = LayerConstants::DEFAULT_LAYER_MASK;
            if (auto layerHandle = layerMgr_->Read(envEntity); layerHandle) {
                layerMask = layerHandle->layerMask;
            }

            Entity probeTarget;
            if (EntityUtil::IsValid(component.reflectionProbe)) {
                auto rcm = GetManager<IReflectionProbeComponentManager>(ecs_);
                if (auto probe = rcm->Read(component.reflectionProbe); probe) {
                    if (auto probeCamera = cameraMgr_->Read(probe->probeCamera); probeCamera) {
                        if (!probeCamera->customColorTargets.empty() &&
                            EntityUtil::IsValid(probeCamera->customColorTargets[0U])) {
                            probeTarget = probeCamera->customColorTargets[0U];
                        }
                    }
                }
            }

            RenderCamera::Environment renderEnv;
            FillRenderEnvironment(
                *gpuHandleMgr_, layerMask, envEntity, component, probeTarget, renderEnv, *dynamicEnvBlendMgr_);

            renderEnv.graphicsState = graphicsState;

            // material custom resources (first check preferred custom resources)
            if (!component.customResources.empty()) {
                const size_t maxCustomCount = Math::min(component.customResources.size(),
                    static_cast<size_t>(RenderSceneDataConstants::MAX_ENV_CUSTOM_RESOURCE_COUNT));
                for (size_t idx = 0; idx < maxCustomCount; ++idx) {
                    renderEnv.customResourceHandles[idx] =
                        gpuHandleMgr_->GetRenderHandleReference(component.customResources[idx]);
                }
            }

            uint32_t blendEnvCount = 0;
            if (auto debc = dynamicEnvBlendMgr_->Read(component.blendEnvironments); debc) {
                blendEnvCount = Math::min(static_cast<uint32_t>(debc->environments.size()),
                    DefaultMaterialCameraConstants::MAX_CAMERA_MULTI_ENVIRONMENT_COUNT);
                for (uint32_t idx = 0U; idx < blendEnvCount; ++idx) {
                    if (EntityUtil::IsValid(debc->environments[idx])) {
                        renderEnv.multiEnvIds[renderEnv.multiEnvCount++] = debc->environments[idx].id;
                    }
                }
            }
            if (renderEnv.multiEnvCount > 0U) {
                FillMultiEnvironments(*environmentMgr_, renderEnv);
            }
            // check for main environment
            if (renderEnv.id == renderConfig.environment.id) {
                renderEnv.flags |= RenderCamera::Environment::EnvironmentFlagBits::ENVIRONMENT_FLAG_MAIN_BIT;
            }
            dsCamera_->AddEnvironment(renderEnv);
        }
    }
}

void RenderSystem::ProcessCameras(
    const RenderConfigurationComponent& renderConfig, const Entity& mainCameraEntity, RenderScene& renderScene)
{
    cameraQuery_.Execute();
    const auto queryResults = cameraQuery_.GetResults();
    if (queryResults.empty()) {
        return; // early out
    }

    // The scene camera and active render cameras are added here. ProcessReflections reflection cameras.
    // This is temporary when moving towards camera based rendering in 3D context.
    const uint32_t mainCameraId = cameraMgr_->GetComponentId(mainCameraEntity);
    const auto cameraCount = queryResults.size();
    vector<RenderCamera> tmpCameras;
    tmpCameras.reserve(cameraCount);
    unordered_map<uint64_t, uint64_t> mvChildToParent; // multi-view child to parent
    RenderScene::Flags sceneFlags { 0u };

    const auto hasOcclusionMaterial =
        dsMaterial_->GetRenderFrameObjectInfo().renderMaterialFlags & RENDER_MATERIAL_OCCLUSION_BIT;

    for (const auto& row : queryResults) {
        const auto id = row.components[0U];
        ScopedHandle<const CameraComponent> handle = cameraMgr_->Read(id);
        const CameraComponent& component = *handle;
        if ((mainCameraId != id) && ((component.sceneFlags & CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT) == 0)) {
            continue;
        }
        const Entity cameraEntity = row.entity;
        uint32_t level = 0U;
        if (auto nodeHandle = nodeMgr_->Read(row.components[2U])) {
            if (!nodeHandle->effectivelyEnabled) {
                continue;
            }
            level = nodeHandle->sceneId;
        }

        const WorldMatrixComponent renderMatrixComponent = worldMatrixMgr_->Get(row.components[1U]);

        renderProcessing_.frameFlags |=
            (component.pipelineFlags & CameraComponent::FORCE_COLOR_PRE_PASS_BIT) ? NEEDS_COLOR_PRE_PASS : 0;

        RenderCamera camera;
        FillRenderCameraBaseFromCameraComponent(*gpuHandleMgr_, *cameraMgr_, *gpuResourceMgr_, component, camera, true);
        // we add entity id as camera name if there isn't name (we need this for render node graphs)
        camera.id = cameraEntity.id;
        camera.name = GetCameraName(*nameMgr_, cameraEntity);
        if (camera.flags & RenderCamera::CAMERA_FLAG_CUBEMAP_BIT) {
            camera.flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_INVERSE_WINDING_BIT;
        }

        if (mainCameraId == id) {
            renderScene.cameraIndex = static_cast<uint32_t>(tmpCameras.size());
            camera.flags |= RenderCamera::CAMERA_FLAG_MAIN_BIT;
        }
        if (hasOcclusionMaterial) {
            camera.flags |= RenderCamera::CAMERA_FLAG_CLEAR_COLOR_BIT;
            camera.clearColorValues.float32[3U] = 0.f;
            camera.colorTargetCustomization->format = BASE_FORMAT_R16G16B16A16_SFLOAT;
        }
        camera.sceneId = level;

        float determinant = 0.0f;
        const Math::Mat4X4 view = Math::Inverse(Math::Mat4X4(renderMatrixComponent.matrix.data), determinant);

        bool isCameraNegative = determinant < 0.0f;
        const Math::Mat4X4 proj = CameraMatrixUtil::CalculateProjectionMatrix(component, isCameraNegative);
        if (isCameraNegative) {
            camera.flags |= RenderCamera::CAMERA_FLAG_INVERSE_WINDING_BIT;
        }

        camera.matrices.view = view;
        camera.matrices.proj = proj;
        const CameraData prevFrameCamData = UpdateAndGetPreviousFrameCameraData(cameraEntity, view, proj);
        camera.matrices.viewPrevFrame = prevFrameCamData.view;
        camera.matrices.projPrevFrame = prevFrameCamData.proj;
        // for orthographic projection use 90 degree perspective projection for the environment and for other
        // projections the same projection as for eveything else.
        if (component.projection == CameraComponent::Projection::ORTHOGRAPHIC) {
            camera.flags |= RenderCamera::CAMERA_FLAG_ENVIRONMENT_PROJECTION_BIT;
            auto aspect = 1.f;
            if (component.aspect > 0.f) {
                aspect = component.aspect;
            } else if (component.renderResolution.y > 0U) {
                aspect =
                    static_cast<float>(component.renderResolution.x) / static_cast<float>(component.renderResolution.y);
            }
            camera.matrices.envProj =
                Math::PerspectiveRhZo(90.f * BASE_NS::Math::DEG2RAD, aspect, component.zNear, component.zFar);
            camera.matrices.envProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
        } else {
            camera.matrices.envProj = camera.matrices.proj;
        }
        FillCameraRenderEnvironment(*dsCamera_, component, camera);
        camera.fog = GetRenderCameraFogFromComponent(layerMgr_, fogMgr_, renderConfig, component);
        camera.shaderFlags |=
            (camera.fog.id != RenderSceneDataConstants::INVALID_ID) ? RenderCamera::CAMERA_SHADER_FOG_BIT : 0U;

        auto postprocessEntity = component.postProcess;
        if (row.components[3U] != IComponentManager::INVALID_COMPONENT_ID) {
            postprocessEntity = row.entity;
            camera.flags |= RenderCamera::CAMERA_FLAG_POST_PROCESS_EFFECTS_BIT;
        } else if (postProcessEffectMgr_ && postProcessEffectMgr_->HasComponent(postprocessEntity)) {
            camera.flags |= RenderCamera::CAMERA_FLAG_POST_PROCESS_EFFECTS_BIT;
        }
        camera.postProcessName = GetPostProcessName(postProcessMgr_, postProcessConfigMgr_, postProcessEffectMgr_,
            nameMgr_, properties_.dataStoreScene, postprocessEntity);

        camera.customPostProcessRenderNodeGraphFile =
            GetPostProcessRenderNodeGraph(postProcessConfigMgr_, component.postProcess);

        // NOTE: setting up the color pre pass with a target name is a temporary solution
        uint64_t prePassCameraHash = 0U;
        const bool createPrePassCam = (component.pipelineFlags & CameraComponent::FORCE_COLOR_PRE_PASS_BIT) ||
                                      (component.pipelineFlags & CameraComponent::ALLOW_COLOR_PRE_PASS_BIT);
        if (createPrePassCam) {
            prePassCameraHash = Hash(camera.id, camera.id);
            camera.prePassColorTargetName = renderScene.name +
                                            DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME +
                                            to_hex(prePassCameraHash) + '_' + to_hex(prePassCameraHash);
        }
        tmpCameras.push_back(camera);
        ProcessCameraAddMultiViewHash(camera, mvChildToParent);
        // The order of setting cameras matter (main camera index is set already)
        if (createPrePassCam) {
            tmpCameras.push_back(CreateColorPrePassRenderCamera(
                *gpuHandleMgr_, *cameraMgr_, *gpuResourceMgr_, camera, component.prePassCamera, prePassCameraHash));
        }
        sceneFlags |= (camera.environment.flags & RenderCamera::Environment::ENVIRONMENT_FLAG_CAMERA_WEATHER_BIT)
                          ? RenderScene::RenderSceneFlagBits::SCENE_FLAG_WEATHER_BIT
                          : 0;
    }
    // add cameras to data store
    for (auto& cam : tmpCameras) {
        // fill multi-view info
        if (cam.flags & RenderCamera::CAMERA_FLAG_MULTI_VIEW_ONLY_BIT) {
            if (const auto iter = mvChildToParent.find(cam.id); iter != mvChildToParent.cend()) {
                cam.multiViewParentCameraId = iter->second;
            }
        }
        dsCamera_->AddCamera(cam);
    }
    renderScene.flags |= sceneFlags;
}

void RenderSystem::ProcessReflection(const ComponentQuery::ResultRow& row,
    const PlanarReflectionComponent& reflComponent, const RenderCamera& camera, const Math::UVec2 targetRes)
{
    // ReflectionsQuery has four required components:
    // (0) PlanarReflectionComponent
    // (1) WorldMatrixComponent
    // (2) NodeComponent
    // (3) RenderMeshComponent
    const WorldMatrixComponent reflectionPlaneMatrix = worldMatrixMgr_->Get(row.components[1u]);

    // cull plane (sphere) from camera
    bool insideFrustum = true;
    if (frustumUtil_ && picking_) {
        const RenderMeshComponent rmc = renderMeshMgr_->Get(row.components[3U]);
        if (const auto meshHandle = meshMgr_->Read(rmc.mesh); meshHandle) {
            // frustum planes created without jitter
            const Frustum frustum = frustumUtil_->CreateFrustum(camera.matrices.proj * camera.matrices.view);
            const auto mam =
                picking_->GetWorldAABB(reflectionPlaneMatrix.matrix, meshHandle->aabbMin, meshHandle->aabbMax);
            const float radius = Math::Magnitude(mam.maxAABB - mam.minAABB) * 0.5f;
            const Math::Vec3 pos = Math::Vec3(reflectionPlaneMatrix.matrix[3U]);
            insideFrustum = frustumUtil_->SphereFrustumCollision(frustum, pos, radius);
            // NOTE: add normal check for camera (cull based on normal and camera view)
        }
    }
    if (!insideFrustum) {
        return; // early out
    }

    // Calculate reflected view matrix from camera matrix.
    // Reflection plane.
    const Math::Vec3 translation = reflectionPlaneMatrix.matrix.w;
    const Math::Vec3 normal = Math::Normalize(Math::GetColumn(reflectionPlaneMatrix.matrix, 1));
    const float distance = -Math::Dot(normal, translation) - reflComponent.clipOffset;
    const Math::Vec4 plane { normal.x, normal.y, normal.z, distance };

    // Calculate mirror matrix from plane.
    const Math::Mat4X4 reflection = CalculateReflectionMatrix(plane);
    const Math::Mat4X4 reflectedView = camera.matrices.view * reflection;

    Math::Mat4X4 reflectedProjection = camera.matrices.proj;

    // NOTE: Should modify near plane of projection matrix to clip in to reflection plane.
    // This effectively optimizes away the un-wanted objects that are behind the plane
    // and otherwise would be visible in the reflection.
    // e.g.
    // CalculateCameraSpaceClipPlane()
    // calculate camera-space projection matrix that has clip plane as near plane.
    // CalculateObliqueProjectionMatrix()

    const Math::Vec4 cameraSpaceClipPlane = CalculateCameraSpaceClipPlane(reflectedView, translation, normal, -1.0f);
    CalculateObliqueProjectionMatrix(reflectedProjection, cameraSpaceClipPlane);

    // If the reflection plane has a postprocess pick the max mip level from blur config.
    auto maxMipBlur = reflectionMaxMipBlur_;
    if (EntityUtil::IsValid(reflComponent.postProcess)) {
        if (auto pp = postProcessMgr_->Read(reflComponent.postProcess)) {
            maxMipBlur = Math::min(maxMipBlur, pp->blurConfiguration.maxMipLevel);
        }
    }
    const ReflectionPlaneTargetUpdate rptu = UpdatePlaneReflectionTargetResolution(
        *gpuResourceMgr_, *gpuHandleMgr_, camera, row.entity, targetRes, maxMipBlur, reflComponent);
    if (rptu.recreated) {
        if (auto handle = planarReflectionMgr_->Write(row.components[0u])) {
            handle->renderTargetResolution[0] = rptu.renderTargetResolution[0];
            handle->renderTargetResolution[1] = rptu.renderTargetResolution[1];
            handle->colorRenderTarget = rptu.colorRenderTarget;
            handle->depthRenderTarget = rptu.depthRenderTarget;
        }
        const auto material = UpdateReflectionPlaneMaterial(
            *renderMeshMgr_, *meshMgr_, *materialMgr_, row.entity, reflComponent.screenPercentage, rptu);
        if (EntityUtil::IsValid(material)) {
            materialModifiedEvents_.push_back(material);
        }
    }

    RenderCamera reflCam;
    const uint64_t reflCamId = Hash(row.entity.id, camera.id);
    reflCam.id = reflCamId;
    reflCam.mainCameraId = camera.id; // link to main camera
    reflCam.sceneId = camera.sceneId;
    reflCam.reflectionId = static_cast<uint32_t>(row.entity.id & 0xFFFFFFFFU);
    reflCam.layerMask = reflComponent.layerMask;
    reflCam.matrices.view = reflectedView;
    reflCam.matrices.proj = reflectedProjection;
    const CameraData prevFrameCamData =
        UpdateAndGetPreviousFrameCameraData({ reflCamId }, reflCam.matrices.view, reflCam.matrices.proj);
    reflCam.matrices.viewPrevFrame = prevFrameCamData.view;
    reflCam.matrices.projPrevFrame = prevFrameCamData.proj;
    const auto xFactor = (static_cast<float>(camera.renderResolution[0]) * reflComponent.screenPercentage) /
                         static_cast<float>(rptu.renderTargetResolution[0]);
    const auto yFactor = (static_cast<float>(camera.renderResolution[1]) * reflComponent.screenPercentage) /
                         static_cast<float>(rptu.renderTargetResolution[1]);
    reflCam.viewport = { camera.viewport[0u] * xFactor, camera.viewport[1u] * yFactor, camera.viewport[2u] * xFactor,
        camera.viewport[3u] * yFactor };
    reflCam.depthTarget = gpuHandleMgr_->GetRenderHandleReference(rptu.depthRenderTarget);
    reflCam.colorTargets[0u] = gpuHandleMgr_->GetRenderHandleReference(rptu.colorRenderTarget);

    reflCam.renderResolution = { rptu.renderTargetResolution[0], rptu.renderTargetResolution[1] };
    reflCam.zNear = camera.zNear;
    reflCam.zFar = camera.zFar;
    reflCam.flags = (reflComponent.additionalFlags & PlanarReflectionComponent::FlagBits::MSAA_BIT)
                        ? RenderCamera::CAMERA_FLAG_MSAA_BIT
                        : 0U;
    reflCam.flags |= (RenderCamera::CAMERA_FLAG_REFLECTION_BIT | RenderCamera::CAMERA_FLAG_INVERSE_WINDING_BIT |
                      RenderCamera::CAMERA_FLAG_CUSTOM_TARGETS_BIT);
    reflCam.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
    reflCam.clearDepthStencil = camera.clearDepthStencil;
    reflCam.clearColorValues = camera.clearColorValues;
    reflCam.cullType = RenderCamera::CameraCullType::CAMERA_CULL_VIEW_FRUSTUM;
    reflCam.environment = camera.environment;
    reflCam.postProcessName = GetPostProcessName(postProcessMgr_, postProcessConfigMgr_, postProcessEffectMgr_,
        nameMgr_, properties_.dataStoreScene, reflComponent.postProcess);
    // If GetPostProcessName returned the default prefix replace with reflection postprocess prefix which matches the
    // default render data store.
    if (reflCam.postProcessName == DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_PREFIX_NAME) {
        reflCam.postProcessName = DefaultMaterialCameraConstants::CAMERA_REFLECTION_POST_PROCESS_PREFIX_NAME;
    }
    dsCamera_->AddCamera(reflCam);
}

void RenderSystem::ProcessReflections(const RenderScene& renderScene)
{
    const auto& cameras = dsCamera_->GetCameras();
    if (cameras.empty()) {
        return; // early out
    }

    reflectionsQuery_.Execute();
    const auto queryResults = reflectionsQuery_.GetResults();
    if (queryResults.empty()) {
        return; // early out
    }

    constexpr RenderCamera::Flags disableFlags { RenderCamera::CAMERA_FLAG_REFLECTION_BIT |
                                                 RenderCamera::CAMERA_FLAG_SHADOW_BIT |
                                                 RenderCamera::CAMERA_FLAG_OPAQUE_BIT };
    constexpr RenderCamera::Flags enableFlags { RenderCamera::CAMERA_FLAG_ALLOW_REFLECTION_BIT };
    for (const auto& row : queryResults) {
        // first loop all cameras to get the maximum size for this frame
        // might be visible with multiple cameras with different camera sizes
        // ReflectionsQuery has four required components:
        // (0) PlanarReflectionComponent
        // (1) WorldMatrixComponent
        // (2) NodeComponent
        // (3) RenderMeshComponent
        // Skip if this node is disabled.
        const NodeComponent nodeComponent = nodeMgr_->Get(row.components[2u]);
        if (!nodeComponent.effectivelyEnabled) {
            continue;
        }
        const PlanarReflectionComponent rc = planarReflectionMgr_->Get(row.components[0u]);
        if ((rc.additionalFlags & PlanarReflectionComponent::FlagBits::ACTIVE_RENDER_BIT) == 0) {
            continue;
        }
        Math::UVec2 targetRes = { 0U, 0U };
        for (const auto& cam : cameras) {
            if (((cam.flags & disableFlags) == 0) && ((cam.flags & enableFlags) != 0) &&
                (cam.sceneId == nodeComponent.sceneId)) {
                ProcessReflectionTargetSize(rc, cam, targetRes);
            }
        }
        // then process with correct frame target resolution
        for (const auto& cam : cameras) {
            if (((cam.flags & disableFlags) == 0) && ((cam.flags & enableFlags) != 0) &&
                (cam.sceneId == nodeComponent.sceneId)) {
                ProcessReflection(row, rc, cam, targetRes);
            }
        }
    }
}

void RenderSystem::ProcessLight(const LightProcessData& lpd)
{
    const auto& lc = lpd.lightComponent;
    RenderLight light { lpd.entity.id, lc.lightLayerMask,
        { lpd.world[3u], 1.0f }, // the last column (3) of the world matrix contains the world position.
        { Math::Normalize(lpd.world * Math::Vec4(0.0f, 0.0f, -1.0f, 0.0f)), 0.0f }, { lc.color, lc.intensity } };

    // See:
    // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
    const float outer = Math::clamp(lc.spotOuterAngle, lc.spotInnerAngle, Math::PI / 2.0f);
    const float inner = Math::clamp(lc.spotInnerAngle, 0.0f, outer);

    if (lc.type == LightComponent::Type::DIRECTIONAL) {
        light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT;
    } else if (lc.type == LightComponent::Type::POINT) {
        light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_POINT_LIGHT_BIT;
    } else if (lc.type == LightComponent::Type::SPOT) {
        light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_SPOT_LIGHT_BIT;

        const float cosInnerConeAngle = cosf(inner);
        const float cosOuterConeAngle = cosf(outer);

        const float lightAngleScale = 1.0f / Math::max(0.001f, cosInnerConeAngle - cosOuterConeAngle);
        const float lightAngleOffset = -cosOuterConeAngle * lightAngleScale;

        light.spotLightParams = { lightAngleScale, lightAngleOffset, inner, outer };
    } else if (lc.type == LightComponent::Type::RECT) {
        light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_RECT_LIGHT_BIT;
        light.lightUsageFlags |=
            lc.rectLight.twoSided ? RenderLight::LightUsageFlagBits::LIGHT_USAGE_TWO_SIDED_LIGHT_BIT : 0;

        const Math::Vec3 dir = Math::Normalize(light.dir);
        Math::Vec3 up = Math::Normalize(Math::Cross(dir, Math::Vec3(1.0f, 0.0f, 0.0f)));
        // Edge case, because dir can be paralell to (1, 0, 0).
        if (up.x == 0.0f && up.y == 0.0f && up.z == 0.0f) {
            up = Math::Vec3(0.0f, 1.0f, 0.0f);
        }
        const Math::Vec3 right = -Math::Cross(up, dir);

        light.dir = Math::Vec4(right * lc.rectLight.width, lc.rectLight.width);
        light.spotLightParams = Math::Vec4(up * lc.rectLight.height, lc.rectLight.height);
    }
    light.range = ComponentUtilFunctions::CalculateSafeLightRange(lc.range, lc.intensity);

    light.sceneId = lpd.sceneId;
    if (lc.shadowEnabled) {
        light.shadowFactors = { Math::clamp01(lc.shadowStrength), lc.shadowDepthBias, lc.shadowNormalBias, 0.0f };
        ProcessShadowCamera(lpd, light);
    }

    dsLight_->AddLight(light);
}

void RenderSystem::ProcessShadowCamera(const LightProcessData lpd, RenderLight& light)
{
    if ((light.lightUsageFlags &
            (RenderLight::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT | RenderLight::LIGHT_USAGE_SPOT_LIGHT_BIT)) == 0) {
        return;
    }
    light.shadowCameraIndex = static_cast<uint32_t>(dsCamera_->GetCameraCount());
    if (light.shadowCameraIndex >= DefaultMaterialCameraConstants::MAX_CAMERA_COUNT) {
        light.shadowCameraIndex = ~0u;
        light.lightUsageFlags &= (~RenderLight::LightUsageFlagBits::LIGHT_USAGE_SHADOW_LIGHT_BIT);
#if (CORE3D_VALIDATION_ENABLED == 1)
        const string onceName = string(to_string(light.id) + "_too_many_cam");
        CORE_LOG_ONCE_W(onceName, "CORE3D_VALIDATION: shadow camera dropped, too many cameras in scene");
#endif
        return; // early out
    }
    light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_SHADOW_LIGHT_BIT;

    float zNear = 0.0f;
    float zFar = 0.0f;
    RenderCamera camera;
    camera.id = SHADOW_CAMERA_START_UNIQUE_ID + light.shadowCameraIndex; // id used for easier uniqueness
    camera.sceneId = lpd.sceneId;
    camera.shadowId = lpd.entity.id;
    camera.layerMask = lpd.lightComponent.shadowLayerMask; // we respect light shadow rendering mask
    if (light.lightUsageFlags & RenderLight::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT) {
        // NOTE: modifies the light camera to follow center of scene
        // Add slight bias offset to radius.
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (std::isinf(lpd.renderScene.worldSceneBoundingSphereRadius)) {
            CORE_LOG_ONCE_W("inf_scene", "Infinite world bounding sphere, shadows won't be visible.");
        }
#endif
        const float nonZeroRadius = Math::max(lpd.renderScene.worldSceneBoundingSphereRadius, Math::EPSILON);
        const float radius = nonZeroRadius + nonZeroRadius * 0.05f;
        const Math::Vec3 lightPos =
            lpd.renderScene.worldSceneCenter - Math::Vec3(light.dir.x, light.dir.y, light.dir.z) * radius;
        camera.matrices.view = Math::LookAtRh(lightPos, lpd.renderScene.worldSceneCenter, { 0.0f, 1.0f, 0.0f });
        camera.matrices.proj = Math::OrthoRhZo(-radius, radius, -radius, radius, 0.001f, radius * 2.0f);
        zNear = 0.0f;
        zFar = 6.0f;
    } else if (light.lightUsageFlags & RenderLight::LIGHT_USAGE_SPOT_LIGHT_BIT) {
        float determinant = 0.0f;
        camera.matrices.view = Math::Inverse(lpd.world, determinant);
        const float yFov = Math::clamp(light.spotLightParams.w * 2.0f, 0.0f, Math::PI);
        zFar = light.range; // use light range for z far
        zNear = Math::max(0.1f, lpd.lightComponent.nearPlane);
        camera.matrices.proj = Math::PerspectiveRhZo(yFov, 1.0f, zNear, zFar);
    }

    camera.matrices.proj[1][1] *= -1.0f; // left-hand NDC while Vulkan right-handed -> flip y

    const CameraData prevFrameCamData =
        UpdateAndGetPreviousFrameCameraData(lpd.entity, camera.matrices.viewPrevFrame, camera.matrices.projPrevFrame);
    camera.matrices.viewPrevFrame = prevFrameCamData.view;
    camera.matrices.projPrevFrame = prevFrameCamData.proj;
    camera.viewport = { 0.0f, 0.0f, 1.0f, 1.0f };
    // get current default resolution
    camera.renderResolution = dsLight_->GetShadowQualityResolution();
    // NOTE: custom shadow camera targets not yet supported
    camera.zNear = zNear;
    camera.zFar = zFar;
    camera.flags = RenderCamera::CAMERA_FLAG_SHADOW_BIT;
    camera.cullType = RenderCamera::CameraCullType::CAMERA_CULL_VIEW_FRUSTUM;

    dsCamera_->AddCamera(camera);
}

void RenderSystem::ProcessLights(RenderScene& renderScene)
{
    lightQuery_.Execute();

    uint32_t spotLightIndex = 0;
    for (const auto& row : lightQuery_.GetResults()) {
        // In addition to the base our lightQuery has two required components and two optional components:
        // (0) LightComponent
        // (1) NodeComponent
        // (2) WorldMatrixComponent
        const NodeComponent nodeComponent = nodeMgr_->Get(row.components[1u]);
        if (nodeComponent.effectivelyEnabled) {
            const LightComponent lightComponent = lightMgr_->Get(row.components[0u]);
            const WorldMatrixComponent renderMatrixComponent = worldMatrixMgr_->Get(row.components[2u]);
            const uint64_t layerMask = !row.IsValidComponentId(3u) ? LayerConstants::DEFAULT_LAYER_MASK
                                                                   : layerMgr_->Get(row.components[3u]).layerMask;
            const Math::Mat4X4 world(renderMatrixComponent.matrix.data);
            const LightProcessData lpd { layerMask, nodeComponent.sceneId, row.entity, lightComponent, world,
                renderScene, spotLightIndex };
            ProcessLight(lpd);
        }
    }
}

void RenderSystem::ProcessPostProcesses(const Entity& mainCameraEntity)
{
    ProcessPostProcessComponents(mainCameraEntity);
    ProcessPostProcessConfigurationComponents();
    ProcessPostProcessEffectComponents();
}

void RenderSystem::ProcessPostProcessComponents(const Entity& mainCameraEntity)
{
    if (!renderContext_ || !postProcessMgr_) {
        return;
    }
    IRenderDataStoreManager& rdsMgr = renderContext_->GetRenderDataStoreManager();
    auto dsPod = refcnt_ptr<IRenderDataStorePod>(rdsMgr.GetRenderDataStore(POD_DATA_STORE_NAME));
    if (!dsPod) {
        return;
    }

    const auto postProcessCount = postProcessMgr_->GetComponentCount();
    if (!postProcessCount) {
        return;
    }

    const auto screenPercentage = [](const ICameraComponentManager* cameraMgr, const Entity& mainCameraEntity) {
        if (ScopedHandle<const CameraComponent> cameraHandle = cameraMgr->Read(mainCameraEntity)) {
            return Math::clamp(cameraHandle->screenPercentage, 0.25f, 1.0f);
        }
        return 1.f;
    }(cameraMgr_, mainCameraEntity);

    const float upscaleRatio = 1.0f / screenPercentage;

    for (IComponentManager::ComponentId id = 0; id < postProcessCount; ++id) {
        const auto handle = postProcessMgr_->Read(id);
        // in reality it shouldn't be possible to get an invalid handle.
        if (!handle) {
            continue;
        }
        const auto& pp = *handle;

        // just copy values (no support for fog control)
        PostProcessConfiguration ppConfig;
        ppConfig.enableFlags = pp.enableFlags;
        ppConfig.bloomConfiguration = pp.bloomConfiguration;
        ppConfig.vignetteConfiguration = pp.vignetteConfiguration;
        ppConfig.colorFringeConfiguration = pp.colorFringeConfiguration;
        ppConfig.ditherConfiguration = pp.ditherConfiguration;
        ppConfig.blurConfiguration = pp.blurConfiguration;
        ppConfig.colorConversionConfiguration = pp.colorConversionConfiguration;
        ppConfig.tonemapConfiguration = pp.tonemapConfiguration;
        ppConfig.fxaaConfiguration = pp.fxaaConfiguration;
        ppConfig.taaConfiguration = pp.taaConfiguration;
        ppConfig.dofConfiguration = pp.dofConfiguration;
        ppConfig.motionBlurConfiguration = pp.motionBlurConfiguration;
        ppConfig.lensFlareConfiguration = pp.lensFlareConfiguration;
        ppConfig.upscaleConfiguration.ratio =
            pp.upscaleConfiguration.ratio == 1.0f ? upscaleRatio : pp.upscaleConfiguration.ratio;
        ppConfig.upscaleConfiguration.smoothScale = pp.upscaleConfiguration.smoothScale;
        ppConfig.upscaleConfiguration.structureSensitivity = pp.upscaleConfiguration.structureSensitivity;
        ppConfig.upscaleConfiguration.edgeSharpness = pp.upscaleConfiguration.edgeSharpness;

        const Entity ppEntity = postProcessMgr_->GetEntity(id);
        const auto ppName = GetPostProcessName(nameMgr_, properties_.dataStoreScene, ppEntity, false);
        // NOTE: camera based new post process interface integration
        RecalculatePostProcesses(ppName, ppConfig);
        auto const dataView = dsPod->Get(ppName);
        if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
            dsPod->Set(ppName, arrayviewU8(ppConfig));
        } else {
            renderProcessing_.postProcessPods.emplace_back(ppName);
            dsPod->CreatePod(POST_PROCESS_NAME, ppName, arrayviewU8(ppConfig));
        }
    }
}

void RenderSystem::ProcessPostProcessConfigurationComponents()
{
    if (!renderContext_ || !postProcessConfigMgr_) {
        return;
    }
    IRenderDataStoreManager& rdsMgr = renderContext_->GetRenderDataStoreManager();

    auto dsPp = refcnt_ptr<IRenderDataStorePostProcess>(rdsMgr.GetRenderDataStore(PP_DATA_STORE_NAME));
    if (!dsPp) {
        return;
    }
    const auto postProcessConfigCount = postProcessConfigMgr_->GetComponentCount();
    for (IComponentManager::ComponentId id = 0; id < postProcessConfigCount; ++id) {
        // NOTE: should check if nothing has changed and not copy data if it has not changed
        const auto handle = postProcessConfigMgr_->Read(id);
        // in reality it shouldn't be possible to get an invalid handle.
        if (!handle) {
            continue;
        }
        const Entity ppEntity = postProcessConfigMgr_->GetEntity(id);
        const auto ppName = GetPostProcessName(nameMgr_, properties_.dataStoreScene, ppEntity, false);
        if (!dsPp->Contains(ppName)) {
            renderProcessing_.postProcessConfigs.emplace_back(ppName);
            dsPp->Create(ppName);
        }
        for (const auto& ref : handle->postProcesses) {
            const IRenderDataStorePostProcess::PostProcess::Variables vars = FillPostProcessConfigurationVars(ref);
            if (dsPp->Contains(ppName, ref.name)) {
                dsPp->Set(ppName, ref.name, vars);
            } else {
                RenderHandleReference shader = gpuHandleMgr_->GetRenderHandleReference(ref.shader);
                dsPp->Create(ppName, ref.name, move(shader));
                dsPp->Set(ppName, ref.name, vars);
            }
        }
    }
}

void RenderSystem::ProcessPostProcessEffectComponents()
{
    if (!renderContext_ || !postProcessEffectMgr_) {
        return;
    }
    if (!dsRenderPostProcesses_) {
        return;
    }

    const auto postProcessEffectCount = postProcessEffectMgr_->GetComponentCount();
    for (IComponentManager::ComponentId id = 0; id < postProcessEffectCount; ++id) {
        // NOTE: should check if nothing has changed and not copy data if it has not changed
        const auto handle = postProcessEffectMgr_->Read(id);
        // in reality it shouldn't be possible to get an invalid handle.
        if (!handle || handle->effects.empty()) {
            continue;
        }
        const Entity ppEntity = postProcessEffectMgr_->GetEntity(id);

        BASE_NS::vector<IRenderDataStoreRenderPostProcesses::PostProcessData> ppd;
        uint64_t ppId = 0U;
        for (const auto& ref : handle->effects) {
            if (ref) {
                ppd.push_back({ ppId++, ref });
            }
        }
        if (!ppd.empty()) {
            const auto ppName = GetPostProcessName(nameMgr_, properties_.dataStoreScene, ppEntity, true);
            dsRenderPostProcesses_->AddData(ppName, ppd);
        }
    }
}

void RenderSystem::RecalculatePostProcesses(BASE_NS::string_view name, RENDER_NS::PostProcessConfiguration& ppConfig)
{
    // process only new post process interfaces for cameras
    if (ppConfig.enableFlags & PostProcessConfiguration::ENABLE_LENS_FLARE_BIT) {
        // fetch cameras if camera post process (should be)
        const auto& cameras = dsCamera_->GetCameras();
        for (const auto& camRef : cameras) {
            if (camRef.postProcessName == name) {
                const Math::Vec3 p = -ppConfig.lensFlareConfiguration.flarePosition;
                const Math::Vec3 flareDir = BASE_NS::Math::Normalize(p);
                // NOTE: the camera view processing should be per camera in RenderNodeDefaultCameraPostProcessController
                // there the inverse is already calculated
                const Math::Vec3 camPos = Math::Inverse(camRef.matrices.view)[3U];
                const float camFar = camRef.zFar;

                const Math::Mat4X4 viewProj = camRef.matrices.proj * camRef.matrices.view;

                Math::Vec4 viewPos = Math::Vec4(camPos + flareDir * camFar, 1.0f);
                Math::Vec4 clipSpacePos = viewProj * viewPos;

                const float rW = clipSpacePos.w == 0.0f ? 0.0f : (1.0f / clipSpacePos.w);
                Math::Vec3 flarePosProj = Math::Vec3(clipSpacePos.x * rW, clipSpacePos.y * rW, clipSpacePos.z * rW);
                flarePosProj.x = 0.5f + (flarePosProj.x * 0.5f);
                flarePosProj.y = 0.5f + (flarePosProj.y * 0.5f);
                // bake the sign in for culling
                const float zSign = (clipSpacePos.z < 0.0f) ? 1.0f : -1.0f;
                flarePosProj.z = (0.5f + (flarePosProj.z * 0.5f)) * zSign;

                // calculate flare pos for render post process
                ppConfig.lensFlareConfiguration.flarePosition = flarePosProj;

                break;
            }
        }
    }
}

void RenderSystem::DestroyRenderDataStores()
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
        // check that render context is still alive
        if (auto renderContext =
                GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
            renderContext) {
            IRenderDataStoreManager& rdsMgr = renderContext_->GetRenderDataStoreManager();
            if (auto dataStore = refcnt_ptr<IRenderDataStorePod>(rdsMgr.GetRenderDataStore(POD_DATA_STORE_NAME))) {
                for (const auto& ref : renderProcessing_.postProcessPods) {
                    dataStore->DestroyPod(POST_PROCESS_NAME, ref.c_str());
                }
            }
            if (auto dataStore =
                    refcnt_ptr<IRenderDataStorePostProcess>(rdsMgr.GetRenderDataStore(PP_DATA_STORE_NAME))) {
                for (const auto& ref : renderProcessing_.postProcessConfigs) {
                    dataStore->Destroy(ref.c_str());
                }
            }
        }
    }
}

void RenderSystem::FetchFullScene()
{
    if (!active_) {
        return;
    }

#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "RenderSystem", "FetchFullScene", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    if ((materialGeneration_ != materialMgr_->GetGenerationCounter()) ||
        (meshGeneration_ != meshMgr_->GetGenerationCounter())) {
        ecs_.ProcessEvents();
    }

    // Process scene settings (if present), look up first active scene.
    const RenderConfigurationComponent renderConfig = GetRenderConfigurationComponent();
    const Entity cameraEntity = ProcessScene(renderConfig);

    RenderScene renderDataScene;
    renderDataScene.customRenderNodeGraphFile = renderConfig.customRenderNodeGraphFile;
    renderDataScene.customPostSceneRenderNodeGraphFile = renderConfig.customPostSceneRenderNodeGraphFile;
    renderDataScene.name = properties_.dataStoreScene;
    renderDataScene.dataStoreNamePrefix = properties_.dataStorePrefix;
    renderDataScene.dataStoreNameCamera = properties_.dataStoreCamera;
    renderDataScene.dataStoreNameLight = properties_.dataStoreLight;
    renderDataScene.dataStoreNameMaterial = properties_.dataStoreMaterial;
    renderDataScene.dataStoreNameMorph = properties_.dataStoreMorph;
    constexpr double uToMsDiv = 1000.0;
    constexpr double uToSDiv = 1000000.0;
    renderDataScene.sceneDeltaTime =
        static_cast<float>(static_cast<double>(deltaTime_) / uToMsDiv); // real delta time used for scene as well
    renderDataScene.totalTime = static_cast<float>(static_cast<double>(totalTime_) / uToSDiv);
    renderDataScene.deltaTime = renderDataScene.sceneDeltaTime;
    renderDataScene.frameIndex = static_cast<uint32_t>((frameIndex_ % std::numeric_limits<uint32_t>::max()));
    renderProcessing_.frameFlags = 0; // zero frame flags for camera processing

    if (!graphicsStateModifiedEvents_.empty()) {
        HandleGraphicsStateEvents();
    }

    if (const auto generation = meshMgr_->GetGenerationCounter(); meshGeneration_ != generation) {
        const auto meshes = meshMgr_->GetComponentCount();
        for (IComponentManager::ComponentId id = 0U; id < meshes; ++id) {
            if (meshMgr_->GetComponentGeneration(id) > meshGeneration_) {
                meshModifiedEvents_.push_back(meshMgr_->GetEntity(id));
            }
        }
        meshGeneration_ = generation;
    }

    if (!materialModifiedEvents_.empty() || !materialDestroyedEvents_.empty()) {
        HandleMaterialEvents();
    }
    // material events needs to be handled before mesh events
    if (!meshModifiedEvents_.empty() || !meshDestroyedEvents_.empty()) {
        HandleMeshEvents();
    }

    // Process all render components.
    ProcessRenderables();

    ProcessEnvironments(renderConfig);
    ProcessCameras(renderConfig, cameraEntity, renderDataScene);
    ProcessReflections(renderDataScene);
    ProcessPostProcesses(cameraEntity);

    // fill frame flags after renderable processing
    // fill shadow caster bounding spheres after renderable processing
    EvaluateRenderDataStoreOutput();

    // Process render node graphs automatically based on camera if needed bits set for properties
    // Some materials might request color pre-pass etc. (needs to be done after renderables are processed)
    ProcessRenderNodeGraphs(renderConfig, renderDataScene);

    renderDataScene.worldSceneCenter = sceneBoundingSpherePosition_;
    renderDataScene.worldSceneBoundingSphereRadius = sceneBoundingSphereRadius_;

    // Process lights.
    ProcessLights(renderDataScene);

    dsScene_->SetScene(renderDataScene);

    // Remove prev frame data from not used cameras
    for (auto iter = cameraData_.begin(); iter != cameraData_.end();) {
        if (iter->second.lastFrameIndex != frameIndex_) {
            iter = cameraData_.erase(iter);
        } else {
            ++iter;
        }
    }
}

void RenderSystem::ProcessRenderNodeGraphs(
    const RenderConfigurationComponent& renderConfig, const RenderScene& renderScene)
{
    auto& orderedRngs = renderProcessing_.orderedRenderNodeGraphs;
    orderedRngs.clear();
    if (!(renderConfig.renderingFlags & RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT)) {
        return;
    }

    const auto& renderCameras = dsCamera_->GetCameras();
    const vector<CameraOrdering> baseCameras =
        SortCameras(renderCameras, (renderProcessing_.frameFlags & NEEDS_COLOR_PRE_PASS));

    // first create scene render node graph if needed
    // we need to have scene render node graph as a separate
    orderedRngs.push_back(GetSceneRenderNodeGraph(renderScene));

    // then, add valid camera render node graphs
    for (const auto& cam : baseCameras) {
        CORE_ASSERT(cam.renderCameraIdx < renderCameras.size());
        const auto& camRef = renderCameras[cam.renderCameraIdx];
        CORE_ASSERT(camRef.id != 0xFFFFFFFFffffffff); // there must be an id for uniqueness
        CameraRngsOutput camRngs = GetCameraRenderNodeGraphs(renderScene, camRef);
        if (!camRngs.rngs.rngHandle) {
            continue;
        }
        orderedRngs.push_back(move(camRngs.rngs.rngHandle));
        if (camRngs.rngs.ppRngHandle) {
            orderedRngs.push_back(move(camRngs.rngs.ppRngHandle));
        }
        for (uint32_t mvIdx = 0U; mvIdx < RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT; ++mvIdx) {
            if (camRngs.multiviewPpHandles[mvIdx]) {
                orderedRngs.push_back(move(camRngs.multiviewPpHandles[mvIdx]));
            }
        }
    }
    // then possible post scene custom render node graph
    if (renderProcessing_.sceneRngs.customPostRng) {
        orderedRngs.push_back(renderProcessing_.sceneRngs.customPostRng);
    }
    // destroy unused after two frames
    const uint64_t ageLimit = (frameIndex_ < 2) ? 0 : (frameIndex_ - 2);
    for (auto iter = renderProcessing_.camIdToRng.begin(); iter != renderProcessing_.camIdToRng.end();) {
        if (iter->second.lastFrameIndex < ageLimit) {
            iter = renderProcessing_.camIdToRng.erase(iter);
        } else {
            ++iter;
        }
    }
}

RenderSystem::CameraRngsOutput RenderSystem::GetCameraRenderNodeGraphs(
    const RenderScene& renderScene, const RenderCamera& renderCamera)
{
    CameraRngsOutput rngs;
    if (renderCamera.customRenderNodeGraph) {
        rngs.rngs.rngHandle = renderCamera.customRenderNodeGraph;
        return rngs;
    }
    constexpr uint32_t rngChangeFlags =
        RenderCamera::CAMERA_FLAG_MSAA_BIT | RenderCamera::CAMERA_FLAG_CUSTOM_TARGETS_BIT;
    auto createNewRngs = [](auto& rngm, const auto& rnUtil, const auto& scene, const auto& obj, const auto& mvCams) {
        const auto descs = rnUtil->GetRenderNodeGraphDescs(scene, obj, 0, mvCams);
        CameraRngsOutput rngs;
        rngs.rngs.rngHandle = rngm.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, descs.camera, {}, scene.name);
        if (!descs.postProcess.nodes.empty()) {
            rngs.rngs.ppRngHandle =
                rngm.Create(IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                    descs.postProcess, {}, scene.name);
        }
        for (size_t mvIdx = 0; mvIdx < mvCams.size(); ++mvIdx) {
            rngs.multiviewPpHandles[mvIdx] =
                rngm.Create(IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC,
                    descs.multiViewCameraPostProcesses[mvIdx], {}, scene.name);
        }
        return rngs;
    };

    IRenderNodeGraphManager& rngm = renderContext_->GetRenderNodeGraphManager();
    if (auto iter = renderProcessing_.camIdToRng.find(renderCamera.id); iter != renderProcessing_.camIdToRng.cend()) {
        // NOTE: not optimal, currently re-creates a render node graph if:
        // * msaa flags have changed
        // * post process name / component has changed
        // * pipeline has changed
        // * rng files have changed
        // * multi-view count has changed
        const bool reCreate =
            ((iter->second.flags & rngChangeFlags) != (renderCamera.flags & rngChangeFlags)) ||
            (iter->second.postProcessName != renderCamera.postProcessName) ||
            (iter->second.renderPipelineType != renderCamera.renderPipelineType) ||
            (iter->second.customRngFile != renderCamera.customRenderNodeGraphFile) ||
            (iter->second.customPostProcessRngFile != renderCamera.customPostProcessRenderNodeGraphFile) ||
            (iter->second.multiViewCameraCount != renderCamera.multiViewCameraCount) ||
            (iter->second.multiViewCameraHash != renderCamera.multiViewCameraHash);
        if (reCreate) {
            iter->second.rngs = {};
            const vector<RenderCamera> multiviewCameras = GetMultiviewCameras(renderCamera);
            rngs = createNewRngs(rngm, renderUtil_, renderScene, renderCamera, multiviewCameras);
            // copy
            iter->second.rngs = rngs.rngs;
            // update multiview post process
            for (size_t mvIdx = 0; mvIdx < multiviewCameras.size(); ++mvIdx) {
                const auto& mvCamera = multiviewCameras[mvIdx];
                auto& mvData = renderProcessing_.camIdToRng[mvCamera.id];
                mvData.rngs.ppRngHandle = rngs.multiviewPpHandles[mvIdx];
                mvData.lastFrameIndex = frameIndex_;
            }
        } else {
            // found and copy the handles
            rngs.rngs = iter->second.rngs;
            // multiview post processes
            for (uint32_t mvIdx = 0; mvIdx < renderCamera.multiViewCameraCount; ++mvIdx) {
                auto& mvData = renderProcessing_.camIdToRng[renderCamera.multiViewCameraIds[mvIdx]];
                rngs.multiviewPpHandles[mvIdx] = mvData.rngs.ppRngHandle;
                mvData.lastFrameIndex = frameIndex_;
            }
        }
        iter->second.flags = renderCamera.flags;
        iter->second.renderPipelineType = renderCamera.renderPipelineType;
        iter->second.lastFrameIndex = frameIndex_;
        iter->second.postProcessName = renderCamera.postProcessName;
        iter->second.customRngFile = renderCamera.customRenderNodeGraphFile;
        iter->second.customPostProcessRngFile = renderCamera.customPostProcessRenderNodeGraphFile;
        iter->second.multiViewCameraCount = renderCamera.multiViewCameraCount;
        iter->second.multiViewCameraHash = renderCamera.multiViewCameraHash;
    } else {
        const vector<RenderCamera> multiviewCameras = GetMultiviewCameras(renderCamera);
        rngs = createNewRngs(rngm, renderUtil_, renderScene, renderCamera, multiviewCameras);
        renderProcessing_.camIdToRng[renderCamera.id] = { rngs.rngs, renderCamera.flags,
            renderCamera.renderPipelineType, frameIndex_, renderCamera.postProcessName,
            renderCamera.customRenderNodeGraphFile, renderCamera.customPostProcessRenderNodeGraphFile,
            renderCamera.multiViewCameraCount, renderCamera.multiViewCameraHash };
        // update multiview post process
        for (size_t mvIdx = 0; mvIdx < multiviewCameras.size(); ++mvIdx) {
            const auto& mvCamera = multiviewCameras[mvIdx];
            auto& mvData = renderProcessing_.camIdToRng[mvCamera.id];
            mvData.rngs.ppRngHandle = rngs.multiviewPpHandles[mvIdx];
            mvData.lastFrameIndex = frameIndex_;
        }
    }
    return rngs;
}

RenderHandleReference RenderSystem::GetSceneRenderNodeGraph(const RenderScene& renderScene)
{
    IRenderNodeGraphManager& rngm = renderContext_->GetRenderNodeGraphManager();
    auto createNewRng = [](auto& rngm, const auto& rnUtil, const auto& scene) {
        const RenderNodeGraphDesc desc = rnUtil->GetRenderNodeGraphDesc(scene, 0);
        return rngm.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, desc, {}, scene.name);
    };
    auto createNewCustomRng = [](auto& rngm, const auto& rnUtil, const auto& scene, const auto& rngFile) {
        const RenderNodeGraphDesc desc = rnUtil->GetRenderNodeGraphDesc(scene, rngFile, 0);
        return rngm.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, desc, {}, scene.name);
    };

    // first check if there's custom render node graph file
    // if not, use the default
    RenderHandleReference handle;
    if (!renderScene.customRenderNodeGraphFile.empty()) {
        const bool reCreate = (renderProcessing_.sceneRngs.customRngFile != renderScene.customRenderNodeGraphFile);
        if (reCreate) {
            renderProcessing_.sceneRngs.customRng = createNewRng(rngm, renderUtil_, renderScene);
        }
        handle = renderProcessing_.sceneRngs.customRng;
        renderProcessing_.sceneRngs.customRngFile = renderScene.customRenderNodeGraphFile;
    } else {
        // clear
        renderProcessing_.sceneRngs.customRng = {};
        renderProcessing_.sceneRngs.customRngFile.clear();
    }
    if (!handle) {
        if ((!renderProcessing_.sceneRngs.rng)) {
            renderProcessing_.sceneRngs.rng = createNewRng(rngm, renderUtil_, renderScene);
        }
        handle = renderProcessing_.sceneRngs.rng;
    }

    // process custom post scene render node graph
    // NOTE: it is not returned by the method
    if (!renderScene.customPostSceneRenderNodeGraphFile.empty()) {
        const bool reCreate =
            (renderProcessing_.sceneRngs.customPostSceneRngFile != renderScene.customPostSceneRenderNodeGraphFile);
        if (reCreate) {
            renderProcessing_.sceneRngs.customPostRng =
                createNewCustomRng(rngm, renderUtil_, renderScene, renderScene.customPostSceneRenderNodeGraphFile);
        }
        renderProcessing_.sceneRngs.customPostSceneRngFile = renderScene.customPostSceneRenderNodeGraphFile;
    } else {
        // clear
        renderProcessing_.sceneRngs.customPostRng = {};
        renderProcessing_.sceneRngs.customPostSceneRngFile.clear();
    }

    return handle;
}

array_view<const RenderHandleReference> RenderSystem::GetRenderNodeGraphs() const
{
    if (renderProcessing_.frameProcessed) {
        return renderProcessing_.orderedRenderNodeGraphs;
    } else {
        return {};
    }
}

RenderSystem::CameraData RenderSystem::UpdateAndGetPreviousFrameCameraData(
    const Entity& entity, const Math::Mat4X4& view, const Math::Mat4X4& proj)
{
    CameraData currData = { view, proj, frameIndex_ };
    if (auto iter = cameraData_.find(entity); iter != cameraData_.end()) {
        const CameraData prevData = iter->second;
        iter->second = currData;
        return prevData; // correct previous frame matrices
    } else {
        cameraData_.insert_or_assign(entity, currData);
        return currData; // current frame returned because of no prev frame matrices
    }
}

vector<RenderCamera> RenderSystem::GetMultiviewCameras(const RenderCamera& renderCamera)
{
    vector<RenderCamera> mvCameras;
    if (renderCamera.multiViewCameraCount > 0U) {
        const auto& cameras = dsCamera_->GetCameras();
        for (uint32_t camIdx = 0; camIdx < renderCamera.multiViewCameraCount; ++camIdx) {
            const uint32_t ci = dsCamera_->GetCameraIndex(renderCamera.multiViewCameraIds[camIdx]);
            if (ci < cameras.size()) {
                mvCameras.push_back(cameras[ci]);
            }
        }
    }
    return mvCameras;
}

void RenderSystem::HandleMaterialEvents() noexcept
{
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "RenderSystem", "HandleMaterialEvents", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    if (!materialDestroyedEvents_.empty()) {
        std::sort(materialDestroyedEvents_.begin(), materialDestroyedEvents_.end());
    }

    if (!materialModifiedEvents_.empty()) {
        std::sort(materialModifiedEvents_.begin(), materialModifiedEvents_.end());
        // creating a component generates created and modified events. filter out materials which were created and
        // modified.
        materialModifiedEvents_.erase(std::unique(materialModifiedEvents_.begin(), materialModifiedEvents_.end()),
            materialModifiedEvents_.cend());

        if (!materialDestroyedEvents_.empty()) {
            // filter out materials which were created/modified, but also destroyed.
            materialModifiedEvents_.erase(std::set_difference(materialModifiedEvents_.cbegin(),
                materialModifiedEvents_.cend(), materialDestroyedEvents_.cbegin(),
                materialDestroyedEvents_.cend(), materialModifiedEvents_.begin()),
                materialModifiedEvents_.cend());
        }
        UpdateMaterialProperties();
        materialModifiedEvents_.clear();
    }
    if (!materialDestroyedEvents_.empty()) {
        RemoveMaterialProperties(*dsMaterial_, *materialMgr_, materialDestroyedEvents_);
        materialDestroyedEvents_.clear();
    }
}

void RenderSystem::HandleMeshEvents() noexcept
{
#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("CORE3D", "RenderPreprocessorSystem", "HandleMeshEvents", CORE3D_PROFILER_DEFAULT_COLOR);
#endif
    if (!meshDestroyedEvents_.empty()) {
        std::sort(meshDestroyedEvents_.begin(), meshDestroyedEvents_.end());
    }

    if (!meshModifiedEvents_.empty()) {
        // creating a component generates created and modified events. filter out materials which were created and
        // modified.
        std::sort(meshModifiedEvents_.begin(), meshModifiedEvents_.end());
        meshModifiedEvents_.erase(
            std::unique(meshModifiedEvents_.begin(), meshModifiedEvents_.end()), meshModifiedEvents_.cend());

        if (!meshDestroyedEvents_.empty()) {
            // filter out meshes which were created/modified, but also destroyed.
            meshModifiedEvents_.erase(
                std::set_difference(meshModifiedEvents_.cbegin(), meshModifiedEvents_.cend(),
                    meshDestroyedEvents_.cbegin(), meshDestroyedEvents_.cend(), meshModifiedEvents_.begin()),
                meshModifiedEvents_.cend());
        }

        vector<uint64_t> additionalMaterials;
        for (const auto& entRef : meshModifiedEvents_) {
            if (auto meshHandle = meshMgr_->Read(entRef); meshHandle) {
                MeshDataWithHandleReference md;
                md.aabbMin = meshHandle->aabbMin;
                md.aabbMax = meshHandle->aabbMax;
                md.meshId = entRef.id;
                const bool hasSkin = (!meshHandle->jointBounds.empty());
                // md.jointBounds
                md.submeshes.resize(meshHandle->submeshes.size());
                for (size_t smIdx = 0; smIdx < md.submeshes.size(); smIdx++) {
                    auto& writeRef = md.submeshes[smIdx];
                    const auto& readRef = meshHandle->submeshes[smIdx];
                    writeRef.materialId = readRef.material.id;
                    if (!readRef.additionalMaterials.empty()) {
                        additionalMaterials.clear();
                        additionalMaterials.resize(readRef.additionalMaterials.size());
                        for (size_t matIdx = 0; matIdx < additionalMaterials.size(); ++matIdx) {
                            additionalMaterials[matIdx] = readRef.additionalMaterials[matIdx].id;
                        }
                        // array view to additional materials
                        writeRef.additionalMaterials = additionalMaterials;
                    }

                    writeRef.meshRenderSortLayer = readRef.renderSortLayer;
                    writeRef.meshRenderSortLayerOrder = readRef.renderSortLayerOrder;
                    writeRef.aabbMin = readRef.aabbMin;
                    writeRef.aabbMax = readRef.aabbMax;

                    SetupSubmeshBuffers(*gpuHandleMgr_, readRef, writeRef);
                    writeRef.submeshFlags = RenderSubmeshFlagsFromMeshFlags(readRef.flags);

                    // Clear skinning bit if joint matrices were not given.
                    if (!hasSkin) {
                        writeRef.submeshFlags &= ~RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT;
                    }

                    writeRef.drawCommand.vertexCount = readRef.vertexCount;
                    writeRef.drawCommand.indexCount = readRef.indexCount;
                    writeRef.drawCommand.instanceCount = readRef.instanceCount;
                    writeRef.drawCommand.drawCountIndirect = readRef.drawCountIndirect;
                    writeRef.drawCommand.strideIndirect = readRef.strideIndirect;
                    writeRef.drawCommand.firstIndex = readRef.firstIndex;
                    writeRef.drawCommand.vertexOffset = readRef.vertexOffset;
                    writeRef.drawCommand.firstInstance = readRef.firstInstance;
                }
                dsMaterial_->UpdateMeshData(md.meshId, md);
            }
        }
        meshModifiedEvents_.clear();
    }
    if (!meshDestroyedEvents_.empty()) {
        // destroy rendering side decoupled material data
        for (const auto& entRef : meshDestroyedEvents_) {
            dsMaterial_->DestroyMeshData(entRef.id);
        }
        meshDestroyedEvents_.clear();
    }
}

void RenderSystem::HandleGraphicsStateEvents() noexcept
{
    std::sort(graphicsStateModifiedEvents_.begin(), graphicsStateModifiedEvents_.end());
    graphicsStateModifiedEvents_.erase(
        std::unique(graphicsStateModifiedEvents_.begin(), graphicsStateModifiedEvents_.end()),
        graphicsStateModifiedEvents_.cend());

    const auto materialCount = materialMgr_->GetComponentCount();
    for (const auto& modifiedEntity : graphicsStateModifiedEvents_) {
        auto handle = graphicsStateMgr_->Read(modifiedEntity);
        if (!handle) {
            continue;
        }
        string_view renderSlot = handle->renderSlot;
        if (renderSlot.empty()) {
            // if no render slot is given select translucent or opaque based on blend state.
            renderSlot =
                std::any_of(handle->graphicsState.colorBlendState.colorAttachments,
                    handle->graphicsState.colorBlendState.colorAttachments +
                        handle->graphicsState.colorBlendState.colorAttachmentCount,
                    [](const GraphicsState::ColorBlendState::Attachment& attachment) { return attachment.enableBlend; })
                    ? DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT
                    : DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE;
        }
        const auto renderSlotId = shaderMgr_->GetRenderSlotId(renderSlot);
        const auto stateHash = shaderMgr_->HashGraphicsState(handle->graphicsState, renderSlotId);
        auto gsRenderHandleRef = shaderMgr_->GetGraphicsStateHandleByHash(stateHash);
        // if the state doesn't match any existing states based on the hash create a new one
        if (!gsRenderHandleRef) {
            const auto path = "3dshaderstates://" + to_hex(stateHash);
            IShaderManager::GraphicsStateCreateInfo createInfo { path, handle->graphicsState };
            IShaderManager::GraphicsStateVariantCreateInfo variantCreateInfo;
            variantCreateInfo.renderSlot = renderSlot;
            gsRenderHandleRef = shaderMgr_->CreateGraphicsState(createInfo, variantCreateInfo);
        }
        if (gsRenderHandleRef) {
            // when there's render handle for the state check that there's also a RenderHandleComponent which points to
            // the render handle.
            auto rhHandle = gpuHandleMgr_->Write(modifiedEntity);
            if (!rhHandle) {
                gpuHandleMgr_->Create(modifiedEntity);
                rhHandle = gpuHandleMgr_->Write(modifiedEntity);
            }
            if (rhHandle) {
                rhHandle->reference = gsRenderHandleRef;
            }
        }
        // add any material using the state to the list of modified materials, so that we update the material to render
        // data store.
        for (IComponentManager::ComponentId id = 0U; id < materialCount; ++id) {
            if (auto materialHandle = materialMgr_->Read(id)) {
                if (materialHandle->materialShader.graphicsState == modifiedEntity ||
                    materialHandle->depthShader.graphicsState == modifiedEntity) {
                    materialModifiedEvents_.push_back(materialMgr_->GetEntity(id));
                }
            }
        }
    }
    graphicsStateModifiedEvents_.clear();
}

void RenderSystem::UpdateMaterialProperties()
{
    // assuming modifiedMaterials was sorted we can assume the next entity is between pos and end.
    for (const auto& entity : materialModifiedEvents_) {
        auto materialHandle = materialMgr_->Read(entity);
        if (!materialHandle) {
            continue;
        }

        // create/update rendering side decoupled material data
        UpdateSingleMaterial(entity, &(*materialHandle));
    }
}

void RenderSystem::UpdateSingleMaterial(const Entity matEntity, const MaterialComponent* materialHandle)
{
    const MaterialComponent& materialComp = (materialHandle) ? *materialHandle : DEF_MATERIAL_COMPONENT;
    const RenderDataDefaultMaterial::InputMaterialUniforms materialUniforms =
        InputMaterialUniformsFromMaterialComponent(matEntity, materialComp);

    // NOTE: we force material updates, no early outs

    array_view<const uint8_t> customData;
    if (materialComp.customProperties) {
        const auto buffer = static_cast<const uint8_t*>(materialComp.customProperties->RLock());
        // NOTE: set and binding are currently not supported, we only support built-in mapping
        // the data goes to a predefined set and binding
        customData = array_view(buffer, materialComp.customProperties->Size());
        materialComp.customProperties->RUnlock();
    }
    // material extensions
    RenderHandleReference handleReferences[RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT];
    array_view<RenderHandleReference> extHandles;
    // extension valid only with non-default material
    if (EntityUtil::IsValid(matEntity)) {
        // first check the preferred vector version
        if (!materialComp.customResources.empty()) {
            const size_t maxCount = Math::min(static_cast<size_t>(materialComp.customResources.size()),
                static_cast<size_t>(RenderDataDefaultMaterial::MAX_MATERIAL_CUSTOM_RESOURCE_COUNT));
            extHandles = { handleReferences, maxCount };
            GetRenderHandleReferences(*gpuHandleMgr_, materialComp.customResources, extHandles);
        }
    }
    const uint32_t transformBits = materialUniforms.texTransformSetBits;
    const RenderMaterialFlags rmfFromBits =
        RenderMaterialLightingFlagsFromMaterialFlags(materialComp.materialLightingFlags);
    {
        const RenderDataDefaultMaterial::MaterialHandlesWithHandleReference materialHandles =
            GetMaterialHandles(materialComp, *gpuHandleMgr_);
        const RenderMaterialFlags rmfFromValues =
            RenderMaterialFlagsFromMaterialValues(materialComp, materialHandles, transformBits);
        const RenderMaterialFlags rmfFromExtraFlags =
            RenderMaterialFlagsFromMaterialValues(materialComp.extraRenderingFlags);
        const RenderMaterialFlags rmf = rmfFromBits | rmfFromValues | rmfFromExtraFlags;
        const RenderDataDefaultMaterial::MaterialData data {
            { gpuHandleMgr_->GetRenderHandleReference(materialComp.materialShader.shader),
                gpuHandleMgr_->GetRenderHandleReference((materialComp.type == MaterialComponent::Type::OCCLUSION)
                                                            ? dmShaderData_.gfxStateOcclusionMaterial
                                                            : materialComp.materialShader.graphicsState) },
            { gpuHandleMgr_->GetRenderHandleReference(materialComp.depthShader.shader),
                gpuHandleMgr_->GetRenderHandleReference(materialComp.depthShader.graphicsState) },
            materialComp.extraRenderingFlags, rmf, materialComp.customRenderSlotId,
            RenderMaterialType(materialComp.type),
            (materialComp.type == MaterialComponent::Type::OCCLUSION) ? uint8_t(0U)
                                                                      : materialComp.renderSort.renderSortLayer,
            materialComp.renderSort.renderSortLayerOrder
        };

        dsMaterial_->UpdateMaterialData(matEntity.id, materialUniforms, materialHandles, data, customData, extHandles);
    }
}

ISystem* IRenderSystemInstance(IEcs& ecs)
{
    return new RenderSystem(ecs);
}

void IRenderSystemDestroy(ISystem* instance)
{
    delete static_cast<RenderSystem*>(instance);
}
CORE3D_END_NAMESPACE()
