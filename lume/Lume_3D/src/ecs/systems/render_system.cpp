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

#include "render_system.h"

#include <algorithm>

#if (CORE3D_VALIDATION_ENABLED == 1)
#include <cinttypes>
#include <string>
#endif

#include <PropertyTools/property_api_impl.inl>
#include <PropertyTools/property_macros.h>

#include <3d/ecs/components/camera_component.h>
#include <3d/ecs/components/environment_component.h>
#include <3d/ecs/components/fog_component.h>
#include <3d/ecs/components/joint_matrices_component.h>
#include <3d/ecs/components/layer_component.h>
#include <3d/ecs/components/light_component.h>
#include <3d/ecs/components/material_component.h>
#include <3d/ecs/components/material_extension_component.h>
#include <3d/ecs/components/mesh_component.h>
#include <3d/ecs/components/name_component.h>
#include <3d/ecs/components/node_component.h>
#include <3d/ecs/components/planar_reflection_component.h>
#include <3d/ecs/components/post_process_component.h>
#include <3d/ecs/components/post_process_configuration_component.h>
#include <3d/ecs/components/render_configuration_component.h>
#include <3d/ecs/components/render_handle_component.h>
#include <3d/ecs/components/render_mesh_batch_component.h>
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
#include <core/util/intf_frustum_util.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/intf_render_data_store_post_process.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/implementation_uids.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_render_node_graph_manager.h>
#include <render/resource_handle.h>

#include "ecs/components/previous_joint_matrices_component.h"
#include "ecs/components/previous_world_matrix_component.h"
#include "ecs/systems/render_preprocessor_system.h"
#include "util/component_util_functions.h"
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
static const RenderSubmesh INIT {};
static const RenderDataDefaultMaterial::MaterialHandles DEF_MATERIAL_HANDLES {};
static const MaterialComponent DEF_MATERIAL_COMPONENT {};
static constexpr uint32_t NEEDS_COLOR_PRE_PASS { 1u << 0u };
static constexpr uint64_t PREPASS_CAMERA_START_UNIQUE_ID { 1 };
static constexpr uint64_t SHADOW_CAMERA_START_UNIQUE_ID { 100 };
#if (CORE3D_VALIDATION_ENABLED == 1)
static constexpr uint32_t MAX_BATCH_SUBMESH_COUNT { 64u };
#endif

static constexpr uint32_t MAX_BATCH_OBJECT_COUNT { PipelineLayoutConstants::MAX_UBO_BIND_BYTE_SIZE /
                                                   PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE };

// typename for POD data. (e.g. "PostProcess") (core/render/intf_render_data_store_pod.h)
static constexpr string_view POST_PROCESS_NAME { "PostProcess" };
static constexpr string_view POD_DATA_STORE_NAME { "RenderDataStorePod" };
static constexpr string_view PP_DATA_STORE_NAME { "RenderDataStorePostProcess" };

// In addition to the base our renderableQuery has two required components and three optional components:
// (0) RenderMeshComponent
// (1) WorldMatrixComponent
// (2) PreviousWorldMatrixComponent
// (3) LayerComponent (optional)
// (4) JointMatrixComponent (optional)
// (5) PreviousJointMatrixComponent (optional)
static constexpr const auto RQ_RMC = 0U;
static constexpr const auto RQ_WM = 1U;
static constexpr const auto RQ_PWM = 2U;
static constexpr const auto RQ_L = 3U;
static constexpr const auto RQ_JM = 4U;
static constexpr const auto RQ_PJM = 5U;

void FillShaderData(IEntityManager& em, IUriComponentManager& uriManager,
    IRenderHandleComponentManager& renderHandleMgr, const IShaderManager& shaderMgr, const string_view renderSlot,
    RenderSystem::DefaultMaterialShaderData::SingleShaderData& shaderData)
{
    const uint32_t renderSlotId = shaderMgr.GetRenderSlotId(renderSlot);
    const IShaderManager::RenderSlotData rsd = shaderMgr.GetRenderSlotData(renderSlotId);

    auto uri = "3dshaders://" + renderSlot;
    auto resourceEntity = LookupResourceByUri(uri, uriManager, renderHandleMgr);
    if (!EntityUtil::IsValid(resourceEntity)) {
        resourceEntity = em.Create();
        renderHandleMgr.Create(resourceEntity);
        renderHandleMgr.Write(resourceEntity)->reference = rsd.shader;
        uriManager.Create(resourceEntity);
        uriManager.Write(resourceEntity)->uri = uri;
    }
    shaderData.shader = em.GetReferenceCounted(resourceEntity);

    uri = "3dshaderstates://";
    uri += renderSlot;
    resourceEntity = LookupResourceByUri(uri, uriManager, renderHandleMgr);
    if (!EntityUtil::IsValid(resourceEntity)) {
        resourceEntity = em.Create();
        renderHandleMgr.Create(resourceEntity);
        renderHandleMgr.Write(resourceEntity)->reference = rsd.graphicsState;
        uriManager.Create(resourceEntity);
        uriManager.Write(resourceEntity)->uri = uri;
    }
    shaderData.gfxState = em.GetReferenceCounted(resourceEntity);

    uri += "_DBL";
    resourceEntity = LookupResourceByUri(uri, uriManager, renderHandleMgr);
    if (!EntityUtil::IsValid(resourceEntity)) {
        // fetch double sided mode (no culling gfx state) (NOTE: could be fetched with name)
        if (rsd.graphicsState) {
            GraphicsState gfxState = shaderMgr.GetGraphicsState(rsd.graphicsState);
            gfxState.rasterizationState.cullModeFlags = CullModeFlagBits::CORE_CULL_MODE_NONE;
            const uint64_t gfxStateHash = shaderMgr.HashGraphicsState(gfxState);
            auto handlDbl = shaderMgr.GetGraphicsStateHandleByHash(gfxStateHash);
            if (handlDbl) {
                resourceEntity = em.Create();
                renderHandleMgr.Create(resourceEntity);
                renderHandleMgr.Write(resourceEntity)->reference = handlDbl;
                uriManager.Create(resourceEntity);
                uriManager.Write(resourceEntity)->uri = uri;
            }
        }
    }
    shaderData.gfxStateDoubleSided = em.GetReferenceCounted(resourceEntity);
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
    if (pipelineFlags & CameraComponent::PipelineFlagBits::DYNAMIC_CUBEMAP_BIT) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_DYNAMIC_CUBEMAP_BIT;
    }
    if ((pipelineFlags & CameraComponent::PipelineFlagBits::DISALLOW_REFLECTION_BIT) == 0U) {
        flags |= RenderCamera::CameraFlagBits::CAMERA_FLAG_ALLOW_REFLECTION_BIT;
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
    const IPostProcessComponentManager* postProcessMgr,
    const IPostProcessConfigurationComponentManager* postProcessConfigMgr, const INameComponentManager* nameMgr,
    const string_view sceneName, const Entity& entity)
{
    if (postProcessMgr && postProcessConfigMgr && nameMgr) {
        if (postProcessMgr->HasComponent(entity) || postProcessConfigMgr->HasComponent(entity)) {
            if (ScopedHandle<const NameComponent> nameHandle = nameMgr->Read(entity);
                nameHandle && (!nameHandle->name.empty())) {
                return fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> { nameHandle->name };
            } else {
                // checks if any of the post process mgrs has valid entity for camera
                fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> ret =
                    DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_PREFIX_NAME;
                ret.append(sceneName);
                ret.append(to_hex(entity.id));
                return ret;
            }
        }
    }
    return (DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_PREFIX_NAME);
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
    const CameraComponent& cc, RenderCamera& renderCamera, const bool checkCustomTargets)
{
    renderCamera.layerMask = cc.layerMask;
    renderCamera.viewport = { cc.viewport[0u], cc.viewport[1u], cc.viewport[2u], cc.viewport[3u] };
    renderCamera.scissor = { cc.scissor[0u], cc.scissor[1u], cc.scissor[2u], cc.scissor[3u] };
    renderCamera.renderResolution = { cc.renderResolution[0u], cc.renderResolution[1u] };
    renderCamera.zNear = cc.zNear;
    renderCamera.zFar = cc.zFar;
    renderCamera.flags = GetRenderCameraFlagsFromComponentFlags(cc.pipelineFlags);
    renderCamera.clearDepthStencil = { cc.clearDepthValue, 0u };
    renderCamera.clearColorValues = { cc.clearColorValue.x, cc.clearColorValue.y, cc.clearColorValue.z,
        cc.clearColorValue.w };
    renderCamera.cullType = GetRenderCameraCullTypeFromComponent(cc.culling);
    renderCamera.renderPipelineType = GetRenderCameraRenderPipelineTypeFromComponent(cc.renderingPipeline);
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

    if (checkCustomTargets && (cc.customColorTargets.size() > 0 || cc.customDepthTarget)) {
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
    for (uint32_t idx = 0; idx < maxMvCount; ++idx) {
        const auto& mvRef = cc.multiViewCameras[idx];
        if (EntityUtil::IsValid(mvRef)) {
            renderCamera.multiViewCameraIds[renderCamera.multiViewCameraCount++] = mvRef.id;
        }
    }
    if (renderCamera.multiViewCameraCount > 0U) {
        if (renderCamera.renderPipelineType == RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_W(to_string(renderCamera.id) + "camera_pipeline_mv",
                "Camera rendering pipeline cannot be LIGHT_FORWARD for multi-view.");
#endif
            renderCamera.renderPipelineType = RenderCamera::RenderPipelineType::FORWARD;
        }
    }

    ValidateRenderCamera(renderCamera);
}

RenderCamera CreateColorPrePassRenderCamera(const IRenderHandleComponentManager& renderHandleMgr,
    const ICameraComponentManager& cameraMgr, const RenderCamera& baseCamera, const Entity& prePassEntity,
    const uint64_t uniqueId)
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
        FillRenderCameraBaseFromCameraComponent(renderHandleMgr, *prePassCameraHandle, rc, false);
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
    rc.name = to_string(uniqueId);
    rc.id = uniqueId; // unique id for main pre-pass
    rc.prePassColorTargetName = {};
    rc.postProcessName = DefaultMaterialCameraConstants::CAMERA_PRE_PASS_POST_PROCESS_PREFIX_NAME;
    return rc;
}

void FillRenderEnvironment(const IRenderHandleComponentManager& renderHandleMgr, const uint64_t& nodeLayerMask,
    const Entity& entity, const EnvironmentComponent& component, RenderCamera::Environment& renderEnv)
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
    }
    renderEnv.radianceCubemap = renderHandleMgr.GetRenderHandleReference(component.radianceCubemap);
    renderEnv.radianceCubemapMipCount = component.radianceCubemapMipCount;
    renderEnv.envMap = renderHandleMgr.GetRenderHandleReference(component.envMap);
    renderEnv.envMapLodLevel = component.envMapLodLevel;
    const size_t shCount =
        std::min(countof(component.irradianceCoefficients), countof(renderEnv.shIndirectCoefficients));
    for (size_t idx = 0; idx < shCount; ++idx) {
        renderEnv.shIndirectCoefficients[idx] = Math::Vec4(component.irradianceCoefficients[idx], 1.0f);
    }
    renderEnv.indirectDiffuseFactor = component.indirectDiffuseFactor;
    renderEnv.indirectSpecularFactor = component.indirectSpecularFactor;
    renderEnv.envMapFactor = component.envMapFactor;
    renderEnv.blendFactor = component.blendFactor;
    renderEnv.rotation = component.environmentRotation;
}

inline constexpr Math::Vec4 LerpVec4(const Math::Vec4& vec0, const Math::Vec4& vec1, const Math::Vec4& blend)
{
    return Math::Vec4(Math::lerp(vec0.x, vec1.x, blend.x), Math::lerp(vec0.y, vec1.y, blend.y),
        Math::lerp(vec0.z, vec1.z, blend.z), Math::lerp(vec0.w, vec1.w, blend.w));
}

void FillCameraRenderEnvironment(
    IRenderDataStoreDefaultCamera* dsCamera, const CameraComponent& component, RenderCamera& camera)
{
    camera.environmentCount = 0U;

    // if dynamic cubemap is used, we create a new environment for camera id with default coefficients
    if (component.pipelineFlags & CameraComponent::DYNAMIC_CUBEMAP_BIT) {
        // copy base
        camera.environment = dsCamera->GetEnvironment(camera.environmentIds[0U]);
        // replace the main environment id
        camera.environment.id = camera.id;
        camera.environmentIds[camera.environmentCount] = camera.environment.id;
        camera.environmentCount += 1U;
        camera.environment.backgroundType = RenderCamera::Environment::BG_TYPE_CUBEMAP;
    }
    // if environments are empty, we use default
    if (component.environments.empty()) {
        const uint64_t id = (component.environment.id == Entity {}.id) ? RenderSceneDataConstants::INVALID_ID
                                                                       : component.environment.id;
        camera.environmentIds[camera.environmentCount++] = id;
    } else {
        const uint32_t maxCount = Math::min(static_cast<uint32_t>(component.environments.size()),
            DefaultMaterialCameraConstants::MAX_ENVIRONMENT_COUNT - camera.environmentCount);
        for (uint32_t idx = 0; idx < maxCount; ++idx) {
            camera.environmentIds[camera.environmentCount++] = component.environments[idx].id;
        }
    }

    // fetch the main environment for camera for fast and easy access
    if (camera.environment.id == RenderSceneDataConstants::INVALID_ID) {
        camera.environment = dsCamera->GetEnvironment(camera.environmentIds[0U]);
    }

    if (component.pipelineFlags & CameraComponent::DYNAMIC_CUBEMAP_BIT) {
        // replace base environment values -> combine indirect diffuse values
        auto& dynamicEnv = camera.environment;
        dynamicEnv.indirectDiffuseFactor = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        for (size_t idx = 0; idx < countof(dynamicEnv.shIndirectCoefficients); ++idx) {
            dynamicEnv.shIndirectCoefficients[idx] = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
        }
        Math::Vec4 blendFactor = Math::Vec4(1.0f, 1.0f, 1.0f, 1.0f);
        for (uint32_t envIdx = 1U; envIdx < camera.environmentCount; ++envIdx) {
            const auto env = dsCamera->GetEnvironment(camera.environmentIds[envIdx]);
            dynamicEnv.indirectDiffuseFactor =
                LerpVec4(dynamicEnv.indirectDiffuseFactor, env.indirectDiffuseFactor, blendFactor);
            for (size_t idx = 0; idx < countof(dynamicEnv.shIndirectCoefficients); ++idx) {
                dynamicEnv.shIndirectCoefficients[idx] =
                    LerpVec4(dynamicEnv.shIndirectCoefficients[idx], env.shIndirectCoefficients[idx], blendFactor);
            }
            blendFactor = env.blendFactor;
        }

        // add the dynamic environment which has the indirect diffuse values blended (somewhat)
        dsCamera->AddEnvironment(camera.environment);
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

RenderDataDefaultMaterial::MaterialHandles MaterialHandles(
    const MaterialComponent& materialDesc, const IRenderHandleComponentManager& gpuManager)
{
    RenderDataDefaultMaterial::MaterialHandles materialHandles;
    auto imageIt = std::begin(materialHandles.images);
    auto samplerIt = std::begin(materialHandles.samplers);
    for (const MaterialComponent::TextureInfo& info : materialDesc.textures) {
        *imageIt++ = gpuManager.GetRenderHandleReference(info.image);
        *samplerIt++ = gpuManager.GetRenderHandleReference(info.sampler);
    }
    return materialHandles;
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

uint32_t RenderMaterialFlagsFromMaterialValues(const MaterialComponent& matComp,
    const RenderDataDefaultMaterial::MaterialHandles& handles, const uint32_t hasTransformBit,
    const bool enableGpuInstancing)
{
    uint32_t rmf = 0;
    // enable built-in specialization for default materials
    CORE_ASSERT(matComp.type <= MaterialComponent::Type::CUSTOM_COMPLEX);
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
    // enables instacing and specializes the shader
    rmf |= (enableGpuInstancing) ? RenderMaterialFlagBits::RENDER_MATERIAL_GPU_INSTANCING_BIT : 0U;
    // NOTE: earlier version had MASK here for opaque bit as well
    return rmf;
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

#if (CORE3D_VALIDATION_ENABLED == 1)
void ValidateInputColor(const Entity material, const MaterialComponent& matComp)
{
    if (matComp.type < MaterialComponent::Type::CUSTOM) {
        const auto& base = matComp.textures[MaterialComponent::TextureIndex::BASE_COLOR];
        if ((base.factor.x > 1.0f) || (base.factor.y > 1.0f) || (base.factor.z > 1.0f) || (base.factor.w > 1.0f)) {
            CORE_LOG_ONCE_W(to_string(material.id) + "_base_fac",
                "CORE3D_VALIDATION: Non custom material type expects base color factor to be <= 1.0f.");
        }
        const auto& mat = matComp.textures[MaterialComponent::TextureIndex::MATERIAL];
        if ((mat.factor.y > 1.0f) || (mat.factor.z > 1.0f)) {
            CORE_LOG_ONCE_W(to_string(material.id) + "_mat_fac",
                "CORE3D_VALIDATION: Non custom material type expects roughness and metallic to be <= 1.0f.");
        }
    }
}
#endif

RenderDataDefaultMaterial::InputMaterialUniforms InputMaterialUniformsFromMaterialComponent(
    const Entity material, const MaterialComponent& matDesc)
{
    static constexpr const RenderDataDefaultMaterial::InputMaterialUniforms defaultInput {};
    RenderDataDefaultMaterial::InputMaterialUniforms mu = defaultInput;

#if (CORE3D_VALIDATION_ENABLED == 1)
    ValidateInputColor(material, matDesc);
#endif

    uint32_t transformBits = 0u;
    static constexpr const uint32_t texCount =
        Math::min(static_cast<uint32_t>(MaterialComponent::TextureIndex::TEXTURE_COUNT),
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

BEGIN_PROPERTY(IRenderSystem::Properties, ComponentMetadata)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreMaterial, "dataStoreMaterial", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreCamera, "dataStoreCamera", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreLight, "dataStoreLight", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreScene, "dataStoreScene", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreMorph, "dataStoreMorph", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStorePrefix, "", 0)
END_PROPERTY();

void SetupSubmeshBuffers(const IRenderHandleComponentManager& renderHandleManager, const MeshComponent& desc,
    const MeshComponent::Submesh& submesh, RenderSubmesh& renderSubmesh)
{
    CORE_STATIC_ASSERT(
        MeshComponent::Submesh::BUFFER_COUNT <= RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
    // calculate real vertex buffer count and fill "safety" handles for default material
    // no default shader variants without joints etc.
    // NOTE:
    // optimize for minimal GetRenderHandleReference calls
    // often the same vertex buffer is used.
    Entity prevEntity = {};
    for (size_t idx = 0; idx < countof(submesh.bufferAccess); ++idx) {
        const auto& acc = submesh.bufferAccess[idx];
        auto& vb = renderSubmesh.vertexBuffers[idx];
        if (EntityUtil::IsValid(prevEntity) && (prevEntity == acc.buffer)) {
            vb.bufferHandle = renderSubmesh.vertexBuffers[idx - 1].bufferHandle;
            vb.bufferOffset = acc.offset;
            vb.byteSize = acc.byteSize;
        } else if (acc.buffer) {
            vb.bufferHandle = renderHandleManager.GetRenderHandleReference(acc.buffer);
            vb.bufferOffset = acc.offset;
            vb.byteSize = acc.byteSize;

            // store the previous entity
            prevEntity = acc.buffer;
        } else {
            vb.bufferHandle = renderSubmesh.vertexBuffers[0].bufferHandle; // expecting safety binding
            vb.bufferOffset = 0;
            vb.byteSize = 0;
        }
    }

    // NOTE: we will get max amount of vertex buffers if there is at least one
    renderSubmesh.vertexBufferCount =
        submesh.bufferAccess[0U].buffer ? static_cast<uint32_t>(countof(submesh.bufferAccess)) : 0U;

    if (submesh.indexBuffer.buffer) {
        renderSubmesh.indexBuffer.bufferHandle =
            renderHandleManager.GetRenderHandleReference(submesh.indexBuffer.buffer);
        renderSubmesh.indexBuffer.bufferOffset = submesh.indexBuffer.offset;
        renderSubmesh.indexBuffer.byteSize = submesh.indexBuffer.byteSize;
        renderSubmesh.indexBuffer.indexType = submesh.indexBuffer.indexType;
    }
    if (submesh.indirectArgsBuffer.buffer) {
        renderSubmesh.indirectArgsBuffer.bufferHandle =
            renderHandleManager.GetRenderHandleReference(submesh.indirectArgsBuffer.buffer);
        renderSubmesh.indirectArgsBuffer.bufferOffset = submesh.indirectArgsBuffer.offset;
        renderSubmesh.indirectArgsBuffer.byteSize = submesh.indirectArgsBuffer.byteSize;
    }

    // Set submesh flags.
    renderSubmesh.submeshFlags = RenderSubmeshFlagsFromMeshFlags(submesh.flags);
}

void GetRenderHandleReferences(const IRenderHandleComponentManager& renderHandleMgr,
    const array_view<const EntityReference> inputs, array_view<RenderHandleReference>& outputs)
{
    for (size_t idx = 0; idx < outputs.size(); ++idx) {
        outputs[idx] = renderHandleMgr.GetRenderHandleReference(inputs[idx]);
    }
}

struct RenderMaterialIndices {
    uint32_t materialIndex { ~0u };
    uint32_t materialCustomResourceIndex { ~0u };
};
RenderMaterialIndices AddSingleMaterial(const IMaterialComponentManager& materialMgr,
    const IMaterialExtensionComponentManager& materialExtMgr, const IRenderHandleComponentManager& renderHandleMgr,
    IRenderDataStoreDefaultMaterial& dataStoreMaterial, const Entity& material, const bool fetchMaterialHandles,
    const bool checkExtMaterial, const uint32_t materialIndex, const uint32_t matInstanceIndex,
    const uint32_t matInstanceCount, const bool enableGpuInstancing)
{
    RenderMaterialIndices rmi { materialIndex, ~0u };
    // scoped
    auto matData = materialMgr.Read(material);
    const MaterialComponent& materialComp = (matData) ? *matData : DEF_MATERIAL_COMPONENT;
    RenderDataDefaultMaterial::InputMaterialUniforms materialUniforms =
        InputMaterialUniformsFromMaterialComponent(material, materialComp);

    array_view<const uint8_t> customData;
    if (materialComp.customProperties) {
        const auto buffer = static_cast<const uint8_t*>(materialComp.customProperties->RLock());
        // NOTE: set and binding are currently not supported, we only support built-in mapping
        // the data goes to a predefined set and binding
        customData = array_view(buffer, materialComp.customProperties->Size());
        materialComp.customProperties->RUnlock();
    }
    // material extensions
    if (checkExtMaterial) {
        // first check the preferred vector version
        if (!materialComp.customResources.empty()) {
            RenderHandleReference handleReferences[MaterialExtensionComponent::ResourceIndex::RESOURCE_COUNT];
            const size_t maxCount = Math::min(static_cast<size_t>(materialComp.customResources.size()),
                static_cast<size_t>(MaterialExtensionComponent::ResourceIndex::RESOURCE_COUNT));
            array_view<RenderHandleReference> handles = { handleReferences, maxCount };
            GetRenderHandleReferences(renderHandleMgr, materialComp.customResources, handles);
            rmi.materialCustomResourceIndex = dataStoreMaterial.AddMaterialCustomResources(material.id, handles);
        } else if (const auto matExtHandle = materialExtMgr.Read(material); matExtHandle) {
            RenderHandleReference handleReferences[MaterialExtensionComponent::ResourceIndex::RESOURCE_COUNT];
            const MaterialExtensionComponent& matExtComp = *matExtHandle;
            std::transform(std::begin(matExtComp.resources), std::end(matExtComp.resources),
                std::begin(handleReferences), [&renderHandleMgr](const EntityReference& matEntity) {
                    return renderHandleMgr.GetRenderHandleReference(matEntity);
                });
            // bind all handles, invalid handles are just ignored
            rmi.materialCustomResourceIndex =
                dataStoreMaterial.AddMaterialCustomResources(material.id, handleReferences);
        }
    }
    // optimization for e.g. with batch GPU instancing (we do not care about material handles)
    if (fetchMaterialHandles) {
        const RenderDataDefaultMaterial::MaterialHandles materialHandles =
            MaterialHandles(materialComp, renderHandleMgr);
        const uint32_t transformBits = materialUniforms.texTransformSetBits;

        const RenderMaterialFlags rmfFromBits =
            RenderMaterialLightingFlagsFromMaterialFlags(materialComp.materialLightingFlags);
        const RenderMaterialFlags rmfFromValues =
            RenderMaterialFlagsFromMaterialValues(materialComp, materialHandles, transformBits, enableGpuInstancing);
        const RenderMaterialFlags rmf = rmfFromBits | rmfFromValues;
        const RenderDataDefaultMaterial::MaterialData data {
            RenderMaterialType(materialComp.type),
            materialComp.extraRenderingFlags,
            rmf,
            materialComp.customRenderSlotId,
            { renderHandleMgr.GetRenderHandleReference(materialComp.materialShader.shader),
                renderHandleMgr.GetRenderHandleReference(materialComp.materialShader.graphicsState) },
            { renderHandleMgr.GetRenderHandleReference(materialComp.depthShader.shader),
                renderHandleMgr.GetRenderHandleReference(materialComp.depthShader.graphicsState) },
        };

        dataStoreMaterial.AddInstanceMaterialData(
            materialIndex, matInstanceIndex, matInstanceCount, materialUniforms, materialHandles, data, customData);
    } else {
        dataStoreMaterial.AddInstanceMaterialData(
            materialIndex, matInstanceIndex, matInstanceCount, materialUniforms, customData);
    }
    return rmi;
}

RenderMaterialIndices AddRenderMaterial(IMaterialComponentManager& materialMgr,
    IMaterialExtensionComponentManager& materialExtMgr, IRenderHandleComponentManager& renderHandleMgr,
    IRenderDataStoreDefaultMaterial& dataStoreMaterial, const Entity& material, const uint32_t instanceCount,
    const bool duplicateMaterialInstances)
{
    const uint32_t materialDuplicateInstanceCount = duplicateMaterialInstances ? instanceCount : 0u;
    const auto matIndices = dataStoreMaterial.GetMaterialIndices(material.id, instanceCount);
    RenderMaterialIndices indices = { matIndices.materialIndex, matIndices.materialCustomResourceIndex };
    if (indices.materialIndex == RenderSceneDataConstants::INVALID_INDEX) {
        indices.materialIndex = dataStoreMaterial.AllocateMaterials(material.id, instanceCount);
        const bool enableGpuInstancing = (instanceCount > 1U);
        // indices, might add extension index
        // material extensions are checked only when material entity is valid i.e. not default material.
        indices = AddSingleMaterial(materialMgr, materialExtMgr, renderHandleMgr, dataStoreMaterial, material, true,
            EntityUtil::IsValid(material), indices.materialIndex, 0U, materialDuplicateInstanceCount,
            enableGpuInstancing);
    }
    return indices;
}

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
};

void UpdateReflectionPlaneMaterial(IRenderMeshComponentManager& renderMeshMgr, IMeshComponentManager& meshMgr,
    IMaterialComponentManager& materialMgr, const Entity& entity, const PlanarReflectionComponent& reflComponent,
    const ReflectionPlaneTargetUpdate& rptu)
{
    // update material
    if (const auto rmcHandle = renderMeshMgr.Read(entity); rmcHandle) {
        const RenderMeshComponent& rmc = *rmcHandle;
        if (const auto meshHandle = meshMgr.Read(rmc.mesh); meshHandle) {
            const MeshComponent& meshComponent = *meshHandle;
            if (!meshComponent.submeshes.empty()) {
                if (auto matHandle = materialMgr.Write(meshComponent.submeshes[0].material); matHandle) {
                    MaterialComponent& matComponent = *matHandle;
                    // NOTE: CLEARCOAT_ROUGHNESS cannot be used due to material flags bit is enabled for lighting
                    matComponent.textures[MaterialComponent::TextureIndex::CLEARCOAT_ROUGHNESS].factor = {
                        static_cast<float>(rptu.mipCount),
                        reflComponent.screenPercentage,
                        static_cast<float>(reflComponent.renderTargetResolution[0u]),
                        static_cast<float>(reflComponent.renderTargetResolution[1u]),
                    };
                }
            }
        }
    }
}

ReflectionPlaneTargetUpdate UpdatePlaneReflectionTargetResolution(IGpuResourceManager& gpuResourceMgr,
    IRenderHandleComponentManager& gpuHandleMgr, IPlanarReflectionComponentManager& planarReflMgr,
    const RenderCamera& sceneCamera, const Entity& entity, PlanarReflectionComponent& reflComp)
{
    // NOTE: if the resource is not yet created one should attach it to material as well
    // these resources should be un-named
    const uint32_t newWidth =
        std::max(static_cast<uint32_t>(static_cast<float>(sceneCamera.renderResolution.x) * reflComp.screenPercentage),
            reflComp.renderTargetResolution[0]);
    const uint32_t newHeight =
        std::max(static_cast<uint32_t>(static_cast<float>(sceneCamera.renderResolution.y) * reflComp.screenPercentage),
            reflComp.renderTargetResolution[1]);

    ReflectionPlaneTargetUpdate rptu;

    const RenderHandle colorRenderTarget = gpuHandleMgr.GetRenderHandle(reflComp.colorRenderTarget);
    const RenderHandle depthRenderTarget = gpuHandleMgr.GetRenderHandle(reflComp.depthRenderTarget);
    // get current mip count
    rptu.mipCount = RenderHandleUtil::IsValid(colorRenderTarget)
                        ? gpuResourceMgr.GetImageDescriptor(gpuResourceMgr.Get(colorRenderTarget)).mipCount
                        : 1u;
    if ((!RenderHandleUtil::IsValid(colorRenderTarget)) || (!RenderHandleUtil::IsValid(depthRenderTarget)) ||
        (newWidth > reflComp.renderTargetResolution[0]) || (newHeight > reflComp.renderTargetResolution[1])) {
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
        if (!EntityUtil::IsValid(reflComp.colorRenderTarget)) {
            reflComp.colorRenderTarget = gpuHandleMgr.GetEcs().GetEntityManager().CreateReferenceCounted();
            gpuHandleMgr.Create(reflComp.colorRenderTarget);
        }
        rptu.mipCount = Math::min(Math::max(DefaultMaterialCameraConstants::REFLECTION_PLANE_MIP_COUNT, rptu.mipCount),
            static_cast<uint32_t>(std::log2f(static_cast<float>(std::max(newWidth, newHeight)))) + 1u);
        gpuHandleMgr.Write(reflComp.colorRenderTarget)->reference =
            reCreateGpuImage(gpuResourceMgr, entity.id, colorRenderTarget, newWidth, newHeight, rptu.mipCount, false);

        if (!EntityUtil::IsValid(reflComp.depthRenderTarget)) {
            reflComp.depthRenderTarget = gpuHandleMgr.GetEcs().GetEntityManager().CreateReferenceCounted();
            gpuHandleMgr.Create(reflComp.depthRenderTarget);
        }
        gpuHandleMgr.Write(reflComp.depthRenderTarget)->reference =
            reCreateGpuImage(gpuResourceMgr, entity.id, depthRenderTarget, newWidth, newHeight, 1u, true);

        reflComp.renderTargetResolution[0] = newWidth;
        reflComp.renderTargetResolution[1] = newHeight;
        rptu.recreated = true;
        planarReflMgr.Set(entity, reflComp);
    }

    return rptu;
}

// Given position and normal of the plane, calculates plane in camera space.
inline Math::Vec4 CalculateCameraSpaceClipPlane(
    const Math::Mat4X4& view, Math::Vec3 pos, Math::Vec3 normal, float sideSign, float clipPlaneOffset)
{
    const Math::Vec3 offsetPos = pos + normal * clipPlaneOffset;
    const Math::Vec3 cpos = Math::MultiplyPoint3X4(view, offsetPos);
    const Math::Vec3 cnormal = Math::Normalize(Math::MultiplyVector(view, normal)) * sideSign;
    return Math::Vec4(cnormal.x, cnormal.y, cnormal.z, -Math::Dot(cpos, cnormal));
}

// See http://aras-p.info/texts/obliqueortho.html
inline void CalculateObliqueProjectionMatrix(Math::Mat4X4& projection, const Math::Vec4& plane)
{
    const Math::Mat4X4 inverseProjection = Inverse(projection);

    const Math::Vec4 q = inverseProjection * Math::Vec4(Sgn(plane.x), Sgn(plane.y), 1.0f, 1.0f);
    const Math::Vec4 c = plane * (2.0f / Math::Dot(plane, q));

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

MinAndMax GetMeshMinAndMax(const IRenderPreprocessorSystem& renderPreprocessorSystem, const Entity renderMeshEntity,
    array_view<const Entity> nextRenderMeshEntities)
{
    const auto meshAabb =
        static_cast<const RenderPreprocessorSystem&>(renderPreprocessorSystem).GetRenderMeshAabb(renderMeshEntity);
    MinAndMax mam { meshAabb.min, meshAabb.max };
    for (const auto& nextRenderMeshEntity : nextRenderMeshEntities) {
        const auto nextMeshAabb = static_cast<const RenderPreprocessorSystem&>(renderPreprocessorSystem)
                                      .GetRenderMeshAabb(nextRenderMeshEntity);
        mam.minAABB = Math::min(mam.minAABB, nextMeshAabb.min);
        mam.maxAABB = Math::max(mam.maxAABB, nextMeshAabb.max);
    }
    return mam;
}

inline void ProcessCameraAddMultiViewHash(const RenderCamera& cam, unordered_map<uint64_t, uint64_t>& childToParent)
{
    for (uint32_t idx = 0; idx < cam.multiViewCameraCount; ++idx) {
        childToParent.insert_or_assign(cam.multiViewCameraIds[idx], cam.id);
    }
}
} // namespace

RenderSystem::RenderSystem(IEcs& ecs)
    : ecs_(ecs), nodeMgr_(GetManager<INodeComponentManager>(ecs)),
      renderMeshBatchMgr_(GetManager<IRenderMeshBatchComponentManager>(ecs)),
      renderMeshMgr_(GetManager<IRenderMeshComponentManager>(ecs)),
      worldMatrixMgr_(GetManager<IWorldMatrixComponentManager>(ecs)),
      prevWorldMatrixMgr_(GetManager<IPreviousWorldMatrixComponentManager>(ecs)),
      renderConfigMgr_(GetManager<IRenderConfigurationComponentManager>(ecs)),
      cameraMgr_(GetManager<ICameraComponentManager>(ecs)), lightMgr_(GetManager<ILightComponentManager>(ecs)),
      planarReflectionMgr_(GetManager<IPlanarReflectionComponentManager>(ecs)),
      materialExtensionMgr_(GetManager<IMaterialExtensionComponentManager>(ecs)),
      materialMgr_(GetManager<IMaterialComponentManager>(ecs)), meshMgr_(GetManager<IMeshComponentManager>(ecs)),
      uriMgr_(GetManager<IUriComponentManager>(ecs)), nameMgr_(GetManager<INameComponentManager>(ecs)),
      environmentMgr_(GetManager<IEnvironmentComponentManager>(ecs)), fogMgr_(GetManager<IFogComponentManager>(ecs)),
      gpuHandleMgr_(GetManager<IRenderHandleComponentManager>(ecs)), layerMgr_(GetManager<ILayerComponentManager>(ecs)),
      jointMatricesMgr_(GetManager<IJointMatricesComponentManager>(ecs)),
      prevJointMatricesMgr_(GetManager<IPreviousJointMatricesComponentManager>(ecs)),
      postProcessMgr_(GetManager<IPostProcessComponentManager>(ecs)),
      postProcessConfigMgr_(GetManager<IPostProcessConfigurationComponentManager>(ecs)),
      RENDER_SYSTEM_PROPERTIES(&properties_, array_view(ComponentMetadata))
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
        frustumUtil_ = GetInstance<IFrustumUtil>(UID_FRUSTUM_UTIL);
        renderContext_ = GetInstance<IRenderContext>(*engine->GetInterface<IClassRegister>(), UID_RENDER_CONTEXT);
        if (renderContext_) {
            picking_ = GetInstance<IPicking>(*renderContext_->GetInterface<IClassRegister>(), UID_PICKING);
            graphicsContext_ =
                GetInstance<IGraphicsContext>(*renderContext_->GetInterface<IClassRegister>(), UID_GRAPHICS_CONTEXT);
            if (graphicsContext_) {
                renderUtil_ = &graphicsContext_->GetRenderUtil();
            }
            shaderMgr_ = &renderContext_->GetDevice().GetShaderManager();
            gpuResourceMgr_ = &renderContext_->GetDevice().GetGpuResourceManager();
        }
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
    dsScene_ =
        static_cast<IRenderDataStoreDefaultScene*>(manager.GetRenderDataStore(properties_.dataStoreScene.data()));
    dsCamera_ =
        static_cast<IRenderDataStoreDefaultCamera*>(manager.GetRenderDataStore(properties_.dataStoreCamera.data()));
    dsLight_ =
        static_cast<IRenderDataStoreDefaultLight*>(manager.GetRenderDataStore(properties_.dataStoreLight.data()));
    dsMaterial_ =
        static_cast<IRenderDataStoreDefaultMaterial*>(manager.GetRenderDataStore(properties_.dataStoreMaterial.data()));
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
        const IShaderManager& shaderMgr = renderContext_->GetDevice().GetShaderManager();
        auto& entityMgr = ecs_.GetEntityManager();
        FillShaderData(entityMgr, *uriMgr_, *gpuHandleMgr_, shaderMgr,
            DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE, dmShaderData_.opaque);
        FillShaderData(entityMgr, *uriMgr_, *gpuHandleMgr_, shaderMgr,
            DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_TRANSLUCENT, dmShaderData_.blend);
        FillShaderData(entityMgr, *uriMgr_, *gpuHandleMgr_, shaderMgr,
            DefaultMaterialShaderConstants::RENDER_SLOT_DEPTH, dmShaderData_.depth);
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
            { *prevWorldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *layerMgr_, ComponentQuery::Operation::OPTIONAL },
            { *jointMatricesMgr_, ComponentQuery::Operation::OPTIONAL },
            { *prevJointMatricesMgr_, ComponentQuery::Operation::OPTIONAL },
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
    const auto materialExtensionGen = materialExtensionMgr_->GetGenerationCounter();
    const auto environmentGen = environmentMgr_->GetGenerationCounter();
    const auto fogGen = fogMgr_->GetGenerationCounter();
    const auto postprocessGen = postProcessMgr_->GetGenerationCounter();
    const auto postprocessConfigurationGen = postProcessConfigMgr_->GetGenerationCounter();
    if (!frameRenderingQueued && (renderConfigurationGeneration_ == renderConfigurationGen) &&
        (cameraGeneration_ == cameraGen) && (lightGeneration_ == lightGen) &&
        (planarReflectionGeneration_ == planarReflectionGen) &&
        (materialExtensionGeneration_ == materialExtensionGen) && (environmentGeneration_ == environmentGen) &&
        (fogGeneration_ == fogGen) && (postprocessGeneration_ == postprocessGen) &&
        (postprocessConfigurationGeneration_ == postprocessConfigurationGen)) {
        return false;
    }

    renderConfigurationGeneration_ = renderConfigurationGen;
    cameraGeneration_ = cameraGen;
    lightGeneration_ = lightGen;
    planarReflectionGeneration_ = planarReflectionGen;
    materialExtensionGeneration_ = materialExtensionGen;
    environmentGeneration_ = environmentGen;
    fogGeneration_ = fogGen;
    postprocessGeneration_ = postprocessGen;
    postprocessConfigurationGeneration_ = postprocessConfigurationGen;

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
    lightQuery_.SetEcsListenersEnabled(false);
    renderableQuery_.SetEcsListenersEnabled(false);
    reflectionsQuery_.SetEcsListenersEnabled(false);
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
    // Grab active camera.
    const auto cameraCount = cameraMgr_->GetComponentCount();
    for (IComponentManager::ComponentId id = 0; id < cameraCount; ++id) {
        if (auto handle = cameraMgr_->Read(id); handle) {
            if (handle->sceneFlags & CameraComponent::SceneFlagBits::MAIN_CAMERA_BIT) {
                cameraEntity = cameraMgr_->GetEntity(id);
                break;
            }
        }
    }
    dsLight_->SetShadowTypes(GetRenderShadowTypes(sc), 0u);

    // NOTE: removed code for "No main camera set, grab 1st one (if any)."

    return cameraEntity;
}

void RenderSystem::EvaluateMaterialModifications(const MaterialComponent& matComp)
{
    // update built-in pipeline modifications for default materials
    CORE_ASSERT(matComp.type <= MaterialComponent::Type::CUSTOM_COMPLEX);
    if (matComp.type < MaterialComponent::Type::CUSTOM) {
        if (matComp.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x > 0.0f) {
            // NOTE: should be alpha blend and not double sided, should be set by material author
            // automatically done by e.g. gltf2 importer
            renderProcessing_.frameFlags |= NEEDS_COLOR_PRE_PASS; // when allowing prepass on demand
        }
    }
}

uint32_t RenderSystem::ProcessSubmesh(const MeshProcessData& mpd, const MeshComponent::Submesh& submesh,
    const uint32_t meshIndex, const uint32_t subMeshIndex, const uint32_t skinJointIndex, const MinAndMax& mam,
    const bool isNegative)
{
    uint32_t materialIndex = RenderSceneDataConstants::INVALID_INDEX;
    // The cost of constructing RenderSubmesh is suprisingly high. alternatives are copy-construction from a constant
    // or perhaps directly assinging the final values.
    RenderSubmesh renderSubmesh { INIT };
    renderSubmesh.id = mpd.renderMeshEntity.id;
    renderSubmesh.meshId = mpd.meshEntity.id;
    renderSubmesh.subMeshIndex = subMeshIndex;
    renderSubmesh.layerMask = mpd.layerMask; // pass node layer mask for render submesh
    renderSubmesh.renderSortLayer = submesh.renderSortLayer;
    renderSubmesh.renderSortLayerOrder = submesh.renderSortLayerOrder;
    renderSubmesh.meshIndex = meshIndex;
    renderSubmesh.skinJointIndex = skinJointIndex;
    renderSubmesh.worldCenter = (mam.minAABB + mam.maxAABB) * 0.5f;
    renderSubmesh.worldRadius = Math::sqrt(Math::max(Math::Distance2(renderSubmesh.worldCenter, mam.minAABB),
        Math::Distance2(renderSubmesh.worldCenter, mam.maxAABB)));

    SetupSubmeshBuffers(*gpuHandleMgr_, mpd.meshComponent, submesh, renderSubmesh);

    // Clear skinning bit if joint matrices were not given.
    if (skinJointIndex == RenderSceneDataConstants::INVALID_INDEX) {
        renderSubmesh.submeshFlags &= ~RenderSubmeshFlagBits::RENDER_SUBMESH_SKIN_BIT;
    }
    if (isNegative) {
        renderSubmesh.submeshFlags |= RenderSubmeshFlagBits::RENDER_SUBMESH_INVERSE_WINDING_BIT;
    }

    renderSubmesh.drawCommand.vertexCount = submesh.vertexCount;
    renderSubmesh.drawCommand.indexCount = submesh.indexCount;
    // NOTE: batch instance count
    renderSubmesh.drawCommand.instanceCount = submesh.instanceCount + mpd.batchInstanceCount;

    // overwrites the render submesh for every new material
    auto addMaterial = [&](Entity materialEntity) {
        if (EntityUtil::IsValid(materialEntity)) {
            if (auto materialData = materialMgr_->Read(materialEntity); materialData) {
                if (materialData->extraRenderingFlags & MaterialComponent::DISABLE_BIT) {
                    return; // early out from lambda with disabled material
                }
                EvaluateMaterialModifications(*materialData);
            }
        }
        RenderMaterialIndices matIndices = AddRenderMaterial(*materialMgr_, *materialExtensionMgr_, *gpuHandleMgr_,
            *dsMaterial_, materialEntity, renderSubmesh.drawCommand.instanceCount, mpd.duplicateMaterialInstances);
        renderSubmesh.materialIndex = matIndices.materialIndex;
        renderSubmesh.customResourcesIndex = matIndices.materialCustomResourceIndex;
        dsMaterial_->AddSubmesh(renderSubmesh);
    };
    addMaterial(submesh.material);
    materialIndex = renderSubmesh.materialIndex;

    // updates only the material related indices in renderSubmesh for additional materials
    // NOTE: there will be as many RenderSubmeshes as materials
    for (const auto& matRef : submesh.additionalMaterials) {
        addMaterial(matRef);
    }
    return materialIndex;
}

void RenderSystem::ProcessMesh(const MeshProcessData& mpd, const MinAndMax& batchMam, const SkinProcessData& spd,
    vector<uint32_t>* submeshMaterials)
{
    CORE_STATIC_ASSERT(sizeof(RenderMeshComponent::customData) == sizeof(RenderMeshData::customData));
    RenderMeshData rmd { mpd.world, mpd.world, mpd.prevWorld, mpd.renderMeshEntity.id, mpd.meshEntity.id,
        mpd.layerMask };
    std::copy(std::begin(mpd.renderMeshComponent.customData), std::end(mpd.renderMeshComponent.customData),
        std::begin(rmd.customData));
    const uint32_t meshIndex = dsMaterial_->AddMeshData(rmd);
    uint32_t skinIndex = RenderSceneDataConstants::INVALID_INDEX;
    const bool useJoints = spd.jointMatricesComponent && (spd.jointMatricesComponent->count > 0);
    if (useJoints) {
        CORE_ASSERT(spd.prevJointMatricesComponent);
        const auto jm = array_view<Math::Mat4X4 const>(
            spd.jointMatricesComponent->jointMatrices, spd.jointMatricesComponent->count);
        const auto pjm = array_view<Math::Mat4X4 const>(
            spd.prevJointMatricesComponent->jointMatrices, spd.prevJointMatricesComponent->count);
        skinIndex = dsMaterial_->AddSkinJointMatrices(jm, pjm);
    }

    const bool isMeshNegative = Math::Determinant(mpd.world) < 0.0f;
    // NOTE: When object is skinned we use the mesh bounding box for all the submeshes because currently
    // there is no way to know here which joints affect one specific renderSubmesh.
    MinAndMax skinnedMeshBounds;
    if (useJoints) {
        skinnedMeshBounds.minAABB = spd.jointMatricesComponent->jointsAabbMin;
        skinnedMeshBounds.maxAABB = spd.jointMatricesComponent->jointsAabbMax;
    }
    const auto aabbs =
        static_cast<RenderPreprocessorSystem*>(renderPreprocessorSystem_)->GetRenderMeshAabbs(mpd.renderMeshEntity);
    CORE_ASSERT(picking_);
    for (uint32_t subMeshIdx = 0; subMeshIdx < mpd.meshComponent.submeshes.size(); ++subMeshIdx) {
        const auto& submesh = mpd.meshComponent.submeshes[subMeshIdx];

        MinAndMax mam = [&]() {
            if (useJoints) {
                return skinnedMeshBounds;
            }
            if (subMeshIdx < aabbs.size()) {
                return MinAndMax { aabbs[subMeshIdx].min, aabbs[subMeshIdx].max };
            } else if (!aabbs.empty()) {
                return MinAndMax { aabbs[0U].min, aabbs[0U].max };
            }
            return MinAndMax {};
        }();
        if (mpd.batchInstanceCount > 0) {
            mam.minAABB = Math::min(mam.minAABB, batchMam.minAABB);
            mam.maxAABB = Math::max(mam.maxAABB, batchMam.maxAABB);
        }
        auto materialIndex = ProcessSubmesh(mpd, submesh, meshIndex, subMeshIdx, skinIndex, mam, isMeshNegative);
        if (submeshMaterials) {
            submeshMaterials->push_back(materialIndex);
        }
    }
}

void RenderSystem::ProcessRenderMeshBatch(const Entity renderMeshBatch, const ComponentQuery::ResultRow* row)
{
    const RenderMeshComponent renderMeshComponent = renderMeshMgr_->Get(row->components[RQ_RMC]);
    const Math::Mat4X4& world = worldMatrixMgr_->Read(row->components[RQ_WM])->matrix;
    const Math::Mat4X4& prevWorld = prevWorldMatrixMgr_->Read(row->components[RQ_PWM])->matrix;
    const uint64_t layerMask = !row->IsValidComponentId(RQ_L) ? LayerConstants::DEFAULT_LAYER_MASK
                                                              : layerMgr_->Get(row->components[RQ_L]).layerMask;
    // NOTE: direct component id for skins added to batch processing
    batches_[renderMeshBatch].push_back({ row->entity, renderMeshComponent.mesh, layerMask, row->components[RQ_JM],
        row->components[RQ_PJM], world, prevWorld });
}

void RenderSystem::ProcessRenderMeshBatch(array_view<const Entity> renderMeshComponents)
{
    uint32_t batchIndex = 0;
    uint32_t batchedCount = 0;
    auto& materialIndices = materialIndices_;
    ScopedHandle<const MeshComponent> meshHandle;
    const uint32_t batchInstCount = static_cast<uint32_t>(renderMeshComponents.size());
    for (const auto& entity : renderMeshComponents) {
        if (const auto row = renderableQuery_.FindResultRow(entity); row) {
            const RenderMeshComponent rmc = renderMeshMgr_->Get(row->components[RQ_RMC]);
            if (!meshHandle) {
                meshHandle = meshMgr_->Read(rmc.mesh);
            }
            if (meshHandle) {
                const Math::Mat4X4& world = worldMatrixMgr_->Read(row->components[RQ_WM])->matrix;
                const Math::Mat4X4& prevWorld = prevWorldMatrixMgr_->Read(row->components[RQ_PWM])->matrix;
                const uint64_t layerMask = !row->IsValidComponentId(RQ_L)
                                               ? LayerConstants::DEFAULT_LAYER_MASK
                                               : layerMgr_->Read(row->components[RQ_L])->layerMask;
                if (batchIndex == 0) {
                    LogBatchValidation(*meshHandle);
                    materialIndices.clear();
                    materialIndices.reserve(meshHandle->submeshes.size());
                    const uint32_t currBatchCount = Math::min(batchInstCount - batchedCount, MAX_BATCH_OBJECT_COUNT);
                    // process AABBs for all instances, the same mesh is used for all instances with their own
                    // transform
                    const auto mam = GetMeshMinAndMax(*renderPreprocessorSystem_, entity,
                        array_view(renderMeshComponents.data() + batchedCount, currBatchCount));

                    batchedCount += currBatchCount;
                    MeshProcessData mpd { layerMask, currBatchCount - 1u, true, entity, rmc.mesh, *meshHandle, rmc,
                        world, prevWorld };
                    // Optional skin, cannot change based on submesh)
                    if (row->IsValidComponentId(RQ_JM) && row->IsValidComponentId(RQ_PJM)) {
                        auto const jointMatricesData = jointMatricesMgr_->Read(row->components[RQ_JM]);
                        auto const prevJointMatricesData = prevJointMatricesMgr_->Read(row->components[RQ_PJM]);
                        const SkinProcessData spd { &(*jointMatricesData), &(*prevJointMatricesData) };
                        ProcessMesh(mpd, mam, spd, &materialIndices);
                    } else {
                        ProcessMesh(mpd, mam, {}, &materialIndices);
                    }
                } else {
                    // NOTE: normal matrix is missing
                    RenderMeshData rmd { world, world, prevWorld, entity.id, rmc.mesh.id, layerMask };
                    std::copy(std::begin(rmc.customData), std::end(rmc.customData), std::begin(rmd.customData));
                    dsMaterial_->AddMeshData(rmd);
                    // NOTE: materials have been automatically duplicated for all instance
                }
                if (++batchIndex == MAX_BATCH_OBJECT_COUNT) {
                    batchIndex = 0;
                }
            }
        }
    }
}

void RenderSystem::ProcessSingleRenderMesh(Entity renderMeshComponent)
{
    // add a single mesh
    if (const auto row = renderableQuery_.FindResultRow(renderMeshComponent); row) {
        const RenderMeshComponent rms = renderMeshMgr_->Get(row->components[RQ_RMC]);
        if (const auto meshData = meshMgr_->Read(rms.mesh); meshData) {
            const WorldMatrixComponent world = worldMatrixMgr_->Get(row->components[RQ_WM]);
            const PreviousWorldMatrixComponent prevWorld = prevWorldMatrixMgr_->Get(row->components[RQ_PWM]);
            const uint64_t layerMask = !row->IsValidComponentId(RQ_L) ? LayerConstants::DEFAULT_LAYER_MASK
                                                                      : layerMgr_->Get(row->components[RQ_L]).layerMask;
            const MeshProcessData mpd { layerMask, 0, false, row->entity, rms.mesh, *meshData, rms, world.matrix,
                prevWorld.matrix };
            // (4, 5) JointMatrixComponents are optional.
            if (row->IsValidComponentId(RQ_JM) && row->IsValidComponentId(RQ_PJM)) {
                auto const jointMatricesData = jointMatricesMgr_->Read(row->components[RQ_JM]);
                auto const prevJointMatricesData = prevJointMatricesMgr_->Read(row->components[RQ_PJM]);
                CORE_ASSERT(jointMatricesData);
                CORE_ASSERT(prevJointMatricesData);
                const SkinProcessData spd { &(*jointMatricesData), &(*prevJointMatricesData) };
                ProcessMesh(mpd, {}, spd, nullptr);
            } else {
                ProcessMesh(mpd, {}, {}, nullptr);
            }
        }
    }
}

void RenderSystem::ProcessRenderables()
{
    renderableQuery_.Execute();
    {
        const auto renderBatchEntities =
            static_cast<RenderPreprocessorSystem*>(renderPreprocessorSystem_)->GetRenderBatchMeshEntities();
        for (const auto& rmc : renderBatchEntities) {
            if (const auto row = renderableQuery_.FindResultRow(rmc); row) {
                const RenderMeshComponent renderMeshComponent = renderMeshMgr_->Get(row->components[RQ_RMC]);
                // batched render mesh components not processed linearly
                ProcessRenderMeshBatch(renderMeshComponent.renderMeshBatch, row);
            }
        }
    }

    {
        auto currentIndex = 0U;
        auto batchStartIndex = 0U;
        Entity currentMesh;
        const auto instancingAllowedEntities =
            static_cast<RenderPreprocessorSystem*>(renderPreprocessorSystem_)->GetInstancingAllowedEntities();
        for (const auto& rmc : instancingAllowedEntities) {
            if (const auto row = renderableQuery_.FindResultRow(rmc); row) {
                const RenderMeshComponent renderMeshComponent = renderMeshMgr_->Get(row->components[RQ_RMC]);
                if (currentMesh != renderMeshComponent.mesh) {
                    // create batch when the mesh changes [batchStartIndex..currentIndex)
                    if (const auto batchSize = currentIndex - batchStartIndex; batchSize) {
                        ProcessRenderMeshBatch({ instancingAllowedEntities.data() + batchStartIndex, batchSize });
                    }

                    batchStartIndex = currentIndex;
                    currentMesh = renderMeshComponent.mesh;
                }
            }
            ++currentIndex;
        }

        // handle the tail
        if (const auto batchSize = currentIndex - batchStartIndex; batchSize) {
            ProcessRenderMeshBatch({ instancingAllowedEntities.data() + batchStartIndex, batchSize });
        }
    }

    {
        const auto instancingDisabledEntities =
            static_cast<RenderPreprocessorSystem*>(renderPreprocessorSystem_)->GetInstancingDisabledEntities();
        for (const auto& rmc : instancingDisabledEntities) {
            ProcessSingleRenderMesh(rmc);
        }
    }

    ProcessBatchRenderables();
}

void RenderSystem::ProcessBatchRenderables()
{
    auto& materialIndices = materialIndices_;
    for (const auto& batchRef : batches_) {
        uint32_t batchIndex = 0;
        uint32_t batchedCount = 0;
        const uint32_t batchInstCount = static_cast<uint32_t>(batchRef.second.size());
        for (uint32_t entIdx = 0; entIdx < batchInstCount; ++entIdx) {
            const auto& inst = batchRef.second[entIdx];
            const Entity& entRef = inst.entity;
            const Entity& meshEntRef = inst.mesh;
            if (const auto meshData = meshMgr_->Read(meshEntRef); meshData) {
                const auto& mesh = *meshData;
                const RenderMeshComponent rmc = renderMeshMgr_->Get(entRef);
                // process the first fully
                if (batchIndex == 0) {
                    LogBatchValidation(mesh);

                    materialIndices.clear();
                    materialIndices.reserve(meshData->submeshes.size());
                    const uint32_t currPatchCount = Math::min(batchInstCount - batchedCount, MAX_BATCH_OBJECT_COUNT);
                    // process AABBs for all instances, the same mesh is used for all instances with their own
                    // transform
                    MinAndMax mam;
                    const BatchIndices batchIndices { ~0u, entIdx + 1, entIdx + currPatchCount };
                    CombineBatchWorldMinAndMax(batchRef.second, batchIndices, mesh, mam);
                    batchedCount += currPatchCount;
                    MeshProcessData mpd { inst.layerMask, currPatchCount - 1u, false, entRef, meshEntRef, mesh, rmc,
                        inst.mtx, inst.prevWorld };
                    // Optional skin, cannot change based on submesh)
                    if (inst.jointId != IComponentManager::INVALID_COMPONENT_ID) {
                        auto const jointMatricesData = jointMatricesMgr_->Read(inst.jointId);
                        auto const prevJointMatricesData = prevJointMatricesMgr_->Read(inst.prevJointId);
                        const SkinProcessData spd { &(*jointMatricesData), &(*prevJointMatricesData) };
                        ProcessMesh(mpd, mam, spd, &materialIndices);
                    } else {
                        ProcessMesh(mpd, mam, {}, &materialIndices);
                    }
                } else {
                    // NOTE: normal matrix is missing
                    RenderMeshData rmd { inst.mtx, inst.mtx, inst.prevWorld, entRef.id, meshEntRef.id, inst.layerMask };
                    std::copy(std::begin(rmc.customData), std::end(rmc.customData), std::begin(rmd.customData));
                    dsMaterial_->AddMeshData(rmd);
                    // NOTE: we force add a new material for every instance object, but we do not fetch texture
                    // handles

                    for (size_t i = 0U, count = meshData->submeshes.size(); i < count; ++i) {
                        AddSingleMaterial(*materialMgr_, *materialExtensionMgr_, *gpuHandleMgr_, *dsMaterial_,
                            meshData->submeshes[i].material, false, false, materialIndices[i], batchIndex, 1U, false);
                    }
                }
                if (++batchIndex == MAX_BATCH_OBJECT_COUNT) {
                    batchIndex = 0;
                }
            }
        }
    }
    // NOTE: we destroy batch entity if its elements were not used in this frame
    DestroyBatchData(batches_);
}

void RenderSystem::CombineBatchWorldMinAndMax(
    const BatchDataVector& batchVec, const BatchIndices& batchIndices, const MeshComponent& mesh, MinAndMax& mam) const
{
    CORE_ASSERT(picking_);
    CORE_ASSERT(batchIndices.batchEndCount <= static_cast<uint32_t>(batchVec.size()));
    const int32_t batchCount =
        static_cast<int32_t>(batchIndices.batchEndCount) - static_cast<int32_t>(batchIndices.batchStartIndex);
    if (batchCount <= 1) {
        return;
    }
    if (batchIndices.submeshIndex == ~0u) {
        for (uint32_t bIdx = batchIndices.batchStartIndex; bIdx < batchIndices.batchEndCount; ++bIdx) {
            const BatchData& bData = batchVec[bIdx];
            const auto meshAabb =
                static_cast<RenderPreprocessorSystem*>(renderPreprocessorSystem_)->GetRenderMeshAabb(bData.entity);
            mam.minAABB = Math::min(mam.minAABB, meshAabb.min);
            mam.maxAABB = Math::max(mam.maxAABB, meshAabb.max);
        }
    } else if (batchIndices.submeshIndex < mesh.submeshes.size()) {
        for (uint32_t bIdx = batchIndices.batchStartIndex; bIdx < batchIndices.batchEndCount; ++bIdx) {
            const BatchData& bData = batchVec[bIdx];
            const auto submeshAabbs =
                static_cast<RenderPreprocessorSystem*>(renderPreprocessorSystem_)->GetRenderMeshAabbs(bData.entity);
            if (batchIndices.submeshIndex < submeshAabbs.size()) {
                mam.minAABB = Math::min(mam.minAABB, submeshAabbs[batchIndices.submeshIndex].min);
                mam.maxAABB = Math::max(mam.maxAABB, submeshAabbs[batchIndices.submeshIndex].min);
            }
        }
    }
}

void RenderSystem::ProcessEnvironments(const RenderConfigurationComponent& renderConfig)
{
    if (!(environmentMgr_ && materialExtensionMgr_ && layerMgr_ && gpuHandleMgr_)) {
        return;
    }

    // default environment
    RenderCamera::Environment defaultEnv {};

    const auto envCount = environmentMgr_->GetComponentCount();
    for (IComponentManager::ComponentId id = 0; id < envCount; ++id) {
        ScopedHandle<const EnvironmentComponent> handle = environmentMgr_->Read(id);
        const EnvironmentComponent& component = *handle;

        const Entity envEntity = environmentMgr_->GetEntity(id);
        uint64_t layerMask = LayerConstants::DEFAULT_LAYER_MASK;
        if (auto layerHandle = layerMgr_->Read(envEntity); layerHandle) {
            layerMask = layerHandle->layerMask;
        }

        RenderCamera::Environment renderEnv;
        FillRenderEnvironment(*gpuHandleMgr_, layerMask, envEntity, component, renderEnv);
        // material custom resources (first check preferred custom resources)
        if (!component.customResources.empty()) {
            const size_t maxCustomCount = Math::min(
                component.customResources.size(), static_cast<size_t>(MaterialExtensionComponent::RESOURCE_COUNT));
            for (size_t idx = 0; idx < maxCustomCount; ++idx) {
                renderEnv.customResourceHandles[idx] =
                    gpuHandleMgr_->GetRenderHandleReference(component.customResources[idx]);
            }
        } else if (const auto matExtHandle = materialExtensionMgr_->Read(envEntity); matExtHandle) {
            const MaterialExtensionComponent& matExtComp = *matExtHandle;
            constexpr size_t maxCount = Math::min(static_cast<size_t>(MaterialExtensionComponent::RESOURCE_COUNT),
                static_cast<size_t>(RenderSceneDataConstants::MAX_ENV_CUSTOM_RESOURCE_COUNT));
            for (size_t idx = 0; idx < maxCount; ++idx) {
                renderEnv.customResourceHandles[idx] =
                    gpuHandleMgr_->GetRenderHandleReference(matExtComp.resources[idx]);
            }
        }
        dsCamera_->AddEnvironment(renderEnv);
        if (renderEnv.id == renderConfig.environment.id) {
            defaultEnv = renderEnv;
        }
    }
    // add default env with default data or render config data
    defaultEnv.id = RenderSceneDataConstants::INVALID_ID;
    dsCamera_->AddEnvironment(defaultEnv);
}

void RenderSystem::ProcessCameras(
    const RenderConfigurationComponent& renderConfig, const Entity& mainCameraEntity, RenderScene& renderScene)
{
    // The scene camera and active render cameras are added here. ProcessReflections reflection cameras.
    // This is temporary when moving towards camera based rendering in 3D context.
    const uint32_t mainCameraId = cameraMgr_->GetComponentId(mainCameraEntity);
    const auto cameraCount = cameraMgr_->GetComponentCount();
    vector<RenderCamera> tmpCameras;
    tmpCameras.reserve(cameraCount);
    unordered_map<uint64_t, uint64_t> mvChildToParent; // multi-view child to parent
    for (IComponentManager::ComponentId id = 0; id < cameraCount; ++id) {
        ScopedHandle<const CameraComponent> handle = cameraMgr_->Read(id);
        const CameraComponent& component = *handle;
        if ((mainCameraId != id) && ((component.sceneFlags & CameraComponent::SceneFlagBits::ACTIVE_RENDER_BIT) == 0)) {
            continue;
        }
        const Entity cameraEntity = cameraMgr_->GetEntity(id);
        const auto worldMatrixComponentId = worldMatrixMgr_->GetComponentId(cameraEntity);
        // Make sure we have render matrix.
        if (worldMatrixComponentId != IComponentManager::INVALID_COMPONENT_ID) {
            const WorldMatrixComponent renderMatrixComponent = worldMatrixMgr_->Get(worldMatrixComponentId);

            float determinant = 0.0f;
            const Math::Mat4X4 view = Math::Inverse(Math::Mat4X4(renderMatrixComponent.matrix.data), determinant);

            RenderCamera::Flags rcFlags = 0;
            bool createPrePassCam = false;
            if (mainCameraId == id) {
                renderScene.cameraIndex = static_cast<uint32_t>(tmpCameras.size());
                rcFlags = RenderCamera::CAMERA_FLAG_MAIN_BIT;
                createPrePassCam = (component.pipelineFlags & CameraComponent::FORCE_COLOR_PRE_PASS_BIT) ||
                                   (component.pipelineFlags & CameraComponent::ALLOW_COLOR_PRE_PASS_BIT);
                renderProcessing_.frameFlags |=
                    (component.pipelineFlags & CameraComponent::FORCE_COLOR_PRE_PASS_BIT) ? NEEDS_COLOR_PRE_PASS : 0;
            }

            bool isCameraNegative = determinant < 0.0f;
            const auto proj = CameraMatrixUtil::CalculateProjectionMatrix(component, isCameraNegative);

            RenderCamera camera;
            FillRenderCameraBaseFromCameraComponent(*gpuHandleMgr_, component, camera, true);
            // we add entity id as camera name if there isn't name (we need this for render node graphs)
            camera.id = cameraEntity.id;
            camera.name = GetCameraName(*nameMgr_, cameraEntity);
            camera.matrices.view = view;
            camera.matrices.proj = proj;
            const CameraData prevFrameCamData = UpdateAndGetPreviousFrameCameraData(cameraEntity, view, proj);
            camera.matrices.viewPrevFrame = prevFrameCamData.view;
            camera.matrices.projPrevFrame = prevFrameCamData.proj;
            camera.flags |= (rcFlags | ((isCameraNegative) ? RenderCamera::CAMERA_FLAG_INVERSE_WINDING_BIT : 0));
            FillCameraRenderEnvironment(dsCamera_, component, camera);
            camera.fog = GetRenderCameraFogFromComponent(layerMgr_, fogMgr_, renderConfig, component);
            camera.shaderFlags |=
                (camera.fog.id != RenderSceneDataConstants::INVALID_ID) ? RenderCamera::CAMERA_SHADER_FOG_BIT : 0U;
            camera.postProcessName = GetPostProcessName(
                postProcessMgr_, postProcessConfigMgr_, nameMgr_, properties_.dataStoreScene, component.postProcess);
            camera.customPostProcessRenderNodeGraphFile =
                GetPostProcessRenderNodeGraph(postProcessConfigMgr_, component.postProcess);

            // NOTE: setting up the color pre pass with a target name is a temporary solution
            if (createPrePassCam) {
                camera.prePassColorTargetName = renderScene.name +
                                                DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME +
                                                to_hex(PREPASS_CAMERA_START_UNIQUE_ID);
            }
            tmpCameras.push_back(camera);
            ProcessCameraAddMultiViewHash(camera, mvChildToParent);
            // The order of setting cameras matter (main camera index is set already)
            if (createPrePassCam) {
                tmpCameras.push_back(CreateColorPrePassRenderCamera(
                    *gpuHandleMgr_, *cameraMgr_, camera, component.prePassCamera, PREPASS_CAMERA_START_UNIQUE_ID));
            }
        }
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
}

void RenderSystem::ProcessReflection(const ComponentQuery::ResultRow& row, const RenderCamera& camera)
{
    // ReflectionsQuery has three required components:
    // (0) PlanarReflectionComponent
    // (1) WorldMatrixComponent
    // (2) NodeComponent
    // (3) RenderMeshComponent
    // Skip if this node is disabled.
    const NodeComponent nodeComponent = nodeMgr_->Get(row.components[2u]);
    if (!nodeComponent.effectivelyEnabled) {
        return;
    }
    PlanarReflectionComponent reflComponent = planarReflectionMgr_->Get(row.components[0u]);
    if ((reflComponent.additionalFlags & PlanarReflectionComponent::FlagBits::ACTIVE_RENDER_BIT) == 0) {
        return;
    }

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

    const ReflectionPlaneTargetUpdate rptu = UpdatePlaneReflectionTargetResolution(
        *gpuResourceMgr_, *gpuHandleMgr_, *planarReflectionMgr_, camera, row.entity, reflComponent);
    if (rptu.recreated) {
        UpdateReflectionPlaneMaterial(*renderMeshMgr_, *meshMgr_, *materialMgr_, row.entity, reflComponent, rptu);
    }

    RenderCamera reflCam;
    const uint64_t reflCamId = Hash(row.entity.id, camera.id);
    reflCam.id = reflCamId;
    reflCam.mainCameraId = camera.id; // link to main camera
    reflCam.layerMask = reflComponent.layerMask;
    reflCam.matrices.view = reflectedView;
    reflCam.matrices.proj = reflectedProjection;
    const CameraData prevFrameCamData =
        UpdateAndGetPreviousFrameCameraData({ reflCamId }, reflCam.matrices.view, reflCam.matrices.proj);
    reflCam.matrices.viewPrevFrame = prevFrameCamData.view;
    reflCam.matrices.projPrevFrame = prevFrameCamData.proj;
    const auto xFactor = (static_cast<float>(camera.renderResolution[0]) * reflComponent.screenPercentage) /
                         static_cast<float>(reflComponent.renderTargetResolution[0]);
    const auto yFactor = (static_cast<float>(camera.renderResolution[1]) * reflComponent.screenPercentage) /
                         static_cast<float>(reflComponent.renderTargetResolution[1]);
    reflCam.viewport = { camera.viewport[0u] * xFactor, camera.viewport[1u] * yFactor, camera.viewport[2u] * xFactor,
        camera.viewport[3u] * yFactor };
    reflCam.depthTarget = gpuHandleMgr_->GetRenderHandleReference(reflComponent.depthRenderTarget);
    reflCam.colorTargets[0u] = gpuHandleMgr_->GetRenderHandleReference(reflComponent.colorRenderTarget);

    reflCam.renderResolution = { reflComponent.renderTargetResolution[0], reflComponent.renderTargetResolution[1] };
    reflCam.zNear = camera.zNear;
    reflCam.zFar = camera.zFar;
    reflCam.flags = (reflComponent.additionalFlags & PlanarReflectionComponent::FlagBits::MSAA_BIT)
                        ? RenderCamera::CAMERA_FLAG_MSAA_BIT
                        : 0U;
    reflCam.flags |= (RenderCamera::CAMERA_FLAG_REFLECTION_BIT | RenderCamera::CAMERA_FLAG_INVERSE_WINDING_BIT);
    reflCam.flags |= (RenderCamera::CAMERA_FLAG_CUSTOM_TARGETS_BIT);
    reflCam.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
    reflCam.clearDepthStencil = camera.clearDepthStencil;
    reflCam.clearColorValues = camera.clearColorValues;
    reflCam.cullType = RenderCamera::CameraCullType::CAMERA_CULL_VIEW_FRUSTUM;
    reflCam.environment = camera.environment;
    reflCam.environmentCount =
        Math::min(camera.environmentCount, DefaultMaterialCameraConstants::MAX_ENVIRONMENT_COUNT);
    CloneData(reflCam.environmentIds, sizeof(reflCam.environmentIds), camera.environmentIds,
        sizeof(camera.environmentIds[0U]) * reflCam.environmentCount);
    reflCam.postProcessName = DefaultMaterialCameraConstants::CAMERA_REFLECTION_POST_PROCESS_PREFIX_NAME;
    dsCamera_->AddCamera(reflCam);
}

void RenderSystem::ProcessReflections(const RenderScene& renderScene)
{
    const size_t cameraCount = dsCamera_->GetCameraCount();
    if (cameraCount == 0) {
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
        for (size_t camIdx = 0; camIdx < cameraCount; ++camIdx) {
            const auto& cameras = dsCamera_->GetCameras();
            if (camIdx < cameras.size()) {
                const auto& cam = cameras[camIdx];
                if (((cam.flags & disableFlags) == 0) && ((cam.flags & enableFlags) != 0)) {
                    ProcessReflection(row, cam);
                }
            }
        }
    }
}

void RenderSystem::ProcessLight(const LightProcessData& lpd)
{
    const auto& lightComponent = lpd.lightComponent;
    RenderLight light { lpd.entity.id, lightComponent.lightLayerMask,
        { lpd.world[3u], 1.0f }, // the last column (3) of the world matrix contains the world position.
        { Math::Normalize(lpd.world * Math::Vec4(0.0f, 0.0f, -1.0f, 0.0f)), 0.0f },
        { lightComponent.color, lightComponent.intensity } };

    // See:
    // https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
    const float outer = Math::clamp(lightComponent.spotOuterAngle, lightComponent.spotInnerAngle, Math::PI / 2.0f);
    const float inner = Math::clamp(lightComponent.spotInnerAngle, 0.0f, outer);

    if (lightComponent.type == LightComponent::Type::DIRECTIONAL) {
        light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT;
    } else if (lightComponent.type == LightComponent::Type::POINT) {
        light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_POINT_LIGHT_BIT;
    } else if (lightComponent.type == LightComponent::Type::SPOT) {
        light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_SPOT_LIGHT_BIT;

        const float cosInnerConeAngle = cosf(inner);
        const float cosOuterConeAngle = cosf(outer);

        const float lightAngleScale = 1.0f / Math::max(0.001f, cosInnerConeAngle - cosOuterConeAngle);
        const float lightAngleOffset = -cosOuterConeAngle * lightAngleScale;

        light.spotLightParams = { lightAngleScale, lightAngleOffset, inner, outer };
    }
    light.range = ComponentUtilFunctions::CalculateSafeLightRange(lightComponent.range, lightComponent.intensity);

    if (lightComponent.shadowEnabled) {
        light.shadowFactors = { Math::clamp01(lightComponent.shadowStrength), lightComponent.shadowDepthBias,
            lightComponent.shadowNormalBias, 0.0f };
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
        const LightComponent lightComponent = lightMgr_->Get(row.components[0u]);
        const NodeComponent nodeComponent = nodeMgr_->Get(row.components[1u]);
        const WorldMatrixComponent renderMatrixComponent = worldMatrixMgr_->Get(row.components[2u]);
        const uint64_t layerMask = !row.IsValidComponentId(3u) ? LayerConstants::DEFAULT_LAYER_MASK
                                                               : layerMgr_->Get(row.components[3u]).layerMask;
        if (nodeComponent.effectivelyEnabled) {
            const Math::Mat4X4 world(renderMatrixComponent.matrix.data);
            const LightProcessData lpd { layerMask, row.entity, lightComponent, world, renderScene, spotLightIndex };
            ProcessLight(lpd);
        }
    }
}

void RenderSystem::ProcessPostProcesses()
{
    if (renderContext_ && postProcessMgr_ && postProcessConfigMgr_) {
        IRenderDataStoreManager& rdsMgr = renderContext_->GetRenderDataStoreManager();
        auto* dsPod = static_cast<IRenderDataStorePod*>(rdsMgr.GetRenderDataStore(POD_DATA_STORE_NAME));
        auto* dsPp = static_cast<IRenderDataStorePostProcess*>(rdsMgr.GetRenderDataStore(PP_DATA_STORE_NAME));
        if ((!dsPod) || (!dsPp)) {
            return;
        }

        const auto postProcessCount = postProcessMgr_->GetComponentCount();
        for (IComponentManager::ComponentId id = 0; id < postProcessCount; ++id) {
            if (const auto handle = postProcessMgr_->Read(id); handle) {
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

                const Entity ppEntity = postProcessMgr_->GetEntity(id);
                const auto ppName = GetPostProcessName(
                    postProcessMgr_, postProcessConfigMgr_, nameMgr_, properties_.dataStoreScene, ppEntity);
                auto const dataView = dsPod->Get(ppName);
                if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                    dsPod->Set(ppName, arrayviewU8(ppConfig));
                } else {
                    renderProcessing_.postProcessPods.emplace_back(ppName);
                    dsPod->CreatePod(POST_PROCESS_NAME, ppName, arrayviewU8(ppConfig));
                }
            }
        }
        const auto postProcessConfigCount = postProcessConfigMgr_->GetComponentCount();
        for (IComponentManager::ComponentId id = 0; id < postProcessConfigCount; ++id) {
            // NOTE: should check if nothing has changed and not copy data if it has not changed
            if (const auto handle = postProcessConfigMgr_->Read(id); handle) {
                const auto& pp = *handle;
                const Entity ppEntity = postProcessConfigMgr_->GetEntity(id);
                const auto ppName = GetPostProcessName(
                    postProcessMgr_, postProcessConfigMgr_, nameMgr_, properties_.dataStoreScene, ppEntity);
                if (!dsPp->Contains(ppName)) {
                    renderProcessing_.postProcessConfigs.emplace_back(ppName);
                    dsPp->Create(ppName);
                }
                for (const auto& ref : pp.postProcesses) {
                    const IRenderDataStorePostProcess::PostProcess::Variables vars =
                        FillPostProcessConfigurationVars(ref);
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
            if (auto* dataStore = static_cast<IRenderDataStorePod*>(rdsMgr.GetRenderDataStore(POD_DATA_STORE_NAME));
                dataStore) {
                for (const auto& ref : renderProcessing_.postProcessPods) {
                    dataStore->DestroyPod(POST_PROCESS_NAME, ref.c_str());
                }
            }
            if (auto* dataStore =
                    static_cast<IRenderDataStorePostProcess*>(rdsMgr.GetRenderDataStore(PP_DATA_STORE_NAME));
                dataStore) {
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
    CORE_CPU_PERF_BEGIN(graphCpuTimer, "ECS", "RenderSystem", "FetchRenderables_Cpu");
#endif

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

    ProcessEnvironments(renderConfig);
    ProcessCameras(renderConfig, cameraEntity, renderDataScene);
    ProcessReflections(renderDataScene);
    ProcessPostProcesses();

    // Process all render components.
    ProcessRenderables();

    // Process render node graphs automatically based on camera if needed bits set for properties
    // Some materials might request color pre-pass etc. (needs to be done after renderables are processed)
    ProcessRenderNodeGraphs(renderConfig, renderDataScene);

    // NOTE: move world sphere calculation to own system
    const auto boundingSphere = static_cast<RenderPreprocessorSystem*>(renderPreprocessorSystem_)->GetBoundingSphere();
    sceneBoundingSpherePosition_ = boundingSphere.center;
    sceneBoundingSphereRadius_ = boundingSphere.radius;

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

#if (CORE3D_DEV_ENABLED == 1)
    CORE_CPU_PERF_END(graphCpuTimer);
#endif
}

void RenderSystem::ProcessRenderNodeGraphs(
    const RenderConfigurationComponent& renderConfig, const RenderScene& renderScene)
{
    auto& orderedRngs = renderProcessing_.orderedRenderNodeGraphs;
    orderedRngs.clear();
    const bool createRngs =
        (renderConfig.renderingFlags & RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT);
    if (createRngs && graphicsContext_ && renderUtil_) {
        struct CameraOrdering {
            uint64_t id { RenderSceneDataConstants::INVALID_ID };
            uint64_t mainId { RenderSceneDataConstants::INVALID_ID };
            size_t renderCameraIdx { 0 };
        };
        const auto& renderCameras = dsCamera_->GetCameras();
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
            if ((cam.flags & ignoreFlags) == 0) {
                if (cam.flags & RenderCamera::CAMERA_FLAG_MAIN_BIT) {
                    mainCamIdx = camIdx;
                } else {
                    if (cam.mainCameraId == RenderSceneDataConstants::INVALID_ID) {
                        baseCameras.push_back({ cam.id, cam.mainCameraId, camIdx });
                    } else {
                        // do not add pre-pass camera if render processing does not need it
                        if (cam.flags & RenderCamera::CAMERA_FLAG_COLOR_PRE_PASS_BIT) {
                            if (renderProcessing_.frameFlags & NEEDS_COLOR_PRE_PASS) {
                                depCameras.push_back({ cam.id, cam.mainCameraId, camIdx });
                            }
                        } else {
                            depCameras.push_back({ cam.id, cam.mainCameraId, camIdx });
                        }
                    }
                }
            }
        }
        // main camera needs to be the last
        if (mainCamIdx < renderCameras.size()) {
            const auto& cam = renderCameras[mainCamIdx];
            baseCameras.push_back({ cam.id, cam.mainCameraId, mainCamIdx });
        }
        // insert dependency cameras to correct positions
        for (const auto& depCam : depCameras) {
            for (size_t idx = 0; idx < baseCameras.size(); ++idx) {
                if (depCam.mainId == baseCameras[idx].id) {
                    baseCameras.insert(baseCameras.begin() + int64_t(idx), depCam);
                    break;
                }
            }
        }
        // now cameras are in correct order if the dependencied were correct in RenderCameras

        // first create scene render node graph if needed
        // we need to have scene render node graph as a separate
        orderedRngs.push_back(GetSceneRenderNodeGraph(renderScene));

        // then, add valid camera render node graphs
        for (const auto& cam : baseCameras) {
            CORE_ASSERT(cam.renderCameraIdx < renderCameras.size());
            const auto& camRef = renderCameras[cam.renderCameraIdx];
            CORE_ASSERT(camRef.id != 0xFFFFFFFFffffffff); // there must be an id for uniqueness
            CameraRngsOutput camRngs = GetCameraRenderNodeGraphs(renderScene, camRef);
            if (camRngs.rngs.rngHandle) {
                orderedRngs.push_back(move(camRngs.rngs.rngHandle));
                if (camRngs.rngs.ppRngHandle) {
                    orderedRngs.push_back(move(camRngs.rngs.ppRngHandle));
                }
                for (uint32_t mvIdx = 0U; mvIdx < RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT;
                     ++mvIdx) {
                    if (camRngs.multiviewPpHandles[mvIdx]) {
                        orderedRngs.push_back(move(camRngs.multiviewPpHandles[mvIdx]));
                    }
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
            if (iter->second.enableAutoDestroy && iter->second.lastFrameIndex < ageLimit) {
                iter = renderProcessing_.camIdToRng.erase(iter);
            } else {
                ++iter;
            }
        }
    }
}

RenderSystem::CameraRngsOutput RenderSystem::GetCameraRenderNodeGraphs(
    const RenderScene& renderScene, const RenderCamera& renderCamera)
{
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
    CameraRngsOutput rngs;
    rngs.rngs.rngHandle = renderCamera.customRenderNodeGraph;
    if (!rngs.rngs.rngHandle) {
        if (auto iter = renderProcessing_.camIdToRng.find(renderCamera.id);
            iter != renderProcessing_.camIdToRng.cend()) {
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
                (iter->second.multiViewCameraCount != renderCamera.multiViewCameraCount);
            if (reCreate) {
                iter->second.rngs = {};
                const vector<RenderCamera> multiviewCameras = GetMultiviewCameras(renderCamera);
                auto newRngs = createNewRngs(rngm, renderUtil_, renderScene, renderCamera, multiviewCameras);
                // copy
                rngs = newRngs;
                iter->second.rngs = move(newRngs.rngs);
                // update multiview post process
                for (size_t mvIdx = 0; mvIdx < multiviewCameras.size(); ++mvIdx) {
                    const auto& mvCamera = multiviewCameras[mvIdx];
                    auto& mvData = renderProcessing_.camIdToRng[mvCamera.id];
                    mvData.rngs.ppRngHandle = move(newRngs.multiviewPpHandles[mvIdx]);
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
            iter->second.lastFrameIndex = frameIndex_;
            iter->second.flags = renderCamera.flags;
            iter->second.renderPipelineType = renderCamera.renderPipelineType;
            iter->second.postProcessName = renderCamera.postProcessName;
            iter->second.customRngFile = renderCamera.customRenderNodeGraphFile;
            iter->second.customPostProcessRngFile = renderCamera.customPostProcessRenderNodeGraphFile;
            iter->second.multiViewCameraCount = renderCamera.multiViewCameraCount;
        } else {
            const vector<RenderCamera> multiviewCameras = GetMultiviewCameras(renderCamera);
            auto newRngs = createNewRngs(rngm, renderUtil_, renderScene, renderCamera, multiviewCameras);
            rngs = newRngs;
            renderProcessing_.camIdToRng[renderCamera.id] = { move(newRngs.rngs), renderCamera.flags,
                renderCamera.renderPipelineType, frameIndex_, true, renderCamera.postProcessName,
                renderCamera.customRenderNodeGraphFile, renderCamera.customPostProcessRenderNodeGraphFile,
                renderCamera.multiViewCameraCount };
            // update multiview post process
            for (size_t mvIdx = 0; mvIdx < multiviewCameras.size(); ++mvIdx) {
                const auto& mvCamera = multiviewCameras[mvIdx];
                auto& mvData = renderProcessing_.camIdToRng[mvCamera.id];
                mvData.rngs.ppRngHandle = move(newRngs.multiviewPpHandles[mvIdx]);
                mvData.lastFrameIndex = frameIndex_;
            }
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
        if (!renderProcessing_.sceneRngs.rng) {
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

ISystem* IRenderSystemInstance(IEcs& ecs)
{
    return new RenderSystem(ecs);
}

void IRenderSystemDestroy(ISystem* instance)
{
    delete static_cast<RenderSystem*>(instance);
}
CORE3D_END_NAMESPACE()
