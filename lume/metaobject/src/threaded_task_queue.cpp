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

#include <condition_variable>
#include <mutex>
#include <thread>

#include <base/containers/vector.h>

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "future.h"
#include "meta_object.h"
#include "task_queue.h"

META_BEGIN_NAMESPACE()

class ThreadedTaskQueue
    : public Internal::MetaObjectFwd<ThreadedTaskQueue, ClassId::ThreadedTaskQueue, IThreadedTaskQueue, TaskQueueImpl> {
public:
    using Super =
        Internal::MetaObjectFwd<ThreadedTaskQueue, ClassId::ThreadedTaskQueue, IThreadedTaskQueue, TaskQueueImpl>;
    using Token = ITaskQueue::Token;

    META_NO_COPY_MOVE(ThreadedTaskQueue)

    ThreadedTaskQueue() = default;
    ~ThreadedTaskQueue() override
    {
        Shutdown();
    }

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = Super::Build(data);
        if (ret) {
            self_ = GetSelf<ITaskQueue>();
            thread_ = std::thread([this]() { ProcessTasks(); });
        }
        return ret;
    }

    bool InvokeTask(const ITaskQueueTask::Ptr& task) override
    {
        auto q = GetTaskQueueRegistry().SetCurrentTaskQueue(self_);
        auto ret = task->Invoke();
        GetTaskQueueRegistry().SetCurrentTaskQueue(q);
        return ret;
    }

    void Shutdown() override
    {
        Close();
        addCondition_.notify_one();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    void CancelTask(Token token) override
    {
        TaskQueueImpl::CancelTask(token);
    }

    Token AddTask(ITaskQueueTask::Ptr p) override
    {
        return AddTask(BASE_NS::move(p), TimeSpan::Milliseconds(0));
    }

    Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay) override
    {
        auto t = TaskQueueImpl::AddTask(BASE_NS::move(p), delay, Time() + delay);
        if (t) {
            addCondition_.notify_one();
        }
        return t;
    }

    IFuture::Ptr AddWaitableTask(ITaskQueueWaitableTask::Ptr p) override
    {
        IPromise::Ptr promise(new Promise);
        BASE_NS::shared_ptr<PromisedQueueTask> task(new PromisedQueueTask(BASE_NS::move(p), promise));
        auto f = task->GetFuture();
        AddTask(BASE_NS::move(task));
        return f;
    }

    void ProcessTasks()
    {
        std::unique_lock lock { mutex_ };
        execThread_ = std::this_thread::get_id();
        while (!terminate_) {
            if (!tasks_.empty()) {
                TimeSpan delta = tasks_.back().executeTime - Time();
                // wait for next execute time (or trigger which ever is first). and see if we can now process things..
                // technically we will always be a bit late here. "it's a best effort"
                if (delta > TimeSpan::Microseconds(0)) {
                    addCondition_.wait_for(lock, std::chrono::microseconds(delta.ToMicroseconds()));
                }
            } else {
                // infinite wait, since the queue is empty..
                addCondition_.wait(lock);
            }
            auto curTime = Time();
            TaskQueueImpl::ProcessTasks(lock, curTime);
        }
    }

private:
    std::condition_variable addCondition_;
    std::thread thread_;
};

namespace Internal {

IObjectFactory::Ptr GetThreadedTaskQueueFactory()
{
    return ThreadedTaskQueue::GetFactory();
}

} // namespace Internal

META_END_NAMESPACE()
