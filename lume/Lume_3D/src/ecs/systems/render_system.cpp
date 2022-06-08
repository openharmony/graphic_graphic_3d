/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
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
#include "util/component_util_functions.h"
#include "util/mesh_util.h"
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

constexpr GpuImageDesc CreateReflectionPlaneGpuImageDesc(bool depthImage)
{
    GpuImageDesc desc;
    desc.depth = 1;
    desc.format = depthImage ? Format::BASE_FORMAT_D16_UNORM : Format::BASE_FORMAT_R8G8B8A8_SRGB;
    desc.memoryPropertyFlags = MemoryPropertyFlagBits::CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
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

inline fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> GetPostProcessName(
    const IPostProcessComponentManager* postProcessMgr, const Entity& entity)
{
    fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> ret =
        DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_PREFIX_NAME;
    if (postProcessMgr && postProcessMgr->HasComponent(entity)) {
        ret.append(to_string(entity.id));
    }
    return ret;
}

inline fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> GetCameraName(
    INameComponentManager& nameMgr, const Entity& entity)
{
    fixed_string<RenderDataConstants::MAX_DEFAULT_NAME_LENGTH> ret;
    if (ScopedHandle<const NameComponent> nameHandle = nameMgr.Read(entity); nameHandle) {
        ret.append(nameHandle->name);
    } else {
        ret.append(to_string(entity.id));
    }
    return ret;
}

// does not update all the variables
void FillRenderCameraBaseFromCameraComponent(const IRenderHandleComponentManager& renderHandleMgr,
    const CameraComponent& cameraComponent, RenderCamera& renderCamera, const bool checkCustomTargets)
{
    renderCamera.layerMask = cameraComponent.layerMask;
    renderCamera.viewport = { cameraComponent.viewport[0u], cameraComponent.viewport[1u], cameraComponent.viewport[2u],
        cameraComponent.viewport[3u] };
    renderCamera.scissor = { cameraComponent.scissor[0u], cameraComponent.scissor[1u], cameraComponent.scissor[2u],
        cameraComponent.scissor[3u] };
    renderCamera.renderResolution = { cameraComponent.renderResolution[0u], cameraComponent.renderResolution[1u] };
    renderCamera.zNear = cameraComponent.zNear;
    renderCamera.zFar = cameraComponent.zFar;
    renderCamera.flags = GetRenderCameraFlagsFromComponentFlags(cameraComponent.pipelineFlags);
    renderCamera.clearDepthStencil = { cameraComponent.clearDepthValue, 0u };
    renderCamera.clearColorValues = { cameraComponent.clearColorValue.x, cameraComponent.clearColorValue.y,
        cameraComponent.clearColorValue.z, cameraComponent.clearColorValue.w };
    renderCamera.cullType = GetRenderCameraCullTypeFromComponent(cameraComponent.culling);
    renderCamera.renderPipelineType = GetRenderCameraRenderPipelineTypeFromComponent(cameraComponent.renderingPipeline);
    if (cameraComponent.customRenderNodeGraph) {
        renderCamera.customRenderNodeGraph =
            renderHandleMgr.GetRenderHandleReference(cameraComponent.customRenderNodeGraph);
    }
    renderCamera.customRenderNodeGraphFile = cameraComponent.customRenderNodeGraphFile;
    const uint32_t maxCount = BASE_NS::Math::min(static_cast<uint32_t>(cameraComponent.colorTargetCustomization.size()),
        RenderSceneDataConstants::MAX_CAMERA_COLOR_TARGET_COUNT);
    for (uint32_t idx = 0; idx < maxCount; ++idx) {
        renderCamera.colorTargetCustomization[idx].format = cameraComponent.colorTargetCustomization[idx].format;
        renderCamera.colorTargetCustomization[idx].usageFlags =
            cameraComponent.colorTargetCustomization[idx].usageFlags;
    }
    renderCamera.depthTargetCustomization.format = cameraComponent.depthTargetCustomization.format;
    renderCamera.depthTargetCustomization.usageFlags = cameraComponent.depthTargetCustomization.usageFlags;

    if (checkCustomTargets && (cameraComponent.customColorTargets.size() > 0 || cameraComponent.customDepthTarget)) {
        renderCamera.targetType = RenderCamera::CameraTargetType::TARGET_TYPE_CUSTOM_TARGETS;
        RenderHandleReference customColorTarget;
        if (cameraComponent.customColorTargets.size() > 0) {
            customColorTarget = renderHandleMgr.GetRenderHandleReference(cameraComponent.customColorTargets[0]);
        }
        RenderHandleReference customDepthTarget =
            renderHandleMgr.GetRenderHandleReference(cameraComponent.customDepthTarget);
        if (customColorTarget.GetHandleType() != RenderHandleType::GPU_IMAGE) {
            CORE_LOG_E("invalid custom render target(s) for camera (%s)", renderCamera.name.c_str());
        }
        renderCamera.depthTarget = move(customDepthTarget);
        renderCamera.colorTargets[0u] = move(customColorTarget);
    }

    ValidateRenderCamera(renderCamera);
}

RenderCamera CreateColorPrePassRenderCamera(const IRenderHandleComponentManager& renderHandleMgr,
    const ICameraComponentManager& cameraMgr, const RenderCamera& baseCamera, const Entity& prePassEntity,
    const uint64_t uniqueId)
{
    RenderCamera rc = baseCamera;
    // reset targets, pre-pass does not support custom targets nor uses main camera targets
    rc.depthTarget = {};
    for (uint32_t idx = 0; idx < countof(rc.colorTargets); ++idx) {
        rc.colorTargets[idx] = {};
    }
    // NOTE: LIGHT_FORWARD prevents additional HDR target creation
    rc.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
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
    rc.prePassColorTarget = {};
    rc.prePassColorTargetName = {};
    rc.postProcessName = DefaultMaterialCameraConstants::CAMERA_PRE_PASS_POST_PROCESS_PREFIX_NAME;
    return rc;
}

void FillRenderEnvironment(const IRenderHandleComponentManager* renderHandleMgr, const uint64_t& nodeLayerMask,
    const Entity& entity, const EnvironmentComponent& component, RenderCamera::Environment& renderEnv)
{
    renderEnv.id = entity.id;
    renderEnv.layerMask = nodeLayerMask;
    renderEnv.shader = renderHandleMgr->GetRenderHandleReference(component.shader);
    if (component.background == EnvironmentComponent::Background::NONE) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_NONE;
    } else if (component.background == EnvironmentComponent::Background::IMAGE) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_IMAGE;
    } else if (component.background == EnvironmentComponent::Background::CUBEMAP) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_CUBEMAP;
    } else if (component.background == EnvironmentComponent::Background::EQUIRECTANGULAR) {
        renderEnv.backgroundType = RenderCamera::Environment::BG_TYPE_EQUIRECTANGULAR;
    }
    renderEnv.radianceCubemap = renderHandleMgr->GetRenderHandleReference(component.radianceCubemap);
    renderEnv.radianceCubemapMipCount = component.radianceCubemapMipCount;
    renderEnv.envMap = renderHandleMgr->GetRenderHandleReference(component.envMap);
    renderEnv.envMapLodLevel = component.envMapLodLevel;
    const size_t shCount =
        std::min(countof(component.irradianceCoefficients), countof(renderEnv.shIndirectCoefficients));
    for (size_t idx = 0; idx < shCount; ++idx) {
        renderEnv.shIndirectCoefficients[idx] = Math::Vec4(component.irradianceCoefficients[idx], 1.0);
    }
    renderEnv.indirectDiffuseFactor = component.indirectDiffuseFactor;
    renderEnv.indirectSpecularFactor = component.indirectSpecularFactor;
    renderEnv.envMapFactor = component.envMapFactor;
    renderEnv.rotation = component.environmentRotation;
}

RenderCamera::Environment GetRenderCameraEnvironmentFromComponent(const ILayerComponentManager* layerMgr,
    const IRenderHandleComponentManager* gpuHandleMgr, const IEnvironmentComponentManager* envMgr,
    const IMaterialExtensionComponentManager* matExtMgr,
    const RenderConfigurationComponent& renderConfigurationComponent, const CameraComponent& cameraComponent)
{
    RenderCamera::Environment renderEnv;
    if (!(envMgr && matExtMgr)) {
        return renderEnv;
    }

    auto fillExtensionResources = [](const IRenderHandleComponentManager* renderHandleMgr,
                                      const MaterialExtensionComponent& component,
                                      RenderCamera::Environment& renderEnv) {
        constexpr uint32_t maxCount = Math::min(static_cast<uint32_t>(MaterialExtensionComponent::RESOURCE_COUNT),
            RenderSceneDataConstants::MAX_ENV_CUSTOM_RESOURCE_COUNT);
        for (uint32_t idx = 0; idx < maxCount; ++idx) {
            renderEnv.customResourceHandles[idx] = renderHandleMgr->GetRenderHandleReference(component.resources[idx]);
        }
    };

    const Entity cameraEnvEntity = cameraComponent.environment;
    // Check if camera has a valid environment
    Entity envEntity = cameraEnvEntity;
    auto envId = envMgr->GetComponentId(envEntity);
    if (envId == IComponentManager::INVALID_COMPONENT_ID) {
        // Next try if the scene has a valid environment
        envEntity = renderConfigurationComponent.environment;
        envId = envMgr->GetComponentId(envEntity);
    }
    if (envId != IComponentManager::INVALID_COMPONENT_ID) {
        if (auto envDataHandle = envMgr->Read(envId); envDataHandle) {
            const EnvironmentComponent& envComponent = *envDataHandle;
            uint64_t layerMask = LayerConstants::DEFAULT_LAYER_MASK;
            if (auto layerHandle = layerMgr->Read(envEntity); layerHandle) {
                layerMask = layerHandle->layerMask;
            }

            FillRenderEnvironment(gpuHandleMgr, layerMask, cameraEnvEntity, envComponent, renderEnv);
            if (const auto matExtHandle = matExtMgr->Read(envEntity); matExtHandle) {
                const MaterialExtensionComponent& matExtComp = *matExtHandle;
                fillExtensionResources(gpuHandleMgr, matExtComp, renderEnv);
            }
        }
    }
    return renderEnv;
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
    // Check if camera has a valid environment
    Entity envEntity = cameraFogEntity;
    auto envId = fogMgr->GetComponentId(envEntity);
    if (envId == IComponentManager::INVALID_COMPONENT_ID) {
        // Next try if the scene has a valid environment
        envEntity = renderConfigurationComponent.environment;
        envId = fogMgr->GetComponentId(envEntity);
    }
    if (envId != IComponentManager::INVALID_COMPONENT_ID) {
        if (auto fogDataHandle = fogMgr->Read(envId); fogDataHandle) {
            const FogComponent& fogComponent = *fogDataHandle;
            uint64_t layerMask = LayerConstants::DEFAULT_LAYER_MASK;
            if (auto layerHandle = layerMgr->Read(envEntity); layerHandle) {
                layerMask = layerHandle->layerMask;
            }

            fillRenderFog(layerMask, cameraFogEntity, fogComponent, renderFog);
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

uint32_t RenderMaterialFlagsFromMaterialValues(const MaterialComponent& matDesc,
    const RenderDataDefaultMaterial::MaterialHandles& handles, const uint32_t hasTransformBit)
{
    uint32_t rmf = 0;
    // enable built-in specialization for default materials
    CORE_ASSERT(matDesc.type <= MaterialComponent::Type::CUSTOM_COMPLEX);
    if (matDesc.type < MaterialComponent::Type::CUSTOM) {
        if (handles.images[MaterialComponent::TextureIndex::NORMAL] ||
            handles.images[MaterialComponent::TextureIndex::CLEARCOAT_NORMAL]) {
            // need to check for tangents as well with submesh
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_NORMAL_MAP_BIT;
        }
        if (matDesc.textures[MaterialComponent::TextureIndex::CLEARCOAT].factor.x > 0.0f) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_CLEAR_COAT_BIT;
        }
        if ((matDesc.textures[MaterialComponent::TextureIndex::SHEEN].factor.x > 0.0f) ||
            (matDesc.textures[MaterialComponent::TextureIndex::SHEEN].factor.y > 0.0f) ||
            (matDesc.textures[MaterialComponent::TextureIndex::SHEEN].factor.z > 0.0f)) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHEEN_BIT;
        }
        if (matDesc.textures[MaterialComponent::TextureIndex::SPECULAR].factor != Math::Vec4(1.f, 1.f, 1.f, 1.f) ||
            handles.images[MaterialComponent::TextureIndex::SPECULAR]) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SPECULAR_BIT;
        }
        if (matDesc.textures[MaterialComponent::TextureIndex::TRANSMISSION].factor.x > 0.0f) {
            rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_TRANSMISSION_BIT;
        }
    }
    if (hasTransformBit) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_TEXTURE_TRANSFORM_BIT;
    }
    if (matDesc.alphaCutoff < 1.0f) {
        rmf |= RenderMaterialFlagBits::RENDER_MATERIAL_SHADER_DISCARD_BIT;
    }
    // NOTE: earlier version had MASK here for opaque bit as well
    return rmf;
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
    RenderDataDefaultMaterial::InputMaterialUniforms mu;
    uint transformBits = 0u;
    {
        constexpr uint32_t index = 0u;
        const auto& tex = matDesc.textures[MaterialComponent::TextureIndex::BASE_COLOR];
        // NOTE: premultiplied alpha, applied here and therefore the baseColor factor is special
        const float alpha = tex.factor.w;
        const Math::Vec4 baseColor = {
            tex.factor.x * alpha,
            tex.factor.y * alpha,
            tex.factor.z * alpha,
            alpha,
        };
#if (CORE3D_VALIDATION_ENABLED == 1)
        ValidateInputColor(material, matDesc);
#endif

        auto& texRef = mu.textureData[index];
        texRef.factor = baseColor;
        texRef.translation = tex.transform.translation;
        texRef.rotation = tex.transform.rotation;
        texRef.scale = tex.transform.scale;
        const bool hasTransform = (((texRef.translation.x != 0.0f) || (texRef.translation.y != 0.0f)) ||
                                   (texRef.rotation != 0.0f) || ((texRef.scale.x != 1.0f) || (texRef.scale.y != 1.0f)));
        transformBits |= static_cast<uint32_t>(hasTransform) << index;
    }
    constexpr uint32_t texCount = Math::min(static_cast<uint32_t>(MaterialComponent::TextureIndex::TEXTURE_COUNT),
        RenderDataDefaultMaterial::MATERIAL_TEXTURE_COUNT);
    for (uint32_t idx = 1u; idx < texCount; ++idx) {
        const auto& tex = matDesc.textures[idx];
        auto& texRef = mu.textureData[idx];
        texRef.factor = tex.factor;
        texRef.translation = tex.transform.translation;
        texRef.rotation = tex.transform.rotation;
        texRef.scale = tex.transform.scale;
        const bool hasTransform = (((texRef.translation.x != 0.0f) || (texRef.translation.y != 0.0f)) ||
                                   (texRef.rotation != 0.0f) || ((texRef.scale.x != 1.0f) || (texRef.scale.y != 1.0f)));
        transformBits |= static_cast<uint32_t>(hasTransform) << idx;
    }
    mu.alphaCutoff = matDesc.alphaCutoff;
    mu.id = material.id;
    mu.texCoordSetBits = matDesc.useTexcoordSetBit;
    mu.texTransformSetBits = transformBits;
    return mu;
}

BEGIN_PROPERTY(IRenderSystem::Properties, ComponentMetadata)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreManager, "IRenderDataStoreManager", PropertyFlags::IS_HIDDEN)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreMaterial, "dataStoreMaterial", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreCamera, "dataStoreCamera", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreLight, "dataStoreLight", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreScene, "dataStoreScene", 0)
    DECL_PROPERTY2(IRenderSystem::Properties, dataStoreMorph, "dataStoreMorph", 0)
END_PROPERTY();

void SetupSubmeshBuffers(const IRenderHandleComponentManager& renderHandleManager, const MeshComponent& desc,
    const MeshComponent::Submesh& submesh, RenderSubmesh& renderSubmesh)
{
    CORE_STATIC_ASSERT(
        MeshComponent::Submesh::BUFFER_COUNT <= RENDER_NS::PipelineStateConstants::MAX_VERTEX_BUFFER_COUNT);
    // calculate real vertex buffer count and fill "safety" handles for default material
    // no default shader variants without joints etc.
    std::transform(std::begin(submesh.bufferAccess), std::end(submesh.bufferAccess),
        std::begin(renderSubmesh.vertexBuffers),
        [&renderSubmesh, &renderHandleManager](const MeshComponent::Submesh::BufferAccess& acc) {
            RenderVertexBuffer renderVb;
            if (EntityUtil::IsValid(acc.buffer)) {
                renderVb.bufferHandle = renderHandleManager.GetRenderHandleReference(acc.buffer);
            }
            if (renderVb.bufferHandle) {
                renderVb.bufferOffset = acc.offset;
                renderVb.byteSize = acc.byteSize;
            } else {
                renderVb.bufferHandle = renderSubmesh.vertexBuffers[0].bufferHandle; // expecting safety binding
                renderVb.bufferOffset = 0;
                renderVb.byteSize = 0;
            }
            // NOTE: we will get max amount of vertex buffers if there is at least one
            // should calculate the real needed vertx input declaration specific count
            if (renderVb.bufferHandle) {
                renderSubmesh.vertexBufferCount++;
            }
            return renderVb;
        });

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

uint32_t AddSingleMaterial(IMaterialComponentManager& materialMgr, IRenderHandleComponentManager& renderHandleMgr,
    IRenderDataStoreDefaultMaterial& dataStoreMaterial, const Entity& material, const bool useEntityId,
    const bool fetchMaterialHandles)
{
    uint32_t materialIdx = RenderSceneDataConstants::INVALID_INDEX;
    {
        // scoped
        auto matData = materialMgr.Read(material);
        const MaterialComponent& materialComp = (matData) ? *matData : DEF_MATERIAL_COMPONENT;
        // optimization for e.g. with batch GPU instancing (we do not care about material handles)
        const RenderDataDefaultMaterial::MaterialHandles materialHandles =
            fetchMaterialHandles ? MaterialHandles(materialComp, renderHandleMgr)
                                 : RenderDataDefaultMaterial::MaterialHandles {};
        const RenderDataDefaultMaterial::InputMaterialUniforms materialUniforms =
            InputMaterialUniformsFromMaterialComponent(material, materialComp);
        const uint32_t transformBits = materialUniforms.texTransformSetBits;

        const RenderMaterialFlags rmfFromBits =
            RenderMaterialLightingFlagsFromMaterialFlags(materialComp.materialLightingFlags);
        const RenderMaterialFlags rmfFromValues =
            RenderMaterialFlagsFromMaterialValues(materialComp, materialHandles, transformBits);
        const RenderMaterialFlags rmf = rmfFromBits | rmfFromValues;
        RenderDataDefaultMaterial::MaterialData data {
            RenderMaterialType(materialComp.type),
            materialComp.extraRenderingFlags,
            rmf,
            materialComp.customRenderSlotId,
            { renderHandleMgr.GetRenderHandleReference(materialComp.materialShader.shader),
                renderHandleMgr.GetRenderHandleReference(materialComp.materialShader.graphicsState) },
            { renderHandleMgr.GetRenderHandleReference(materialComp.depthShader.shader),
                renderHandleMgr.GetRenderHandleReference(materialComp.depthShader.graphicsState) },
        };
        array_view<const uint8_t> customData;
        if (materialComp.customProperties) {
            const auto buffer = static_cast<const uint8_t*>(materialComp.customProperties->RLock());
            constexpr auto setAndBinding = sizeof(uint32_t) * 2u;
            customData = array_view(buffer + setAndBinding, materialComp.customProperties->Size() - setAndBinding);
            materialComp.customProperties->RUnlock();
        }
        if (useEntityId) {
            materialIdx =
                dataStoreMaterial.AddMaterialData(material.id, materialUniforms, materialHandles, data, customData);
        } else {
            materialIdx = dataStoreMaterial.AddMaterialData(materialUniforms, materialHandles, data, customData);
        }
    }
    return materialIdx;
}

struct RenderMaterialIndices {
    uint32_t materialIndex { ~0u };
    uint32_t materialCustomResourceIndex { ~0u };
};
RenderMaterialIndices AddRenderMaterial(IMaterialComponentManager& materialMgr,
    IMaterialExtensionComponentManager& materialExtMgr, IRenderHandleComponentManager& renderHandleMgr,
    IRenderDataStoreDefaultMaterial& dataStoreMaterial, const Entity& material)
{
    RenderMaterialIndices indices;
    indices.materialIndex = dataStoreMaterial.GetMaterialIndex(material.id);
    if ((indices.materialIndex == RenderSceneDataConstants::INVALID_INDEX) && EntityUtil::IsValid(material)) {
        indices.materialIndex =
            AddSingleMaterial(materialMgr, renderHandleMgr, dataStoreMaterial, material, true, true);
        if (const auto matExtHandle = materialExtMgr.Read(material); matExtHandle) {
            const MaterialExtensionComponent& matExtComp = *matExtHandle;
            RenderHandleReference handleReferences[MaterialExtensionComponent::ResourceIndex::RESOURCE_COUNT];
            std::transform(std::begin(matExtComp.resources), std::end(matExtComp.resources),
                std::begin(handleReferences), [&renderHandleMgr](const EntityReference& matEntity) {
                    return renderHandleMgr.GetRenderHandleReference(matEntity);
                });
            // bind all handles, invalid handles are just ignored
            indices.materialCustomResourceIndex =
                dataStoreMaterial.AddMaterialCustomResources(material.id, handleReferences);
        }
    } else if ((indices.materialIndex != RenderSceneDataConstants::INVALID_INDEX) && EntityUtil::IsValid(material)) {
        indices.materialCustomResourceIndex = dataStoreMaterial.GetMaterialCustomResourceIndex(material.id);
    }

    // we add the default material component with default entity id (only added once)
    if (indices.materialIndex == RenderSceneDataConstants::INVALID_INDEX) {
        const Entity invalidEntity {};
        indices.materialIndex =
            AddSingleMaterial(materialMgr, renderHandleMgr, dataStoreMaterial, invalidEntity, true, true);
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
    const uint32_t newWidth = static_cast<uint32_t>(sceneCamera.renderResolution.x * reflComp.screenPercentage);
    const uint32_t newHeight = static_cast<uint32_t>(sceneCamera.renderResolution.y * reflComp.screenPercentage);

    ReflectionPlaneTargetUpdate rptu;

    const RenderHandle colorRenderTarget = gpuHandleMgr.GetRenderHandle(reflComp.colorRenderTarget);
    const RenderHandle depthRenderTarget = gpuHandleMgr.GetRenderHandle(reflComp.depthRenderTarget);
    // get current mip count
    rptu.mipCount = RenderHandleUtil::IsValid(colorRenderTarget)
                        ? gpuResourceMgr.GetImageDescriptor(gpuResourceMgr.Get(colorRenderTarget)).mipCount
                        : 1u;
    if ((!RenderHandleUtil::IsValid(colorRenderTarget)) || (!RenderHandleUtil::IsValid(depthRenderTarget)) ||
        (newWidth != reflComp.renderTargetResolution[0]) || (newHeight != reflComp.renderTargetResolution[1])) {
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
            } else if (depthImage) {
                return gpuResourceMgr.Create(
                    DefaultMaterialCameraConstants::CAMERA_DEPTH_PREFIX_NAME + to_string(id), desc);
            } else {
                return gpuResourceMgr.Create(
                    DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + to_string(id), desc);
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

Math::Mat4X4 CalculateProjectionMatrix(CameraComponent const& cameraComponent, bool& isCameraNegative)
{
    switch (cameraComponent.projection) {
        case CameraComponent::Projection::ORTHOGRAPHIC: {
            auto orthoProj = Math::OrthoRhZo(cameraComponent.xMag * -0.5f, cameraComponent.xMag * 0.5f,
                cameraComponent.yMag * -0.5f, cameraComponent.yMag * 0.5f, cameraComponent.zNear, cameraComponent.zFar);
            orthoProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
            return orthoProj;
        }

        case CameraComponent::Projection::PERSPECTIVE: {
            auto persProj = Math::PerspectiveRhZo(
                cameraComponent.yFov, cameraComponent.aspect, cameraComponent.zNear, cameraComponent.zFar);
            persProj[1][1] *= -1.f; // left-hand NDC while Vulkan right-handed -> flip y
            return persProj;
        }

        case CameraComponent::Projection::CUSTOM: {
            isCameraNegative = Math::Determinant(cameraComponent.customProjectionMatrix) < 0.0f;
            return cameraComponent.customProjectionMatrix;
        }

        default:
            return Math::Mat4X4();
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
      RENDER_SYSTEM_PROPERTIES(&properties_, array_view(ComponentMetadata))
{
    if (IEngine* engine = ecs_.GetClassFactory().GetInterface<IEngine>(); engine) {
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
        if (properties_.dataStoreManager) {
            SetDataStorePointers(*properties_.dataStoreManager);
        }
    }
}

void RenderSystem::SetDataStorePointers(IRenderDataStoreManager& manager)
{
    properties_.dataStoreManager = &manager;

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
        } else {
            CORE_LOG_E("DEPRECATED USAGE: RenderPreprocessorSystem not found. Add system to system graph.");
        }

        SetDataStorePointers(renderContext_->GetRenderDataStoreManager());
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
            { *nodeMgr_, ComponentQuery::Operation::REQUIRE },
            { *worldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *prevWorldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *layerMgr_, ComponentQuery::Operation::OPTIONAL },
            { *jointMatricesMgr_, ComponentQuery::Operation::OPTIONAL },
            { *prevJointMatricesMgr_, ComponentQuery::Operation::OPTIONAL },
        };
        renderableQuery_.SetEcsListenersEnabled(true);
        renderableQuery_.SetupQuery(*renderMeshMgr_, operations);
    }
    {
        const ComponentQuery::Operation operations[] = {
            { *worldMatrixMgr_, ComponentQuery::Operation::REQUIRE },
            { *nodeMgr_, ComponentQuery::Operation::REQUIRE },
        };
        reflectionsQuery_.SetEcsListenersEnabled(true);
        reflectionsQuery_.SetupQuery(*planarReflectionMgr_, operations);
    }
}

bool RenderSystem::Update(bool frameRenderingQueued, uint64_t totalTime, uint64_t deltaTime)
{
    if (!frameRenderingQueued) {
        return false;
    }
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
            break;
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

void RenderSystem::ProcessSubmesh(const MeshProcessData& mpd, const MeshComponent::Submesh& submesh,
    const uint32_t meshIndex, const uint32_t subMeshIndex, const uint32_t skinJointIndex, const MinAndMax& mam,
    const bool isNegative)
{
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
    renderSubmesh.worldRadius = Math::max(Math::Magnitude(renderSubmesh.worldCenter - mam.minAABB),
        Math::Magnitude(mam.maxAABB - renderSubmesh.worldCenter));

    {
        mpd.sceneBoundingVolume.sumOfSubmeshPoints += renderSubmesh.worldCenter;
        mpd.sceneBoundingVolume.minAABB = Math::min(mpd.sceneBoundingVolume.minAABB, mam.minAABB);
        mpd.sceneBoundingVolume.maxAABB = Math::max(mpd.sceneBoundingVolume.maxAABB, mam.maxAABB);
        mpd.sceneBoundingVolume.submeshCount++;
    }

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
    auto addMaterial = [&renderSubmesh, this](Entity materialEntity) {
        RenderMaterialIndices matIndices =
            AddRenderMaterial(*materialMgr_, *materialExtensionMgr_, *gpuHandleMgr_, *dsMaterial_, materialEntity);
        if (EntityUtil::IsValid(materialEntity)) {
            if (auto materialData = materialMgr_->Read(materialEntity); materialData) {
                EvaluateMaterialModifications(*materialData);
            }
        }
        renderSubmesh.materialIndex = matIndices.materialIndex;
        renderSubmesh.customResourcesIndex = matIndices.materialCustomResourceIndex;
        dsMaterial_->AddSubmesh(renderSubmesh);
    };
    addMaterial(submesh.material);
    // updates only the material related indices in renderSubmesh for additional materials
    // NOTE: there will be as many RenderSubmeshes as materials
    for (const auto& matRef : submesh.additionalMaterials) {
        addMaterial(matRef);
    }
}

void RenderSystem::ProcessMesh(const MeshProcessData& mpd, const MinAndMax& batchMam, const SkinProcessData& spd)
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

    CORE_ASSERT(picking_);
    for (uint32_t subMeshIdx = 0; subMeshIdx < mpd.meshComponent.submeshes.size(); ++subMeshIdx) {
        const auto& submesh = mpd.meshComponent.submeshes[subMeshIdx];
        MinAndMax mam =
            useJoints ? skinnedMeshBounds : picking_->GetWorldAABB(mpd.world, submesh.aabbMin, submesh.aabbMax);
        if (mpd.batchInstanceCount > 0) {
            mam.minAABB = Math::min(mam.minAABB, batchMam.minAABB);
            mam.maxAABB = Math::max(mam.maxAABB, batchMam.maxAABB);
        }
        ProcessSubmesh(mpd, submesh, meshIndex, subMeshIdx, skinIndex, mam, isMeshNegative);
    }
}

void RenderSystem::ProcessRenderables(SceneBoundingVolumeHelper& sceneBoundingVolumeHelper)
{
    renderableQuery_.Execute();

    for (const auto& row : renderableQuery_.GetResults()) {
        // In addition to the base our renderableQuery has two required components and one optional component:
        // (0) RenderMeshComponent
        // (1) NodeComponent
        // (2) WorldMatrixComponent
        // (3) PreviousWorldMatrixComponent
        // (4) LayerComponent (optional)
        // (5) JointMatrixComponent (optional)
        // (6) PreviousJointMatrixComponent (optional)
        const RenderMeshComponent renderMeshComponent = renderMeshMgr_->Get(row.components[0]);
        const NodeComponent nodeComponent = nodeMgr_->Get(row.components[1]);
        const WorldMatrixComponent renderMatrixComponent = worldMatrixMgr_->Get(row.components[2u]);
        const PreviousWorldMatrixComponent prevWorld = prevWorldMatrixMgr_->Get(row.components[3u]);
        const uint64_t layerMask = !row.IsValidComponentId(4u) ? LayerConstants::DEFAULT_LAYER_MASK
                                                               : layerMgr_->Get(row.components[4u]).layerMask;
        // batched render mesh components not processed linearly
        if (EntityUtil::IsValid(renderMeshComponent.renderMeshBatch)) {
            if (nodeComponent.effectivelyEnabled && (layerMask != LayerConstants::NONE_LAYER_MASK)) {
                // NOTE: direct component id for skins added to batch processing
                batches_[renderMeshComponent.renderMeshBatch].push_back(
                    { row.entity, renderMeshComponent.mesh, layerMask, row.components[5u], row.components[6u],
                        renderMatrixComponent.matrix, prevWorld.matrix });
            }
            continue;
        }
        // ignore disabled and nodes without layer mask
        if (nodeComponent.effectivelyEnabled && (layerMask != LayerConstants::NONE_LAYER_MASK)) {
            if (const auto meshData = meshMgr_->Read(renderMeshComponent.mesh); meshData) {
                const auto& mesh = *meshData;

                MeshProcessData mpd { layerMask, 0, row.entity, renderMeshComponent.mesh, mesh, renderMeshComponent,
                    renderMatrixComponent.matrix, prevWorld.matrix, sceneBoundingVolumeHelper };
                // (5, 6) JointMatrixComponents are optional.
                if (row.IsValidComponentId(5u) && row.IsValidComponentId(6u)) {
                    auto const jointMatricesData = jointMatricesMgr_->Read(row.components[5u]);
                    auto const prevJointMatricesData = prevJointMatricesMgr_->Read(row.components[6u]);
                    CORE_ASSERT(jointMatricesData);
                    CORE_ASSERT(prevJointMatricesData);
                    const SkinProcessData spd { &(*jointMatricesData), &(*prevJointMatricesData) };
                    ProcessMesh(mpd, {}, spd);
                } else {
                    ProcessMesh(mpd, {}, {});
                }
            }
        }
    }

    ProcessBatchRenderables(sceneBoundingVolumeHelper);
}

namespace {
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
} // namespace

void RenderSystem::ProcessBatchRenderables(SceneBoundingVolumeHelper& sceneBoundingVolumeHelper)
{
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

                    const uint32_t currPatchCount = Math::min(batchInstCount - batchedCount, MAX_BATCH_OBJECT_COUNT);
                    // process AABBs for all instances, the same mesh is used for all instances with their own transform
                    MinAndMax mam;
                    const BatchIndices batchIndices { ~0u, entIdx + 1, entIdx + currPatchCount };
                    CombineBatchWorldMinAndMax(batchRef.second, batchIndices, mesh, mam);
                    batchedCount += currPatchCount;
                    MeshProcessData mpd { inst.layerMask, currPatchCount - 1u, entRef, meshEntRef, mesh, rmc, inst.mtx,
                        inst.prevWorld, sceneBoundingVolumeHelper };
                    // Optional skin, cannot change based on submesh)
                    if (inst.jointId != IComponentManager::INVALID_COMPONENT_ID) {
                        auto const jointMatricesData = jointMatricesMgr_->Read(inst.jointId);
                        auto const prevJointMatricesData = prevJointMatricesMgr_->Read(inst.prevJointId);
                        const SkinProcessData spd { &(*jointMatricesData), &(*prevJointMatricesData) };
                        ProcessMesh(mpd, mam, spd);
                    } else {
                        ProcessMesh(mpd, mam, {});
                    }
                } else {
                    // NOTE: normal matrix is missing
                    RenderMeshData rmd { inst.mtx, inst.mtx, inst.prevWorld, entRef.id, meshEntRef.id, inst.layerMask };
                    std::copy(std::begin(rmc.customData), std::end(rmc.customData), std::begin(rmd.customData));
                    dsMaterial_->AddMeshData(rmd);
                    // NOTE: we force add a new material for every instance object, but we do not fetch texture handles
                    AddSingleMaterial(
                        *materialMgr_, *gpuHandleMgr_, *dsMaterial_, mesh.submeshes[0].material, false, false);
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
        // process first the full mesh, and then only transform per batch the AABB
        MinAndMax meshAABB;
        for (const auto& subRef : mesh.submeshes) {
            const BatchData& bData = batchVec[0u];
            const MinAndMax localMam = picking_->GetWorldAABB(bData.mtx, subRef.aabbMin, subRef.aabbMax);
            meshAABB.minAABB = Math::min(meshAABB.minAABB, localMam.minAABB);
            meshAABB.maxAABB = Math::max(meshAABB.maxAABB, localMam.maxAABB);
        }
        mam.minAABB = Math::min(mam.minAABB, meshAABB.minAABB);
        mam.maxAABB = Math::max(mam.maxAABB, meshAABB.maxAABB);
        // then start from 2nd index in batch
        for (uint32_t bIdx = batchIndices.batchStartIndex + 1; bIdx < batchIndices.batchEndCount; ++bIdx) {
            const BatchData& bData = batchVec[bIdx];
            const MinAndMax localMam = picking_->GetWorldAABB(bData.mtx, meshAABB.minAABB, meshAABB.maxAABB);
            mam.minAABB = Math::min(mam.minAABB, localMam.minAABB);
            mam.maxAABB = Math::max(mam.maxAABB, localMam.maxAABB);
        }
    } else if (batchIndices.submeshIndex < mesh.submeshes.size()) {
        const auto& subRef = mesh.submeshes[batchIndices.submeshIndex];
        for (uint32_t bIdx = batchIndices.batchStartIndex; bIdx < batchIndices.batchEndCount; ++bIdx) {
            const BatchData& bData = batchVec[bIdx];
            const MinAndMax localMam = picking_->GetWorldAABB(bData.mtx, subRef.aabbMin, subRef.aabbMax);
            mam.minAABB = Math::min(mam.minAABB, localMam.minAABB);
            mam.maxAABB = Math::max(mam.maxAABB, localMam.maxAABB);
        }
    }
}

void RenderSystem::CalculateSceneBounds(const SceneBoundingVolumeHelper& sceneBoundingVolumeHelper)
{
    if (sceneBoundingVolumeHelper.submeshCount == 0) {
        sceneBoundingSphereRadius_ = 0.0f;
        sceneBoundingSpherePosition_ = Math::Vec3();
    } else {
        const auto boundingSpherePosition =
            sceneBoundingVolumeHelper.sumOfSubmeshPoints / static_cast<float>(sceneBoundingVolumeHelper.submeshCount);

        const float radMin = Math::Magnitude(boundingSpherePosition - sceneBoundingVolumeHelper.minAABB);
        const float radMax = Math::Magnitude(sceneBoundingVolumeHelper.maxAABB - boundingSpherePosition);
        const float boundingSphereRadius = Math::max(radMin, radMax);

        // Compensate jitter and adjust scene bounding sphere only if change in bounds is meaningful.
        if (sceneBoundingSphereRadius_ > 0.0f) {
            // Calculate distance to new bounding sphere origin from current sphere.
            const float pointDistance = Math::Magnitude(boundingSpherePosition - sceneBoundingSpherePosition_);
            // Calculate distance to edge of new bounding sphere from current sphere origin.
            const float sphereEdgeDistance = pointDistance + boundingSphereRadius;

            // Calculate step size for adjustment, use 10% granularity from current bounds.
            constexpr float granularityPct = 0.10f;
            const float granularity = sceneBoundingSphereRadius_ * granularityPct;

            // Calculate required change of size, in order to fit new sphere inside current bounds.
            const float radDifference = sphereEdgeDistance - sceneBoundingSphereRadius_;
            const float posDifference = Math::Magnitude(boundingSpherePosition - sceneBoundingSpherePosition_);
            // We need to adjust only if the change is bigger than the step size.
            if ((Math::abs(radDifference) > granularity) || (posDifference > granularity)) {
                // Calculate how many steps we need to change and in to which direction.
                const int32_t radAmount =
                    (int32_t)ceil((boundingSphereRadius - sceneBoundingSphereRadius_) / granularity);
                const int32_t posAmount = (int32_t)ceil(posDifference / granularity);
                if ((radAmount != 0) || (posAmount != 0)) {
                    // Update size and position of the bounds.
                    sceneBoundingSpherePosition_ = boundingSpherePosition;
                    sceneBoundingSphereRadius_ = sceneBoundingSphereRadius_ + (radAmount * granularity);
                }
            }
        } else {
            // No existing bounds, start with new values.
            sceneBoundingSphereRadius_ = boundingSphereRadius;
            sceneBoundingSpherePosition_ = boundingSpherePosition;
        }
    }
}

void RenderSystem::ProcessCameras(
    const RenderConfigurationComponent& renderConfig, const Entity& mainCameraEntity, RenderScene& renderScene)
{
    // The scene camera and active render cameras are added here. ProcessReflections reflection cameras.
    // This is temporary when moving towards camera based rendering in 3D context.
    const uint32_t mainCameraId = cameraMgr_->GetComponentId(mainCameraEntity);
    const auto cameraCount = cameraMgr_->GetComponentCount();
    for (IComponentManager::ComponentId id = 0; id < cameraCount; ++id) {
        const CameraComponent component = cameraMgr_->Get(id);
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
                renderScene.cameraIndex = static_cast<uint32_t>(dsCamera_->GetCameraCount());
                rcFlags = RenderCamera::CAMERA_FLAG_MAIN_BIT;
                createPrePassCam = (component.pipelineFlags & CameraComponent::FORCE_COLOR_PRE_PASS_BIT) ||
                                   (component.pipelineFlags & CameraComponent::ALLOW_COLOR_PRE_PASS_BIT);
                renderProcessing_.frameFlags |=
                    (component.pipelineFlags & CameraComponent::FORCE_COLOR_PRE_PASS_BIT) ? NEEDS_COLOR_PRE_PASS : 0;
            }

            bool isCameraNegative = determinant < 0.0f;
            const auto proj = CalculateProjectionMatrix(component, isCameraNegative);

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
            camera.environment = GetRenderCameraEnvironmentFromComponent(
                layerMgr_, gpuHandleMgr_, environmentMgr_, materialExtensionMgr_, renderConfig, component);
            camera.fog = GetRenderCameraFogFromComponent(layerMgr_, fogMgr_, renderConfig, component);
            camera.postProcessName = GetPostProcessName(postProcessMgr_, component.postProcess);

            // NOTE: setting up the color pre pass with a target name is a temporary solution
            if (createPrePassCam) {
                camera.prePassColorTargetName = DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME +
                                                renderScene.name + to_string(PREPASS_CAMERA_START_UNIQUE_ID);
            }
            dsCamera_->AddCamera(camera);
            // The order of setting cameras matter (main camera index is set already)
            if (createPrePassCam) {
                dsCamera_->AddCamera(CreateColorPrePassRenderCamera(
                    *gpuHandleMgr_, *cameraMgr_, camera, component.prePassCamera, PREPASS_CAMERA_START_UNIQUE_ID));
            }
        }
    }
}

bool RenderSystem::ProcessReflection(const ComponentQuery::ResultRow& row, const RenderCamera& camera)
{
    // ReflectionsQuery has three required components:
    // (0) PlanarReflectionComponent
    // (1) WorldMatrixComponent
    // (2) NodeComponent
    // Skip if this node is disabled.
    const NodeComponent nodeComponent = nodeMgr_->Get(row.components[2u]);
    if (!nodeComponent.effectivelyEnabled) {
        return false;
    }
    PlanarReflectionComponent reflComponent = planarReflectionMgr_->Get(row.components[0u]);
    if ((reflComponent.additionalFlags & PlanarReflectionComponent::FlagBits::ACTIVE_RENDER_BIT) == 0) {
        return false;
    }
    const WorldMatrixComponent reflectionPlaneMatrix = worldMatrixMgr_->Get(row.components[1u]);

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
    reflCam.id = row.entity.id;
    reflCam.layerMask = reflComponent.layerMask;
    reflCam.matrices.view = reflectedView;
    reflCam.matrices.proj = reflectedProjection;
    const CameraData prevFrameCamData =
        UpdateAndGetPreviousFrameCameraData(row.entity, reflCam.matrices.view, reflCam.matrices.proj);
    reflCam.matrices.viewPrevFrame = prevFrameCamData.view;
    reflCam.matrices.projPrevFrame = prevFrameCamData.proj;
    reflCam.viewport = { camera.viewport[0u], camera.viewport[1u], camera.viewport[2u], camera.viewport[3u] };
    reflCam.targetType = RenderCamera::CameraTargetType::TARGET_TYPE_CUSTOM_TARGETS;
    reflCam.depthTarget = gpuHandleMgr_->GetRenderHandleReference(reflComponent.depthRenderTarget);
    reflCam.colorTargets[0u] = gpuHandleMgr_->GetRenderHandleReference(reflComponent.colorRenderTarget);

    reflCam.renderResolution = { reflComponent.renderTargetResolution[0], reflComponent.renderTargetResolution[1] };
    reflCam.zNear = camera.zNear;
    reflCam.zFar = camera.zFar;
    reflCam.flags = camera.flags & (~(RenderCamera::CAMERA_FLAG_MSAA_BIT | RenderCamera::CAMERA_FLAG_HISTORY_BIT));
    reflCam.flags |= (RenderCamera::CAMERA_FLAG_REFLECTION_BIT | RenderCamera::CAMERA_FLAG_INVERSE_WINDING_BIT);
    reflCam.renderPipelineType = RenderCamera::RenderPipelineType::LIGHT_FORWARD;
    reflCam.clearDepthStencil = camera.clearDepthStencil;
    reflCam.clearColorValues = camera.clearColorValues;
    reflCam.cullType = RenderCamera::CameraCullType::CAMERA_CULL_VIEW_FRUSTUM;
    reflCam.environment = camera.environment;
    reflCam.postProcessName = DefaultMaterialCameraConstants::CAMERA_REFLECTION_POST_PROCESS_PREFIX_NAME;
    dsCamera_->AddCamera(reflCam);

    return true;
}

bool RenderSystem::ProcessReflections(Entity cameraEntity, const RenderScene& renderScene)
{
    reflectionsQuery_.Execute();

    const auto queryResults = reflectionsQuery_.GetResults();
    const size_t cameraCount = dsCamera_->GetCameraCount();
#if (CORE3D_VALIDATION_ENABLED == 1)
    if ((!queryResults.empty()) && (!EntityUtil::IsValid(cameraEntity))) {
        CORE_LOG_ONCE_W("3d_validation_refl_main_cam", "CORE3D_VALIDATION: reflections supported only for main camera");
    }
#endif
    if ((!EntityUtil::IsValid(cameraEntity)) || queryResults.empty() || (renderScene.cameraIndex >= cameraCount)) {
        return false;
    }

    // copy needed (a single camera is added to dataStoreCamera_ in every loop)
    const auto camera = dsCamera_->GetCameras()[renderScene.cameraIndex];
    bool hasReflections = false;
    for (const auto& row : queryResults) {
        if (ProcessReflection(row, camera)) {
            hasReflections = true;
        }
    }

    // The flag does not have any purpose ATM, remove?
    return hasReflections;
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
    light.lightUsageFlags |= RenderLight::LightUsageFlagBits::LIGHT_USAGE_SHADOW_LIGHT_BIT;
    light.shadowCameraIndex = static_cast<uint32_t>(dsCamera_->GetCameraCount());

    float zNear = 0.0f;
    float zFar = 0.0f;
    RenderCamera camera;
    camera.id = SHADOW_CAMERA_START_UNIQUE_ID + light.shadowCameraIndex; // id used for easier uniqueness
    camera.shadowId = lpd.entity.id;
    camera.layerMask = lpd.lightComponent.shadowLayerMask; // we respect light shadow rendering mask
    if (light.lightUsageFlags & RenderLight::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT) {
        // NOTE: modifies the light camera to follow center of scene
        // Add slight bias offset to radius.
        const float radius =
            lpd.renderScene.worldSceneBoundingSphereRadius + lpd.renderScene.worldSceneBoundingSphereRadius * 0.05f;
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
    if (properties_.dataStoreManager) {
        IRenderDataStoreManager& rdsMgr = *properties_.dataStoreManager;
        auto* dataStore = static_cast<IRenderDataStorePod*>(rdsMgr.GetRenderDataStore(POD_DATA_STORE_NAME));
        if (!dataStore) {
            return;
        }

        const auto postProcessCount = postProcessMgr_->GetComponentCount();
        for (IComponentManager::ComponentId id = 0; id < postProcessCount; ++id) {
            if (const auto handle = postProcessMgr_->Read(id); handle) {
                const auto& pp = *handle;

                // just copy values (no support for fog control)
                PostProcessConfiguration ppConfig;
                ppConfig.enableFlags = pp.enableFlags;
                ppConfig.tonemapConfiguration = pp.tonemapConfiguration;
                ppConfig.bloomConfiguration = pp.bloomConfiguration;
                ppConfig.vignetteConfiguration = pp.vignetteConfiguration;
                ppConfig.colorFringeConfiguration = pp.colorFringeConfiguration;
                ppConfig.ditherConfiguration = pp.ditherConfiguration;
                ppConfig.blurConfiguration = pp.blurConfiguration;
                ppConfig.colorConversionConfiguration = pp.colorConversionConfiguration;
                ppConfig.tonemapConfiguration = pp.tonemapConfiguration;
                ppConfig.fxaaConfiguration = pp.fxaaConfiguration;

                const Entity ppEntity = postProcessMgr_->GetEntity(id);
                const auto ppName = GetPostProcessName(postProcessMgr_, ppEntity);
                auto const dataView = dataStore->Get(ppName);
                if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
                    dataStore->Set(ppName, arrayviewU8(ppConfig));
                } else {
                    renderProcessing_.postProcessPods.emplace_back(ppName);
                    dataStore->CreatePod(POST_PROCESS_NAME, ppName, arrayviewU8(ppConfig));
                }
            }
        }
    }
}

void RenderSystem::DestroyRenderDataStores()
{
    if (properties_.dataStoreManager) {
        IRenderDataStoreManager& rdsMgr = *properties_.dataStoreManager;
        if (auto* dataStore = static_cast<IRenderDataStorePod*>(rdsMgr.GetRenderDataStore(POD_DATA_STORE_NAME));
            dataStore) {
            for (const auto& ref : renderProcessing_.postProcessPods) {
                dataStore->DestroyPod(POST_PROCESS_NAME, ref.c_str());
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
    if (renderConfig.customRenderNodeGraph) {
        renderDataScene.customRenderNodeGraph =
            gpuHandleMgr_->GetRenderHandleReference(renderConfig.customRenderNodeGraph);
    }
    renderDataScene.customRenderNodeGraphFile = renderConfig.customRenderNodeGraphFile;
    renderDataScene.name = properties_.dataStoreScene;
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

    ProcessCameras(renderConfig, cameraEntity, renderDataScene);
    ProcessReflections(cameraEntity, renderDataScene);
    ProcessPostProcesses();

    SceneBoundingVolumeHelper sceneBoundingVolumeHelper; // does not try to be exact atm

    // Process all render components.
    ProcessRenderables(sceneBoundingVolumeHelper);

    // Process render node graphs automatically based on camera if needed bits set for properties
    // Some materials might request color pre-pass etc. (needs to be done after renderables are processed)
    ProcessRenderNodeGraphs(renderConfig, renderDataScene);

    // NOTE: move world sphere calculation to own system
    CalculateSceneBounds(sceneBoundingVolumeHelper);

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
    renderProcessing_.orderedRenderNodeGraphs.clear();
    const bool createRngs =
        (renderConfig.renderingFlags & RenderConfigurationComponent::SceneRenderingFlagBits::CREATE_RNGS_BIT);
    if (createRngs && graphicsContext_ && renderUtil_) {
        // first create scene render node graph if needed
        // we need to have scene render node graph as a separate
        renderProcessing_.orderedRenderNodeGraphs.emplace_back(GetSceneRenderNodeGraph(renderScene));

        // NOTE: should combine specific color pre pass for every render camera
        RenderHandleReference mainCamRng;
        RenderHandleReference mainColorPrePassHandle;
        const auto& renderCameras = dsCamera_->GetCameras();
        // then process all needed active cameras
        for (uint32_t camIdx = 0; camIdx < renderCameras.size(); ++camIdx) {
            const auto& camRef = renderCameras[camIdx];
            // main camera or shadow cameras are not interesting to us here
            if (camRef.flags & RenderCamera::CAMERA_FLAG_SHADOW_BIT) {
                continue;
            }
            CORE_ASSERT(camRef.id != 0xFFFFFFFFffffffff); // there must be an id for uniqueness
            const RenderHandleReference handle = GetCameraRenderNodeGraph(renderScene, camRef);
            if (camIdx == renderScene.cameraIndex) {
                mainCamRng = handle;
            } else {
                // we do not push pre pass camera to rendering here
                if (camRef.flags & RenderCamera::CAMERA_FLAG_COLOR_PRE_PASS_BIT) {
                    mainColorPrePassHandle =
                        (renderProcessing_.frameFlags & NEEDS_COLOR_PRE_PASS) ? handle : RenderHandleReference {};
                } else {
                    // NOTE: temporary insert for "pre-pass" cameras (e.g. reflection)
                    // +1 for scene index
                    if (camRef.flags & RenderCamera::CAMERA_FLAG_REFLECTION_BIT) {
                        renderProcessing_.orderedRenderNodeGraphs.insert(
                            renderProcessing_.orderedRenderNodeGraphs.begin() + 1u, handle);
                    } else {
                        renderProcessing_.orderedRenderNodeGraphs.emplace_back(handle);
                    }
                }
            }
        }

        // then main camera as the final camera
        if (renderScene.cameraIndex < renderCameras.size()) {
            if (mainColorPrePassHandle) {
                renderProcessing_.orderedRenderNodeGraphs.emplace_back(mainColorPrePassHandle);
            }
            if (mainCamRng) {
                renderProcessing_.orderedRenderNodeGraphs.emplace_back(mainCamRng);
            }
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

RenderHandleReference RenderSystem::GetCameraRenderNodeGraph(
    const RenderScene& renderScene, const RenderCamera& renderCamera)
{
    constexpr uint32_t rngChangeFlags = RenderCamera::CAMERA_FLAG_MSAA_BIT;
    auto createNewRng = [](auto& rngm, const auto& rnUtil, const auto& scene, const auto& obj) {
        const RenderNodeGraphDesc desc = rnUtil->GetRenderNodeGraphDesc(scene, obj, 0);
        return rngm.Create(
            IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, desc, {}, scene.name);
    };

    IRenderNodeGraphManager& rngm = renderContext_->GetRenderNodeGraphManager();
    RenderHandleReference handle = renderCamera.customRenderNodeGraph;
    if (!handle) {
        if (auto iter = renderProcessing_.camIdToRng.find(renderCamera.id);
            iter != renderProcessing_.camIdToRng.cend()) {
            // NOTE: not optimal, currently re-creates a render node graph if:
            // * msaa flags have changed
            // * post process name / component has changed
            // * pipeline has changed
            const bool reCreate = ((iter->second.flags & rngChangeFlags) != (renderCamera.flags & rngChangeFlags)) ||
                                  (iter->second.postProcessName != renderCamera.postProcessName) ||
                                  (iter->second.renderPipelineType != renderCamera.renderPipelineType);
            if (reCreate) {
                iter->second.handle = {};
                iter->second.handle = createNewRng(rngm, renderUtil_, renderScene, renderCamera);
            }
            iter->second.lastFrameIndex = frameIndex_;
            iter->second.flags = renderCamera.flags;
            iter->second.renderPipelineType = renderCamera.renderPipelineType;
            iter->second.postProcessName = renderCamera.postProcessName;
            handle = iter->second.handle;
        } else {
            auto newRng = createNewRng(rngm, renderUtil_, renderScene, renderCamera);
            handle = newRng;
            renderProcessing_.camIdToRng[renderCamera.id] = { move(newRng), renderCamera.flags,
                renderCamera.renderPipelineType, frameIndex_,
                !(renderCamera.flags & RenderCamera::CAMERA_FLAG_COLOR_PRE_PASS_BIT), renderCamera.postProcessName };
        }
    }
    return handle;
}

RenderHandleReference RenderSystem::GetSceneRenderNodeGraph(const RenderScene& renderScene)
{
    RenderHandleReference handle = renderScene.customRenderNodeGraph;
    if (!handle) {
        handle = renderProcessing_.sceneRng;
    }
    if (!handle) {
        IRenderNodeGraphManager& rngm = renderContext_->GetRenderNodeGraphManager();
        auto createNewRng = [](auto& rngm, const auto& rnUtil, const auto& scene) {
            const RenderNodeGraphDesc desc = rnUtil->GetRenderNodeGraphDesc(scene, 0);
            return rngm.Create(
                IRenderNodeGraphManager::RenderNodeGraphUsageType::RENDER_NODE_GRAPH_STATIC, desc, {}, scene.name);
        };

        renderProcessing_.sceneRng = createNewRng(rngm, renderUtil_, renderScene);
        handle = renderProcessing_.sceneRng;
    }
    return handle;
}

array_view<const RenderHandleReference> RenderSystem::GetRenderNodeGraphs() const
{
    return renderProcessing_.orderedRenderNodeGraphs;
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

ISystem* IRenderSystemInstance(IEcs& ecs)
{
    return new RenderSystem(ecs);
}

void IRenderSystemDestroy(ISystem* instance)
{
    delete static_cast<RenderSystem*>(instance);
}
CORE3D_END_NAMESPACE()
