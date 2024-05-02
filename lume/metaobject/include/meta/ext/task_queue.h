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

#include <meta/ext/object.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_promise.h>
#include <meta/interface/intf_task_queue_extend.h>

META_BEGIN_NAMESPACE()

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

template<class FinalClass, const ClassInfo& Info, const ClassInfo& SuperInfo, class... Interfaces>
class TaskQueueFwd : public BaseObjectFwd<FinalClass, Info, SuperInfo, ITaskQueueExtend, Interfaces...> {
public:
    bool Build(const IMetadata::Ptr& data) override
    {
        SetExtend(this->template GetSelf<ITaskQueueExtend>().get());
        return true;
    }

public:
    using Token = ITaskQueue::Token;

    Token AddTask(ITaskQueueTask::Ptr p) override
    {
        return this->template GetBaseAs<ITaskQueue>()->AddTask(BASE_NS::move(p));
    }
    Token AddTask(ITaskQueueTask::Ptr p, const TimeSpan& delay) override
    {
        return this->template GetBaseAs<ITaskQueue>()->AddTask(BASE_NS::move(p), delay);
    }
    IFuture::Ptr AddWaitableTask(ITaskQueueWaitableTask::Ptr p) override
    {
        return this->template GetBaseAs<ITaskQueue>()->AddWaitableTask(BASE_NS::move(p));
    }
    void CancelTask(Token t) override
    {
        this->template GetBaseAs<ITaskQueue>()->CancelTask(t);
    }

public:
    void SetExtend(ITaskQueueExtend* extend) override
    {
        this->template GetBaseAs<ITaskQueueExtend>()->SetExtend(extend);
    }
    bool InvokeTask(const ITaskQueueTask::Ptr& task) override
    {
        return this->template GetBaseAs<ITaskQueueExtend>()->InvokeTask(task);
    }
    void Shutdown() override
    {
        this->template GetBaseAs<ITaskQueueExtend>()->Shutdown();
    }

private:
};

META_END_NAMESPACE()

#endif
