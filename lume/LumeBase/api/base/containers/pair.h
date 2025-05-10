/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef API_BASE_CONTAINERS_PAIR_H
#define API_BASE_CONTAINERS_PAIR_H

#include <base/containers/type_traits.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T1, class T2>
struct pair {
    template<class U1 = T1, class U2 = T2,
        enable_if_t<is_default_constructible_v<U1> && is_default_constructible_v<U2>, int> = 0>
    constexpr pair() noexcept(is_nothrow_default_constructible_v<U1>&& is_nothrow_default_constructible_v<U2>)
        : first(), second()
    {}

    template<class U1 = T1, class U2 = T2,
        enable_if_t<is_copy_constructible_v<U1> && is_copy_constructible_v<U2>, int> = 0>
    constexpr
#if (__cplusplus == 202002L)
        explicit(!(is_convertible_v<const U1&, U1> && is_convertible_v<const U2&, U2>))
#endif
            pair(const T1& x, const T2& y) noexcept(
                is_nothrow_copy_constructible_v<U1>&& is_nothrow_copy_constructible_v<U2>)
        : first(x), second(y)
    {}

    template<class U1 = T1, class U2 = T2,
        enable_if_t<is_constructible_v<T1, U1> && is_constructible_v<T2, U2>, int> = 0>
    constexpr
#if (__cplusplus == 202002L)
        explicit(!(is_convertible_v<U1, T1> && is_convertible_v<U2, T2>))
#endif
            pair(U1&& x, U2&& y) noexcept(is_nothrow_constructible_v<T1, U1>&& is_nothrow_constructible_v<T2, U2>)
        : first(BASE_NS::forward<U1>(x)), second(BASE_NS::forward<U2>(y))
    {}

    template<class U1, class U2,
        enable_if_t<is_constructible_v<T1, const U1&> && is_constructible_v<T2, const U2&>, int> = 0>
    constexpr
#if (__cplusplus == 202002L)
        explicit(!(is_convertible_v<const U1&, T1> && is_convertible_v<const U2&, T2>))
#endif
            pair(const pair<U1, U2>& p)
        : first(p.first), second(p.second)
    {}

    template<class U1, class U2, enable_if_t<is_constructible_v<T1, U1> && is_constructible_v<T2, U2>, int> = 0>
    constexpr
#if (__cplusplus == 202002L)
        explicit(!(is_convertible_v<U1, T1> && is_convertible_v<U2, T2>))
#endif
            pair(pair<U1, U2>&& p) noexcept(is_nothrow_constructible_v<T1, U1>&& is_nothrow_constructible_v<T2, U2>)
        : first(BASE_NS::forward<U1>(p.first)), second(BASE_NS::forward<U2>(p.second))
    {}

    pair(const pair& other) = default;

    pair(pair&& other) noexcept = default;

    constexpr pair& operator=(const pair& other)
    {
        if (this != &other) {
            first = other.first;
            second = other.second;
        }
        return *this;
    }

    template<class U1, class U2,
        enable_if_t<is_assignable_v<T1&, const U1&> && is_assignable_v<T2&, const U2&>, int> = 0>
    constexpr pair& operator=(const pair<U1, U2>& other)
    {
        first = other.first;
        second = other.second;

        return *this;
    }

    constexpr pair& operator=(pair&& other) noexcept
    {
        if (this != &other) {
            first = BASE_NS::move(other.first);
            second = BASE_NS::move(other.second);
        }
        return *this;
    }

    template<class U1, class U2, enable_if_t<is_assignable_v<T1&, U1> && is_assignable_v<T2&, U2>, int> = 0>
    constexpr pair& operator=(pair<U1, U2>&& other) noexcept(
        is_nothrow_constructible_v<T1, U1>&& is_nothrow_constructible_v<T2, U2>)
    {
        first = BASE_NS::forward<U1>(other.first);
        second = BASE_NS::forward<U2>(other.second);

        return *this;
    }

    using first_type = T1;
    using second_type = T2;
    first_type first;
    second_type second;
};

template<class T1, class T2>
pair(T1, T2) -> pair<T1, T2>;

template<class T1, class T2, class U1, class U2>
constexpr bool operator==(const BASE_NS::pair<T1, T2>& lhs, const BASE_NS::pair<U1, U2>& rhs)
{
    return (lhs.first == rhs.first) && (lhs.second == rhs.second);
}

template<class T1, class T2, class U1, class U2>
constexpr bool operator!=(const BASE_NS::pair<T1, T2>& lhs, const BASE_NS::pair<U1, U2>& rhs)
{
    return !(lhs == rhs);
}

template<class T1, class T2, class U1, class U2>
constexpr bool operator<(const BASE_NS::pair<T1, T2>& lhs, const BASE_NS::pair<U1, U2>& rhs)
{
    if (lhs.first < rhs.first) {
        return true;
    }
    if (rhs.first < lhs.first) {
        return false;
    }
    if (lhs.second < rhs.second) {
        return true;
    }
    return false;
}

template<class T1, class T2, class U1, class U2>
constexpr bool operator<=(const BASE_NS::pair<T1, T2>& lhs, const BASE_NS::pair<U1, U2>& rhs)
{
    return !(rhs < lhs);
}

template<class T1, class T2, class U1, class U2>
constexpr bool operator>(const BASE_NS::pair<T1, T2>& lhs, const BASE_NS::pair<U1, U2>& rhs)
{
    return (rhs < lhs);
}

template<class T1, class T2, class U1, class U2>
constexpr bool operator>=(const BASE_NS::pair<T1, T2>& lhs, const BASE_NS::pair<U1, U2>& rhs)
{
    return !(lhs < rhs);
}
BASE_END_NAMESPACE()

#endif // //API_BASE_CONTAINERS_PAIR_H
