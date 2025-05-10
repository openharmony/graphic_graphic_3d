/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Helper to register Any construction
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-15
 */

#ifndef META_EXT_ANY_BUILDER_H
#define META_EXT_ANY_BUILDER_H

#include <meta/base/meta_types.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/intf_property_register.h>

META_BEGIN_NAMESPACE()

/// The default version of any builder that is used for registering any types
template<typename AnyType>
class DefaultAnyBuilder : public AnyBuilder {
public:
    IAny::Ptr Construct() override
    {
        return IAny::Ptr(new AnyType);
    }
    ObjectId GetObjectId() const override
    {
        return AnyType::StaticGetClassId();
    }
    BASE_NS::string GetTypeName() const override
    {
        return BASE_NS::string(AnyType::StaticGetTypeIdString());
    }
};

/// Register user defined any type
template<typename AnyType>
void RegisterUserAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateShared<DefaultAnyBuilder<AnyType>>());
}

/// Register user type using built-in any type
template<typename Type>
void RegisterTypeForBuiltinAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateShared<DefaultAnyBuilder<AnyType<Type>>>());
}

/// Register user type using built-in array any type
template<typename Type>
void RegisterTypeForBuiltinArrayAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateShared<DefaultAnyBuilder<ArrayAnyType<Type>>>());
}

/// Unregister, counter part for RegisterUserAny
template<typename Type>
void UnregisterUserAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(Type::StaticGetClassId());
}

/// Unregister, counter part for RegisterTypeForBuiltinAny
template<typename Type>
void UnregisterTypeForBuiltinAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(AnyType<Type>::StaticGetClassId());
}

/// Unregister, counter part for RegisterTypeForBuiltinArrayAny
template<typename Type>
void UnregisterTypeForBuiltinArrayAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(ArrayAnyType<Type>::StaticGetClassId());
}

META_END_NAMESPACE()

#endif
