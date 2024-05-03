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

#include "threading/task_queue.h"

#include <atomic>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

CORE_BEGIN_NAMESPACE()
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
