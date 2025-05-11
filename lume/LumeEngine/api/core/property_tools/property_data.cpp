/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <climits>
#include <cstdlib>

#include <base/containers/array_view.h>
#include <base/containers/iterator.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/util/compile_time_hashes.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property.h>
#include <core/property_tools/property_data.h>

using namespace CORE_NS;
using BASE_NS::array_view;
using BASE_NS::FNV1aHash;
using BASE_NS::string;
using BASE_NS::string_view;

namespace {
unsigned long ContainerIndex(const string_view name, size_t& pos)
{
    const char* start = name.substr(pos).data();
    char* end = nullptr;
    const unsigned long index = std::strtoul(start, &end, 10); // 10: base
    // check that conversion stopped at the closing square bracket
    if (!end || *end != ']') {
        return ULONG_MAX;
    }
    // move past the closing square bracket
    pos = static_cast<size_t>(end - name.data()) + 1U;
    return index;
}

ptrdiff_t ContainerOffset(const uintptr_t baseOffset, const Property& property, const PropertyData::PropertyOffset& ret,
    const unsigned long index)
{
    ptrdiff_t offset = PTRDIFF_MAX;
    // calculate offset to the index. for arrays a direct offset, but for containers need to get the addess
    // inside the container and compensate the base and member offsets.
    auto* containerMethods = property.metaData.containerMethods;
    if (property.type.isArray && (index < property.count)) {
        offset = static_cast<ptrdiff_t>(index * containerMethods->property.size);
    } else if (!property.type.isArray && baseOffset && (index < containerMethods->size(property.offset + baseOffset))) {
        offset = static_cast<ptrdiff_t>(
            containerMethods->get(property.offset + baseOffset, index) - baseOffset - ret.offset);
    }
    return offset;
}

bool ParseIndex(const string_view name, const uintptr_t baseOffset, const Property& property,
    array_view<const Property>& properties, size_t& pos, PropertyData::PropertyOffset& ret)
{
    const unsigned long index = ContainerIndex(name, pos);
    if (index == ULONG_MAX) {
        return false;
    }

    // calculate offset to the index.
    const ptrdiff_t offset = ContainerOffset(baseOffset, property, ret, index);
    if (offset == PTRDIFF_MAX) {
        return false;
    }

    auto* containerMethods = property.metaData.containerMethods;
    ret.property = &containerMethods->property;
    ret.offset = static_cast<uintptr_t>(static_cast<ptrdiff_t>(ret.offset) + offset);
    ret.index = index;

    if (pos < name.size() && name[pos] == '.') {
        ++pos;
        // continue search from the member properties.
        properties = containerMethods->property.metaData.memberProperties;
    }
    return true;
}

PropertyData::PropertyOffset FindProperty(
    array_view<const Property> properties, const string_view name, const uintptr_t baseOffset)
{
    PropertyData::PropertyOffset ret { nullptr, 0U, 0U };

    // trim down to name without array index or member variable.
    static constexpr string_view delimiters = ".[";

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
            ret.index = static_cast<size_t>(property - properties.cbegin());

            // if we have only a name we are done
            if (pos == string_view::npos) {
                break;
            } else if (name[pos] == '.') {
                // there needs to be at least one character to be a valid name.
                if ((name.size() - pos) < 2U) {
                    ret = {};
                    break;
                }
                ++pos;
                // continue search from the member properties.
                properties = property->metaData.memberProperties;
            } else if (name[pos] == '[') {
                // there needs to be at least three characters (e.g. [0]) to be a valid array index. the propery must
                // also be an array.
                static constexpr size_t minLength = 3U;
                if (!property->metaData.containerMethods || (pos >= name.size()) || ((name.size() - pos) < minLength)) {
                    ret = {};
                    break;
                }
                ++pos;

                if (!ParseIndex(name, baseOffset, *property, properties, pos, ret)) {
                    ret = {};
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
    if (dataHandle_ || dataHandleW_) {
        return false;
    }
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
    return { nullptr, 0U, 0U };
}

bool PropertyData::WUnlock(const IPropertyHandle& handle) // (releases the datahandle lock, and removes ref)
{
    BASE_ASSERT(dataHandleW_);
    BASE_ASSERT(dataHandleW_ == dataHandle_);
    BASE_ASSERT(dataHandleW_ == &handle);
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
    if (dataHandle_ || dataHandleW_) {
        return false;
    }
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
    return { nullptr, 0U, 0U };
}

bool PropertyData::RUnlock(const IPropertyHandle& handle) // (releases the datahandle lock, and removes ref)
{
    BASE_ASSERT(dataHandle_);
    BASE_ASSERT(dataHandleW_ == nullptr);
    BASE_ASSERT(dataHandle_ == &handle);
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

PropertyData::PropertyOffset PropertyData::FindProperty(const IPropertyHandle& handle, const string_view propertyPath)
{
    if (auto owner = handle.Owner()) {
        const auto baseAddress = reinterpret_cast<uintptr_t>(handle.RLock());
        handle.RUnlock();
        return ::FindProperty(owner->MetaData(), propertyPath, baseAddress);
    }
    return {};
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

size_t PropertyData::PropertyCount() const
{
    if (owner_) {
        const auto& props = owner_->MetaData();
        return props.size();
    }
    return 0;
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
