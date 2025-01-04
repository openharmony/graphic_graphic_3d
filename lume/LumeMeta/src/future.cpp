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

#include "future.h"

#include <meta/interface/object_type_info.h>

META_BEGIN_NAMESPACE()

Future::StateType Future::GetState() const
{
    std::unique_lock lock { mutex_ };
    return state_;
}

Future::StateType Future::Wait() const
{
    std::unique_lock lock { mutex_ };
    while (state_ == IFuture::WAITING) {
        cond_.wait(lock);
    }
    return state_;
}

Future::StateType Future::WaitFor(const TimeSpan& time) const
{
    std::unique_lock lock { mutex_ };
    while (state_ == IFuture::WAITING) {
        if (cond_.wait_for(lock, std::chrono::microseconds(time.ToMicroseconds())) == std::cv_status::timeout) {
            return IFuture::WAITING;
        }
    }
    return state_;
}

IAny::Ptr Future::GetResult() const
{
    std::unique_lock lock { mutex_ };
    while (state_ == IFuture::WAITING) {
        cond_.wait(lock);
    }
    return result_;
}

IFuture::Ptr Future::Then(const IFutureContinuation::Ptr& func, const ITaskQueue::Ptr& queue)
{
    std::unique_lock lock { mutex_ };
    IFuture::Ptr result;
    if (state_ == IFuture::ABANDONED) {
        BASE_NS::shared_ptr<Future> f(new Future);
        f->SetAbandoned();
        result = BASE_NS::move(f);
    } else {
        ContinuationData d { queue == nullptr, queue };
        d.continuation.reset(new ContinuationQueueTask(func));
        result = d.continuation->GetFuture();
        continuations_.push_back(BASE_NS::move(d));
        if (state_ == IFuture::COMPLETED) {
            ActivateContinuation(lock);
        }
    }
    return result;
}

void Future::Cancel()
{
    bool notify = false;
    ITaskQueue::Token token {};
    {
        std::unique_lock lock { mutex_ };
        if (state_ != IFuture::COMPLETED) {
            notify = true;
            token = token_;
            token_ = {};
            for (auto&& v : continuations_) {
                v.continuation->SetAbandoned();
            }
            continuations_.clear();
            state_ = IFuture::ABANDONED;
        }
    }
    if (token) {
        if (auto q = queue_.lock()) {
            q->CancelTask(token);
        }
    }
    if (notify) {
        cond_.notify_all();
    }
}

void Future::ActivateContinuation(const ContinuationData& d, const IAny::Ptr& result)
{
    if (auto q = d.queue.lock()) {
        d.continuation->SetParam(result);
        auto token = q->AddTask(d.continuation);
        d.continuation->SetQueueInfo(q, token);
    } else if (d.runInline) {
        d.continuation->SetParam(result);
        d.continuation->Invoke();
    } else {
        d.continuation->SetAbandoned();
    }
}

void Future::ActivateContinuation(std::unique_lock<std::mutex>& lock)
{
    BASE_NS::vector<ContinuationData> cdata = BASE_NS::move(continuations_);
    auto result = result_;
    lock.unlock();
    for (auto&& v : cdata) {
        ActivateContinuation(v, result);
    }
}

void Future::SetResult(IAny::Ptr p)
{
    std::unique_lock lock { mutex_ };
    token_ = {};
    if (state_ == IFuture::WAITING) {
        result_ = BASE_NS::move(p);
        state_ = IFuture::COMPLETED;
        ActivateContinuation(lock);
        cond_.notify_all();
    }
}

void Future::SetAbandoned()
{
    std::unique_lock lock { mutex_ };
    if (state_ == IFuture::WAITING) {
        state_ = IFuture::ABANDONED;
        token_ = {};

        for (auto&& v : continuations_) {
            v.continuation->SetAbandoned();
        }
        continuations_.clear();
        cond_.notify_all();
    }
}

void Future::SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token)
{
    std::unique_lock lock { mutex_ };
    if (state_ == IFuture::WAITING) {
        queue_ = queue;
        token_ = token;
    }
}

Promise::~Promise()
{
    SetAbandoned();
}

void Promise::Set(const IAny::Ptr& res)
{
    if (future_) {
        future_->SetResult(res);
        future_ = nullptr;
    }
}

void Promise::SetAbandoned()
{
    if (future_) {
        future_->SetAbandoned();
        future_ = nullptr;
    }
}

IFuture::Ptr Promise::GetFuture()
{
    if (!future_) {
        future_.reset(new Future);
    }
    return future_;
}

void Promise::SetQueueInfo(const ITaskQueue::Ptr& queue, ITaskQueue::Token token)
{
    if (future_) {
        future_->SetQueueInfo(queue, token);
    }
}

namespace Internal {

IObjectFactory::Ptr GetPromiseFactory()
{
    return Promise::GetFactory();
}

} // namespace Internal

META_END_NAMESPACE()
