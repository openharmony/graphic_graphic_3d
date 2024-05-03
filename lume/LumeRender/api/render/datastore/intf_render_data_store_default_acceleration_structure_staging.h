/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef API_RENDER_IRENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
#define API_RENDER_IRENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H

#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()

/** Staging acceleration buffer offset */
struct StagingAccelerationStructureBufferOffset {
    /** Buffer handle */
    RenderHandleReference handle;
    /** Buffer offset in bytes */
    uint32_t offset { 0 };
};

/** Staging acceleration structure build geometry data */
struct StagingAccelerationStructureBuildGeometryData {
    /** Geometry info */
    AccelerationStructureBuildGeometryInfo info;

    /** Handle to existing acceleration structure which is used a src for dst update. */
    RenderHandleReference srcAccelerationStructure;
    /** Handle to dst acceleration structure which is to be build. */
    RenderHandleReference dstAccelerationStructure;
    /** Handle and offset to build scratch data. */
    StagingAccelerationStructureBufferOffset scratchBuffer;
};

/** Staging acceleratoin structure geometry triangles data */
struct StagingAccelerationStructureGeometryTrianglesData {
    /** Triangles info */
    AccelerationStructureGeometryTrianglesInfo info;

    /** Vertex buffer with offset */
    StagingAccelerationStructureBufferOffset vertexData;
    /** Index buffer with offset */
    StagingAccelerationStructureBufferOffset indexData;
    /** Transform buffer (4x3 matrices), additional */
    StagingAccelerationStructureBufferOffset transformData;
};

/** Staging acceleration structure geometry instances data */
struct StagingAccelerationStructureGeometryInstancesData {
    /** Instances info */
    AccelerationStructureGeometryInstancesInfo info;

    /** Buffer resource and offset for structures */
    StagingAccelerationStructureBufferOffset data;
};

/** Staging acceleration structure geometry AABBs data */
struct StagingAccelerationStructureGeometryAabbsData {
    /** AABBs info */
    AccelerationStructureGeometryAabbsInfo info;

    /** Buffer resource and offset for AabbPositions */
    StagingAccelerationStructureBufferOffset data;
};

/** Staging acceleration structure instance.
 */
struct StagingAccelerationStructureInstance {
    /** Affine transform matrix (4x3 used) */
    // BASE_NS::Math::Mat4X4 transform{};
    float transform[4][4];
    /** User specified index accessable in ray shaders with InstanceCustomIndexKHR (24 bits) */
    uint32_t instanceCustomIndex { 0u };
    /** Mask, a visibility mask for geometry (8 bits). Instance may only be hit if cull mask & instance.mask != 0. */
    uint8_t mask { 0u };
    /** GeometryInstanceFlags for this instance */
    GeometryInstanceFlags flags { 0u };

    /** Acceleration structure handle */
    RenderHandleReference accelerationStructureReference;
};

/**
 * IRenderDataStoreDefaultAccelerationStructureStaging interface.
 * Interface to add acceleration structure build operations.
 *
 * Normally run after normal staging operations.
 * I.e. current frame buffers are already staged.
 *
 * Internally synchronized.
 *
 * NOTE: One needs to input RenderHandleReferences therefore it has it's own structures
 */
class IRenderDataStoreDefaultAccelerationStructureStaging : public IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "6c2fe0b6-3bba-4048-8177-2853f2431b60" };

    /** Build acceleration structure.
     * @param buildInfo Build info for acceleration structure.
     * @param geoms Geometry data for building.
     */
    virtual void BuildAccelerationStructure(const StagingAccelerationStructureBuildGeometryData& buildData,
        const BASE_NS::array_view<const StagingAccelerationStructureGeometryTrianglesData> geometries) = 0;

    /** Build acceleration structure.
     * @param buildInfo Build info for acceleration structure.
     * @param geoms Geometry data for building.
     */
    virtual void BuildAccelerationStructure(const StagingAccelerationStructureBuildGeometryData& buildData,
        const BASE_NS::array_view<const StagingAccelerationStructureGeometryInstancesData> geometries) = 0;

    /** Build acceleration structure.
     * @param buildInfo Build info for acceleration structure.
     * @param geoms Geometry data for building.
     */
    virtual void BuildAccelerationStructure(const StagingAccelerationStructureBuildGeometryData& buildData,
        const BASE_NS::array_view<const StagingAccelerationStructureGeometryAabbsData> geometries) = 0;

    /** Copy acceleration structure instance data.
     * @param bufferOffset Instance buffer with offset. (MemoryPropertyFlags: CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
     * CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT)
     * @param data Array view of data to be copied.
     */
    virtual void CopyAccelerationStructureInstanceData(const RenderHandleReference& buffer, const uint32_t offset,
        const BASE_NS::array_view<const StagingAccelerationStructureInstance> instances) = 0;

protected:
    IRenderDataStoreDefaultAccelerationStructureStaging() = default;
    ~IRenderDataStoreDefaultAccelerationStructureStaging() override = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
