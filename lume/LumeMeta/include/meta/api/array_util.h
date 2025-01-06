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

#ifndef META_API_ARRAY_UTIL_H
#define META_API_ARRAY_UTIL_H

#include <meta/api/util.h>
#include <meta/interface/property/array_property.h>

META_BEGIN_NAMESPACE()

/// Get shared_ptr of IInterface from an array any in given location, or nullptr if not compatible
inline BASE_NS::shared_ptr<CORE_NS::IInterface> GetPointerAt(IArrayAny::IndexType index, const IProperty::ConstPtr& p)
{
    ArrayPropertyLock lock { p };
    return lock ? GetPointer(lock->GetAnyAt(index)) : nullptr;
}

/// Get interface pointer from an array any in given location, or nullptr if not compatible
template<typename Interface>
inline BASE_NS::shared_ptr<Interface> GetPointerAt(IArrayAny::IndexType index, const IProperty::ConstPtr& p)
{
    return interface_pointer_cast<Interface>(GetPointerAt(index, p));
}
/// Check if property is an array property
inline bool IsArrayProperty(const IProperty::ConstPtr& p)
{
    return IsArray(GetInternalAny(p));
}

/// Check if property is an array property
inline bool IsArrayProperty(const IProperty& p)
{
    return IsArray(GetInternalAny(p));
}

/// Check if array property item is compatible with given type
inline bool IsItemCompatible(
    const IProperty& p, const TypeId& id, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    if (auto arr = interface_cast<IArrayAny>(GetInternalAny(p))) {
        return IsItemCompatible(*arr, id, dir);
    }
    return false;
}

/// Check if array property item is compatible with given type
inline bool IsItemCompatible(
    const IProperty::ConstPtr& p, const TypeId& id, CompatibilityDirection dir = CompatibilityDirection::BOTH)
{
    return p && IsItemCompatible(*p, id, dir);
}

META_END_NAMESPACE()

#endif
