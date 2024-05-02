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

#ifndef META_BASE_INTERFACE_TRAITS_H
#define META_BASE_INTERFACE_TRAITS_H

#include <base/containers/type_traits.h>
#include <base/util/uid.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/type_traits.h>

BASE_BEGIN_NAMESPACE()
namespace Internals {
class ptr_base;
}
template<typename>
class shared_ptr;

template<typename>
class weak_ptr;
BASE_END_NAMESPACE()

META_BEGIN_NAMESPACE()

template<class C>
class HasGetInterfaceMethod final {
    template<class T>
    static BASE_NS::true_type TestSignature(CORE_NS::IInterface* (T::*)(const BASE_NS::Uid&));

    template<class T>
    static decltype(TestSignature((CORE_NS::IInterface * (T::*)(const BASE_NS::Uid&)) & T::GetInterface)) Test(
        std::nullptr_t);

    template<class T>
    static BASE_NS::false_type Test(...);

public:
    static constexpr bool value = decltype(Test<C>(nullptr))::value; // NOLINT(readability-identifier-naming)
};

// NOLINTBEGIN(readability-identifier-naming)
/**
 * @brief Check if type has GetInterface member functions (see IInterface).
 */
template<class C>
inline constexpr bool HasGetInterfaceMethod_v = HasGetInterfaceMethod<C>::value;

/**
 * @brief Check if type is an interface, i.e. implements reference counting and GetInterface member functions.
 */
template<class C>
inline constexpr bool IsKindOfInterface_v =
    BASE_NS::is_convertible_v<BASE_NS::remove_const_t<C>*, CORE_NS::IInterface*>;
// NOLINTEND(readability-identifier-naming)

// type trait for checking if the type has equality comparison
template<class T>
struct HasEqualOperator {
    template<class U, class V>
    static auto Test(U*) -> decltype(BASE_NS::declval<U>() == BASE_NS::declval<V>());
    template<typename, typename>
    static auto Test(...) -> BASE_NS::false_type;

    using type = typename BASE_NS::is_same<bool, decltype(Test<T, T>(nullptr))>::type;
};

/**
 * @brief Check if the type can be compared with equality operators.
 */
template<class C>
inline constexpr bool HasEqualOperator_v = HasEqualOperator<C>::type::value; // NOLINT(readability-identifier-naming)

template<class T>
struct HasInEqualOperator {
    template<class U, class V>
    static auto Test(U*) -> decltype(BASE_NS::declval<U>() != BASE_NS::declval<V>());
    template<typename, typename>
    static auto Test(...) -> BASE_NS::false_type;

    using type = typename BASE_NS::is_same<bool, decltype(Test<T, T>(nullptr))>::type;
};

// NOLINTBEGIN(readability-identifier-naming)
/**
 * @brief Check if the type can be compared with in-equality operators.
 */
template<class C>
inline constexpr bool HasInEqualOperator_v = HasInEqualOperator<C>::type::value;
// NOLINTEND(readability-identifier-naming)

// NOLINTBEGIN(readability-identifier-naming)
/**
 * @brief Check if the type is "kind of" smart pointer, i.e. derived from BASE_NS::ptr_base.
 */
template<typename type>
constexpr bool IsKindOfPointer_v =
    BASE_NS::is_convertible_v<BASE_NS::remove_const_t<type&>, BASE_NS::Internals::ptr_base&>;
// NOLINTEND(readability-identifier-naming)

// NOLINTBEGIN(readability-identifier-naming)
/**
 * @brief Check if the type is a pointer which is convertible to IInterface pointer.
 */
template<typename type>
constexpr bool IsKindOfIInterface_v = BASE_NS::is_convertible_v<BASE_NS::remove_const_t<type>, CORE_NS::IInterface*>;
// NOLINTEND(readability-identifier-naming)

/**
 * @brief SFINAE construct for checking if entity can be used as property bind function.
 */
template<typename Func, typename Type>
using EnableIfProperBindFunction = decltype(BASE_NS::declval<void(const Type&)>()(BASE_NS::declval<Func>()()));

template<typename T>
T CallTestFunc(const T&);

template<typename Func>
using EnableIfBindFunction = decltype(CallTestFunc(BASE_NS::declval<Func>()()));

template<typename Func, typename InvokeType, typename = void>
struct CanInvokeWithArgumentsImpl {
    constexpr static bool value = false; // NOLINT(readability-identifier-naming)
};

template<typename Func, typename Ret, typename... Args>
struct CanInvokeWithArgumentsImpl<Func, Ret(Args...),
    decltype(BASE_NS::declval<Func>()(BASE_NS::declval<Args>()...), void())> {
    constexpr static bool value = true; // NOLINT(readability-identifier-naming)
};

// NOLINTBEGIN(readability-identifier-naming)
/**
 * @brief Check if callable entity is compatible with given function signature.
 */
template<typename Func, typename InvokeType>
constexpr bool CanInvokeWithArguments_v = CanInvokeWithArgumentsImpl<Func, InvokeType>::value;
// NOLINTEND(readability-identifier-naming)

template<typename Func, typename InvokeType>
using EnableIfCanInvokeWithArguments = typename BASE_NS::enable_if_t<CanInvokeWithArguments_v<Func, InvokeType>>;

template<typename Type>
using ToggleConst = BASE_NS::conditional_t<BASE_NS::is_const_v<Type>, BASE_NS::remove_const_t<Type>, const Type>;

template<typename Type>
struct ToggleConstSharedPtrImpl;

template<typename Type>
struct ToggleConstSharedPtrImpl<BASE_NS::shared_ptr<Type>> {
    using type = BASE_NS::shared_ptr<const Type>;
};

template<typename Type>
struct ToggleConstSharedPtrImpl<BASE_NS::shared_ptr<const Type>> {
    using type = BASE_NS::shared_ptr<Type>;
};

template<typename Type>
using ToggleConstSharedPtr = typename ToggleConstSharedPtrImpl<Type>::type;

/*
  We want to check if one can cast shared_ptr<BaseProp> to PropPtr. This requires in our case baseclass relation.
  * (1) BaseProp and PropPtr are both const or non-const
     - This case is easy, just check directly the conversion.
  * (2) BaseProp is const and PropPtr is non-const
     - This is not allowed case since we don't cast away constness
  * (3) BaseProp is non-const and PropPtr is const
     - This case is just adding a const

  Since PropPtr is the derived class, we need to see if we can convert that to the base class:
    is_convertible<PropPtr, shared_ptr<BaseProp>> works for (1) and (2) but not for (3)
  We want it to work for (1) and (3) but not (2), so we can achieve this by swapping the constness of the types.
*/

// NOLINTBEGIN(readability-identifier-naming)
template<typename BaseProp, typename PropPtr>
inline constexpr bool IsCompatibleBaseProperty_v =
    BASE_NS::is_convertible_v<ToggleConstSharedPtr<PropPtr>, BASE_NS::shared_ptr<ToggleConst<BaseProp>>>;
// NOLINTEND(readability-identifier-naming)

template<typename BaseProp, typename PropPtr>
using EnableIfCompatibleBaseProperty = BASE_NS::enable_if_t<IsCompatibleBaseProperty_v<BaseProp, PropPtr>>;

/**
 * @brief Check if type is shared_ptr
 */
template<typename>
struct IsSharedPtr {
    static constexpr bool value = false; // NOLINT(readability-identifier-naming)
};

template<typename T>
struct IsSharedPtr<BASE_NS::shared_ptr<T>> {
    static constexpr bool value = true; // NOLINT(readability-identifier-naming)
    template<typename Type>
    using rebind = BASE_NS::shared_ptr<Type>;
};

template<typename>
struct IsWeakPtr {
    static constexpr bool value = false; // NOLINT(readability-identifier-naming)
};

template<typename T>
struct IsWeakPtr<BASE_NS::weak_ptr<T>> {
    static constexpr bool value = true; // NOLINT(readability-identifier-naming)
    template<typename Type>
    using rebind = BASE_NS::weak_ptr<Type>;
};

template<typename Type>
constexpr bool IsWeakPtr_v = IsWeakPtr<Type>::value; // NOLINT(readability-identifier-naming)

/**
 * @brief Check if type is shared_ptr or weak_ptr
 */
template<typename>
struct IsSharedOrWeakPtr {
    static constexpr bool value = false; // NOLINT(readability-identifier-naming)
};

template<typename T>
struct IsSharedOrWeakPtr<BASE_NS::shared_ptr<T>> {
    static constexpr bool value = true; // NOLINT(readability-identifier-naming)
    template<typename Type>
    using rebind = BASE_NS::shared_ptr<Type>;
    static constexpr bool is_const = BASE_NS::is_const_v<T>; // NOLINT(readability-identifier-naming)
};

template<typename T>
struct IsSharedOrWeakPtr<BASE_NS::weak_ptr<T>> {
    static constexpr bool value = true; // NOLINT(readability-identifier-naming)
    template<typename Type>
    using rebind = BASE_NS::weak_ptr<Type>;
    static constexpr bool is_const = BASE_NS::is_const_v<T>; // NOLINT(readability-identifier-naming)
};

template<typename Type>
constexpr bool IsConstPtr_v = IsSharedOrWeakPtr<Type>::is_const; // NOLINT(readability-identifier-naming)

/**
 * @brief Check if type is shared_ptr or weak_ptr
 */
template<typename T>
constexpr bool IsSharedOrWeakPtr_v = IsSharedOrWeakPtr<T>::value; // NOLINT(readability-identifier-naming)

template<typename Type>
using InterfaceCheck = META_NS::BoolWrap<IsKindOfIInterface_v<BASE_NS::remove_const_t<typename Type::element_type>*>>;

// NOLINTBEGIN(readability-identifier-naming)
template<typename Type>
constexpr bool IsInterfacePtr_v = IsSharedOrWeakPtr_v<Type> && META_NS::IsDetectedWithValue_v<InterfaceCheck, Type>;
// NOLINTEND(readability-identifier-naming)

// NOLINTBEGIN(readability-identifier-naming)
template<typename Type>
constexpr bool IsInterfaceWeakPtr_v = IsInterfacePtr_v<Type> && IsWeakPtr<Type>::value;
// NOLINTEND(readability-identifier-naming)

template<typename M>
struct FuncToSignature;

template<typename Ret, typename Class, typename... Args>
struct FuncToSignature<Ret (Class::*)(Args...)> {
    using type = Ret(Args...);
    constexpr static bool IS_CONST = false;
};

template<typename Ret, typename Class, typename... Args>
struct FuncToSignature<Ret (Class::*)(Args...) const> {
    using type = Ret(Args...);
    constexpr static bool IS_CONST = true;
};

template<typename Ret, typename... Args>
struct FuncToSignature<Ret(Args...)> {
    using type = Ret(Args...);
    constexpr static bool IS_CONST = false;
};

template<typename Ret, typename... Args>
struct FuncToSignature<Ret(Args...) const> {
    using type = Ret(Args...);
    constexpr static bool IS_CONST = true;
};

template<typename M>
struct FuncToSignature : FuncToSignature<decltype(&M::operator())> {};

/**
 * @brief Convert member function types, function types and callable entity types to function signature.
 * Note: Does not work with multiple overloads of operator().
 */
template<typename M>
using FuncToSignature_t = typename FuncToSignature<M>::type;

META_END_NAMESPACE()

#endif
