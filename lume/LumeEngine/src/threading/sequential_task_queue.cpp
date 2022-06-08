/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "threading/sequential_task_queue.h"

#include <algorithm>

#include <core/log.h>
#include <core/namespace.h>

#include "os/platform.h"

CORE_BEGIN_NAMESPACE()
// -- Sequential task queue.
SequentialTaskQueue::SequentialTaskQueue(const IThreadPool::Ptr& threadPool) : TaskQueue(threadPool) {}

SequentialTaskQueue::~SequentialTaskQueue()
{
    Wait();
}

void SequentialTaskQueue::Execute()
{
    // A function that executes tasks one by one.
    for (auto& entry : tasks_) {
        (*entry.task)();
    }
}

void SequentialTaskQueue::Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    CORE_ASSERT(std::find(tasks_.begin(), tasks_.end(), taskIdentifier) == tasks_.end());

    tasks_.emplace_back(taskIdentifier, std::move(task));
}

void SequentialTaskQueue::SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    auto it = std::find(tasks_.begin(), tasks_.end(), afterIdentifier);
    if (it != tasks_.end()) {
        tasks_.emplace(++it, taskIdentifier, std::move(task));
    } else {
        tasks_.emplace_back(taskIdentifier, std::move(task));
    }
}

void SequentialTaskQueue::SubmitBefore(
    uint64_t beforeIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    CORE_ASSERT(std::find(tasks_.begin(), tasks_.end(), taskIdentifier) == tasks_.end());

    auto it = std::find(tasks_.begin(), tasks_.end(), beforeIdentifier);
    if (it != tasks_.end()) {
        tasks_.emplace(it, taskIdentifier, std::move(task));
    }
}

void SequentialTaskQueue::Remove(uint64_t taskIdentifier)
{
    auto it = std::find(tasks_.begin(), tasks_.end(), taskIdentifier);
    if (it != tasks_.end()) {
        tasks_.erase(it);
    }
}

void SequentialTaskQueue::Clear()
{
    Wait();
    tasks_.clear();
}
CORE_END_NAMESPACE()
