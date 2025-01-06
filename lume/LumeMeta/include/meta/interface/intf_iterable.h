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

#ifndef META_INTERFACE_IITERABLE_H
#define META_INTERFACE_IITERABLE_H

#include <meta/base/interface_macros.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/intf_lockable.h>

META_BEGIN_NAMESPACE()

/**
 * @brief The TraversalType enum is used to define a traversal order in a hierarchy.
 */
enum class TraversalType : uint32_t {
    /** Traverse only immediate children */
    NO_HIERARCHY = 0,
    /** Traverse a hierarchy using any order, this usually just maps to one of the orders below */
    FULL_HIERARCHY = 1,
    /** Traverse a hierarchy using a post-order depth-first algorithm */
    DEPTH_FIRST_POST_ORDER = 2,
    /** Traverse a hierarchy using a pre-order depth-first algorithm */
    DEPTH_FIRST_PRE_ORDER = 3,
    /** Traverse a hierarchy using a breadth first algorithm */
    BREADTH_FIRST_ORDER = 4
};

struct IterateStrategy {
    TraversalType traversal { TraversalType::NO_HIERARCHY };
    LockType lock { LockType::UNIQUE_LOCK };
};

struct IterationParameters {
    ICallable& function;
    IterateStrategy strategy;
};

META_REGISTER_INTERFACE(IIterable, "76bfbc71-bbfe-476c-9ef5-9c01a3bf267d")

/**
 * @brief Result type for iterating to support recursive iteration with stopping
 */
struct IterationResult {
    enum Type { FAILED, STOP, CONTINUE } value;

    IterationResult(Type v = CONTINUE) : value(v) {}
    IterationResult(bool) = delete;

    bool Continue() const
    {
        return value == CONTINUE;
    }

    operator bool() const
    {
        return value != FAILED;
    }
};

/**
 * @brief The IIterable interface which allows to iterate over elements e.g. in container
 */
class IIterable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IIterable)
public:
    /* @brief Invoke function for each element in order as long as the function returns true. In case false is
     *        returned, the iteration stops. Mutating the element sequence within the invoked
     *        function results undefined behaviour. Mutating the element value passed to the function
     *        is permitted.
     * @param func The function being invoked, must be one of the callables below
     * @return True if function was called, i.e. it was compatible with the value type
     */
    virtual IterationResult Iterate(const IterationParameters& params) = 0;
    virtual IterationResult Iterate(const IterationParameters& params) const = 0;
};

/**
 * @brief Callable interface for IIterable
 */
template<typename Type>
class IIterableCallable : public ICallable {
    META_INTERFACE(ICallable, IIterableCallable, UidFromType<Type>())
public:
    using ArgType = Type&;
    virtual IterationResult Invoke(Type&) = 0;
};

/**
 * @brief Callable interface for IIterable with const reference
 */
template<typename Type>
class IIterableConstCallable : public ICallable {
    META_INTERFACE(ICallable, IIterableConstCallable, UidFromType<Type>())
public:
    using ArgType = const Type&;
    virtual IterationResult Invoke(const Type&) = 0;
};

META_END_NAMESPACE()

#endif
