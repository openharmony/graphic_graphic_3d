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
#ifndef META_API_DEFERRED_CALLBACK_H
#define META_API_DEFERRED_CALLBACK_H

#include <meta/base/interface_traits.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_lifecycle.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/simple_event.h>

META_BEGIN_NAMESPACE();

/**
 * @brief Implementation of ITaskQueueTask for deferred calls.
 */
template<typename Func>
class QueueTask : public IntroduceInterfaces<ITaskQueueTask> {
    META_INTERFACE(IntroduceInterfaces<ITaskQueueTask>, QueueTask, "426c3d10-89e5-4cf7-a314-a5155ede1595");

public:
    QueueTask(Func func) : func_(BASE_NS::move(func)) {}

    bool Invoke() override
    {
        func_();
        return false;
    }

private:
    Func func_;
};

template<typename Func, typename Interface, typename Signature = typename Interface::FunctionType>
class DeferredCallable;

/**
 * @brief Implementation of callable for deferred calls.
 */
template<typename Func, typename Interface, typename... Args>
class DeferredCallable<Func, Interface, void(Args...)> : public IntroduceInterfaces<Interface> {
    template<typename F>
    struct ControlBlock {
        ControlBlock(F f) : func_(BASE_NS::move(f)) {}

        void Call(Args... args) const
        {
            func_(args...);
        }

    private:
        const F func_;
    };

public:
    DeferredCallable(Func func, const ITaskQueue::Ptr& q)
        : control_(new ControlBlock<Func>(BASE_NS::move(func))), queue_(q)
    {}

    /*
    ~DeferredCallable()
    {
        // todo: implement to make sure there is no call in progress any more
        // either by using the task queue, or with synchronisation primitives
    }
    */
    void Invoke(Args... args) override
    {
        auto q = queue_.lock();
        if (q) {
            q->AddTask(ITaskQueueTask::Ptr(new QueueTask([control = BASE_NS::weak_ptr(control_), args...]() mutable {
                auto p = control.lock();
                if (p) {
                    p->Call(args...);
                }
            })));
        }
    }

private:
    BASE_NS::shared_ptr<ControlBlock<Func>> control_;
    ITaskQueue::WeakPtr queue_;
};

/**
 * @brief Make deferred callable from callable entity (e.g. lambda) by giving callable interface.
 * @param func Callable entity which is invoked.
 * @param queue Task queue that the call is posted to.
 */
template<typename CallableInterface, typename Func,
    typename = EnableIfCanInvokeWithArguments<Func, typename CallableInterface::FunctionType>>
auto MakeDeferred(Func func, const ITaskQueue::Ptr& queue)
{
    using DType = DeferredCallable<Func, CallableInterface>;
    return typename DType::Ptr(new DType(BASE_NS::move(func), queue));
}
META_END_NAMESPACE();
#endif
