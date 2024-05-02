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

#ifndef META_API_INTERNAL_DEPTH_FIRST_ITERATION_H
#define META_API_INTERNAL_DEPTH_FIRST_ITERATION_H

#include <meta/api/internal/iteration_types.h>

META_BEGIN_NAMESPACE()
namespace Internal {

template<template<typename> class InterableCallable, typename Iterable, typename Func>
IterationResult DepthFirstOrderIterate(const Iterable& i, Func& func, IterateStrategy is)
{
    auto ite = interface_pointer_cast<IIterable>(i);
    if (!ite) {
        return IterationResult::CONTINUE;
    }
    auto f = [&](IterationForwardArgType<Func> arg) {
        if (is.traversal == TraversalType::DEPTH_FIRST_PRE_ORDER && !func(arg)) {
            return IterationResult::STOP;
        }
        if constexpr (IsInterfacePtr_v<PlainType_t<decltype(arg)>>) {
            auto res = DepthFirstOrderIterate<InterableCallable>(arg, func, is);
            if (res.value != IterationResult::CONTINUE) {
                return res.value;
            }
        }
        if (is.traversal == TraversalType::DEPTH_FIRST_POST_ORDER && !func(arg)) {
            return IterationResult::STOP;
        }
        return IterationResult::CONTINUE;
    };

    IterationCallable<InterableCallable, decltype(f)> ff(BASE_NS::move(f));
    InterfaceLock lock(is.lock, ite);
    return ite->Iterate(IterationParameters { ff, is });
}

template<template<typename> class InterableCallable, typename Iterable>
IterationResult DepthFirstOrderIterate(const Iterable& i, ICallable& func, IterateStrategy is)
{
    using CallableType = InterableCallable<IObject::Ptr>;
    auto f = func.GetInterface<CallableType>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return IterationResult::FAILED;
    }
    auto lf = [&](typename CallableType::ArgType obj) { return f->Invoke(obj); };
    return DepthFirstOrderIterate<InterableCallable>(i, lf, is);
}

} // namespace Internal
META_END_NAMESPACE()

#endif
