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

#ifndef CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_DATA_H
#define CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_DATA_H

#include <PropertyTools/property_value.h>
#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <core/namespace.h>
#include <core/property/intf_property_handle.h>

CORE_BEGIN_NAMESPACE()
struct Property;
class IPropertyApi;
class PropertyData : public IPropertyHandle {
public:
    PropertyData();

    PropertyData(const PropertyData& other) = delete;
    PropertyData(PropertyData&& other) = delete;
    PropertyData& operator=(const PropertyData& other) = delete;
    PropertyData& operator=(PropertyData&& other) = delete;

    struct PropertyOffset {
        constexpr explicit operator bool() const noexcept
        {
            return property != nullptr;
        }
        const Property* property { nullptr };
        uintptr_t offset { 0U };
        size_t index { 0 };
        BASE_NS::string propertyPath;
    };

    bool WLock(IPropertyHandle& handle);         // no-copy direct-access (Locks the datahandle);
    bool WUnlock(const IPropertyHandle& handle); // (releases the datahandle lock, and removes ref)
    bool RLock(const IPropertyHandle& handle);   // no-copy direct-access (Locks the datahandle);
    bool RUnlock(const IPropertyHandle& handle); // (releases the datahandle lock, and removes ref)

    /**  no-copy direct write access
     * Searches for property and locks the handle
     * if property was found, returns valid PropertyOffset and keeps it locked
     * if not found, releases the lock immediately and returns null PropertyOffset
     */
    PropertyOffset WLock(IPropertyHandle& handle, BASE_NS::string_view propertyPath);

    /**  no-copy direct read access
     * Searches for property and locks the handle
     * if property was found, returns valid PropertyOffset and keeps it locked
     * if not found, releases the lock immediately and returns null PropertyOffset
     */
    PropertyOffset RLock(const IPropertyHandle& handle, BASE_NS::string_view propertyPath);

    /** Find offset and property type for a property.
     * @param properties Property metadata to search from.
     * @param propertyPath property path to search for.
     * @param baseOffset offset to (already locked) property data.
     * @return PropertyOffset-structure. If property was not found, returns null PropertyOffset.
     */
    static PropertyOffset FindProperty(
        BASE_NS::array_view<const Property> properties, BASE_NS::string_view propertyPath, uintptr_t baseOffset);

    /** Find offset and property type for a property.
     * @param properties Property metadata to search from.
     * @param propertyPath Property path to search for.
     * @return PropertyOffset-structure. If property was not found, returns null PropertyOffset.
     */
    static PropertyOffset FindProperty(
        BASE_NS::array_view<const Property> properties, BASE_NS::string_view propertyPath);

    ~PropertyData() override;
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

private:
    const IPropertyApi* owner_ = nullptr;
    size_t size_ = 0;
    const void* data_ = nullptr;
    uint8_t* dataW_ = nullptr;
    const IPropertyHandle* dataHandle_ = nullptr;
    IPropertyHandle* dataHandleW_ = nullptr;
};
CORE_END_NAMESPACE()

#endif // CORE__ECS_HELPER__PROPERTY_TOOLS__PROPERTY_DATA_H
