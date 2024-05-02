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

#ifndef META_API_INTERNAL_ITERATION_TYPES_H
#define META_API_INTERNAL_ITERATION_TYPES_H

#include <meta/api/locking.h>
#include <meta/base/interface_traits.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_iterable.h>

META_BEGIN_NAMESPACE()
namespace Internal {

template<typename Func>
struct IterationFuncType;

template<typename R, typename Type>
struct IterationFuncType<R(Type)> {
    using ArgType = PlainType_t<Type>;
    using ActualType = Type;
};

template<typename T>
using IterationArgType = typename IterationFuncType<FuncToSignature_t<T>>::ArgType;
template<typename T>
using IterationForwardArgType = typename IterationFuncType<FuncToSignature_t<T>>::ArgType;

template<template<typename> class Intf, typename Func>
class IterationCallable : public IntroduceInterfaces<Intf<IterationArgType<Func>>> {
public:
    IterationCallable(Func f) : func_(BASE_NS::move(f)) {}

protected:
    Func func_;

    using ArgType = typename IntroduceInterfaces<Intf<IterationArgType<Func>>>::ArgType;
    IterationResult Invoke(ArgType arg) final
    {
        if constexpr (BASE_NS::is_same_v<IterationResult, decltype(func_(arg))> ||
                      BASE_NS::is_same_v<IterationResult::Type, decltype(func_(arg))>) {
            return func_(arg);
        } else {
            return func_(arg) ? IterationResult::CONTINUE : IterationResult::STOP;
        }
    }
};

template<typename T>
using DisableIfCallable = BASE_NS::enable_if_t<!BASE_NS::is_convertible_v<T, const ICallable::Ptr&>>;

template<typename Func>
auto MakeIterationCallable(Func f)
{
    return ICallable::Ptr(new IterationCallable<IIterableCallable, Func>(BASE_NS::move(f)));
}
template<typename Func>
auto MakeIterationConstCallable(Func f)
{
    return ICallable::Ptr(new IterationCallable<IIterableConstCallable, Func>(BASE_NS::move(f)));
}

template<template<typename> class InterableCallable, typename Iterable, typename Func>
auto CallIterate(const Iterable& i, Func&& func, IterateStrategy is)
{
    IterationCallable<InterableCallable, Func> f(BASE_NS::forward<Func>(func));
    InterfaceLock lock(is.lock, i);
    return i->Iterate(IterationParameters { f, is });
}

template<template<typename> class InterableCallable, typename Iterable>
auto CallIterate(const Iterable& i, ICallable& func, IterateStrategy is)
{
    InterfaceLock lock(is.lock, i);
    return i->Iterate(IterationParameters { func, is });
}

} // namespace Internal
META_END_NAMESPACE()

#endif
