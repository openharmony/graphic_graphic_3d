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

#ifndef META_API_INTERNAL_BREADTH_FIRST_ITERATION_H
#define META_API_INTERNAL_BREADTH_FIRST_ITERATION_H

#include <meta/api/internal/iteration_types.h>

META_BEGIN_NAMESPACE()
namespace Internal {

template<typename Type>
void AddToQueue(BASE_NS::vector<IIterable::ConstPtr>& vec, const Type& t)
{
    if constexpr (IsInterfacePtr_v<PlainType_t<Type>>) {
        if (auto i = interface_pointer_cast<IIterable>(t)) {
            vec.push_back(i);
        }
    }
}

template<template<typename> class InterableCallable, typename Iterable, typename Func>
IterationResult BreadthFirstOrderIterate(const Iterable& i, Func& func, IterateStrategy is)
{
    auto ite = interface_pointer_cast<IIterable>(i);
    if (!ite) {
        return IterationResult::CONTINUE;
    }
    using ParamType = IterationForwardArgType<Func>;

    BASE_NS::vector<IIterable::ConstPtr> queue;
    auto f = [&](ParamType arg) {
        if (!func(arg)) {
            return IterationResult::STOP;
        }
        AddToQueue(queue, arg);
        return IterationResult::CONTINUE;
    };
    IterationCallable<InterableCallable, decltype(f)> ff(BASE_NS::move(f));
    InterfaceLock lock(is.lock, ite);

    IterationResult res;

    queue.push_back(ite);
    while (!queue.empty()) {
        auto i = queue[0];
        queue.erase(queue.begin());
        res = i->Iterate(IterationParameters { ff, is });
        if (res.value != IterationResult::CONTINUE) {
            return res;
        }
    }
    return res;
}

template<template<typename> class InterableCallable, typename Iterable>
IterationResult BreadthFirstOrderIterate(const Iterable& i, ICallable& func, IterateStrategy is)
{
    using CallableType = InterableCallable<IObject::Ptr>;
    auto f = func.GetInterface<CallableType>();
    if (!f) {
        CORE_LOG_W("Incompatible function with Iterate");
        return IterationResult::FAILED;
    }
    auto lf = [&](typename CallableType::ArgType obj) { return f->Invoke(obj); };
    return BreadthFirstOrderIterate<InterableCallable>(i, lf, is);
}

} // namespace Internal
META_END_NAMESPACE()

#endif
