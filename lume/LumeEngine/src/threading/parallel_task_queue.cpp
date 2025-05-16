/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "threading/parallel_task_queue.h"

#include <algorithm>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::unordered_map;
using BASE_NS::vector;

// -- Parallel task queue.
ParallelTaskQueue::ParallelTaskQueue(const IThreadPool::Ptr& threadPool) : TaskQueue(threadPool) {}

ParallelTaskQueue::~ParallelTaskQueue()
{
    Wait();
}

void ParallelTaskQueue::Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    CORE_ASSERT(std::find(tasks_.cbegin(), tasks_.cend(), taskIdentifier) == tasks_.cend());

    tasks_.emplace_back(taskIdentifier, std::move(task));
}

void ParallelTaskQueue::SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    CORE_ASSERT(std::find(tasks_.cbegin(), tasks_.cend(), taskIdentifier) == tasks_.cend());

    auto it = std::find(tasks_.begin(), tasks_.end(), afterIdentifier);
    if (it != tasks_.end()) {
        Entry entry(taskIdentifier, std::move(task));
        entry.dependencies.push_back(afterIdentifier);

        tasks_.push_back(std::move(entry));
    } else {
        tasks_.emplace_back(taskIdentifier, std::move(task));
    }
}

void ParallelTaskQueue::SubmitAfter(
    array_view<const uint64_t> afterIdentifiers, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task)
{
    if (std::all_of(
            afterIdentifiers.cbegin(), afterIdentifiers.cend(), [&tasks = tasks_](const uint64_t afterIdentifier) {
                return std::any_of(tasks.cbegin(), tasks.cend(),
                    [afterIdentifier](const TaskQueue::Entry& entry) { return entry.identifier == afterIdentifier; });
            })) {
        Entry entry(taskIdentifier, std::move(task));
        entry.dependencies.insert(entry.dependencies.cend(), afterIdentifiers.begin(), afterIdentifiers.end());

        tasks_.push_back(std::move(entry));
    } else {
        tasks_.emplace_back(taskIdentifier, std::move(task));
    }
}

void ParallelTaskQueue::Remove(uint64_t taskIdentifier)
{
    auto it = std::find(tasks_.cbegin(), tasks_.cend(), taskIdentifier);
    if (it != tasks_.cend()) {
        tasks_.erase(it);
    }
}

void ParallelTaskQueue::Clear()
{
    Wait();
    tasks_.clear();
}

void ParallelTaskQueue::Execute()
{
#if (CORE_VALIDATION_ENABLED == 1)
    // NOTE: Check the integrity of the task queue (no circular deps etc.)
#endif
    // gather dependencies for each task
    vector<vector<const CORE_NS::IThreadPool::ITask*>> dependencies;
    dependencies.reserve(tasks_.size());
    for (auto& task : tasks_) {
        auto& deps = dependencies.emplace_back();
        for (const auto& dependency : task.dependencies) {
            if (auto pos = std::find_if(tasks_.cbegin(), tasks_.cend(),
                                        [dependency](const Entry &entry) { return entry.identifier == dependency; });
                pos != tasks_.cend()) {
                deps.push_back(pos->task.get());
            }
        }
    }

    // submit each task with its dependency information. threadpool will run a task when the dependencies are ready. now
    // we have an IResult for every task, but we could use PushNowWait for tasks that are leafs.
    vector<CORE_NS::IThreadPool::IResult::Ptr> states;
    states.reserve(tasks_.size());
    std::transform(std::begin(tasks_), std::end(tasks_), std::begin(dependencies), std::back_inserter(states),
        [this](TaskQueue::Entry& entry, const vector<const CORE_NS::IThreadPool::ITask*>& dependencies) {
            if (dependencies.empty()) {
                return threadPool_->Push(BASE_NS::move(entry.task));
            }
            return threadPool_->Push(BASE_NS::move(entry.task), dependencies);
        });
    tasks_.clear();

    // wait for tasks to complete.
    for (const auto& state : states) {
        state->Wait();
    }
}
CORE_END_NAMESPACE()
