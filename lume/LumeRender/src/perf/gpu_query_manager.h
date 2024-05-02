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

#ifndef RENDER_PERF__GPU_QUERY_MANAGER_H
#define RENDER_PERF__GPU_QUERY_MANAGER_H

#include <cstdint>

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <render/namespace.h>

#include "device/gpu_resource_handle_util.h"

RENDER_BEGIN_NAMESPACE()
class Device;
class GpuQuery;

/** GpuQueryManager.
 * Do not use or call directly. This is managed and handled automatically in the renderer.
 * Not internally synchronized.
 */
class GpuQueryManager final {
public:
    EngineResourceHandle Create(const BASE_NS::string_view name, BASE_NS::unique_ptr<GpuQuery> gpuQuery);

    EngineResourceHandle GetGpuHandle(BASE_NS::string_view name) const;
    GpuQuery* Get(const EngineResourceHandle handle) const;

private:
    BASE_NS::unordered_map<BASE_NS::string, EngineResourceHandle> nameToHandle_;
    BASE_NS::vector<BASE_NS::unique_ptr<GpuQuery>> resources_;
};
RENDER_END_NAMESPACE()

#endif // CORE__PERF__GPU_QUERY_MANAGER_H
