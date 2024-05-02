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
