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

#ifndef RENDER_RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
#define RENDER_RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H

#include <cstdint>
#include <mutex>

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

struct AccelerationStructureBuildConsumeStruct {
    struct Geom {
        StagingAccelerationStructureBuildGeometryData data;
        GeometryType geometryType { CORE_GEOMETRY_TYPE_TRIANGLES };
        uint32_t startIndex { 0u };
        uint32_t count { 0u };
    };
    BASE_NS::vector<Geom> geometry;
    BASE_NS::vector<StagingAccelerationStructureGeometryTrianglesData> triangles;
    BASE_NS::vector<StagingAccelerationStructureGeometryAabbsData> aabbs;
    BASE_NS::vector<StagingAccelerationStructureGeometryInstancesData> instances;
};

struct AccelerationStructureInstanceConsumeStruct {
    struct Target {
        StagingAccelerationStructureBufferOffset bufferOffset;
        uint32_t startIndex { 0u };
        uint32_t count { 0u };
    };
    BASE_NS::vector<Target> copyInfo;
    BASE_NS::vector<StagingAccelerationStructureInstance> instances;
};

/**
RenderDataStoreDefaultAccelerationStructureStaging implementation.
*/
class RenderDataStoreDefaultAccelerationStructureStaging final
    : public IRenderDataStoreDefaultAccelerationStructureStaging {
public:
    RenderDataStoreDefaultAccelerationStructureStaging(IRenderContext& renderContext, const BASE_NS::string_view name);
    ~RenderDataStoreDefaultAccelerationStructureStaging() override;

    void CommitFrameData() override {};
    void PreRender() override;
    void PostRender() override;
    void PreRenderBackend() override {};
    void PostRenderBackend() override {};
    void Clear() override;
    uint32_t GetFlags() const override
    {
        return 0;
    };

    void BuildAccelerationStructure(const StagingAccelerationStructureBuildGeometryData& buildData,
        const BASE_NS::array_view<const StagingAccelerationStructureGeometryTrianglesData> geometries) override;
    void BuildAccelerationStructure(const StagingAccelerationStructureBuildGeometryData& buildData,
        const BASE_NS::array_view<const StagingAccelerationStructureGeometryInstancesData> geometries) override;
    void BuildAccelerationStructure(const StagingAccelerationStructureBuildGeometryData& buildData,
        const BASE_NS::array_view<const StagingAccelerationStructureGeometryAabbsData> geometries) override;

    void CopyAccelerationStructureInstanceData(const RenderHandleReference& buffer, const uint32_t offset,
        const BASE_NS::array_view<const StagingAccelerationStructureInstance> instances) override;

    bool HasStagingData() const;
    AccelerationStructureBuildConsumeStruct ConsumeStagingBuildData();
    AccelerationStructureInstanceConsumeStruct ConsumeStagingInstanceData();

    // for plugin / factory interface
    static constexpr const char* const TYPE_NAME = "RenderDataStoreDefaultAccelerationStructureStaging";

    static IRenderDataStore* Create(IRenderContext& renderContext, char const* name);
    static void Destroy(IRenderDataStore* instance);

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

private:
    IGpuResourceManager& gpuResourceMgr_;
    const BASE_NS::string name_;

    AccelerationStructureBuildConsumeStruct stagingBuild_;
    AccelerationStructureInstanceConsumeStruct stagingInstance_;
    // in pre render data moved here
    AccelerationStructureBuildConsumeStruct frameStagingBuild_;
    AccelerationStructureInstanceConsumeStruct frameStagingInstance_;

    mutable std::mutex mutex_;
};
RENDER_END_NAMESPACE()

#endif // RENDER_RENDER__NODE_DATA__RENDER_DATA_STORE_DEFAULT_ACCELERATION_STRUCTURE_STAGING_H
