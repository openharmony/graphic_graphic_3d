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

#ifndef CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_DATA_H
#define CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_DATA_H

#include <PropertyTools/property_value.h>
#include <cstddef>

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <core/property/intf_property_handle.h>

CORE_BEGIN_NAMESPACE()
class IPropertyApi;
class PropertyData : public IPropertyHandle {
public:
    PropertyData();

    PropertyData(const PropertyData& other) = delete;
    PropertyData(PropertyData&& other) = delete;
    PropertyData& operator=(const PropertyData& other) = delete;
    PropertyData& operator=(PropertyData&& other) = delete;

    struct PropertyOffset {
        constexpr operator bool() const noexcept
        {
            return (property);
        }
        const Property* property { nullptr };
        uintptr_t offset { 0U };
        size_t index { 0 };
    };

    bool WLock(IPropertyHandle& data);         // no-copy direct-access (Locks the datahandle);
    bool WUnlock(const IPropertyHandle& data); // (releases the datahandle lock, and removes ref)
    bool RLock(const IPropertyHandle& data);   // no-copy direct-access (Locks the datahandle);
    bool RUnlock(const IPropertyHandle& data); // (releases the datahandle lock, and removes ref)

    /**  no-copy direct write access
     * Searches for property and locks the data
     * if property was found, returns valid PropertyOffset and keeps it locked
     * if not found, releases the lock immediately and returns null PropertyOffset
     */
    PropertyOffset WLock(IPropertyHandle& data, BASE_NS::string_view propertyPath);

    /**  no-copy direct read access
     * Searches for property and locks the data
     * if property was found, returns valid PropertyOffset and keeps it locked
     * if not found, releases the lock immediately and returns null PropertyOffset
     */
    PropertyOffset RLock(const IPropertyHandle& data, BASE_NS::string_view propertyPath);

    /** Find offset and property type for a property
     * @param properties Property metadata to search from
     * @param propertyPath property path to search for
     * @param baseOffset offset to (already locked) property data
     * @return PropertyOffset-structure. If property was not found, returns null PropertyOffset
     */
    static PropertyOffset FindProperty(
        BASE_NS::array_view<const Property> properties, BASE_NS::string_view propertyPath, uintptr_t baseOffset);

    virtual ~PropertyData();
    size_t PropertyCount() const;

    BASE_NS::array_view<const Property> MetaData() const;
    const Property* MetaData(size_t index) const;

    // deprecated accessors..  (PropertyValue is deprecated)
    PropertyValue Get(size_t index);
    PropertyValue Get(size_t index) const;

    PropertyValue Get(BASE_NS::string_view name);
    PropertyValue Get(BASE_NS::string_view name) const;

    PropertyValue operator[](size_t index);
    PropertyValue operator[](size_t index) const;

    PropertyValue operator[](const BASE_NS::string_view& name);
    PropertyValue operator[](const BASE_NS::string_view& name) const;

    // Implement the IPropertyHandle api.
    const IPropertyApi* Owner() const override;
    size_t Size() const override;
    const void* RLock() const override;
    void RUnlock() const override;
    void* WLock() override;
    void WUnlock() override;

protected:
    void Reset();
    const IPropertyApi* owner_;
    size_t size_;
    const void* data_;
    uint8_t* dataW_;
    const IPropertyHandle* dataHandle_;
    IPropertyHandle* dataHandleW_;
};
CORE_END_NAMESPACE()

#endif // CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_DATA_H
