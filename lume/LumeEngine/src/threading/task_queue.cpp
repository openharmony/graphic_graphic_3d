/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "threading/task_queue.h"

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include <base/containers/vector.h>
#include <core/log.h>
#include <core/namespace.h>

#include "os/platform.h"

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::unique_ptr;

// -- TaskQueue ExecuteAsyncTask, runs TaskQueue::Execute.
TaskQueue::ExecuteAsyncTask::ExecuteAsyncTask(TaskQueue& queue) : queue_(queue) {}

void TaskQueue::ExecuteAsyncTask::operator()()
{
    queue_.Execute();
    queue_.isRunningAsync_ = false;
}

void TaskQueue::ExecuteAsyncTask::Destroy()
{
    delete this;
}

// -- TaskQueue
TaskQueue::TaskQueue(const IThreadPool::Ptr& threadPool) : threadPool_(threadPool), isRunningAsync_(false) {}

TaskQueue::~TaskQueue() = default;

void TaskQueue::ExecuteAsync()
{
    CORE_ASSERT(threadPool_ != nullptr);

    if (!IsRunningAsync()) {
        isRunningAsync_ = true;

        // Execute in new thread.
        asyncOperation_ = threadPool_->Push(IThreadPool::ITask::Ptr { new ExecuteAsyncTask(*this) });
    }
}

bool TaskQueue::IsRunningAsync() const
{
    return isRunningAsync_;
}

void TaskQueue::Wait()
{
    if (IsRunningAsync()) {
        asyncOperation_->Wait();
        isRunningAsync_ = false;
    }
}

// -- TaskQueue entry.
TaskQueue::Entry::Entry() {}

TaskQueue::Entry::Entry(uint64_t identifier, IThreadPool::ITask::Ptr task) : task(move(task)), identifier(identifier) {}

bool TaskQueue::Entry::operator==(uint64_t rhsIdentifier) const
{
    return identifier == rhsIdentifier;
}

bool TaskQueue::Entry::operator==(const TaskQueue::Entry& other) const
{
    return identifier == other.identifier;
}
CORE_END_NAMESPACE()
