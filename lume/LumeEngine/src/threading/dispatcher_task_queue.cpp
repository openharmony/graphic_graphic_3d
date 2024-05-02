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

#include "threading/dispatcher_task_queue.h"

#include <algorithm>
#include <cstddef>
#include <deque>
#include <mutex>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::vector;

// -- Dispatcher task queue.
DispatcherTaskQueue::DispatcherTaskQueue(const IThreadPool::Ptr& threadPool) : TaskQueue(threadPool) {}

DispatcherTaskQueue::~DispatcherTaskQueue()
{
    Wait();
}

void DispatcherTaskQueue::Remove(uint64_t taskIdentifier)
{
    std::lock_guard lock(queueLock_);

    auto it = std::find(tasks_.begin(), tasks_.end(), taskIdentifier);
    if (it != tasks_.end()) {
        tasks_.erase(it);
    }
}

void DispatcherTaskQueue::Clear()
{
    Wait();
    {
        std::lock_guard lock(queueLock_);

        tasks_.clear();
    }
}

void DispatcherTaskQueue::Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    std::lock_guard lock(queueLock_);

    tasks_.emplace_back(taskIdentifier, move(task));
}

void DispatcherTaskQueue::SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    std::lock_guard lock(queueLock_);

    auto it = std::find(tasks_.begin(), tasks_.end(), afterIdentifier);
    if (it != tasks_.end()) {
        tasks_.emplace(++it, taskIdentifier, move(task));
    } else {
        tasks_.emplace_back(taskIdentifier, move(task));
    }
}

void DispatcherTaskQueue::SubmitAfter(
    BASE_NS::array_view<const uint64_t> afterIdentifiers, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    ptrdiff_t pos = -1;
    for (const auto afterIdentifier : afterIdentifiers) {
        auto it = std::find(tasks_.begin(), tasks_.end(), afterIdentifier);
        if (it != tasks_.end()) {
            pos = std::max(pos, std::distance(tasks_.begin(), it));
        }
    }
    if (pos >= 0) {
        tasks_.emplace(tasks_.begin() + (pos + 1), taskIdentifier, move(task));
    } else {
        tasks_.emplace_back(taskIdentifier, move(task));
    }
}

void DispatcherTaskQueue::Execute()
{
    Entry entry;
    bool hasTaskEntry = false;

    {
        // Retrieve first task in task queue.
        std::lock_guard lock(queueLock_);

        if (!tasks_.empty()) {
            entry = BASE_NS::move(tasks_.front());
            tasks_.pop_front();
            hasTaskEntry = true;
        }
    }

    if (hasTaskEntry) {
        // Execute.
        (*entry.task)();

        {
            // Move to completed list and finish.
            std::lock_guard lock(queueLock_);
            finishedTasks_.emplace_back(std::move(entry));
        }
    }
}

vector<uint64_t> DispatcherTaskQueue::CollectFinishedTasks()
{
    std::lock_guard lock(queueLock_);

    vector<uint64_t> result;
    result.reserve(finishedTasks_.size());
    for (auto& entry : finishedTasks_) {
        result.emplace_back(entry.identifier);
    }

    finishedTasks_.clear();

    return result;
}
CORE_END_NAMESPACE()
