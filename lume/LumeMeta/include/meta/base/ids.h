/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2022-2023. All rights reserved.
 * Description: Uid related types
 * Author: Mikael Kilpeläinen
 */
#ifndef META_BASE_IDS_H
#define META_BASE_IDS_H

#include <base/containers/string.h>
#include <base/util/uid_util.h>

#include <meta/base/meta_types.h>
#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

template<typename>
class IdBase {
public:
    constexpr IdBase(const BASE_NS::Uid& id = {}) noexcept : id_(id) {}

    explicit constexpr IdBase(const char (&str)[37]) noexcept : id_(str) {}

    BASE_NS::string ToString() const noexcept
    {
        return BASE_NS::string(BASE_NS::to_string(id_));
    }

    constexpr BASE_NS::Uid ToUid() const noexcept
    {
        return id_;
    }

    constexpr bool IsValid() const noexcept
    {
        return id_.data[0] && id_.data[1];
    }

    constexpr bool operator==(const IdBase& r) const noexcept
    {
        return id_ == r.id_;
    }
    constexpr bool operator!=(const IdBase& r) const noexcept
    {
        return id_ != r.id_;
    }
    constexpr bool operator<(const IdBase& r) const noexcept
    {
        return id_ < r.id_;
    }
    constexpr int Compare(const IdBase& r) const noexcept
    {
        return id_.compare(r.id_);
    }

protected:
    BASE_NS::Uid id_;
};

/// Type id class, for plain types and interfaces
class TypeId : public IdBase<TypeId> {
public:
    using IdBase<TypeId>::IdBase;
};

/// Object id class, for concrete class types
class ObjectId : public IdBase<ObjectId> {
public:
    using IdBase<ObjectId>::IdBase;
};

/// Instance id class, for object instances
class InstanceId : public IdBase<InstanceId> {
public:
    using IdBase<InstanceId>::IdBase;
};

META_TYPE(META_NS::TypeId);
META_TYPE(META_NS::ObjectId);
META_TYPE(META_NS::InstanceId);

/// Get type id for Type
template<typename Type>
inline constexpr TypeId GetTypeId()
{
    return UidFromType<Type>();
}

/// Get type id for array with element Type
template<typename Type>
inline constexpr TypeId GetArrayTypeId()
{
    return ArrayUidFromType<Type>();
}

META_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()

template<>
inline uint64_t hash(const META_NS::TypeId& value) // NOLINT(readability-inconsistent-declaration-parameter-name)
{
    return hash(value.ToUid());
}
template<>
inline uint64_t hash(const META_NS::ObjectId& value) // NOLINT(readability-inconsistent-declaration-parameter-name)
{
    return hash(value.ToUid());
}
template<>
inline uint64_t hash(const META_NS::InstanceId& value) // NOLINT(readability-inconsistent-declaration-parameter-name)
{
    return hash(value.ToUid());
}
BASE_END_NAMESPACE()

#endif
