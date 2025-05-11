/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2023. All rights reserved.
 * Description: Implemetation bits of For Each iteration for IIterables
 * Author: Mikael Kilpeläinen
 * Create: 2023-04-03
 */

#ifndef META_API_INTERNAL_ITERATION_H
#define META_API_INTERNAL_ITERATION_H

#include <meta/api/internal/breadth_first_iteration.h>
#include <meta/api/internal/depth_first_iteration.h>
#include <meta/api/internal/iteration_types.h>
#include <meta/base/interface_traits.h>
#include <meta/interface/intf_iterable.h>

META_BEGIN_NAMESPACE()
namespace Internal {

template<template<typename> class InterableCallable, typename Iterable, typename Func>
IterationResult IterateImpl(const Iterable& ite, Func&& func, IterateStrategy is)
{
    if (!ite) {
        return IterationResult::FAILED;
    }
    if (is.traversal == TraversalType::NO_HIERARCHY) {
        return CallIterate<InterableCallable>(ite, BASE_NS::forward<Func>(func), is);
    }
    // map the FULL_HIERARCHY to BREADTH_FIRST_ORDER by default
    if (is.traversal == TraversalType::FULL_HIERARCHY) {
        is.traversal = TraversalType::BREADTH_FIRST_ORDER;
    }
    if (is.traversal == TraversalType::DEPTH_FIRST_PRE_ORDER || is.traversal == TraversalType::DEPTH_FIRST_POST_ORDER) {
        return DepthFirstOrderIterate<InterableCallable>(ite, func, is);
    }
    if (is.traversal == TraversalType::BREADTH_FIRST_ORDER) {
        return BreadthFirstOrderIterate<InterableCallable>(ite, func, is);
    }
    return IterationResult::FAILED;
}

template<typename Iterable, typename Func, typename = DisableIfCallable<Func>>
IterationResult Iterate(const BASE_NS::shared_ptr<Iterable>& c, Func&& func, IterateStrategy is)
{
    return IterateImpl<IIterableCallable>(interface_pointer_cast<IIterable>(c), BASE_NS::forward<Func>(func), is);
}

template<typename Iterable>
IterationResult Iterate(const BASE_NS::shared_ptr<Iterable>& c, const ICallable::Ptr& func, IterateStrategy is)
{
    return IterateImpl<IIterableCallable>(interface_pointer_cast<IIterable>(c), *func, is);
}

template<typename Iterable, typename Func, typename = DisableIfCallable<Func>>
IterationResult ConstIterate(const BASE_NS::shared_ptr<Iterable>& c, Func&& func, IterateStrategy is)
{
    return IterateImpl<IIterableConstCallable>(interface_pointer_cast<IIterable>(c), BASE_NS::forward<Func>(func), is);
}

template<typename Iterable>
IterationResult ConstIterate(const BASE_NS::shared_ptr<Iterable>& c, const ICallable::Ptr& func, IterateStrategy is)
{
    return IterateImpl<IIterableConstCallable>(interface_pointer_cast<IIterable>(c), *func, is);
}

template<typename Func, typename = DisableIfCallable<Func>>
IterationResult Iterate(IIterable* c, Func&& func, IterateStrategy is)
{
    return IterateImpl<IIterableCallable>(c, BASE_NS::forward<Func>(func), is);
}

template<typename Func, typename = DisableIfCallable<Func>>
IterationResult ConstIterate(const IIterable* c, Func&& func, IterateStrategy is)
{
    return IterateImpl<IIterableConstCallable>(c, BASE_NS::forward<Func>(func), is);
}

} // namespace Internal
META_END_NAMESPACE()

#endif
