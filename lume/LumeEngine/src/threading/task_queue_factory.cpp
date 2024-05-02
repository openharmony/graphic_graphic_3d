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

#include "threading/task_queue_factory.h"

#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/util/uid.h>
#include <core/log.h>
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

namespace {
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

    explicit TaskResult(std::shared_ptr<State>&& future) : future_(BASE_NS::move(future)) {}

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
    std::shared_ptr<State> future_;
};

// -- ThreadPool
class ThreadPool final : public IThreadPool {
public:
    explicit ThreadPool(size_t threadCount)
        : threadCount_(threadCount), threads_(make_unique<ThreadContext[]>(threadCount))
    {
        CORE_ASSERT(threads_);

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

    IResult::Ptr Push(ITask::Ptr function) override
    {
        auto taskState = std::make_shared<TaskResult::State>();
        if (taskState) {
            if (function) {
                {
                    std::lock_guard lock(mutex_);
                    q_.Push(Task(move(function), taskState));
                }
                cv_.notify_one();
            } else {
                // mark as done if the there was no function.
                taskState->Done();
            }
        }
        return IResult::Ptr { new TaskResult(BASE_NS::move(taskState)) };
    }

    void PushNoWait(ITask::Ptr function) override
    {
        if (function) {
            {
                std::lock_guard lock(mutex_);
                q_.Push(Task(move(function)));
            }
            cv_.notify_one();
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
        refcnt_.fetch_add(1, std::memory_order_relaxed);
    }

    void Unref() override
    {
        if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }

protected:
    ~ThreadPool() final
    {
        Stop(true);
    }

private:
    // Helper which holds a pointer to a queued task function and the result state.
    struct Task {
        ITask::Ptr function_;
        std::shared_ptr<TaskResult::State> state_;

        ~Task() = default;
        Task() = default;
        explicit Task(ITask::Ptr&& function, std::shared_ptr<TaskResult::State> state)
            : function_(move(function)), state_(CORE_NS::move(state))
        {
            CORE_ASSERT(this->function_ && this->state_);
        }
        explicit Task(ITask::Ptr&& function) : function_(move(function))
        {
            CORE_ASSERT(this->function_);
        }
        Task(Task&&) = default;
        Task& operator=(Task&&) = default;
        Task(const Task&) = delete;
        Task& operator=(const Task&) = delete;

        void operator()() const
        {
            (*function_)();
            if (state_) {
                state_->Done();
            }
        }
    };

    template<typename T>
    class Queue {
    public:
        bool Push(T&& value)
        {
            q_.push(move(value));
            return true;
        }

        bool Pop(T& v)
        {
            if (q_.empty()) {
                v = {};
                return false;
            }
            v = CORE_NS::move(q_.front());
            q_.pop();
            return true;
        }

    private:
        std::queue<T> q_;
    };

    struct ThreadContext {
        std::thread thread;
        bool exit { false };
    };

    void Clear()
    {
        Task f;
        std::lock_guard lock(mutex_);
        while (q_.Pop(f)) {
            // Intentionally empty.
        }
    }

    // At the moment Stop is called only from the destructor with waitForAllTasksToComplete=true.
    // If this doesn't change the class can be simplified a bit.
    void Stop(bool waitForAllTasksToComplete)
    {
        if (isStop_) {
            return;
        }
        if (waitForAllTasksToComplete) {
            // Wait all tasks to complete before returning.
            if (isDone_) {
                return;
            }
            std::lock_guard lock(mutex_);
            isDone_ = true;
        } else {
            isStop_ = true;

            // Ask all the threads to stop and not process any more tasks.
            auto threads = array_view(threads_.get(), threadCount_);
            {
                auto lock = std::lock_guard(mutex_);
                for (auto& context : threads) {
                    context.exit = true;
                }
            }
            Clear();
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

        // Get function to process.
        Task func;
        bool isPop = [this, &func]() {
            std::lock_guard lock(mutex_);
            return q_.Pop(func);
        }();

        while (true) {
            while (isPop) {
                // Run task function.
                func();

                // If the thread is wanted to stop, return even if the queue is not empty yet.
                std::lock_guard lock(mutex_);
                if (context.exit) {
                    return;
                }

                // Get next function.
                isPop = q_.Pop(func);
            }

            // The queue is empty here, wait for the next task.
            std::unique_lock lock(mutex_);

            // Try to wait for next task to process.
            cv_.wait(lock, [this, &func, &isPop, &context]() {
                isPop = q_.Pop(func);
                return isPop || isDone_ || context.exit;
            });

            if (!isPop) {
                return;
            }
        }
    }

    size_t threadCount_ { 0 };
    unique_ptr<ThreadContext[]> threads_;

    Queue<Task> q_;
    bool isDone_ { false };
    bool isStop_ { false };

    std::mutex mutex_;
    std::condition_variable cv_;
    std::atomic<int32_t> refcnt_ { 0 };
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
