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

#ifndef META_API_TASK_H
#define META_API_TASK_H

#include <base/containers/type_traits.h>

#include <meta/interface/detail/any.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Implementation of waitable task for task queues (see ITaskQueue::AddWaitableTask)
 */
template<typename Func>
class QueueWaitableTask : public IntroduceInterfaces<ITaskQueueWaitableTask> {
public:
    QueueWaitableTask(Func func) : func_(BASE_NS::move(func)) {}

    IAny::Ptr Invoke() override
    {
        using Result = BASE_NS::remove_reference_t<decltype(func_())>;
        if constexpr (!BASE_NS::is_same_v<Result, void>) {
            return IAny::Ptr(new Any<Result>(func_()));
        } else {
            func_();
            return nullptr;
        }
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
