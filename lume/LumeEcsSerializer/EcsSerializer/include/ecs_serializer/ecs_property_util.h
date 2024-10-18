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

#ifndef API_ECS_SERIALIZER_PROPERTYUTIL_H
#define API_ECS_SERIALIZER_PROPERTYUTIL_H

#include <PropertyTools/property_data.h>

#include <base/containers/string.h>
#include <core/property/intf_property_api.h>
#include <core/property/property_types.h>
#include <core/property/scoped_handle.h>
#include <ecs_serializer/namespace.h>

ECS_SERIALIZER_BEGIN_NAMESPACE()

inline bool IsPropertyContainer(const Property& property)
{
    return property.type == PROPERTYTYPE(IPropertyHandle*);
}

IPropertyHandle* ResolveContainerProperty(const IPropertyHandle& handle,
    const string& propertyPath, string& path, string& name)
{
    // Extract property path.
    auto separatorPosition = propertyPath.find_first_of('.');
    if (separatorPosition == string::npos) {
        return nullptr;
    }

    path = propertyPath.substr(0, separatorPosition);
    name = propertyPath.substr(separatorPosition + 1);

    IPropertyHandle* result = nullptr;

    uintptr_t offset = uintptr_t(handle.RLock());

    // Get potential container.
    auto propertyData = PropertyData::FindProperty(handle.Owner()->MetaData(), path, offset);
    if (propertyData) {
        // Ensure it is a container.
        if (IsPropertyContainer(*propertyData.property)) {
            // Try to flush value to container.
            result = *(IPropertyHandle**)(propertyData.offset);
        }
    }

    handle.RUnlock();

    return result;
}

ECS_SERIALIZER_END_NAMESPACE()

#endif // API_ECS_SERIALIZER_PROPERTYUTIL_H
