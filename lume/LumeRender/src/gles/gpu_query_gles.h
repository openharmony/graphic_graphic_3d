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

#ifndef VULKAN_GPU_QUERY_GLES_H
#define VULKAN_GPU_QUERY_GLES_H

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "perf/gpu_query.h"

RENDER_BEGIN_NAMESPACE()
class Device;

struct GpuQueryPlatformDataGLES : public GpuQueryPlatformData {
    uint32_t queryObject;
};

/** GpuQueryGLES. */
class GpuQueryGLES final : public GpuQuery {
public:
    GpuQueryGLES(Device& device, const GpuQueryDesc& desc);
    ~GpuQueryGLES();

    void NextQueryIndex() override;

    const GpuQueryDesc& GetDesc() const override;
    const GpuQueryPlatformData& GetPlatformData() const override;

private:
    uint32_t queryIndex_ { 0 };
    BASE_NS::vector<GpuQueryPlatformDataGLES> plats_;
    GpuQueryDesc desc_;
};
BASE_NS::unique_ptr<GpuQuery> CreateGpuQueryGLES(Device& device, const GpuQueryDesc& desc);
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_QUERY_GLES_H
