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

#include "threading/parallel_task_queue.h"

#include <algorithm>
#include <condition_variable>
#include <mutex>

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

struct ParallelTaskQueue::TaskState {
    unordered_map<uint64_t, bool> finished;
    std::condition_variable cv;
    std::mutex mutex;
};

class ParallelTaskQueue::Task final : public IThreadPool::ITask {
public:
    explicit Task(TaskState& state, IThreadPool::ITask& task, uint64_t id);

    void operator()() override;

protected:
    void Destroy() override;

private:
    TaskState& state_;
    IThreadPool::ITask& task_;
    uint64_t id_;
};

ParallelTaskQueue::Task::Task(TaskState& state, IThreadPool::ITask& task, uint64_t id)
    : state_(state), task_(task), id_(id)
{}

void ParallelTaskQueue::Task::operator()()
{
    // Run task.
    task_();

    // Mark task as completed.
    std::unique_lock lock(state_.mutex);
    state_.finished[id_] = true;

    // Notify that there is completed task.
    state_.cv.notify_one();
}

void ParallelTaskQueue::Task::Destroy()
{
    delete this;
}

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

void ParallelTaskQueue::QueueTasks(vector<size_t>& waiting, TaskState& state)
{
    if (waiting.empty()) {
        // No more tasks to proecss.
        return;
    }

    for (vector<size_t>::const_iterator it = waiting.cbegin(); it != waiting.cend();) {
        // Entry to handle.
        Entry& entry = tasks_[*it];

        // Can run this task?
        bool canRun = true;
        for (const auto& dep : entry.dependencies) {
            if (!state.finished.contains(dep)) {
                // Task that is marked as dependency is not executed yet.
                canRun = false;
                break;
            }
        }

        if (canRun) {
            // This task can be executed.
            // Remove task from waiting list.
            it = waiting.erase(it);

            // Push to execution queue.
            threadPool_->PushNoWait(IThreadPool::ITask::Ptr { new Task(state, *entry.task, entry.identifier) });
        } else {
            ++it;
        }
    }
}

void ParallelTaskQueue::Execute()
{
#if (CORE_VALIDATION_ENABLED == 1)
    // NOTE: Check the integrity of the task queue (no circular deps etc.)
#endif
    vector<size_t> waiting;
    waiting.resize(tasks_.size());
    for (size_t i = 0; i < tasks_.size(); ++i) {
        waiting[i] = i;
    }

    TaskState state;
    state.finished.reserve(tasks_.size());

    {
        // Keep on pushing tasks to queue until all done.
        std::unique_lock lock(state.mutex);
        state.cv.wait(lock, [this, &waiting, &state]() {
            // Push new tasks to queue.
            QueueTasks(waiting, state);
            return state.finished.size() == tasks_.size();
        });
    }
}
CORE_END_NAMESPACE()
