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

class TypeId : public IdBase<TypeId> {
public:
    using IdBase<TypeId>::IdBase;
};

class ObjectId : public IdBase<ObjectId> {
public:
    using IdBase<ObjectId>::IdBase;
};

class InstanceId : public IdBase<InstanceId> {
public:
    using IdBase<InstanceId>::IdBase;
};

META_TYPE(TypeId);
META_TYPE(ObjectId);
META_TYPE(InstanceId);

META_END_NAMESPACE()

BASE_BEGIN_NAMESPACE()
template<>
inline uint64_t hash(const META_NS::TypeId& value)
{
    return hash(value.ToUid());
}
template<>
inline uint64_t hash(const META_NS::ObjectId& value)
{
    return hash(value.ToUid());
}
template<>
inline uint64_t hash(const META_NS::InstanceId& value)
{
    return hash(value.ToUid());
}
BASE_END_NAMESPACE()

#endif
