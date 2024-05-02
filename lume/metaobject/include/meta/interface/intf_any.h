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

#ifndef META_INTERFACE_ANY_H
#define META_INTERFACE_ANY_H

#include <core/plugin/intf_interface.h>

#include <meta/base/expected.h>
#include <meta/base/ids.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAny, "9e6d554e-3647-4040-a5c4-ea299fa85b8f")
META_REGISTER_INTERFACE(IArrayAny, "d3c49aa9-9314-4680-b23a-7d2a24a0af30")

enum class AnyReturn {
    NOTHING_TO_DO = 2,
    SUCCESS = 1,
    UNUSED = 0,
    FAIL = -1,
    INVALID_ARGUMENT = -2,
    INCOMPATIBLE_TYPE = -3,
    NOT_SUPPORTED = -4,
    RECURSIVE_CALL = -5
};
inline bool operator<(AnyReturn l, AnyReturn r)
{
    return int64_t(l) < int64_t(r);
}
inline bool operator>(AnyReturn l, AnyReturn r)
{
    return int64_t(l) > int64_t(r);
}

using AnyReturnValue = ReturnValue<AnyReturn>;

enum class CompatibilityDirection { GET, SET, BOTH };
enum class TypeIdRole { CURRENT, ITEM, ARRAY };
enum class CloneValueType { COPY_VALUE, DEFAULT_VALUE };

struct AnyCloneOptions {
    /** Should value be cloned. Ignored if role != TypeIdRole::CURRENT */
    CloneValueType value { CloneValueType::COPY_VALUE };
    /** Type of Any (same as source, item or array type) the clone should be. */
    TypeIdRole role { TypeIdRole::CURRENT };
};

class IAny : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAny)
public:
    virtual ObjectId GetClassId() const = 0;
    virtual const BASE_NS::array_view<const TypeId> GetCompatibleTypes(CompatibilityDirection) const = 0;
    virtual AnyReturnValue GetData(const TypeId& id, void* data, size_t size) const = 0;
    virtual AnyReturnValue SetData(const TypeId& id, const void* data, size_t size) = 0;
    virtual AnyReturnValue CopyFrom(const IAny& any) = 0;
    virtual IAny::Ptr Clone(const AnyCloneOptions& options) const = 0;
    virtual TypeId GetTypeId(TypeIdRole role) const = 0;
    virtual BASE_NS::string GetTypeIdString() const = 0;

    TypeId GetTypeId() const
    {
        return GetTypeId(TypeIdRole::CURRENT);
    }
    bool IsArray() const
    {
        return GetTypeId(TypeIdRole::CURRENT) == GetTypeId(TypeIdRole::ARRAY);
    }
    IAny::Ptr Clone() const
    {
        return Clone({ CloneValueType::COPY_VALUE });
    }
    IAny::Ptr Clone(bool withValue) const
    {
        return Clone({ withValue ? CloneValueType::COPY_VALUE : CloneValueType::DEFAULT_VALUE });
    }
    template<typename T>
    AnyReturnValue GetValue(T& value) const
    {
        return GetData(UidFromType<T>(), &value, sizeof(T)); /*NOLINT(bugprone-sizeof-expression)*/
    }
    template<typename T>
    AnyReturnValue SetValue(const T& value)
    {
        return SetData(UidFromType<T>(), &value, sizeof(T)); /*NOLINT(bugprone-sizeof-expression)*/
    }
};

class IArrayAny : public IAny {
    META_INTERFACE(IAny, IArrayAny)
public:
    using IndexType = size_t;

    virtual AnyReturnValue GetDataAt(size_t index, const TypeId& id, void* data, size_t size) const = 0;
    virtual AnyReturnValue SetDataAt(size_t index, const TypeId& id, const void* data, size_t size) = 0;
    virtual AnyReturnValue SetAnyAt(IndexType index, const IAny& value) = 0;
    virtual AnyReturnValue GetAnyAt(IndexType index, IAny& value) const = 0;
    virtual AnyReturnValue InsertAnyAt(IndexType index, const IAny& value) = 0;
    virtual AnyReturnValue RemoveAt(IndexType index) = 0;
    virtual void RemoveAll() = 0;

    virtual IndexType GetSize() const = 0;

    template<typename T>
    AnyReturnValue GetValueAt(IndexType index, T& value) const
    {
        return GetDataAt(index, UidFromType<T>(), &value, sizeof(T)); /*NOLINT(bugprone-sizeof-expression)*/
    }
    template<typename T>
    AnyReturnValue SetValueAt(IndexType index, const T& value)
    {
        return SetDataAt(index, UidFromType<T>(), &value, sizeof(T)); /*NOLINT(bugprone-sizeof-expression)*/
    }
};

inline bool IsArray(const IAny& any)
{
    return any.GetInterface<IArrayAny>() != nullptr;
}

inline bool IsArray(const IAny::ConstPtr& any)
{
    return any && any->GetInterface<IArrayAny>() != nullptr;
}

template<typename T>
inline T GetValue(const IAny& any, NonDeduced_t<T> defaultValue = {})
{
    T t;
    if (!any.GetValue(t)) {
        t = defaultValue;
    }
    return t;
}

template<typename T>
inline T GetValueAt(const IArrayAny& array, IArrayAny::IndexType index, NonDeduced_t<T> defaultValue = {})
{
    T t;
    if (!array.GetValueAt(index, t)) {
        t = defaultValue;
    }
    return t;
}

inline bool IsCompatible(const IAny& any, const TypeId& uid, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    for (auto&& v : any.GetCompatibleTypes(dir)) {
        if (uid == v) {
            return true;
        }
    }
    return false;
}

inline bool IsCompatibleWith(
    const IAny& any, const IAny& other, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    for (auto&& v : other.GetCompatibleTypes(dir)) {
        if (IsCompatible(any, v, dir)) {
            return true;
        }
    }
    return false;
}

template<typename T>
inline bool IsCompatibleWith(const IAny& any, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    return IsCompatible(any, UidFromType<BASE_NS::remove_const_t<BASE_NS::remove_reference_t<T>>>(), dir);
}

inline bool IsSetCompatible(const IAny& any, const TypeId& uid)
{
    return IsCompatible(any, uid, CompatibilityDirection::SET);
}

inline bool IsGetCompatible(const IAny& any, const TypeId& uid)
{
    return IsCompatible(any, uid, CompatibilityDirection::GET);
}

template<typename T>
inline bool IsSetCompatibleWith(const IAny& any)
{
    return IsCompatibleWith<T>(any, CompatibilityDirection::SET);
}

template<typename T>
inline bool IsGetCompatibleWith(const IAny& any)
{
    return IsCompatibleWith<T>(any, CompatibilityDirection::GET);
}

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IAny)
META_INTERFACE_TYPE(META_NS::IArrayAny)

#endif
