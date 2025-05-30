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

#include "util/property_util.h"

#include <cinttypes>

#include <base/math/vector.h>
#include <core/log.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_types.h>
#include <core/property_tools/core_metadata.inl>
#include <render/property/property_types.h>

#include "util/json_util.h"
#include "util/log.h"

RENDER_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

namespace {
constexpr size_t BUFFER_HANDLE_REF_BYTE_SIZE = sizeof(BindableBufferWithHandleReference);
constexpr size_t IMAGE_HANDLE_REF_BYTE_SIZE = sizeof(BindableImageWithHandleReference);
constexpr size_t SAMPLER_HANDLE_REF_BYTE_SIZE = sizeof(BindableSamplerWithHandleReference);
constexpr size_t MAX_STRUCT_HANDLE_REF_BYTE_SIZE =
    Math::max(BUFFER_HANDLE_REF_BYTE_SIZE, Math::max(IMAGE_HANDLE_REF_BYTE_SIZE, SAMPLER_HANDLE_REF_BYTE_SIZE));

uint32_t GetPropertyTypeByteSize(const PropertyTypeDecl& typeDecl)
{
    uint32_t byteSize = 0; // zero means that un-supported property type
    switch (typeDecl) {
        case PropertyType::UINT32_T:
        case PropertyType::INT32_T:
        case PropertyType::FLOAT_T:
        case PropertyType::BOOL_T:
            byteSize = sizeof(uint32_t);
            break;
        case PropertyType::UVEC2_T:
        case PropertyType::IVEC2_T:
        case PropertyType::VEC2_T:
            byteSize = sizeof(Math::UVec2);
            break;
        case PropertyType::UVEC3_T:
        case PropertyType::IVEC3_T:
        case PropertyType::VEC3_T:
            byteSize = sizeof(Math::UVec3);
            break;
        case PropertyType::UVEC4_T:
        case PropertyType::IVEC4_T:
        case PropertyType::VEC4_T:
            byteSize = sizeof(Math::UVec4);
            break;
        case PropertyType::MAT3X3_T:
            byteSize = sizeof(Math::Mat3X3);
            break;
        case PropertyType::MAT4X4_T:
            byteSize = sizeof(Math::Mat4X4);
            break;
    }
    return byteSize;
}

constexpr MetaData GetMetaData(const PropertyTypeDecl& typeDecl)
{
    switch (typeDecl) {
        case PropertyType::UINT32_T:
            return PropertyType::MetaDataFrom<uint32_t>();
        case PropertyType::INT32_T:
            return PropertyType::MetaDataFrom<int32_t>();
        case PropertyType::FLOAT_T:
            return PropertyType::MetaDataFrom<float>();

        case PropertyType::BOOL_T:
            return PropertyType::MetaDataFrom<bool>();

        case PropertyType::UVEC2_T:
            return PropertyType::MetaDataFrom<Math::UVec2>();
        case PropertyType::IVEC2_T:
            return PropertyType::MetaDataFrom<Math::IVec2>();
        case PropertyType::VEC2_T:
            return PropertyType::MetaDataFrom<Math::Vec2>();

        case PropertyType::UVEC3_T:
            return PropertyType::MetaDataFrom<Math::UVec3>();
        case PropertyType::IVEC3_T:
            return PropertyType::MetaDataFrom<Math::IVec3>();
        case PropertyType::VEC3_T:
            return PropertyType::MetaDataFrom<Math::Vec3>();

        case PropertyType::UVEC4_T:
            return PropertyType::MetaDataFrom<Math::UVec4>();
        case PropertyType::IVEC4_T:
            return PropertyType::MetaDataFrom<Math::IVec4>();
        case PropertyType::VEC4_T:
            return PropertyType::MetaDataFrom<Math::Vec4>();

        case PropertyType::MAT3X3_T:
            return PropertyType::MetaDataFrom<Math::Mat3X3>();
        case PropertyType::MAT4X4_T:
            return PropertyType::MetaDataFrom<Math::Mat4X4>();
    }
    return {};
}
} // namespace

CustomPropertyPodContainer::CustomPropertyPodContainer(size_t reserveByteSize)
{
    data_.reserve(reserveByteSize);
}

size_t CustomPropertyPodContainer::PropertyCount() const
{
    return metaData_.size();
}

const Property* CustomPropertyPodContainer::MetaData(size_t index) const
{
    if (index < metaData_.size()) {
        return &metaData_[index];
    }

    return nullptr;
}

array_view<const Property> CustomPropertyPodContainer::MetaData() const
{
    return { metaData_ };
}

uint64_t CustomPropertyPodContainer::Type() const
{
    return 0;
}

IPropertyHandle* CustomPropertyPodContainer::Create() const
{
    return nullptr;
}

IPropertyHandle* CustomPropertyPodContainer::Clone(const IPropertyHandle* src) const
{
    return nullptr;
}

void CustomPropertyPodContainer::Release(IPropertyHandle* handle) const {}

void CustomPropertyPodContainer::Reset()
{
    metaStrings_.clear();
    metaData_.clear();
    data_.clear();
}

void CustomPropertyPodContainer::ReservePropertyCount(size_t propertyCount)
{
    reservePropertyCount_ = propertyCount;
    metaStrings_.reserve(reservePropertyCount_);
    metaData_.reserve(reservePropertyCount_);
}

// CustomProperties IPropertyHandle
const IPropertyApi* CustomPropertyPodContainer::Owner() const
{
    return this;
}

size_t CustomPropertyPodContainer::Size() const
{
    return data_.size();
}

const void* CustomPropertyPodContainer::RLock() const
{
    return data_.data();
}

void CustomPropertyPodContainer::RUnlock() const {}

void* CustomPropertyPodContainer::WLock()
{
    return data_.data();
}

void CustomPropertyPodContainer::WUnlock() {}

//

void CustomPropertyPodContainer::AddOffsetProperty(const string_view propertyName, const string_view displayName,
    const uintptr_t offset, const PropertyTypeDecl& typeDecl)
{
    const size_t byteSize = GetPropertyTypeByteSize(typeDecl);
    const bool reserved = (metaStrings_.size() < reservePropertyCount_);
    if ((byteSize > 0) && reserved) {
        metaStrings_.push_back({ string { propertyName }, string { displayName } });
        const auto& strings = metaStrings_.back();
        const Property meta {
            strings.name,                                        // name
            FNV1aHash(strings.name.data(), strings.name.size()), // hash
            typeDecl,                                            // type
            1U,                                                  // count
            byteSize,                                            // size
            offset,                                              // offset
            strings.displayName,                                 // displayName
            0U,                                                  // flags
            GetMetaData(typeDecl),                               // metaData
        };
        metaData_.push_back(meta);
        data_.resize(Math::max(data_.size(), meta.offset + meta.size));
    } else {
        CORE_LOG_W("unsupported property addition for custom property POD container");
    }
}

void CustomPropertyPodContainer::AddOffsetProperty(const string_view propertyName, const string_view displayName,
    const uintptr_t offset, const PropertyTypeDecl& typeDecl, const array_view<const uint8_t> data)
{
    const size_t byteSize = GetPropertyTypeByteSize(typeDecl);
    const bool reserved = (metaStrings_.size() < reservePropertyCount_);
    if ((byteSize > 0) && reserved) {
        metaStrings_.push_back({ string { propertyName }, string { displayName } });
        const auto& strings = metaStrings_.back();
        const Property meta {
            strings.name,                                        // name
            FNV1aHash(strings.name.data(), strings.name.size()), // hash
            typeDecl,                                            // type
            1U,                                                  // count
            byteSize,                                            // size
            offset,                                              // offset
            strings.displayName,                                 // displayName
            0U,                                                  // flags
            GetMetaData(typeDecl),                               // metaData
        };
        metaData_.push_back(meta);
        data_.resize(data_.size() + meta.size);
        if (data.size_bytes() == byteSize) {
            CloneData(data_.data() + offset, data_.size_in_bytes(), data.data(), data.size_bytes());
        }
    } else {
        CORE_LOG_W("unsupported property addition for custom property POD container");
    }
}

bool CustomPropertyPodContainer::SetValue(const string_view propertyName, const array_view<uint8_t> data)
{
    for (const auto& metaRef : metaData_) {
        if ((metaRef.displayName == propertyName) && (metaRef.size == data.size_bytes())) {
            const size_t endData = metaRef.offset + metaRef.size;
            if (endData <= data_.size_in_bytes()) {
                CloneData(data_.data() + metaRef.offset, data_.size_in_bytes(), data.data(), data.size_bytes());
                return true;
            }
        }
    }
    return false;
}

bool CustomPropertyPodContainer::SetValue(const size_t byteOffset, const array_view<uint8_t> data)
{
    const size_t endData = byteOffset + data.size_bytes();
    if (endData <= data_.size_in_bytes()) {
        CloneData(data_.data() + byteOffset, data_.size_in_bytes(), data.data(), data.size_bytes());
        return true;
    }
    return false;
}

array_view<const uint8_t> CustomPropertyPodContainer::GetValue(const string_view propertyName) const
{
    for (const auto& metaRef : metaData_) {
        if (metaRef.displayName == propertyName) {
            const size_t endData = metaRef.offset + metaRef.size;
            if (endData <= data_.size_in_bytes()) {
                return { data_.data() + metaRef.offset, metaRef.size };
            }
        }
    }
    return {};
}

size_t CustomPropertyPodContainer::GetByteSize() const
{
    return data_.size_in_bytes();
}

BASE_NS::array_view<const uint8_t> CustomPropertyPodContainer::GetData() const
{
    return data_;
}

//

PropertyTypeDecl CustomPropertyPodHelper::GetPropertyTypeDeclaration(const string_view type)
{
    if (type == "vec4") {
        return PropertyType::VEC4_T;
    } else if (type == "uvec4") {
        return PropertyType::UVEC4_T;
    } else if (type == "ivec4") {
        return PropertyType::IVEC4_T;
    } else if (type == "vec3") {
        return PropertyType::VEC3_T;
    } else if (type == "uvec3") {
        return PropertyType::UVEC3_T;
    } else if (type == "ivec3") {
        return PropertyType::IVEC3_T;
    } else if (type == "vec2") {
        return PropertyType::VEC2_T;
    } else if (type == "uvec2") {
        return PropertyType::UVEC2_T;
    } else if (type == "ivec2") {
        return PropertyType::IVEC2_T;
    } else if (type == "float") {
        return PropertyType::FLOAT_T;
    } else if (type == "uint") {
        return PropertyType::UINT32_T;
    } else if (type == "int") {
        return PropertyType::INT32_T;
    } else if (type == "bool") {
        return PropertyType::BOOL_T;
    } else if (type == "mat3x3") {
        return PropertyType::MAT3X3_T;
    } else if (type == "mat4x4") {
        return PropertyType::MAT4X4_T;
    } else {
        CORE_LOG_W("RENDER_VALIDATION: Invalid property type only int, uint, float, bool, and XvecX variants, and "
                   "mat3x3 and mat4x4 are supported");
    }
    // NOTE: does not handle invalid types
    return PropertyType::INVALID;
}

size_t CustomPropertyPodHelper::GetPropertyTypeAlignment(const PropertyTypeDecl& propertyType)
{
    size_t align = 1U;
    static_assert(sizeof(float) == sizeof(uint32_t) && sizeof(float) == sizeof(int32_t));
    switch (propertyType) {
        case PropertyType::FLOAT_T:
            [[fallthrough]];
        case PropertyType::UINT32_T:
            [[fallthrough]];
        case PropertyType::INT32_T:
            [[fallthrough]];
        case PropertyType::BOOL_T:
            align = sizeof(float);
            break;
        case PropertyType::VEC2_T:
            [[fallthrough]];
        case PropertyType::UVEC2_T:
            [[fallthrough]];
        case PropertyType::IVEC2_T:
            align = sizeof(float) * 2U;
            break;
        case PropertyType::VEC3_T:
            [[fallthrough]];
        case PropertyType::UVEC3_T:
            [[fallthrough]];
        case PropertyType::IVEC3_T:
            [[fallthrough]];
        case PropertyType::VEC4_T:
            [[fallthrough]];
        case PropertyType::UVEC4_T:
            [[fallthrough]];
        case PropertyType::IVEC4_T:
            [[fallthrough]];
        case PropertyType::MAT3X3_T:
            [[fallthrough]];
        case PropertyType::MAT4X4_T:
            align = sizeof(float) * 4U;
            break;
    }
    return align;
}

template<typename T>
inline void SafeFromJsonValue(const json::value* value, T& val)
{
    if (value) {
        FromJson(*value, val);
    }
}

void CustomPropertyPodHelper::SetCustomPropertyBlobValue(const PropertyTypeDecl& propertyType, const json::value* value,
    CustomPropertyPodContainer& customProperties, const size_t offset)
{
    if (propertyType == PropertyType::VEC4_T) {
        Math::Vec4 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec4) });
    } else if (propertyType == PropertyType::UVEC4_T) {
        Math::UVec4 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec4) });
    } else if (propertyType == PropertyType::IVEC4_T) {
        Math::IVec4 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec4) });
    } else if (propertyType == PropertyType::VEC3_T) {
        Math::Vec3 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec3) });
    } else if (propertyType == PropertyType::UVEC3_T) {
        Math::UVec3 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec3) });
    } else if (propertyType == PropertyType::IVEC3_T) {
        Math::IVec3 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec3) });
    } else if (propertyType == PropertyType::VEC2_T) {
        Math::Vec2 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec2) });
    } else if (propertyType == PropertyType::UVEC2_T) {
        Math::UVec2 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec2) });
    } else if (propertyType == PropertyType::IVEC2_T) {
        Math::IVec2 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Vec2) });
    } else if (propertyType == PropertyType::FLOAT_T) {
        float val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(float) });
    } else if (propertyType == PropertyType::UINT32_T) {
        uint32_t val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(float) });
    } else if (propertyType == PropertyType::INT32_T) {
        int32_t val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(float) });
    } else if (propertyType == PropertyType::BOOL_T) {
        bool tmpVal;
        SafeFromJsonValue(value, tmpVal);
        uint32_t val = tmpVal;
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(float) });
    } else if (propertyType == PropertyType::MAT3X3_T) {
        Math::Mat3X3 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Mat3X3) });
    } else if (propertyType == PropertyType::MAT4X4_T) {
        Math::Mat4X4 val;
        SafeFromJsonValue(value, val);
        customProperties.SetValue(offset, array_view { reinterpret_cast<uint8_t*>(&val), sizeof(Math::Mat4X4) });
    } else {
        CORE_LOG_W("RENDER_VALIDATION: Invalid property type only int, uint, float, and XvecX variants supported");
    }
    // NOTE: does not handle invalid types
}

// bindings

template<typename T>
inline void DestroyHelper(T& t)
{
    {
        t.~T();
    }
}

CustomPropertyBindingContainer::CustomPropertyBindingContainer(CustomPropertyWriteSignal& writeSignal)
    : writeSignal_(writeSignal)
{}

CustomPropertyBindingContainer::~CustomPropertyBindingContainer()
{
    if (!data_.empty()) {
        PLUGIN_ASSERT(metaData_.size() <= data_.size());
        for (const auto& meta : metaData_) {
            CORE_ASSERT(meta.offset < data_.size_in_bytes());
            switch (meta.type) {
                case PropertyType::BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T: {
                    PLUGIN_ASSERT(meta.size == BUFFER_HANDLE_REF_BYTE_SIZE);
                    if (auto* resource = (BindableBufferWithHandleReference*)(data_.data() + meta.offset); resource) {
                        DestroyHelper(*resource);
                    }
                    break;
                }
                case PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T: {
                    PLUGIN_ASSERT(meta.size == IMAGE_HANDLE_REF_BYTE_SIZE);
                    if (auto* resource = (BindableImageWithHandleReference*)(data_.data() + meta.offset); resource) {
                        DestroyHelper(*resource);
                    }
                    break;
                }
                case PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T: {
                    PLUGIN_ASSERT(meta.size == SAMPLER_HANDLE_REF_BYTE_SIZE);
                    if (auto* resource = (BindableSamplerWithHandleReference*)(data_.data() + meta.offset); resource) {
                        DestroyHelper(*resource);
                    }
                    break;
                }
                default: {
                    CORE_LOG_E("custom property binding destruction error");
                    break;
                }
            }
        }
    }
}

size_t CustomPropertyBindingContainer::PropertyCount() const
{
    return metaData_.size();
}

const Property* CustomPropertyBindingContainer::MetaData(size_t index) const
{
    if (index < metaData_.size()) {
        return &metaData_[index];
    }

    return nullptr;
}

array_view<const Property> CustomPropertyBindingContainer::MetaData() const
{
    return { metaData_ };
}

uint64_t CustomPropertyBindingContainer::Type() const
{
    return 0;
}

IPropertyHandle* CustomPropertyBindingContainer::Create() const
{
    return nullptr;
}

IPropertyHandle* CustomPropertyBindingContainer::Clone(const IPropertyHandle* src) const
{
    return nullptr;
}

void CustomPropertyBindingContainer::Release(IPropertyHandle* handle) const {}

void CustomPropertyBindingContainer::ReservePropertyCount(size_t propertyCount)
{
    reservePropertyCount_ = propertyCount;
    metaStrings_.reserve(reservePropertyCount_);
    metaData_.reserve(reservePropertyCount_);
    // max size
    data_.reserve(reservePropertyCount_ * MAX_STRUCT_HANDLE_REF_BYTE_SIZE);
}

// CustomProperties IPropertyHandle
const IPropertyApi* CustomPropertyBindingContainer::Owner() const
{
    return this;
}

size_t CustomPropertyBindingContainer::Size() const
{
    return data_.size();
}

const void* CustomPropertyBindingContainer::RLock() const
{
    return data_.data();
}

void CustomPropertyBindingContainer::RUnlock() const {}

void* CustomPropertyBindingContainer::WLock()
{
    return data_.data();
}

void CustomPropertyBindingContainer::WUnlock()
{
    // signal that properties have been written
    writeSignal_.Signal();
}

//

void CustomPropertyBindingContainer::AddOffsetProperty(const string_view propertyName, const string_view displayName,
    const uintptr_t offset, const PropertyTypeDecl& typeDecl)
{
    const bool reserved = (metaStrings_.size() < reservePropertyCount_);
    size_t byteSize = 0;
    switch (typeDecl) {
        case PropertyType::BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T: {
            byteSize = BUFFER_HANDLE_REF_BYTE_SIZE;
            break;
        }
        case PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T: {
            byteSize = IMAGE_HANDLE_REF_BYTE_SIZE;
            break;
        }
        case PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T: {
            byteSize = SAMPLER_HANDLE_REF_BYTE_SIZE;
            break;
        }
    }
    if ((byteSize > 0) && reserved) {
        metaStrings_.push_back({ string { propertyName }, string { displayName } });
        const auto& strings = metaStrings_.back();
        const Property meta {
            strings.name,                                        // name
            FNV1aHash(strings.name.data(), strings.name.size()), // hash
            typeDecl,                                            // type
            1U,                                                  // count
            byteSize,                                            // size
            offset,                                              // offset
            strings.displayName,                                 // displayName
            0U,                                                  // flags
            GetMetaData(typeDecl),                               // metaData
        };
        metaData_.push_back(meta);
        // the data has already been reserved in ReservePropertyCount()
        data_.resize(Math::max(data_.size(), meta.offset + meta.size));
        switch (meta.type) {
            case PropertyType::BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T: {
                new (data_.data() + meta.offset) BindableBufferWithHandleReference;
                break;
            }
            case PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T: {
                new (data_.data() + meta.offset) BindableImageWithHandleReference;
                break;
            }
            case PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T: {
                new (data_.data() + meta.offset) BindableSamplerWithHandleReference;
                break;
            }
        }
    } else {
        CORE_LOG_W("unsupported property addition for custom property binding container");
    }
}

size_t CustomPropertyBindingContainer::GetByteSize() const
{
    return data_.size_in_bytes();
}

// CustomPropertyBindingHelper

PropertyTypeDecl CustomPropertyBindingHelper::GetPropertyTypeDeclaration(const string_view type)
{
    if (type == "buffer") {
        return PropertyType::BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T;
    } else if (type == "image") {
        return PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T;
    } else if (type == "sampler") {
        return PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T;
    } else {
        CORE_LOG_W("RENDER_VALIDATION: Invalid property type only buffer, image, and sampler supported (not %s)",
            string(type).c_str());
    }
    // NOTE: does not handle invalid types
    return PropertyType::INVALID;
}

size_t CustomPropertyBindingHelper::GetPropertyTypeAlignment(const PropertyTypeDecl& propertyType)
{
    size_t align = 1U;
    switch (propertyType) {
        case PropertyType::BINDABLE_BUFFER_WITH_HANDLE_REFERENCE_T:
            align = sizeof(BindableBufferWithHandleReference);
            break;
        case PropertyType::BINDABLE_IMAGE_WITH_HANDLE_REFERENCE_T:
            align = sizeof(BindableImageWithHandleReference);
            break;
        case PropertyType::BINDABLE_SAMPLER_WITH_HANDLE_REFERENCE_T:
            align = sizeof(BindableSamplerWithHandleReference);
            break;
    }
    return align;
}
RENDER_END_NAMESPACE()
