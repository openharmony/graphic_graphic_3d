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
#include <render/device/pipeline_state_desc.h>
#include <render/nodecontext/intf_render_node.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_util.h>

#include "render/datastore/render_data_store_default_material.h"

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
inline bool operator<(const SlotSubmeshIndex& lhs, const SlotSubmeshIndex& rhs)
{
    if (lhs.sortLayerKey <= rhs.sortLayerKey) {
        return (lhs.sortKey < rhs.sortKey);
    } else {
        return false;
    }
}

inline bool operator>(const SlotSubmeshIndex& lhs, const SlotSubmeshIndex& rhs)
{
    if (lhs.sortLayerKey >= rhs.sortLayerKey) {
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

void UpdateCustomCameraClears(const RenderCamera& camera, RenderPass& renderPass)
{
    auto& subpassDesc = renderPass.subpassDesc;
    if ((subpassDesc.depthAttachmentCount == 1) &&
        (subpassDesc.depthAttachmentIndex < PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT)) {
        auto& attRef = renderPass.renderPassDesc.attachments[subpassDesc.depthAttachmentIndex];
        if ((camera.flags & RenderCamera::CAMERA_FLAG_CLEAR_DEPTH_BIT)) {
            attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            attRef.clearValue.depthStencil = camera.clearDepthStencil;
        } else { // no clear requested
            attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
    }
    // NOTE: clear/handles only the first target ATM, keep MSAA as the first color attachment
    if ((subpassDesc.colorAttachmentCount > 0) &&
        subpassDesc.colorAttachmentIndices[0] < PipelineStateConstants::MAX_RENDER_PASS_ATTACHMENT_COUNT) {
        auto& attRef = renderPass.renderPassDesc.attachments[subpassDesc.colorAttachmentIndices[0u]];
        if (camera.flags & RenderCamera::CAMERA_FLAG_CLEAR_COLOR_BIT) {
            attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_CLEAR;
            attRef.clearValue.color = camera.clearColorValues;
        } else { // no clear requested
            attRef.loadOp = AttachmentLoadOp::CORE_ATTACHMENT_LOAD_OP_DONT_CARE;
        }
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
    }
    return stores;
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
    if (camera.targetType == RenderCamera::CameraTargetType::TARGET_TYPE_CUSTOM_TARGETS) {
        UpdateCustomCameraTargets(camera, renderPass);
    }
    // NOTE: UpdateCustomCameraClears(camera, renderPass) is not yet called
    UpdateRenderArea(camera.scissor, renderPass.renderPassDesc.renderArea);
}

void RenderNodeSceneUtil::UpdateRenderPassFromCustomCamera(
    const RenderCamera& camera, const bool isNamedCamera, RenderPass& renderPass)
{
    renderPass.renderPassDesc.renderArea = { 0, 0, camera.renderResolution.x, camera.renderResolution.y };
    // NOTE: legacy support is only taken into account when isNamedCamera flag is true
    if ((camera.targetType == RenderCamera::CameraTargetType::TARGET_TYPE_CUSTOM_TARGETS) && isNamedCamera) {
        UpdateCustomCameraTargets(camera, renderPass);
    }
    UpdateCustomCameraClears(camera, renderPass);
    UpdateRenderArea(camera.scissor, renderPass.renderPassDesc.renderArea);
}

void RenderNodeSceneUtil::GetRenderSlotSubmeshes(const IRenderDataStoreDefaultCamera& dataStoreCamera,
    const IRenderDataStoreDefaultMaterial& dataStoreMaterial, const uint32_t cameraId,
    const IRenderNodeSceneUtil::RenderSlotInfo& renderSlotInfo, vector<SlotSubmeshIndex>& refSubmeshIndices)
{
    // Get IFrustumUtil from global plugin registry.
    auto frustumUtil = GetInstance<IFrustumUtil>(UID_FRUSTUM_UTIL);

    // 64 bit sorting key should have
    // material hash with:
    // shader, materialIndex, meshId, and depth

    Math::Vec3 camWorldPos { 0.0f, 0.0f, 0.0f };
    uint64_t camLayerMask { RenderSceneDataConstants::INVALID_ID };
    Frustum camFrustum;
    if (const auto& cameras = dataStoreCamera.GetCameras(); cameraId < static_cast<uint32_t>(cameras.size())) {
        const Math::Mat4X4 viewInv = Math::Inverse(cameras[cameraId].matrices.view);
        camWorldPos = Math::Vec3(viewInv[3u]); // take world position from matrix
        camLayerMask = cameras[cameraId].layerMask;
        if (renderSlotInfo.cullType == RenderSlotCullType::VIEW_FRUSTUM_CULL) {
            camFrustum = frustumUtil->CreateFrustum(cameras[cameraId].matrices.proj * cameras[cameraId].matrices.view);
        }
    }

    constexpr uint64_t maxUDepth = RenderDataStoreDefaultMaterial::SLOT_SORT_MAX_DEPTH;
    constexpr uint64_t sDepthShift = RenderDataStoreDefaultMaterial::SLOT_SORT_DEPTH_SHIFT;
    constexpr uint64_t sRenderMask = RenderDataStoreDefaultMaterial::SLOT_SORT_HASH_MASK;
    constexpr uint64_t sRenderShift = RenderDataStoreDefaultMaterial::SLOT_SORT_HASH_SHIFT;
    // NOTE: might need to change to log value or similar
    constexpr float depthUintCoeff { 1000.0f }; // one centimeter is one uint step

    // NOTE: material sort should be based on PSO not shader handle
    const auto& slotSubmeshIndices = dataStoreMaterial.GetSlotSubmeshIndices(renderSlotInfo.id);
    const auto& slotSubmeshMatData = dataStoreMaterial.GetSlotSubmeshMaterialData(renderSlotInfo.id);
    const auto& submeshes = dataStoreMaterial.GetSubmeshes();

    refSubmeshIndices.clear();
    refSubmeshIndices.reserve(slotSubmeshIndices.size());
    for (size_t idx = 0; idx < slotSubmeshIndices.size(); ++idx) {
        const uint32_t submeshIndex = slotSubmeshIndices[idx];
        const auto& submesh = submeshes[submeshIndex];
        const bool notCulled =
            ((renderSlotInfo.cullType != RenderSlotCullType::VIEW_FRUSTUM_CULL) ||
                frustumUtil->SphereFrustumCollision(camFrustum, submesh.worldCenter, submesh.worldRadius));
        const auto& submeshMatData = slotSubmeshMatData[idx];
        const bool discardedMat = (submeshMatData.renderMaterialFlags & renderSlotInfo.materialDiscardFlags);
        if ((camLayerMask & submesh.layerMask) && notCulled && (!discardedMat)) {
            const float distSq = Math::Distance2(submesh.worldCenter, camWorldPos);
            uint64_t sortKey = Math::min(maxUDepth, static_cast<uint64_t>(distSq * depthUintCoeff));
            if (renderSlotInfo.sortType == RenderSlotSortType::BY_MATERIAL) {
                sortKey |= (((uint64_t)submeshMatData.renderSortHash & sRenderMask) << sRenderShift);
            } else {
                sortKey = (sortKey << sDepthShift) | ((uint64_t)slotSubmeshMatData[idx].renderSortHash & sRenderMask);
            }
            refSubmeshIndices.emplace_back(SlotSubmeshIndex { (uint32_t)submeshIndex,
                submeshMatData.combinedRenderSortLayer, sortKey, submeshMatData.renderSortHash,
                submeshMatData.shader.GetHandle(), submeshMatData.gfxState.GetHandle() });
        }
    }

    // front-to-back and by-material render layer sort is 0 -> 63
    // back-to-front render layer sort is 63 -> 0
    if (renderSlotInfo.sortType == RenderSlotSortType::FRONT_TO_BACK ||
        renderSlotInfo.sortType == RenderSlotSortType::BY_MATERIAL) {
        std::sort(refSubmeshIndices.begin(), refSubmeshIndices.end(), Less<SlotSubmeshIndex>());
    } else if (renderSlotInfo.sortType == RenderSlotSortType::BACK_TO_FRONT) {
        std::sort(refSubmeshIndices.begin(), refSubmeshIndices.end(), Greater<SlotSubmeshIndex>());
    }
}

SceneRenderDataStores RenderNodeSceneUtilImpl::GetSceneRenderDataStores(
    const IRenderNodeContextManager& renderNodeContextMgr, const string_view sceneDataStoreName)
{
    return RenderNodeSceneUtil::GetSceneRenderDataStores(renderNodeContextMgr, sceneDataStoreName);
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
        dataStoreCamera, dataStoreMaterial, cameraId, renderSlotInfo, refSubmeshIndices);
}

const IInterface* RenderNodeSceneUtilImpl::GetInterface(const Uid& uid) const
{
    if (uid == IRenderNodeSceneUtil::UID) {
        return this;
    }
    return nullptr;
}

IInterface* RenderNodeSceneUtilImpl::GetInterface(const Uid& uid)
{
    if (uid == IRenderNodeSceneUtil::UID) {
        return this;
    }
    return nullptr;
}

void RenderNodeSceneUtilImpl::Ref() {}

void RenderNodeSceneUtilImpl::Unref() {}
CORE3D_END_NAMESPACE()
