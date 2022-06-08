/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef RENDER_PERF__GPU_QUERY_H
#define RENDER_PERF__GPU_QUERY_H

#include <cstdint>

#include <base/containers/unique_ptr.h>
#include <render/device/pipeline_state_desc.h>
#include <render/namespace.h>

RENDER_BEGIN_NAMESPACE()
class Device;
struct GpuQueryDesc {
    QueryType queryType { QueryType::CORE_QUERY_TYPE_TIMESTAMP };
    QueryPipelineStatisticFlags queryPipelineStatisticsFlags { 0 };
};
struct GpuQueryPlatformData {};

/**
 * GpuQuery.
 * Gpu query class.
 */
class GpuQuery {
public:
    GpuQuery() = default;
    virtual ~GpuQuery() = default;

    GpuQuery(const GpuQuery&) = delete;
    GpuQuery& operator=(const GpuQuery&) = delete;

    virtual void NextQueryIndex() = 0;

    virtual const GpuQueryDesc& GetDesc() const = 0;
    virtual const GpuQueryPlatformData& GetPlatformData() const = 0;

private:
};
RENDER_END_NAMESPACE()

#endif // CORE__PERF__GPU_QUERY_H
