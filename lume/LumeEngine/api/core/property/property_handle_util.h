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

#ifndef API_CORE_PROPERTY_IPROPERTY_HANDLE_UTIL_H
#define API_CORE_PROPERTY_IPROPERTY_HANDLE_UTIL_H

#include <base/containers/string_view.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/property.h>
#include <core/property/property_types.h>
#include <core/property/scoped_handle.h>

CORE_BEGIN_NAMESPACE()
template<typename T>
ScopedHandle<T> MakeScopedHandle(
    IPropertyHandle& handle, BASE_NS::string_view propertyName, const PropertyTypeDecl& propertyType)
{
    for (const auto& metaData : handle.Owner()->MetaData()) {
        if (metaData.name == propertyName) {
            if (metaData.type == propertyType) {
                if (auto scoped = ScopedHandle<T>(&handle); scoped) {
                    scoped.data_ = reinterpret_cast<decltype(scoped.data_)>(
                        reinterpret_cast<uintptr_t>(scoped.data_) + metaData.offset);
                    return scoped;
                }
            }
            break;
        }
    }
    return {};
}

template<typename T>
ScopedHandle<T> MakeScopedHandle(IPropertyHandle& handle, BASE_NS::string_view propertyName)
{
    using BaseT = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<BASE_NS::remove_extent_t<T>>>;
    constexpr const auto propertyType = PropertySystem::PropertyTypeDeclFromType<BaseT, BASE_NS::is_array_t<T>>();
    return MakeScopedHandle<T>(handle, propertyName, propertyType);
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
bool SetPropertyValue(IPropertyHandle& handle, BASE_NS::string_view propertyName, T&& propertyValue)
{
    using BaseT = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<BASE_NS::remove_extent_t<T>>>;
    constexpr const auto propertyType = PropertySystem::PropertyTypeDeclFromType<BaseT, BASE_NS::is_array_t<T>>();
    return SetPropertyValue(handle, propertyName, propertyType, BASE_NS::forward<T>(propertyValue));
}

template<typename T>
T GetPropertyValue(IPropertyHandle& handle, BASE_NS::string_view propertyName, const PropertyTypeDecl& propertyType)
{
    using BaseT = BASE_NS::remove_const_t<BASE_NS::remove_reference_t<T>>;
    if (auto scoped = MakeScopedHandle<const BaseT>(handle, propertyName, propertyType); scoped) {
        return *scoped;
    }
    return {};
}

template<typename T>
T GetPropertyValue(IPropertyHandle& handle, BASE_NS::string_view propertyName)
{
    constexpr const auto propertyType = PropertySystem::PropertyTypeDeclFromType<T, BASE_NS::is_array_t<T>>();
    return GetPropertyValue<T>(handle, propertyName, propertyType);
}
CORE_END_NAMESPACE()
#endif // API_CORE_PROPERTY_IPROPERTY_HANDLE_UTIL_H
