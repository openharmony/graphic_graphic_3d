/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
