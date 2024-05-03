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

#ifndef API_BASE_CONTAINERS_STRING_VIEW_H
#define API_BASE_CONTAINERS_STRING_VIEW_H

#include <cstddef>
#include <cstdint>

#include <base/containers/iterator.h>
#include <base/containers/type_traits.h>
#include <base/namespace.h>
#include <base/util/hash.h>

BASE_BEGIN_NAMESPACE()
template<class CharT>
class basic_string_view;

using string_view = BASE_NS::basic_string_view<char>;
using wstring_view = BASE_NS::basic_string_view<wchar_t>;

template<class CharT>
class basic_string_view {
public:
    using value_type = CharT;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using const_iterator = BASE_NS::const_iterator<basic_string_view>;
    using iterator = BASE_NS::iterator<basic_string_view>;
    using const_reverse_iterator = BASE_NS::reverse_iterator<const_iterator>;
    using reverse_iterator = BASE_NS::reverse_iterator<iterator>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    static const size_type npos = size_type(-1);

    constexpr basic_string_view() noexcept = default;
    constexpr basic_string_view(const basic_string_view& other) noexcept = default;
    constexpr basic_string_view(CharT const* const s, size_type count);
    constexpr basic_string_view(CharT const* const s);

    ~basic_string_view() = default;

    constexpr basic_string_view& operator=(const basic_string_view& view) noexcept = default;

    // Iterators
    /** returns an iterator to the beginning */
    constexpr const_iterator begin() const noexcept;
    constexpr const_iterator cbegin() const noexcept;

    /** returns an iterator to the end */
    constexpr const_iterator end() const noexcept;
    constexpr const_iterator cend() const noexcept;

    /** returns a reverse iterator to the beginning */
    constexpr const_reverse_iterator rbegin() const noexcept;
    constexpr const_reverse_iterator crbegin() const noexcept;

    /** returns a reverse iterator to the end */
    constexpr const_reverse_iterator rend() const noexcept;
    constexpr const_reverse_iterator crend() const noexcept;

    // Element access
    /** accesses the specified character */
    constexpr const_reference operator[](size_type pos) const;

    /** accesses the specified character with bounds checking */
    constexpr const_reference at(size_type pos) const;

    /** accesses the first character */
    constexpr const_reference front() const;

    /** accesses the last character */
    constexpr const_reference back() const;

    /** returns a pointer to the first character of a view */
    constexpr const_pointer data() const noexcept;

    // Capacity
    /** returns the number of characters */
    constexpr size_type size() const noexcept;
    constexpr size_type length() const noexcept;

    /** returns the maximum number of characters */
    constexpr size_type max_size() const noexcept;

    /** checks whether the view is empty */
    constexpr bool empty() const noexcept;

    // Modifiers
    /** shrinks the view by moving its start forward */
    constexpr void remove_prefix(size_type n);

    /** shrinks the view by moving its end backward */
    constexpr void remove_suffix(size_type n);

    /** swaps the contents */
    constexpr void swap(basic_string_view& v) noexcept;

    // Operations
    /** copies characters */
    constexpr size_type copy(CharT* dest, size_type count, size_type pos = 0) const;

    /** returns a substring */
    constexpr basic_string_view substr(size_type pos = 0, size_type count = npos) const;
    /** compares two views */
    constexpr int compare(basic_string_view v) const noexcept;
    constexpr int compare(size_type pos1, size_type count1, basic_string_view v) const;
    constexpr int compare(
        size_type pos1, size_type count1, basic_string_view v, size_type pos2, size_type count2) const;
    constexpr int compare(CharT const* const s) const;
    constexpr int compare(size_type pos1, size_type count1, CharT const* const s) const;
    constexpr int compare(size_type pos1, size_type count1, CharT const* const s, size_type count2) const;
    /** find substring in the view */
    constexpr size_type find(const CharT str, size_type pos = 0) const noexcept;
    constexpr size_type find(const basic_string_view& str, size_type pos = 0) const noexcept;
    /* find the last occurrence of a substring in the view */
    constexpr size_type rfind(const CharT str, size_type pos = npos) const noexcept;
    constexpr size_type rfind(const basic_string_view& str, size_type pos = npos) const noexcept;
    /* find first occurance of characters in the view */
    constexpr size_type find_first_of(const basic_string_view& str, size_type pos = 0) const noexcept;
    constexpr size_type find_first_of(CharT ch, size_type pos = 0) const noexcept;
    /* find last occurrence of characters in the view */
    constexpr size_type find_last_of(const basic_string_view& str, size_type pos = npos) const noexcept;
    constexpr size_type find_last_of(CharT ch, size_type pos = npos) const noexcept;

    /* checks if the string view starts with the given prefix */
    constexpr bool starts_with(basic_string_view sv) const noexcept;
    constexpr bool starts_with(CharT ch) const noexcept;
    constexpr bool starts_with(const CharT* s) const;

    /* checks if the string view ends with the given suffix */
    constexpr bool ends_with(basic_string_view sv) const noexcept;
    constexpr bool ends_with(CharT ch) const noexcept;
    constexpr bool ends_with(const CharT* s) const;

    /* find first absence of characters
    find_first_not_of

    find last absence of characters
    find_last_not_of
*/
private:
    const_pointer begin_ { nullptr };
    size_type size_ { 0 };
};

template<class CharT>
constexpr size_t constexpr_strlen(CharT const* const str) noexcept
{
    if (!str) {
        return 0u;
    }
    auto tmp = str;
    while (*tmp) {
        ++tmp;
    }
    return static_cast<size_t>(tmp - str);
}

template<class CharT>
constexpr basic_string_view<CharT>::basic_string_view(CharT const* const s, size_type count) : begin_(s), size_(count)
{}

template<class CharT>
constexpr basic_string_view<CharT>::basic_string_view(CharT const* const s) : begin_(s), size_(constexpr_strlen(s))
{}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_iterator basic_string_view<CharT>::begin() const noexcept
{
    return const_iterator(begin_);
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_iterator basic_string_view<CharT>::cbegin() const noexcept
{
    return const_iterator(begin_);
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_iterator basic_string_view<CharT>::end() const noexcept
{
    return const_iterator(begin_ + size_);
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_iterator basic_string_view<CharT>::cend() const noexcept
{
    return const_iterator(begin_ + size_);
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reverse_iterator basic_string_view<CharT>::rbegin() const noexcept
{
    return const_reverse_iterator(const_iterator(begin_ + size_));
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reverse_iterator basic_string_view<CharT>::crbegin() const noexcept
{
    return const_reverse_iterator(const_iterator(begin_ + size_));
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reverse_iterator basic_string_view<CharT>::rend() const noexcept
{
    return const_reverse_iterator(const_iterator(begin_));
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reverse_iterator basic_string_view<CharT>::crend() const noexcept
{
    return const_reverse_iterator(const_iterator(begin_));
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_pointer basic_string_view<CharT>::data() const noexcept
{
    return begin_;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reference basic_string_view<CharT>::operator[](size_type pos) const
{
    return begin_[pos];
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reference basic_string_view<CharT>::at(size_type pos) const
{
    return begin_[pos];
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reference basic_string_view<CharT>::front() const
{
    // could assert that size_ > 0
    return begin_[0];
}

template<class CharT>
constexpr typename basic_string_view<CharT>::const_reference basic_string_view<CharT>::back() const
{
    // could assert that size_ > 0
    return begin_[size_ - 1];
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::size() const noexcept
{
    return size_;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::length() const noexcept
{
    return size_;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::max_size() const noexcept
{
    return npos - 1;
}

template<class CharT>
constexpr bool basic_string_view<CharT>::empty() const noexcept
{
    return !size_;
}

template<class CharT>
constexpr void basic_string_view<CharT>::remove_prefix(size_type n)
{
    if (n > size_) {
        begin_ += size_;
        size_ = 0;
    } else {
        size_ -= n;
        begin_ += n;
    }
}

template<class CharT>
constexpr void basic_string_view<CharT>::remove_suffix(size_type n)
{
    if (n > size_) {
        size_ = 0;
    } else {
        size_ -= n;
    }
}

template<class CharT>
constexpr void basic_string_view<CharT>::swap(basic_string_view& v) noexcept
{
    basic_string_view sv(v);
    v = *this;
    *this = sv;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::copy(
    CharT* dest, size_type count, size_type pos) const
{
    if (pos > size_) {
        return {}; // should throw
    }
    auto const left = size_ - pos;
    auto const copy = (left < count) ? left : count;
    const_pointer end = begin_ + pos + copy;
    const_pointer i = begin_ + pos;
    while (i < end) {
        *dest++ = *i++;
    }
    return copy;
}

template<class CharT>
constexpr basic_string_view<CharT> basic_string_view<CharT>::substr(size_type pos, size_type count) const
{
    if (pos > size_) {
        return {}; // should throw
    }
    if (auto const left = size_ - pos; left < count) {
        return { begin_ + pos, left };
    } else {
        return { begin_ + pos, count };
    }
}

template<class CharT>
constexpr int basic_string_view<CharT>::compare(basic_string_view v) const noexcept
{
    auto const size = (size_ < v.size_) ? size_ : v.size_;
    auto const* ptr1 = begin_;
    auto const* ptr2 = v.begin_;
    auto const* end = begin_ + size;
    while (ptr1 != end) {
        auto const res = static_cast<int>(*ptr1++) - static_cast<int>(*ptr2++);
        if (res) {
            return res;
        }
    }
    if (size_ < v.size_) {
        return -1;
    } else if (size_ > v.size_) {
        return 1;
    }
    return 0;
}

template<class CharT>
constexpr int basic_string_view<CharT>::compare(size_type pos1, size_type count1, basic_string_view v) const
{
    return substr(pos1, count1).compare(v);
}

template<class CharT>
constexpr int basic_string_view<CharT>::compare(
    size_type pos1, size_type count1, basic_string_view v, size_type pos2, size_type count2) const
{
    return substr(pos1, count1).compare(v.substr(pos2, count2));
}

template<class CharT>
constexpr int basic_string_view<CharT>::compare(CharT const* const s) const
{
    auto const size = size_;
    auto const* ptr1 = begin_;
    auto const* ptr2 = s;
    auto const* end = begin_ + size;
    while ((ptr1 != end) && *ptr2) {
        auto const res = static_cast<int>(*ptr1++) - static_cast<int>(*ptr2++);
        if (res) {
            return res;
        }
    }
    if ((ptr1 == end) && *ptr2) {
        return -1;
    } else if ((ptr1 != end) && !*ptr2) {
        return 1;
    }
    return 0;
}

template<class CharT>
constexpr int basic_string_view<CharT>::compare(size_type pos1, size_type count1, CharT const* const s) const
{
    return substr(pos1, count1).compare(s);
}

template<class CharT>
constexpr int basic_string_view<CharT>::compare(
    size_type pos1, size_type count1, CharT const* const s, size_type count2) const
{
    return substr(pos1, count1).compare(basic_string_view(s, count2));
}

template<class CharT>
constexpr bool operator==(const basic_string_view<CharT> lhs, const basic_string_view<CharT> rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    return lhs.compare(rhs) == 0;
}

template<class CharT, int = 1>
constexpr bool operator==(
    const basic_string_view<CharT> lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    return lhs.compare(rhs) == 0;
}

template<class CharT, int = 2>
constexpr bool operator==(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_string_view<CharT> rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return false;
    }
    return lhs.compare(rhs) == 0;
}

template<class CharT>
constexpr bool operator!=(const basic_string_view<CharT> lhs, const basic_string_view<CharT> rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return true;
    }
    return lhs.compare(rhs) != 0;
}

template<class CharT, int = 1>
constexpr bool operator!=(
    const basic_string_view<CharT> lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return true;
    }
    return lhs.compare(rhs) != 0;
}

template<class CharT, int = 2>
constexpr bool operator!=(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_string_view<CharT> rhs) noexcept
{
    if (lhs.size() != rhs.size()) {
        return true;
    }
    return lhs.compare(rhs) != 0;
}

template<class CharT>
constexpr bool operator<(const basic_string_view<CharT> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) < 0;
}

template<class CharT, int = 1>
constexpr bool operator<(
    const basic_string_view<CharT> lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    return lhs.compare(rhs) < 0;
}

template<class CharT, int = 2>
constexpr bool operator<(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) < 0;
}

template<class CharT>
constexpr bool operator<=(const basic_string_view<CharT> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) <= 0;
}

template<class CharT, int = 1>
constexpr bool operator<=(
    const basic_string_view<CharT> lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    return lhs.compare(rhs) <= 0;
}

template<class CharT, int = 2>
constexpr bool operator<=(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) <= 0;
}

template<class CharT>
constexpr bool operator>(const basic_string_view<CharT> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) > 0;
}

template<class CharT, int = 1>
constexpr bool operator>(
    const basic_string_view<CharT> lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    return lhs.compare(rhs) > 0;
}

template<class CharT, int = 2>
constexpr bool operator>(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) > 0;
}

template<class CharT>
constexpr bool operator>=(const basic_string_view<CharT> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) >= 0;
}

template<class CharT, int = 1>
constexpr bool operator>=(
    const basic_string_view<CharT> lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    return lhs.compare(rhs) >= 0;
}

template<class CharT, int = 2>
constexpr bool operator>=(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_string_view<CharT> rhs) noexcept
{
    return lhs.compare(rhs) >= 0;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::find(
    const CharT str, size_type pos) const noexcept
{
    if (pos >= length()) {
        return string_view::npos;
    }

    auto first1 = begin() + static_cast<difference_type>(pos);
    auto end1 = end();

    while (first1 != end1 && (*first1 != str)) {
        ++first1;
    }
    if (first1 != end1) {
        return static_cast<size_type>(first1 - begin());
    } else {
        return string_view::npos;
    }
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::find(
    const basic_string_view& str, size_type pos) const noexcept
{
    if (str.size() == 1) {
        return find(str[0], pos);
    }

    if (pos >= length() || (pos + str.length() > length())) {
        return string_view::npos;
    }

    auto first1 = begin() + static_cast<difference_type>(pos);
    const auto end1 = end();
    const auto first2 = str.begin();
    const auto end2 = str.end();

    while (first1 != end1) {
        // look for first character
        while (first1 != end1 && (*first1 != *first2)) {
            ++first1;
        }
        if (first1 != end1) {
            // continue comparing the next characters
            auto next1 = first1;
            ++next1;
            if (next1 != end1) {
                auto next2 = first2 + 1;
                while (*next1 == *next2) {
                    ++next2;
                    if (next2 == end2) {
                        // reached end of searched string - found it
                        return static_cast<size_type>(first1 - begin());
                    }
                    ++next1;
                    if (next1 == end1) {
                        // reached end of string - no match
                        return string_view::npos;
                    }
                }
                // try again starting from the next
                ++first1;
            } else {
                // reached end of string - no match
                return string_view::npos;
            }
        }
    }
    // reached end of string - no match
    return string_view::npos;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::rfind(
    const CharT str, size_type pos) const noexcept
{
    return find_last_of(str, pos);
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::rfind(
    const basic_string_view& str, size_type pos) const noexcept
{
    const auto len = length();
    if (len == 0) {
        return string_view::npos;
    }
    const auto len2 = str.length();
    if (len2 > len) {
        return string_view::npos;
    }
    if ((pos == npos) || (pos > len)) {
        pos = len;
    }
    if (len2 == 0) {
        return pos;
    }
    if ((pos + len2) > len) {
        pos = len - len2;
    }
    if (pos < 0) {
        return string_view::npos;
    }
    CharT const* s1 = data() + pos;
    const auto todo = pos;
    for (auto it = 0u; it <= todo; it++, pos--, s1--) {
        const auto diff = basic_string_view<CharT>(s1, len2).compare(str);
        if (diff == 0) {
            return pos;
        }
    };
    return string_view::npos;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::find_first_of(
    const basic_string_view& str, size_type pos) const noexcept
{
    const auto len = length();
    if (len == 0) {
        return string_view::npos;
    }
    if (pos >= len) {
        return string_view::npos;
    }
    CharT const* s1 = data() + pos;
    CharT const* const s2 = str.data();
    CharT const* const s3 = s2 + str.length();
    for (; pos < len; pos++, s1++) {
        for (auto s = s2; s != s3; s++) {
            if (*s == *s1) {
                return pos;
            }
        }
    }
    return string_view::npos;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::find_first_of(
    CharT ch, size_type pos) const noexcept
{
    const auto len = length();
    if (len == 0) {
        return string_view::npos;
    }
    if (pos >= len) {
        return string_view::npos;
    }
    CharT const* s1 = data() + pos;
    for (; pos < len; pos++, s1++) {
        if (*s1 == ch) {
            return pos;
        }
    }
    return string_view::npos;
}

template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::find_last_of(
    const basic_string_view& str, size_type pos) const noexcept
{
    const auto len = length();
    if (len == 0) {
        return string_view::npos;
    }
    if (pos >= len) {
        pos = len - 1;
    }
    CharT const* s1 = data() + pos;
    CharT const* const s2 = str.data();
    CharT const* const s3 = s2 + str.length();
    for (;;) {
        for (auto s = s2; s != s3; s++) {
            if (*s == *s1) {
                return pos;
            }
        }
        if (pos == 0) {
            break;
        }
        pos--;
        s1--;
    }
    return string_view::npos;
}
template<class CharT>
constexpr typename basic_string_view<CharT>::size_type basic_string_view<CharT>::find_last_of(
    CharT ch, size_type pos) const noexcept
{
    const auto len = length();
    if (len == 0) {
        return string_view::npos;
    }
    if (pos >= len) {
        pos = len - 1;
    }
    CharT const* s1 = data() + pos;
    for (;;) {
        if (*s1 == ch) {
            return pos;
        }
        if (pos == 0) {
            break;
        }
        pos--;
        s1--;
    }
    return string_view::npos;
}

template<class CharT>
constexpr bool basic_string_view<CharT>::starts_with(basic_string_view sv) const noexcept
{
    return compare(0U, sv.size_, sv) == 0;
}

template<class CharT>
constexpr bool basic_string_view<CharT>::starts_with(CharT ch) const noexcept
{
    return (size_) && (*begin_ == ch);
}

template<class CharT>
constexpr bool basic_string_view<CharT>::starts_with(const CharT* s) const
{
    if (!s) {
        return true;
    }
    auto const* ptr1 = begin_;
    auto const* ptr2 = s;
    auto const* end = begin_ + size_;
    while ((ptr1 != end) && *ptr2) {
        if ((*ptr1++) != (*ptr2++)) {
            return false;
        }
    }
    return !(*ptr2);
}

template<class CharT>
constexpr bool basic_string_view<CharT>::ends_with(basic_string_view sv) const noexcept
{
    return (size_ >= sv.size_) && (compare(size_ - sv.size_, sv.size_, sv) == 0);
}

template<class CharT>
constexpr bool basic_string_view<CharT>::ends_with(CharT ch) const noexcept
{
    return size_ && *(begin_ + size_ - 1U) == ch;
}

template<class CharT>
constexpr bool basic_string_view<CharT>::ends_with(const CharT* s) const
{
    return ends_with(basic_string_view(s));
}

template<typename T>
uint64_t hash(const T& b);

template<>
inline uint64_t hash(const string_view& value)
{
    return BASE_NS::FNV1aHash(value.data(), value.size());
}
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_STRING_VIEW_H