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

#ifndef META_SRC_FUTURE_H
#define META_SRC_FUTURE_H

#include <condition_variable>
#include <mutex>
#include <thread>

#include <base/containers/shared_ptr.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/ext/minimal_object.h>
#include <meta/ext/object_factory.h>
#include <meta/ext/task_queue.h>
#include <meta/interface/intf_future.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_promise.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/object_type_info.h>

META_BEGIN_NAMESPACE()

class ContinuationQueueTask;

class IFutureSetInfo : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IFutureSetInfo, "3aad605c-080f-447a-b013-f1f7d68216ba")
public:
    virtual void SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token) = 0;
};

class Future final : public IntroduceInterfaces<IFuture, IFutureSetInfo> {
public:
    StateType GetState() const override;
    StateType Wait() const override;
    StateType WaitFor(const TimeSpan& time) const override;
    IAny::Ptr GetResult() const override;
    IFuture::Ptr Then(const IFutureContinuation::Ptr& func, const ITaskQueue::Ptr& queue) override;
    void Cancel() override;

    void SetResult(IAny::Ptr p);
    void SetAbandoned();

    void SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token) override;

private:
    // Then continuation support
    struct ContinuationData {
        bool runInline {};
        ITaskQueue::WeakPtr queue;
        BASE_NS::shared_ptr<ContinuationQueueTask> continuation;
    };

    void ActivateContinuation(std::unique_lock<std::mutex>& lock);
    void ActivateContinuation(ContinuationData d, const IAny::Ptr& result);

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cond_;
    IAny::Ptr result_;
    StateType state_ { IFuture::WAITING };
    ITaskQueue::WeakPtr queue_;
    ITaskQueue::Token token_ {};
    BASE_NS::vector<ContinuationData> continuations_;
};

class Promise final : public IntroduceInterfaces<MinimalObject, IPromise> {
    META_OBJECT(Promise, META_NS::ClassId::Promise, IntroduceInterfaces)
public:
    Promise() = default;
    META_NO_COPY_MOVE(Promise)
public:
    ~Promise() override;

    void Set(const IAny::Ptr& res) override;
    void SetAbandoned() override;
    [[nodiscard]] IFuture::Ptr GetFuture() override;
    void SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token) override;

private:
    BASE_NS::shared_ptr<Future> future_;
};

class ContinuationQueueTask : public IntroduceInterfaces<ITaskQueueTask> {
public:
    explicit ContinuationQueueTask(IFutureContinuation::Ptr task) : task_(BASE_NS::move(task)) {}

    void SetParam(IAny::Ptr arg)
    {
        arg_ = BASE_NS::move(arg);
    }

    void SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token)
    {
        promise_.SetQueueInfo(queue, token);
    }

    bool Invoke() override
    {
        promise_.Set(task_->Invoke(arg_));
        return false;
    }

    [[nodiscard]] IFuture::Ptr GetFuture()
    {
        return promise_.GetFuture();
    }

    void SetAbandoned()
    {
        promise_.SetAbandoned();
    }

private:
    IAny::Ptr arg_;
    IFutureContinuation::Ptr task_;
    Promise promise_;
};

META_END_NAMESPACE()

#endif
