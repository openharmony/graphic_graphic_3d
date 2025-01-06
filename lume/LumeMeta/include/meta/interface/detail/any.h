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

#ifndef META_INTERFACE_DETAIL_ANY_H
#define META_INTERFACE_DETAIL_ANY_H

#include <meta/interface/detail/any_pointer_compatibility.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_value.h>

META_BEGIN_NAMESPACE()

constexpr char BUILTIN_ANY_TAG[] = "BuiltAny";
constexpr char BUILTIN_ARRAY_ANY_TAG[] = "ArrayAny";

template<typename Type>
class BaseTypedAny : public IntroduceInterfaces<IAny, IValue> {
public:
    static constexpr TypeId TYPE_ID = UidFromType<Type>();
    static constexpr bool IS_PTR_TYPE = IsInterfacePtr_v<Type>;

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(BUILTIN_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<Type>::name;
    }

    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection dir)
    {
        if constexpr (IS_PTR_TYPE) {
            return AnyPC<Type>::template GetCompatibleTypes<Type>(dir);
        }

        static constexpr TypeId uids[] = { TYPE_ID };
        return uids;
    }
    ObjectId GetClassId() const override
    {
        return StaticGetClassId();
    }
    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir) const override
    {
        return StaticGetCompatibleTypes(dir);
    }
    AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const override
    {
        if (IsValidGetArgs(id, data, size)) {
            if constexpr (IS_PTR_TYPE) {
                using PCType = AnyPC<Type>;
                using IIType = typename PCType::IIType;
                auto ret = PCType::GetData(id, data, interface_pointer_cast<IIType>(InternalGetValue()));
                if (ret) {
                    return ret;
                }
            }

            *static_cast<Type*>(data) = InternalGetValue();
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) override
    {
        if (IsValidSetArgs(id, data, size)) {
            if constexpr (IS_PTR_TYPE) {
                using PCType = AnyPC<Type>;
                typename PCType::IIPtrType p;
                if (PCType::SetData(id, data, p)) {
                    if (auto ptr = interface_pointer_cast<typename Type::element_type>(p); ptr || !p) {
                        return InternalSetValue(ptr);
                    }
                }
            }
            return InternalSetValue(*static_cast<const Type*>(data));
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue CopyFrom(const IAny& any) override
    {
        if constexpr (IS_PTR_TYPE) {
            typename AnyPC<Type>::IIPtrType p;
            if (any.GetValue(p)) {
                if (auto ptr = interface_pointer_cast<typename Type::element_type>(p); ptr || !p) {
                    return InternalSetValue(ptr);
                }
            }
        } else {
            if (META_NS::IsCompatible(any, TYPE_ID, CompatibilityDirection::GET)) {
                Type value;
                if (any.GetValue(value)) {
                    return InternalSetValue(value);
                }
            }
        }
        return AnyReturn::FAIL;
    }

    TypeId GetTypeId(TypeIdRole role) const override
    {
        if (role == TypeIdRole::ARRAY) {
            return ArrayUidFromType<Type>();
        }
        if (role == TypeIdRole::ITEM) {
            return ItemUidFromType<Type>();
        }
        return TYPE_ID;
    }
    TypeId GetTypeId() const
    {
        return TYPE_ID;
    }
    BASE_NS::string GetTypeIdString() const override
    {
        return BASE_NS::string(StaticGetTypeIdString());
    }
    using IAny::SetValue;
    AnyReturnValue SetValue(const IAny& value) override
    {
        return CopyFrom(value);
    }
    using IAny::GetValue;
    const IAny& GetValue() const override
    {
        return *this;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return META_NS::IsCompatible(*this, id);
    }

protected:
    virtual AnyReturnValue InternalSetValue(const Type& value) = 0;
    virtual const Type& InternalGetValue() const = 0;

private:
    static constexpr bool IsValidGetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if constexpr (IS_PTR_TYPE) {
            if (AnyPC<Type>::IsValidGetArgs(uid, data, size)) {
                return true;
            }
        }
        return data && sizeof(Type) == size && uid == TYPE_ID; /*NOLINT(bugprone-sizeof-expression)*/
    }
    static constexpr bool IsValidSetArgs(const TypeId& uid, const void* data, size_t size)
    {
        if constexpr (IS_PTR_TYPE) {
            if (AnyPC<Type>::IsValidSetArgs(uid, data, size)) {
                return true;
            }
        }
        return data && sizeof(Type) == size && uid == TYPE_ID; /*NOLINT(bugprone-sizeof-expression)*/
    }
};

template<typename T, bool Compare = HasEqualOperator_v<T>>
struct DefaultCompare {
    static constexpr bool Equal(const T& v1, const T& v2)
    {
        if constexpr (Compare) {
            return v1 == v2;
        } else {
            return false;
        }
    }
    template<typename NewType>
    using Rebind = DefaultCompare<NewType>;
};

/**
 * @brief Default IAny implementation which supports a single type.
 */
template<typename Type, typename Compare = DefaultCompare<Type>>
class Any : public BaseTypedAny<Type> {
    using Super = BaseTypedAny<Type>;

public:
    explicit Any(Type v = {}) : value_(BASE_NS::move(v)) {}
    AnyReturnValue InternalSetValue(const Type& value) override
    {
        if (!Compare::Equal(value, value_)) {
            value_ = value;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }
    const Type& InternalGetValue() const override
    {
        return value_;
    }
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;
    IAny::Ptr Clone(bool withValue) const
    {
        return Clone({ withValue ? CloneValueType::COPY_VALUE : CloneValueType::DEFAULT_VALUE });
    }

    bool operator==(const Any<Type>& other) const noexcept
    {
        return (Compare::Equal(other.InternalGetValue(), value_));
    }

    AnyReturnValue ResetValue() override
    {
        return InternalSetValue({});
    }

private:
    Type value_;
};

template<typename Type>
class BaseTypedArrayAny : public IntroduceInterfaces<IArrayAny, IValue> {
public:
    static constexpr TypeId VECTOR_TYPE_ID = UidFromType<BASE_NS::vector<Type>>();
    static constexpr TypeId ARRAY_TYPE_ID = ArrayUidFromType<Type>();
    static constexpr TypeId ITEM_TYPE_ID = ItemUidFromType<Type>();
    static constexpr TypeId TYPE_ID = ARRAY_TYPE_ID;
    using ItemType = Type;
    using ArrayType = BASE_NS::vector<Type>;

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(BUILTIN_ARRAY_ANY_TAG);
    }

    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<ArrayType>::name;
    }
    ObjectId GetClassId() const override
    {
        return StaticGetClassId();
    }

    const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection dir) const override
    {
        static constexpr TypeId uids[] = { ARRAY_TYPE_ID, VECTOR_TYPE_ID };
        return uids;
    }
    const BASE_NS::array_view<const TypeId> GetItemCompatibleTypes(CompatibilityDirection dir) const override
    {
        return BaseTypedAny<Type>::StaticGetCompatibleTypes(dir);
    }
    AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const override
    {
        if (IsValidVectorArgs(id, data, size)) {
            *static_cast<ArrayType*>(data) = InternalGetValue();
            return AnyReturn::SUCCESS;
        }
        if (IsValidArrayArgs(id, data)) {
            auto& value = InternalGetValue();
            const auto valueSize = value.size() * sizeof(Type); /*NOLINT(bugprone-sizeof-expression)*/
            if (size >= valueSize) {
                BASE_NS::CloneData(data, size, value.data(), valueSize);
                return AnyReturn::SUCCESS;
            }
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) override
    {
        if (IsValidVectorArgs(id, data, size)) {
            return InternalSetValue(*static_cast<const ArrayType*>(data));
        }
        if (IsValidArrayArgs(id, data)) {
            auto p = static_cast<const Type*>(data);
            return InternalSetValue(
                BASE_NS::vector<Type>(p, p + size / sizeof(Type))); /*NOLINT(bugprone-sizeof-expression)*/
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue CopyFrom(const IAny& any) override
    {
        if (META_NS::IsCompatible(any, TYPE_ID, CompatibilityDirection::GET)) {
            ArrayType value;
            if (any.GetValue(value)) {
                return InternalSetValue(value);
            }
        }
        return AnyReturn::FAIL;
    }

    TypeId GetTypeId(TypeIdRole role) const override
    {
        if (role == TypeIdRole::ARRAY) {
            return ARRAY_TYPE_ID;
        }
        if (role == TypeIdRole::ITEM) {
            return ITEM_TYPE_ID;
        }
        return TYPE_ID;
    }
    TypeId GetTypeId() const
    {
        return TYPE_ID;
    }
    BASE_NS::string GetTypeIdString() const override
    {
        return BASE_NS::string(StaticGetTypeIdString());
    }
    using IAny::SetValue;
    AnyReturnValue SetValue(const IAny& value) override
    {
        return CopyFrom(value);
    }
    using IAny::GetValue;
    const IAny& GetValue() const override
    {
        return *this;
    }
    bool IsCompatible(const TypeId& id) const override
    {
        return META_NS::IsCompatible(*this, id);
    }

protected:
    virtual AnyReturnValue InternalSetValue(const ArrayType& value) = 0;
    virtual const ArrayType& InternalGetValue() const = 0;

private:
    static constexpr bool IsValidVectorArgs(const TypeId& uid, const void* data, size_t size)
    {
        return data && sizeof(ArrayType) == size && uid == VECTOR_TYPE_ID;
    }
    static constexpr bool IsValidArrayArgs(const TypeId& uid, const void* data)
    {
        return data && uid == ARRAY_TYPE_ID;
    }
};

/**
 * @brief Default IArrayAny implementation which supports a single type.
 */
template<typename Type, typename Compare = DefaultCompare<BASE_NS::vector<Type>>>
class ArrayAny : public BaseTypedArrayAny<Type> {
    using Super = BaseTypedArrayAny<Type>;
public:
    using ArrayType = typename Super::ArrayType;
    using ItemType = typename Super::ItemType;
    using Super::ITEM_TYPE_ID;
    static constexpr auto ITEM_SIZE = sizeof(ItemType); /*NOLINT(bugprone-sizeof-expression)*/

public:
    explicit constexpr ArrayAny(ArrayType v = {}) : value_(BASE_NS::move(v)) {}
    explicit constexpr ArrayAny(const BASE_NS::array_view<const Type>& v) : value_(v.begin(), v.end()) {}
#ifdef BASE_VECTOR_HAS_INITIALIZE_LIST
    constexpr ArrayAny(std::initializer_list<Type> v) : value_(ArrayType(v)) {}
#endif

    AnyReturnValue GetDataAt(size_t index, const TypeId& id, void* data, size_t size) const override
    {
        if (IsValidItemArgs(id, data, size) && index < GetSize()) {
            *static_cast<ItemType*>(data) = value_[index];
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue SetDataAt(size_t index, const TypeId& id, const void* data, size_t size) override
    {
        if (IsValidItemArgs(id, data, size) && index < GetSize()) {
            value_[index] = *static_cast<const ItemType*>(data);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue SetAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        ItemType v;
        if (value.GetData(ITEM_TYPE_ID, &v, ITEM_SIZE)) {
            return SetDataAt(index, ITEM_TYPE_ID, &v, ITEM_SIZE);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue GetAnyAt(IArrayAny::IndexType index, IAny& value) const override
    {
        ItemType v;
        if (GetDataAt(index, ITEM_TYPE_ID, &v, ITEM_SIZE)) {
            return value.SetData(ITEM_TYPE_ID, &v, ITEM_SIZE);
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue InsertAnyAt(IArrayAny::IndexType index, const IAny& value) override
    {
        ItemType v;
        if (value.GetData(ITEM_TYPE_ID, &v, ITEM_SIZE)) {
            index = index < value_.size() ? index : value_.size();
            value_.insert(value_.begin() + index, v);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    AnyReturnValue RemoveAt(IArrayAny::IndexType index) override
    {
        if (index < value_.size()) {
            value_.erase(value_.begin() + index);
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::INVALID_ARGUMENT;
    }
    void RemoveAll() override
    {
        value_.clear();
    }

    IArrayAny::IndexType GetSize() const noexcept override
    {
        return value_.size();
    }
    IAny::Ptr Clone(const AnyCloneOptions& options) const override;
    IAny::Ptr Clone(bool withValue) const
    {
        return Clone({ withValue ? CloneValueType::COPY_VALUE : CloneValueType::DEFAULT_VALUE });
    }

    const ArrayType& InternalGetValue() const override
    {
        return value_;
    }

    void PushBack(ItemType item)
    {
        value_.push_back(BASE_NS::move(item));
    }

    AnyReturnValue ResetValue() override
    {
        return InternalSetValue({});
    }

private:
    AnyReturnValue InternalSetValue(const ArrayType& value) override
    {
        if (!Compare::Equal(value, value_)) {
            value_ = value;
            return AnyReturn::SUCCESS;
        }
        return AnyReturn::NOTHING_TO_DO;
    }

    static constexpr bool IsValidItemArgs(const TypeId& uid, const void* data, size_t size)
    {
        return data && ITEM_SIZE == size && uid == Super::ITEM_TYPE_ID;
    }

protected:
    ArrayType value_;
};

template<class Type, class Compare>
IAny::Ptr Any<Type, Compare>::Clone(const AnyCloneOptions& options) const
{
    if (options.role == TypeIdRole::ARRAY) {
        return IAny::Ptr(new ArrayAny<Type, typename Compare::template Rebind<BASE_NS::vector<Type>>>());
    }
    return IAny::Ptr(new Any { options.value == CloneValueType::COPY_VALUE ? value_ : Type {} });
}

template<class Type, class Compare>
IAny::Ptr ArrayAny<Type, Compare>::Clone(const AnyCloneOptions& options) const
{
    if (options.role == TypeIdRole::ITEM) {
        return IAny::Ptr(new Any<Type, typename Compare::template Rebind<Type>>());
    }
    return IAny::Ptr(new ArrayAny { options.value == CloneValueType::COPY_VALUE ? value_ : ArrayType {} });
}

template<typename Type, typename = EnableSpecialisationType>
struct MapAnyType {
    using AnyType = Any<Type>;
    using ArrayAnyType = ArrayAny<Type>;
};
template<class Type>
static IAny::Ptr ConstructAny(Type v = {})
{
    return IAny::Ptr { new typename MapAnyType<Type>::AnyType(BASE_NS::move(v)) };
}

template<class Type>
static IAny::Ptr ConstructAny(BASE_NS::vector<Type> v)
{
    return IAny::Ptr { new typename MapAnyType<Type>::ArrayAnyType(BASE_NS::move(v)) };
}

template<class Type>
static IArrayAny::Ptr ConstructArrayAny(BASE_NS::vector<Type> v = {})
{
    return IArrayAny::Ptr { new typename MapAnyType<Type>::ArrayAnyType(BASE_NS::move(v)) };
}

template<class Type>
static IArrayAny::Ptr ConstructArrayAny(BASE_NS::array_view<const Type> v)
{
    return IArrayAny::Ptr { new typename MapAnyType<Type>::ArrayAnyType(v) };
}

#ifdef BASE_VECTOR_HAS_INITIALIZE_LIST
template<class Type>
static IArrayAny::Ptr ConstructArrayAny(std::initializer_list<Type> l)
{
    return IArrayAny::Ptr { new typename MapAnyType<Type>::ArrayAnyType(l) };
}
#endif

META_END_NAMESPACE()

#endif
