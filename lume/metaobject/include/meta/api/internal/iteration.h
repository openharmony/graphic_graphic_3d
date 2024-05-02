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

} // namespace Internal
META_END_NAMESPACE()

#endif
