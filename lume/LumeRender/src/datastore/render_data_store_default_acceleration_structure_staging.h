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

#ifndef RENDER_DATASTORE_RENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
#define RENDER_DATASTORE_RENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H

#include <atomic>
#include <cstdint>
#include <mutex>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/util/uid.h>
#include <render/datastore/intf_render_data_store_default_acceleration_structure_staging.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IGpuResourceManager;
class IRenderContext;

struct AsConsumeStruct {
    enum class OperationType : uint8_t {
        UNDEFINED_OP = 0,
        BUILD_OP = 1,
        INSTANCE_COPY_OP = 2,
    };
    struct AsData {
        OperationType operationType { OperationType::UNDEFINED_OP };
        uint32_t index { 0U };
    };

    struct CopyTarget {
        BufferOffsetWithHandleReference bufferOffset;
        uint32_t startIndex { 0u };
        uint32_t count { 0u };
    };
    struct Geom {
        AsBuildGeometryDataWithHandleReference data;
        GeometryType geometryType { CORE_GEOMETRY_TYPE_TRIANGLES };
        uint32_t startIndex { 0u };
        uint32_t count { 0u };
    };

    BASE_NS::vector<Geom> geometry;
    BASE_NS::vector<AsGeometryTrianglesDataWithHandleReference> geomTriangles;
    BASE_NS::vector<AsGeometryAabbsDataWithHandleReference> geomAabbs;
    BASE_NS::vector<AsGeometryInstancesDataWithHandleReference> geomInstances;

    BASE_NS::vector<CopyTarget> instanceCopyInfo;
    BASE_NS::vector<AsInstanceWithHandleReference> instanceCopyData;

    // actual indices and info to data
    BASE_NS::vector<AsData> data;
};

/**
RenderDataStoreDefaultAccelerationStructureStaging implementation.
*/
class RenderDataStoreDefaultAccelerationStructureStaging final
    : public IRenderDataStoreDefaultAccelerationStructureStaging {
public:
    RenderDataStoreDefaultAccelerationStructureStaging(IRenderContext& renderContext, BASE_NS::string_view name);
    ~RenderDataStoreDefaultAccelerationStructureStaging() override;

    // IRenderDataStore
    void PreRender() override;
    void PostRender() override;
    void PreRenderBackend() override {}
    void PostRenderBackend() override {}
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0;
    }

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    // IRenderDataStoreDefaultAccelerationStructureStaging
    void BuildAccelerationStructure(const AsBuildGeometryDataWithHandleReference& buildData,
        BASE_NS::array_view<const AsGeometryTrianglesDataWithHandleReference> geometries) override;
    void BuildAccelerationStructure(const AsBuildGeometryDataWithHandleReference& buildData,
        BASE_NS::array_view<const AsGeometryInstancesDataWithHandleReference> geometries) override;
    void BuildAccelerationStructure(const AsBuildGeometryDataWithHandleReference& buildData,
        BASE_NS::array_view<const AsGeometryAabbsDataWithHandleReference> geometries) override;

    void CopyAccelerationStructureInstanceData(const BufferOffsetWithHandleReference& dstBuffer,
        BASE_NS::array_view<const AsInstanceWithHandleReference> instances) override;

    bool HasStagingData() const;
    AsConsumeStruct ConsumeStagingData();

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreDefaultAccelerationStructureStaging";

    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(IRenderContext& renderContext, const char* name);

private:
    IGpuResourceManager& gpuResourceMgr_;
    const BASE_NS::string name_;

    AsConsumeStruct staging_;
    // in pre render data moved here
    AsConsumeStruct frameStaging_;

    mutable std::mutex mutex_;

    std::atomic_int32_t refcnt_ { 0 };
};
RENDER_END_NAMESPACE()

#endif // RENDER_DATASTORE_RENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
