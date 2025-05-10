/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Task helpers
 * Author: Mikael Kilpeläinen
 * Create: 2023-02-14
 */

#ifndef META_API_TASK_H
#define META_API_TASK_H

#include <base/containers/type_traits.h>

#include <meta/interface/detail/any.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

template<typename Func>
IAny::Ptr InvokeWaitableTaskFunction(Func&& func)
{
    using Result = BASE_NS::remove_reference_t<decltype(func())>;
    if constexpr (!BASE_NS::is_same_v<Result, void>) {
        return IAny::Ptr(ConstructAny(func()));
    } else {
        func();
        return nullptr;
    }
}

/**
 * @brief Implementation of waitable task for task queues (see ITaskQueue::AddWaitableTask)
 */
template<typename Func>
class QueueWaitableTask : public IntroduceInterfaces<ITaskQueueWaitableTask> {
public:
    QueueWaitableTask(Func func) : func_(BASE_NS::move(func)) {}

    IAny::Ptr Invoke() override
    {
        return InvokeWaitableTaskFunction(func_);
    }

private:
    Func func_;
};

/**
 * @brief Create waitable task from callable entity (e.g. lambda).
 */
template<typename Func>
ITaskQueueWaitableTask::Ptr CreateWaitableTask(Func func)
{
    return ITaskQueueWaitableTask::Ptr(new QueueWaitableTask(BASE_NS::move(func)));
}

META_END_NAMESPACE()

#endif
