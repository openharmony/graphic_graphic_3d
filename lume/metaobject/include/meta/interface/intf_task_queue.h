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
#ifndef META_INTERFACE_ITASKQUEUE_H
#define META_INTERFACE_ITASKQUEUE_H

#include <meta/base/time_span.h>
#include <meta/base/types.h>
#include <meta/interface/intf_callable.h>
#include <meta/interface/intf_clock.h>
#include <meta/interface/intf_future.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ITaskQueueTask, "64fd1333-f59b-4f96-901c-1a7efa50387c")
META_REGISTER_INTERFACE(ITaskQueueWaitableTask, "cf2ef384-b14f-4bb7-acda-2c4b0bfa4917")
META_REGISTER_INTERFACE(ITaskScheduleInfo, "d9fe0fb1-84dd-4210-bf33-6fdbe650615b")
META_REGISTER_INTERFACE(ITaskQueue, "138b0a6d-4a97-4ad6-bba3-1bee7cc36d58")
META_REGISTER_INTERFACE(IPollingTaskQueue, "16e007c1-f8f8-4e9d-b5d7-d7016f9c54d3")
META_REGISTER_INTERFACE(IThreadedTaskQueue, "42be1ec0-5711-4377-aa40-5270be31ad7d")

/**
 * @brief The ITaskQueueTask interface defines the interface which a class
 *        must implement to be schedulable in a task queue.
 */
class ITaskQueueTask : public META_NS::ICallable {
    META_INTERFACE(META_NS::ICallable, ITaskQueueTask);

public:
    using FunctionType = bool();
    /**
     * @brief Called by the task queue to execute the task.
     * @return If Invoke returns true, the task is rescheduled in the same task queue.
     */
    virtual bool Invoke() = 0;
};

/**
 * @brief The ITaskQueueWaitableTask defines the interface which a class
 *        must implement to be schedulable as a waitable task in a task queue.
 */
class ITaskQueueWaitableTask : public META_NS::ICallable {
    META_INTERFACE(META_NS::ICallable, ITaskQueueWaitableTask);

public:
    using FunctionType = IAny::Ptr();
    /**
     * @brief Called by the task queue to execute the task.
     * @return Result of the task.
     */
    virtual IAny::Ptr Invoke() = 0;
};

/**
 * @brief The ITaskQueue interface defines the interface for a class which
 *        implements a task queue where tasks can be scheduled.
 */
class ITaskQueue : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITaskQueue);

public:
    using CallableType = ITaskQueueTask;
    using Token = const void*;
    /**
     * @brief Add a task to the task queue and execute the task as soon as possible.
     * @param p The task to execute.
     * @return Queue token for the task. The token can be used to cancel the task.
     */
    virtual Token AddTask(ITaskQueueTask::Ptr p) = 0;
    /**
     * @brief Add a task to the task queue and execute it after a given delay.
     * @note The delay defines the minimum delay after which the task is executed.
     *       Depending on the task queue implementation and platform specifics the
     *       task may actually be executed substantially later than the given delay.
     * @param p The task to execute.
     * @param delay Do not run the task earlier than this delay.
     * @return Queue token for the task. The token can be used to cancel the task.
     */
    virtual Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay) = 0;
    /**
     * @brief Add a task to the task queue and return an object which can be
     *        used to wait for the task to complete.
     * @param p The task to execute.
     * @return A future object which can be waited on.
     */
    virtual IFuture::Ptr AddWaitableTask(ITaskQueueWaitableTask::Ptr p) = 0;
    /**
     * @brief Cancel a task.
     * @note If task is already running, waits for completion (unless the call to cancel comes from the
     *       task itself).
     *       If the task has not been started yet, it is removed from the queue immediately.
     * @param token The queue token of the task to cancel.
     */
    virtual void CancelTask(Token) = 0;
};

/**
 * @brief The IPollingTaskQueue interface defines an interface to be implemented
 *        by task queues which must be manually processed by the application.
 */
class IPollingTaskQueue : public ITaskQueue {
    META_INTERFACE(ITaskQueue, IPollingTaskQueue);

public:
    /**
     * @brief Execute all queued tasks. The function returns once all of the tests
     *        have been executed.
     * @note  If a task reschedules itself while being run, it will not be run a second
     *        time until ProcessTasks() is called again.
     */
    virtual void ProcessTasks() = 0;
};

/**
 * @brief The IThreadedTaskQueue interface defines an interface to be implemented
 *        by tasks queues which run their tasks in a separate thread.
 */
class IThreadedTaskQueue : public ITaskQueue {
    META_INTERFACE(ITaskQueue, IThreadedTaskQueue);

public:
    /** Interface defined for completeness
        Possibly add methods to wait for the queue empty */
};

/**
 * @brief The ITaskScheduleInfo interface is used to set the task queue where this token is
 *  scheduled to run with the token.
 */
class ITaskScheduleInfo : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITaskScheduleInfo);

public:
    virtual void SetQueueAndToken(const ITaskQueue::Ptr&, ITaskQueue::Token) = 0;
};

/** Built-in task queue implementations */
META_REGISTER_CLASS(PollingTaskQueue, "d4a5944e-db40-4603-834e-457a21123e2c", ObjectCategoryBits::NO_CATEGORY)
META_REGISTER_CLASS(ThreadedTaskQueue, "009bf37a-d490-4a01-b7bb-f3365cc0a8da", ObjectCategoryBits::NO_CATEGORY)

META_END_NAMESPACE()

#endif // META_INTERFACE_ITASKQUEUE_H
