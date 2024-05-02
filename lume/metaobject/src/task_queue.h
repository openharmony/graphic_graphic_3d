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

#ifndef META_SRC_TASK_QUEUE_H
#define META_SRC_TASK_QUEUE_H

#include <mutex>
#include <thread>

#include <base/containers/vector.h>

#include <meta/interface/intf_clock.h>
#include <meta/interface/intf_task_queue.h>

META_BEGIN_NAMESPACE()

class TaskQueueImpl : public ITaskQueueExtend {
public:
    using Token = ITaskQueue::Token;

    void SetExtend(ITaskQueueExtend* extend) override
    {
        extend_ = extend ? extend : this;
    }
    void Shutdown() override
    {
        Close();
    }

    void CancelTask(Token token)
    {
        if (token != nullptr) {
            std::unique_lock lock { mutex_ };
            Token executingToken = execToken_;
            if (token == execToken_) {
                // Currently executing task is requested to cancel.
                // Tasks are temporarily removed from the queue while execution, so the currently running task is not in
                // the queue anymore. Setting execToken_ to null will cause the task to not be re-added.
                execToken_ = nullptr;
            }

            // If we are currently executing the task in different thread, wait for it to complete.
            if (std::this_thread::get_id() != execThread_) {
                while (!terminate_ && token == executingToken) {
                    lock.unlock();
                    // sleep a bit.
                    std::this_thread::yield();
                    lock.lock();
                    executingToken = execToken_;
                }
            }

            // Remove all tasks from the queue, with the same token. (if any)
            // One can push the same task to the queue multiple times currently.
            // (ie. you "can" schedule the same task with different "delays")
            // So we remove all scheduled tasks with same token.
            // Also redo/rearm might have add the task back while we were waiting/yielding.
            for (auto it = tasks_.begin(); it != tasks_.end();) {
                if (it->operation.get() == token) {
                    it = tasks_.erase(it);
                } else {
                    it++;
                }
            }
        }
    }

    Token AddTaskImpl(ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime)
    {
        Token ret { p.get() };

        if (auto i = interface_cast<ITaskScheduleInfo>(p)) {
            i->SetQueueAndToken(self_.lock(), ret);
        }

        // insertion sort the tasks
        if (tasks_.empty()) {
            tasks_.emplace_back(delay, excTime, BASE_NS::move(p));
        } else if (tasks_.size() == 1) {
            if (tasks_.front().executeTime >= excTime) {
                tasks_.emplace_back(delay, excTime, BASE_NS::move(p));
            } else {
                tasks_.insert(tasks_.begin(), { delay, excTime, BASE_NS::move(p) });
            }
        } else {
            bool found = false;
            for (auto it = tasks_.end() - 1; it >= tasks_.begin(); --it) {
                if (it->executeTime > excTime) {
                    // task in list should execute after us, so insert there.
                    tasks_.insert(it + 1, { delay, excTime, BASE_NS::move(p) });
                    found = true;
                    break;
                }
            }
            if (!found) {
                // add last then ..
                tasks_.insert(tasks_.begin(), { delay, excTime, BASE_NS::move(p) });
            }
        }
        return ret;
    }

    Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay, const TimeSpan& excTime)
    {
        if (p) {
            std::unique_lock lock { mutex_ };
            return AddTaskImpl(BASE_NS::move(p), delay, excTime);
        }
        return nullptr;
    }

    TimeSpan Time() const
    {
        using namespace std::chrono;
        return TimeSpan::Microseconds(
            duration_cast<microseconds>(high_resolution_clock::now().time_since_epoch()).count());
    }

    void ProcessTasks(std::unique_lock<std::mutex>& lock, TimeSpan curTime)
    {
        // Must only be called while having the lock
        BASE_NS::vector<Task> rearm;
        while (!terminate_ && !tasks_.empty() && curTime >= tasks_.back().executeTime) {
            auto task = BASE_NS::move(tasks_.back());
            tasks_.pop_back();
            execToken_ = task.operation.get();
            lock.unlock();
            bool redo = extend_->InvokeTask(task.operation);
            lock.lock();
            // Note execToken_ has been set to null if the executing task is cancelled.
            if ((redo) && (execToken_ != nullptr)) {
                // Reschedule the task again.
                rearm.emplace_back(BASE_NS::move(task));
            }
            execToken_ = nullptr;
        }

        // rearm the tasks.. (if we are not shutting down)
        if (!terminate_) {
            for (auto it = rearm.rbegin(); it != rearm.rend(); ++it) {
                auto& task = *it;
                if (task.delay > TimeSpan()) {
                    // calculate the next executeTime in phase.. (ie. how many events missed)
                    uint64_t dt = task.delay.ToMicroseconds();
                    uint64_t et = task.executeTime.ToMicroseconds();
                    uint64_t ct = curTime.ToMicroseconds();
                    // calculate the next executeTime in phase..
                    et += dt;
                    if (et <= ct) {
                        // "ticks" how many events would have ran.. (rounded up)
                        auto ticks = ((ct - et) + (dt - 1)) / dt;
                        // and based on the "ticks" we can now count the next execution time.
                        et += (ticks * dt);
                        CORE_LOG_V("Skipped ticks %d", (int)ticks);
                    }
                    task.executeTime = TimeSpan::Microseconds(et);
                } else {
                    task.executeTime = curTime;
                }
                AddTaskImpl(task.operation, task.delay, task.executeTime);
            }
        }
    }

    void Close()
    {
        std::unique_lock lock { mutex_ };
        terminate_ = true;
        tasks_.clear();
    }

    struct Task {
        Task() = default;
        Task(TimeSpan d, TimeSpan e, const ITaskQueueTask::Ptr& p) : delay(d), executeTime(e), operation(p) {}

        TimeSpan delay;
        TimeSpan executeTime;
        ITaskQueueTask::Ptr operation { nullptr };
    };

protected:
    std::mutex mutex_;

    ITaskQueueExtend* extend_ { this };
    bool terminate_ {};
    std::thread::id execThread_;
    // currently running task..
    Token execToken_ { nullptr };
    BASE_NS::vector<Task> tasks_;
    ITaskQueue::WeakPtr self_;
};

META_END_NAMESPACE()

#endif
