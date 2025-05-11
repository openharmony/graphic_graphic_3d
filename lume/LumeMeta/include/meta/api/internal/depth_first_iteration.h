/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Implementation of depth first iterations
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-04
 */

#ifndef META_API_INTERNAL_DEPTH_FIRST_ITERATION_H
#define META_API_INTERNAL_DEPTH_FIRST_ITERATION_H

#include <meta/api/internal/iteration_types.h>

META_BEGIN_NAMESPACE()
namespace Internal {

template<template<typename> class InterableCallable, typename Iterable, typename Func>
IterationResult DepthFirstOrderIterate(const Iterable& i, Func& func, IterateStrategy is)
{
    auto ite = interface_cast<IIterable>(i);
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
