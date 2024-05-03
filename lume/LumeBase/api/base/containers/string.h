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

#ifndef API_BASE_CONTAINERS_STRING_H
#define API_BASE_CONTAINERS_STRING_H

#include <cstddef>
#include <cstdint>

#include <base/containers/allocator.h>
#include <base/containers/iterator.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
template<class CharT>
class basic_string;

using string = BASE_NS::basic_string<char>;
using wstring = BASE_NS::basic_string<wchar_t>;

template<class CharT>
class basic_string {
public:
    using string_view = basic_string_view<CharT>;
    using value_type = CharT;
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using reference = value_type&;
    using const_reference = const value_type&;
    using const_iterator = BASE_NS::const_iterator<basic_string>;
    using iterator = BASE_NS::iterator<basic_string>;
    using const_reverse_iterator = BASE_NS::reverse_iterator<const_iterator>;
    using reverse_iterator = BASE_NS::reverse_iterator<iterator>;
    using size_type = size_t;
    using difference_type = ptrdiff_t;

    static constexpr size_type npos = size_type(-1);

    template<typename T>
    using StringViewLikeNotCStr =
        enable_if_t<is_convertible_v<const T&, string_view> && !is_convertible_v<const T&, const_pointer>>;

    template<typename T>
    using StringViewLike = enable_if_t<is_convertible_v<const T&, string_view>>;

    basic_string() noexcept : basic_string(default_allocator()) {}

    ~basic_string()
    {
        if (!is_short()) {
            allocator_.free(data_.longString.begin);
        }
    }

    explicit basic_string(allocator& alloc) noexcept : allocator_(alloc)
    {
        set_short(true);
        data_.shortString.begin[1] = '\0';
        data_.shortString.size = static_cast<value_type>(shortCapacity);
    }

    basic_string(const basic_string& str) : basic_string(str, default_allocator()) {}

    basic_string(const basic_string& str, allocator& alloc) : allocator_(alloc)
    {
        if (str.data_.longString.isShort) {
            data_.shortString = str.data_.shortString;
        } else {
            const auto len = str.data_.longString.size;
            if (auto ptr = allocator_.alloc(len + 1); ptr) {
                // destination and source are valid and the allocation sizes are for at least len characters.
                CloneData(ptr, len * sizeof(value_type), str.data_.longString.begin, len * sizeof(value_type));
                ptr[len] = '\0';

                data_.longString = str.data_.longString;
                data_.longString.capacity = len;
                data_.longString.begin = ptr;
            } else {
                set_short(true);
                data_.shortString.begin[1] = '\0';
                data_.shortString.size = static_cast<value_type>(shortCapacity);
            }
        }
    }

    basic_string(basic_string&& a) noexcept : allocator_(a.allocator_), data_(BASE_NS::move(a.data_))
    {
        a.set_short(true);
        a.data_.shortString.begin[1] = '\0';
        a.data_.shortString.size = static_cast<value_type>(shortCapacity);
    }

    basic_string(const_pointer const str) : basic_string(str, default_allocator()) {}

    basic_string(const_pointer const str, allocator& alloc) : basic_string(str, constexpr_strlen(str), alloc) {}

    basic_string(const_pointer const str, size_type count) : basic_string(str, count, default_allocator()) {}

    basic_string(const_pointer const str, size_type count, allocator& alloc) : allocator_(alloc)
    {
        construct(str, count);
    }

    basic_string(size_type count, const value_type a) : basic_string(count, a, default_allocator()) {}

    basic_string(size_type count, const value_type a, allocator& alloc) : allocator_(alloc)
    {
        set_short(true);
        data_.shortString.size = static_cast<value_type>(shortCapacity);
        assign(count, a);
    }

    template<class StringT, class = StringViewLikeNotCStr<StringT>>
    explicit basic_string(const StringT& a) : basic_string(a, default_allocator())
    {}

    template<class StringT, class = StringViewLikeNotCStr<StringT>>
    explicit basic_string(const StringT& a, allocator& alloc) : allocator_(alloc)
    {
        if constexpr (is_same_v<StringT, string_view>) {
            construct(a.data(), a.size());
        } else {
            const auto view = string_view(a);
            construct(view.data(), view.size());
        }
    }

    template<class StringT, class = StringViewLike<StringT>>
    basic_string(const StringT& a, size_type pos, size_type n) : basic_string(a, pos, n, default_allocator())
    {}

    template<class StringT, class = StringViewLike<StringT>>
    basic_string(const StringT& a, size_type pos, size_type n, allocator& alloc)
        : basic_string(string_view(a).substr(pos, n), alloc)
    {}

    bool empty() const noexcept
    {
        return !size();
    }

    void clear()
    {
        assign("", 0);
    }

    value_type& back()
    {
        return *(data() + size() - 1);
    }

    const value_type& back() const
    {
        return *(data() + size() - 1);
    }

    value_type& front()
    {
        return *data();
    }

    const value_type& front() const
    {
        return *data();
    }

    size_type capacity() const noexcept
    {
        if (is_short()) {
            return shortCapacity;
        } else {
            return data_.longString.capacity;
        }
    }

    size_type length() const noexcept
    {
        return size();
    }

    size_type size() const noexcept
    {
        if (is_short()) {
            return shortCapacity - data_.shortString.size;
        } else {
            return data_.longString.size;
        }
    }

    basic_string& operator=(const basic_string& a)
    {
        if (&a != this) {
            assign(a.data(), a.length());
        }
        return *this;
    }

    basic_string& operator=(basic_string&& a) noexcept
    {
        if (&a != this) {
            if (!is_short()) {
                allocator_.free(data_.longString.begin);
            }
            allocator_ = a.allocator_;
            data_ = BASE_NS::move(a.data_);

            a.set_short(true);
            a.data_.shortString.begin[1] = '\0';
            a.data_.shortString.size = static_cast<value_type>(shortCapacity);
        }
        return *this;
    }

    basic_string& operator=(const value_type ch)
    {
        auto ptr = data();
        *ptr++ = ch;
        *ptr++ = '\0';
        set_size(1);
        return *this;
    }

    basic_string& operator=(const_pointer const a)
    {
        if (data() != a) {
            assign(a, constexpr_strlen(a));
        }
        return *this;
    }

    template<class StringT, class = StringViewLikeNotCStr<StringT>>
    basic_string& operator=(const StringT& a)
    {
        const auto view = string_view(a);
        if (data() != view.data()) {
            assign(view.data(), view.length());
        } else {
            resize(view.length());
        }
        return *this;
    }

    basic_string& assign(const_pointer const str)
    {
        const auto view = string_view(str);
        return assign(view.data(), view.length());
    }

    basic_string& assign(const_pointer const str, size_type count)
    {
        if (count) {
            reserve(count);
            const pointer dst = data();
            const size_type cap = capacity();
            if (static_cast<size_type>((dst > str) ? (dst - str) : (str - dst)) >= count) {
                CloneData(dst, cap * sizeof(value_type), str, count * sizeof(value_type));
            } else {
                MoveData(dst, cap * sizeof(value_type), str, count * sizeof(value_type));
            }
            dst[count] = '\0';
        } else {
            const pointer dst = data();
            *dst = '\0';
        }
        set_size(count);
        return *this;
    }

    basic_string& assign(size_type count, value_type ch)
    {
        if (count) {
            reserve(count);
            const pointer dst = data();
            const size_type cap = capacity();
            count = (count <= cap) ? count : cap;
            // dst is valid, count fits capacity
            ClearToValue(dst, cap * sizeof(value_type), static_cast<uint8_t>(ch), count * sizeof(value_type));
            dst[count] = '\0';
            set_size(count);
        } else {
            const pointer dst = data();
            *dst = '\0';
            set_size(0);
        }
        return *this;
    }

    const_iterator cbegin() const noexcept
    {
        return begin();
    }

    const_iterator cend() const noexcept
    {
        return end();
    }

    iterator begin() noexcept
    {
        return iterator(data());
    }

    iterator end() noexcept
    {
        return iterator(data() + size());
    }

    const_iterator begin() const noexcept
    {
        return const_iterator(data());
    }

    const_iterator end() const noexcept
    {
        return const_iterator(data() + size());
    }

    pointer data() noexcept
    {
        if (is_short()) {
            return data_.shortString.begin + 1;
        } else {
            return data_.longString.begin;
        }
    }

    const_pointer data() const noexcept
    {
        if (is_short()) {
            return data_.shortString.begin + 1;
        } else {
            return data_.longString.begin;
        }
    }

    const_pointer c_str() const
    {
        return data();
    }

    operator string_view() const noexcept
    {
        return string_view(data(), size());
    }

    size_type copy(pointer dst, size_type todo) const
    {
        if (const auto len = size(); todo > len) {
            todo = len;
        }
        auto ptr = data();
        const auto end = ptr + todo;
        while (ptr != end) {
            *dst++ = *ptr++;
        }
        return todo;
    }

    void reserve(size_type size)
    {
        if (size > capacity()) {
            // setup new storage with old data
            allocate(size);
        }
    }

    void resize(size_type size, value_type ch)
    {
        if (const auto oldSize = length(); size < oldSize) {
            data()[size] = '\0';
        } else if (size > oldSize) {
            reserve(size);
            const auto ptr = data() + oldSize;
            const size_type cap = capacity();
            size = (size <= cap) ? size : cap;
            const auto count = size - oldSize;
            // ptr is valid, count fits capacity
            ClearToValue(ptr, cap * sizeof(value_type), static_cast<uint8_t>(ch), count * sizeof(value_type));
            ptr[count] = '\0';
        }

        set_size(size);
    }

    void resize(size_type size)
    {
        resize(size, '\0');
    }

    template<class StringT, class = StringViewLikeNotCStr<StringT>>
    basic_string& operator+=(const StringT& a)
    {
        const auto view = string_view(a);
        return append(view.data(), view.length());
    }

    basic_string& operator+=(const value_type a)
    {
        return push_back(a);
    }

    basic_string& operator+=(const_pointer const a)
    {
        return append(a, constexpr_strlen(a));
    }

    const_reference operator[](size_type i) const
    {
        return data()[i];
    }

    reference operator[](size_type i)
    {
        return data()[i];
    }

    basic_string& replace(const_iterator first, const_iterator last, const string_view& str)
    {
        const auto pos = first - cbegin();
        const auto replace = last - first;
        const auto add = static_cast<difference_type>(str.length());
        if (add < replace) {
            CloneData(data() + pos, replace * sizeof(value_type), str.data(), add * sizeof(value_type));
            erase(first + add, last);
        } else if (add > replace) {
            CloneData(data() + pos, replace * sizeof(value_type), str.data(), replace * sizeof(value_type));
            insert(static_cast<size_type>(pos + replace), str.data() + replace, static_cast<size_type>(add - replace));
        } else {
            CloneData(data() + pos, replace * sizeof(value_type), str.data(), add * sizeof(value_type));
        }
        return *this;
    }

    string_view substr(size_type pos = 0, size_type count = npos) const
    {
        return string_view(*this).substr(pos, count);
    }

    basic_string& erase()
    {
        return erase(0, npos);
    }

    basic_string& erase(const size_type off)
    {
        return erase(off, npos);
    }

    basic_string& erase(const size_type off, size_type count)
    {
        const auto oldSize = size();
        if (off > oldSize) {
            return *this;
        } else if (count == 0) {
            return *this;
        } else {
            auto newSize = oldSize;
            const auto dst = data() + off;
            if (count < (oldSize - off)) {
                const auto dstSize = capacity() - off;
                const auto src = dst + count;
                const auto srcSize = oldSize + 1 - off - count;
                // dst and src are valid, dst < src, and srcSize doesn't exceed capacity.
                MoveData(dst, dstSize * sizeof(value_type), src, srcSize * sizeof(value_type));
                newSize -= count;
            } else {
                *dst = '\0';
                newSize = off;
            }

            set_size(newSize);
        }
        return *this;
    }

    iterator erase(const_iterator pos)
    {
        const auto offset = pos - cbegin();
        const auto count = 1U;
        erase(static_cast<size_type>(offset), count);

        return iterator(begin() + offset);
    }

    iterator erase(const_iterator first, const_iterator last)
    {
        const auto offset = first - cbegin();
        const auto count = static_cast<size_type>(last - first);
        erase(static_cast<size_type>(offset), count);

        return iterator(begin() + offset);
    }

    basic_string& insert(size_type pos, const_pointer str)
    {
        const auto view = string_view(str);
        return insert(pos, view.data(), view.length());
    }

    basic_string& insert(size_type pos, const_pointer str, size_type n)
    {
        const auto oldSize = size();
        if (pos > oldSize) {
            pos = oldSize;
        }
        const auto newSize = oldSize + n;
        if (newSize > capacity()) {
            const auto oldPtr = data();
            const auto ptr = allocator_.alloc(newSize + 1);
            CloneData(ptr, newSize * sizeof(value_type), oldPtr, pos * sizeof(value_type));
            CloneData(ptr + pos + n, (newSize - pos - n) * sizeof(value_type), oldPtr + pos,
                (oldSize - pos) * sizeof(value_type));
            CloneData(ptr + pos, (newSize - pos) * sizeof(value_type), str, n * sizeof(value_type));
            ptr[newSize] = '\0';

            if (!is_short()) {
                allocator_.free(oldPtr);
            }

            data_.longString.capacity = newSize;
            data_.longString.size = newSize;
            data_.longString.begin = ptr;
            set_short(false);
        } else {
            auto ptr = data();
            if (pos < oldSize) {
                auto dst = ptr + newSize;
                auto src = ptr + oldSize;
                auto count = oldSize + 1 - pos;
                while (count--) {
                    *dst-- = *src--;
                }
            }
            CloneData(ptr + pos, (newSize - pos) * sizeof(value_type), str, n * sizeof(value_type));
            ptr[newSize] = '\0';

            set_size(newSize);
        }
        return *this;
    }

    basic_string& push_back(value_type a)
    {
        const auto oldSize = size();
        if (size_type cap = capacity(); oldSize == cap) {
            if (!grow(1)) {
                return *this;
            }
        }

        const auto ptr = data();
        ptr[oldSize] = a;
        ptr[oldSize + 1] = '\0';
        set_size(oldSize + 1);
        return *this;
    }

    basic_string& append(size_type count, value_type a)
    {
        if (count) {
            const auto oldSize = size();
            const auto newSize = oldSize + count;
            if (size_type cap = capacity(); cap < newSize) {
                if (!grow(newSize)) {
                    return *this;
                }
            }

            const auto ptr = data();
            for (auto i = ptr + oldSize; i != (ptr + newSize); ++i) {
                *i = a;
            }
            ptr[newSize] = '\0';

            set_size(newSize);
        }
        return *this;
    }

    basic_string& append(const_pointer const a)
    {
        return append(a, constexpr_strlen(a));
    }

    basic_string& append(const_pointer const str, size_type count)
    {
        if (str && count) {
            size_type oldSize;
            size_type oldCapacity;
            pointer dst;
            if (is_short()) {
                oldSize = shortCapacity - data_.shortString.size;
                oldCapacity = shortCapacity;
                dst = data_.shortString.begin + 1;
            } else {
                oldSize = data_.longString.size;
                oldCapacity = data_.longString.capacity;
                dst = data_.longString.begin;
            }

            const auto newSize = oldSize + count;
            if (oldCapacity < newSize) {
                oldCapacity += oldCapacity / 2;
                if (!allocate(newSize < oldCapacity ? oldCapacity : newSize)) {
                    return *this;
                }
                dst = data_.longString.begin;
                oldCapacity = data_.longString.capacity;
            }

            // dst and src are valid, oldSize + count is less than capacity.
            CloneData(dst + oldSize, oldCapacity * sizeof(value_type), str, count * sizeof(value_type));
            dst[newSize] = '\0';
            set_size(newSize);
        }
        return *this;
    }

    basic_string& append(const string_view& b)
    {
        return append(b.data(), b.length());
    }

    basic_string& append(const string_view& b, size_type pos)
    {
        auto count = b.length();
        count = (pos < count) ? (count - pos) : 0;
        return append(b.data() + pos, count);
    }

    basic_string& append(const string_view& b, size_type pos, size_type count)
    {
        auto const strLen = b.length();
        if (pos > strLen) {
            count = 0;
            pos = strLen ? (strLen - 1) : strLen;
        } else if (count == npos) {
            count = strLen - pos;
        } else if (count > (strLen - pos)) {
            count = strLen - pos;
        }
        return append(b.data() + pos, count);
    }

    /** compares two strings */
    int compare(string_view v) const noexcept
    {
        return string_view(*this).compare(v);
    }

    int compare(size_type pos1, size_type count1, string_view v) const
    {
        return substr(pos1, count1).compare(v);
    }

    int compare(size_type pos1, size_type count1, string_view v, size_type pos2, size_type count2) const
    {
        return substr(pos1, count1).compare(v.substr(pos2, count2));
    }

    int compare(CharT const* const s) const
    {
        return string_view(*this).compare(s);
    }

    int compare(size_type pos1, size_type count1, CharT const* const s) const
    {
        return substr(pos1, count1).compare(s);
    }

    int compare(size_type pos1, size_type count1, CharT const* const s, size_type count2) const
    {
        return substr(pos1, count1).compare(basic_string_view(s, count2));
    }

    /** find substring in the view */
    size_type find(const value_type str, size_type pos = 0) const noexcept
    {
        return string_view(*this).find(str, pos);
    }

    size_type find(const string_view& str, size_type pos = 0) const noexcept
    {
        return string_view(*this).find(str, pos);
    }

    /* find the last occurrence of a substring in the view */
    size_type rfind(const value_type str, size_type pos = npos) const noexcept
    {
        return string_view(*this).rfind(str, pos);
    }

    size_type rfind(const string_view& str, size_type pos = npos) const noexcept
    {
        return string_view(*this).rfind(str, pos);
    }

    /* find first occurance of characters in the view */
    size_type find_first_of(const string_view& str, size_type pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(str, pos);
    }

    size_type find_first_of(value_type ch, size_type pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(ch, pos);
    }

    /* find last occurrence of characters in the view */
    size_type find_last_of(const string_view& str, size_type pos = npos) const noexcept
    {
        return string_view(*this).find_last_of(str, pos);
    }

    size_type find_last_of(value_type ch, size_type pos = npos) const noexcept
    {
        return string_view(*this).find_last_of(ch, pos);
    }

    /* checks if the string starts with the given prefix */
    bool starts_with(basic_string_view<CharT> sv) const noexcept
    {
        return operator basic_string_view<CharT>().starts_with(sv);
    }

    bool starts_with(CharT ch) const noexcept
    {
        return operator basic_string_view<CharT>().starts_with(ch);
    }

    bool starts_with(const CharT* s) const
    {
        return operator basic_string_view<CharT>().starts_with(s);
    }

    /* checks if the string ends with the given suffix */
    bool ends_with(basic_string_view<CharT> sv) const noexcept
    {
        return operator basic_string_view<CharT>().ends_with(sv);
    }

    bool ends_with(CharT ch) const noexcept
    {
        return operator basic_string_view<CharT>().ends_with(ch);
    }

    bool ends_with(const CharT* s) const
    {
        return operator basic_string_view<CharT>().ends_with(s);
    }
    /* find first absence of characters
    find_first_not_of

    find last absence of characters
    find_last_not_of */

    basic_string& upper()
    {
        for (size_type i = 0; i < length(); i++) {
            if (data()[i] >= 'a' && data()[i] <= 'z') {
                data()[i] = (data()[i] - 'a') + 'A';
            }
        }
        return *this;
    }

    basic_string& lower()
    {
        for (size_type i = 0; i < length(); i++) {
            if (data()[i] >= 'A' && data()[i] <= 'Z') {
                data()[i] = (data()[i] - 'A') + 'a';
            }
        }
        return *this;
    }

    basic_string toUpper() const
    {
        basic_string res;
        res.resize(length());
        for (size_type i = 0; i < length(); i++) {
            if (data()[i] >= 'a' && data()[i] <= 'z') {
                res[i] = (data()[i] - 'a') + 'A';
            } else {
                res[i] = data()[i];
            }
        }
        return res;
    }

    basic_string toLower() const
    {
        basic_string res;
        res.resize(length());
        for (size_type i = 0; i < length(); i++) {
            if (data()[i] >= 'A' && data()[i] <= 'Z') {
                res[i] = (data()[i] - 'A') + 'a';
            } else {
                res[i] = data()[i];
            }
        }
        return res;
    }

protected:
    inline bool is_short() const
    {
        return data_.longString.isShort;
    }

    inline void set_short(bool isShort)
    {
        data_.longString.isShort = isShort;
    }

    inline void set_size(size_type size)
    {
        if (is_short()) {
            data_.shortString.size = static_cast<value_type>(shortCapacity - size);
        } else {
            data_.longString.size = size;
        }
    }

    inline bool grow(const size_type minCapacity)
    {
        auto cap = capacity();
        cap += cap / 2;
        return allocate(minCapacity < cap ? cap : minCapacity);
    }

    bool allocate(size_type size)
    {
        // setup new storage with old data
        if (auto ptr = allocator_.alloc(size + 1); ptr) {
            const auto oldSize = length();
            if (oldSize) {
                CloneData(ptr, size * sizeof(value_type), data(), oldSize * sizeof(value_type));
            }
            ptr[oldSize] = '\0';

            if (!is_short()) {
                allocator_.free(data_.longString.begin);
            }

            set_short(false);
            data_.longString.capacity = size;
            data_.longString.size = oldSize;
            data_.longString.begin = ptr;
            return true;
        }
        return false;
    }

    void construct(const_pointer const str, size_type count)
    {
        if (!str || count == 0) {
            set_short(true);
            data_.shortString.begin[1] = '\0';
            data_.shortString.size = static_cast<value_type>(shortCapacity);
        } else {
            pointer dst;
            if (count <= shortCapacity) {
                dst = data_.shortString.begin + 1;
                set_short(true);
                data_.shortString.size = static_cast<value_type>(shortCapacity - count);
            } else {
                dst = allocator_.alloc(count + 1);
                if (!dst) {
                    set_short(true);
                    data_.shortString.begin[1] = '\0';
                    data_.shortString.size = static_cast<value_type>(shortCapacity);
                    return;
                }
                set_short(false);
                data_.longString.capacity = count;
                data_.longString.size = count;
                data_.longString.begin = dst;
            }
            // destination and source are valid and the allocation sizes are for at least count characters.
            CloneData(dst, count * sizeof(value_type), str, count * sizeof(value_type));
            dst[count] = '\0';
        }
    }

    struct LongString {
        bool isShort;
        size_type capacity;
        size_type size;
        pointer begin;
    };

    // short string capacity: one value_type for the size and another for isShort
    static constexpr auto shortCapacity = (sizeof(LongString) - 2u * sizeof(value_type)) / sizeof(value_type);

    struct ShortString {
        // add one for isShort in the begining
        value_type begin[shortCapacity + 1];
        value_type size;
    };

    union Data {
        LongString longString;
        ShortString shortString;
    };

    // Wrapper to create a "re-seatable" reference.
    class Wrapper {
    public:
        inline Wrapper(allocator& a) : allocator_(&a) {}
        inline Wrapper& operator=(allocator& a)
        {
            allocator_ = &a;
            return *this;
        }
        inline pointer alloc(size_type size)
        {
            if ((allocator_) && (allocator_->alloc)) {
                return static_cast<pointer>(allocator_->alloc(allocator_->instance, sizeof(value_type) * size));
            }
            return nullptr;
        }
        inline void free(void* ptr)
        {
            if ((allocator_) && (allocator_->free)) {
                allocator_->free(allocator_->instance, ptr);
            }
        }
        allocator& get()
        {
            BASE_ASSERT(allocator_ != nullptr);
            return *allocator_;
        }
        const allocator& get() const
        {
            BASE_ASSERT(allocator_ != nullptr);
            return *allocator_;
        }

    private:
        allocator* allocator_ { nullptr };
    } allocator_;
    Data data_;
};

inline string operator+(const string& a, const string& b)
{
    string res;
    res.reserve(a.length() + b.length());
    res = a;
    res += b;
    return res;
}

inline string operator+(const string& a, const char* b)
{
    string res;
    const auto sv = string_view(b);
    res.reserve(a.length() + sv.length());
    res = a;
    res += sv;
    return res;
}

inline string operator+(const string& a, char b)
{
    string res;
    res.reserve(a.length() + 1);
    res = a;
    res += b;
    return res;
}

inline string operator+(const char* a, const string& b)
{
    string res;
    const auto sv = string_view(a);
    res.reserve(sv.length() + b.length());
    res = sv;
    res += b;
    return res;
}

inline string operator+(char a, const string& b)
{
    string res;
    res.reserve(1 + b.length());
    res = a;
    res += b;
    return res;
}

inline string operator+(const string_view& a, const string_view& b)
{
    string res;
    res.reserve(a.length() + b.length());
    res = a;
    res += b;
    return res;
}

inline string operator+(const string_view& a, char b)
{
    string res;
    res.reserve(a.length() + 1);
    res = a;
    res += b;
    return res;
}

inline string operator+(const char a, string_view& b)
{
    string res;
    res.reserve(b.length() + 1);
    res = a;
    res += b;
    return res;
}

inline string operator+(string&& a, string&& b)
{
    if (a.capacity() >= b.capacity()) {
        return move(a.append(b));
    } else {
        return move(b.insert(0, a.data(), a.size()));
    }
}

inline string operator+(string&& a, const char* b)
{
    return move(a.append(b));
}

inline string operator+(string&& a, char b)
{
    return move(a.push_back(b));
}

inline string operator+(const char* a, string&& b)
{
    return move(b.insert(0, a));
}

inline string operator+(char a, string&& b)
{
    return move(b.insert(0, &a, 1));
}

template<class CharT>
inline bool operator==(const basic_string<CharT>& lhs, const basic_string<CharT>& rhs) noexcept
{
    return string_view(lhs) == string_view(rhs);
}

template<class CharT>
inline bool operator==(const basic_string<CharT>& lhs, const CharT* const rhs) noexcept
{
    return string_view(lhs) == rhs;
}

template<class CharT>
inline bool operator==(const CharT* const lhs, const basic_string<CharT>& rhs) noexcept
{
    return lhs == string_view(rhs);
}

template<class CharT>
inline bool operator!=(const basic_string<CharT>& lhs, const basic_string<CharT>& rhs) noexcept
{
    return string_view(lhs) != string_view(rhs);
}

template<class CharT>
inline bool operator!=(const basic_string<CharT>& lhs, const CharT* const rhs) noexcept
{
    return string_view(lhs) != rhs;
}

template<class CharT>
inline bool operator!=(const CharT* const lhs, const basic_string<CharT>& rhs) noexcept
{
    return lhs != string_view(rhs);
}

template<class CharT>
inline bool operator<(const basic_string<CharT>& lhs, const basic_string<CharT>& rhs) noexcept
{
    return string_view(lhs) < string_view(rhs);
}

template<class CharT>
inline bool operator<(const basic_string<CharT>& lhs, const CharT* const rhs) noexcept
{
    return string_view(lhs) < rhs;
}

template<class CharT>
inline bool operator<(const CharT* const lhs, const basic_string<CharT>& rhs) noexcept
{
    return lhs < string_view(rhs);
}

template<class CharT>
inline bool operator<=(const basic_string<CharT>& lhs, const basic_string<CharT>& rhs) noexcept
{
    return string_view(lhs) <= string_view(rhs);
}

template<class CharT>
inline bool operator<=(const basic_string<CharT>& lhs, const CharT* const rhs) noexcept
{
    return string_view(lhs) <= rhs;
}

template<class CharT>
inline bool operator<=(const CharT* const lhs, const basic_string<CharT>& rhs) noexcept
{
    return lhs <= string_view(rhs);
}

template<class CharT>
inline bool operator>(const basic_string<CharT>& lhs, const basic_string<CharT>& rhs) noexcept
{
    return string_view(lhs) > string_view(rhs);
}

template<class CharT>
inline bool operator>(const basic_string<CharT>& lhs, const CharT* const rhs) noexcept
{
    return string_view(lhs) > rhs;
}

template<class CharT>
inline bool operator>(const CharT* const lhs, const basic_string<CharT>& rhs) noexcept
{
    return lhs > string_view(rhs);
}

template<class CharT>
inline bool operator>=(const basic_string<CharT>& lhs, const basic_string<CharT>& rhs) noexcept
{
    return string_view(lhs) >= string_view(rhs);
}

template<class CharT>
inline bool operator>=(const basic_string<CharT>& lhs, const CharT* const rhs) noexcept
{
    return string_view(lhs) >= rhs;
}

template<class CharT>
inline bool operator>=(const CharT* const lhs, const basic_string<CharT>& rhs) noexcept
{
    return lhs >= string_view(rhs);
}

template<typename T>
uint64_t hash(const T&);

template<>
inline uint64_t hash(const string& value)
{
    return BASE_NS::FNV1aHash(value.data(), value.size());
}
BASE_END_NAMESPACE()

#endif // API_BASE_CONTAINERS_STRING_H
