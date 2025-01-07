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
#include <base/math/quaternion_util.h>
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

constexpr float CUBE_MAP_LOD_COEFF { 8.0f };
// nine vectors which are used in spherical harmonics calculations
constexpr BASE_NS::Math::Vec4 DEFAULT_SH_INDIRECT_COEFFICIENTS[9u] {
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 0.0f },
};

constexpr uint32_t CUBEMAP_EXTRA_CAMERA_COUNT { 5U };
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

void GenerateCubemapMatrices(vector<Math::Mat4X4>& matrices)
{
    if (matrices.empty()) {
        matrices.resize(CUBEMAP_EXTRA_CAMERA_COUNT);
        // x-
        matrices[0U] = Mat4Cast(Math::AngleAxis((Math::DEG2RAD * -90.0f), Math::Vec3(0.0f, 1.0f, 0.0f)));
        matrices[0U] = Math::Scale(matrices[0U], { 1.f, 1.f, -1.f });
        // +y
        matrices[1U] = Mat4Cast(Math::AngleAxis((Math::DEG2RAD * -90.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
        matrices[1U] = Math::Scale(matrices[1U], { 1.f, 1.f, -1.f });
        // -y
        matrices[2U] = Mat4Cast(Math::AngleAxis((Math::DEG2RAD * 90.0f), Math::Vec3(1.0f, 0.0f, 0.0f)));
        matrices[2U] = Math::Scale(matrices[2U], { 1.f, 1.f, -1.f });
        // +z
        matrices[3U] = Mat4Cast(Math::AngleAxis((Math::DEG2RAD * 180.0f), Math::Vec3(0.0f, 1.0f, 0.0f)));
        matrices[3U] = Math::Scale(matrices[3U], { -1.f, 1.f, 1.f });
        // -z
        matrices[4U] = Mat4Cast(Math::AngleAxis((Math::DEG2RAD * 0.0f), Math::Vec3(0.0f, 1.0f, 0.0f)));
        matrices[4U] = Math::Scale(matrices[4U], { -1.f, 1.f, 1.f });
    }
}

inline constexpr Math::UVec2 GetPacked64(const uint64_t value)
{
    return { static_cast<uint32_t>(value >> 32) & 0xFFFFffff, static_cast<uint32_t>(value & 0xFFFFffff) };
}

constexpr Math::UVec4 GetMultiViewCameraIndicesFunc(const IRenderDataStoreDefaultCamera* rds, const RenderCamera& cam)
{
    Math::UVec4 mvIndices { 0U, 0U, 0U, 0U };
    CORE_STATIC_ASSERT(RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT == 7U);
    const uint32_t inputCount =
        Math::min(cam.multiViewCameraCount, RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT);
    for (uint32_t idx = 0U; idx < inputCount; ++idx) {
        const uint64_t id = cam.multiViewCameraIds[idx];
        if (id != RenderSceneDataConstants::INVALID_ID) {
            mvIndices[0U]++; // recalculates the count
            const uint32_t index = mvIndices[0U];
            const uint32_t viewIndexShift =
                (index >= CORE_MULTI_VIEW_VIEW_INDEX_MODULO) ? CORE_MULTI_VIEW_VIEW_INDEX_SHIFT : 0U;
            const uint32_t finalViewIndex = index % CORE_MULTI_VIEW_VIEW_INDEX_MODULO;
            mvIndices[finalViewIndex] =
                (Math::min(rds->GetCameraIndex(id), CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT - 1U) &
                    CORE_MULTI_VIEW_VIEW_INDEX_MASK)
                << viewIndexShift;
        }
    }
    return mvIndices;
}

constexpr Math::UVec4 GetCubemapMultiViewCameraIndicesFunc(
    const IRenderDataStoreDefaultCamera* rds, const RenderCamera& cam, const array_view<const uint32_t> cameraIndices)
{
    Math::UVec4 mvIndices { 0U, 0U, 0U, 0U };
    CORE_STATIC_ASSERT(RenderSceneDataConstants::MAX_MULTI_VIEW_LAYER_CAMERA_COUNT == 7U);
    constexpr uint32_t inputCount = CUBEMAP_EXTRA_CAMERA_COUNT;
    mvIndices[0U] = inputCount; // multi-view camera count
    // NOTE: keeps compatibility with the old code
    for (uint32_t idx = 0U; idx < inputCount; ++idx) {
        const uint32_t writeIndex = idx + 1U;
        const uint32_t viewIndexShift =
            (writeIndex >= CORE_MULTI_VIEW_VIEW_INDEX_MODULO) ? CORE_MULTI_VIEW_VIEW_INDEX_SHIFT : 0U;
        const uint32_t finalViewIndex = writeIndex % CORE_MULTI_VIEW_VIEW_INDEX_MODULO;
        const uint32_t camId = cameraIndices[idx];
        mvIndices[finalViewIndex] |=
            (Math::min(camId, CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT - 1U) & CORE_MULTI_VIEW_VIEW_INDEX_MASK)
            << viewIndexShift;
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

    CORE_STATIC_ASSERT((sizeof(DefaultCameraMatrixStruct) % CORE_MIN_UNIFORM_BUFFER_OFFSET_ALIGNMENT) == 0);
    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    {
        const string bufferName =
            stores_.dataStoreNameScene.c_str() + DefaultMaterialCameraConstants::CAMERA_DATA_BUFFER_NAME;
        camHandle_ = gpuResourceMgr.Create(
            bufferName, { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                            sizeof(DefaultCameraMatrixStruct) * CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT });
    }
    {
        const string bufferName =
            stores_.dataStoreNameScene.c_str() + DefaultMaterialSceneConstants::SCENE_ENVIRONMENT_DATA_BUFFER_NAME;
        envHandle_ = gpuResourceMgr.Create(
            bufferName, { CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                            (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                            CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                            sizeof(DefaultMaterialEnvironmentStruct) * CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT });
    }

    IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
    const RenderHandle outputs[2U] { camHandle_.GetHandle(), envHandle_.GetHandle() };
    rngShareMgr.RegisterRenderNodeOutputs(outputs);
}

void RenderNodeDefaultCameras::PreExecuteFrame()
{
    IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
    const RenderHandle outputs[2U] { camHandle_.GetHandle(), envHandle_.GetHandle() };
    rngShareMgr.RegisterRenderNodeOutputs(outputs);
    // reset
    cubemapCameras_.clear();
}

void RenderNodeDefaultCameras::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const IRenderDataStoreDefaultCamera* dsCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const IRenderDataStoreDefaultLight* dsLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));
    if (!dsCamera) {
        return;
    }
    const auto& cameras = dsCamera->GetCameras();
    const auto& environments = dsCamera->GetEnvironments();
    if (cameras.empty() && environments.empty()) {
        return;
    }
    const auto& gpuResMgr = renderNodeContextMgr_->GetGpuResourceManager();
    if (uint8_t* data = reinterpret_cast<uint8_t*>(gpuResMgr.MapBuffer(camHandle_.GetHandle())); data) {
        const uint32_t originalCameraCount = static_cast<uint32_t>(cameras.size());
        // add normal cameras to GPU
        AddCameras(dsCamera, dsLight, cameras, data, 0U);
        // add cubemap cameras to GPU
        AddCameras(dsCamera, dsLight, cubemapCameras_, data, originalCameraCount);

        gpuResMgr.UnmapBuffer(camHandle_.GetHandle());
    }
    jitterIndex_ = (jitterIndex_ + 1) % HALTON_SAMPLE_COUNT;

    if (uint8_t* data = reinterpret_cast<uint8_t*>(gpuResMgr.MapBuffer(envHandle_.GetHandle())); data) {
        AddEnvironments(dsCamera, environments, data);

        gpuResMgr.UnmapBuffer(envHandle_.GetHandle());
    }
}

void RenderNodeDefaultCameras::AddCameras(const IRenderDataStoreDefaultCamera* dsCamera,
    const IRenderDataStoreDefaultLight* dsLight, const array_view<const RenderCamera> cameras, uint8_t* const data,
    const uint32_t cameraOffset)
{
    const uint32_t cameraCount = static_cast<uint32_t>(
        Math::max(0, Math::min(static_cast<int32_t>(CORE_DEFAULT_MATERIAL_MAX_CAMERA_COUNT - cameraOffset),
            static_cast<int32_t>(cameras.size()))));
    for (uint32_t idx = 0; idx < cameraCount; ++idx) {
        const auto& currCamera = cameras[idx];
        // take into account the offset to GPU cameras
        auto dat = reinterpret_cast<DefaultCameraMatrixStruct* const>(
            data + sizeof(DefaultCameraMatrixStruct) * (idx + cameraOffset));

        const auto view = ResolveViewMatrix(currCamera);
        const JitterProjection jp = GetProjectionMatrix(currCamera, false);
        const Math::Mat4X4 viewProj = jp.proj * view;
        dat->view = view;
        dat->proj = jp.proj;
        dat->viewProj = viewProj;

        dat->viewInv = Math::Inverse(view);
        dat->projInv = Math::Inverse(jp.proj);
        dat->viewProjInv = Math::Inverse(viewProj);

        const JitterProjection jpPrevFrame = GetProjectionMatrix(currCamera, true);
        const Math::Mat4X4 viewProjPrevFrame = jpPrevFrame.proj * currCamera.matrices.viewPrevFrame;
        dat->viewPrevFrame = currCamera.matrices.viewPrevFrame;
        dat->projPrevFrame = jpPrevFrame.proj;
        dat->viewProjPrevFrame = viewProjPrevFrame;

        // possible shadow matrices
        const Math::Mat4X4 shadowViewProj = GetShadowBiasMatrix(dsLight, currCamera) * viewProj;
        dat->shadowViewProj = shadowViewProj;
        dat->shadowViewProjInv = Math::Inverse(shadowViewProj);

        // jitter data
        dat->jitter = jp.jitter;
        dat->jitterPrevFrame = jpPrevFrame.jitter;

        const Math::UVec2 packedId = GetPacked64(currCamera.id);
        const Math::UVec2 packedLayer = GetPacked64(currCamera.layerMask);
        dat->indices = { packedId.x, packedId.y, packedLayer.x, packedLayer.y };
        dat->multiViewIndices = GetMultiViewCameraIndices(dsCamera, currCamera, cameraCount);

        // frustum planes
        if (frustumUtil_) {
            // frustum planes created without jitter
            const Frustum frustum = frustumUtil_->CreateFrustum(jp.baseProj * view);
            CloneData(dat->frustumPlanes, CORE_DEFAULT_CAMERA_FRUSTUM_PLANE_COUNT * sizeof(Math::Vec4), frustum.planes,
                Frustum::PLANE_COUNT * sizeof(Math::Vec4));
        }

        // padding
        dat->counts = { currCamera.environment.multiEnvCount, 0U, 0U, 0U };
        dat->pad0 = { 0U, 0U, 0U, 0U };
        dat->matPad0 = ZERO_MATRIX_4X4;
        dat->matPad1 = ZERO_MATRIX_4X4;
    }
}

BASE_NS::Math::UVec4 RenderNodeDefaultCameras::GetMultiViewCameraIndices(
    const IRenderDataStoreDefaultCamera* rds, const RenderCamera& cam, const uint32_t cameraCount)
{
    // first check if cubemap
    if (cam.flags & RenderCamera::CameraFlagBits::CAMERA_FLAG_CUBEMAP_BIT) {
        // generate new indices and add cameras
        const size_t currSize = cubemapCameras_.size();
        cubemapCameras_.resize(currSize + CUBEMAP_EXTRA_CAMERA_COUNT);
        GenerateCubemapMatrices(cubemapMatrices_);

        const uint32_t startCameraIndex = cameraCount + static_cast<uint32_t>(currSize);
        const uint32_t camIndices[CUBEMAP_EXTRA_CAMERA_COUNT] = {
            startCameraIndex + 0U,
            startCameraIndex + 1U,
            startCameraIndex + 2U,
            startCameraIndex + 3U,
            startCameraIndex + 4U,
        };
        for (size_t idx = 0; idx < CUBEMAP_EXTRA_CAMERA_COUNT; ++idx) {
            CORE_ASSERT(currSize + idx < cubemapCameras_.size());
            CORE_ASSERT(idx < cubemapMatrices_.size());
            auto& currCamera = cubemapCameras_[currSize + idx];
            currCamera = cam;
            currCamera.flags = 0U;
            currCamera.matrices.view = cubemapMatrices_[idx] * currCamera.matrices.view;
            currCamera.matrices.viewPrevFrame = currCamera.matrices.view;
        }
        return GetCubemapMultiViewCameraIndicesFunc(rds, cam, { camIndices, CUBEMAP_EXTRA_CAMERA_COUNT });
    } else {
        return GetMultiViewCameraIndicesFunc(rds, cam);
    }
}

BASE_NS::Math::Mat4X4 RenderNodeDefaultCameras::ResolveViewMatrix(const RenderCamera& camera) const
{
    auto view = camera.matrices.view;
    if (camera.flags & RenderCamera::CAMERA_FLAG_CUBEMAP_BIT) {
        Math::Mat4X4 temporary = Mat4Cast(Math::AngleAxis((Math::DEG2RAD * 90.0f), Math::Vec3(0.0f, 1.0f, 0.0f)));
        temporary = Math::Scale(temporary, { 1.f, 1.f, -1.f });
        view = temporary * view;
    }
    return view;
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

namespace {
Math::UVec4 GetMultiEnvironmentIndices(
    const RenderCamera::Environment& env, const array_view<const RenderCamera::Environment> envs)
{
    if (env.multiEnvCount > 0U) {
        Math::UVec4 multiEnvIndices = { 0U, 0U, 0U, 0U };
        // the first value in multiEnvIndices is the count
        // first index is the main environment, next indices are the blend environments
        const uint32_t maxEnvCount = Math::min(env.multiEnvCount, 3U);
        for (uint32_t idx = 0U; idx < maxEnvCount; ++idx) {
            multiEnvIndices[0U]++;
            uint32_t multiEnvIdx = 0U;
            for (uint32_t envIdx = 0U; envIdx < static_cast<uint32_t>(envs.size()); ++envIdx) {
                const auto& envRef = envs[envIdx];
                if (envRef.id == env.multiEnvIds[idx]) {
                    multiEnvIdx = envIdx;
                }
            }
            CORE_ASSERT(idx + 1U <= 3U);
            multiEnvIndices[idx + 1U] = multiEnvIdx;
        }
        return multiEnvIndices;
    } else {
        return { 0U, 0U, 0U, 0U };
    }
}
} // namespace

void RenderNodeDefaultCameras::AddEnvironments(const IRenderDataStoreDefaultCamera* dsCamera,
    const array_view<const RenderCamera::Environment> environments, uint8_t* const data)
{
    CORE_ASSERT(data);
    const auto* dataEnd = data + sizeof(DefaultMaterialEnvironmentStruct) * CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT;

    const uint32_t envCount = static_cast<uint32_t>(
        Math::min(CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT, static_cast<uint32_t>(environments.size())));
    for (uint32_t idx = 0; idx < envCount; ++idx) {
        auto* dat = data + (sizeof(DefaultMaterialEnvironmentStruct) * idx);

        const auto& currEnv = environments[idx];
        const Math::UVec4 multiEnvIndices = GetMultiEnvironmentIndices(currEnv, environments);
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
        };
        constexpr size_t countOfSh = countof(envStruct.shIndirectCoefficients);
        if (currEnv.radianceCubemap || (currEnv.multiEnvCount > 0U)) {
            for (size_t jdx = 0; jdx < countOfSh; ++jdx) {
                envStruct.shIndirectCoefficients[jdx] = currEnv.shIndirectCoefficients[jdx];
            }
        } else {
            for (size_t jdx = 0; jdx < countOfSh; ++jdx) {
                envStruct.shIndirectCoefficients[jdx] = DEFAULT_SH_INDIRECT_COEFFICIENTS[jdx];
            }
        }

        if (!CloneData(dat, size_t(dataEnd - dat), &envStruct, sizeof(DefaultMaterialEnvironmentStruct))) {
            CORE_LOG_E("environment ubo copying failed.");
        }
    }
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
