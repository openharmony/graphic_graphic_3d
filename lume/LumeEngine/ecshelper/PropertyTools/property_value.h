/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

#ifndef CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_VALUE_H
#define CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_VALUE_H

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/namespace.h>
#include <core/property/property.h>
#include <base/util/log.h>
#include <base/containers/type_traits.h>
#include <core/property/property_types.h>

CORE_BEGIN_NAMESPACE()
class PropertyValue {
public:
    PropertyValue() noexcept : count_(0), data_(nullptr), index_(0) {}
    ~PropertyValue() = default;
    PropertyValue(const PropertyValue& other) noexcept = default;

    PropertyValue(PropertyValue&& other) noexcept
        : type_(other.type_), // exchange(other.type_, PropertyType::INVALID)),
          count_(BASE_NS::exchange(other.count_, 0U)), data_(BASE_NS::exchange(other.data_, nullptr)),
          index_(BASE_NS::exchange(other.index_, 0U))
    {}
    explicit constexpr PropertyValue(const Property* type, void* rawData, size_t cnt) noexcept
        : type_(type), count_(cnt), data_(rawData), index_(0U)
    {}

    constexpr size_t Size() const
    {
        return count_;
    }
    constexpr PropertyTypeDecl GetType() const
    {
        return type_->type;
    }
    constexpr operator PropertyTypeDecl() const
    {
        return type_->type;
    }
    constexpr bool operator==(const PropertyTypeDecl& other) const
    {
        return type_->type.compareHash == other.compareHash;
    }

    BASE_NS::array_view<const Property> MetaData() const
    {
        if (type_) {
            return type_->metaData.memberProperties;
        }
        return {};
    }
    const Property* MetaData(size_t index) const
    {
        if (type_) {
            const auto& meta = type_->metaData.memberProperties;
            if (index < meta.size()) {
                return &meta[index];
            }
        }
        return nullptr;
    }

    // void* access
    explicit constexpr operator void*()
    {
        return data_;
    }
    explicit constexpr operator void const*() const
    {
        return data_;
    }

    // safe
    PropertyValue operator[](const size_t index) const
    {
        BASE_ASSERT(type_->type.isArray); // This accessor type is valid only for arrays.
        BASE_ASSERT(index < type_->count);
        return PropertyValue(type_, data_, 1, index);
    }
    PropertyValue operator[](const size_t index)
    {
        BASE_ASSERT(type_->type.isArray); // This accessor type is valid only for arrays.
        BASE_ASSERT(index < type_->count);
        return PropertyValue(type_, data_, 1, index);
    }

    PropertyValue operator[](BASE_NS::string_view name) const
    {
        uintptr_t offset = reinterpret_cast<uintptr_t>(data_);
        const auto nameHash = BASE_NS::FNV1aHash(name.data(), name.size());
        for (const auto& p : type_->metaData.memberProperties) {
            if ((nameHash == p.hash) && (p.name == name)) {
                return PropertyValue(&p, reinterpret_cast<void*>(offset + p.offset), p.count);
            }
        }
        // no such member property
        return {};
    }

    // unsafe
    template<typename T>
    [[deprecated]] constexpr operator T() const
    {
        return reinterpret_cast<T*>(data_)[index_];
    }

    template<typename T>
    [[deprecated]] constexpr operator T&()
    {
        return reinterpret_cast<T*>(data_)[index_];
    }

    template<typename T>
    [[deprecated]] PropertyValue& operator=(T v)
    {
        reinterpret_cast<T*>(data_)[index_] = v;
        return *this;
    }

    PropertyValue& operator=(const PropertyValue& other) noexcept = default;

    PropertyValue& operator=(PropertyValue&& other) noexcept
    {
        if (this != &other) {
            type_ = BASE_NS::exchange(other.type_, {});
            count_ = BASE_NS::exchange(other.count_, 0U);
            data_ = BASE_NS::exchange(other.data_, nullptr);
            index_ = BASE_NS::exchange(other.index_, 0U);
        }
        return *this;
    }

private:
    const Property* type_ { nullptr };
    size_t count_;
    void* data_;
    size_t index_;
    explicit constexpr PropertyValue(const Property* type, void* rawData, size_t cnt, size_t index)
        : type_(type), count_(cnt), data_(rawData), index_(index)
    {}
};
CORE_END_NAMESPACE()

#endif // CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_VALUE_H
