/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 * Description: Generic task/job queue implementation
 * Author: Jani Kattelus
 * Create: 2022-02-10
 */

#include <mutex>
#include <thread>

#include <base/containers/vector.h>

#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "future.h"
#include "object.h"
#include "task_queue.h"

META_BEGIN_NAMESPACE()

// notice, this is object only so we can construct it via object registery
class PollingTaskQueue : public IntroduceInterfaces<MetaObject, IPollingTaskQueue, TaskQueueImpl> {
    META_OBJECT(PollingTaskQueue, ClassId::PollingTaskQueue, IntroduceInterfaces)
public:
    using Token = ITaskQueue::Token;

    bool Build(const IMetadata::Ptr& data) override
    {
        bool ret = Super::Build(data);
        if (ret) {
            self_ = GetSelf<ITaskQueue>();
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
        return TaskQueueImpl::AddTask(BASE_NS::move(p), delay, Time() + delay);
    }

    IFuture::Ptr AddWaitableTask(ITaskQueueWaitableTask::Ptr p) override
    {
        IPromise::Ptr promise(new Promise);
        BASE_NS::shared_ptr<PromisedQueueTask> task(new PromisedQueueTask(BASE_NS::move(p), promise));
        auto f = task->GetFuture();
        AddTask(BASE_NS::move(task));
        return f;
    }

    void ProcessTasks() override
    {
        TimeSpan ctime = Time();
        std::unique_lock lock { mutex_ };
        if (ctime != lastTime_) {
            lastTime_ = ctime;
            execThread_ = std::this_thread::get_id();
            TaskQueueImpl::ProcessTasks(lock, ctime);
            execThread_ = std::thread::id();
        } else {
            // non issue. but.
            CORE_LOG_V("Double call to ProcessTasks.");
        }
    }

private:
    TimeSpan lastTime_;
};
// Internal api for engine task queue
namespace Internal {

IObjectFactory::Ptr GetPollingTaskQueueFactory()
{
    return PollingTaskQueue::GetFactory();
}

} // namespace Internal

META_END_NAMESPACE()
