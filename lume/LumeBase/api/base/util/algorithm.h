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

#ifndef API_BASE_UTIL_ALGORITHM_H
#define API_BASE_UTIL_ALGORITHM_H

#include <cstddef>

#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T = void>
struct Less {
    constexpr bool operator()(const T& lhs, const T& rhs) const noexcept
    {
        return lhs < rhs;
    }
};

template<>
struct Less<void> {
    template<typename T1, typename T2>
    constexpr bool operator()(const T1& lhs, const T2& rhs) const noexcept
    {
        return lhs < rhs;
    }
};

template<typename It, typename Comparison>
void InsertionSort(It first, const It last, Comparison&& func)
{
    const auto len = last - first;
    for (ptrdiff_t i = 1; i < len; ++i) {
        auto x = BASE_NS::move(*(first + i));
        auto j = i;
        while (j > 0 && func(x, *(first + (j - 1)))) {
            *(first + j) = BASE_NS::move(*(first + (j - 1)));
            j = j - 1;
        }
        *(first + j) = BASE_NS::move(x);
    }
}

template<typename It>
inline void InsertionSort(It first, const It last)
{
    InsertionSort(first, last, BASE_NS::Less<decltype(*first)> {});
}

template<typename Iter, typename T, typename Comparison>
constexpr Iter LinearLowerBound(Iter first, Iter last, const T& value, Comparison&& pred) noexcept
{
    while ((first != last)) {
        if (!pred(*first, value)) {
            return first;
        }
        ++first;
    }
    return first;
}

template<typename Iter, typename T, typename Comparison, std::ptrdiff_t limit = 10>
constexpr Iter LowerBound(Iter first, Iter last, const T& value, Comparison&& pred) noexcept
{
    auto size = (last - first);

    while (size > 0) {
        if (size < limit) {
            return LinearLowerBound(first, last, value, BASE_NS::forward<Comparison>(pred));
        }

        const auto midIndex = size / 2;
        const auto mid = first + midIndex;
        if (pred(*mid, value)) {
            first = mid + 1;
            size -= midIndex + 1;
        } else {
            size = midIndex;
        }
    }
    return first;
}

template<typename Iter, typename T, std::ptrdiff_t limit = 10>
constexpr Iter LowerBound(Iter first, Iter last, const T& value) noexcept
{
    return LowerBound<Iter, T, BASE_NS::Less<>, limit>(first, last, value, BASE_NS::Less<> {});
}

BASE_END_NAMESPACE()
#endif // API_BASE_UTIL_ALGORITHM_H