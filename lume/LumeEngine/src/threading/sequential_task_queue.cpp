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

#include "threading/sequential_task_queue.h"

#include <algorithm>
#include <cstddef>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

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
    CORE_ASSERT(std::find(tasks_.cbegin(), tasks_.cend(), taskIdentifier) == tasks_.cend());

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

void SequentialTaskQueue::SubmitAfter(
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
        tasks_.emplace(tasks_.begin() + (pos + 1), taskIdentifier, std::move(task));
    } else {
        tasks_.emplace_back(taskIdentifier, std::move(task));
    }
}

void SequentialTaskQueue::SubmitBefore(
    uint64_t beforeIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    CORE_ASSERT(std::find(tasks_.cbegin(), tasks_.cend(), taskIdentifier) == tasks_.cend());

    auto it = std::find(tasks_.begin(), tasks_.end(), beforeIdentifier);
    if (it != tasks_.end()) {
        tasks_.emplace(it, taskIdentifier, std::move(task));
    }
}

void SequentialTaskQueue::Remove(uint64_t taskIdentifier)
{
    auto it = std::find(tasks_.cbegin(), tasks_.cend(), taskIdentifier);
    if (it != tasks_.cend()) {
        tasks_.erase(it);
    }
}

void SequentialTaskQueue::Clear()
{
    Wait();
    tasks_.clear();
}
CORE_END_NAMESPACE()
