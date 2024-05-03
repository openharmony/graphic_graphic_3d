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

#include "gpu_query_gles.h"

#include <render/namespace.h>

#include "device/device.h"
#include "gles/device_gles.h"
#include "gles/gl_functions.h"
#include "perf/gpu_query.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
GpuQueryGLES::GpuQueryGLES(Device& device, const GpuQueryDesc& desc) : desc_(desc)
{
    PLUGIN_ASSERT(static_cast<DeviceGLES&>(device).IsActive());
    BASE_NS::vector<GLuint> queries(device.GetCommandBufferingCount() + 1);
    glGenQueries(static_cast<GLsizei>(queries.size()), queries.data());
    plats_.reserve(queries.size());
    for (const auto q : queries) {
        plats_.emplace_back().queryObject = q;
    }
}

GpuQueryGLES::~GpuQueryGLES()
{
    for (const auto& plat : plats_) {
        glDeleteQueries(1, &plat.queryObject);
    }
}

void GpuQueryGLES::NextQueryIndex()
{
    queryIndex_ = (queryIndex_ + 1) % ((uint32_t)plats_.size());
}

const GpuQueryDesc& GpuQueryGLES::GetDesc() const
{
    return desc_;
}

const GpuQueryPlatformData& GpuQueryGLES::GetPlatformData() const
{
    PLUGIN_ASSERT(queryIndex_ < plats_.size());
    return plats_[queryIndex_];
}

BASE_NS::unique_ptr<GpuQuery> CreateGpuQueryGLES(Device& device, const GpuQueryDesc& desc)
{
    return BASE_NS::make_unique<GpuQueryGLES>(device, desc);
}
RENDER_END_NAMESPACE()
