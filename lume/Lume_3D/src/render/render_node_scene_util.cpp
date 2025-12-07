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

#include "render_node_scene_util.h"

#include <algorithm>
#include <cinttypes>

#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_material.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/vector.h>
#include <base/math/matrix_util.h>
#include <core/implementation_uids.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/util/intf_frustum_util.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_node_context_descriptor_set_manager.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "render/datastore/render_data_store_default_material.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
static constexpr uint64_t INVALID_CAM_ID { 0xFFFFFFFFffffffff };

inline bool operator<(const SlotSubmeshIndex& lhs, const SlotSubmeshIndex& rhs)
{
    if (lhs.sortLayerKey < rhs.sortLayerKey) {
        return true;
    } else if (lhs.sortLayerKey == rhs.sortLayerKey) {
        return (lhs.sortKey < rhs.sortKey);
    } else {
        return false;
    }
}

inline bool operator>(const SlotSubmeshIndex& lhs, const SlotSubmeshIndex& rhs)
{
    if (lhs.sortLayerKey < rhs.sortLayerKey) {
        return true;
    } else if (lhs.sortLayerKey == rhs.sortLayerKey) {
        return (lhs.sortKey > rhs.sortKey);
    } else {
        return false;
    }
}

template<class T>
struct Less {
    constexpr bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs < rhs;
    }
};

template<class T>
struct Greater {
    constexpr bool operator()(const T& lhs, const T& rhs) const
    {
        return lhs > rhs;
    }
};

void UpdateRenderArea(const Math::Vec4& scissor, RenderPassDesc::RenderArea& renderArea)
{
    // prevent larger than image render area
    if (scissor.x >= 0.0f && scissor.y >= 0.0f && scissor.z <= 1.0f && scissor.w <= 1.0f) {
        const float fWidth = static_cast<float>(renderArea.extentWidth);
        const float fHeight = static_cast<float>(renderArea.extentHeight);

        const float offsetX = (fWidth * scissor.x);
        const float offsetY = (fHeight * scissor.y);
        const float extentWidth = (fWidth * scissor.z);
        const float extentHeight = (fHeight * scissor.w);

        renderArea = {
            static_cast<int32_t>(offsetX),
            static_cast<int32_t>(offsetY),
            static_cast<uint32_t>(extentWidth),
            static_cast<uint32_t>(extentHeight),
        };
    }
}

void UpdateCustomCameraTargets(const RenderCamera& camera, RenderPass& renderPass)
{
    auto& subpassDesc = renderPass.subpassDesc;
    RenderPassDesc& renderPassDesc = renderPass.renderPassDesc;
    if ((camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) == 0) {
        if ((subpassDesc.depthAttachmentCount == 1) && camera.depthTarget) {
            renderPassDesc.attachmentHandles[subpassDesc.depthAttachmentIndex] = camera.depthTarget.GetHandle();
        }
        if ((subpassDesc.colorAttachmentCount >= 1) && camera.colorTargets[0u]) {
            renderPassDesc.attachmentHandles[subpassDesc.colorAttachmentIndices[0]] =
                camera.colorTargets[0u].GetHandle();
        }
    }
}

void UpdateCustomCameraLoadStore(const RenderCamera& camera, RenderPass& renderPass)
{
    // NOTE: if clear bits given the camera RNG loadOp is overrided with clear
    // otherwise the loadOp is the one from RNG
    auto& subpassDesc = renderPass.subpassDesc;
    if ((subpassDesc.depthAttachmentCount == 1) &&
        (subpassDesc.depthAttachmentIndex < PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT)) {
        auto& attRef = renderPass.renderPassDesc.attachments[subpassDesc.depthAttachmentIndex];
        if ((camera.flags & RenderCamera::CAMERA_FLAG_CLEAR_DEPTH_BIT)) {
            attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            attRef.clearValue.depthStencil = camera.clearDepthStencil;
        }
        if (((camera.flags & RenderCamera::CAMERA_FLAG_MSAA_BIT) == 0) &&
            (camera.flags & RenderCamera::CAMERA_FLAG_OUTPUT_DEPTH_BIT)) {
            attRef.storeOp = AttachmentStoreOp::CORE_ATTACHMENT_STORE_OP_STORE;
        }
    }
    for (uint32_t idx = 0; idx < subpassDesc.colorAttachmentCount; ++idx) {
        if (subpassDesc.colorAttachmentIndices[idx] < PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT) {
            auto& attRef = renderPass.renderPassDesc.attachments[subpassDesc.colorAttachmentIndices[idx]];
            if (camera.flags & RenderCamera::CAMERA_FLAG_CLEAR_COLOR_BIT) {
                attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
                attRef.clearValue.color = camera.clearColorValues;
            }
        }
    }
}

void UpdateCameraFlags(const RenderCamera& camera, RenderPass& renderPass)
{
    // NOTE: does not over write viewmask if it has some special values
    if (renderPass.subpassDesc.viewMask <= 1U) {
        if (camera.multiViewCameraCount > 0U) {
            const uint32_t layerCount = camera.multiViewCameraCount + 1U;
            renderPass.subpassDesc.viewMask = (1U << layerCount) - 1U;
        } else if (camera.flags & RenderCamera::CAMERA_FLAG_CUBEMAP_BIT) {
            renderPass.subpassDesc.viewMask = 0x3F; // all cubemap layers
        }
    }
}

bool IsObjectCulled(IFrustumUtil& frustumUtil, const Frustum& camFrustum, const array_view<const Frustum> addFrustums,
    const RenderSubmesh& submesh)
{
    bool notCulled =
        frustumUtil.SphereFrustumCollision(camFrustum, submesh.bounds.worldCenter, submesh.bounds.worldRadius);
    if (!notCulled) {
        // if culled by main camera check additional cameras
        for (const auto& fRef : addFrustums) {
            notCulled =
                frustumUtil.SphereFrustumCollision(fRef, submesh.bounds.worldCenter, submesh.bounds.worldRadius);
            if (notCulled) {
                break;
            }
        }
    }
    return !notCulled;
}

inline constexpr RenderSlotCullType GetRenderSlotBaseCullType(
    const RenderSlotCullType cullType, const RenderCamera& camera)
{
    if (camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_CUBEMAP_BIT) {
        return RenderSlotCullType::NONE;
    } else {
        return cullType;
    }
}
} // namespace

SceneRenderDataStores RenderNodeSceneUtil::GetSceneRenderDataStores(
    const IRenderNodeContextManager& renderNodeContextMgr, const string_view sceneDataStoreName)
{
    SceneRenderDataStores stores;
    stores.dataStoreNameScene = sceneDataStoreName.empty() ? "RenderDataStoreDefaultScene" : sceneDataStoreName;
    const auto& renderDataStoreMgr = renderNodeContextMgr.GetRenderDataStoreManager();
    const IRenderDataStoreDefaultScene* dsScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores.dataStoreNameScene));
    if (dsScene) {
        const RenderScene& rs = dsScene->GetScene();
        stores.dataStoreNameMaterial =
            rs.dataStoreNameMaterial.empty() ? "RenderDataStoreDefaultMaterial" : rs.dataStoreNameMaterial;
        stores.dataStoreNameCamera =
            rs.dataStoreNameCamera.empty() ? "RenderDataStoreDefaultCamera" : rs.dataStoreNameCamera;
        stores.dataStoreNameLight =
            rs.dataStoreNameLight.empty() ? "RenderDataStoreDefaultLight" : rs.dataStoreNameLight;
        stores.dataStoreNameMorph = rs.dataStoreNameMorph.empty() ? "RenderDataStoreMorph" : rs.dataStoreNameMorph;
        stores.dataStoreNamePrefix = rs.dataStoreNamePrefix;
    }
    return stores;
}

string RenderNodeSceneUtil::GetSceneRenderDataStore(const SceneRenderDataStores& sceneRds, const string_view dsName)
{
    return string(sceneRds.dataStoreNamePrefix + dsName);
}

ViewportDesc RenderNodeSceneUtil::CreateViewportFromCamera(const RenderCamera& camera)
{
    const float fRenderResX = static_cast<float>(camera.renderResolution.x);
    const float fRenderResY = static_cast<float>(camera.renderResolution.y);

    const float offsetX = (fRenderResX * camera.viewport.x);
    const float offsetY = (fRenderResY * camera.viewport.y);
    const float extentWidth = (fRenderResX * camera.viewport.z);
    const float extentHeight = (fRenderResY * camera.viewport.w);

    return ViewportDesc {
        offsetX,
        offsetY,
        extentWidth,
        extentHeight,
        0.0f,
        1.0f,
    };
}

ScissorDesc RenderNodeSceneUtil::CreateScissorFromCamera(const RenderCamera& camera)
{
    const float fRenderResX = static_cast<float>(camera.renderResolution.x);
    const float fRenderResY = static_cast<float>(camera.renderResolution.y);

    const float offsetX = (fRenderResX * camera.scissor.x);
    const float offsetY = (fRenderResY * camera.scissor.y);
    const float extentWidth = (fRenderResX * camera.scissor.z);
    const float extentHeight = (fRenderResY * camera.scissor.w);

    return ScissorDesc {
        static_cast<int32_t>(offsetX),
        static_cast<int32_t>(offsetY),
        static_cast<uint32_t>(extentWidth),
        static_cast<uint32_t>(extentHeight),
    };
}

void RenderNodeSceneUtil::UpdateRenderPassFromCamera(const RenderCamera& camera, RenderPass& renderPass)
{
    renderPass.renderPassDesc.renderArea = { 0, 0, camera.renderResolution.x, camera.renderResolution.y };
    if (camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_CUSTOM_TARGETS_BIT) {
        UpdateCustomCameraTargets(camera, renderPass);
    }
    // NOTE: UpdateCustomCameraClears(camera, renderPass) is not yet called
    UpdateRenderArea(camera.scissor, renderPass.renderPassDesc.renderArea);
    UpdateCameraFlags(camera, renderPass);
}

void RenderNodeSceneUtil::UpdateRenderPassFromCustomCamera(
    const RenderCamera& camera, const bool isNamedCamera, RenderPass& renderPass)
{
    renderPass.renderPassDesc.renderArea = { 0, 0, camera.renderResolution.x, camera.renderResolution.y };
    // NOTE: legacy support is only taken into account when isNamedCamera flag is true
    if ((camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_CUSTOM_TARGETS_BIT) && isNamedCamera) {
        UpdateCustomCameraTargets(camera, renderPass);
    }
    UpdateCustomCameraLoadStore(camera, renderPass);
    UpdateRenderArea(camera.scissor, renderPass.renderPassDesc.renderArea);
    UpdateCameraFlags(camera, renderPass);
}

void RenderNodeSceneUtil::GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t cameraIndex,
    const array_view<const uint32_t> addCameraIndices, const IRenderNodeSceneUtil::RenderSlotInfo& renderSlotInfo,
    vector<SlotSubmeshIndex>& refSubmeshIndices)
{
    // Get IFrustumUtil from global plugin registry.
    auto frustumUtil = GetInstance<IFrustumUtil>(UID_FRUSTUM_UTIL);
    if (!frustumUtil) {
        return;
    }

    // 64 bit sorting key should have
    // material hash with:
    // shader, materialIndex, meshId, and depth

    double camSortCoefficient = 1000.0;
    Math::Mat4X4 camView { Math::IDENTITY_4X4 };
    uint64_t camLayerMask { RenderSceneDataConstants::INVALID_ID };
    uint32_t camLevel { 0U };
    uint32_t camReflection { 0U };
    Frustum camFrustum;
    bool reflectionCamera = false;
    const auto& cameras = dataStoreCamera.GetCameras();
    const uint32_t maxCameraCount = static_cast<uint32_t>(cameras.size());
    RenderSlotCullType rsCullType = renderSlotInfo.cullType;
    // process first camera
    if (cameraIndex < maxCameraCount) {
        const auto& cam = cameras[cameraIndex];
        camView = cam.matrices.view;
        const float camZFar = Math::max(0.01f, cam.zFar);
        // max uint coefficient for sorting
        camSortCoefficient = double(UINT32_MAX) / double(camZFar);
        camLayerMask = cam.layerMask;
        camLevel = cam.sceneId;
        camReflection = cam.reflectionId;
        reflectionCamera = cam.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_REFLECTION_BIT;
        rsCullType = GetRenderSlotBaseCullType(rsCullType, cam);
        if (rsCullType == RenderSlotCullType::VIEW_FRUSTUM_CULL) {
            camFrustum = frustumUtil->CreateFrustum(cam.matrices.proj * cam.matrices.view);
        }
    }
    vector<Frustum> addFrustums;
    if (rsCullType == RenderSlotCullType::VIEW_FRUSTUM_CULL) {
        addFrustums.reserve(addCameraIndices.size());
        for (const auto& indexRef : addCameraIndices) {
            if (indexRef < maxCameraCount) {
                addFrustums.push_back(
                    frustumUtil->CreateFrustum(cameras[indexRef].matrices.proj * cameras[indexRef].matrices.view));
            }
        }
    }

    constexpr uint64_t maxUDepth = RenderDataStoreDefaultMaterial::SLOT_SORT_MAX_DEPTH;
    constexpr uint64_t sDepthShift = RenderDataStoreDefaultMaterial::SLOT_SORT_DEPTH_SHIFT;
    constexpr uint64_t sRenderMask = RenderDataStoreDefaultMaterial::SLOT_SORT_HASH_MASK;
    constexpr uint64_t sRenderShift = RenderDataStoreDefaultMaterial::SLOT_SORT_HASH_SHIFT;

    // NOTE: material sort should be based on PSO not shader handle
    const auto& slotSubmeshIndices = dataStoreMaterial.GetSlotSubmeshIndices(renderSlotInfo.id);
    const auto& slotSubmeshMatData = dataStoreMaterial.GetSlotSubmeshMaterialData(renderSlotInfo.id);
    const auto& submeshes = dataStoreMaterial.GetSubmeshes();

    refSubmeshIndices.clear();
    refSubmeshIndices.reserve(slotSubmeshIndices.size());
    for (size_t idx = 0; idx < slotSubmeshIndices.size(); ++idx) {
        const uint32_t submeshIndex = slotSubmeshIndices[idx];
        const auto& submesh = submeshes[submeshIndex];
        // Skip mesh if it's not in the same scene, layer, or in case of a planar reflection camera the plane itself.
        if (camLevel != submesh.layers.sceneId) {
            continue;
        }
        if (reflectionCamera && (camReflection == (submesh.indices.id & 0xFFFFFFFFU))) {
            continue;
        }
        if ((camLayerMask & submesh.layers.layerMask) == 0U) {
            continue;
        }
        const auto& submeshMatData = slotSubmeshMatData[idx];
        const bool notCulled =
            ((submeshMatData.renderMaterialFlags & RenderMaterialFlagBits::RENDER_MATERIAL_CAMERA_EFFECT_BIT) ||
                (rsCullType != RenderSlotCullType::VIEW_FRUSTUM_CULL) ||
                (!IsObjectCulled(*frustumUtil, camFrustum, addFrustums, submesh)));
        const bool discardedMat = (submeshMatData.renderMaterialFlags & renderSlotInfo.materialDiscardFlags);
        if (notCulled && (!discardedMat)) {
            const Math::Vec4 pos = (camView * Math::Vec4(submesh.bounds.worldCenter, 1.0f));
            const float zVal = Math::abs(pos.z);
            uint64_t sortKey = Math::min(maxUDepth, static_cast<uint64_t>(double(zVal) * camSortCoefficient));
            if (renderSlotInfo.sortType == RenderSlotSortType::BY_MATERIAL) {
                // High 32bits for render sort hash, Low 32bits for depth
                sortKey |= (((uint64_t)submeshMatData.renderSortHash & sRenderMask) << sRenderShift);
            } else {
                // High 32bits for depth, Low 32bits for render sort hash
                sortKey = (sortKey << sDepthShift) | ((uint64_t)slotSubmeshMatData[idx].renderSortHash & sRenderMask);
            }
            refSubmeshIndices.push_back(SlotSubmeshIndex { (uint32_t)submeshIndex,
                submeshMatData.combinedRenderSortLayer, sortKey, submeshMatData.shader, submeshMatData.gfxState });
        }
    }

    if (renderSlotInfo.sortType == RenderSlotSortType::BY_MATERIAL) {
        // 1. Assemble similar materials into groups
        // 2. For each group calculate submesh with minimum distance to camera.
        // 3. Sort groups from front to back order based on distance

        // First sort by material, so identical materials grouped together
        std::sort(refSubmeshIndices.begin(), refSubmeshIndices.end(), Less<SlotSubmeshIndex>());

        struct MaterialGroup {
            // Denotes a range in refSubmeshIndices
            uint32_t submeshesStart = 0;
            uint32_t submeshesSize = 0;
            uint32_t sortLayerKey = 0U;
            uint32_t renderSortHash = 0;
            uint32_t minDepth = UINT32_MAX;
        };

        // Determine materials closest to camera
        vector<MaterialGroup> materialGroups;
        {
            MaterialGroup* group = &materialGroups.emplace_back();

            for (auto& mesh : refSubmeshIndices) {
                uint32_t meshRenderHash = static_cast<uint32_t>((mesh.sortKey >> sRenderShift) & sRenderMask);
                uint32_t meshDepth = static_cast<uint32_t>(mesh.sortKey & 0xffffffff); // Unpack low 32Bits for depth

                if ((meshRenderHash != group->renderSortHash) || (mesh.sortLayerKey != group->sortLayerKey)) {
                    // Create new group
                    if (group->submeshesSize > 0) {
                        uint32_t nextStart = group->submeshesStart + group->submeshesSize;
                        group = &materialGroups.emplace_back();
                        group->submeshesStart = nextStart;
                    }
                    group->sortLayerKey = mesh.sortLayerKey;
                    group->renderSortHash = meshRenderHash;
                }
                group->minDepth = Math::min(group->minDepth, meshDepth);
                group->submeshesSize++;
            }
        }

        std::sort(materialGroups.begin(), materialGroups.end(), [](const MaterialGroup& a, const MaterialGroup& b) {
            if (a.sortLayerKey < b.sortLayerKey) {
                return true;
            }
            if (a.sortLayerKey > b.sortLayerKey) {
                return false;
            }
            return a.minDepth < b.minDepth;
        });

        // Create new sorted array
        vector<SlotSubmeshIndex> sortedSubmeshIndices(refSubmeshIndices.size());
        auto sortedIt = sortedSubmeshIndices.begin();
        for (auto& matGroup : materialGroups) {
            const auto itBegin = refSubmeshIndices.begin() + matGroup.submeshesStart;
            const auto itEnd = itBegin + matGroup.submeshesSize;
            std::copy(itBegin, itEnd, sortedIt);
            sortedIt += matGroup.submeshesSize;
        }

        refSubmeshIndices = std::move(sortedSubmeshIndices);
    }
    // front-to-back render layer sort is 0 -> 63
    // back-to-front render layer sort is 63 -> 0
    else if (renderSlotInfo.sortType == RenderSlotSortType::FRONT_TO_BACK) {
        std::sort(refSubmeshIndices.begin(), refSubmeshIndices.end(), Less<SlotSubmeshIndex>());
    } else if (renderSlotInfo.sortType == RenderSlotSortType::BACK_TO_FRONT) {
        std::sort(refSubmeshIndices.begin(), refSubmeshIndices.end(), Greater<SlotSubmeshIndex>());
    }
}

SceneBufferHandles RenderNodeSceneUtil::GetSceneBufferHandles(
    RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName)
{
    SceneBufferHandles buffers;
    const auto& gpuMgr = renderNodeContextMgr.GetGpuResourceManager();
    buffers.camera = gpuMgr.GetBufferHandle(sceneName + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME);
    buffers.material = gpuMgr.GetBufferHandle(sceneName + DefaultMaterialMaterialConstants::MATERIAL_DATA_BUFFER_NAME);
    buffers.materialTransform =
        gpuMgr.GetBufferHandle(sceneName + DefaultMaterialMaterialConstants::MATERIAL_TRANSFORM_DATA_BUFFER_NAME);
    buffers.materialCustom =
        gpuMgr.GetBufferHandle(sceneName + DefaultMaterialMaterialConstants::MATERIAL_USER_DATA_BUFFER_NAME);
    buffers.mesh = gpuMgr.GetBufferHandle(sceneName + DefaultMaterialMaterialConstants::MESH_DATA_BUFFER_NAME);
    buffers.skinJoint = gpuMgr.GetBufferHandle(sceneName + DefaultMaterialMaterialConstants::SKIN_DATA_BUFFER_NAME);

    const RenderHandle defaultBuffer = gpuMgr.GetBufferHandle("CORE_DEFAULT_GPU_BUFFER");
    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(defaultBuffer, valid, buffers.camera);
    checkValidity(defaultBuffer, valid, buffers.material);
    checkValidity(defaultBuffer, valid, buffers.materialTransform);
    checkValidity(defaultBuffer, valid, buffers.materialCustom);
    checkValidity(defaultBuffer, valid, buffers.mesh);
    checkValidity(defaultBuffer, valid, buffers.skinJoint);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all scene buffers not found.", renderNodeContextMgr.GetName().data());
    }
    return buffers;
}

SceneCameraBufferHandles RenderNodeSceneUtil::GetSceneCameraBufferHandles(
    RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName,
    const BASE_NS::string_view cameraName)
{
    SceneCameraBufferHandles buffers;
    const auto& gpuMgr = renderNodeContextMgr.GetGpuResourceManager();
    buffers.generalData = gpuMgr.GetBufferHandle(
        sceneName + DefaultMaterialCameraConstants::CAMERA_GENERAL_BUFFER_PREFIX_NAME + cameraName);
    buffers.environment = gpuMgr.GetBufferHandle(
        sceneName + DefaultMaterialCameraConstants::CAMERA_ENVIRONMENT_BUFFER_PREFIX_NAME + cameraName);
    buffers.fog =
        gpuMgr.GetBufferHandle(sceneName + DefaultMaterialCameraConstants::CAMERA_FOG_BUFFER_PREFIX_NAME + cameraName);
    buffers.postProcess = gpuMgr.GetBufferHandle(
        sceneName + DefaultMaterialCameraConstants::CAMERA_POST_PROCESS_BUFFER_PREFIX_NAME + cameraName);

    buffers.light = gpuMgr.GetBufferHandle(
        sceneName + DefaultMaterialCameraConstants::CAMERA_LIGHT_BUFFER_PREFIX_NAME + cameraName);
    buffers.lightCluster = gpuMgr.GetBufferHandle(
        sceneName + DefaultMaterialCameraConstants::CAMERA_LIGHT_CLUSTER_BUFFER_PREFIX_NAME + cameraName);

    const RenderHandle defaultBuffer = gpuMgr.GetBufferHandle("CORE_DEFAULT_GPU_BUFFER");
    auto checkValidity = [](const RenderHandle defaultBuffer, bool& valid, RenderHandle& buffer) {
        if (!RenderHandleUtil::IsValid(buffer)) {
            valid = false;
            buffer = defaultBuffer;
        }
    };
    bool valid = true;
    checkValidity(defaultBuffer, valid, buffers.generalData);
    checkValidity(defaultBuffer, valid, buffers.environment);
    checkValidity(defaultBuffer, valid, buffers.fog);
    checkValidity(defaultBuffer, valid, buffers.postProcess);
    checkValidity(defaultBuffer, valid, buffers.light);
    checkValidity(defaultBuffer, valid, buffers.lightCluster);
    if (!valid) {
        CORE_LOG_E(
            "RN: %s, invalid configuration, not all camera buffers found.", renderNodeContextMgr.GetName().data());
    }
    return buffers;
}

SceneCameraImageHandles RenderNodeSceneUtil::GetSceneCameraImageHandles(
    RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName,
    const BASE_NS::string_view cameraName, const RenderCamera& camera)
{
    SceneCameraImageHandles handles;
    const auto& gpuMgr = renderNodeContextMgr.GetGpuResourceManager();
    handles.radianceCubemap = camera.environment.radianceCubemap.GetHandle();
    const bool tryFetchCreatedRadianceCubemap =
        ((!RenderHandleUtil::IsValid(handles.radianceCubemap)) && (camera.environment.multiEnvCount > 0U)) ||
        (camera.environment.backgroundType == RenderCamera::Environment::BackgroundType::BG_TYPE_SKY &&
            !RenderHandleUtil::IsValid(handles.radianceCubemap));
    if (tryFetchCreatedRadianceCubemap) {
        handles.radianceCubemap =
            gpuMgr.GetImageHandle(sceneName + DefaultMaterialSceneConstants::ENVIRONMENT_RADIANCE_CUBEMAP_PREFIX_NAME +
                                  to_hex(camera.environment.id));
    }
    if (!RenderHandleUtil::IsValid(handles.radianceCubemap)) {
        handles.radianceCubemap =
            gpuMgr.GetImageHandle(DefaultMaterialGpuResourceConstants::CORE_DEFAULT_RADIANCE_CUBEMAP);
    }
    return handles;
}

SceneRenderCameraData RenderNodeSceneUtil::GetSceneCameraData(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const uint64_t cameraId,
    const BASE_NS::string_view cameraName)
{
    const auto scene = dataStoreScene.GetScene();
    uint32_t cameraIdx = scene.cameraIndex;
    SceneRenderCameraDataFlags flags = 0;
    if (cameraId != INVALID_CAM_ID) {
        cameraIdx = dataStoreCamera.GetCameraIndex(cameraId);
    } else if (!(cameraName.empty())) {
        cameraIdx = dataStoreCamera.GetCameraIndex(cameraName);
        flags |= SceneRenderCameraDataFlagBits::SCENE_CAMERA_DATA_FLAG_NAMED_BIT;
    } else {
        // legacy support for created render node graphs without render node graph injection
        flags |= SceneRenderCameraDataFlagBits::SCENE_CAMERA_DATA_FLAG_LEGACY_MAIN_BIT;
    }

    if (const auto cameras = dataStoreCamera.GetCameras(); cameraIdx < (uint32_t)cameras.size()) {
        return { cameraIdx, flags, cameras[cameraIdx] };
    } else {
        const auto tmpName = to_string(cameraId) + "RenderNodeSceneUtil::GetSceneCamere";
        CORE_LOG_ONCE_W(tmpName, "Render camera not found  (id:%" PRIx64 " name:%s", cameraId, cameraName.data());
        return {};
    }
}

void RenderNodeSceneUtil::GetMultiViewCameraIndices(
    const IRenderDataStoreDefaultCamera& rds, const RenderCamera& cam, vector<uint32_t>& mvIndices)
{
    CORE_STATIC_ASSERT(RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT == 7U);
    const uint32_t inputCount =
        Math::min(cam.multiViewCameraCount, RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT);
    mvIndices.clear();
    mvIndices.reserve(inputCount);
    for (uint32_t idx = 0U; idx < inputCount; ++idx) {
        const uint64_t id = cam.multiViewCameraIds[idx];
        if (id != RenderSceneDataConstants::INVALID_ID) {
            mvIndices.push_back(Math::min(rds.GetCameraIndex(id), CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT - 1U));
        }
    }
}

RenderNodeSceneUtil::FrameGlobalDescriptorSets RenderNodeSceneUtil::GetFrameGlobalDescriptorSets(
    const IRenderNodeContextManager& rncm, const SceneRenderDataStores& stores, const string_view cameraName,
    const FrameGlobalDescriptorSetFlags flags)
{
    // re-fetch global descriptor sets every frame
    const INodeContextDescriptorSetManager& dsMgr = rncm.GetDescriptorSetManager();
    const string_view us = stores.dataStoreNameScene;
    FrameGlobalDescriptorSets fgds;
    fgds.valid = true;
    if (flags & FrameGlobalDescriptorSetFlagBits::GLOBAL_SET_0) {
        fgds.set0 = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_SET0_GLOBAL_DESCRIPTOR_SET_PREFIX_NAME + cameraName);
        fgds.valid = fgds.valid && RenderHandleUtil::IsValid(fgds.set0);
    }
    if (flags & FrameGlobalDescriptorSetFlagBits::GLOBAL_SET_1) {
        fgds.set1 = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_SET1_GLOBAL_DESCRIPTOR_SET_NAME);
        fgds.valid = fgds.valid && RenderHandleUtil::IsValid(fgds.set1);
    }
    if (flags & FrameGlobalDescriptorSetFlagBits::GLOBAL_SET_2) {
        fgds.set2 = dsMgr.GetGlobalDescriptorSets(
            us + DefaultMaterialMaterialConstants::MATERIAL_RESOURCES_GLOBAL_DESCRIPTOR_SET_NAME);
        fgds.set2Default = dsMgr.GetGlobalDescriptorSet(
            us + DefaultMaterialMaterialConstants::MATERIAL_DEFAULT_RESOURCE_GLOBAL_DESCRIPTOR_SET_NAME);
        fgds.valid = fgds.valid && RenderHandleUtil::IsValid(fgds.set2Default);
#if (CORE3D_VALIDATION_ENABLED == 1)
        if (fgds.set2.empty()) {
            CORE_LOG_ONCE_W("core3d_global_descriptor_set_render_slot_issues",
                "CORE3D_VALIDATION: Global descriptor set for default material env not found");
        }
#endif
    }
    if (!fgds.valid) {
        CORE_LOG_ONCE_E("core3d_global_descriptor_set_rs_all_issues",
            "Global descriptor set 0/1/2 for default material not "
            "found (RenderNodeDefaultCameraController needed)");
    }
    return fgds;
}

SceneRenderDataStores RenderNodeSceneUtilImpl::GetSceneRenderDataStores(
    const IRenderNodeContextManager& renderNodeContextMgr, const string_view sceneDataStoreName)
{
    return RenderNodeSceneUtil::GetSceneRenderDataStores(renderNodeContextMgr, sceneDataStoreName);
}

BASE_NS::string RenderNodeSceneUtilImpl::GetSceneRenderDataStore(
    const SceneRenderDataStores& sceneRds, const BASE_NS::string_view dsName)
{
    return RenderNodeSceneUtil::GetSceneRenderDataStore(sceneRds, dsName);
}

ViewportDesc RenderNodeSceneUtilImpl::CreateViewportFromCamera(const RenderCamera& camera)
{
    return RenderNodeSceneUtil::CreateViewportFromCamera(camera);
}

ScissorDesc RenderNodeSceneUtilImpl::CreateScissorFromCamera(const RenderCamera& camera)
{
    return RenderNodeSceneUtil::CreateScissorFromCamera(camera);
}

void RenderNodeSceneUtilImpl::UpdateRenderPassFromCamera(const RenderCamera& camera, RenderPass& renderPass)
{
    return RenderNodeSceneUtil::UpdateRenderPassFromCamera(camera, renderPass);
}

void RenderNodeSceneUtilImpl::UpdateRenderPassFromCustomCamera(
    const RenderCamera& camera, const bool isNamedCamera, RenderPass& renderPass)
{
    return RenderNodeSceneUtil::UpdateRenderPassFromCustomCamera(camera, isNamedCamera, renderPass);
}

void RenderNodeSceneUtilImpl::GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t cameraId,
    const IRenderNodeSceneUtil::RenderSlotInfo& renderSlotInfo, vector<SlotSubmeshIndex>& refSubmeshIndices)
{
    return RenderNodeSceneUtil::GetRenderSlotSubmeshes(
        dataStoreCamera, dataStoreMaterial, cameraId, {}, renderSlotInfo, refSubmeshIndices);
}

void RenderNodeSceneUtilImpl::GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t cameraIndex,
    const array_view<const uint32_t> addCameraIndices, const IRenderNodeSceneUtil::RenderSlotInfo& renderSlotInfo,
    vector<SlotSubmeshIndex>& refSubmeshIndices)
{
    return RenderNodeSceneUtil::GetRenderSlotSubmeshes(
        dataStoreCamera, dataStoreMaterial, cameraIndex, addCameraIndices, renderSlotInfo, refSubmeshIndices);
}

SceneBufferHandles RenderNodeSceneUtilImpl::GetSceneBufferHandles(
    IRenderNodeContextManager& renderNodeContextMgr, const string_view sceneName)
{
    return RenderNodeSceneUtil::GetSceneBufferHandles(renderNodeContextMgr, sceneName);
}

SceneCameraBufferHandles RenderNodeSceneUtilImpl::GetSceneCameraBufferHandles(
    IRenderNodeContextManager& renderNodeContextMgr, const string_view sceneName, const string_view cameraName)
{
    return RenderNodeSceneUtil::GetSceneCameraBufferHandles(renderNodeContextMgr, sceneName, cameraName);
}

SceneCameraImageHandles RenderNodeSceneUtilImpl::GetSceneCameraImageHandles(
    RENDER_NS::IRenderNodeContextManager& renderNodeContextMgr, const BASE_NS::string_view sceneName,
    const BASE_NS::string_view cameraName, const RenderCamera& camera)
{
    return RenderNodeSceneUtil::GetSceneCameraImageHandles(renderNodeContextMgr, sceneName, cameraName, camera);
}

SceneRenderCameraData RenderNodeSceneUtilImpl::GetSceneCameraData(const IRenderDataStoreDefaultScene& dataStoreScene,
    const IRenderDataStoreDefaultCamera& dataStoreCamera, const uint64_t cameraId,
    const BASE_NS::string_view cameraName)
{
    return RenderNodeSceneUtil::GetSceneCameraData(dataStoreScene, dataStoreCamera, cameraId, cameraName);
}

const IInterface* RenderNodeSceneUtilImpl::GetInterface(const Uid& uid) const
{
    if ((uid == IRenderNodeSceneUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

IInterface* RenderNodeSceneUtilImpl::GetInterface(const Uid& uid)
{
    if ((uid == IRenderNodeSceneUtil::UID) || (uid == IInterface::UID)) {
        return this;
    }
    return nullptr;
}

void RenderNodeSceneUtilImpl::Ref() {}

void RenderNodeSceneUtilImpl::Unref() {}
CORE3D_END_NAMESPACE()
