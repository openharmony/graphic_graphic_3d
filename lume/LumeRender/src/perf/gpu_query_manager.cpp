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

    resources_.push_back(move(gpuQuery));
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
