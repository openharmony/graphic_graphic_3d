/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <algorithm>

#include <base/containers/allocator.h>
#include <core/log.h>
#include <core/property/intf_property_api.h>

using namespace CORE_NS;
using BASE_NS::array_view;
using BASE_NS::FNV1aHash;
using BASE_NS::string;
using BASE_NS::string_view;
using BASE_NS::vector;

namespace {
PropertyData::PropertyOffset FindProperty(
    array_view<const Property> properties, const string_view name, const uintptr_t baseOffset);

PropertyData::PropertyOffset FindPropertyMember(
    const Property& property, const string_view name, const uintptr_t baseOffset)
{
    PropertyData::PropertyOffset ret = FindProperty(property.metaData.memberProperties, name, baseOffset);
    if (ret) {
        ret.offset += property.offset;
    }
    return ret;
}

PropertyData::PropertyOffset FindProperty(
    array_view<const Property> properties, const string_view name, const uintptr_t baseOffset)
{
    PropertyData::PropertyOffset ret { nullptr, 0U, 0U };
    // trim down to name without array index or member variable.
    constexpr string_view delimiters = ".[";
    const auto delimPos = name.find_first_of(delimiters);
    auto property = std::find_if(properties.begin(), properties.end(),
        [baseName = name.substr(0, delimPos)](const Property& p) { return p.name == baseName; });
    if (property != properties.end()) {
        // if we have only a name we are done
        if (delimPos == string_view::npos) {
            ret.property = &*property;
            ret.offset += property->offset;
            ret.index = std::distance(properties.begin(), property);
        } else if (name[delimPos] == '[') {
            // there needs to be more than two characters after [ to be a valid array index. the propery must also be an
            // array.
            if ((name.size() - delimPos > 2) && property->metaData.containerMethods) { // 2: 2 chs on right of '['
                const char* start = name.substr(delimPos + 1).data();
                char* end;
                const unsigned long index = strtoul(start, &end, 10); // 10: base
                if (end && *end == ']') {
                    // probably managed to convert an array index
                    size_t size = property->type.isArray
                                      ? property->count
                                      : property->metaData.containerMethods->size(property->offset + baseOffset);
                    if (index < size) {
                        const auto arrayPos = static_cast<size_t>(end - name.data()) + 1;
                        if (arrayPos < name.size() && name[arrayPos] == '.') {
                            ret = FindPropertyMember(
                                property->metaData.containerMethods->property, name.substr(arrayPos + 1), baseOffset);
                        } else {
                            ret.property = &property->metaData.containerMethods->property;
                            ret.index = index;
                        }

                        if (property->type.isArray) {
                            ret.offset += property->offset + index * property->metaData.containerMethods->property.size;
                        } else {
                            ret.offset +=
                                property->metaData.containerMethods->get(property->offset + baseOffset, index) -
                                baseOffset;
                        }
                    }
                }
            }
        } else {
            ret = FindPropertyMember(*property, name.substr(delimPos + 1), baseOffset);
        }
    }
    return ret;
}
} // namespace

PropertyData::PropertyData()
{
    Reset();
}

bool PropertyData::WLock(IPropertyHandle& aHandle) // no-copy direct-access (Locks the datahandle);
{
    CORE_ASSERT(dataHandle_ == nullptr);
    CORE_ASSERT(dataHandleW_ == nullptr);
    dataHandleW_ = &aHandle;
    dataHandle_ = dataHandleW_;
    owner_ = dataHandleW_->Owner();
    size_ = dataHandleW_->Size();
    dataW_ = static_cast<uint8_t*>(dataHandleW_->WLock());
    data_ = dataW_;
    return true;
}

PropertyData::PropertyOffset PropertyData::WLock(IPropertyHandle& data, const string_view propertyPath)
{
    if (WLock(data)) {
        const uintptr_t baseOffset = reinterpret_cast<uintptr_t>(dataW_);
        if (auto po = FindProperty(Owner()->MetaData(), propertyPath, baseOffset); po) {
            return po;
        }
    }
    WUnlock(data);
    return { 0, 0 };
}

bool PropertyData::WUnlock(const IPropertyHandle& aHandle) // (releases the datahandle lock, and removes ref)
{
    CORE_ASSERT(dataHandleW_);
    CORE_ASSERT(dataHandleW_ == dataHandle_);
    CORE_ASSERT(dataHandleW_ == &aHandle);
    if (dataHandleW_ == &aHandle) {
        if (dataHandleW_) {
            dataHandleW_->WUnlock();
            Reset();
        }
        return true;
    }
    return false;
}

bool PropertyData::RLock(const IPropertyHandle& aHandle) // no-copy direct-access (Locks the datahandle);
{
    CORE_ASSERT(dataHandle_ == nullptr);
    CORE_ASSERT(dataHandleW_ == nullptr);
    dataHandleW_ = nullptr;
    dataHandle_ = &aHandle;
    owner_ = dataHandle_->Owner();
    size_ = dataHandle_->Size();
    data_ = dataHandle_->RLock();
    dataW_ = nullptr;
    return true;
}

PropertyData::PropertyOffset PropertyData::RLock(const IPropertyHandle& data, const string_view propertyPath)
{
    if (RLock(data)) {
        const uintptr_t baseOffset = reinterpret_cast<uintptr_t>(data_);
        if (auto po = FindProperty(Owner()->MetaData(), propertyPath, baseOffset); po) {
            return po;
        }
    }
    RUnlock(data);
    return { 0, 0 };
}

bool PropertyData::RUnlock(const IPropertyHandle& aHandle) // (releases the datahandle lock, and removes ref)
{
    CORE_ASSERT(dataHandle_);
    CORE_ASSERT(dataHandleW_ == nullptr);
    CORE_ASSERT(dataHandle_ == &aHandle);
    if (dataHandle_ == &aHandle) {
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
    if (owner_) {
        const auto& props_ = owner_->MetaData();
        if (index < props_.size() && dataW_) {
            const auto& meta = props_[index];
            return PropertyValue(
                &meta, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(dataW_) + meta.offset), meta.count);
        }
    }
    return PropertyValue();
}

PropertyValue PropertyData::Get(size_t index) const
{
    if (owner_) {
        const auto& props_ = owner_->MetaData();
        if (index < props_.size()) {
            const auto& meta = props_[index];
            return PropertyValue(
                &meta, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data_) + meta.offset), meta.count);
        }
    }
    return PropertyValue();
}

size_t PropertyData::PropertyCount() const
{
    if (owner_) {
        const auto& props_ = owner_->MetaData();
        return props_.size();
    }
    return 0;
}

PropertyValue PropertyData::Get(const string_view name)
{
    if (owner_) {
        const auto& props_ = owner_->MetaData();
        if (!props_.empty() && dataW_) {
            const auto nameHash = FNV1aHash(name.data(), name.size());
            for (const auto& meta : props_) {
                if (meta.hash == nameHash && meta.name == name) {
                    return PropertyValue(
                        &meta, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(dataW_) + meta.offset), meta.count);
                }
            }
        }
    }
    return PropertyValue();
}

PropertyValue PropertyData::Get(const string_view name) const
{
    if (owner_) {
        const auto& props_ = owner_->MetaData();
        if (!props_.empty()) {
            const auto nameHash = FNV1aHash(name.data(), name.size());
            for (const auto& meta : props_) {
                if (meta.hash == nameHash && meta.name == name) {
                    return PropertyValue(
                        &meta, reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(data_) + meta.offset), meta.count);
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
