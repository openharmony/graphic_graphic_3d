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

#ifndef API_CORE_THREADING_INTF_THREAD_POOL_H
#define API_CORE_THREADING_INTF_THREAD_POOL_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <core/plugin/intf_interface.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
/** \addtogroup group_threading
 *  @{
 */
/** Interface for thread safe thread pool.
 * ITask instances pushed to the queue are executed in different threads and completing can be waited with the returned
 * IResult.
 */
class IThreadPool : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "5b0d3810-cbcf-418a-a2e1-5df69fde1c09" };

    using Ptr = BASE_NS::refcnt_ptr<IThreadPool>;

    /** Interface for tasks that can be submitted to execution in the pool. */
    class ITask {
    public:
        virtual void operator()() = 0;

        struct Deleter {
            constexpr Deleter() noexcept = default;
            void operator()(ITask* ptr) const
            {
                ptr->Destroy();
            }
        };
        using Ptr = BASE_NS::unique_ptr<ITask, Deleter>;

    protected:
        virtual ~ITask() = default;
        virtual void Destroy() = 0;
    };

    /** Interface for result tasks that can be submitted to execution in the pool. */
    class IResult {
    public:
        virtual void Wait() = 0;
        virtual bool IsDone() const = 0;

        struct Deleter {
            constexpr Deleter() noexcept = default;
            void operator()(IResult* ptr) const
            {
                ptr->Destroy();
            }
        };
        using Ptr = BASE_NS::unique_ptr<IResult, Deleter>;

    protected:
        virtual ~IResult() = default;
        virtual void Destroy() = 0;
    };

    /** Adds a task to be executed.
     * @param task Pointer to a task instance.
     * @return Pointer to a result instance which can be waited
     */
    virtual IResult::Ptr Push(ITask::Ptr task) = 0;

    /** Adds a task to be executed.
     * @param task Pointer to a task instance.
     */
    virtual void PushNoWait(ITask::Ptr task) = 0;

    /** Get the number of threads this pool has.
     * @return Total number of threads in the pool.
     */
    virtual uint32_t GetNumberOfThreads() const = 0;

protected:
    virtual ~IThreadPool() = default;
};

class ITaskQueue {
public:
    /** Adds a task to be executed later.
     * @param taskIdentifier Identifier for the task.
     * @param task Pointer to a task instance.
     */
    virtual void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) = 0;

    /** Adds a task to be executed later with a dependency.
     * @param afterIdentifier Identifier of the task that must be executed prior this task. If the identifier is
     * unknown, the task will be added without dependency.
     * @param taskIdentifier Identifier for the task.
     * @param task Pointer to a task instance.
     */
    virtual void SubmitAfter(uint64_t afterIdentifier, uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) = 0;

    /** Adds a task to be executed later with multiple dependencies.
     * @param afterIdentifiers Identifiers of the tasks that must be executed prior this task. Unknown identifiers will
     * be skipped.
     * @param taskIdentifier Identifier for the task.
     * @param task Pointer to a task instance.
     */
    virtual void SubmitAfter(BASE_NS::array_view<const uint64_t> afterIdentifiers, uint64_t taskIdentifier,
        IThreadPool::ITask::Ptr&& task) = 0;

    /** Remove all tasks from queue. */
    virtual void Clear() = 0;

    /** Execute tasks in queue. */
    virtual void Execute() = 0;
};

/** Thread safe task dispatcher, executes one task per execute call and removes it once it is finished.
 */
class IDispatcherTaskQueue : public ITaskQueue {
public:
    /** Reports finished tasks, allows to check which tasks have been completed.
     * @return Task ids of tasks that have finished.
     */
    virtual BASE_NS::vector<uint64_t> CollectFinishedTasks() = 0;

    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IDispatcherTaskQueue* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IDispatcherTaskQueue, Deleter>;

protected:
    IDispatcherTaskQueue() = default;
    virtual ~IDispatcherTaskQueue() = default;
    IDispatcherTaskQueue(const IDispatcherTaskQueue& other) = delete;
    virtual void Destroy() = 0;
};

/** Non-thread safe parallel task queue, executes all tasks at once in parallel. This queue type is not thread safe and
 * should be only used from one thread.
 */
class IParallelTaskQueue : public ITaskQueue {
public:
    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IParallelTaskQueue* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IParallelTaskQueue, Deleter>;

protected:
    IParallelTaskQueue() = default;
    virtual ~IParallelTaskQueue() = default;
    IParallelTaskQueue(const IParallelTaskQueue& other) = delete;
    virtual void Destroy() = 0;
};

/**  Non-thread safe sequential task queue, executes tasks sequentially one-by-one. This queue type is not thread safe
 * and should be only used from one thread. */
class ISequentialTaskQueue : public ITaskQueue {
public:
    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(ISequentialTaskQueue* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<ISequentialTaskQueue, Deleter>;

protected:
    ISequentialTaskQueue() = default;
    virtual ~ISequentialTaskQueue() = default;
    ISequentialTaskQueue(const ISequentialTaskQueue& other) = delete;
    virtual void Destroy() = 0;
};

/** Factory for creating thread pools and task queues.
 */
class ITaskQueueFactory : public IInterface {
public:
    static constexpr BASE_NS::Uid UID { "5b0d3810-cbcf-418a-a2e1-5df69fde1c09" };

    using Ptr = BASE_NS::refcnt_ptr<ITaskQueueFactory>;

    /** Get the number of concurrent threads supported by the device.
     * @return Number of concurrent threads supported.
     */
    virtual uint32_t GetNumberOfCores() const = 0;

    /** Create a thread safe thread pool.
     * @param threadCount number of threads created in the pool.
     * @return Thread pool instance.
     */
    virtual IThreadPool::Ptr CreateThreadPool(const uint32_t threadCount) const = 0;

    /** Create a thread safe task dispatcher.
     * @param threadPool Optional thread pool.
     * @return Task dispatcher instance.
     */
    virtual IDispatcherTaskQueue::Ptr CreateDispatcherTaskQueue(const IThreadPool::Ptr& threadPool) const = 0;

    /** Create a non-thread safe parallel task queue.
     * @param threadPool Thread pool where tasks are executed in parallel.
     * @return Parallel task queue instance.
     */
    virtual IParallelTaskQueue::Ptr CreateParallelTaskQueue(const IThreadPool::Ptr& threadPool) const = 0;

    /** Create a non-thread safe sequential task queue.
     * @param threadPool Optional thread pool.
     * @return Sequential task queue instance.
     */
    virtual ISequentialTaskQueue::Ptr CreateSequentialTaskQueue(const IThreadPool::Ptr& threadPool) const = 0;

protected:
    virtual ~ITaskQueueFactory() = default;
};
/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_THREADING_INTF_THREAD_POOL_H
