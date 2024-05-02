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

#ifndef META_API_TASK_QUEUE_H
#define META_API_TASK_QUEUE_H

#include <base/containers/type_traits.h>

#include <meta/api/task.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_promise.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

template<typename Func>
auto CreateFutureFromResultFunction(Func func)
{
    // todo: add support for lighter promise construction
    auto p = GetObjectRegistry().Create<IPromise>(ClassId::Promise);
    auto fut = p->GetFuture();
    p->Set(QueueWaitableTask(BASE_NS::move(func)).Invoke());
    return Future<PlainType_t<decltype(func())>>(fut);
}

template<typename Func>
auto AddFutureTaskOrRunDirectly(const ITaskQueue::Ptr& queue, Func func)
{
    auto q = GetTaskQueueRegistry().GetCurrentTaskQueue();
    if (q == queue) {
        return CreateFutureFromResultFunction(BASE_NS::move(func));
    }
    return Future<PlainType_t<decltype(func())>>(queue->AddWaitableTask(CreateWaitableTask(func)));
}

META_END_NAMESPACE()

#endif
