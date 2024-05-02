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
#ifndef META_INTERFACE_IFUTURE_H
#define META_INTERFACE_IFUTURE_H

#include <meta/base/interface_macros.h>
#include <meta/base/time_span.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_callable.h>

META_BEGIN_NAMESPACE()

class ITaskQueue;

META_REGISTER_INTERFACE(IFutureContinuation, "5bf12e6d-4a49-4a5a-973c-4352c0095edd")

/**
 * @brief Callable type for future continuation support.
 */
class IFutureContinuation : public META_NS::ICallable {
    META_INTERFACE(META_NS::ICallable, IFutureContinuation);

public:
    using FunctionType = IAny::Ptr(const IAny::Ptr&);

    /**
     * @brief Invokes the continuation with the future's value and returns a new value.
     */
    virtual IAny::Ptr Invoke(const IAny::Ptr&) = 0;
};

META_REGISTER_INTERFACE(IFuture, "0321fd50-8835-422b-aff6-1e090026fb56")

/**
 * @brief Future that lets wait for a result
 */
class IFuture : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IFuture);

public:
    enum StateType {
        WAITING,   /// There is no result yet and Wait would block
        COMPLETED, /// There is a result, Wait no longer blocks
        ABANDONED  /// The underlying task was abandoned, Wait does not block and Get returns nullptr
    };
    /**
     * @brief Returns the current state of the future
     */
    virtual StateType GetState() const = 0;
    /**
     * @brief Calling Wait will block until unblocked by promise indicating the job was done or abandoned.
     * Returns state, either Succeeded or Abandoned
     */
    virtual StateType Wait() const = 0;
    /**
     * @brief Wait for given amount of time. Returns state, which is Waiting if the timeout expired.
     */
    virtual StateType WaitFor(const TimeSpan& time) const = 0;
    /**
     * @brief Calls internally Wait and returns the result after Wait returns.
     * Notice that both, abandoning task and void result value, means returning nullptr.
     */
    virtual IAny::Ptr GetResult() const = 0;
    /**
     * @brief Attach continuation function to the future which is called when there is a result
     * The continuation function is executed in the given task queue or inline in the same thread as this one if null.
     */
    virtual IFuture::Ptr Then(const IFutureContinuation::Ptr& func, const BASE_NS::shared_ptr<ITaskQueue>& queue) = 0;
    /**
     * @brief Get the result and try to convert it to given type T. If type mismatch or task was abandoned,
     * returns the value given.
     */
    template<typename T>
    T GetResultOr(T def) const
    {
        if (auto p = GetResult()) {
            return GetValue<T>(*p, BASE_NS::move(def));
        }
        return BASE_NS::move(def);
    }

    /**
     * @brief Cancel the task. Affects only the immediate future, not continuations.
     * @note If currently being executed, wait for it to finish.
     */
    virtual void Cancel() = 0;
};

META_REGISTER_INTERFACE(IPromise, "252e664b-76f3-44c7-abc3-6f14aa2d7fd6")

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IFuture)

#endif
