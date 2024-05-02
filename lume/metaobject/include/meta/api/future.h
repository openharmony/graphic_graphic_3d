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

#ifndef META_API_FUTURE_H
#define META_API_FUTURE_H

#include <base/containers/type_traits.h>

#include <meta/interface/detail/any.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_future.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Callable implementation for continuation functions used with futures.
 */
template<typename Func>
class ContinuationFunction : public IntroduceInterfaces<IFutureContinuation> {
    META_INTERFACE(
        IntroduceInterfaces<IFutureContinuation>, ContinuationFunction, "f4736552-7365-4c8f-bbe9-a065e2c30382");

public:
    ContinuationFunction(Func func) : func_(BASE_NS::move(func)) {}

    IAny::Ptr Invoke(const IAny::Ptr& value) override
    {
        using Result = BASE_NS::remove_reference_t<decltype(func_(value))>;
        if constexpr (!BASE_NS::is_same_v<Result, void>) {
            return IAny::Ptr(new Any<Result>(func_(value)));
        } else {
            func_(value);
            return nullptr;
        }
    }

private:
    Func func_;
};

/**
 * @brief Create continuation function from callable entity (e.g. lambda).
 * The callable entity has to take IAny::Ptr as parameter which is the value from the future.
 */
template<typename Func>
IFutureContinuation::Ptr CreateContinuation(Func func)
{
    return IFutureContinuation::Ptr(new ContinuationFunction(BASE_NS::move(func)));
}

template<typename Param>
struct ContinuationTypedFuntionTypeImpl {
    using Type = void(Param);
};

template<>
struct ContinuationTypedFuntionTypeImpl<void> {
    using Type = void();
};

template<typename Param>
using ContinuationTypedFuntionType = typename ContinuationTypedFuntionTypeImpl<Param>::Type;

template<typename Type>
class Future {
public:
    using StateType = IFuture::StateType;

    Future(IFuture::Ptr fut) : fut_(BASE_NS::move(fut)) {}

    StateType GetState() const
    {
        return fut_ ? fut_->GetState() : IFuture::ABANDONED;
    }
    StateType Wait() const
    {
        return fut_ ? fut_->Wait() : IFuture::ABANDONED;
    }
    StateType WaitFor(const TimeSpan& time) const
    {
        return fut_ ? fut_->WaitFor(time) : IFuture::ABANDONED;
    }
    IFuture::Ptr Then(const IFutureContinuation::Ptr& func, const BASE_NS::shared_ptr<ITaskQueue>& queue)
    {
        return fut_ ? fut_->Then(func, queue) : nullptr;
    }
    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, IFutureContinuation::FunctionType>>
    auto Then(Func func, const BASE_NS::shared_ptr<ITaskQueue>& queue)
    {
        return Future<decltype(func(nullptr))>(fut_->Then(CreateContinuation(func), queue));
    }
    template<typename Func, typename = EnableIfCanInvokeWithArguments<Func, ContinuationTypedFuntionType<Type>>>
    auto Then(Func func, const BASE_NS::shared_ptr<ITaskQueue>& queue, int = 0)
    {
        using ReturnType = decltype(func(Type {}));
        return Future<ReturnType>(fut_->Then(CreateContinuation([f = BASE_NS::move(func)](const IAny::Ptr& v) {
            if (v) {
                Type value {};
                if (v->GetValue(value)) {
                    return f(value);
                }
                CORE_LOG_W("Type mismatch for future then");
            }
            if constexpr (!BASE_NS::is_same_v<ReturnType, void>) {
                return ReturnType {};
            }
        }),
            queue));
    }

    Type GetResult() const
    {
        if (fut_) {
            return fut_->GetResultOr<Type>(Type {});
        }
        return Type {};
    }
    IFuture::Ptr GetFuture() const
    {
        return fut_;
    }

    operator IFuture::Ptr() const
    {
        return fut_;
    }

    explicit operator bool() const
    {
        return fut_ != nullptr;
    }

private:
    IFuture::Ptr fut_;
};

template<typename Type>
Type GetResultOr(const Future<Type>& f, NonDeduced_t<Type> def)
{
    auto fut = f.GetFuture();
    if (fut) {
        if (auto p = fut->GetResult()) {
            return GetValue<Type>(*p, BASE_NS::move(def));
        }
    }
    return BASE_NS::move(def);
}

META_END_NAMESPACE()

#endif
