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

#ifndef META_INTERFACE_DETAIL_MULTI_TYPE_ANY_H
#define META_INTERFACE_DETAIL_MULTI_TYPE_ANY_H

#include <meta/interface/detail/any.h>

META_BEGIN_NAMESPACE()

constexpr char MULTI_TYPE_ANY_TAG[] = "MultiAny";
constexpr char MULTI_TYPE_ARRAY_ANY_TAG[] = "MultsAny";

struct StaticCastConv {
    template<typename T1, typename T2>
    static T1 ToType(const T2& v)
    {
        return static_cast<T1>(v);
    }
};

template<typename Type, typename Conv, typename... CompatType>
class MultiTypeAny : public IntroduceInterfaces<IAny, IValue> {
    using Super = IntroduceInterfaces;

public:
    static constexpr TypeId TYPE_ID = UidFromType<Type>();

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(MULTI_TYPE_ANY_TAG);
    }
    static constexpr BASE_NS::string_view StaticGetTypeIdString()
    {
        return MetaType<Type>::name;
    }
    static const BASE_NS::array_view<const TypeId> StaticGetCompatibleTypes(CompatibilityDirection)
    {
        static constexpr TypeId ids[] = { TYPE_ID, UidFromType<CompatType>()... };
        return ids;
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
        if (!data) {
            return AnyReturn::INVALID_ARGUMENT;
        }
        if (sizeof(Type) == size && id == TYPE_ID) { /*NOLINT(bugprone-sizeof-expression)*/
            *static_cast<Type*>(data) = InternalGetValue();
            return AnyReturn::SUCCESS;
        }

        AnyReturnValue res = AnyReturn::INVALID_ARGUMENT;
        [[maybe_unused]] bool r = ((res = [&]() -> AnyReturnValue {
            if (sizeof(CompatType) == size && id == UidFromType<CompatType>()) { /*NOLINT(bugprone-sizeof-expression)*/
                *static_cast<CompatType*>(data) = Conv::template ToType<CompatType>(InternalGetValue());
                return AnyReturn::SUCCESS;
            }
            return AnyReturn::INVALID_ARGUMENT;
        }()) || ...);
        return res;
    }
    AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) override
    {
        if (!data) {
            return AnyReturn::INVALID_ARGUMENT;
        }
        if (sizeof(Type) == size && id == TYPE_ID) { /*NOLINT(bugprone-sizeof-expression)*/
            return InternalSetValue(*static_cast<const Type*>(data));
        }
        AnyReturnValue res = AnyReturn::INVALID_ARGUMENT;
        [[maybe_unused]] bool r = ((res = [&]() -> AnyReturnValue {
            if (sizeof(CompatType) == size && id == UidFromType<CompatType>()) { /*NOLINT(bugprone-sizeof-expression)*/
                return InternalSetValue(Conv::template ToType<Type>(*static_cast<const CompatType*>(data)));
            }
            return AnyReturn::INVALID_ARGUMENT;
        }()) || ...);
        return res;
    }
    AnyReturnValue CopyFrom(const IAny& any) override
    {
        if (META_NS::IsCompatible(any, TYPE_ID, CompatibilityDirection::GET)) {
            Type value;
            if (any.GetValue(value)) {
                return InternalSetValue(value);
            }
        }

        AnyReturnValue res = AnyReturn::FAIL;
        [[maybe_unused]] bool r = ((res = [&]() -> AnyReturnValue {
            if (META_NS::IsCompatible(any, UidFromType<CompatType>(), CompatibilityDirection::GET)) {
                CompatType value;
                if (any.GetValue(value)) {
                    return InternalSetValue(Conv::template ToType<Type>(value));
                }
            }
            return AnyReturn::FAIL;
        }()) || ...);
        return res;
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

    AnyReturnValue ResetValue() override
    {
        return InternalSetValue({});
    }

protected:
    virtual AnyReturnValue InternalSetValue(const Type& value) = 0;
    virtual const Type& InternalGetValue() const = 0;
};

// idea is just to provide array any that gives you correct any type when cloning ITEM
template<typename Type>
class ArrayMultiTypeAnyBase : public ArrayAny<Type> {
    using Super = ArrayAny<Type>;

public:
    using Super::Super;

    static constexpr ObjectId StaticGetClassId()
    {
        return MakeUid<Type>(MULTI_TYPE_ARRAY_ANY_TAG);
    }

    ObjectId GetClassId() const override
    {
        return StaticGetClassId();
    }
};

META_END_NAMESPACE()

#endif
