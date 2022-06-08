/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "render_node_default_lights.h"

#include <algorithm>

#include <3d/render/default_material_constants.h>
#include <3d/render/intf_render_data_store_default_camera.h>
#include <3d/render/intf_render_data_store_default_light.h>
#include <3d/render/intf_render_data_store_default_scene.h>
#include <base/math/matrix_util.h>
#include <core/log.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/nodecontext/intf_render_node_context_manager.h>
#include <render/nodecontext/intf_render_node_graph_share_manager.h>

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
// NOTE: temporary, no cluster calculations yet
constexpr uint32_t DEFAULT_CLUSTER_INDEX_COUNT { 256u };
// offset to DefaultMaterialSingleLightStruct is 8*4
constexpr uint32_t LIGHT_LIST_OFFSET { 16u * 4u };

struct LightCounts {
    uint32_t directionalLightCount { 0u };
    uint32_t pointLightCount { 0u };
    uint32_t spotLightCount { 0u };
};

struct SortData {
    RenderLight::LightUsageFlags lightUsageFlags { 0u };
    uint32_t index { 0u };
};

vector<SortData> SortLights(const array_view<const RenderLight> lights, const uint32_t lightCount)
{
    vector<SortData> sortedFlags(lightCount);
    for (uint32_t idx = 0; idx < lightCount; ++idx) {
        sortedFlags[idx].lightUsageFlags = lights[idx].lightUsageFlags;
        sortedFlags[idx].index = idx;
    }
    std::sort(sortedFlags.begin(), sortedFlags.end(),
        [](const auto& lhs, const auto& rhs) { return ((lhs.lightUsageFlags & 0x7u) < (rhs.lightUsageFlags & 0x7u)); });
    return sortedFlags;
}

void CopySingleLight(const RenderCamera& camera, const bool validCamera, const RenderLight& currLight,
    DefaultMaterialSingleLightStruct* memLight)
{
    Math::Vec4 pos = currLight.pos;
    Math::Vec4 dir = currLight.dir;
#ifdef CORE_USE_VIEWSPACE_LIGHTING
    Math::Vec4 viewPos = Math::Vec4(currLight.pos.x, currLight.pos.y, currLight.pos.z, 1.0f);
    Math::Vec4 viewDir = Math::Vec4(currLight.dir.x, currLight.dir.y, currLight.dir.z, 0.0f);
    if (validCamera) {
        viewPos = camera.matrices.view * viewPos;
        viewDir = camera.matrices.view * viewDir;
    }
    pos = viewPos;
    // normalization in shader not needed
    dir = Math::Vec4(Math::Normalize(viewDir), 0.0f);
#endif
    memLight->pos = pos;
    memLight->dir = dir;
    constexpr float epsilonForMinDivisor { 0.0001f };
    memLight->dir.w = std::max(epsilonForMinDivisor, currLight.range);
    memLight->color = Math::Vec4(Math::Vec3(currLight.color) * currLight.color.w, currLight.color.w);
    memLight->spotLightParams = currLight.spotLightParams;
    memLight->shadowFactors = currLight.shadowFactors;
    memLight->flags = { currLight.lightUsageFlags, currLight.shadowCameraIndex, currLight.shadowIndex, 0u };
    memLight->indices = { static_cast<uint32_t>(currLight.id >> 32U), static_cast<uint32_t>(currLight.id & 0xFFFFffff),
        static_cast<uint32_t>(currLight.layerMask >> 32U), static_cast<uint32_t>(currLight.layerMask & 0xFFFFffff) };
}

template<typename DataType>
DataType* MapBuffer(IRenderNodeGpuResourceManager& gpuResourceManager, const RenderHandle handle)
{
    return reinterpret_cast<DataType*>(gpuResourceManager.MapBuffer(handle));
}
} // namespace

void RenderNodeDefaultLights::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    stores_ = RenderNodeSceneUtil::GetSceneRenderDataStores(
        renderNodeContextMgr, renderNodeGraphData.renderNodeGraphDataStoreName);

    const string bufferName =
        stores_.dataStoreNameScene.c_str() + DefaultMaterialLightingConstants::LIGHT_DATA_BUFFER_NAME;
    const string clusterBufferName =
        stores_.dataStoreNameScene.c_str() + DefaultMaterialLightingConstants::LIGHT_CLUSTER_DATA_BUFFER_NAME;

    auto& gpuResourceMgr = renderNodeContextMgr.GetGpuResourceManager();
    lightBufferHandle_ = gpuResourceMgr.Create(
        bufferName, {
                        CORE_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                        (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                        CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                        sizeof(DefaultMaterialLightStruct),
                    });
    lightClusterBufferHandle_ = gpuResourceMgr.Create(
        clusterBufferName, {
                               CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                               (CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT | CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT),
                               CORE_ENGINE_BUFFER_CREATION_DYNAMIC_RING_BUFFER,
                               sizeof(uint32_t) * DEFAULT_CLUSTER_INDEX_COUNT,
                           });
    if (lightBufferHandle_ && lightClusterBufferHandle_) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        const RenderHandle handles[] = { lightBufferHandle_.GetHandle(), lightClusterBufferHandle_.GetHandle() };
        rngShareMgr.RegisterRenderNodeOutputs(handles);
    }
}

void RenderNodeDefaultLights::PreExecuteFrame()
{
    if (lightBufferHandle_) {
        IRenderNodeGraphShareManager& rngShareMgr = renderNodeContextMgr_->GetRenderNodeGraphShareManager();
        const RenderHandle handle = lightBufferHandle_.GetHandle();
        rngShareMgr.RegisterRenderNodeOutputs({ &handle, 1u });
    }
}

void RenderNodeDefaultLights::ExecuteFrame(IRenderCommandList& cmdList)
{
    const auto& renderDataStoreMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    const auto* dataStoreScene =
        static_cast<IRenderDataStoreDefaultScene*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameScene));
    const auto* dataStoreCamera =
        static_cast<IRenderDataStoreDefaultCamera*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameCamera));
    const auto* dataStoreLight =
        static_cast<IRenderDataStoreDefaultLight*>(renderDataStoreMgr.GetRenderDataStore(stores_.dataStoreNameLight));

    if (dataStoreScene && dataStoreLight && dataStoreCamera) {
        auto& gpuResourceMgr = renderNodeContextMgr_->GetGpuResourceManager();
        const auto scene = dataStoreScene->GetScene();
        const uint32_t sceneCameraIdx = scene.cameraIndex;
        const auto& lights = dataStoreLight->GetLights();
        const auto& cameras = dataStoreCamera->GetCameras();
        const bool validCamera = (sceneCameraIdx < static_cast<uint32_t>(cameras.size()));
        // light buffer update (needs to be updated every frame)
        if (auto data = MapBuffer<uint8_t>(gpuResourceMgr, lightBufferHandle_.GetHandle()); data) {
            // NOTE: do not read data from mapped buffer (i.e. do not use mapped buffer as input to anything)
            LightCounts lightCounts;
            const uint32_t lightCount = std::min(CORE_DEFAULT_MATERIAL_MAX_LIGHT_COUNT, (uint32_t)lights.size());
            vector<SortData> sortedFlags = SortLights(lights, lightCount);

            const RenderCamera camera = validCamera ? cameras[sceneCameraIdx] : RenderCamera {};

            auto* singleLightStruct = reinterpret_cast<DefaultMaterialSingleLightStruct*>(data + LIGHT_LIST_OFFSET);
            for (const auto& sortData : sortedFlags) {
                using UsageFlagBits = RenderLight::LightUsageFlagBits;
                if (sortData.lightUsageFlags & UsageFlagBits::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT) {
                    lightCounts.directionalLightCount++;
                } else if (sortData.lightUsageFlags & UsageFlagBits::LIGHT_USAGE_POINT_LIGHT_BIT) {
                    lightCounts.pointLightCount++;
                } else if (sortData.lightUsageFlags & UsageFlagBits::LIGHT_USAGE_SPOT_LIGHT_BIT) {
                    lightCounts.spotLightCount++;
                }

                CopySingleLight(camera, validCamera, lights[sortData.index], singleLightStruct++);
            }

            DefaultMaterialLightStruct* lightStruct = reinterpret_cast<DefaultMaterialLightStruct*>(data);
            lightStruct->directionalLightBeginIndex = 0;
            lightStruct->directionalLightCount = lightCounts.directionalLightCount;
            lightStruct->pointLightBeginIndex = lightCounts.directionalLightCount;
            lightStruct->pointLightCount = lightCounts.pointLightCount;
            lightStruct->spotLightBeginIndex = lightCounts.directionalLightCount + lightCounts.pointLightCount;
            lightStruct->spotLightCount = lightCounts.spotLightCount;
            lightStruct->pad0 = 0;
            lightStruct->pad1 = 0;
            lightStruct->clusterSizes = Math::UVec4(0, 0, 0, 0);
            lightStruct->clusterFactors = Math::Vec4(0.0f, 0.0f, 0.0f, 0.0f);

            gpuResourceMgr.UnmapBuffer(lightBufferHandle_.GetHandle());
        }
    }
}

// for plugin / factory interface
RENDER_NS::IRenderNode* RenderNodeDefaultLights::Create()
{
    return new RenderNodeDefaultLights();
}

void RenderNodeDefaultLights::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultLights*>(instance);
}
CORE3D_END_NAMESPACE()
