/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Implementation of breadth first iterations
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-10
 */

#ifndef META_API_INTERNAL_BREADTH_FIRST_ITERATION_H
#define META_API_INTERNAL_BREADTH_FIRST_ITERATION_H

#include <meta/api/internal/iteration_types.h>

META_BEGIN_NAMESPACE()
namespace Internal {

template<typename Type>
void AddToQueue(BASE_NS::vector<const IIterable*>& vec, const Type& t)
{
    if constexpr (IsInterfacePtr_v<PlainType_t<Type>> || IsKindOfIInterface_v<Type>) {
        if (auto i = interface_cast<IIterable>(t)) {
            vec.push_back(i);
        }
    }
}

template<template<typename> class InterableCallable, typename Iterable, typename Func>
IterationResult BreadthFirstOrderIterate(const Iterable& i, Func& func, IterateStrategy is)
{
    auto ite = interface_cast<IIterable>(i);
    if (!ite) {
        return IterationResult::CONTINUE;
    }
    using ParamType = IterationForwardArgType<Func>;

    BASE_NS::vector<const IIterable*> queue;
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
