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

#include "render_data_store_default_acceleration_structure_staging.h"

#include <cstdint>

#include <render/device/intf_device.h>
#include <render/device/intf_gpu_resource_manager.h>
#include <render/intf_render_context.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
RenderDataStoreDefaultAccelerationStructureStaging::RenderDataStoreDefaultAccelerationStructureStaging(
    IRenderContext& renderContext, const string_view name)
    : gpuResourceMgr_(renderContext.GetDevice().GetGpuResourceManager()), name_(name)
{}

RenderDataStoreDefaultAccelerationStructureStaging::~RenderDataStoreDefaultAccelerationStructureStaging() = default;

void RenderDataStoreDefaultAccelerationStructureStaging::PreRender()
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    std::lock_guard<std::mutex> lock(mutex_);

    // get stuff ready for render nodes
    frameStaging_ = move(staging_);

    staging_.geometry.clear();
    staging_.geomTriangles.clear();
    staging_.geomAabbs.clear();
    staging_.geomInstances.clear();
    staging_.instanceCopyInfo.clear();
    staging_.instanceCopyData.clear();
    staging_.data.clear();
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::PostRender() {}

void RenderDataStoreDefaultAccelerationStructureStaging::Clear()
{
    // The data cannot be automatically cleared here
}

void RenderDataStoreDefaultAccelerationStructureStaging::Ref()
{
    refcnt_.fetch_add(1, std::memory_order_relaxed);
}

void RenderDataStoreDefaultAccelerationStructureStaging::Unref()
{
    if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
        std::atomic_thread_fence(std::memory_order_acquire);
        delete this;
    }
}

int32_t RenderDataStoreDefaultAccelerationStructureStaging::GetRefCount()
{
    return refcnt_;
}

void RenderDataStoreDefaultAccelerationStructureStaging::BuildAccelerationStructure(
    const AsBuildGeometryDataWithHandleReference& buildData,
    const BASE_NS::array_view<const AsGeometryTrianglesDataWithHandleReference> geometries)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    if (!geometries.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);

        // next op
        const uint32_t index = static_cast<uint32_t>(staging_.geometry.size());
        staging_.data.push_back({ AsConsumeStruct::OperationType::BUILD_OP, index });

        auto& prims = staging_.geomTriangles;
        const uint32_t startIndex = static_cast<uint32_t>(prims.size());
        const uint32_t count = static_cast<uint32_t>(geometries.size());
        prims.append(geometries.begin(), geometries.end());
        staging_.geometry.push_back(AsConsumeStruct::Geom {
            buildData,
            GeometryType::CORE_GEOMETRY_TYPE_TRIANGLES,
            startIndex,
            count,
        });
    }
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::BuildAccelerationStructure(
    const AsBuildGeometryDataWithHandleReference& buildData,
    const BASE_NS::array_view<const AsGeometryInstancesDataWithHandleReference> geometries)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    if (!geometries.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);

        // next op
        const uint32_t index = static_cast<uint32_t>(staging_.geometry.size());
        staging_.data.push_back({ AsConsumeStruct::OperationType::BUILD_OP, index });

        auto& prims = staging_.geomInstances;
        const uint32_t startIndex = static_cast<uint32_t>(prims.size());
        const uint32_t count = static_cast<uint32_t>(geometries.size());
        prims.append(geometries.begin(), geometries.end());
        staging_.geometry.push_back(AsConsumeStruct::Geom {
            buildData,
            GeometryType::CORE_GEOMETRY_TYPE_INSTANCES,
            startIndex,
            count,
        });
    }
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::BuildAccelerationStructure(
    const AsBuildGeometryDataWithHandleReference& buildData,
    const BASE_NS::array_view<const AsGeometryAabbsDataWithHandleReference> geometries)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    if (!geometries.empty()) {
        std::lock_guard<std::mutex> lock(mutex_);

        // next op
        const uint32_t index = static_cast<uint32_t>(staging_.geometry.size());
        staging_.data.push_back({ AsConsumeStruct::OperationType::BUILD_OP, index });

        auto& prims = staging_.geomAabbs;
        const uint32_t startIndex = static_cast<uint32_t>(prims.size());
        const uint32_t count = static_cast<uint32_t>(geometries.size());
        prims.append(geometries.begin(), geometries.end());
        staging_.geometry.push_back(AsConsumeStruct::Geom {
            buildData,
            GeometryType::CORE_GEOMETRY_TYPE_AABBS,
            startIndex,
            count,
        });
    }
#endif
}

void RenderDataStoreDefaultAccelerationStructureStaging::CopyAccelerationStructureInstanceData(
    const BufferOffsetWithHandleReference& dstBuffer,
    const BASE_NS::array_view<const AsInstanceWithHandleReference> instances)
{
#if (RENDER_VULKAN_RT_ENABLED == 1)
    if ((!instances.empty()) && dstBuffer.handle) {
        std::lock_guard<std::mutex> lock(mutex_);

        // next op
        const uint32_t index = static_cast<uint32_t>(staging_.instanceCopyInfo.size());
        staging_.data.push_back({ AsConsumeStruct::OperationType::INSTANCE_COPY_OP, index });

        auto& prims = staging_.instanceCopyData;
        const uint32_t startIndex = static_cast<uint32_t>(prims.size());
        const uint32_t count = static_cast<uint32_t>(instances.size());
        prims.append(instances.begin(), instances.end());
        staging_.instanceCopyInfo.push_back(AsConsumeStruct::CopyTarget {
            dstBuffer,
            startIndex,
            count,
        });
    }
#endif
}

bool RenderDataStoreDefaultAccelerationStructureStaging::HasStagingData() const
{
    if (frameStaging_.geometry.empty()) {
        return false;
    } else {
        return true;
    }
}

AsConsumeStruct RenderDataStoreDefaultAccelerationStructureStaging::ConsumeStagingData()
{
    AsConsumeStruct scs = move(frameStaging_);
    return scs;
}

// for plugin / factory interface
refcnt_ptr<IRenderDataStore> RenderDataStoreDefaultAccelerationStructureStaging::Create(
    IRenderContext& renderContext, const char* name)
{
    return refcnt_ptr<IRenderDataStore>(new RenderDataStoreDefaultAccelerationStructureStaging(renderContext, name));
}
RENDER_END_NAMESPACE()
