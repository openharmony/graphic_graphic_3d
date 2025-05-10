/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Typed Property interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-11-13
 */

#ifndef META_INTERFACE_DETAIL_ARRAY_PROPERTY_H
#define META_INTERFACE_DETAIL_ARRAY_PROPERTY_H

#include <meta/interface/detail/property.h>

META_BEGIN_NAMESPACE()

template<typename Base>
class ConstTypelessArrayPropertyInterfaceImpl : public Base {
public:
    using IndexType = IArrayAny::IndexType;

    template<typename Prop>
    explicit ConstTypelessArrayPropertyInterfaceImpl(Prop* p) : Base(p)
    {}

    /// Get array size
    IndexType GetSize() const
    {
        if (auto arr = interface_cast<IArrayAny>(&this->GetValueAny())) {
            return arr->GetSize();
        }
        return {};
    }
    /**
     * @brief Get value at given location to an any
     * @param index Location in the array to get
     * @param any Any where the value is copied to.
     * @notice The any needs to be compatible with the array item type.
     * @return Result of the operation
     */
    AnyReturnValue GetAnyAt(IndexType index, IAny& any) const
    {
        if (auto arr = interface_cast<IArrayAny>(&this->GetValueAny())) {
            return arr->GetAnyAt(index, any);
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    /**
     * @brief Construct any and get value for given location
     * @param index Location in the array to get
     * @return Pointer to any with the value at location 'index' or nullptr in case of failure.
     */
    IAny::Ptr GetAnyAt(IndexType index) const
    {
        IAny::Ptr res = ConstructItemAny();
        if (res) {
            if (!GetAnyAt(index, *res)) {
                res.reset();
            }
        }
        return res;
    }
    /// Construct any with the array item type
    IAny::Ptr ConstructItemAny() const
    {
        IAny::Ptr res;
        if (auto i = interface_cast<IPropertyInternalAny>(this->p_)) {
            if (auto internal = i->GetInternalAny()) {
                res = internal->Clone({ CloneValueType::DEFAULT_VALUE, TypeIdRole::ITEM });
            }
        }
        return res;
    }
};

using ConstTypelessArrayPropertyInterface = ConstTypelessArrayPropertyInterfaceImpl<ConstTypelessPropertyInterface>;

class TypelessArrayPropertyInterface : public ConstTypelessArrayPropertyInterfaceImpl<TypelessPropertyInterface> {
public:
    using PropertyType = IProperty*;
    using IndexType = IArrayAny::IndexType;

    TypelessArrayPropertyInterface(PropertyType p)
        : ConstTypelessArrayPropertyInterfaceImpl<TypelessPropertyInterface>(p)
    {}

    /**
     * @brief Set value for given location from an any
     * @param index Location in the array to set
     * @param any Any to copy value from.
     * @return Result of the operation
     */
    AnyReturnValue SetAnyAt(IndexType index, const IAny& any)
    {
        if (auto c = this->GetValueAny().Clone(true)) {
            if (auto arr = interface_cast<IArrayAny>(c)) {
                auto res = arr->SetAnyAt(index, any);
                return res ? this->SetValueAny(*arr) : res;
            }
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    /**
     * @brief Add value to end of array from an any
     * @param any Any to copy value from.
     * @return Result of the operation
     */
    AnyReturnValue AddAny(const IAny& any)
    {
        return InsertAnyAt(-1, any);
    }
    /**
     * @brief Insert value to given location from an any
     * @param index Location in the array to insert.
     * @param any Any to copy value from.
     * @return Result of the operation
     */
    AnyReturnValue InsertAnyAt(IndexType index, const IAny& v)
    {
        if (auto c = this->GetValueAny().Clone(true)) {
            if (auto arr = interface_cast<IArrayAny>(c)) {
                auto res = arr->InsertAnyAt(index, v);
                return res ? this->SetValueAny(*arr) : res;
            }
        }
        return AnyReturn::INCOMPATIBLE_TYPE;
    }
    /**
     * @brief Remove value at give location
     * @param index Location in the array to remove from.
     * @return true on success
     */
    bool RemoveAt(IndexType index)
    {
        if (auto c = this->GetValueAny().Clone(true)) {
            if (auto arr = interface_cast<IArrayAny>(c)) {
                return arr->RemoveAt(index) && this->SetValueAny(*arr);
            }
        }
        return false;
    }
};

template<typename Type>
using ArrayPropertyBaseType = BASE_NS::conditional_t<BASE_NS::is_const_v<Type>, ConstTypelessArrayPropertyInterface,
    TypelessArrayPropertyInterface>;

template<typename Type>
class ArrayPropertyInterface : public ArrayPropertyBaseType<Type> {
    using Super = ArrayPropertyBaseType<Type>;

public:
    using ValueType = BASE_NS::remove_const_t<Type>;
    using PropertyType = typename PropertyBaseType<Type>::PropertyType;
    using IndexType = IArrayAny::IndexType;

    explicit ArrayPropertyInterface(PropertyType p) : Super(p) {}

    /**
     * @brief Get value at given location
     * @param index Location from which to get value
     * @return Value at given location or value-initialized value in case failure
     */
    ValueType GetValueAt(IndexType index) const
    {
        Any<ValueType> any;
        if (auto arr = interface_cast<IArrayAny>(&this->GetValueAny())) {
            arr->GetAnyAt(index, any);
        }
        return any.InternalGetValue();
    }
    /**
     * @brief Set value at given location
     * @param index Location to set value
     * @param v Value to set
     * @return True on success
     */
    bool SetValueAt(IndexType index, const Type& v)
    {
        return this->SetAnyAt(index, Any<ValueType> { v });
    }
    /**
     * @brief Ad value at the end of array
     * @param v Value to add
     * @return True on success
     */
    bool AddValue(const Type& v)
    {
        return InsertValueAt(-1, v);
    }
    /**
     * @brief Insert value at given location
     * @param index Location to insert value
     * @param v Value to insert
     * @return True on success
     */
    bool InsertValueAt(IndexType index, const Type& v)
    {
        return this->InsertAnyAt(index, Any<ValueType> { v });
    }
    /// Get default value of the array property
    BASE_NS::vector<ValueType> GetDefaultValue() const
    {
        BASE_NS::vector<ValueType> v;
        this->GetDefaultValueAny().GetValue(v);
        return v;
    }
    /// Set default value of the array property
    AnyReturnValue SetDefaultValue(BASE_NS::array_view<const ValueType> value)
    {
        return this->SetDefaultValueAny(ArrayAny<ValueType>(value));
    }

    /// Set default value of the array property
    template<typename T, typename = BASE_NS::enable_if_t<BASE_NS::is_same_v<T, ValueType>>>
    AnyReturnValue SetDefaultValue(BASE_NS::vector<T> value)
    {
        return this->SetDefaultValueAny(ArrayAny<ValueType>(BASE_NS::move(value)));
    }

    /// Set default value of the array property and optionally reset current value to it
    AnyReturnValue SetDefaultValue(BASE_NS::vector<ValueType> value, bool resetToDefault)
    {
        auto ret = this->SetDefaultValueAny(ArrayAny<ValueType>(BASE_NS::move(value)));
        if (resetToDefault && ret) {
            this->ResetValue();
        }
        return ret;
    }
    /// Get value of the array property
    BASE_NS::vector<ValueType> GetValue() const
    {
        BASE_NS::vector<ValueType> v {};
        this->GetValueAny().GetValue(v);
        return v;
    }
    /// Set value of the array property
    AnyReturnValue SetValue(BASE_NS::array_view<const ValueType> value)
    {
        return this->SetValueAny(ArrayAny<ValueType>(value));
    }
    /// Set value of the array property
    template<typename T, typename = BASE_NS::enable_if_t<BASE_NS::is_same_v<T, ValueType>>>
    AnyReturnValue SetValue(BASE_NS::vector<T> value)
    {
        return this->SetValueAny(ArrayAny<ValueType>(BASE_NS::move(value)));
    }
    /// Find index of first matching value in the array property or -1 if no such value
    IndexType FindFirstValueOf(const Type& v) const
    {
        for (IndexType i = 0; i != this->GetSize(); ++i) {
            if (GetValueAt(i) == v) {
                return i;
            }
        }
        return -1;
    }
};

template<typename Type>
class TypedArrayPropertyLock final : public ArrayPropertyInterface<Type> {
    using PropertyType = typename ArrayPropertyInterface<Type>::PropertyType;
    using IT = ArrayPropertyInterface<Type>;
    using InterfaceType = BASE_NS::conditional_t<BASE_NS::is_const_v<Type>, const IT*, IT*>;

    META_NO_COPY_MOVE(TypedArrayPropertyLock)

    bool CanConstructFrom(PropertyType p) const
    {
        return p && p->IsCompatible(META_NS::GetArrayTypeId<BASE_NS::remove_const_t<Type>>());
    }

public:
    TypedArrayPropertyLock(NoCheckT, PropertyType p) : ArrayPropertyInterface<Type>(p)
    {
        if (auto i = interface_cast<ILockable>(this->GetProperty())) {
            i->Lock();
        }
    }
    explicit TypedArrayPropertyLock(PropertyType p) : TypedArrayPropertyLock(NOCHECK, CanConstructFrom(p) ? p : nullptr)
    {}
    explicit TypedArrayPropertyLock(const IProperty::Ptr& p) : TypedArrayPropertyLock(p.get()) {}
    explicit TypedArrayPropertyLock(const IProperty::ConstPtr& p) : TypedArrayPropertyLock(p.get()) {}
    ~TypedArrayPropertyLock()
    {
        if (auto i = interface_cast<ILockable>(this->GetProperty())) {
            i->Unlock();
        }
    }

    InterfaceType operator->() const
    {
        return const_cast<TypedArrayPropertyLock*>(this);
    }

    bool IsValid() const
    {
        return this->GetProperty() != nullptr;
    }

    explicit operator bool() const
    {
        return IsValid();
    }
};

template<typename Property>
class ArrayPropertyLock final : public ArrayPropertyBaseType<Property> {
    using InterfaceType = ArrayPropertyBaseType<Property>*;

    META_NO_COPY_MOVE(ArrayPropertyLock)

    bool CanConstructFrom(Property* p) const
    {
        auto i = interface_cast<IPropertyInternalAny>(p);
        return i && IsArray(i->GetInternalAny());
    }

public:
    ArrayPropertyLock(NoCheckT, Property* p) : ArrayPropertyBaseType<Property>(p)
    {
        if (auto i = interface_cast<ILockable>(this->GetProperty())) {
            i->Lock();
        }
    }

    explicit ArrayPropertyLock(Property* p) : ArrayPropertyLock(NOCHECK, CanConstructFrom(p) ? p : nullptr) {}

    explicit ArrayPropertyLock(BASE_NS::shared_ptr<Property> p) : ArrayPropertyLock(p.get()) {}
    ~ArrayPropertyLock()
    {
        if (auto i = interface_cast<ILockable>(this->GetProperty())) {
            i->Unlock();
        }
    }

    InterfaceType operator->() const
    {
        return const_cast<ArrayPropertyLock*>(this);
    }

    bool IsValid() const
    {
        return this->GetProperty() != nullptr;
    }

    explicit operator bool() const
    {
        return IsValid();
    }
};

META_END_NAMESPACE()

#endif
