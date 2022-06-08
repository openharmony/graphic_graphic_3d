/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "gpu_query_manager.h"

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <render/namespace.h>

#include "device/gpu_resource_handle_util.h"
#include "perf/gpu_query.h"
#include "util/log.h"

using namespace BASE_NS;

RENDER_BEGIN_NAMESPACE()
EngineResourceHandle GpuQueryManager::Create(const string_view name, unique_ptr<GpuQuery> gpuQuery)
{
    PLUGIN_ASSERT(nameToHandle_.count(name) == 0 && "not an unique gpu query name");

    resources_.emplace_back(move(gpuQuery));
    const uint32_t index = (uint32_t)resources_.size() - 1;

    const EngineResourceHandle handle =
        RenderHandleUtil::CreateEngineResourceHandle(RenderHandleType::UNDEFINED, index, 0);
    nameToHandle_[name] = handle;
    return handle;
}

EngineResourceHandle GpuQueryManager::GetGpuHandle(const string_view name) const
{
    PLUGIN_ASSERT(nameToHandle_.count(name) > 0 && "gpu query name not found");

    const auto iter = nameToHandle_.find(name);
    if (iter != nameToHandle_.cend()) {
        return iter->second;
    } else {
        return {};
    }
}

GpuQuery* GpuQueryManager::Get(const EngineResourceHandle handle) const
{
    const uint32_t index = RenderHandleUtil::GetIndexPart(handle);
    PLUGIN_ASSERT(index < (uint32_t)resources_.size() && "gpu query name not found");

    if (index < (uint32_t)resources_.size()) {
        return resources_[index].get();
    } else {
        return nullptr;
    }
}
RENDER_END_NAMESPACE()
