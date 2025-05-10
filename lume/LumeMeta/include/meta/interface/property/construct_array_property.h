/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Typed Property interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-11-13
 */

#ifndef META_INTERFACE_PROPERTY_CONSTRUCT_ARRAY_PROPERTY_H
#define META_INTERFACE_PROPERTY_CONSTRUCT_ARRAY_PROPERTY_H

#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/array_property.h>

META_BEGIN_NAMESPACE()

/**
 * @brief Construct array property
 * @param obr object registry instance
 * @param name Name of the property
 * @param value Default value for the array property
 * @param flags Object flags for the array property
 * @return Typed array property
 */
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
/**
 * @brief Construct array property
 * @param name Name of the property
 * @param value Default value for the array property
 * @param flags Object flags for the array property
 * @return Typed array property
 */
template<typename T>
ArrayProperty<T> ConstructArrayProperty(BASE_NS::string_view name, BASE_NS::array_view<const T> value = {},
    ObjectFlagBitsValue flags = ObjectFlagBits::SERIALIZE)
{
    return ConstructArrayProperty(GetObjectRegistry(), name, BASE_NS::move(value), flags);
}
/**
 * @brief Construct array property with IAny default value
 * @param name Name of the property
 * @param value Default value for the array property
 * @param flags Object flags for the array property
 * @return Typed array property
 */
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
