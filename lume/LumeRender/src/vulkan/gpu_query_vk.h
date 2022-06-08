/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef VULKAN_GPU_QUERY_VK_H
#define VULKAN_GPU_QUERY_VK_H

#include <vulkan/vulkan_core.h>

#include <base/containers/vector.h>
#include <render/namespace.h>

#include "perf/gpu_query.h"

RENDER_BEGIN_NAMESPACE()
class Device;

struct GpuQueryPlatformDataVk : public GpuQueryPlatformData {
    VkQueryPool queryPool { VK_NULL_HANDLE };
};

/** GpuQueryVk. */
class GpuQueryVk final : public GpuQuery {
public:
    GpuQueryVk(Device& device, const GpuQueryDesc& desc);
    ~GpuQueryVk();

    void NextQueryIndex() override;

    const GpuQueryDesc& GetDesc() const override;
    const GpuQueryPlatformData& GetPlatformData() const override;

private:
    Device& device_;

    static constexpr const uint32_t QUERY_COUNT_PER_POOL { 2 };

    uint32_t queryIndex_ { 0 };
    BASE_NS::vector<GpuQueryPlatformDataVk> plats_;
    GpuQueryDesc desc_;
};
BASE_NS::unique_ptr<GpuQuery> CreateGpuQueryVk(Device& device, const GpuQueryDesc& desc);
RENDER_END_NAMESPACE()

#endif // VULKAN_GPU_QUERY_VK_H
