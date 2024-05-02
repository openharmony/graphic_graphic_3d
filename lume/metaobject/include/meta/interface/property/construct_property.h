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

#ifndef META_INTERFACE_PROPERTY_CONSTRUCT_PROPERTY_H
#define META_INTERFACE_PROPERTY_CONSTRUCT_PROPERTY_H

#include <meta/interface/intf_any.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/property.h>

META_BEGIN_NAMESPACE()

struct ValuePtrBase {};

template<typename Type, const META_NS::ClassInfo& ClassInfo>
struct ValuePtrInstance : ValuePtrBase {
    static constexpr ObjectId ID = ClassInfo.Id();
    using TypePtr = typename Type::Ptr;
    static_assert(IsInterfacePtr_v<TypePtr>, "Invalid type for ValuePtr");
};

template<typename Type>
struct ValuePtrImpl {
    template<const META_NS::ClassInfo& ClassInfo>
    using Instance = ValuePtrInstance<Type, ClassInfo>;
};
template<typename Type, const META_NS::ClassInfo& ClassInfo>
using ValuePtr = ValuePtrInstance<Type, ClassInfo>;

template<typename T>
struct PropertyType {
    using Type = T;
};
template<typename T, const META_NS::ClassInfo& ClassInfo>
struct PropertyType<ValuePtr<T, ClassInfo>> {
    using Type = typename ValuePtr<T, ClassInfo>::TypePtr;
};
template<typename T>
using PropertyType_v = typename PropertyType<T>::Type; // NOLINT(readability-identifier-naming)

template<typename T>
Property<T> ConstructProperty(IObjectRegistry& obr, BASE_NS::string_view name, const T& value = {},
    ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    Property<T> p(NOCHECK, obr.GetPropertyRegister().Create(ClassId::StackProperty, name));
    if (auto i = interface_cast<IPropertyInternalAny>(p.GetProperty())) {
        i->SetInternalAny(IAny::Ptr(new Any<T>));
    }
    auto access = p.GetUnlockedAccess();
    access.SetDefaultValue(value);
    if (auto f = interface_cast<IObjectFlags>(p.GetProperty())) {
        f->SetObjectFlags(flags);
    }
    return p;
}

template<typename T, typename = BASE_NS::enable_if_t<!BASE_NS::is_convertible_v<T*, ValuePtrBase*>>>
Property<T> ConstructProperty(
    BASE_NS::string_view name, const T& value = {}, ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    return ConstructProperty(GetObjectRegistry(), name, value, flags);
}

template<typename T, typename Param = typename T::TypePtr>
Property<typename T::TypePtr> ConstructProperty(
    BASE_NS::string_view name, const Param& value = {}, ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    auto& obr = GetObjectRegistry();
    Property<typename T::TypePtr> p(NOCHECK, obr.GetPropertyRegister().Create(ClassId::StackProperty, name));
    if (auto i = interface_cast<IPropertyInternalAny>(p.GetProperty())) {
        auto obj = obr.Create<IAny>(T::ID);
        if (!obj) {
            CORE_LOG_E("invalid class id for ValuePtr");
            return nullptr;
        }
        i->SetInternalAny(obj);
    }
    auto access = p.GetUnlockedAccess();
    access.SetDefaultValueAny(Any<Param>(value));
    if (auto f = interface_cast<IObjectFlags>(p.GetProperty())) {
        f->SetObjectFlags(flags);
    }
    return p;
}

inline IProperty::Ptr ConstructPropertyAny(
    BASE_NS::string_view name, const IAny& value, ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    auto p = GetObjectRegistry().GetPropertyRegister().Create(ClassId::StackProperty, name);
    if (auto i = interface_cast<IPropertyInternalAny>(p)) {
        i->SetInternalAny(value.Clone(true));
    }
    if (auto f = interface_cast<IObjectFlags>(p)) {
        f->SetObjectFlags(flags);
    }
    return p;
}

META_END_NAMESPACE()

#endif
