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

#include "render_node_default_camera_controller.h"

#if (CORE3D_VALIDATION_ENABLED == 1)
#include <cinttypes>
#include <string>
#endif

#include <3d/implementation_uids.h>
#include <3d/intf_graphics_context.h>
#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/shaders/common/3d_dm_structures_common.h>
#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/math/float_packer.h>
#include <base/math/mathf.h>
#include <base/math/matrix_util.h>
#include <base/math/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_class_register.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/datastore/intf_render_data_store_pod.h>
#include <render/datastore/render_data_store_render_pods.h>
#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/intf_shader_manager.h>
#include <render/intf_render_context.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_pipeline_descriptor_set_binder.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>
#include <render/nodecontext/intf_render_node_parser_util.h>
#include <render/nodecontext/intf_render_node_util.h>
#include <render/render_data_structures.h>

#include "render/datastore/render_data_store_weather.h"
// NOTE: do not include in header
#include "render_light_helper.h"

namespace {
#include <render/shaders/common/render_post_process_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
constexpr bool USE_IMMUTABLE_SAMPLERS { false };
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
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (camera.id == RenderSceneDataConstants::INVALID_ID) {
        CORE_LOG_ONCE_I("valid_r_c_id" + to_string(camera.id), "Invalid camera id (cam id %" PRIu64 ")", camera.id);
    }
#endif
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

constexpr GpuImageDesc OUTPUT_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_2D, BASE_FORMAT_R8G8B8A8_SRGB,
    CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0U, CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 1U, 1U, 1U, 1U, 1U,
    CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };
constexpr GpuImageDesc COLOR_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_2D,
    BASE_FORMAT_R16G16B16A16_SFLOAT, CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0U, CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 1U, 1U, 1U, 1U, 1U,
    CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };
constexpr GpuImageDesc VELOCITY_NORMAL_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_2D,
    BASE_FORMAT_R16G16B16A16_SFLOAT, CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, 0U,
    CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 1U, 1U, 1U, 1U, 1U, CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };
constexpr GpuImageDesc HISTORY_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_2D,
    BASE_FORMAT_B10G11R11_UFLOAT_PACK32, CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | CORE_IMAGE_USAGE_SAMPLED_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 0U, CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 1U, 1U, 1U, 1U, 1U,
    CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };
constexpr GpuImageDesc BASE_COLOR_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_2D, BASE_FORMAT_R8G8B8A8_SRGB,
    CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
        CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, 0U,
    CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 1U, 1U, 1U, 1U, 1U, CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };
constexpr GpuImageDesc MATERIAL_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_2D, BASE_FORMAT_R8G8B8A8_UNORM,
    CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
        CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, 0U,
    CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 1U, 1U, 1U, 1U, 1U, CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };

constexpr GpuImageDesc DEPTH_DEFAULT_DESC { CORE_IMAGE_TYPE_2D, CORE_IMAGE_VIEW_TYPE_2D, BASE_FORMAT_D32_SFLOAT,
    CORE_IMAGE_TILING_OPTIMAL,
    CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
        CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT, 0U,
    CORE_ENGINE_IMAGE_CREATION_DYNAMIC_BARRIERS, 1U, 1U, 1U, 1U, 1U, CORE_SAMPLE_COUNT_1_BIT, ComponentMapping {} };

RenderNodeDefaultCameraController::ShadowBuffers GetShadowBufferNodeData(
    IRenderNodeGpuResourceManager& gpuResourceMgr, const string_view sceneName)
{
    RenderNodeDefaultCameraController::ShadowBuffers sb;
    sb.vsmSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_VSM_SHADOW_SAMPLER);
    sb.pcfSamplerHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_PCF_SHADOW_SAMPLER);

    sb.depthHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_DEPTH_BUFFER_NAME);
    sb.vsmColorHandle =
        gpuResourceMgr.GetImageHandle(sceneName + DefaultMaterialLightingConstants::SHADOW_VSM_COLOR_BUFFER_NAME);
    if (!RenderHandleUtil::IsValid(sb.depthHandle)) {
        sb.depthHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE_WHITE");
    }
    if (!RenderHandleUtil::IsValid(sb.vsmColorHandle)) {
        sb.vsmColorHandle = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");
    }

    return sb;
}

void ValidateColorDesc(const IRenderNodeGpuResourceManager& gpuResourceMgr, const GpuImageDesc& input,
    const bool bilinearSampling, GpuImageDesc& desc)
{
    desc.imageType = input.imageType;
    desc.imageViewType = input.imageViewType;
    desc.imageTiling = input.imageTiling;
    if (desc.format == BASE_FORMAT_UNDEFINED) {
        desc.format = input.format;
    }
    if (desc.usageFlags == 0) {
        desc.usageFlags = input.usageFlags;
    }
    if (desc.memoryPropertyFlags == 0) {
        desc.memoryPropertyFlags = input.memoryPropertyFlags;
    }
    if (desc.createFlags == 0) {
        desc.createFlags = input.createFlags;
    }
    if (desc.engineCreationFlags == 0) {
        desc.engineCreationFlags = input.engineCreationFlags;
    }
    desc.width = Math::max(desc.width, input.width);
    desc.height = Math::max(desc.height, input.height);
    desc.depth = Math::max(desc.depth, input.depth);

    desc.mipCount = Math::max(desc.mipCount, input.mipCount);
    desc.layerCount = Math::max(desc.layerCount, input.layerCount);

    if ((desc.layerCount == 1U) && (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_2D_ARRAY)) {
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    } else if ((desc.layerCount > 1U) && (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_2D)) {
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
    if (desc.sampleCountFlags == 0) {
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
    }

    if (bilinearSampling) {
        desc.format = GetValidColorFormat(gpuResourceMgr, desc.format);
    }
}

ImageViewType GetImageViewType(const uint32_t layerCount, const ImageViewType imageViewType)
{
    if ((layerCount == 1U) && (imageViewType == CORE_IMAGE_VIEW_TYPE_2D_ARRAY)) {
        return CORE_IMAGE_VIEW_TYPE_2D;
    } else if ((layerCount > 1U) && (imageViewType == CORE_IMAGE_VIEW_TYPE_2D)) {
        return CORE_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
    return imageViewType;
}

void ValidateDepthDesc(
    const IRenderNodeGpuResourceManager& gpuResourceMgr, const GpuImageDesc& input, GpuImageDesc& desc)
{
    desc.imageType = input.imageType;
    desc.imageViewType = input.imageViewType;
    desc.imageTiling = input.imageTiling;
    if (desc.format == BASE_FORMAT_UNDEFINED) {
        desc.format = input.format;
    }
    if (desc.usageFlags == 0) {
        desc.usageFlags = input.usageFlags;
    }
    if (desc.memoryPropertyFlags == 0) {
        desc.memoryPropertyFlags = input.memoryPropertyFlags;
    }
    if (desc.createFlags == 0) {
        desc.createFlags = input.createFlags;
    }
    if (desc.engineCreationFlags == 0) {
        desc.engineCreationFlags = input.engineCreationFlags;
    }
    desc.width = Math::max(desc.width, input.width);
    desc.height = Math::max(desc.height, input.height);
    desc.depth = Math::max(desc.depth, input.depth);

    desc.mipCount = Math::max(desc.mipCount, input.mipCount);
    desc.layerCount = Math::max(desc.layerCount, input.layerCount);

    if ((desc.layerCount == 1U) && (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_2D_ARRAY)) {
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D;
    } else if ((desc.layerCount > 1U) && (desc.imageViewType == CORE_IMAGE_VIEW_TYPE_2D)) {
        desc.imageViewType = CORE_IMAGE_VIEW_TYPE_2D_ARRAY;
    }
    if (desc.sampleCountFlags == 0) {
        desc.sampleCountFlags = CORE_SAMPLE_COUNT_1_BIT;
    }

    desc.format = GetValidDepthFormat(gpuResourceMgr, desc.format);
}

struct CreatedTargetHandles {
    RenderHandle color;
    RenderHandle depth;
    RenderHandle colorResolve;
    RenderHandle colorResolveUpscaled;
    RenderHandle colorMsaa;
    RenderHandle depthMsaa;
    RenderHandle baseColor;
    RenderHandle velNor;
    RenderHandle material;
};

CreatedTargetHandles FillCreatedTargets(const RenderNodeDefaultCameraController::CreatedTargets& targets)
{
    return CreatedTargetHandles {
        targets.outputColor.GetHandle(),
        targets.depth.GetHandle(),
        targets.colorResolve.GetHandle(),
        targets.colorResolveUpscaled.GetHandle(),
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

void CreateBaseColorTarget(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const string_view us, const string_view customCamRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& targets)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    CORE_LOG_I("CORE3D_VALIDATION: creating camera base color target %s", customCamRngId.data());
#endif
    GpuImageDesc desc = cameraResourceSetup.inputImageDescs.output;
    desc.width = camera.renderResolution.x;
    desc.height = camera.renderResolution.y;
    desc.imageViewType = GetImageViewType(desc.layerCount, desc.imageViewType);
    targets.outputColor =
        gpuResourceMgr.Create(us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + customCamRngId, desc);
    targets.imageDescs.output = desc;
}

void CreateVelocityTarget(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view us, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
    const bool createVelocity = (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) ||
                                (camera.renderPipelineType == RenderCamera::RenderPipelineType::FORWARD);
    if (createVelocity) {
        GpuImageDesc desc = cameraResourceSetup.inputImageDescs.velocityNormal;
        desc.width = targetDesc.width;
        desc.height = targetDesc.height;
        desc.layerCount = targetDesc.layerCount;
        desc.imageViewType = GetImageViewType(desc.layerCount, desc.imageViewType);
        // patch even RNG given flags
        if (camera.flags & RenderCamera::CAMERA_FLAG_OUTPUT_VELOCITY_NORMAL_BIT) {
            desc.usageFlags |= CORE_IMAGE_USAGE_SAMPLED_BIT;
            // we cannot have transient and lazy allocation
            desc.usageFlags &= (~CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
            desc.memoryPropertyFlags &= (~CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
        }
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.velocityNormal = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "VEL_NOR" + customCameraRngId, desc);
#else
        createdTargets.velocityNormal = gpuResourceMgr.Create(createdTargets.velocityNormal, desc);
#endif
        // store
        createdTargets.imageDescs.velocityNormal = desc;
    }
}

void CreateDeferredTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view us, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
    if (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
        GpuImageDesc desc = cameraResourceSetup.inputImageDescs.baseColor;
        desc.width = targetDesc.width;
        desc.height = targetDesc.height;
        desc.layerCount = targetDesc.layerCount;
        desc.imageViewType = GetImageViewType(desc.layerCount, desc.imageViewType);
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.baseColor = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "BC_" + customCameraRngId, desc);
#else
        createdTargets.baseColor = gpuResourceMgr.Create(createdTargets.baseColor, desc);
#endif
        // store
        createdTargets.imageDescs.baseColor = desc;

        // create material
        desc = cameraResourceSetup.inputImageDescs.material;
        desc.width = targetDesc.width;
        desc.height = targetDesc.height;
        desc.layerCount = targetDesc.layerCount;
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.material = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "MAT_" + customCameraRngId, desc);
#else
        createdTargets.material = gpuResourceMgr.Create(createdTargets.material, desc);
#endif
        // store
        createdTargets.imageDescs.material = desc;
    }
}

void CreateHistoryTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view us, const string_view customCameraRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
    if (camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_HISTORY_BIT) {
        GpuImageDesc desc = cameraResourceSetup.inputImageDescs.history;
        desc.width = targetDesc.width;
        desc.height = targetDesc.height;
        desc.layerCount = targetDesc.layerCount;
        desc.imageViewType = GetImageViewType(desc.layerCount, desc.imageViewType);
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.history[0u] = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "HIS0_" + customCameraRngId, desc);
        createdTargets.history[1u] = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "HIS1_" + customCameraRngId, desc);
#else
        createdTargets.history[0u] = gpuResourceMgr.Create(createdTargets.history[0u], desc);
        createdTargets.history[1u] = gpuResourceMgr.Create(createdTargets.history[1u], desc);
#endif

        // store
        createdTargets.imageDescs.history = desc;
    }
}

void CreateColorTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view us, const string_view customCamRngId,
    const RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets, float upscaleRatio)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    CORE_LOG_D("CORE3D_VALIDATION: creating camera color targets %s", customCamRngId.data());
#endif
    // This (re-)creates all the needed color targets
    GpuImageDesc desc = cameraResourceSetup.inputImageDescs.color;
    desc.width = targetDesc.width;
    desc.height = targetDesc.height;
    desc.layerCount = targetDesc.layerCount;
    desc.imageViewType = GetImageViewType(desc.layerCount, desc.imageViewType);
    if (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) {
        const bool directBackbufferResolve =
            camera.renderPipelineType == RenderCamera::RenderPipelineType::LIGHT_FORWARD;

        GpuImageDesc msaaDesc = desc;
        if (directBackbufferResolve) {
            msaaDesc.format = targetDesc.format;
        }
        msaaDesc.engineCreationFlags |= CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
        msaaDesc.sampleCountFlags = camera.msaaSampleCountFlags;
        msaaDesc.usageFlags = CORE_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        msaaDesc.memoryPropertyFlags =
            CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;

#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.colorMsaa = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "MSAA_" + customCamRngId, msaaDesc);
#else
        createdTargets.colorMsaa = gpuResourceMgr.Create(createdTargets.colorMsaa, msaaDesc);
#endif
    }

    if (camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.colorResolve = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_COLOR_PREFIX_NAME + "RESL_" + customCamRngId, desc);
#else
        createdTargets.colorResolve = gpuResourceMgr.Create(createdTargets.colorResolve, desc);
#endif
        if (upscaleRatio > 1.0f) {
            desc.width = static_cast<uint32_t>(static_cast<float>(desc.width) * upscaleRatio);
            desc.height = static_cast<uint32_t>(static_cast<float>(desc.height) * upscaleRatio);
#if (CORE3D_VALIDATION_ENABLED == 1)
            createdTargets.colorResolveUpscaled = gpuResourceMgr.Create(
                us + DefaultMaterialCameraConstants::CAMERA_COLOR_UPSCALED_PREFIX_NAME + "RESL_" + customCamRngId,
                desc);
#else
            createdTargets.colorResolveUpscaled = gpuResourceMgr.Create(createdTargets.colorResolveUpscaled, desc);
#endif
        }
    }

    CreateVelocityTarget(gpuResourceMgr, camera, targetDesc, us, customCamRngId, cameraResourceSetup, createdTargets);
    CreateDeferredTargets(gpuResourceMgr, camera, targetDesc, us, customCamRngId, cameraResourceSetup, createdTargets);

    CreateHistoryTargets(gpuResourceMgr, camera, targetDesc, us, customCamRngId, cameraResourceSetup, createdTargets);
}

void CreateDepthTargets(IRenderNodeGpuResourceManager& gpuResourceMgr, const RenderCamera& camera,
    const GpuImageDesc& targetDesc, const string_view us, const string_view customCamRngId,
    RenderNodeDefaultCameraController::CameraResourceSetup& cameraResourceSetup,
    RenderNodeDefaultCameraController::CreatedTargets& createdTargets)
{
#if (CORE3D_VALIDATION_ENABLED == 1)
    CORE_LOG_D("CORE3D_VALIDATION: creating camera depth targets %s", customCamRngId.data());
#endif
    // this (re-)creates all the needed depth targets
    // we support cameras without depth targets (default depth is created if no msaa)
    GpuImageDesc desc = cameraResourceSetup.inputImageDescs.depth;
    desc.layerCount = targetDesc.layerCount;
    desc.width = targetDesc.width;
    desc.height = targetDesc.height;
    desc.imageViewType = GetImageViewType(desc.layerCount, desc.imageViewType);
    if (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) {
        GpuImageDesc msaaDesc = desc;
        msaaDesc.engineCreationFlags |= CORE_ENGINE_IMAGE_CREATION_RESET_STATE_ON_FRAME_BORDERS;
        msaaDesc.sampleCountFlags = camera.msaaSampleCountFlags;
        // If MSAA targets have input attachment bit they are not created as renderbuffers and
        // EXT_multisample_render_to_texture supports only depth with renderbuffers.
        msaaDesc.usageFlags = CORE_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
        msaaDesc.memoryPropertyFlags =
            CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
#if (CORE3D_VALIDATION_ENABLED == 1)
        createdTargets.depthMsaa = gpuResourceMgr.Create(
            us + DefaultMaterialCameraConstants::CAMERA_DEPTH_PREFIX_NAME + "MSAA_" + customCamRngId, msaaDesc);
#else
        createdTargets.depthMsaa = gpuResourceMgr.Create(createdTargets.depthMsaa, msaaDesc);
#endif
    }
    const bool createBasicDepth = (((camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) == 0) ||
                                      ((camera.flags & RenderCamera::CAMERA_FLAG_OUTPUT_DEPTH_BIT) != 0)) &&
                                  (!RenderHandleUtil::IsValid(camera.depthTarget.GetHandle()));
    if (createBasicDepth) {
        if (camera.flags & RenderCamera::CAMERA_FLAG_OUTPUT_DEPTH_BIT) {
            desc.usageFlags |= CORE_IMAGE_USAGE_SAMPLED_BIT;
            // we cannot have transient and lazy allocation
            desc.usageFlags &= (~CORE_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT);
            desc.memoryPropertyFlags &= (~CORE_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
        }
        createdTargets.depth =
            gpuResourceMgr.Create(us + DefaultMaterialCameraConstants::CAMERA_DEPTH_PREFIX_NAME + customCamRngId, desc);
    }

    // store
    createdTargets.imageDescs.depth = desc;
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
    const bool resChanged = ((desc.width != camRes.outResolution.x) || (desc.height != camRes.outResolution.y));
    // NOTE: currently compares only the output not the color i.e. the intermediate
    const bool formatChanged = (!RenderHandleUtil::IsValid(camRes.colorTarget))
                                   ? (desc.format != camRes.inputImageDescs.output.format)
                                   : false;
    const bool multiviewChanged = (camera.multiViewCameraCount > 0U) != camRes.isMultiview;
    const bool msaaCountChanged =
        (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) && (camera.msaaSampleCountFlags != camRes.sampleCountFlags);

    bool changed = false;
    if (creationFlagsChanged || pipelineChanged || resChanged || formatChanged || multiviewChanged ||
        msaaCountChanged) {
        changed = true;
    }
    return changed;
}

bool DepthTargetsRecreationNeeded(const RenderCamera& camera,
    const RenderNodeDefaultCameraController::CameraResourceSetup& camRes, const GpuImageDesc& desc)
{
    constexpr RenderCamera::Flags importantFlags { RenderCamera::CAMERA_FLAG_OUTPUT_DEPTH_BIT };
    const bool formatChanged =
        (!RenderHandleUtil::IsValid(camRes.depthTarget)) ? (desc.format != camRes.inputImageDescs.depth.format) : false;
    const bool resChanged = ((desc.width != camRes.outResolution.x) || (desc.height != camRes.outResolution.y));
    const RenderCamera::Flags newFlags = camera.flags & importantFlags;
    const RenderCamera::Flags oldFlags = camRes.camFlags & importantFlags;
    const bool creationFlagsChanged = newFlags != oldFlags;
    const bool multiviewChanged = (camera.multiViewCameraCount > 0U) != camRes.isMultiview;
    const bool msaaCountChanged =
        (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) && (camera.msaaSampleCountFlags != camRes.sampleCountFlags);

    bool changed = false;
    if (formatChanged || resChanged || creationFlagsChanged || multiviewChanged || msaaCountChanged) {
        changed = true;
    }
    return changed;
}
} // namespace

void RenderNodeDefaultCameraController::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    // get flags for rt
    {
        rtEnabled_ = false;
        IRenderContext& renderContext = renderNodeContextMgr_->GetRenderContext();
        if (auto* renderContextClassRegister = renderContext.GetInterface<CORE_NS::IClassRegister>()) {
            if (auto* graphicsContext =
                    CORE_NS::GetInstance<IGraphicsContext>(*renderContextClassRegister, UID_GRAPHICS_CONTEXT)) {
                const IGraphicsContext::CreateInfo ci = graphicsContext->GetCreateInfo();
                if (ci.createFlags & IGraphicsContext::CreateInfo::ENABLE_ACCELERATION_STRUCTURES_BIT) {
                    rtEnabled_ = true;
                }
            }
        }
    }

    SetDefaultGpuImageDescs();
    ParseRenderNodeInputs();

    globalDescs_ = {};
    if constexpr (RenderLightHelper::ENABLE_CLUSTERED_LIGHTING) {
        // load the light clustering shader
        if (renderNodeContextMgr_) {
            auto& shaderMgr = renderNodeContextMgr_->GetShaderManager();

            clusterBinders_ = {};
            clusterBinders_.shaderHandleCluster =
                shaderMgr.GetShaderHandle("3dshaders://computeshader/core3d_cluster_lights.shader");
            clusterBinders_.pipelineLayout = shaderMgr.GetReflectionPipelineLayout(clusterBinders_.shaderHandleCluster);

            clusterBinders_.pl = shaderMgr.GetPipelineLayout(
                shaderMgr.GetPipelineLayoutHandle("3dpipelinelayouts://core3d_cluster_lights.shaderpl"));

            auto& psoMgr = renderNodeContextMgr.GetPsoManager();
            clusterBinders_.psoHandle =
                psoMgr.GetComputePsoHandle(clusterBinders_.shaderHandleCluster, clusterBinders_.pipelineLayout, {});
        }
    }

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
    dsWeatherName_ = RenderNodeSceneUtil::GetSceneRenderDataStore(stores_, RenderDataStoreWeather::TYPE_NAME);

    currentScene_ = {};
    currentScene_.customCamRngName = jsonInputs_.customCameraName + '_' + to_hex(jsonInputs_.customCameraId);

    // id checked first for custom camera
    if (jsonInputs_.customCameraId != INVALID_CAM_ID) {
        currentScene_.customCameraId = jsonInputs_.customCameraId;
    } else if (!jsonInputs_.customCameraName.empty()) {
        currentScene_.customCameraName = jsonInputs_.customCameraName;
    }

    camRes_.backendType = renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderBackend;
    defaultCubemap_ = renderNodeContextMgr_->GetGpuResourceManager().GetImageHandle(
        DefaultMaterialGpuResourceConstants::CORE_DEFAULT_SKYBOX_CUBEMAP);

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    defaultSamplers_.cubemapHandle =
        gpuResourceMgr.GetSamplerHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP_SAMPLER);
    defaultSamplers_.linearHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_CLAMP");
    defaultSamplers_.nearestHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_NEAREST_CLAMP");
    defaultSamplers_.linearMipHandle = gpuResourceMgr.GetSamplerHandle("CORE_DEFAULT_SAMPLER_LINEAR_MIPMAP_CLAMP");
    defaultColorPrePassHandle_ = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_GPU_IMAGE");

    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    if (dataStoreScene && dataStoreCamera && dataStoreLight) {
        UpdateCurrentScene(*dataStoreScene, *dataStoreCamera, *dataStoreLight);
        prevCameraResolution_ = { currentScene_.camera.renderResolution };
        CreateResources();
        CreateBuffers();

        // get the camera buffers
        uboHandles_.cameraData = renderNodeContextMgr_->GetGpuResourceManager().GetBufferHandle(
            dataStoreScene->GetName() + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);
        if (rtEnabled_) {
            uboHandles_.tlas = renderNodeContextMgr_->GetGpuResourceManager().GetBufferHandle(
                dataStoreScene->GetName() + DefaultMaterialMaterialConstants::MATERIAL_TLAS_PREFIX_NAME);
        }
        RegisterOutputs();
    }
    shadowBuffers_ = GetShadowBufferNodeData(gpuResourceMgr, stores_.dataStoreNameScene);
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

    if constexpr (RenderLightHelper::ENABLE_CLUSTERED_LIGHTING) {
        if (!clusterBinders_.clusterBuffersSet0) {
            auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();

            const DescriptorCounts dc =
                renderNodeContextMgr_->GetRenderNodeUtil().GetDescriptorCounts(clusterBinders_.pl);
            descriptorSetMgr.ResetAndReserve(dc);

            const RenderHandle setDescHandle = descriptorSetMgr.CreateDescriptorSet(0u, clusterBinders_.pl);
            clusterBinders_.clusterBuffersSet0 = descriptorSetMgr.CreateDescriptorSetBinder(
                setDescHandle, clusterBinders_.pl.descriptorSetLayouts[0u].bindings);
        }
    }
    if (!globalDescs_.dmSet0Binder) {
        auto& descriptorSetMgr = renderNodeContextMgr_->GetDescriptorSetManager();
        const IRenderNodeShaderManager& shaderMgr = renderNodeContextMgr_->GetShaderManager();
        const IShaderManager::RenderSlotData shaderRsd = shaderMgr.GetRenderSlotData(
            shaderMgr.GetRenderSlotId(DefaultMaterialShaderConstants::RENDER_SLOT_FORWARD_OPAQUE));
        const PipelineLayout pl = shaderMgr.GetPipelineLayout(shaderRsd.pipelineLayout.GetHandle());

        const string_view us = stores_.dataStoreNameScene;
        string camName;
        if (currentScene_.customCameraId != INVALID_CAM_ID) {
            camName = to_string(currentScene_.customCameraId);
        } else if (!(currentScene_.customCameraName.empty())) {
            camName = currentScene_.customCameraName;
        }
        globalDescs_ = {};
        const auto& bindings = pl.descriptorSetLayouts[0U].bindings;
        globalDescs_.dmSet0 = descriptorSetMgr.CreateGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_SET0_GLOBAL_DESCRIPTOR_SET_PREFIX_NAME + camName, bindings);
        globalDescs_.dmSet0Binder =
            descriptorSetMgr.CreateDescriptorSetBinder(globalDescs_.dmSet0.GetHandle(), bindings);
    }
}

void RenderNodeDefaultCameraController::ExecuteFrame(IRenderCommandList& cmdList)
{
    UpdateBuffers();
    UpdateGlobalDescriptorSets(cmdList);
    if constexpr (RenderLightHelper::ENABLE_CLUSTERED_LIGHTING) {
        ClusterLights(cmdList);
    }
}

void RenderNodeDefaultCameraController::UpdateGlobalDescriptorSets(IRenderCommandList& cmdList)
{
    if (!globalDescs_.dmSet0Binder) {
        return;
    }

    auto& binder = *globalDescs_.dmSet0Binder;
    {
        uint32_t bindingIndex = 0;
        binder.BindBuffer(bindingIndex++, uboHandles_.cameraData, 0u);
        binder.BindBuffer(bindingIndex++, uboHandles_.generalData.GetHandle(), 0u);

        const RenderHandle radianceCubemap = currentScene_.cameraEnvRadianceHandle;
        const RenderHandle colorPrePass = RenderHandleUtil::IsValid(currentScene_.prePassColorTarget)
                                              ? currentScene_.prePassColorTarget
                                              : defaultColorPrePassHandle_;

        binder.BindBuffer(
            bindingIndex++, { uboHandles_.environment.GetHandle(), 0u, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE });
        binder.BindBuffer(
            bindingIndex++, { uboHandles_.fog.GetHandle(), 0u, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE });
        binder.BindBuffer(
            bindingIndex++, { uboHandles_.light.GetHandle(), 0u, PipelineStateConstants ::GPU_BUFFER_WHOLE_SIZE });
        binder.BindBuffer(
            bindingIndex++, { uboHandles_.postProcess.GetHandle(), 0u, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE });
        binder.BindBuffer(bindingIndex++,
            { uboHandles_.lightCluster.GetHandle(), 0u, PipelineStateConstants::GPU_BUFFER_WHOLE_SIZE });
        // use immutable samplers for all set 0 samplers
        AdditionalDescriptorFlags descFlags = 0U;
        if constexpr (USE_IMMUTABLE_SAMPLERS) {
            descFlags = CORE_ADDITIONAL_DESCRIPTOR_IMMUTABLE_SAMPLER_BIT;
        }
        BindableImage bi;
        bi.handle = colorPrePass;
        bi.samplerHandle = defaultSamplers_.linearMipHandle;
        binder.BindImage(bindingIndex++, bi, descFlags);
        bi.handle = shadowBuffers_.vsmColorHandle;
        bi.samplerHandle = shadowBuffers_.vsmSamplerHandle;
        binder.BindImage(bindingIndex++, bi, descFlags);
        bi.handle = shadowBuffers_.depthHandle;
        bi.samplerHandle = shadowBuffers_.pcfSamplerHandle;
        binder.BindImage(bindingIndex++, bi, descFlags);
        bi.handle = radianceCubemap;
        bi.samplerHandle = defaultSamplers_.cubemapHandle;
        binder.BindImage(bindingIndex++, bi, descFlags);

        // bind acceleration structure
        if (rtEnabled_) {
            binder.BindBuffer(bindingIndex++, uboHandles_.tlas, 0U);
        }
    }

    const RenderHandle handles[] { binder.GetDescriptorSetHandle() };
    const DescriptorSetLayoutBindingResources resources[] { binder.GetDescriptorSetLayoutBindingResources() };
    cmdList.UpdateDescriptorSets(handles, resources);
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

    const auto camHandles = RenderNodeSceneUtil::GetSceneCameraImageHandles(
        *renderNodeContextMgr_, stores_.dataStoreNameScene, currentScene_.camera.name, currentScene_.camera);
    currentScene_.cameraEnvRadianceHandle = camHandles.radianceCubemap;

    if (!currentScene_.camera.prePassColorTargetName.empty()) {
        currentScene_.prePassColorTarget =
            renderNodeContextMgr_->GetGpuResourceManager().GetImageHandle(currentScene_.camera.prePassColorTargetName);
    }

    currentScene_.cameraIdx = cameraIdx;
    currentScene_.sceneTimingData = { scene.sceneDeltaTime, scene.deltaTime, scene.totalTime,
        *reinterpret_cast<const float*>(&scene.frameIndex) };
    screenPercentage_ = Math::clamp(currentScene_.camera.screenPercentage, 0.25f, 1.0f);

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
    shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_OUTPUT, colorTarget);
    shrMgr.RegisterGlobalRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_OUTPUT, colorTarget);

    shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_BUFFER, uboHandles_.cameraData);
    shrMgr.RegisterGlobalRenderNodeOutput(
        DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_BUFFER, uboHandles_.cameraData);

    // color is output always, depth usually (not with MSAA if not forced)
    // colorResolve is used with msaa and hdr, the output is the 32 bit srgb color usually
    const RenderHandle regDepth = RenderHandleUtil::IsValid(cth.depth) ? cth.depth : depthTarget;
    const RenderHandle regColor = RenderHandleUtil::IsValid(cth.colorResolve) ? cth.colorResolve : colorTarget;

    const RenderHandle regColorUpscaled =
        (RenderHandleUtil::IsValid(cth.colorResolveUpscaled) &&
            currentScene_.camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD)
            ? cth.colorResolveUpscaled
            : colorTarget;

    shrMgr.RegisterRenderNodeOutput(
        DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_UPSCALED, regColorUpscaled);
    shrMgr.RegisterGlobalRenderNodeOutput(
        DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_UPSCALED, regColorUpscaled);

    if (RenderHandleUtil::IsValid(regDepth)) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH, regDepth);
        shrMgr.RegisterGlobalRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH, regDepth);
    }
    shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR, regColor);
    shrMgr.RegisterGlobalRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR, regColor);
    if (RenderHandleUtil::IsValid(cth.velNor)) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_VELOCITY_NORMAL, cth.velNor);
        shrMgr.RegisterGlobalRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_VELOCITY_NORMAL, cth.velNor);
    }
    // NOTE: HDR does not have float intermediate target ATM
    if (currentScene_.camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_BASE_COLOR, cth.baseColor);
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_MATERIAL, cth.material);
        shrMgr.RegisterGlobalRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_BASE_COLOR, cth.baseColor);
        shrMgr.RegisterGlobalRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_MATERIAL, cth.material);
    } else if (currentScene_.camera.flags & (RenderCamera::CAMERA_FLAG_MSAA_BIT)) {
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH_MSAA, cth.depthMsaa);
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_MSAA, cth.colorMsaa);
        shrMgr.RegisterGlobalRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH_MSAA, cth.depthMsaa);
        shrMgr.RegisterGlobalRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_MSAA, cth.colorMsaa);
    }

    // output history
    if (currentScene_.camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_HISTORY_BIT) {
        const uint32_t currIndex = camRes_.historyFlipFrame;
        const uint32_t nextIndex = (currIndex + 1u) % 2u;
        shrMgr.RegisterRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY, createdTargets_.history[currIndex].GetHandle());
        shrMgr.RegisterRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY_NEXT,
            createdTargets_.history[nextIndex].GetHandle());
        shrMgr.RegisterGlobalRenderNodeOutput(
            DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY, createdTargets_.history[currIndex].GetHandle());
        shrMgr.RegisterGlobalRenderNodeOutput(DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY_NEXT,
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
    bool validDepthHandle = RenderHandleUtil::IsValid(camRes_.depthTarget);
    const bool isHdr = (camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD);
    const bool isMsaa = (camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT);
    const bool isDeferred = (camera.renderPipelineType == RenderCamera::RenderPipelineType::DEFERRED);
    const bool isMultiview = (camera.multiViewCameraCount > 0U);
    const uint32_t mvLayerCount = isMultiview ? (camera.multiViewCameraCount + 1U) : 1U;
    if (validDepthHandle && isMultiview) {
        validDepthHandle = false;
#if (CORE3D_VALIDATION_ENABLED == 1)
        CORE_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "cam_controller_multiview_depth",
            "CORE3D_VALIDATION: Multi-view not supported with custom depth targets (creating new layered depth)");
#endif
    }
    if ((!validDepthHandle) || isMsaa || isHdr || isDeferred) {
        GpuImageDesc colorDesc = RenderHandleUtil::IsValid(camRes_.colorTarget)
                                     ? gpuResMgr.GetImageDescriptor(camRes_.colorTarget)
                                     : gpuResMgr.GetImageDescriptor(createdTargets_.outputColor.GetHandle());

        if (isMultiview) {
            colorDesc.layerCount = mvLayerCount;
        }

        const bool upscaleChanged = camera.renderResolution != prevCameraResolution_;
        const float upscaleRatio = 1.0f / screenPercentage_;
        prevCameraResolution_ = camera.renderResolution;
        const bool colorFormatChanged = prevColorFormat_ != camRes_.inputImageDescs.output.format;
        prevColorFormat_ = camRes_.inputImageDescs.output.format;

        const bool colorTargetChanged =
            ColorTargetsRecreationNeeded(camera, camRes_, colorDesc) || upscaleChanged || colorFormatChanged;

        GpuImageDesc depthDesc;
        if (!validDepthHandle) {
            depthDesc = colorDesc;
            depthDesc.format = createdTargets_.imageDescs.depth.format;
        } else {
            depthDesc = gpuResMgr.GetImageDescriptor(camRes_.depthTarget);
        }

        const bool depthFormatChanged = prevDepthFormat_ != camRes_.inputImageDescs.depth.format;
        prevDepthFormat_ = camRes_.inputImageDescs.depth.format;

        bool depthTargetChanged =
            DepthTargetsRecreationNeeded(camera, camRes_, depthDesc) || upscaleChanged || depthFormatChanged;
        // depth size needs to match color
        if ((colorDesc.width != depthDesc.width) || (colorDesc.height != depthDesc.height)) {
            depthTargetChanged = true;
            depthDesc.width = colorDesc.width;
            depthDesc.height = colorDesc.height;
        }

        camRes_.outResolution.x = colorDesc.width;
        camRes_.outResolution.y = colorDesc.height;
        camRes_.renResolution = currentScene_.camera.renderResolution;
        camRes_.camFlags = camera.flags;
        camRes_.pipelineType = camera.renderPipelineType;
        camRes_.isMultiview = (camera.multiViewCameraCount > 0U);
        camRes_.sampleCountFlags = camera.msaaSampleCountFlags;

        if (isMultiview) {
            colorDesc.layerCount = mvLayerCount;
        }

        if ((camRes_.renResolution.x < 1U) || (camRes_.renResolution.y < 1U)) {
            camRes_.renResolution.x = Math::max(1u, colorDesc.width);
            camRes_.renResolution.y = Math::max(1u, colorDesc.height);
#if (CORE3D_VALIDATION_ENABLED == 1)
            const string_view nodeName = renderNodeContextMgr_->GetName();
            CORE_LOG_ONCE_E(nodeName + "cam_controller_renRes",
                "CORE3D_VALIDATION: RN:%s camera render resolution resized to match target %ux%u", nodeName.data(),
                camRes_.renResolution.x, camRes_.renResolution.y);
#endif
        }

        const bool enableRenderRes = (camera.renderPipelineType != RenderCamera::RenderPipelineType::LIGHT_FORWARD);
        if (enableRenderRes) {
            colorDesc.width = camRes_.renResolution.x;
            colorDesc.height = camRes_.renResolution.y;
            depthDesc.width = camRes_.renResolution.x;
            depthDesc.height = camRes_.renResolution.y;
        }

        // all decisions are done based on color and depth targets
        const string_view us = stores_.dataStoreNameScene;
        if (colorTargetChanged) {
            CreateColorTargets(gpuResMgr, camera, colorDesc, us, currentScene_.customCamRngName, camRes_,
                createdTargets_, upscaleRatio);
        }
        if (depthTargetChanged) {
            CreateDepthTargets(
                gpuResMgr, camera, depthDesc, us, currentScene_.customCamRngName, camRes_, createdTargets_);
        }
    }
}

void RenderNodeDefaultCameraController::CreateResourceBaseTargets()
{
    IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    const auto& camera = currentScene_.camera;

    camRes_.colorTarget = camera.colorTargets[0].GetHandle();
    camRes_.depthTarget = camera.depthTarget.GetHandle();
    // update formats if given
    auto UpdateTargetFormats = [](const auto& input, auto& output) {
        if (input.format != BASE_FORMAT_UNDEFINED) {
            output.format = input.format;
        }
        if (input.usageFlags != 0) {
            output.usageFlags = input.usageFlags;
        }
    };
    UpdateTargetFormats(camera.colorTargetCustomization[0U], camRes_.inputImageDescs.output);
    UpdateTargetFormats(camera.colorTargetCustomization[0U], camRes_.inputImageDescs.color);
    UpdateTargetFormats(camera.depthTargetCustomization, camRes_.inputImageDescs.depth);
    if (camera.flags & RenderCamera::CAMERA_FLAG_MAIN_BIT) {
#if (CORE3D_VALIDATION_ENABLED == 1)
        if ((!RenderHandleUtil::IsValid(camRes_.colorTarget)) &&
            (camera.flags & RenderCamera::CAMERA_FLAG_CUBEMAP_BIT)) {
            CORE_LOG_ONCE_W(renderNodeContextMgr_->GetName() + "cubemap_def_backbuffer",
                "CORE3D_VALIDATION: camera (%s) main camera with default backbuffer cannot be cubemap camera",
                currentScene_.customCamRngName.data());
        }
#endif
        if ((!RenderHandleUtil::IsValid(camRes_.colorTarget)) &&
            ((camera.flags & RenderCamera::CAMERA_FLAG_CUBEMAP_BIT) == 0)) {
            camRes_.colorTarget = gpuResourceMgr.GetImageHandle("CORE_DEFAULT_BACKBUFFER");
#if (CORE3D_VALIDATION_ENABLED == 1)
            CORE_LOG_ONCE_I(renderNodeContextMgr_->GetName() + "using_def_backbuffer",
                "CORE3D_VALIDATION: camera (%s) using CORE_DEFAULT_BACKBUFFER", currentScene_.customCamRngName.data());
#endif
        }
    }
    if (!RenderHandleUtil::IsValid(camRes_.colorTarget)) {
        const string_view us = stores_.dataStoreNameScene;
        if (createdTargets_.outputColor) {
            const GpuImageDesc colorDesc = gpuResourceMgr.GetImageDescriptor(createdTargets_.outputColor.GetHandle());
            if ((colorDesc.width != camera.renderResolution.x) || (colorDesc.height != camera.renderResolution.y)) {
                CreateBaseColorTarget(
                    gpuResourceMgr, camera, us, currentScene_.customCamRngName, camRes_, createdTargets_);
            }
        } else {
            CreateBaseColorTarget(gpuResourceMgr, camera, us, currentScene_.customCamRngName, camRes_, createdTargets_);
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
    constexpr MemoryPropertyFlags memPropertyFlags =
        (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    const string_view us = stores_.dataStoreNameScene;
    uboHandles_.generalData =
        gpuResourceMgr.Create(us + DefaultMaterialCameraConstants::CAMERA_GENERAL_BUFFER_PREFIX_NAME + camName,
            GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memPropertyFlags,
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                static_cast<uint32_t>(sizeof(DefaultMaterialGeneralDataStruct)) });
    uboHandles_.environment =
        gpuResourceMgr.Create(us + DefaultMaterialCameraConstants::CAMERA_ENVIRONMENT_BUFFER_PREFIX_NAME + camName,
            GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memPropertyFlags,
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                static_cast<uint32_t>(sizeof(DefaultMaterialEnvironmentStruct)) *
                    CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT });
    uboHandles_.fog = gpuResourceMgr.Create(
        us + DefaultMaterialCameraConstants::CAMERA_FOG_BUFFER_PREFIX_NAME + camName,
        GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memPropertyFlags,
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, static_cast<uint32_t>(sizeof(DefaultMaterialFogStruct)) });
    uboHandles_.postProcess = gpuResourceMgr.Create(
        us + DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_BUFFER_PREFIX_NAME + camName,
        GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memPropertyFlags,
            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, static_cast<uint32_t>(sizeof(GlobalPostProcessStruct)) });

    uboHandles_.light =
        gpuResourceMgr.Create(us + DefaultMaterialCameraConstants::CAMERA_LIGHT_BUFFER_PREFIX_NAME + camName,
            GpuBufferDesc { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT, memPropertyFlags,
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER, sizeof(DefaultMaterialLightStruct) });

    // NOTE: storage buffer
    uboHandles_.lightCluster =
        gpuResourceMgr.Create(us + DefaultMaterialCameraConstants::CAMERA_LIGHT_CLUSTER_BUFFER_PREFIX_NAME + camName,
            GpuBufferDesc { CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT, memPropertyFlags,
                CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                sizeof(DefaultMaterialLightClusterData) * CORE_DEFAULT_MATERIAL_MAX_CLUSTERS_COUNT });
}

void RenderNodeDefaultCameraController::UpdateBuffers()
{
    UpdateGeneralUniformBuffer();
    UpdateLightBuffer();
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

namespace {
Math::UVec4 GetMultiEnvironmentIndices(const RenderCamera& cam)
{
    if (cam.environment.multiEnvCount > 0U) {
        Math::UVec4 multiEnvIndices = { 0U, 0U, 0U, 0U };
        // the first value in multiEnvIndices is the count
        // first index is the main environment, next indices are the blend environments
        for (uint32_t idx = 1U; idx < cam.environment.multiEnvCount + 1U; ++idx) {
            multiEnvIndices[0U]++;
            multiEnvIndices[idx] = idx;
        }
        return multiEnvIndices;
    } else {
        return { 0U, 0U, 0U, 0U };
    }
}

inline RenderDataStoreWeather::WeatherSettings GetWeatherSettings(
    const IRenderNodeRenderDataStoreManager& rdsMgr, const string_view dsName)
{
    if (const auto* dsWeather = static_cast<RenderDataStoreWeather*>(rdsMgr.GetRenderDataStore(dsName)); dsWeather) {
        return dsWeather->GetWeatherSettings();
    } else {
        return {};
    }
}

} // namespace

void RenderNodeDefaultCameraController::UpdateEnvironmentUniformBuffer()
{
    BASE_NS::Math::Vec4
        defaultSHIndirectCoefficients[9u] {}; // nine vectors which are used in spherical harmonics calculations
    defaultSHIndirectCoefficients[0] = { 1.0f, 1.0f, 1.0f, 1.0f };
    constexpr uint32_t envByteSize =
        sizeof(DefaultMaterialEnvironmentStruct) * CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT;

    const auto& rdsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    if (const auto* dsCamera =
            static_cast<IRenderDataStoreDefaultCamera*>(rdsMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
        dsCamera) {
        const RenderDataStoreWeather::WeatherSettings ws = GetWeatherSettings(rdsMgr, dsWeatherName_);

        // the environments needs to be in camera sorted order
        // we fetch environments one by one based on their id
        // in typical scenario there's only one environment present and others are filled with default data
        IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboHandles_.environment.GetHandle()));
            data) {
            const auto* dataEnd = data + envByteSize;
            const auto& camera = currentScene_.camera;
            const Math::UVec4 multiEnvIndices = GetMultiEnvironmentIndices(camera);
            // process main environment and multi envs
            const uint32_t envCount = 1U + camera.environment.multiEnvCount;
            for (uint32_t idx = 0; idx < envCount; ++idx) {
                const RenderCamera::Environment currEnv =
                    (idx == 0U) ? camera.environment
                                : dsCamera->GetEnvironment(camera.environment.multiEnvIds[idx - 1U]);

                const Math::UVec2 id = GetPacked64(currEnv.id);
                const Math::UVec2 layer = GetPacked64(currEnv.layerMask);
                const float radianceCubemapLodCoeff =
                    (currEnv.radianceCubemapMipCount != 0)
                        ? Math::min(CUBE_MAP_LOD_COEFF, static_cast<float>(currEnv.radianceCubemapMipCount))
                        : CUBE_MAP_LOD_COEFF;
                DefaultMaterialEnvironmentStruct envStruct {
                    Math::Vec4((Math::Vec3(currEnv.indirectSpecularFactor) * currEnv.indirectSpecularFactor.w),
                        currEnv.indirectSpecularFactor.w),
                    Math::Vec4(Math::Vec3(currEnv.indirectDiffuseFactor) * currEnv.indirectDiffuseFactor.w,
                        currEnv.indirectDiffuseFactor.w),
                    Math::Vec4(Math::Vec3(currEnv.envMapFactor) * currEnv.envMapFactor.w, currEnv.envMapFactor.w),
                    Math::Vec4(radianceCubemapLodCoeff, currEnv.envMapLodLevel, 0.0f, 0.0f),
                    currEnv.blendFactor,
                    Math::Mat4Cast(currEnv.rotation),
                    Math::UVec4(id.x, id.y, layer.x, layer.y),
                    {},
                    multiEnvIndices,
                    {},
                    {},
                    {},
                    {},
                };
                constexpr size_t countOfSh = countof(envStruct.shIndirectCoefficients);
                if (currEnv.radianceCubemap || (currEnv.multiEnvCount > 0U)) {
                    for (size_t jdx = 0; jdx < countOfSh; ++jdx) {
                        envStruct.shIndirectCoefficients[jdx] = currEnv.shIndirectCoefficients[jdx];
                    }
                } else {
                    for (size_t jdx = 0; jdx < countOfSh; ++jdx) {
                        envStruct.shIndirectCoefficients[jdx] = defaultSHIndirectCoefficients[jdx];
                    }
                }

                envStruct.packedSun = Math::Vec4(ws.sunDirElevation, 0.0f);
                envStruct.groundColor = ws.groundColor;
                if (currEnv.backgroundType == RenderCamera::Environment::BG_TYPE_SKY) {
                    envStruct.packedSun.w = 1.0f;
                } else {
                    envStruct.packedSun.w = 0.0f;
                }

                envStruct.packedPhases =
                    Math::Vec4(ws.timeOfDay, ws.moonBrightness, ws.nightGlow, ws.skyViewBrightness);

                if (!CloneData(data, size_t(dataEnd - data), &envStruct, sizeof(DefaultMaterialEnvironmentStruct))) {
                    CORE_LOG_E("environment ubo copying failed.");
                }
                data = data + sizeof(DefaultMaterialEnvironmentStruct);
            }
            gpuResourceMgr.UnmapBuffer(uboHandles_.environment.GetHandle());
        }
    }
}

void RenderNodeDefaultCameraController::UpdateFogUniformBuffer()
{
    const auto& camera = currentScene_.camera;
    const RenderCamera::Fog& fog = camera.fog;
    const Math::UVec2 id = GetPacked64(fog.id);
    const Math::UVec2 layer = GetPacked64(fog.layerMask);
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

void RenderNodeDefaultCameraController::UpdateLightBuffer()
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    // NOTE: should be culled by camera view frustum (or clustering can handle that)
    if (dataStoreScene && dataStoreLight) {
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const auto scene = dataStoreScene->GetScene();
        const uint32_t sceneId = currentScene_.camera.sceneId;
        const auto& lights = dataStoreLight->GetLights();
        const Math::Vec4 shadowAtlasSizeInvSize = RenderLightHelper::GetShadowAtlasSizeInvSize(*dataStoreLight);
        const uint32_t shadowCount = dataStoreLight->GetLightCounts().shadowCount;
        // light buffer update (needs to be updated every frame)
        if (auto data = reinterpret_cast<uint8_t*>(gpuResourceMgr.MapBuffer(uboHandles_.light.GetHandle())); data) {
            // NOTE: do not read data from mapped buffer (i.e. do not use mapped buffer as input to anything)
            RenderLightHelper::LightCounts lightCounts;
            const uint32_t lightCount = std::min(CORE_DEFAULT_MATERIAL_MAX_LIGHT_COUNT, (uint32_t)lights.size());
            vector<RenderLightHelper::SortData> sortedFlags =
                RenderLightHelper::SortLights(lights, lightCount, sceneId);

            auto* singleLightStruct =
                reinterpret_cast<DefaultMaterialSingleLightStruct*>(data + RenderLightHelper::LIGHT_LIST_OFFSET);
            for (size_t idx = 0; idx < sortedFlags.size(); ++idx) {
                const auto& sortData = sortedFlags[idx];
                const auto& light = lights[sortData.index];
                // drop lights with no matching camera layers
                if (light.layerMask & currentScene_.camera.layerMask) {
                    RenderLightHelper::EvaluateLightCounts(sortData.lightUsageFlags, lightCounts);
                    RenderLightHelper::CopySingleLight(lights[sortData.index], shadowCount, singleLightStruct++);
                }
            }

            DefaultMaterialLightStruct* lightStruct = reinterpret_cast<DefaultMaterialLightStruct*>(data);
            uint32_t currLightCount = 0U;

            lightStruct->directionalLightBeginIndex = 0U;
            lightStruct->directionalLightCount = lightCounts.directionalLightCount;
            currLightCount += lightCounts.directionalLightCount;

            lightStruct->pointLightBeginIndex = currLightCount;
            lightStruct->pointLightCount = lightCounts.pointLightCount;
            currLightCount += lightCounts.pointLightCount;

            lightStruct->spotLightBeginIndex = currLightCount;
            lightStruct->spotLightCount = lightCounts.spotLightCount;
            currLightCount += lightCounts.spotLightCount;

            lightStruct->rectLightBeginIndex = currLightCount;
            lightStruct->rectLightCount = lightCounts.rectLightCount;

            lightStruct->clusterSizes = Math::UVec4(0, 0, 0, 0);
            lightStruct->clusterFactors = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);
            lightStruct->atlasSizeInvSize = shadowAtlasSizeInvSize;
            lightStruct->additionalFactors = { 0.0f, 0.0f, 0.0f, 0.0f };

            gpuResourceMgr.UnmapBuffer(uboHandles_.light.GetHandle());
        }
    }
}

void RenderNodeDefaultCameraController::UpdatePostProcessConfiguration()
{
    const auto& dataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    if (auto* dataStore = static_cast<IRenderDataStorePod*>(dataStoreMgr.GetRenderDataStore(POD_DATA_STORE_NAME));
        dataStore) {
        auto const dataView = dataStore->Get(currentScene_.camera.postProcessName);
        if (dataView.data() && (dataView.size_bytes() == sizeof(PostProcessConfiguration))) {
            const PostProcessConfiguration* data = (const PostProcessConfiguration*)dataView.data();
            currentRenderPPConfiguration_ =
                renderNodeContextMgr_->GetRenderNodeUtil().GetRenderPostProcessConfiguration(*data);
        }
    }
}

void RenderNodeDefaultCameraController::ClusterLights(RENDER_NS::IRenderCommandList& cmdList)
{
    cmdList.BindPipeline(clusterBinders_.psoHandle);

    {
        auto& binder = *clusterBinders_.clusterBuffersSet0;

        uint32_t binding = 0;
        binder.BindBuffer(binding++, uboHandles_.cameraData, 0);
        binder.BindBuffer(binding++, uboHandles_.generalData.GetHandle(), 0);
        binder.BindBuffer(binding++, uboHandles_.light.GetHandle(), 0);
        binder.BindBuffer(binding++, uboHandles_.lightCluster.GetHandle(), 0);

        if (!binder.GetDescriptorSetLayoutBindingValidity()) {
            return;
        }

        cmdList.UpdateDescriptorSet(binder.GetDescriptorSetHandle(), binder.GetDescriptorSetLayoutBindingResources());
        cmdList.BindDescriptorSet(0, binder.GetDescriptorSetHandle());
    }

    const uint32_t tgx = (CORE_DEFAULT_MATERIAL_MAX_CLUSTERS_COUNT + (LIGHT_CLUSTER_TGS - 1)) / LIGHT_CLUSTER_TGS;
    cmdList.Dispatch(tgx, 1, 1);

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

void RenderNodeDefaultCameraController::SetDefaultGpuImageDescs()
{
    camRes_.inputImageDescs.depth = DEPTH_DEFAULT_DESC;

    camRes_.inputImageDescs.output = OUTPUT_DEFAULT_DESC;
    camRes_.inputImageDescs.color = COLOR_DEFAULT_DESC;
    camRes_.inputImageDescs.velocityNormal = VELOCITY_NORMAL_DEFAULT_DESC;
    camRes_.inputImageDescs.history = HISTORY_DEFAULT_DESC;
    camRes_.inputImageDescs.baseColor = BASE_COLOR_DEFAULT_DESC;
    camRes_.inputImageDescs.material = MATERIAL_DEFAULT_DESC;
}

void RenderNodeDefaultCameraController::ParseRenderNodeInputs()
{
    const IRenderNodeParserUtil& parserUtil = renderNodeContextMgr_->GetRenderNodeParserUtil();
    const auto jsonVal = renderNodeContextMgr_->GetNodeJson();
    jsonInputs_.customCameraName = parserUtil.GetStringValue(jsonVal, "customCameraName");
    jsonInputs_.customCameraId = parserUtil.GetUintValue(jsonVal, "customCameraId");

    const vector<RenderNodeGraphInputs::RenderNodeGraphGpuImageDesc> imageDescs =
        parserUtil.GetGpuImageDescs(jsonVal, "gpuImageDescs");

    for (const auto& ref : imageDescs) {
        if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_OUTPUT) {
            camRes_.inputImageDescs.output = ref.desc;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_DEPTH) {
            camRes_.inputImageDescs.depth = ref.desc;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR) {
            camRes_.inputImageDescs.color = ref.desc;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_COLOR_UPSCALED) {
            camRes_.inputImageDescs.colorUpscaled = ref.desc;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_HISTORY) {
            camRes_.inputImageDescs.history = ref.desc;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_VELOCITY_NORMAL) {
            camRes_.inputImageDescs.velocityNormal = ref.desc;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_BASE_COLOR) {
            camRes_.inputImageDescs.baseColor = ref.desc;
        } else if (ref.name == DefaultMaterialRenderNodeConstants::CORE_DM_CAMERA_MATERIAL) {
            camRes_.inputImageDescs.material = ref.desc;
        }
    }

    // validate
    const IRenderNodeGpuResourceManager& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
    ValidateDepthDesc(gpuResourceMgr, DEPTH_DEFAULT_DESC, camRes_.inputImageDescs.depth);
    ValidateColorDesc(gpuResourceMgr, OUTPUT_DEFAULT_DESC, true, camRes_.inputImageDescs.output);
    ValidateColorDesc(gpuResourceMgr, COLOR_DEFAULT_DESC, true, camRes_.inputImageDescs.color);
    ValidateColorDesc(gpuResourceMgr, VELOCITY_NORMAL_DEFAULT_DESC, true, camRes_.inputImageDescs.velocityNormal);
    ValidateColorDesc(gpuResourceMgr, HISTORY_DEFAULT_DESC, true, camRes_.inputImageDescs.history);
    ValidateColorDesc(gpuResourceMgr, BASE_COLOR_DEFAULT_DESC, false, camRes_.inputImageDescs.baseColor);
    ValidateColorDesc(gpuResourceMgr, MATERIAL_DEFAULT_DESC, false, camRes_.inputImageDescs.material);
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
