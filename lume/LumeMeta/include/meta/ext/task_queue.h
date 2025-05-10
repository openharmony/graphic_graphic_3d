/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Helpers for task queues
 * Author: Mikael Kilpeläinen
 * Create: 2023-02-23
 */

#ifndef META_EXT_TASK_QUEUE_H
#define META_EXT_TASK_QUEUE_H

#include <base/containers/type_traits.h>

#include <meta/ext/object_fwd.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_promise.h>
#include <meta/interface/intf_task_queue_extend.h>

META_BEGIN_NAMESPACE()

/// Task implementation for futures
class PromisedQueueTask : public IntroduceInterfaces<ITaskQueueTask, ITaskScheduleInfo> {
public:
    PromisedQueueTask(ITaskQueueWaitableTask::Ptr task, IPromise::Ptr p)
        : task_(BASE_NS::move(task)), promise_(BASE_NS::move(p))
    {}

    bool Invoke() override
    {
        auto res = task_->Invoke();
        // make sure the task is destroyed before future wait returns,
        // so that any captured objects are also destroyed.
        task_.reset();
        promise_->Set(BASE_NS::move(res));
        return false;
    }

    void SetQueueAndToken(const ITaskQueue::Ptr& q, ITaskQueue::Token t) override
    {
        promise_->SetQueueInfo(q, t);
    }

    [[nodiscard]] IFuture::Ptr GetFuture()
    {
        return promise_->GetFuture();
    }

    IPromise::Ptr GetPromise()
    {
        return promise_;
    }

private:
    ITaskQueueWaitableTask::Ptr task_;
    IPromise::Ptr promise_;
};

META_END_NAMESPACE()

#endif
