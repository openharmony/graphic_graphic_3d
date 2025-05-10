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

#include "render_node_default_acceleration_structure_staging.h"

#include <cinttypes>

#include <render/datastore/intf_render_data_store_manager.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
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
namespace {
constexpr uint32_t Align(uint32_t value, uint32_t align)
{
    if (value == 0) {
        return 0;
    }
    return ((value + align) / align) * align;
}

vector<AsGeometryTrianglesData> ConvertAsGeometryTrianglesData(
    const array_view<const AsGeometryTrianglesDataWithHandleReference> input)
{
    vector<AsGeometryTrianglesData> output;
    output.reserve(input.size());
    for (const auto& ref : input) {
        output.push_back(
            AsGeometryTrianglesData { ref.info, { ref.vertexData.handle.GetHandle(), ref.vertexData.offset },
                { ref.indexData.handle.GetHandle(), ref.indexData.offset },
                { ref.transformData.handle.GetHandle(), ref.transformData.offset } });
    }
    return output;
}

vector<AsGeometryAabbsData> ConvertAsGeometryAabbsData(
    const array_view<const AsGeometryAabbsDataWithHandleReference> input)
{
    vector<AsGeometryAabbsData> output;
    output.reserve(input.size());
    for (const auto& ref : input) {
        output.push_back(AsGeometryAabbsData { ref.info, { ref.data.handle.GetHandle(), ref.data.offset } });
    }
    return output;
}

vector<AsGeometryInstancesData> ConvertAsGeometryInstancesData(
    const array_view<const AsGeometryInstancesDataWithHandleReference> input)
{
    vector<AsGeometryInstancesData> output;
    output.reserve(input.size());
    for (const auto& ref : input) {
        output.push_back(AsGeometryInstancesData { ref.info, { ref.data.handle.GetHandle(), ref.data.offset } });
    }
    return output;
}
} // namespace

void RenderNodeDefaultAccelerationStructureStaging::InitNode(IRenderNodeContextManager& renderNodeContextMgr)
{
    renderNodeContextMgr_ = &renderNodeContextMgr;

    const auto& renderNodeGraphData = renderNodeContextMgr_->GetRenderNodeGraphData();
    dsName_ = renderNodeGraphData.renderNodeGraphName + "RenderDataStoreDefaultAccelerationStructureStaging";
}

void RenderNodeDefaultAccelerationStructureStaging::PreExecuteFrame()
{
    scratchOffsetHelper_.clear();
    frameScratchOffsetIndex_ = 0U;
}

void RenderNodeDefaultAccelerationStructureStaging::ExecuteFrame(IRenderCommandList& cmdList)
{
    if (renderNodeContextMgr_->GetRenderNodeGraphData().renderingConfiguration.renderBackend !=
        DeviceBackendType::VULKAN) {
        return;
    }
#if (RENDER_VULKAN_RT_ENABLED == 1)
    const IRenderNodeRenderDataStoreManager& rdsMgr = renderNodeContextMgr_->GetRenderDataStoreManager();
    RenderDataStoreDefaultAccelerationStructureStaging* ds =
        static_cast<RenderDataStoreDefaultAccelerationStructureStaging*>(rdsMgr.GetRenderDataStore(dsName_));
    if (!ds) {
        return;
    }
    if (ds->HasStagingData()) {
        AsConsumeStruct stagingData = ds->ConsumeStagingData();

        ExecuteFrameProcessScratch(stagingData);

        for (const auto& ref : stagingData.data) {
            if (ref.operationType == AsConsumeStruct::OperationType::INSTANCE_COPY_OP) {
                ExecuteFrameProcessInstanceData(cmdList, stagingData, ref);
            } else if (ref.operationType == AsConsumeStruct::OperationType::BUILD_OP) {
                ExecuteFrameProcessGeometryData(cmdList, stagingData, ref);
            }
        }
    }
#endif
}

void RenderNodeDefaultAccelerationStructureStaging::ExecuteFrameProcessGeometryData(
    IRenderCommandList& cmdList, const AsConsumeStruct& fullData, const AsConsumeStruct::AsData& data)
{
    PLUGIN_ASSERT(data.operationType == AsConsumeStruct::OperationType::BUILD_OP);
    if (data.index < fullData.geometry.size()) {
        const auto& geomRef = fullData.geometry[data.index];
        const auto& triangles = fullData.geomTriangles;
        const auto& aabbs = fullData.geomAabbs;
        const auto& instances = fullData.geomInstances;

        const uint32_t startIndex = geomRef.startIndex;
        const uint32_t count = geomRef.count;
        PLUGIN_ASSERT(frameScratchOffsetIndex_ < static_cast<uint32_t>(scratchOffsetHelper_.size()));
        const BufferOffset bufferOffset { rawScratchBuffer_, scratchOffsetHelper_[frameScratchOffsetIndex_] };
        frameScratchOffsetIndex_++; // advance
        AsBuildGeometryData geometry { { geomRef.data.info }, geomRef.data.srcAccelerationStructure.GetHandle(),
            geomRef.data.dstAccelerationStructure.GetHandle(), bufferOffset };
        if ((geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_TRIANGLES) &&
            (startIndex + count <= static_cast<uint32_t>(triangles.size()))) {
            const auto& ref = triangles[startIndex];
            const vector<AsGeometryTrianglesData> info = ConvertAsGeometryTrianglesData({ &ref, count });
            cmdList.BuildAccelerationStructures(geometry, info, {}, {});
        } else if (geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_AABBS &&
                   (startIndex + count <= static_cast<uint32_t>(aabbs.size()))) {
            const auto& ref = aabbs[startIndex];
            const vector<AsGeometryAabbsData> info = ConvertAsGeometryAabbsData({ &ref, count });
            cmdList.BuildAccelerationStructures(geometry, {}, info, {});
        } else if ((geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_INSTANCES) &&
                   (startIndex + count <= static_cast<uint32_t>(instances.size()))) {
            const auto& ref = instances[startIndex];
            const vector<AsGeometryInstancesData> info = ConvertAsGeometryInstancesData({ &ref, count });
            cmdList.BuildAccelerationStructures(geometry, {}, {}, info);
        }
    }
}

void RenderNodeDefaultAccelerationStructureStaging::ExecuteFrameProcessInstanceData(
    IRenderCommandList& cmdList, const AsConsumeStruct& fullData, const AsConsumeStruct::AsData& data)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
#if (RENDER_HAS_VULKAN_BACKEND == 1)
    PLUGIN_ASSERT(data.operationType == AsConsumeStruct::OperationType::INSTANCE_COPY_OP);
    if (data.index < fullData.instanceCopyInfo.size()) {
        const auto& dataRef = fullData.instanceCopyInfo[data.index];
        if ((!dataRef.bufferOffset.handle) || (dataRef.count == 0)) {
            return;
        }
        if (dataRef.startIndex + dataRef.count > static_cast<uint32_t>(fullData.instanceCopyData.size())) {
            return;
        }

        asInstanceHelper_.clear();
        asInstanceHelper_.resize(dataRef.count);
        for (size_t idx = 0; idx < asInstanceHelper_.size(); ++idx) {
            const auto& srcRef = fullData.instanceCopyData[dataRef.startIndex + idx];
            auto& dstRef = asInstanceHelper_[idx];
            dstRef.transform = srcRef.transform;
            dstRef.instanceCustomIndex = srcRef.instanceCustomIndex;
            dstRef.flags = srcRef.flags;
            dstRef.mask = srcRef.mask;
            dstRef.accelerationStructure = srcRef.accelerationStructure.GetHandle();
        }
        cmdList.CopyAccelerationStructureInstances(
            { dataRef.bufferOffset.handle.GetHandle(), dataRef.bufferOffset.offset }, asInstanceHelper_);
    }
#endif
#endif
}

void RenderNodeDefaultAccelerationStructureStaging::ExecuteFrameProcessScratch(const AsConsumeStruct& fullData)
{
    // calculate scratch sizes (the scratch buffer is only used in backend and creation is deferred in this
    // ExecuteFrame

    auto GetInfos = [](const auto input, auto& output) {
        output.clear();
        output.reserve(input.size());
        for (const auto& ref : input) {
            output.push_back(ref.info);
        }
        return output;
    };

    IDevice& device = renderNodeContextMgr_->GetRenderContext().GetDevice();
    const auto& triangles = fullData.geomTriangles;
    const auto& aabbs = fullData.geomAabbs;
    const auto& instances = fullData.geomInstances;
    uint32_t frameScratchSize = 0U;

    vector<AsGeometryTrianglesInfo> triInfos;
    vector<AsGeometryAabbsInfo> aabbInfos;
    vector<AsGeometryInstancesInfo> instanceInfos;
    for (const auto& geomRef : fullData.geometry) {
        const uint32_t startIndex = geomRef.startIndex;
        const uint32_t count = geomRef.count;
        AsBuildSizes asbs;
        if ((geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_TRIANGLES) &&
            (startIndex + count <= static_cast<uint32_t>(triangles.size()))) {
            const auto& triRef = triangles[startIndex];
            GetInfos(array_view { &triRef, count }, triInfos);
            asbs = device.GetAccelerationStructureBuildSizes(geomRef.data.info, triInfos, {}, {});
        } else if (geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_AABBS &&
                   (startIndex + count <= static_cast<uint32_t>(aabbs.size()))) {
            const auto& aabbRef = aabbs[startIndex];
            GetInfos(array_view { &aabbRef, count }, aabbInfos);
            asbs = device.GetAccelerationStructureBuildSizes(geomRef.data.info, {}, aabbInfos, {});
        } else if ((geomRef.geometryType == GeometryType::CORE_GEOMETRY_TYPE_INSTANCES) &&
                   (startIndex + count <= static_cast<uint32_t>(instances.size()))) {
            const auto& instanceRef = instances[startIndex];
            GetInfos(array_view { &instanceRef, count }, instanceInfos);
            asbs = device.GetAccelerationStructureBuildSizes(geomRef.data.info, {}, {}, instanceInfos);
        }

        scratchOffsetHelper_.push_back(frameScratchSize);
        frameScratchSize +=
            Align(asbs.buildScratchSize, PipelineLayoutConstants::MIN_UBO_BIND_OFFSET_ALIGNMENT_BYTE_SIZE);
    }
    if (frameScratchSize > 0) {
        // allocate scratch
        const GpuBufferDesc scratchDesc {
            BufferUsageFlagBits::CORE_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
                BufferUsageFlagBits::CORE_BUFFER_USAGE_STORAGE_BUFFER_BIT, // usageFlags
            CORE_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,                         // memoryPropertyFlags
            0U,                                                            // engineCreationFlags
            frameScratchSize,                                              // byteSize
            BASE_NS::Format::BASE_FORMAT_UNDEFINED,                        // format
        };
#if (RENDER_VALIDATION_ENABLED == 1)
        scratchBuffer_ =
            renderNodeContextMgr_->GetGpuResourceManager().Create("CORE_DEFAULT_FRAME_SCRATCH_BUFFER", scratchDesc);
#else
        scratchBuffer_ = renderNodeContextMgr_->GetGpuResourceManager().Create(scratchBuffer_, scratchDesc);
#endif
        rawScratchBuffer_ = scratchBuffer_.GetHandle();
    } else {
        scratchBuffer_ = {};
        rawScratchBuffer_ = {};
    }
    frameScratchOffsetIndex_ = 0U;
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
