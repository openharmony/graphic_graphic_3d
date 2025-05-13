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

#ifndef META_API_INTERFACE_OBJECT_H
#define META_API_INTERFACE_OBJECT_H

#include <base/containers/shared_ptr.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/interface_traits.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Common base class for all InterfaceObjects.
 */
class InterfaceObjectBase {
public:
    InterfaceObjectBase() = default;
    virtual ~InterfaceObjectBase() = default;
    META_DEFAULT_COPY_MOVE(InterfaceObjectBase)
    /// Returns true if InterfaceObject contains a valid shared_ptr<T>, false otherwise.
    explicit operator bool() const noexcept
    {
        return GetInterfacePtr() != nullptr;
    }
    /// Returns true if underlying pointers are the same, false otherwise.
    bool operator==(const InterfaceObjectBase& other) const noexcept
    {
        return GetInterfacePtr() == other.GetInterfacePtr();
    }
    /// Returns true if underlying pointers are different, false otherwise.
    bool operator!=(const InterfaceObjectBase& other) const noexcept
    {
        return GetInterfacePtr() != other.GetInterfacePtr();
    }
    /// @see CORE_NS::IInterface::GetInterface
    const CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid) const
    {
        const auto p = GetInterfacePtr();
        return p ? p->GetInterface(uid) : nullptr;
    }
    /// @see CORE_NS::IInterface::GetInterface
    CORE_NS::IInterface* GetInterface(const BASE_NS::Uid& uid)
    {
        const auto p = GetInterfacePtr();
        return p ? p->GetInterface(uid) : nullptr;
    }

protected:
    virtual CORE_NS::IInterface* GetInterfacePtr() const noexcept = 0;
};

namespace Internal {
/**
 * @brief Converts vector types
 * @param from The vector to convert from
 * @return Returns a vector of InterfaceObjects initialized with elements from source array.
 */
template<class To, class From>
constexpr auto ArrayCast(BASE_NS::vector<From>&& from) noexcept
{
    if constexpr (BASE_NS::is_same_v<To, From>) {
        return BASE_NS::move(from);
    } else {
        BASE_NS::vector<To> converted;
        if (!from.empty()) {
            converted.reserve(from.size());
            for (auto&& n : from) {
                converted.emplace_back(n);
            }
        }
        return converted;
    }
}

} // namespace Internal

/**
 * @brief The InterfaceObject class is the base BASE_NS::shared_ptr<T> wrapper inherited by actual interface wrapper
 * objects.
 */
template<typename T>
class InterfaceObject : public InterfaceObjectBase {
    static_assert(
        BASE_NS::is_base_of<CORE_NS::IInterface, T>(), "InterfaceObject type must be derived from CORE_NS::IInterface");

public:
    using UnderlyingType = T;
    using Ptr = BASE_NS::shared_ptr<UnderlyingType>;
    using WeakPtr = BASE_NS::weak_ptr<UnderlyingType>;
    using IInterfacePtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
    InterfaceObject() = delete;
    explicit InterfaceObject(const Ptr& p) noexcept : ptr_(p) {}
    explicit InterfaceObject(const IInterfacePtr& p) : ptr_(interface_pointer_cast<UnderlyingType>(p)) {}
    /// Assignment operator
    auto& operator=(const IInterfacePtr& p) noexcept
    {
        ptr_ = interface_pointer_cast<UnderlyingType>(p);
        return *this;
    }
    /// InterfaceObject<T> to BASE_NS::shared_ptr<T> operator
    operator Ptr() const noexcept
    {
        return ptr_;
    }
    /// InterfaceObject<T> to BASE_NS::shared_ptr<CORE_NS::IInterface> operator
    operator IInterfacePtr() const noexcept
    {
        return ptr_;
    }
    /// Return the underlying BASE_NS::shared_ptr<T>
    Ptr GetPtr() const noexcept
    {
        return ptr_;
    }
    /// Return the underlying BASE_NS::shared_ptr<T> converted to BASE_NS::shared_ptr<Interface>
    template<typename Interface>
    BASE_NS::shared_ptr<Interface> GetPtr() const
    {
        return interface_pointer_cast<Interface>(ptr_);
    }
    /// Release the underlying object.
    void Release()
    {
        ptr_.reset();
    }

protected:
    /// Helper method for safely calling a method from the underlying interface
    template<typename Interface, typename Fn>
    auto CallPtr(Fn&& fn) const
    {
        auto* p = interface_cast<Interface>(ptr_);
        using Result = BASE_NS::remove_reference_t<decltype(fn(*p))>;
        if constexpr (!BASE_NS::is_void_v<Result>) {
            return p ? fn(*p) : Result {};
        } else if (p) {
            fn(*p);
        }
    }

    CORE_NS::IInterface* GetInterfacePtr() const noexcept override
    {
        return ptr_.get();
    }
    Ptr ptr_;
};

// InterfaceObject definition
#define META_INTERFACE_OBJECT(Class, BaseClass, Interface)                                   \
    using ClassType = Class;                                                                 \
    using BaseClassType = BaseClass;                                                         \
    using InterfaceType = Interface;                                                         \
    Class() = delete;                                                                        \
    ~Class() override = default;                                                             \
    explicit Class(const IInterfacePtr& p) : BaseClass(interface_pointer_cast<Interface>(p)) \
    {}                                                                                       \
    META_DEFAULT_COPY_MOVE(Class)                                                            \
    operator InterfaceType::Ptr() const noexcept                                             \
    {                                                                                        \
        return GetPtr<InterfaceType>();                                                      \
    }                                                                                        \
    auto& operator=(const InterfaceType::Ptr& p) noexcept                                    \
    {                                                                                        \
        ptr_ = interface_pointer_cast<UnderlyingType>(p);                                    \
        return *this;                                                                        \
    }

// InterfaceObject default interface function call wrapper
#define META_INTERFACE_OBJECT_CALL_PTR(Function) CallPtr<InterfaceType>([&](auto& p) { return p.Function; })

#define META_INTERFACE_OBJECT_PROPERTY_ACCESSOR(PropertyName)  \
public:                                                        \
    auto PropertyName() const                                  \
    {                                                          \
        return META_INTERFACE_OBJECT_CALL_PTR(PropertyName()); \
    }

// InterfaceObject default readonly property wrapper
#define META_INTERFACE_OBJECT_READONLY_PROPERTY(Type, PropertyName) \
    META_INTERFACE_OBJECT_PROPERTY_ACCESSOR(PropertyName)           \
    auto Get##PropertyName() const                                  \
    {                                                               \
        return Type(META_NS::GetValue(PropertyName()));             \
    }

// InterfaceObject default property wrapper
#define META_INTERFACE_OBJECT_PROPERTY(Type, PropertyName)      \
    META_INTERFACE_OBJECT_READONLY_PROPERTY(Type, PropertyName) \
    auto& Set##PropertyName(const Type& value)                  \
    {                                                           \
        if (auto pr = PropertyName()) {                         \
            pr->SetValue(value);                                \
        }                                                       \
        return *this;                                           \
    }

// InterfaceObject default readonly array property wrapper
#define META_INTERFACE_OBJECT_READONLY_ARRAY_PROPERTY(Type, PropertyName, AccessorName)                   \
    META_INTERFACE_OBJECT_PROPERTY_ACCESSOR(PropertyName)                                                 \
    auto Get##PropertyName() const                                                                        \
    {                                                                                                     \
        auto pr = PropertyName();                                                                         \
        using ReturnType = decltype(pr->GetValue());                                                      \
        auto value = pr ? pr->GetValue() : ReturnType {};                                                 \
        return META_NS::Internal::ArrayCast<Type, typename ReturnType::value_type>(BASE_NS::move(value)); \
    }                                                                                                     \
    auto Get##AccessorName##Count() const                                                                 \
    {                                                                                                     \
        auto pr = PropertyName();                                                                         \
        return pr ? pr->GetSize() : 0;                                                                    \
    }                                                                                                     \
    auto Get##AccessorName##At(size_t index) const                                                        \
    {                                                                                                     \
        auto pr = PropertyName();                                                                         \
        return Type(pr ? pr->GetValueAt(index) : decltype(pr->GetValueAt(index)) {});                     \
    }

// InterfaceObject default array property wrapper
#define META_INTERFACE_OBJECT_ARRAY_PROPERTY(Type, PropertyName, AccessorName)      \
    META_INTERFACE_OBJECT_READONLY_ARRAY_PROPERTY(Type, PropertyName, AccessorName) \
    auto& Set##PropertyName(BASE_NS::array_view<const Type> value)                  \
    {                                                                               \
        if (auto pr = PropertyName()) {                                             \
            pr->SetValue(value);                                                    \
        }                                                                           \
        return *this;                                                               \
    }                                                                               \
    auto& Set##AccessorName##At(size_t index, const Type& value)                    \
    {                                                                               \
        if (auto pr = PropertyName()) {                                             \
            pr->SetValueAt(index, value);                                           \
        }                                                                           \
        return *this;                                                               \
    }

struct InitializationType {};

/**
 * @brief Some interface wrappers support creating an instance at wrapper object initialization.
 *        Such classes can be made to automatically initialize the underlying object to a new class
 *        instance defining Instanate
 * @code
 *        using namespace META_NS;
 *        Object o(CreateNew); // Create a new instance of the underlying object suitable for this class.
 * @endcode
 */
constexpr InitializationType CreateNew;

#define META_INTERFACE_OBJECT_INSTANTIATE(Class, ClassId)                                                 \
    Class(const META_NS::InitializationType& t) : BaseClassType(::META_NS::CreateObjectInstance(ClassId)) \
    {}

/// Some methods support specifying whether the call should be asynchronous or synchronous through AsyncCallType
/// template parameter.
struct AsyncCallType {
    bool async {};
};
/// Call should be synchronous and directly return the asynchronous call return value.
constexpr AsyncCallType Sync { false };
/// Call should be asynchronous and return a Future object as its return value.
constexpr AsyncCallType Async { true };

#define META_API_ASYNC template<const META_NS::AsyncCallType& CallType = META_NS::Sync>

#define META_INTERFACE_OBJECT_ASYNC_CALL_PTR(SyncReturnType, Function) \
    [&]() -> auto                                                      \
    {                                                                  \
        auto f = META_INTERFACE_OBJECT_CALL_PTR(Function);             \
        if constexpr (CallType.async) {                                \
            return f;                                                  \
        } else {                                                       \
            return SyncReturnType(f.GetResult());                      \
        }                                                              \
    }                                                                  \
    ()

META_END_NAMESPACE()

// NOLINTBEGIN(readability-identifier-naming) to keep std like syntax

template<typename T, typename S>
BASE_NS::shared_ptr<T> interface_pointer_cast(const META_NS::InterfaceObject<S>& io)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    return interface_pointer_cast<T>(io.GetPtr());
}

template<typename T, typename S>
T* interface_cast(const META_NS::InterfaceObject<S>& io)
{
    static_assert(META_NS::HasGetInterfaceMethod_v<T>, "T::GetInterface not defined");
    return interface_cast<T>(io.GetPtr());
}

// NOLINTEND(readability-identifier-naming) to keep std like syntax

#endif // META_API_INTERFACE_OBJECT_H
