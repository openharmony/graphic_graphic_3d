/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_THREADING_DISPATHCER_TASK_QUEUE_H
#define CORE_THREADING_DISPATHCER_TASK_QUEUE_H

#include <deque>
#include <mutex>

#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

#include "threading/task_queue.h"

CORE_BEGIN_NAMESPACE()
// Thread safe task dispatcher, executes one task per execute call and removes it once it is finished.
class DispatcherTaskQueue final : public TaskQueue {
public:
    /** Constructor for the dispatcher task queue.
        @param threads Optional thread pool, if support for threading is desired.
    */
    explicit DispatcherTaskQueue(const IThreadPool::Ptr& threadPool);
    DispatcherTaskQueue(const DispatcherTaskQueue& other) = delete;
    ~DispatcherTaskQueue() override;

    /** Reports finished tasks, allows to check which tasks have been completed.
        @return Task ids of tasks that have finished.
    */
    BASE_NS::vector<uint64_t> CollectFinishedTasks();

    void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) override;
    void SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task);
    void Remove(uint64_t taskIdentifier) override;

    void Clear() override;

    void Execute() override;

private:
    std::deque<Entry> tasks_;
    BASE_NS::vector<Entry> finishedTasks_;
    std::mutex queueLock_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_DISPATHCER_TASK_QUEUE_H
