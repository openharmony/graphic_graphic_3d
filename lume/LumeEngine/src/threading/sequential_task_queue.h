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

#ifndef CORE_THREADING_SEQUENTIAL_TASK_QUEUE_H
#define CORE_THREADING_SEQUENTIAL_TASK_QUEUE_H

#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

#include "threading/task_queue.h"

BASE_BEGIN_NAMESPACE()
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
// Non-thread safe sequential task queue, executes tasks sequentially one-by-one.
// This queue type is not thread safe and should be only used from one thread.
class SequentialTaskQueue final : public TaskQueue {
public:
    /** Constructor for the sequential task queue.
        @param threads Optional thread pool, if support for threading is desired.
    */
    explicit SequentialTaskQueue(const IThreadPool::Ptr& threadPool);
    SequentialTaskQueue(const SequentialTaskQueue& other) = delete;
    ~SequentialTaskQueue() override;

    /** Submit task to execution queue, to be run after another task.
        @param afterIdentifier Identifier of the task that is run prior the submitted task.
        @param taskIdentifier Identifier of the task, must be unique.
        @param task Task to execute.
    */
    void SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task);

    void SubmitAfter(
        BASE_NS::array_view<const uint64_t> afterIdentifiers, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task);

    /** Submit task to execution queue, to be run before another task.
        @param beforeIdentifier Identifier of the task that is run after the submitted task.
        @param taskIdentifier Identifier of the task, must be unique.
        @param task Task to execute.
    */
    void SubmitBefore(uint64_t beforeIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task);

    void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void Remove(uint64_t taskIdentifier) override;

    void Clear() override;

    void Execute() override;

private:
    BASE_NS::vector<TaskQueue::Entry> tasks_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_SEQUENTIAL_TASK_QUEUE_H
