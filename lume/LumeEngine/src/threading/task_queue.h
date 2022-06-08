/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_THREADING_TASK_QUEUE_H
#define CORE_THREADING_TASK_QUEUE_H

#include <atomic>
#include <functional>

#include <base/containers/vector.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

CORE_BEGIN_NAMESPACE()

// Helper class for running std::function as a ThreadPool task.
class FunctionTask final : public IThreadPool::ITask {
public:
    static Ptr Create(std::function<void()> func)
    {
        return Ptr { new FunctionTask(func) };
    }

    explicit FunctionTask(std::function<void()> func) : func_(func) {};

    void operator()() override
    {
        func_();
    }

protected:
    void Destroy() override
    {
        delete this;
    }

private:
    std::function<void()> func_;
};

// Abstract base class for task queues.
/* Usage examples:
 *  1. SequentialTaskQueue queue(threadManager.GetThreadPool());
 *  2. queue.Submit("identifier A", FunctionTask::Create(function));
 *  3. queue.Submit("identifier B", FunctionTask::Create(std::bind(&Classname::function, this)));
 *  4. queue.Submit("identifier C", FunctionTask::Create([]() { <code> }));
 */
class TaskQueue {
public:
    /** Constructor for the task queue.
        @param aThreadPool Optional thread pool, if support for threading is desired.
    */
    explicit TaskQueue(const IThreadPool::Ptr& threadPool);
    TaskQueue(const TaskQueue& other) = delete;
    TaskQueue& operator=(const TaskQueue& other) = delete;

    virtual ~TaskQueue();

    /** Submit task to end of execution queue.
        @param taskIdentifier Identifier of the task, must be unique.
        @param task Task to execute.
    */
    virtual void Submit(uint64_t taskIdentifier, IThreadPool::ITask::Ptr&& task) = 0;

    /** Remove task from execution queue.
        @param taskIdentifier Identifier of the task.
    */
    virtual void Remove(uint64_t taskIdentifier) = 0;

    /** Remove all tasks from execution queue. */
    virtual void Clear() = 0;

    /** Execute task queue in this thread. */
    virtual void Execute() = 0;

    /** Execute task queue asynchronously in new thread. */
    virtual void ExecuteAsync();

    /** Checks if task queue is running asynchronously.
        @return True if task queue is currently running and can't be re-executed.
    */
    bool IsRunningAsync() const;

    /** Waits until task queue has completed asynchronous execution */
    void Wait();

protected:
    class ExecuteAsyncTask final : public IThreadPool::ITask {
    public:
        explicit ExecuteAsyncTask(TaskQueue& queue);
        void operator()() override;

    protected:
        void Destroy() override;

    private:
        TaskQueue& queue_;
    };

    struct Entry {
        Entry();
        Entry(uint64_t identifier, IThreadPool::ITask::Ptr task);
        bool operator==(uint64_t identifier) const;
        bool operator==(const Entry& other) const;

        IThreadPool::ITask::Ptr task;
        uint64_t identifier;
        BASE_NS::vector<uint64_t> dependencies;
    };

    IThreadPool::Ptr threadPool_;
    IThreadPool::IResult::Ptr asyncOperation_;
    std::atomic_bool isRunningAsync_;
};
CORE_END_NAMESPACE()

#endif // CORE_THREADING_TASK_QUEUE_H
