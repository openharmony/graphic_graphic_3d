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

#include "gpu_query_vk.h"

#include <vulkan/vulkan_core.h>

#include <render/namespace.h>

#include "device/device.h"
#include "perf/gpu_query.h"
#include "vulkan/device_vk.h"
#include "vulkan/validate_vk.h"

RENDER_BEGIN_NAMESPACE()
GpuQueryVk::GpuQueryVk(Device& device, const GpuQueryDesc& desc) : device_(device), desc_(desc)
{
    plats_.resize(device_.GetCommandBufferingCount() + 1);

    constexpr VkQueryPoolCreateFlags createFlags { 0 };
    const VkQueryType queryType = (VkQueryType)desc_.queryType;
    const VkQueryPipelineStatisticFlags pipelineStatisticsFlags =
        (VkQueryPipelineStatisticFlags)desc_.queryPipelineStatisticsFlags;
    const VkQueryPoolCreateInfo createInfo {
        VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO, // sType
        nullptr,                                  // pNext
        createFlags,                              // flags
        queryType,                                // queryType
        QUERY_COUNT_PER_POOL,                     // queryCount
        pipelineStatisticsFlags,                  // pipelineStatistics
    };

    const VkDevice vkDevice = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    for (auto& ref : plats_) {
        VALIDATE_VK_RESULT(vkCreateQueryPool(vkDevice, // device
            &createInfo,                               // pCreateInfo
            nullptr,                                   // pAllocator
            &ref.queryPool));                          // pQueryPool
    }
}

GpuQueryVk::~GpuQueryVk()
{
    const VkDevice device = ((const DevicePlatformDataVk&)device_.GetPlatformData()).device;
    for (auto& ref : plats_) {
        if (ref.queryPool) {
            vkDestroyQueryPool(device, // device
                ref.queryPool,         // queryPool
                nullptr);              // pAllocator
        }
    }
}

void GpuQueryVk::NextQueryIndex()
{
    queryIndex_ = (queryIndex_ + 1) % ((uint32_t)plats_.size());
}

const GpuQueryDesc& GpuQueryVk::GetDesc() const
{
    return desc_;
}

const GpuQueryPlatformData& GpuQueryVk::GetPlatformData() const
{
    return plats_[queryIndex_];
}

BASE_NS::unique_ptr<GpuQuery> CreateGpuQueryVk(Device& device, const GpuQueryDesc& desc)
{
    return BASE_NS::make_unique<GpuQueryVk>(device, desc);
}
RENDER_END_NAMESPACE()
