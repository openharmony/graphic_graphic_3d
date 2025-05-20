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

#ifndef API_CORE_PROPERTY_IPROPERTY_HANDLE_UTIL_H
#define API_CORE_PROPERTY_IPROPERTY_HANDLE_UTIL_H

#include <algorithm>
#include <climits>
#include <cstdlib>

#include <base/containers/string_view.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property.h>
#include <core/property/property_types.h>
#include <core/property/scoped_handle.h>
#include <core/property_tools/property_data.h>

CORE_BEGIN_NAMESPACE()
template<typename T, typename PropertyHandle>
ScopedHandle<T> MakeScopedHandle(
    PropertyHandle& handle, BASE_NS::string_view propertyName, const PropertyTypeDecl& propertyType)
{
    const auto baseAddress = reinterpret_cast<uintptr_t>(handle.RLock());
    if (baseAddress) {
        const auto result = PropertyData::FindProperty(handle.Owner()->MetaData(), propertyName, baseAddress);
        handle.RUnlock();
        if (result && (result.property->type == propertyType)) {
            auto scoped = ScopedHandle<T>(handle);
            if (scoped) {
                scoped.data_ = reinterpret_cast<decltype(scoped.data_)>(result.offset);
                return scoped;
            }
        }
    }
    return {};
}

template<typename T>
bool SetPropertyValue(
    IPropertyHandle& handle, BASE_NS::string_view propertyName, const PropertyTypeDecl& propertyType, T&& propertyValue)
{
    using BaseT = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<T>>;
    if (auto scoped = MakeScopedHandle<BaseT>(handle, propertyName, propertyType); scoped) {
        *scoped = BASE_NS::forward<T>(propertyValue);
        return true;
    }
    return false;
}

template<typename T>
T GetPropertyValue(
    const IPropertyHandle& handle, BASE_NS::string_view propertyName, const PropertyTypeDecl& propertyType)
{
    using BaseT = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<T>>;
    if (auto scoped = MakeScopedHandle<const BaseT>(handle, propertyName, propertyType); scoped) {
        return *scoped;
    }
    return {};
}

/** Create a ScopedHandle to read or write a property value using an offset.
 * @param handle Pointer to a handle.
 * @param offset Byte offset to the property value form the begining of the handles RLock/ Wlock address.
 * @return ScopedHandle, which may be invalid locking the handle failed.
 */
template<typename T, typename PropertyHandle>
ScopedHandle<T> MakeScopedHandle(PropertyHandle* handle, uintptr_t offset)
{
    auto scoped = ScopedHandle<T>(handle);
    if (scoped) {
        scoped.data_ = reinterpret_cast<decltype(scoped.data_)>(reinterpret_cast<uintptr_t>(scoped.data_) + offset);
    }
    return scoped;
}

/** Create a ScopedHandle to read or write a property value using an offset.
 * @param handle Handle.
 * @param offset Byte offset to the property value form the begining of the handles RLock/ Wlock address.
 * @return ScopedHandle, which may be invalid locking the handle failed.
 */
template<typename T, typename PropertyHandle>
ScopedHandle<T> MakeScopedHandle(PropertyHandle& handle, uintptr_t offset)
{
    auto scoped = ScopedHandle<T>(handle);
    if (scoped) {
        scoped.data_ = reinterpret_cast<decltype(scoped.data_)>(reinterpret_cast<uintptr_t>(scoped.data_) + offset);
    }
    return scoped;
}

/** Create a ScopedHandle to read or write a named property value.
 * @param handle Handle.
 * @param propertyName Name of the property to set.
 * @return ScopedHandle, which may be invalid if name and type did not match any property.
 */
template<typename T, typename PropertyHandle>
ScopedHandle<T> MakeScopedHandle(PropertyHandle& handle, BASE_NS::string_view propertyName)
{
    using BaseT = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<BASE_NS::remove_extent_t<T>>>;
    static constexpr auto propertyType = PropertySystem::PropertyTypeDeclFromType<BaseT, BASE_NS::is_array_t<T>>();
    return MakeScopedHandle<T>(handle, propertyName, propertyType);
}

/** Create a ScopedHandle to read or write a named property value.
 * @param handle Pointer to a handle.
 * @param propertyName Name of the property to set.
 * @return ScopedHandle, which may be invalid if name and type did not match any property.
 */
template<typename T, typename PropertyHandle>
ScopedHandle<T> MakeScopedHandle(PropertyHandle* handle, BASE_NS::string_view propertyName)
{
    if (handle) {
        return MakeScopedHandle<T>(*handle, propertyName);
    }
    return {};
}

/** Set a named property value.
 * @param handle Pointer to a handle.
 * @param propertyName Name of the property to set.
 * @param propertyValue Value to assign to the property.
 * @return True name and type matched a property, otherwise false.
 */
template<typename T>
bool SetPropertyValue(IPropertyHandle& handle, BASE_NS::string_view propertyName, T&& propertyValue)
{
    using BaseT = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<BASE_NS::remove_extent_t<T>>>;
    static constexpr auto propertyType = PropertySystem::PropertyTypeDeclFromType<BaseT, BASE_NS::is_array_t<T>>();
    return SetPropertyValue(handle, propertyName, propertyType, BASE_NS::forward<T>(propertyValue));
}

/** Set a named property value.
 * @param handle Handle.
 * @param propertyName Name of the property to set.
 * @param propertyValue Value to assign to the property.
 * @return True name and type matched a property, otherwise false.
 */
template<typename T>
bool SetPropertyValue(IPropertyHandle* handle, BASE_NS::string_view propertyName, T&& propertyValue)
{
    if (handle) {
        return SetPropertyValue(*handle, propertyName, BASE_NS::forward<T>(propertyValue));
    }
    return false;
}

/** Query the value of a named property.
 * @param handle Pointer to handle.
 * @param propertyName Name of the property to query.
 * @return Value of the property or a default constructed value if name and type did not match any property.
 */
template<typename T>
T GetPropertyValue(const IPropertyHandle& handle, BASE_NS::string_view propertyName)
{
    static constexpr auto propertyType = PropertySystem::PropertyTypeDeclFromType<T, BASE_NS::is_array_t<T>>();
    return GetPropertyValue<T>(handle, propertyName, propertyType);
}

/** Query the value of a named property.
 * @param handle Handle.
 * @param propertyName Name of the property to query.
 * @return Value of the property or a default constructed value if name and type did not match any property.
 */
template<typename T>
T GetPropertyValue(const IPropertyHandle* handle, BASE_NS::string_view propertyName)
{
    if (handle) {
        return GetPropertyValue<T>(*handle, propertyName);
    }
    return {};
}
CORE_END_NAMESPACE()
#endif // API_CORE_PROPERTY_IPROPERTY_HANDLE_UTIL_H
