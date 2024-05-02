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

#ifndef META_API_TIMER_H
#define META_API_TIMER_H

#include <meta/api/deferred_callback.h>
#include <meta/api/make_callback.h>
#include <meta/api/threading/mutex.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Helper class to launch timers, this type is not copyable, only movable. Destructor will stop the timer.
 */
class Timer {
public:
    /**
     * @brief Type of the timer, single shot happens only once when as recurring keeps triggering after given timeout
     */
    enum TimerType { SINGLE_SHOT, RECURRING };

    Timer() = default;
    Timer(const Timer&) = delete;
    Timer(Timer&& t) : control_(BASE_NS::move(t.control_)) {}
    ~Timer()
    {
        Stop();
    }

    /**
     * @brief Constructor that calls corresponding Start function (See Start below).
     */
    template<typename Func>
    Timer(const TimeSpan& interval, Func func, TimerType type, const ITaskQueue::Ptr& queue)
    {
        Start(interval, BASE_NS::move(func), type, queue);
    }

    /**
     * @brief Constructor that calls corresponding Start function (See Start below).
     */
    template<typename Func>
    Timer(const TimeSpan& interval, Func func, TimerType type, const BASE_NS::Uid& queueId)
    {
        Start(interval, BASE_NS::move(func), type, queueId);
    }

    Timer& operator=(const Timer&) = delete;
    Timer& operator=(Timer&& t)
    {
        Stop();
        control_ = BASE_NS::move(t.control_);
        return *this;
    }

    /**
     * @brief Start timer.
     * @param interval Time interval which after this timer triggers.
     * @param func Callable entity (e.g. lambda) which is called.
     * @param type Type of the timer.
     * @param queue Queue to which the timer task is posted to (this dictates on what thread the func is called).
     */
    template<typename Func>
    bool Start(const TimeSpan& interval, Func func, TimerType type, const ITaskQueue::Ptr& queue)
    {
        if (!queue) {
            return false;
        }
        Stop();
        control_ = CreateShared<Control>();
        control_->queue = queue;
        control_->token =
            queue->AddTask(MakeCallback<ITaskQueueTask>([control = control_, type, callback = BASE_NS::move(func)] {
                callback();
                bool ret = type == RECURRING;
                if (!ret) {
                    if (control) {
                        CORE_NS::UniqueLock lock { control->mutex };
                        control->token = {};
                    }
                }
                return ret;
            }),
                interval);
        return true;
    }

    /**
     * @brief Start timer.
     * @param interval Time interval which after this timer triggers.
     * @param func Callable entity (e.g. lambda) which is called.
     * @param type Type of the timer.
     * @param queueId Uid of queue to which the timer task is posted to (this dictates on what thread the func is
     * called).
     */
    template<typename Func>
    bool Start(const TimeSpan& interval, Func func, TimerType type, const BASE_NS::Uid& queueId)
    {
        return Start(interval, BASE_NS::move(func), type, GetTaskQueueRegistry().GetTaskQueue(queueId));
    }

    /**
     * @brief Stop timer
     */
    void Stop()
    {
        if (control_) {
            CORE_NS::UniqueLock lock { control_->mutex };
            if (control_->queue && control_->token) {
                control_->queue->CancelTask(control_->token);
            }
        }
        control_.reset();
    }

    /**
     * @brief Detach timer, this can be used if you don't want destructor to stop the timer
     */
    ITaskQueue::Token Detach()
    {
        ITaskQueue::Token ret {};
        if (control_) {
            CORE_NS::UniqueLock lock { control_->mutex };
            ret = control_->token;
        }
        control_.reset();
        return ret;
    }

    /**
     * @brief Returns true if the timer is currently running
     */
    bool IsRunning() const
    {
        if (control_) {
            CORE_NS::UniqueLock lock { control_->mutex };
            return control_->token != ITaskQueue::Token {};
        }
        return false;
    }

private:
    struct Control {
        mutable CORE_NS::Mutex mutex;
        ITaskQueue::Ptr queue;
        ITaskQueue::Token token {};
    };
    BASE_NS::shared_ptr<Control> control_;
};

/**
 * @brief Start single shot timer, returns the token that can be used to cancel it (which can be just ignored).
 * @param interval Time interval which after this timer triggers.
 * @param func Callable entity (e.g. lambda) which is called.
 * @param queue Queue to which the timer task is posted to (this dictates on what thread the func is called).
 */
template<typename Func>
inline ITaskQueue::Token SingleShotTimer(const TimeSpan& interval, Func func, const ITaskQueue::Ptr& queue)
{
    Timer t;
    t.Start(interval, BASE_NS::move(func), Timer::SINGLE_SHOT, queue);
    return t.Detach();
}

/**
 * @brief Start single shot timer, returns the token that can be used to cancel it (which can be just ignored).
 * @param interval Time interval which after this timer triggers.
 * @param func Callable entity (e.g. lambda) which is called.
 * @param queueId Uid of queue to which the timer task is posted to (this dictates on what thread the func is called).
 */
template<typename Func>
inline ITaskQueue::Token SingleShotTimer(const TimeSpan& interval, Func func, const BASE_NS::Uid& queueId)
{
    Timer t;
    t.Start(interval, BASE_NS::move(func), Timer::SINGLE_SHOT, queueId);
    return t.Detach();
}

META_END_NAMESPACE()

#endif
