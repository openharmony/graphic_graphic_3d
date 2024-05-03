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

#ifndef API_BASE_CONTAINERS_TYPE_TRAITS_H
#define API_BASE_CONTAINERS_TYPE_TRAITS_H

#include <cstddef>

#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T, T v>
struct integral_constant {
    static constexpr T value = v;
    using value_type = T;
    using type = integral_constant;
    constexpr operator value_type() const noexcept
    {
        return (value);
    }
    constexpr value_type operator()() const noexcept
    {
        return (value);
    }
};

using false_type = integral_constant<bool, false>;
using true_type = integral_constant<bool, true>;

template<bool B, class T = void>
struct enable_if {};
template<class T>
struct enable_if<true, T> {
    using type = T;
};
template<bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;

template<class T, class U>
struct is_same : false_type {};
template<class T>
struct is_same<T, T> : true_type {};

template<class T, class U>
constexpr auto is_same_v = is_same<T, U>::value;

template<class T>
struct remove_extent {
    using type = T;
};
template<class T>
struct remove_extent<T[]> {
    using type = T;
};
template<class T, size_t N>
struct remove_extent<T[N]> {
    using type = T;
};
template<class T>
using remove_extent_t = typename remove_extent<T>::type;

template<class T, unsigned N = 0>
struct extent : integral_constant<size_t, 0> {};

template<class T>
struct extent<T[], 0> : integral_constant<size_t, 0> {};

template<class T, unsigned N>
struct extent<T[], N> : extent<T, N - 1> {};

template<class T, size_t I>
struct extent<T[I], 0> : integral_constant<size_t, I> {};

template<class T, size_t I, unsigned N>
struct extent<T[I], N> : extent<T, N - 1> {};

template<class T, unsigned N = 0>
inline constexpr size_t extent_v = extent<T, N>::value;

template<class T>
struct remove_const {
    using type = T;
};
template<class T>
struct remove_const<const T> {
    using type = T;
};
template<class T>
using remove_const_t = typename remove_const<T>::type;

template<class T>
struct is_const : false_type {};
template<class T>
struct is_const<const T> : true_type {};
template<class T>
constexpr auto is_const_v = is_const<T>::value;

template<class T>
struct is_array : false_type {};
template<class T>
struct is_array<T[]> : true_type {};
template<class T, size_t N>
struct is_array<T[N]> : true_type {};
template<class T>
constexpr bool is_array_v = is_array<T>::value;
template<class T>
using is_array_t = typename is_array<T>::type;

template<class T>
struct is_pointer : false_type {};
template<class T>
struct is_pointer<T*> : true_type {};
template<class T>
constexpr auto is_pointer_v = is_pointer<T>::value;

template<class T>
struct is_reference : false_type {};
template<class T>
struct is_reference<T&> : true_type {};
template<class T>
struct is_reference<T&&> : true_type {};
template<class T>
constexpr bool is_reference_v = is_reference<T>::value;

template<class T>
struct is_lvalue_reference : false_type {};
template<class T>
struct is_lvalue_reference<T&> : true_type {};
template<class T>
constexpr bool is_lvalue_reference_v = is_lvalue_reference<T>::value;

template<class T>
struct is_rvalue_reference : false_type {};
template<class T>
struct is_rvalue_reference<T&&> : true_type {};
template<class T>
constexpr bool is_rvalue_reference_v = is_rvalue_reference<T>::value;

template<class... _Types>
using void_t = void;

template<class T, class = void>
struct add_lvalue_reference {
    using type = T;
};
template<class T, class = void>
struct add_rvalue_reference {
    using type = T;
};
template<class T>
struct add_lvalue_reference<T, void_t<T&>> {
    using type = T&;
};
template<class T>
struct add_rvalue_reference<T, void_t<T&>> {
    using type = T&&;
};
template<class T>
using add_lvalue_reference_t = typename add_lvalue_reference<T>::type;
template<class T>
using add_rvalue_reference_t = typename add_rvalue_reference<T>::type;

template<class T>
struct remove_pointer {
    using type = T;
};
template<class T>
struct remove_pointer<T*> {
    using type = T;
};
template<class T>
struct remove_pointer<T* const> {
    using type = T;
};
template<class T>
using remove_pointer_t = typename remove_pointer<T>::type;

template<class T>
add_rvalue_reference_t<T> declval() noexcept;

template<class T>
struct remove_reference {
    using type = T;
};
template<class T>
struct remove_reference<T&> {
    using type = T;
};
template<class T>
struct remove_reference<T&&> {
    using type = T;
};
template<class T>
using remove_reference_t = typename remove_reference<T>::type;
template<class T>
constexpr remove_reference_t<T>&& move(T&& _Arg) noexcept
{
    static_assert(!is_const<typename remove_reference<T>::type>::value, "move of const object is invalid.");
    return (static_cast<remove_reference_t<T>&&>(_Arg));
}
template<class T>
constexpr T&& forward(remove_reference_t<T>& _Arg) noexcept
{
    return (static_cast<T&&>(_Arg));
}
template<class T>
constexpr T&& forward(remove_reference_t<T>&& _Arg) noexcept
{
    static_assert(!is_lvalue_reference_v<T>, "bad forward call");
    return (static_cast<T&&>(_Arg));
}

using nullptr_t = decltype(nullptr);
template<class T, class U = T>
constexpr T exchange(T& obj, U&& new_value)
{
    T old_value = move(obj);
    obj = forward<U>(new_value);
    return old_value;
}

template<typename T>
struct type_identity {
    using type = T;
};

template<class T>
using type_identity_t = typename type_identity<T>::type;

template<bool Condition, class TrueType, class FalseType>
struct conditional {
    using type = TrueType;
};
template<class TrueType, class FalseType>
struct conditional<false, TrueType, FalseType> {
    using type = FalseType;
};
template<bool Condition, class TrueType, class FalseType>
using conditional_t = typename conditional<Condition, TrueType, FalseType>::type;

template<class T>
struct is_void : is_same<void, remove_const_t<T>> {};
template<typename T>
inline constexpr bool is_void_v = is_void<T>::value;

template<typename T>
struct is_floating_point
    : integral_constant<bool, is_same_v<float, remove_const_t<T>> || is_same_v<double, remove_const_t<T>> ||
                                  is_same_v<long double, remove_const_t<T>>> {};
template<typename T>
inline constexpr bool is_floating_point_v = is_floating_point<T>::value;

namespace detail {
template<typename T>
struct is_integral : false_type {};

template<>
struct is_integral<bool> : true_type {};

template<>
struct is_integral<signed char> : true_type {};

template<>
struct is_integral<short> : true_type {};

template<>
struct is_integral<int> : true_type {};

template<>
struct is_integral<long> : true_type {};

template<>
struct is_integral<long long> : true_type {};

template<>
struct is_integral<unsigned char> : true_type {};

template<>
struct is_integral<unsigned short> : true_type {};

template<>
struct is_integral<unsigned int> : true_type {};

template<>
struct is_integral<unsigned long> : true_type {};

template<>
struct is_integral<unsigned long long> : true_type {};
} // namespace detail

template<typename T>
struct is_integral : detail::is_integral<remove_const_t<T>>::type {};

template<class T>
inline constexpr bool is_integral_v = is_integral<T>::value;

template<class T>
struct is_arithmetic : integral_constant<bool, is_integral_v<T> || is_floating_point_v<T>> {};

template<class T>
inline constexpr bool is_arithmetic_v = is_arithmetic<T>::value;

template<typename T, bool = is_arithmetic_v<T>>
struct is_unsigned : integral_constant<bool, T(0) < T(-1)> {};

template<typename T>
struct is_unsigned<T, false> : false_type {};

template<typename T>
inline constexpr bool is_unsigned_v = is_unsigned<T>::value;

template<typename T, bool = is_arithmetic_v<T>>
struct is_signed : integral_constant<bool, T(-1) < T(0)> {};

template<typename T>
struct is_signed<T, false> : false_type {};

template<typename T>
inline constexpr bool is_signed_v = is_signed<T>::value;

namespace detail {
template<typename T>
auto is_returnable(int) -> decltype(void(static_cast<T (*)()>(nullptr)), true_type {});

template<typename>
auto is_returnable(...) -> false_type;

template<typename T>
inline constexpr bool is_returnable_v = decltype(is_returnable<T>(0))::value;

template<typename From, typename To>
auto is_implicitly_convertible(int) -> decltype(void(declval<void (&)(To)>()(declval<From>())), true_type {});

template<typename, typename>
auto is_implicitly_convertible(...) -> false_type;

template<typename From, typename To>
inline constexpr bool is_implicitly_convertible_v = decltype(is_implicitly_convertible<From, To>(0))::value;
} // namespace detail

template<typename From, typename To>
struct is_convertible
    : integral_constant<bool, (is_void_v<From> && is_void_v<To>) ||
                                  (detail::is_returnable_v<To> && detail::is_implicitly_convertible_v<From, To>)> {};
template<typename From, typename To>
constexpr bool is_convertible_v = is_convertible<From, To>::value;

// return underlying type, works atleast for enums.
template<typename _Ty>
struct underlying_type {
    using type = __underlying_type(_Ty);
};
template<typename _Ty>
using underlying_type_t = typename underlying_type<_Ty>::type;

template<typename T, size_t N>
constexpr size_t countof(T (&)[N]) noexcept
{
    return N;
}
BASE_END_NAMESPACE()

#endif // API_BASE_TYPE_TRAITS_H
