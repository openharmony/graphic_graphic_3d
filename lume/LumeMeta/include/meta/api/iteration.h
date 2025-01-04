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

#ifndef META_API_ITERATION_H
#define META_API_ITERATION_H

#include <meta/api/internal/iteration.h>
#include <meta/interface/intf_lockable.h>

META_BEGIN_NAMESPACE()

/// Create suitable callable for iteration from lambda (or alike)
using Internal::MakeIterationCallable;

/// Create suitable const callable for iteration from lambda (or alike)
using Internal::MakeIterationConstCallable;

/**
 * @brief Iterate over sequence of elements (e.g. container) and call function for each element.
 *        Takes unique lock over the sequence of elements if it is ILockable
 * @param c Sequence of elements, must implement IIterable
 * @param func Function that is called for each element, can mutate the element value
 * @param traversal Control how to iterate over elements
 * @return True if the iteration did not fail
 */
template<typename Iterable, typename Func>
bool ForEachUnique(
    const BASE_NS::shared_ptr<Iterable>& c, Func&& func, TraversalType traversal = TraversalType::NO_HIERARCHY)
{
    return Internal::Iterate(
        c,
        [f = BASE_NS::forward<Func>(func)](Internal::IterationArgType<Func>& arg) {
            f(arg);
            return true;
        },
        IterateStrategy { traversal, LockType::UNIQUE_LOCK });
}

/**
 * @brief Iterate over sequence of elements (e.g. container) and call function for each element.
 *        Takes shared lock over the sequence of elements if it is IReadWriteLockable
 * @param c Sequence of elements, must implement IIterable
 * @param func Function that is called for each element, cannot mutate the element value
 * @param traversal Control how to iterate over elements
 * @return True if the iteration did not fail
 */
template<typename Iterable, typename Func>
bool ForEachShared(
    const BASE_NS::shared_ptr<Iterable>& c, Func&& func, TraversalType traversal = TraversalType::NO_HIERARCHY)
{
    return Internal::ConstIterate(
        c,
        [f = BASE_NS::forward<Func>(func)](const Internal::IterationArgType<Func>& arg) {
            f(arg);
            return true;
        },
        IterateStrategy { traversal, LockType::SHARED_LOCK });
}

/**
 * @brief Iterate over sequence of elements (e.g. container) and call function for each element
 *        as long as the function returns true. If it returns false, stop iteration.
 *        Takes unique lock over the sequence of elements if it is ILockable
 * @param c Sequence of elements, must implement IIterable
 * @param func Function that is called for each element, can mutate the element value
 * @param traversal Control how to iterate over elements
 * @return True if the iteration was stopped (this usually indicates the user function returned false)
 */
template<typename Iterable, typename Func>
bool IterateUnique(
    const BASE_NS::shared_ptr<Iterable>& c, Func&& func, TraversalType traversal = TraversalType::NO_HIERARCHY)
{
    return Internal::Iterate(c, BASE_NS::forward<Func>(func), IterateStrategy { traversal, LockType::UNIQUE_LOCK })
               .value == IterationResult::STOP;
}

/**
 * @brief Iterate over sequence of elements (e.g. container) and call function for each element
 *        as long as the function returns true. If it returns false, stop iteration.
 *        Takes shared lock over the sequence of elements if it is IReadWriteLockable
 * @param c Sequence of elements, must implement IIterable
 * @param func Function that is called for each element, cannot mutate the element value
 * @param traversal Control how to iterate over elements
 * @return True if the iteration was stopped (this usually indicates the user function returned false)
 */
template<typename Iterable, typename Func>
bool IterateShared(
    const BASE_NS::shared_ptr<Iterable>& c, Func&& func, TraversalType traversal = TraversalType::NO_HIERARCHY)
{
    return Internal::ConstIterate(c, BASE_NS::forward<Func>(func), IterateStrategy { traversal, LockType::SHARED_LOCK })
               .value == IterationResult::STOP;
}

/**
 * @brief Iterate over sequence of elements (e.g. container) and call function for each element
 *        as long as the function returns true. If it returns false, stop iteration.
 *        Takes unique lock over the sequence of elements if it is ILockable
 * @param c Sequence of elements, must implement IIterable
 * @param func Function that is called for each element, can mutate the element value
 * @param is Strategy how to iterate over elements
 * @return True if the iteration was stopped (this usually indicates the user function returned false)
 */
template<typename Iterable, typename Func>
bool Iterate(const BASE_NS::shared_ptr<Iterable>& c, Func&& func, IterateStrategy is)
{
    return Internal::Iterate(c, BASE_NS::forward<Func>(func), is).value == IterationResult::STOP;
}

/**
 * @brief Iterate over sequence of elements (e.g. container) and call function for each element
 *        as long as the function returns true. If it returns false, stop iteration.
 *        Takes shared lock over the sequence of elements if it is IReadWriteLockable
 * @param c Sequence of elements, must implement IIterable
 * @param func Function that is called for each element, cannot mutate the element value
 * @param is Strategy how to iterate over elements
 * @return True if the iteration was stopped (this usually indicates the user function returned false)
 */
template<typename Iterable, typename Func>
bool ConstIterate(const BASE_NS::shared_ptr<Iterable>& c, Func&& func, IterateStrategy is)
{
    return Internal::ConstIterate(c, BASE_NS::forward<Func>(func), is).value == IterationResult::STOP;
}

META_END_NAMESPACE()

#endif
