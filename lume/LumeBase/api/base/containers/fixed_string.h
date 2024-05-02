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

#ifndef API_BASE_CONTAINERS_BASIC_FIXED_STRING_H
#define API_BASE_CONTAINERS_BASIC_FIXED_STRING_H

#include <cstddef>
#include <cstdint>

#include <base/containers/allocator.h>
#include <base/containers/string_view.h>
#include <base/containers/type_traits.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class CharT, size_t maxSize>
class basic_fixed_string;

template<size_t maxSize>
using fixed_string = BASE_NS::basic_fixed_string<char, maxSize>;

template<size_t maxSize>
using fixed_wstring = BASE_NS::basic_fixed_string<wchar_t, maxSize>;

template<class CharT, size_t maxSize>
class basic_fixed_string {
public:
    using string_view = basic_string_view<CharT>;
    using value_type = CharT;
    using size_type = typename string_view::size_type;
    static constexpr size_type npos = string_view::npos;
    basic_fixed_string() = default;
    ~basic_fixed_string() = default;

    constexpr basic_fixed_string(const basic_fixed_string& a) noexcept
    {
        initialize({ a.data_, a.len_ });
    }

    constexpr basic_fixed_string(basic_fixed_string&& a) noexcept
    {
        initialize({ a.data_, a.len_ });
    }

    template<size_t N>
    constexpr basic_fixed_string(const basic_fixed_string<CharT, N>& a) noexcept
    {
        initialize({ a.data(), a.size() });
    }

    template<size_t N>
    constexpr basic_fixed_string(const CharT (&a)[N])
    {
        initialize({ a, N - 1 });
    }

    constexpr basic_fixed_string(const CharT* const a) noexcept
    {
        initialize({ a, constexpr_strlen(a) });
    }

    constexpr basic_fixed_string(CharT a) noexcept
    {
        initialize({ &a, 1 });
    }

    constexpr basic_fixed_string(const basic_string_view<CharT>& a) noexcept
    {
        initialize(a);
    }

    constexpr basic_fixed_string(const size_t size) noexcept : len_((size < maxSize) ? size : maxSize) {}

    template<typename T, typename = void>
    struct HasData : false_type {};
    template<typename T>
    struct HasData<T, void_t<decltype(declval<T>().data())>> : true_type {};
    template<typename T, typename = void>
    struct HasSize : false_type {};
    template<typename T>
    struct HasSize<T, void_t<decltype(declval<T>().size())>> : true_type {};
    template<typename T>
    using HasDataAndSize = enable_if_t<(HasData<T>::value && HasSize<T>::value)>;
    template<class StringT, class = HasDataAndSize<StringT>>
    constexpr explicit basic_fixed_string(const StringT& s) noexcept;

    constexpr bool empty() const noexcept
    {
        return len_ == 0;
    }

    constexpr size_t size() const noexcept
    {
        return len_;
    }

    constexpr size_t length() const noexcept
    {
        return len_;
    }

    constexpr size_t capacity() const noexcept
    {
        return maxSize;
    }

    constexpr basic_fixed_string& operator=(const basic_fixed_string& a) noexcept
    {
        initialize({ a.data_, a.len_ });
        return *this;
    }

    constexpr basic_fixed_string& operator=(basic_fixed_string&& a) noexcept
    {
        initialize({ a.data_, a.len_ });
        return *this;
    }

    template<size_t N>
    constexpr basic_fixed_string& operator=(const basic_fixed_string<CharT, N>& a) noexcept
    {
        initialize({ a.data(), a.size() });
        return *this;
    }

    template<size_t N>
    constexpr basic_fixed_string& operator=(basic_fixed_string<CharT, N>&& a) noexcept
    {
        initialize({ a.data(), a.size() });
        return *this;
    }

    constexpr basic_fixed_string& operator=(const string_view& a) noexcept
    {
        initialize(a);
        return *this;
    }

    constexpr basic_fixed_string& operator=(CharT const* const& a) noexcept
    {
        initialize({ a, constexpr_strlen(a) });
        return *this;
    }

    constexpr basic_fixed_string operator+(const string_view& b) const
    {
        basic_fixed_string res(*this);
        res.append_impl(b);
        return res;
    }

    constexpr basic_fixed_string operator+(CharT const* const& a) const
    {
        basic_fixed_string res(*this);
        res.append_impl({ a, constexpr_strlen(a) });
        return res;
    }

    constexpr basic_fixed_string& operator+=(const string_view& a)
    {
        append_impl(a);
        return *this;
    }

    constexpr basic_fixed_string& operator+=(CharT const* const& a)
    {
        append_impl({ a, constexpr_strlen(a) });
        return *this;
    }

    constexpr basic_fixed_string& operator+=(CharT a)
    {
        append_impl({ &a, 1 });
        return *this;
    }

    constexpr CharT const* data() const
    {
        return data_;
    }

    constexpr CharT* data()
    {
        return data_;
    }

    constexpr CharT const* c_str() const
    {
        return data_;
    }

    constexpr operator string_view() const noexcept
    {
        return string_view(data_, len_);
    }

    constexpr size_type copy(CharT* const dst, size_type todo, size_type pos = 0) const
    {
        return string_view(*this).copy(dst, todo, pos);
    }

    /** find substring in the view */
    constexpr size_type find(const CharT str, size_type pos = 0) const noexcept
    {
        return string_view(*this).find(str, pos);
    }

    constexpr size_type find(const string_view& str, size_type pos = 0) const noexcept
    {
        return string_view(*this).find(str, pos);
    }

    /* find the last occurrence of a substring in the view */
    constexpr size_type rfind(const CharT str, size_type pos = npos) const noexcept
    {
        return string_view(*this).rfind(str, pos);
    }

    constexpr size_type rfind(const string_view& str, size_type pos = npos) const noexcept
    {
        return string_view(*this).rfind(str, pos);
    }

    /* find first occurance of characters in the view */
    constexpr size_type find_first_of(const string_view& str, size_type pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(str, pos);
    }

    constexpr size_type find_first_of(CharT ch, size_type pos = 0) const noexcept
    {
        return string_view(*this).find_first_of(ch, pos);
    }

    /* find last occurrence of characters in the view */
    constexpr size_type find_last_of(const string_view& str, size_type pos = npos) const noexcept
    {
        return string_view(*this).find_last_of(str, pos);
    }

    constexpr size_type find_last_of(CharT ch, size_type pos = npos) const noexcept
    {
        return string_view(*this).find_last_of(ch, pos);
    }

    /* find first absence of characters
    find_first_not_of

    find last absence of characters
    find_last_not_of */
    constexpr basic_fixed_string& append(const string_view& b)
    {
        append_impl(b);
        return *this;
    }

    constexpr basic_fixed_string& append(CharT const* const a)
    {
        append_impl({ a, constexpr_strlen(a) });
        return *this;
    }

    constexpr basic_fixed_string& append(CharT a)
    {
        append_impl({ &a, 1 });
        return *this;
    }

    basic_fixed_string& replace(size_t first, size_t last, const basic_string_view<CharT>& str)
    {
        const auto replace = last - first;
        const auto add = str.length();
        const auto newSize = len_ + add - replace;
        if (add < replace) {
            CloneData(data() + first, replace, str.data(), add);
            MoveData(data() + first + add, len_ - first - add, data() + last, len_ - last);
        } else if (add > replace) {
            const auto start = newSize < maxSize ? newSize : maxSize;
            for (auto i = start; i > last; --i) {
                data_[i] = data_[i - add + replace];
            }
            CloneData(data() + first, len_, str.data(), add);
        } else {
            CloneData(data() + first, replace, str.data(), add);
        }
        len_ = newSize < maxSize ? newSize : maxSize;
        data_[len_] = 0;
        return *this;
    }

protected:
    constexpr void initialize(const string_view& other)
    {
        len_ = other.length();
        len_ = (len_ < maxSize) ? len_ : maxSize;
        other.copy(data_, len_);
        data_[len_] = 0;
    }

    constexpr void append_impl(const string_view& other)
    {
        size_t todo = other.length();
        todo = ((todo + len_) > maxSize) ? (maxSize - len_) : todo;
        other.copy(data_ + len_, todo);
        len_ += todo;
        data_[len_] = 0;
    }

    size_t len_ { 0 };
    CharT data_[maxSize + 1] { 0 };
};

template<class CharT, size_t maxSize>
template<class StringT, class>
constexpr basic_fixed_string<CharT, maxSize>::basic_fixed_string(const StringT& s) noexcept
{
    initialize({ s.data(), s.length() });
}
template<class CharT, size_t N>
basic_fixed_string(const CharT (&)[N]) -> basic_fixed_string<CharT, N - 1>;

template<class CharT, size_t M, size_t N>
constexpr basic_fixed_string<CharT, M + N> operator+(
    const basic_fixed_string<CharT, M>& lhs, const basic_fixed_string<CharT, N>& rhs) noexcept
{
    basic_fixed_string<CharT, M + N> res { lhs };
    res.append(rhs);
    return res;
}

template<class CharT, size_t M, size_t N>
constexpr basic_fixed_string<CharT, M + N> operator+(
    const CharT (&lhs)[M], const basic_fixed_string<CharT, N>& rhs) noexcept
{
    basic_fixed_string<CharT, M + N> res { lhs };
    res.append(rhs);
    return res;
}

template<class CharT, size_t M, size_t N>
constexpr basic_fixed_string<CharT, M + N> operator+(
    const basic_fixed_string<CharT, M>& lhs, const CharT (&rhs)[N]) noexcept
{
    basic_fixed_string<CharT, M + N> res { lhs };
    res.append(rhs);
    return res;
}

template<class CharT, size_t N>
constexpr basic_fixed_string<CharT, 1 + N> operator+(CharT lhs, const basic_fixed_string<CharT, N>& rhs) noexcept
{
    basic_fixed_string<CharT, 1 + N> res(string_view(&lhs, 1));
    res.append(rhs);
    return res;
}

template<class CharT, size_t M, size_t N>
constexpr bool operator==(const basic_fixed_string<CharT, M>& lhs, const basic_fixed_string<CharT, N>& rhs) noexcept
{
    return string_view(lhs) == string_view(rhs);
}

template<class CharT, size_t M, int = 1>
constexpr bool operator==(
    const basic_fixed_string<CharT, M>& lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    return string_view(lhs) == rhs;
}

template<class CharT, size_t M, int = 2>
constexpr bool operator==(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_fixed_string<CharT, M>& rhs) noexcept
{
    return lhs == string_view(rhs);
}

template<class CharT, size_t M, size_t N>
constexpr bool operator!=(const basic_fixed_string<CharT, M>& lhs, const basic_fixed_string<CharT, N>& rhs) noexcept
{
    return string_view(lhs) != string_view(rhs);
}

template<class CharT, size_t M, int = 1>
constexpr bool operator!=(
    const basic_fixed_string<CharT, M>& lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    return string_view(lhs) != rhs;
}

template<class CharT, size_t M, int = 2>
constexpr bool operator!=(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_fixed_string<CharT, M>& rhs) noexcept
{
    return lhs != string_view(rhs);
}

template<class CharT, size_t M, size_t N>
constexpr bool operator<(const basic_fixed_string<CharT, M>& lhs, const basic_fixed_string<CharT, N>& rhs) noexcept
{
    return string_view(lhs) < string_view(rhs);
}

template<class CharT, size_t M, int = 1>
constexpr bool operator<(
    const basic_fixed_string<CharT, M>& lhs, const type_identity_t<basic_string_view<CharT>> rhs) noexcept
{
    return string_view(lhs) < rhs;
}

template<class CharT, size_t M, int = 2>
constexpr bool operator<(
    const type_identity_t<basic_string_view<CharT>> lhs, const basic_fixed_string<CharT, M>& rhs) noexcept
{
    return lhs < string_view(rhs);
}

template<typename Number, typename = enable_if_t<is_integral_v<Number>>>
constexpr fixed_string<21u> to_string(Number num)
{
    fixed_string<21u> str;
    uint64_t n = static_cast<uint64_t>(num);
    // negate negative values
    if constexpr (is_signed<Number>::value) {
        if (num < 0) {
            n = static_cast<uint64_t>(-num);
        }
    }

    // write starting from the end
    const auto end = str.data() + str.capacity();
    auto p = end - 1;
    do {
        *p-- = '0' + static_cast<char>(n % 10U);
        n /= 10U;
    } while (n != 0U);

    // add sign if needed
    if constexpr (is_signed<Number>::value) {
        if (num < 0) {
            *p-- = '-';
        }
    }

    ++p;
    str.append(string_view(p, static_cast<size_t>(end - p)));
    return str;
}

template<typename Number, typename = enable_if_t<is_integral_v<Number>>>
constexpr fixed_string<21u> to_hex(Number num)
{
    fixed_string<21u> str;
    uint64_t n = static_cast<uint64_t>(num);
    // negate negative values
    if constexpr (is_signed<Number>::value) {
        if (num < 0) {
            n = static_cast<uint64_t>(-num);
        }
    }

    // write starting from the end
    const auto end = str.data() + str.capacity();
    auto p = end - 1;
    const char* hex = "0123456789ABCDEF";
    do {
        *p-- = hex[(n % 16U)];
        n /= 16U;
    } while (n != 0U);

    // add sign if needed
    if constexpr (is_signed<Number>::value) {
        if (num < 0) {
            *p-- = '-';
        }
    }

    ++p;
    str.append(string_view(p, static_cast<string_view::size_type>(end - p)));
    return str;
}

template<size_t maxSize>
inline uint64_t hash(const fixed_string<maxSize>& value)
{
    return BASE_NS::FNV1aHash(value.data(), value.size());
}
BASE_END_NAMESPACE()
#endif // API_BASE_CONTAINERS_BASIC_FIXED_STRING_H