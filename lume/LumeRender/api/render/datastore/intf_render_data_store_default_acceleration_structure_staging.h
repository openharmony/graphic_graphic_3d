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

#ifndef API_RENDER_IRENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
#define API_RENDER_IRENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H

#include <base/math/matrix.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()

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
     * @param buildInfo Build info for acceleration structure. (Optional scratchBuffer)
     * @param geoms Geometry data for building.
     */
    virtual void BuildAccelerationStructure(const AsBuildGeometryDataWithHandleReference& buildData,
        const BASE_NS::array_view<const AsGeometryTrianglesDataWithHandleReference> geometries) = 0;

    /** Build acceleration structure.
     * @param buildInfo Build info for acceleration structure. (Optional scratchBuffer)
     * @param geoms Geometry data for building.
     */
    virtual void BuildAccelerationStructure(const AsBuildGeometryDataWithHandleReference& buildData,
        const BASE_NS::array_view<const AsGeometryInstancesDataWithHandleReference> geometries) = 0;

    /** Build acceleration structure.
     * @param buildInfo Build info for acceleration structure. (Optional scratchBuffer)
     * @param geoms Geometry data for building.
     */
    virtual void BuildAccelerationStructure(const AsBuildGeometryDataWithHandleReference& buildData,
        const BASE_NS::array_view<const AsGeometryAabbsDataWithHandleReference> geometries) = 0;

    /** Copy acceleration structure instance data.
     * @param dstBuffer Instance buffer with offset. (MemoryPropertyFlags: CORE_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
     * CORE_MEMORY_PROPERTY_HOST_COHERENT_BIT)
     * @param data Array view of data to be copied.
     */
    virtual void CopyAccelerationStructureInstanceData(const BufferOffsetWithHandleReference& dstBuffer,
        const BASE_NS::array_view<const AsInstanceWithHandleReference> instances) = 0;

protected:
    IRenderDataStoreDefaultAccelerationStructureStaging() = default;
    ~IRenderDataStoreDefaultAccelerationStructureStaging() override = default;
};
RENDER_END_NAMESPACE()

#endif // API_RENDER_IRENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
