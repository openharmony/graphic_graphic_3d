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

#ifndef META_INTERFACE_PROPERTY_CONSTRUCT_ARRAY_PROPERTY_H
#define META_INTERFACE_PROPERTY_CONSTRUCT_ARRAY_PROPERTY_H

#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/array_property.h>

META_BEGIN_NAMESPACE()

template<typename T>
ArrayProperty<T> ConstructArrayProperty(IObjectRegistry& obr, BASE_NS::string_view name,
    BASE_NS::array_view<const T> value = {}, ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    ArrayProperty<T> p(NOCHECK, obr.GetPropertyRegister().Create(ClassId::StackProperty, name));
    if (auto i = interface_cast<IPropertyInternalAny>(p.GetProperty())) {
        i->SetInternalAny(ConstructArrayAny<T>(BASE_NS::move(value)));
    }
    SetObjectFlags(p.GetProperty(), flags, true);
    return p;
}

template<typename T>
ArrayProperty<T> ConstructArrayProperty(BASE_NS::string_view name, BASE_NS::array_view<const T> value = {},
    ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    return ConstructArrayProperty(GetObjectRegistry(), name, BASE_NS::move(value), flags);
}

template<typename T>
ArrayProperty<T> ConstructArrayPropertyAny(
    BASE_NS::string_view name, const IAny& value, ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    auto p = ConstructArrayProperty<T>(name, {}, flags);
    if (p) {
        p.GetUnlockedAccess().SetDefaultValueAny(value);
    }
    return p;
}

// helpers to handle default values with Value Ptr stuff
template<typename T>
IAny::Ptr ConstructArrayAnyHelper(BASE_NS::vector<T> value)
{
    return ConstructArrayAny(BASE_NS::move(value));
}
template<typename T, typename Param = typename T::TypePtr, typename = typename T::TypePtr>
IAny::Ptr ConstructArrayAnyHelper(BASE_NS::vector<Param> value)
{
    return ConstructArrayAny(BASE_NS::move(value));
}

META_END_NAMESPACE()

#endif
