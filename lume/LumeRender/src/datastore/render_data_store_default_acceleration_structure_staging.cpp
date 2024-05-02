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

#include "render_data_store_default_acceleration_structure_staging.h"

#include <cstdint>

#include <render/device/gpu_resource_desc.h>
#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

#include "device/gpu_resource_manager.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreDefaultAccelerationStructureStaging::RenderDataStoreDefaultAccelerationStructureStaging(
    IRenderContext& renderContext, const string_view name)
    : gpuResourceMgr_(renderContext.GetDevice().GetGpuResourceManager()), name_(name)
{}

RenderDataStoreDefaultAccelerationStructureStaging::~RenderDataStoreDefaultAccelerationStructureStaging() {}

void RenderDataStoreDefaultAccelerationStructureStaging::PreRender()
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    // get stuff ready for render nodes

    std::lock_guard<std::mutex> lock(mutex_);

    frameStagingBuild_ = move(stagingBuild_);
    frameStagingInstance_ = move(stagingInstance_);
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::PostRender() {}

void RenderDataStoreDefaultAccelerationStructureStaging::Clear()
{
    // The data cannot be automatically cleared here
}

void RenderDataStoreDefaultAccelerationStructureStaging::BuildAccelerationStructure(
    const StagingAccelerationStructureBuildGeometryData& buildData,
    const BASE_NS::array_view<const StagingAccelerationStructureGeometryTrianglesData> geometries)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    std::lock_guard<std::mutex> lock(mutex_);

    if (!geometries.empty()) {
        auto& geoms = stagingBuild_.triangles;
        const uint32_t startIndex = static_cast<uint32_t>(geoms.size());
        const uint32_t count = static_cast<uint32_t>(geometries.size());
        geoms.reserve(startIndex + count);
        for (const auto& geometriesRef : geometries) {
            geoms.push_back(geometriesRef);
        }
        stagingBuild_.geometry.push_back(AccelerationStructureBuildConsumeStruct::Geom {
            buildData,
            GeometryType::CORE_GEOMETRY_TYPE_TRIANGLES,
            startIndex,
            count,
        });
    }
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::BuildAccelerationStructure(
    const StagingAccelerationStructureBuildGeometryData& buildData,
    const BASE_NS::array_view<const StagingAccelerationStructureGeometryInstancesData> geometries)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    std::lock_guard<std::mutex> lock(mutex_);

    if (!geometries.empty()) {
        auto& geoms = stagingBuild_.instances;
        const uint32_t startIndex = static_cast<uint32_t>(geoms.size());
        const uint32_t count = static_cast<uint32_t>(geometries.size());
        geoms.reserve(startIndex + count);
        for (const auto& geometriesRef : geometries) {
            geoms.push_back(geometriesRef);
        }
        stagingBuild_.geometry.push_back(AccelerationStructureBuildConsumeStruct::Geom {
            buildData,
            GeometryType::CORE_GEOMETRY_TYPE_INSTANCES,
            startIndex,
            count,
        });
    }
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::BuildAccelerationStructure(
    const StagingAccelerationStructureBuildGeometryData& buildData,
    const BASE_NS::array_view<const StagingAccelerationStructureGeometryAabbsData> geometries)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    std::lock_guard<std::mutex> lock(mutex_);

    if (!geometries.empty()) {
        auto& geoms = stagingBuild_.aabbs;
        const uint32_t startIndex = static_cast<uint32_t>(geoms.size());
        const uint32_t count = static_cast<uint32_t>(geometries.size());
        geoms.reserve(startIndex + count);
        for (const auto& geometriesRef : geometries) {
            geoms.push_back(geometriesRef);
        }
        stagingBuild_.geometry.push_back(AccelerationStructureBuildConsumeStruct::Geom {
            buildData,
            GeometryType::CORE_GEOMETRY_TYPE_AABBS,
            startIndex,
            count,
        });
    }
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::CopyAccelerationStructureInstanceData(
    const RenderHandleReference& buffer, const uint32_t offset,
    const BASE_NS::array_view<const StagingAccelerationStructureInstance> instances)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    std::lock_guard<std::mutex> lock(mutex_);

    if ((!instances.empty()) && buffer) {
        auto& ins = stagingInstance_.instances;
        const uint32_t startIndex = static_cast<uint32_t>(ins.size());
        const uint32_t count = static_cast<uint32_t>(instances.size());
        ins.reserve(startIndex + count);
        for (const auto& instancesRef : instances) {
            ins.push_back(instancesRef);
        }
        stagingInstance_.copyInfo.push_back(AccelerationStructureInstanceConsumeStruct::Target {
            { buffer, offset },
            startIndex,
            count,
        });
    }
#endif
}

bool RenderDataStoreDefaultAccelerationStructureStaging::HasStagingData() const
{
    if (frameStagingBuild_.geometry.empty() && frameStagingInstance_.copyInfo.empty()) {
        return false;
    } else {
        return true;
    }
}

AccelerationStructureBuildConsumeStruct RenderDataStoreDefaultAccelerationStructureStaging::ConsumeStagingBuildData()
{
    AccelerationStructureBuildConsumeStruct scs = move(frameStagingBuild_);
    return scs;
}

AccelerationStructureInstanceConsumeStruct
RenderDataStoreDefaultAccelerationStructureStaging::ConsumeStagingInstanceData()
{
    AccelerationStructureInstanceConsumeStruct scs = move(frameStagingInstance_);
    return scs;
}

// for plugin / factory interface
IRenderDataStore* RenderDataStoreDefaultAccelerationStructureStaging::Create(
    IRenderContext& renderContext, char const* name)
{
    return new RenderDataStoreDefaultAccelerationStructureStaging(renderContext, name);
}

void RenderDataStoreDefaultAccelerationStructureStaging::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreDefaultAccelerationStructureStaging*>(instance);
}
RENDER_END_NAMESPACE()
