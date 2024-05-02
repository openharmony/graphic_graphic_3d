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

#ifndef META_BASE_TYPE_TRAITS_H
#define META_BASE_TYPE_TRAITS_H

#include <base/containers/type_traits.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

template<typename... Types>
struct TypeList {};

template<bool B>
struct BoolWrap {
    constexpr static bool value = B; // NOLINT(readability-identifier-naming)
};

// same concept as https://en.cppreference.com/w/cpp/experimental/is_detected but simplified for our needs
template<typename Void, template<class...> class Op, typename... Args>
struct IsDetected {
    static constexpr bool value = false; // NOLINT(readability-identifier-naming)
    using type = BoolWrap<false>;
};

template<template<class...> class Op, typename... Args>
struct IsDetected<decltype(BASE_NS::declval<Op<Args...>>(), void()), Op, Args...> {
    static constexpr bool value = true; // NOLINT(readability-identifier-naming)
    using type = Op<Args...>;
};

template<template<class...> class Op, typename... Args>
constexpr bool IsDetected_v = IsDetected<void, Op, Args...>::value; // NOLINT(readability-identifier-naming)

// NOLINTBEGIN(readability-identifier-naming)
template<template<class...> class Op, typename... Args>
constexpr bool IsDetectedWithValue_v = IsDetected<void, Op, Args...>::type::value;
// NOLINTEND(readability-identifier-naming)

template<typename T>
using PlainType_t = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<T>>; // NOLINT(readability-identifier-naming)

template<size_t... Ints>
struct IndexSequence {};

template<size_t Size, size_t... Ints>
struct MakeIndexSequenceImpl {
    using type = typename MakeIndexSequenceImpl<Size - 1, Size - 1, Ints...>::type;
};

template<size_t... Ints>
struct MakeIndexSequenceImpl<0, Ints...> {
    using type = IndexSequence<Ints...>;
};

template<typename... Args>
using MakeIndexSequenceFor = typename MakeIndexSequenceImpl<sizeof...(Args)>::type;

template<size_t Size>
using MakeIndexSequence = typename MakeIndexSequenceImpl<Size>::type;

template<typename Type>
struct NonDeduced {
    using type = Type;
};
template<typename Type>
using NonDeduced_t = typename NonDeduced<Type>::type;

template<typename T>
constexpr bool is_enum_v = __is_enum(T); // NOLINT(readability-identifier-naming)

META_END_NAMESPACE()

/*
    Description:
        This can turn SFINAE failures into false values, allowing to construct constexpr bool variables

    Example:

    template<typename Type>
    using KinfOfInterfaceCheck = BoolWrap<IsKindOfIInterface_v<typename Type::element_type*>>;

    template<typename Type>
    constexpr bool IsInterfacePtr_v = IsKindOfPointer_v<Type> && IsDetectedWithValue_v<KinfOfInterfaceCheck, Type>;

    this cannot be written directly since Type::element_type fails to compile.

    Or

    template<typename Type>
    using HasMyMember = decltype(BASE_NS::declval<Type>().MyMember());

    template<typename Type>
    constexpr bool HasMyMember_v = IsDetected_v<HasMyMember, Type>;
*/

#endif
