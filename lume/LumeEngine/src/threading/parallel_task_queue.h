/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_THREADING_PARALLEL_QUEUE_H
#define CORE_THREADING_PARALLEL_QUEUE_H

#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

#include "threading/task_queue.h"

CORE_BEGIN_NAMESPACE()
// Non-thread safe parallel task queue, executes all tasks at once in parallel.
// This queue type is not thread safe and should be only used from one thread.
class ParallelTaskQueue final : public TaskQueue {
public:
    /** Constructor for the parallel task queue.
        @param threads Optional thread pool, if support for threading is desired.
    */
    explicit ParallelTaskQueue(const IThreadPool::Ptr& threadPool);
    ParallelTaskQueue(const ParallelTaskQueue& other) = delete;
    ~ParallelTaskQueue() override;

    /** Submit task to execution queue, to be run after another task.
        @param afterIdentifier Identifier of the task that is run prior the submitted task.
        @param taskIdentifier Identifier of the task, must be unique.
        @param task Task to execute.
    */
    void SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task);

    void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void Remove(uint64_t taskIdentifier) override;

    void Clear() override;

    void Execute() override;

private:
    struct TaskState;
    class Task;

    void QueueTasks(BASE_NS::vector<size_t>& waiting, TaskState& taskState);

    BASE_NS::vector<TaskQueue::Entry> tasks_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_TASK_QUEUE_H
