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

#include "render_node_default_acceleration_structure_staging.h"

#include <cinttypes>

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/namespace.h>
#include <render/nodecontext/intf_node_context_pso_manager.h>
#include <render/nodecontext/intf_render_command_list.h>
#include <render/nodecontext/intf_render_node_context_manager.h>

#include "datastore/render_data_store_default_acceleration_structure_staging.h"

#if RENDER_HAS_VULKAN_BACKEND
#include <vulkan/vulkan_core.h>

#include "device/gpu_resource_manager.h"
#include "vulkan/gpu_buffer_vk.h"
#endif

#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
namespace {} // namespace

void RenderNodeDefaultAccelerationStructureStaging::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    dsName_ = renderNodeGraphData.renderNodeGraphName + "RenderDataStoreDefaultAccelerationStructureStaging";
}

void RenderNodeDefaultAccelerationStructureStaging::PreExecuteFrame()
{
    // re-create needed gpu resources
}

void RenderNodeDefaultAccelerationStructureStaging::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderBackend !=
        DeviceBackendType::VULKAN) {
        return;
    }
#if (RENDER_VULKAN_RT_ENABLED == 1)
    const IRenderNodeRenderDataStoreManager& rdsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    if (RenderDataStoreDefaultAccelerationStructureStaging* ds =
            static_cast<RenderDataStoreDefaultAccelerationStructureStaging*>(rdsMgr.GetRenderDataStore(dsName_));
        ds) {
        if (ds->HasStagingData()) {
            // order does not matter ATM (the former is deferred to device the latter is coherently copied)
            AccelerationStructureBuildConsumeStruct stagingBuildData = ds->ConsumeStagingBuildData();
            const auto& triangles = stagingBuildData.triangles;
            const auto& aabbs = stagingBuildData.aabbs;
            const auto& instances = stagingBuildData.instances;
            for (const auto geomRef : stagingBuildData.geometry) {
                const uint32_t startIndex = geomRef.startIndex;
                const uint32_t count = geomRef.count;
                PLUGIN_ASSERT(count <= 1);
                AccelerationStructureBuildGeometryData geometry { { geomRef.data.info },
                    geomRef.data.srcAccelerationStructure.GetHandle(),
                    geomRef.data.dstAccelerationStructure.GetHandle(),
                    { geomRef.data.scratchBuffer.handle.GetHandle(), geomRef.data.scratchBuffer.offset } };
                if ((geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_TRIANGLES) &&
                    (startIndex + count <= static_cast<uint32_t>(triangles.size()))) {
                    const auto& triRef = triangles[startIndex];
                    AccelerationStructureGeometryTrianglesData triData { triRef.info,
                        { triRef.vertexData.handle.GetHandle(), triRef.vertexData.offset },
                        { triRef.indexData.handle.GetHandle(), triRef.indexData.offset },
                        { triRef.transformData.handle.GetHandle(), triRef.transformData.offset } };
                    cmdList.BuildAccelerationStructures(move(geometry), { &triData, 1u }, {}, {});
                } else if (geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_AABBS &&
                           (startIndex + count <= static_cast<uint32_t>(aabbs.size()))) {
                    const auto& aabbRef = aabbs[startIndex];
                    AccelerationStructureGeometryAabbsData aabbData { aabbRef.info,
                        { aabbRef.data.handle.GetHandle(), aabbRef.data.offset } };
                    cmdList.BuildAccelerationStructures(move(geometry), {}, { &aabbData, count }, {});
                } else if ((geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_INSTANCES) &&
                           (startIndex + count <= static_cast<uint32_t>(instances.size()))) {
                    const auto& instanceRef = instances[startIndex];
                    AccelerationStructureGeometryInstancesData instanceData { instanceRef.info,
                        { instanceRef.data.handle.GetHandle(), instanceRef.data.offset } };
                    cmdList.BuildAccelerationStructures(move(geometry), {}, {}, { &instanceData, count });
                }
            }
            ExecuteFrameProcessInstanceData(ds);
        }
    }
#endif
}

void RenderNodeDefaultAccelerationStructureStaging::ExecuteFrameProcessInstanceData(
    RenderDataStoreDefaultAccelerationStructureStaging* dataStore)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
#if (RENDER_HAS_VULKAN_BACKEND == 1)
    if (dataStore) {
        AccelerationStructureInstanceConsumeStruct stagingInstanceData = dataStore->ConsumeStagingInstanceData();
        auto& gpuResourceMgr =
            static_cast<RenderNodeGpuResourceManager&>(renderNodeContextMgr_->GetGpuResourceManager());
        const auto& gpuResourceMgrImpl = static_cast<const GpuResourceManager&>(gpuResourceMgr.GetGpuResourceManager());
        for (const auto dataRef : stagingInstanceData.copyInfo) {
            if (dataRef.bufferOffset.handle && (dataRef.count > 0)) {
                const RenderHandle dstHandle = dataRef.bufferOffset.handle.GetHandle();
                const GpuBufferDesc dstBufferDesc = gpuResourceMgr.GetBufferDescriptor(dstHandle);
                if (uint8_t* dstDataBegin = static_cast<uint8_t*>(gpuResourceMgr.MapBuffer(dstHandle)); dstDataBegin) {
                    const uint8_t* dstDataEnd = dstDataBegin + dstBufferDesc.byteSize;
                    // loop and copy all instances
                    for (uint32_t idx = 0; idx < dataRef.count; ++idx) {
                        const auto& instanceRef = stagingInstanceData.instances[dataRef.startIndex + idx];
                        uint64_t accelerationStructureReference = 0;
                        if (const GpuBufferVk* accelPtr = gpuResourceMgrImpl.GetBuffer<GpuBufferVk>(
                                instanceRef.accelerationStructureReference.GetHandle());
                            accelPtr) {
                            accelerationStructureReference =
                                accelPtr->GetPlatformDataAccelerationStructure().deviceAddress;
                        }
                        VkTransformMatrixKHR transformMatrix = { 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f };

                        VkAccelerationStructureInstanceKHR instance {
                            transformMatrix,                 // transform;
                            instanceRef.instanceCustomIndex, // instanceCustomIndex : 24
                            instanceRef.mask,                // mask : 8
                            0,                               // instanceShaderBindingTableRecordOffset : 24
                            VkGeometryInstanceFlagsKHR(instanceRef.flags), // flags : 8
                            accelerationStructureReference,                // accelerationStructureReference
                        };
                        const size_t byteSize = sizeof(VkAccelerationStructureInstanceKHR);
                        uint8_t* dstData = dstDataBegin + byteSize * idx;
                        CloneData(dstData, size_t(dstDataEnd - dstData), &instance, byteSize);
                    }

                    gpuResourceMgr.UnmapBuffer(dstHandle);
                } else {
                    PLUGIN_LOG_E("accel staging: dstHandle %" PRIu64, dstHandle.id);
                }
            }
        }
    }
#endif
#endif
}

// for plugin / factory interface
IRenderNode* RenderNodeDefaultAccelerationStructureStaging::Create()
{
    return new RenderNodeDefaultAccelerationStructureStaging();
}

void RenderNodeDefaultAccelerationStructureStaging::Destroy(IRenderNode* instance)
{
    delete static_cast<RenderNodeDefaultAccelerationStructureStaging*>(instance);
}
RENDER_END_NAMESPACE()
