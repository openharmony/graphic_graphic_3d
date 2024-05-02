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

#ifndef SRC_META_FUTURE_H
#define SRC_META_FUTURE_H

#include <mutex>
#include <thread>

#include <meta/base/interface_macros.h>
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

class Future final : public IntroduceInterfaces<IFuture> {
public:
    StateType GetState() const override;
    StateType Wait() const override;
    StateType WaitFor(const TimeSpan& time) const override;
    IAny::Ptr GetResult() const override;
    IFuture::Ptr Then(const IFutureContinuation::Ptr& func, const ITaskQueue::Ptr& queue) override;
    void Cancel() override;

    void SetResult(IAny::Ptr p);
    void SetAbandoned();

    void SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token);

private:
    // Then continuation support
    struct ContinuationData {
        bool runInline {};
        ITaskQueue::WeakPtr queue;
        BASE_NS::shared_ptr<ContinuationQueueTask> continuation;
    };

    void ActivateContinuation(std::unique_lock<std::mutex>& lock);
    void ActivateContinuation(const ContinuationData& d, const IAny::Ptr& result);

private:
    mutable std::mutex mutex_;
    mutable std::condition_variable cond_;
    IAny::Ptr result_;
    StateType state_ { IFuture::WAITING };
    ITaskQueue::WeakPtr queue_;
    ITaskQueue::Token token_ {};
    BASE_NS::vector<ContinuationData> continuations_;
};

class Promise final : public MinimalObject<META_NS::ClassId::Promise, IPromise> {
    using Super = MinimalObject<META_NS::ClassId::Promise, IPromise>;

public:
    Promise() = default;
    META_NO_COPY_MOVE(Promise)
private:
    META_DEFINE_OBJECT_TYPE_INFO(Promise, META_NS::ClassId::Promise)

public:
    static const StaticObjectMetadata& GetStaticObjectMetadata()
    {
        return StaticObjectMeta();
    }
    static StaticObjectMetadata& StaticObjectMeta()
    {
        static StaticObjectMetadata meta { META_NS::ClassId::Promise, nullptr };
        return meta;
    }

public:
    static BASE_NS::vector<BASE_NS::Uid> GetStaticInterfaces()
    {
        return Super::GetInterfacesVector();
    }

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
