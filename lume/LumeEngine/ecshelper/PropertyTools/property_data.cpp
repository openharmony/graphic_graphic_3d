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

#include "PropertyTools/property_data.h"

#include <PropertyTools/property_value.h>
#include <algorithm>
#include <climits>
#include <cstdlib>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/util/compile_time_hashes.h>
#include <core/log.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property.h>

using namespace CORE_NS;
using BASE_NS::array_view;
using BASE_NS::FNV1aHash;
using BASE_NS::string;
using BASE_NS::string_view;

namespace {
bool ParseIndex(const string_view name, const uintptr_t baseOffset, const Property& property,
    array_view<const Property>& properties, size_t& pos, PropertyData::PropertyOffset& ret)
{
    // there needs to be at least three characters to be a valid array index. the propery must also be an
    // array.
    if (((name.size() - pos) < 3U) || !property.metaData.containerMethods) { // 3: min length e.g. [0]
        ret = {};
        return false;
    }
    ret.propertyPath = name.substr(0, pos);
    ++pos;

    const char* start = name.substr(pos).data();
    char* end = nullptr;
    const unsigned long index = strtoul(start, &end, 10); // 10: base
    // check that conversion stopped at the closing square bracket
    if (!end || *end != ']') {
        ret = {};
        return false;
    }
    // move past the closing square bracket and store the path so far
    pos = static_cast<size_t>(end - name.data()) + 1U;
    ret.propertyPath = name.substr(0, pos);

    auto* containerMethods = property.metaData.containerMethods;

    // calculate offset to the index. for arrays a direct offset, but for containers need to get the addess
    // inside the container and compensate the base and member offsets.
    ptrdiff_t offset = PTRDIFF_MAX;
    if (property.type.isArray && (index < property.count)) {
        offset = static_cast<ptrdiff_t>(index * containerMethods->property.size);
    } else if (!property.type.isArray && baseOffset) {
        if (index < containerMethods->size(property.offset + baseOffset)) {
            offset = static_cast<ptrdiff_t>(
                containerMethods->get(property.offset + baseOffset, index) - baseOffset - ret.offset);
        }
    }

    if (offset != PTRDIFF_MAX) {
        ret.property = &containerMethods->property;
        ret.offset = static_cast<uintptr_t>(static_cast<ptrdiff_t>(ret.offset) + offset);
        ret.index = index;

        if (pos < name.size() && name[pos] == '.') {
            ++pos;
            // continue search from the member properties.
            properties = containerMethods->property.metaData.memberProperties;
        }
    } else {
        ret = {};
        return false;
    }
    return true;
}

PropertyData::PropertyOffset FindProperty(
    array_view<const Property> properties, const string_view name, const uintptr_t baseOffset)
{
    PropertyData::PropertyOffset ret { nullptr, 0U, 0U, {} };

    // trim down to name without array index or member variable.
    constexpr const string_view delimiters = ".[";

    size_t pos = 0U;
    while (pos < name.size()) {
        const auto delimPos = name.find_first_of(delimiters, pos);
        auto baseName = name.substr(pos, delimPos - pos);
        pos = delimPos;
        if (baseName.empty()) {
            ret = {};
            break;
        }

        auto property = std::find_if(
            properties.cbegin(), properties.cend(), [baseName](const Property& p) { return p.name == baseName; });
        if (property != properties.cend()) {
            // remember what property this was
            ret.property = &*property;
            ret.offset += property->offset;
            ret.index = static_cast<size_t>(std::distance(properties.cbegin(), property));

            // if we have only a name we are done
            if (pos == string_view::npos) {
                ret.propertyPath = name;
            } else if (name[pos] == '.') {
                // there needs to be at least one character to be a valid name.
                if ((name.size() - pos) < 2U) {
                    ret = {};
                    break;
                }
                ret.propertyPath = name.substr(0, pos);
                ++pos;
                // continue search from the member properties.
                properties = property->metaData.memberProperties;
            } else if (name[pos] == '[') {
                if (!ParseIndex(name, baseOffset, *property, properties, pos, ret)) {
                    break;
                }
            }
        } else if (!properties.empty()) {
            ret = {};
            break;
        } else {
            break;
        }
    }
    return ret;
}
} // namespace

PropertyData::PropertyData()
{
    Reset();
}

bool PropertyData::WLock(IPropertyHandle& handle) // no-copy direct-access (Locks the datahandle);
{
    CORE_ASSERT(dataHandle_ == nullptr);
    CORE_ASSERT(dataHandleW_ == nullptr);
    dataHandleW_ = &handle;
    dataHandle_ = dataHandleW_;
    owner_ = dataHandleW_->Owner();
    size_ = dataHandleW_->Size();
    dataW_ = static_cast<uint8_t*>(dataHandleW_->WLock());
    data_ = dataW_;
    return true;
}

PropertyData::PropertyOffset PropertyData::WLock(IPropertyHandle& handle, const string_view propertyPath)
{
    if (WLock(handle)) {
        const uintptr_t baseOffset = reinterpret_cast<uintptr_t>(dataW_);
        if (auto po = FindProperty(Owner()->MetaData(), propertyPath, baseOffset); po) {
            return po;
        }
    }
    WUnlock(handle);
    return { nullptr, 0U, 0U, {} };
}

bool PropertyData::WUnlock(const IPropertyHandle& handle) // (releases the datahandle lock, and removes ref)
{
    CORE_ASSERT(dataHandleW_);
    CORE_ASSERT(dataHandleW_ == dataHandle_);
    CORE_ASSERT(dataHandleW_ == &handle);
    if (dataHandleW_ == &handle) {
        if (dataHandleW_) {
            dataHandleW_->WUnlock();
            Reset();
        }
        return true;
    }
    return false;
}

bool PropertyData::RLock(const IPropertyHandle& handle) // no-copy direct-access (Locks the datahandle);
{
    CORE_ASSERT(dataHandle_ == nullptr);
    CORE_ASSERT(dataHandleW_ == nullptr);
    dataHandleW_ = nullptr;
    dataHandle_ = &handle;
    owner_ = dataHandle_->Owner();
    size_ = dataHandle_->Size();
    data_ = dataHandle_->RLock();
    dataW_ = nullptr;
    return true;
}

PropertyData::PropertyOffset PropertyData::RLock(const IPropertyHandle& handle, const string_view propertyPath)
{
    if (RLock(handle)) {
        const uintptr_t baseOffset = reinterpret_cast<uintptr_t>(data_);
        if (auto po = FindProperty(Owner()->MetaData(), propertyPath, baseOffset); po) {
            return po;
        }
    }
    RUnlock(handle);
    return { nullptr, 0U, 0U, {} };
}

bool PropertyData::RUnlock(const IPropertyHandle& handle) // (releases the datahandle lock, and removes ref)
{
    CORE_ASSERT(dataHandle_);
    CORE_ASSERT(dataHandleW_ == nullptr);
    CORE_ASSERT(dataHandle_ == &handle);
    if (dataHandle_ == &handle) {
        if (dataHandle_) {
            dataHandle_->RUnlock();
            Reset();
        }
        return true;
    }
    return false;
}

PropertyData::PropertyOffset PropertyData::FindProperty(
    const array_view<const Property> properties, const string_view propertyPath, const uintptr_t baseOffset)
{
    PropertyData::PropertyOffset offset = ::FindProperty(properties, propertyPath, baseOffset);
    if (offset) {
        offset.offset += baseOffset;
    }
    return offset;
}

PropertyData::PropertyOffset PropertyData::FindProperty(
    const array_view<const Property> properties, const string_view propertyPath)
{
    return ::FindProperty(properties, propertyPath, 0U);
}

PropertyData::~PropertyData()
{
    if (dataHandleW_) {
        WUnlock(*dataHandleW_);
    }
    if (dataHandle_) {
        RUnlock(*dataHandle_);
    }
}

void PropertyData::Reset()
{
    size_ = 0;
    data_ = nullptr;
    dataW_ = nullptr;
    owner_ = nullptr;
    dataHandle_ = nullptr;
    dataHandleW_ = nullptr;
}

array_view<const Property> PropertyData::MetaData() const
{
    if (owner_) {
        return owner_->MetaData();
    }
    return {};
}

const Property* PropertyData::MetaData(size_t index) const
{
    if (owner_) {
        const auto& meta = owner_->MetaData();
        if (index < meta.size()) {
            return &meta[index];
        }
    }
    return nullptr;
}

PropertyValue PropertyData::Get(size_t index)
{
    if (owner_ && dataW_) {
        const auto& props = owner_->MetaData();
        if (index < props.size()) {
            const auto& meta = props[index];
            return PropertyValue(&meta, static_cast<void*>(dataW_ + meta.offset), meta.count);
        }
    }
    return PropertyValue();
}

PropertyValue PropertyData::Get(size_t index) const
{
    if (owner_ && data_) {
        const auto& props = owner_->MetaData();
        if (index < props.size()) {
            const auto& meta = props[index];
            return PropertyValue(&meta,
                const_cast<void*>(static_cast<const void*>(static_cast<const uint8_t*>(data_) + meta.offset)),
                meta.count);
        }
    }
    return PropertyValue();
}

size_t PropertyData::PropertyCount() const
{
    if (owner_) {
        const auto& props = owner_->MetaData();
        return props.size();
    }
    return 0;
}

PropertyValue PropertyData::Get(const string_view name)
{
    if (owner_ && dataW_) {
        const auto& props = owner_->MetaData();
        if (!props.empty()) {
            const auto nameHash = FNV1aHash(name.data(), name.size());
            for (const auto& meta : props) {
                if (meta.hash == nameHash && meta.name == name) {
                    return PropertyValue(&meta, static_cast<void*>(dataW_ + meta.offset), meta.count);
                }
            }
        }
    }
    return PropertyValue();
}

PropertyValue PropertyData::Get(const string_view name) const
{
    if (owner_ && data_) {
        const auto& props = owner_->MetaData();
        if (!props.empty()) {
            const auto nameHash = FNV1aHash(name.data(), name.size());
            for (const auto& meta : props) {
                if (meta.hash == nameHash && meta.name == name) {
                    return PropertyValue(&meta,
                        const_cast<void*>(static_cast<const void*>(static_cast<const uint8_t*>(data_) + meta.offset)),
                        meta.count);
                }
            }
        }
    }
    return PropertyValue();
}

PropertyValue PropertyData::operator[](size_t index)
{
    return Get(index);
}

PropertyValue PropertyData::operator[](size_t index) const
{
    return Get(index);
}

PropertyValue PropertyData::operator[](const string_view& name)
{
    return Get(name);
}

PropertyValue PropertyData::operator[](const string_view& name) const
{
    return Get(name);
}

const IPropertyApi* PropertyData::Owner() const
{
    return owner_;
}

size_t PropertyData::Size() const
{
    return size_;
}

const void* PropertyData::RLock() const
{
    return data_;
}

void* PropertyData::WLock()
{
    return dataW_;
}

void PropertyData::RUnlock() const {}

void PropertyData::WUnlock() {}
