/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Task queue helpers
 * Author: Mikael Kilpeläinen
 * Create: 2023-02-15
 */

#ifndef META_API_TASK_QUEUE_H
#define META_API_TASK_QUEUE_H

#include <base/containers/type_traits.h>

#include <meta/api/future.h>
#include <meta/api/task.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_promise.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

template<typename Func>
auto CreateFutureFromResultFunction(Func&& func)
{
    auto fut =
        GetTaskQueueRegistry().ConstructFutureWithValue(InvokeWaitableTaskFunction(BASE_NS::forward<Func>(func)));
    return Future<PlainType_t<decltype(func())>>(fut);
}

template<typename Func>
auto AddFutureTaskOrRunDirectly(const ITaskQueue::Ptr& queue, Func&& func)
{
    auto q = GetTaskQueueRegistry().GetCurrentTaskQueue();
    if (q == queue) {
        return CreateFutureFromResultFunction(BASE_NS::forward<Func>(func));
    }
    return Future<PlainType_t<decltype(func())>>(queue->AddWaitableTask(CreateWaitableTask(BASE_NS::move(func))));
}

META_END_NAMESPACE()

#endif
