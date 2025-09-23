/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef OHOS_3D_UTILS_H
#define OHOS_3D_UTILS_H

#include <string>

#include <meta/api/make_callback.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <scene/interface/intf_scene.h>

namespace OHOS::Render3D {
#define RETURN_IF_NULL(ptr)                                  \
    do {                                                     \
        if (!(ptr)) {                                        \
            CORE_LOG_E("%s is null in %s", #ptr, __func__);  \
            return;                                          \
        }                                                    \
    } while (0)

#define RETURN_IF_NULL_WITH_VALUE(ptr, ret)                  \
    do {                                                     \
        if (!(ptr)) {                                        \
            CORE_LOG_E("%s is null in %s", #ptr, __func__);  \
            return ret;                                      \
        }                                                    \
    } while (0)
// tasks execute in the engine/render thread.
static constexpr BASE_NS::Uid ENGINE_THREAD { "2070e705-d061-40e4-bfb7-90fad2c280af" };

// tasks execute in the javascript mainthread. *NOT IMPLEMENTED*
static constexpr BASE_NS::Uid JS_THREAD { "b2e8cef3-453a-4651-b564-5190f8b5190d" };
static constexpr BASE_NS::Uid JS_RELEASE_THREAD { "3784fa96-b25b-4e9c-bbf1-e897d36f73af" };
static constexpr BASE_NS::Uid IO_QUEUE { "be88e9a0-9cd8-45ab-be48-937953dc258f" };

template <typename V>
struct InvokeReturn {
    V value;
    std::string error;

    explicit InvokeReturn(const V &v) : value(v), error("")
    {}

    explicit InvokeReturn(const V &v, const std::string &errorMsg) : value(v), error(errorMsg)
    {}

    explicit operator bool() const
    {
        return error.empty();
    }

    bool operator!() const
    {
        return !(error.empty());
    }
};

// run synchronous task in specific tq.
template <typename func>
META_NS::IAny::Ptr ExecSyncTask(const META_NS::ITaskQueue::Ptr tq, func &&fun)
{
    return tq
        ->AddWaitableTask(BASE_NS::move(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun))))
        ->GetResult();
}

// run task synchronously in engine thread.
template <typename func>
META_NS::IAny::Ptr ExecSyncTask(func &&fun)
{
    return ExecSyncTask(META_NS::GetTaskQueueRegistry().GetTaskQueue(ENGINE_THREAD), BASE_NS::move(fun));
}
}  // namespace OHOS::Render3D
#endif  // OHOS_3D_UTILS_H