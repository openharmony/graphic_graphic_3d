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

#include "render_node_default_cameras.h"

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <base/containers/allocator.h>
#include <base/math/matrix_util.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/util/intf_frustum_util.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>

namespace {
#include "3d/shaders/common/3d_dm_structures_common.h"
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;
using namespace RENDER_NS;

namespace {
constexpr BASE_NS::Math::Mat4X4 ZERO_MATRIX_4X4 = {};
constexpr BASE_NS::Math::Mat4X4 SHADOW_BIAS_MATRIX = BASE_NS::Math::Mat4X4 { 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.5f, 0.5f, 0.0f, 1.0f };
constexpr BASE_NS::Math::Mat4X4 GetShadowBias(const uint32_t shadowIndex, const uint32_t shadowCount)
{
    const float theShadowCount = static_cast<float>(Math::max(1u, shadowCount));
    const float invShadowCount = (1.0f / theShadowCount);
    const float sc = 0.5f * invShadowCount;
    const float so = invShadowCount * static_cast<float>(shadowIndex);
    return BASE_NS::Math::Mat4X4 { sc, 0.0f, 0.0f, 0.0f, 0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, sc + so, 0.5f,
        0.0f, 1.0f };
}

constexpr uint32_t HALTON_SAMPLE_COUNT { 16u };
Math::Vec2 GetHaltonOffset(const uint32_t haltonIndex)
{
    // positive halton
    constexpr const Math::Vec2 halton16[] = {
        { 0.500000f, 0.333333f }, // 00
        { 0.250000f, 0.666667f }, // 01
        { 0.750000f, 0.111111f }, // 02
        { 0.125000f, 0.444444f }, // 03
        { 0.625000f, 0.777778f }, // 04
        { 0.375000f, 0.222222f }, // 05
        { 0.875000f, 0.555556f }, // 06
        { 0.062500f, 0.888889f }, // 07
        { 0.562500f, 0.037037f }, // 08
        { 0.312500f, 0.370370f }, // 09
        { 0.812500f, 0.703704f }, // 10
        { 0.187500f, 0.148148f }, // 11
        { 0.687500f, 0.481481f }, // 12
        { 0.437500f, 0.814815f }, // 13
        { 0.937500f, 0.259259f }, // 14
        { 0.031250f, 0.592593f }, // 15
    };
    return halton16[haltonIndex];
}

inline constexpr Math::UVec2 GetPacked64(const uint64_t value)
{
    return { static_cast<uint32_t>(value >> 32) & 0xFFFFffff, static_cast<uint32_t>(value & 0xFFFFffff) };
}

inline constexpr Math::UVec4 GetMultiViewCameraIndices(
    const IRenderDataStoreDefaultCamera* rds, const RenderCamera& cam)
{
    Math::UVec4 mvIndices { 0U, 0U, 0U, 0U };
    CORE_STATIC_ASSERT(RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT == 3U);
    const uint32_t inputCount = cam.multiViewCameraCount;
    for (uint32_t idx = 0U; idx < inputCount; ++idx) {
        const uint64_t id = cam.multiViewCameraIds[idx];
        if (id != RenderSceneDataConstants::INVALID_ID) {
            mvIndices[0U]++; // recalculates the count
            mvIndices[mvIndices[0U]] = Math::min(rds->GetCameraIndex(id), CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT - 1U);
        }
    }
    return mvIndices;
}
} // namespace

void RenderNodeDefaultCameras::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    // Get IFrustumUtil from global plugin registry.
    frustumUtil_ = GetInstance<IFrustumUtil>(UID_FRUSTUM_UTIL);

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);
    const string bufferName =
        stores_.dataStoreNameScene.c_str() + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME;

    CORE_STATIC_ASSERT((sizeof(DefaultCameraMatrixStruct) % CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) == 0);
    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    resHandle_ = gpuResourceMgr.Create(
        bufferName, { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                        sizeof(DefaultCameraMatrixStruct) * CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT });
    if (resHandle_) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        const RenderHandle handle = resHandle_.GetHandle();
        rngShareMgr.RegisterRenderNodeOutputs({ &handle, 1u });
    }
}

void RenderNodeDefaultCameras::PreExecuteFrame()
{
    if (resHandle_) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        const RenderHandle handle = resHandle_.GetHandle();
        rngShareMgr.RegisterRenderNodeOutputs({ &handle, 1u });
    }
}

void RenderNodeDefaultCameras::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const IRenderDataStoreDefaultCamera* dsCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const IRenderDataStoreDefaultLight* dsLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));
    if (dsCamera) {
        const auto& cameras = dsCamera->GetCameras();
        if (!cameras.empty()) {
            const auto& gpuResMgr = renderNodeContextMgr_->GetGpuResourceManager();
            if (uint8_t* data = reinterpret_cast<uint8_t*>(gpuResMgr.MapBuffer(resHandle_.GetHandle())); data) {
                const uint32_t cameraCount =
                    Math::min(CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT, static_cast<uint32_t>(cameras.size()));

                for (uint32_t idx = 0; idx < cameraCount; ++idx) {
                    const auto& currCamera = cameras[idx];
                    const Math::UVec4 mvCameraIndices = GetMultiViewCameraIndices(dsCamera, currCamera);
                    auto dat =
                        reinterpret_cast<DefaultCameraMatrixStruct*>(data + sizeof(DefaultCameraMatrixStruct) * idx);

                    const JitterProjection jp = GetProjectionMatrix(currCamera, false);
                    const Math::Mat4X4 viewProj = jp.proj * currCamera.matrices.view;
                    dat->view = currCamera.matrices.view;
                    dat->proj = jp.proj;
                    dat->viewProj = viewProj;

                    dat->viewInv = Math::Inverse(currCamera.matrices.view);
                    dat->projInv = Math::Inverse(jp.proj);
                    dat->viewProjInv = Math::Inverse(viewProj);

                    const JitterProjection jpPrevFrame = GetProjectionMatrix(currCamera, true);
                    const Math::Mat4X4 viewProjPrevFrame = jpPrevFrame.proj * currCamera.matrices.viewPrevFrame;
                    dat->viewPrevFrame = currCamera.matrices.viewPrevFrame;
                    dat->projPrevFrame = jpPrevFrame.proj;
                    dat->viewProjPrevFrame = viewProjPrevFrame;

                    // possible shadow matrices
                    const Math::Mat4X4 shadowBias = GetShadowBiasMatrix(dsLight, currCamera);
                    const Math::Mat4X4 shadowViewProj = shadowBias * viewProj;
                    dat->shadowViewProj = shadowViewProj;
                    dat->shadowViewProjInv = Math::Inverse(shadowViewProj);

                    // jitter data
                    dat->jitter = jp.jitter;
                    dat->jitterPrevFrame = jpPrevFrame.jitter;

                    const Math::UVec2 packedId = GetPacked64(currCamera.id);
                    const Math::UVec2 packedLayer = GetPacked64(currCamera.layerMask);
                    dat->indices = { packedId.x, packedId.y, packedLayer.x, packedLayer.y };
                    dat->multiViewIndices = mvCameraIndices;

                    // frustum planes
                    if (frustumUtil_) {
                        // frustum planes created without jitter
                        const Frustum frustum = frustumUtil_->CreateFrustum(jp.baseProj * currCamera.matrices.view);
                        CloneData(dat->frustumPlanes, CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT * sizeof(Math::Vec4),
                            frustum.planes, Frustum::PLANE_COUNT * sizeof(Math::Vec4));
                    }

                    // padding
                    dat->counts = { currCamera.environmentCount, 0U, 0U, 0U };
                    dat->pad0 = { 0U, 0U, 0U, 0U };
                    dat->matPad0 = ZERO_MATRIX_4X4;
                    dat->matPad1 = ZERO_MATRIX_4X4;
                }

                gpuResMgr.UnmapBuffer(resHandle_.GetHandle());
            }
        }
    }

    jitterIndex_ = (jitterIndex_ + 1) % HALTON_SAMPLE_COUNT;
}

RenderNodeDefaultCameras::JitterProjection RenderNodeDefaultCameras::GetProjectionMatrix(
    const RenderCamera& camera, const bool prevFrame) const
{
    JitterProjection jp;
    jp.baseProj = prevFrame ? camera.matrices.projPrevFrame : camera.matrices.proj;
    jp.proj = jp.baseProj;
    if (camera.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_JITTER_BIT) {
        // NOTE: currently calculates the same jitter for both frames to get zero velocity
        const uint32_t jitterIndex = jitterIndex_;
        const Math::Vec2 renderRes = Math::Vec2(static_cast<float>(Math::max(1u, camera.renderResolution.x)),
            static_cast<float>(Math::max(1u, camera.renderResolution.y)));
        const Math::Vec2 haltonOffset = GetHaltonOffset(jitterIndex);
        const Math::Vec2 haltonOffsetRes =
            Math::Vec2((haltonOffset.x * 2.0f - 1.0f) / renderRes.x, (haltonOffset.y * 2.0f - 1.0f) / renderRes.y);

        jp.jitter = Math::Vec4(haltonOffset.x, haltonOffset.y, haltonOffsetRes.x, haltonOffsetRes.y);
        jp.proj[2U][0U] += haltonOffsetRes.x;
        jp.proj[2U][1U] += haltonOffsetRes.y;
    }
    return jp;
}

BASE_NS::Math::Mat4X4 RenderNodeDefaultCameras::GetShadowBiasMatrix(
    const IRenderDataStoreDefaultLight* dataStore, const RenderCamera& camera) const
{
    if (dataStore) {
        const auto lightCounts = dataStore->GetLightCounts();
        const uint32_t shadowCount = lightCounts.shadowCount;
        const auto lights = dataStore->GetLights();
        for (const auto& lightRef : lights) {
            if (lightRef.id == camera.shadowId) {
                return GetShadowBias(lightRef.shadowIndex, shadowCount);
            }
        }
    }
    return SHADOW_BIAS_MATRIX;
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultCameras::Create()
{
    return new RenderNodeDefaultCameras();
}

void RenderNodeDefaultCameras::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultCameras*>(instance);
}
CORE3D_END_NAMESPACE()
