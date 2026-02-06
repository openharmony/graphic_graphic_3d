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

#include "threading/task_queue_factory.h"

#include <algorithm>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <thread>

// need notice
#include <base/containers/array_view.h>
#include <base/containers/atomics.h>
#include <base/containers/iterator.h>
#include <base/containers/shared_ptr.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/math/mathf.h>
#include <base/util/uid.h>
#include <core/log.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/threading/intf_thread_pool.h>

#include "threading/dispatcher_impl.h"
#include "threading/parallel_impl.h"
#include "threading/sequential_impl.h"

#ifdef PLATFORM_HAS_JAVA
#include <os/java/java_internal.h>
#endif

CORE_BEGIN_NAMESPACE()
using BASE_NS::array_view;
using BASE_NS::make_unique;
using BASE_NS::move;
using BASE_NS::unique_ptr;
using BASE_NS::Math::max;

namespace {
constexpr uint32_t RES_TYPE_EXT_ENGINE_SET_QOS = 10028;

#ifdef PLATFORM_HAS_JAVA
/** RAII class for handling thread setup/release. */
class JavaThreadContext final {
public:
    JavaThreadContext()
    {
        JNIEnv* env = nullptr;
        javaVm_ = java_internal::GetJavaVM();

#ifndef NDEBUG
        // Check that the thread was not already attached.
        // It's not really a problem as another attach is a no-op, but we will be detaching the
        // thread later and it may be unexpected for the user.
        jint result = javaVm_->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
        CORE_ASSERT_MSG((result != JNI_OK), "Thread already attached");
#endif

        javaVm_->AttachCurrentThread(&env, nullptr);
    }

    ~JavaThreadContext()
    {
        javaVm_->DetachCurrentThread();
    }
    JavaVM* javaVm_ { nullptr };
};
#endif // PLATFORM_HAS_JAVA

// -- TaskResult, returned by ThreadPool::Push and can be waited on.
class TaskResult final : public IThreadPool::IResult {
public:
    // Task state which can be waited and marked as done.
    class State {
    public:
        void Done()
        {
            {
                auto lock = std::lock_guard(mutex_);
                done_ = true;
            }
            cv_.notify_all();
        }

        void Wait()
        {
            auto lock = std::unique_lock(mutex_);
            cv_.wait(lock, [this]() { return done_; });
        }

        bool IsDone() const
        {
            auto lock = std::lock_guard(mutex_);
            return done_;
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        bool done_ { false };
    };

    explicit TaskResult(BASE_NS::shared_ptr<State>&& future) : future_(BASE_NS::move(future)) {}

    void Wait() final
    {
        if (future_) {
            future_->Wait();
        }
    }
    bool IsDone() const final
    {
        if (future_) {
            return future_->IsDone();
        }
        return true;
    }

protected:
    void Destroy() final
    {
        delete this;
    }

private:
    BASE_NS::shared_ptr<State> future_;
};

// -- ThreadPool
class ThreadPool final : public IThreadPool {
public:
    explicit ThreadPool(size_t threadCount)
        : threadCount_(max(size_t(1), threadCount)), threads_(make_unique<ThreadContext[]>(threadCount_))
    {
        CORE_ASSERT(threads_);

        if (threadCount == 0U) {
            CORE_LOG_W("Threadpool minimum thread count is 1");
        }
        // Create thread containers.
        auto threads = array_view<ThreadContext>(threads_.get(), threadCount_);
        for (ThreadContext& context : threads) {
            // Set-up thread function.
            context.thread = std::thread(&ThreadPool::ThreadProc, this, std::ref(context));
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    IResult::Ptr Push(ITask::Ptr task) override
    {
        auto taskState = BASE_NS::make_shared<TaskResult::State>();
        if (taskState) {
            if (task) {
                std::lock_guard lock(mutex_);
                q_.push_back(BASE_NS::make_shared<Task>(BASE_NS::move(task), taskState));
                cv_.notify_one();
            } else {
                // mark as done if the there was no function.
                taskState->Done();
            }
        }
        return IResult::Ptr { new TaskResult(BASE_NS::move(taskState)) };
    }

    IResult::Ptr Push(ITask::Ptr task, BASE_NS::array_view<const ITask* const> dependencies) override
    {
        if (dependencies.empty()) {
            return Push(BASE_NS::move(task));
        }
        auto taskState = BASE_NS::make_shared<TaskResult::State>();
        if (taskState) {
            if (task) {
                BASE_NS::vector<BASE_NS::weak_ptr<Task>> deps;
                deps.reserve(dependencies.size());
                {
                    std::lock_guard lock(mutex_);
                    for (const ITask* dep : dependencies) {
                        if (auto pos = std::find_if(q_.cbegin(), q_.cend(),
                                [dep](const BASE_NS::shared_ptr<Task>& task) { return task && (*task == dep); });
                            pos != q_.cend()) {
                            deps.push_back(*pos);
                        }
                    }
                    q_.push_back(BASE_NS::make_shared<Task>(BASE_NS::move(task), taskState, BASE_NS::move(deps)));
                    cv_.notify_one();
                }
            } else {
                // mark as done if the there was no function.
                taskState->Done();
            }
        }
        return IResult::Ptr { new TaskResult(BASE_NS::move(taskState)) };
    }

    void PushNoWait(ITask::Ptr task) override
    {
        if (task) {
            std::lock_guard lock(mutex_);
            q_.push_back(BASE_NS::make_shared<Task>(BASE_NS::move(task)));
            cv_.notify_one();
        }
    }

    void PushNoWait(ITask::Ptr task, BASE_NS::array_view<const ITask* const> dependencies) override
    {
        if (dependencies.empty()) {
            return PushNoWait(BASE_NS::move(task));
        }

        if (task) {
            BASE_NS::vector<BASE_NS::weak_ptr<Task>> deps;
            deps.reserve(dependencies.size());
            {
                std::lock_guard lock(mutex_);
                for (const ITask* dep : dependencies) {
                    if (auto pos = std::find_if(q_.cbegin(), q_.cend(),
                        [dep](const BASE_NS::shared_ptr<Task>& task) { return task && (*task == dep); });
                        pos != q_.cend()) {
                        deps.push_back(*pos);
                    }
                }
                q_.push_back(BASE_NS::make_shared<Task>(BASE_NS::move(task), BASE_NS::move(deps)));
                cv_.notify_one();
            }
        }
    }

    uint32_t GetNumberOfThreads() const override
    {
        return static_cast<uint32_t>(threadCount_);
    }

    // IInterface
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        if ((uid == IThreadPool::UID) || (uid == IInterface::UID)) {
            return this;
        }
        return nullptr;
    }

    IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        if ((uid == IThreadPool::UID) || (uid == IInterface::UID)) {
            return this;
        }
        return nullptr;
    }

    void Ref() override
    {
        BASE_NS::AtomicIncrementRelaxed(&refcnt_);
    }

    void Unref() override
    {
        if (BASE_NS::AtomicDecrementRelease(&refcnt_) == 0) {
            BASE_NS::AtomicFenceAcquire();
            delete this;
        }
    }

protected:
    ~ThreadPool() final
    {
        Stop();
    }

private:
    struct ThreadContext {
        std::thread thread;
    };

    // Helper which holds a pointer to a queued task function and the result state.
    struct Task {
        ITask::Ptr function_;
        BASE_NS::shared_ptr<TaskResult::State> state_;
        BASE_NS::vector<BASE_NS::weak_ptr<Task>> dependencies_;
        bool running_ { false };

        ~Task() = default;
        Task() = default;

        Task(ITask::Ptr&& function, BASE_NS::shared_ptr<TaskResult::State> state,
            BASE_NS::vector<BASE_NS::weak_ptr<Task>>&& dependencies)
            : function_(BASE_NS::move(function)),
              state_(BASE_NS::move(state)), dependencies_ { BASE_NS::move(dependencies) }
        {
            CORE_ASSERT(this->function_ && this->state_);
        }

        Task(ITask::Ptr&& function, BASE_NS::shared_ptr<TaskResult::State> state)
            : function_(BASE_NS::move(function)), state_(BASE_NS::move(state))
        {
            CORE_ASSERT(this->function_ && this->state_);
        }

        explicit Task(ITask::Ptr&& function) : function_(BASE_NS::move(function))
        {
            CORE_ASSERT(this->function_);
        }

        Task(ITask::Ptr&& function, BASE_NS::vector<BASE_NS::weak_ptr<Task>>&& dependencies)
            : function_(BASE_NS::move(function)), dependencies_ { BASE_NS::move(dependencies) }
        {
            CORE_ASSERT(this->function_);
        }

        Task(Task&&) = default;
        Task& operator=(Task&&) = default;
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        inline void operator()() const
        {
            (*function_)();
            if (state_) {
                state_->Done();
            }
        }

        inline bool operator==(const ITask* task) const
        {
            return function_.get() == task;
        }

        // Task can run if it's not already running and there are no dependencies, or all the dependencies are ready.
        inline bool CanRun() const
        {
            return !running_ &&
                   (dependencies_.empty() ||
                       std::all_of(std::begin(dependencies_), std::end(dependencies_),
                           [](const BASE_NS::weak_ptr<Task>& dependency) { return dependency.expired(); }));
        }
    };

    // Looks for a task that can be executed.
    BASE_NS::shared_ptr<Task> FindRunnable()
    {
        if (q_.empty()) {
            return {};
        }
        for (auto& task : q_) {
            if (task && task->CanRun()) {
                task->running_ = true;
                return task;
            }
        }
        return {};
    }

    void Clear()
    {
        std::lock_guard lock(mutex_);
        q_.clear();
    }

    // At the moment Stop is called only from the destructor with waitForAllTasksToComplete=true.
    // If this doesn't change the class can be simplified a bit.
    void Stop()
    {
        // Wait all tasks to complete before returning.
        if (isDone_) {
            return;
        }
        {
            std::lock_guard lock(mutex_);
            isDone_ = true;
        }

        // Trigger all waiting threads.
        cv_.notify_all();

        // Wait for all threads to finish.
        auto threads = array_view(threads_.get(), threadCount_);
        for (auto& context : threads) {
            if (context.thread.joinable()) {
                context.thread.join();
            }
        }

        Clear();
    }

    void ThreadProc(ThreadContext& context)
    {
#ifdef PLATFORM_HAS_JAVA
        // RAII class for handling thread setup/release.
        JavaThreadContext javaContext;
#endif

        while (true) {
            // Function to process.
            BASE_NS::shared_ptr<Task> task;
            {
                std::unique_lock lock(mutex_);

                // Try to wait for next task to process.
                cv_.wait(lock, [this, &task]() {
                    task = FindRunnable();
                    return task || isDone_;
                });
            }
            // If there was no task it means we are stopping and thread can exit.
            if (!task) {
            // need notice
                return;
            }

            while (task) {
                // Run task function.
                {
                    CORE_CPU_PERF_SCOPE("CORE", "ThreadPoolTask", "", CORE_PROFILER_DEFAULT_COLOR);
                    (*task)();
                }

                std::lock_guard lock(mutex_);
                // After running the task remove it from the queue. Any dependent tasks will see their weak_ptr expire
                // idicating that the dependency has been completed.
                if (auto pos = std::find_if(q_.cbegin(), q_.cend(),
                        [&task](const BASE_NS::shared_ptr<Task>& queuedTask) { return queuedTask == task; });
                    pos != q_.cend()) {
                    q_.erase(pos);
                }
                task.reset();

                // Get next function.
                if (auto pos = std::find_if(std::begin(q_), std::end(q_),
                        [](const BASE_NS::shared_ptr<Task>& task) { return (task) && (task->CanRun()); });
                    pos != std::end(q_)) {
                    task = *pos;
                    task->running_ = true;
                    // Check if there are more runnable tasks and notify workers as needed.
                    auto runnable = std::min(static_cast<ptrdiff_t>(threadCount_),
                        std::count_if(pos + 1, std::end(q_),
                            [](const BASE_NS::shared_ptr<Task>& task) { return (task) && (task->CanRun()); }));
                    while (runnable--) {
                        cv_.notify_one();
                    }
                }
            }
        }
    }

    size_t threadCount_ { 0 };
    unique_ptr<ThreadContext[]> threads_;

    std::deque<BASE_NS::shared_ptr<Task>> q_;

    bool isDone_ { false };

    std::mutex mutex_;
    std::condition_variable cv_;
    int32_t refcnt_ { 0 };
};
} // namespace

uint32_t TaskQueueFactory::GetNumberOfCores() const
{
    uint32_t result = std::thread::hardware_concurrency();
    if (result == 0) {
        // If not detectable, default to 4.
        result = 4;
    }

    return result;
}

IThreadPool::Ptr TaskQueueFactory::CreateThreadPool(const uint32_t threadCount) const
{
    return IThreadPool::Ptr { new ThreadPool(threadCount) };
}

IDispatcherTaskQueue::Ptr TaskQueueFactory::CreateDispatcherTaskQueue(const IThreadPool::Ptr& threadPool) const
{
    return IDispatcherTaskQueue::Ptr { make_unique<DispatcherImpl>(threadPool).release() };
}

IParallelTaskQueue::Ptr TaskQueueFactory::CreateParallelTaskQueue(const IThreadPool::Ptr& threadPool) const
{
    return IParallelTaskQueue::Ptr { make_unique<ParallelImpl>(threadPool).release() };
}

ISequentialTaskQueue::Ptr TaskQueueFactory::CreateSequentialTaskQueue(const IThreadPool::Ptr& threadPool) const
{
    return ISequentialTaskQueue::Ptr { make_unique<SequentialImpl>(threadPool).release() };
}

// IInterface
const IInterface* TaskQueueFactory::GetInterface(const BASE_NS::Uid& uid) const
{
    if (uid == ITaskQueueFactory::UID) {
        return static_cast<const ITaskQueueFactory*>(this);
    }
    return nullptr;
}

IInterface* TaskQueueFactory::GetInterface(const BASE_NS::Uid& uid)
{
    if (uid == ITaskQueueFactory::UID) {
        return static_cast<ITaskQueueFactory*>(this);
    }
    return nullptr;
}

void TaskQueueFactory::Ref() {}

void TaskQueueFactory::Unref() {}
CORE_END_NAMESPACE()
