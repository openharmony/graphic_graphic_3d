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

// NOTE: do not include in header
#include "render_light_helper.h"

namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
} // namespace

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

namespace {
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
                               sizeof(uint32_t) * RenderLightHelper::DEFAULT_CLUSTER_INDEX_COUNT,
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
        const Math::Vec4 shadowAtlasSizeInvSize = RenderLightHelper::GetShadowAtlasSizeInvSize(*dataStoreLight);
        const uint32_t shadowCount = dataStoreLight->GetLightCounts().shadowCount;
        // light buffer update (needs to be updated every frame)
        if (auto data = MapBuffer<uint8_t>(gpuResourceMgr, lightBufferHandle_.GetHandle()); data) {
            // NOTE: do not read data from mapped buffer (i.e. do not use mapped buffer as input to anything)
            RenderLightHelper::LightCounts lightCounts;
            const uint32_t lightCount = std::min(CORE_DEFAULT_MATERIAL_MAX_LIGHT_COUNT, (uint32_t)lights.size());
            vector<RenderLightHelper::SortData> sortedFlags = RenderLightHelper::SortLights(lights, lightCount);

            const RenderCamera camera = validCamera ? cameras[sceneCameraIdx] : RenderCamera {};

            auto* singleLightStruct =
                reinterpret_cast<DefaultMaterialSingleLightStruct*>(data + RenderLightHelper::LIGHT_LIST_OFFSET);
            for (const auto& sortData : sortedFlags) {
                using UsageFlagBits = RenderLight::LightUsageFlagBits;
                if (sortData.lightUsageFlags & UsageFlagBits::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT) {
                    lightCounts.directionalLightCount++;
                } else if (sortData.lightUsageFlags & UsageFlagBits::LIGHT_USAGE_POINT_LIGHT_BIT) {
                    lightCounts.pointLightCount++;
                } else if (sortData.lightUsageFlags & UsageFlagBits::LIGHT_USAGE_SPOT_LIGHT_BIT) {
                    lightCounts.spotLightCount++;
                }

                RenderLightHelper::CopySingleLight(lights[sortData.index], shadowCount, singleLightStruct++);
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
            lightStruct->atlasSizeInvSize = shadowAtlasSizeInvSize;
            lightStruct->additionalFactors = { 0.0f, 0.0f, 0.0f, 0.0f };

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
