/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef API_BASE_CONTAINERS_ARRAYVIEW_H
#define API_BASE_CONTAINERS_ARRAYVIEW_H

#include <cassert>
#include <cstdint>
#include <iterator>
#include <type_traits>

/** @ingroup group_containers_arrayview */
/** Array view */
template<class T>
class array_view {
public:
    struct Iterator {
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = T;
        using pointer = T*;
        using reference = T&;

        Iterator(pointer ptr) : m_ptr(ptr) {}

        reference operator*() const
        {
            return *m_ptr;
        }
        pointer operator->()
        {
            return m_ptr;
        }
        Iterator& operator++()
        {
            m_ptr++;
            return *this;
        }
        Iterator operator++(int)
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }
        friend bool operator==(const Iterator& a, const Iterator& b)
        {
            return a.m_ptr == b.m_ptr;
        };
        friend bool operator!=(const Iterator& a, const Iterator& b)
        {
            return a.m_ptr != b.m_ptr;
        };

    private:
        pointer m_ptr;
    };
    using value_type = T;
    using difference_type = size_t;
    using pointer = value_type*;
    using reference = value_type&;

    using size_type = size_t;
    using const_reference = const value_type&;
    using const_pointer = const value_type*;

    using iterator = Iterator;
    using const_iterator = Iterator;

    constexpr array_view() noexcept : begin_(nullptr), size_(0), end_(nullptr) {}
    constexpr array_view(pointer aBegin, pointer aEnd) noexcept : begin_(aBegin), size_(aEnd - aBegin), end_(aEnd)
    {
        assert(end_ >= begin_);
    }
    constexpr array_view(pointer aBegin, size_type aSize) noexcept : begin_(aBegin), size_(aSize), end_(aBegin + aSize)
    {}
    template<size_t N>
    constexpr array_view(value_type (&arr)[N]) noexcept : begin_(arr), size_(N), end_(arr + N)
    {}
    template<class U, class = std::enable_if_t<std::is_same<std::remove_const_t<T>, U>::value>>
    constexpr array_view(const array_view<U>& other) noexcept
        : begin_(other.begin_), size_(other.size_), end_(other.end_)
    {}
    template<class U, class = std::enable_if_t<std::is_same<std::remove_const_t<T>, typename U::value_type>::value>>
    constexpr array_view(U& container) noexcept : array_view(container.data(), container.size())
    {}
    template<class U, class = std::enable_if_t<std::is_same<std::remove_const_t<T>, typename U::value_type>::value>>
    constexpr array_view(const U& container) noexcept : array_view(container.data(), container.size())
    {}
    ~array_view() = default;
    constexpr size_type size() const noexcept
    {
        return size_;
    }
    constexpr size_type size_bytes() const noexcept
    {
        return size_ * sizeof(T);
    }
    constexpr bool empty() const noexcept
    {
        return size_ == 0;
    }
    constexpr const_pointer data() const noexcept
    {
        return begin_;
    }
    constexpr pointer data() noexcept
    {
        return begin_;
    }
    constexpr reference at(size_type aIndex) noexcept
    {
        // If index out-of-range, undefined behaviour on release builds, assert on debug builds.
        assert(aIndex < size());
        return begin_[aIndex];
    }
    constexpr const_reference at(size_type aIndex) const noexcept
    {
        // If index out-of-range, undefined behaviour on release builds, assert on debug builds.
        assert(aIndex < size());
        return begin_[aIndex];
    }
    constexpr reference operator[](size_type aIndex)
    {
        // If index out-of-range, undefined behaviour on release builds, assert on debug builds.
        assert(aIndex < size());
        return begin_[aIndex];
    }
    constexpr const_reference operator[](size_type aIndex) const
    {
        // If index out-of-range, undefined behaviour on release builds, assert on debug builds.
        assert(aIndex < size());
        return begin_[aIndex];
    }
    constexpr iterator begin() noexcept
    {
        return iterator(begin_);
    }
    constexpr iterator end() noexcept
    {
        return iterator(begin_ + size_);
    }
    constexpr const_iterator begin() const noexcept
    {
        return const_iterator(begin_);
    }
    constexpr const_iterator end() const noexcept
    {
        return const_iterator(begin_ + size_);
    }
    constexpr const_iterator cbegin() const noexcept
    {
        return begin();
    }
    constexpr const_iterator cend() const noexcept
    {
        return end();
    }

private:
    template<class U>
    friend class array_view;

    pointer begin_;
    size_type size_;
    pointer end_;
};

template<typename T, size_t N>
constexpr size_t countof(T (&)[N]) noexcept
{
    return N;
}
template<typename T, size_t N>
constexpr array_view<T> arrayview(T (&arr)[N]) noexcept
{
    return array_view<T>(arr, N);
}
// Returns a const uint8_t array_view of any object.
template<typename T>
constexpr array_view<const uint8_t> arrayviewU8(const T& arr) noexcept
{
    return array_view(reinterpret_cast<const uint8_t*>(&arr), sizeof(arr));
}

#endif // API_BASE_CONTAINERS_ARRAYVIEW_H
