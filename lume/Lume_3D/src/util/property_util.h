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

#ifndef CORE3D_UTIL_PROPERTY_UTIL_H
#define CORE3D_UTIL_PROPERTY_UTIL_H

#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <core/json/json.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property_handle_util.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
/*
 * Implements a custom property blob for simple PODs e.g. shader custom shader properties which are stored in a single
 * byte data.
 * Supports float -> vec4 with u/i -variants
 * - Single byte data blob
 * - Multiple offsets customized properties for that data
 * - Can only hold simple data within a single byte-blob
 *
 * NOTE: One must use Reserve(propertyCount) before adding properties
 */
class CustomPropertyPodContainer final : public CORE_NS::IPropertyHandle, CORE_NS::IPropertyApi {
public:
    CustomPropertyPodContainer() = default;
    /* Reserve a predefined byte size to hold the property data */
    explicit CustomPropertyPodContainer(size_t reserveByteSize);

    ~CustomPropertyPodContainer() override = default;

    CustomPropertyPodContainer(const CustomPropertyPodContainer& other) = delete;
    CustomPropertyPodContainer(CustomPropertyPodContainer&& other) = delete;
    CustomPropertyPodContainer& operator=(const CustomPropertyPodContainer& other) = delete;
    CustomPropertyPodContainer& operator=(CustomPropertyPodContainer&& other) = delete;

    // IPropertyHandle
    const CORE_NS::IPropertyApi* Owner() const override;
    size_t Size() const override;
    const void* RLock() const override;
    void RUnlock() const override;
    void* WLock() override;
    void WUnlock() override;

    // IPropertyApi
    size_t PropertyCount() const override;
    const CORE_NS::Property* MetaData(size_t index) const override;
    BASE_NS::array_view<const CORE_NS::Property> MetaData() const override;
    uint64_t Type() const override;
    CORE_NS::IPropertyHandle* Create() const override;
    CORE_NS::IPropertyHandle* Clone(const CORE_NS::IPropertyHandle* src) const override;
    void Release(CORE_NS::IPropertyHandle* handle) const override;

    // Reset, to remove old properties
    void Reset();
    // Reserve before adding any properties
    void ReservePropertyCount(size_t propertyCount);

    void AddOffsetProperty(BASE_NS::string_view propertyName, BASE_NS::string_view displayName, uintptr_t offset,
        const CORE_NS::PropertyTypeDecl& typeDecl);
    void AddOffsetProperty(BASE_NS::string_view propertyName, BASE_NS::string_view displayName, uintptr_t offset,
        const CORE_NS::PropertyTypeDecl& typeDecl, BASE_NS::array_view<const uint8_t> data);
    bool SetValue(BASE_NS::string_view propertyName, BASE_NS::array_view<const uint8_t> data);
    bool SetValue(size_t byteOffset, BASE_NS::array_view<const uint8_t> data);
    BASE_NS::array_view<const uint8_t> GetValue(BASE_NS::string_view name) const;
    // byte size of the data container
    size_t GetByteSize() const;

    void CopyValues(const CustomPropertyPodContainer& other);

    // Uses property handle util
    template<typename T>
    bool SetValue(BASE_NS::string_view propertyName, T&& propertyValue)
    {
        return CORE_NS::SetPropertyValue(*this, propertyName, BASE_NS::forward<T>(propertyValue));
    }
    template<typename T>
    T GetValue(BASE_NS::string_view propertyName)
    {
        return CORE_NS::GetPropertyValue<T>(*this, propertyName);
    }

private:
    struct Strings {
        BASE_NS::string name;
        BASE_NS::string displayName;
    };
    BASE_NS::vector<Strings> metaStrings_;
    BASE_NS::vector<CORE_NS::Property> metaData_;
    BASE_NS::vector<uint8_t> data_;

    size_t reservePropertyCount_ { 0 };
};

/*
 * CustomPropertyDataHelper
 */
class CustomPropertyPodHelper final {
public:
    CustomPropertyPodHelper() = default;
    ~CustomPropertyPodHelper() = default;

    static CORE_NS::PropertyTypeDecl GetPropertyTypeDeclaration(const BASE_NS::string_view type);
    static size_t GetPropertyTypeAlignment(const CORE_NS::PropertyTypeDecl& propertyType);
    static void SetCustomPropertyBlobValue(const CORE_NS::PropertyTypeDecl& propertyType,
        const CORE_NS::json::value* value, CustomPropertyPodContainer& customProperties, const size_t offset);
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_UTIL_PROPERTY_UTIL_H
