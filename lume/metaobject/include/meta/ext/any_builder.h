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

#ifndef META_EXT_ANY_BUILDER_H
#define META_EXT_ANY_BUILDER_H

#include <meta/base/meta_types.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/property/intf_property_register.h>

META_BEGIN_NAMESPACE()

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
};

template<typename AnyType>
void RegisterUserAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateShared<DefaultAnyBuilder<AnyType>>());
}

template<typename Type>
void RegisterTypeForBuiltinAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateShared<DefaultAnyBuilder<Any<Type>>>());
}

template<typename Type>
void RegisterTypeForBuiltinArrayAny()
{
    GetObjectRegistry().GetPropertyRegister().RegisterAny(CreateShared<DefaultAnyBuilder<ArrayAny<Type>>>());
}

template<typename Type>
void UnregisterUserAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(Type::StaticGetClassId());
}

template<typename Type>
void UnregisterTypeForBuiltinAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(Any<Type>::StaticGetClassId());
}

template<typename Type>
void UnregisterTypeForBuiltinArrayAny()
{
    GetObjectRegistry().GetPropertyRegister().UnregisterAny(ArrayAny<Type>::StaticGetClassId());
}

META_END_NAMESPACE()

#endif
