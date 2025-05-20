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
