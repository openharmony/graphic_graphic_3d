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

#ifndef META_INTERFACE_PROPERTY_REGISTER_H
#define META_INTERFACE_PROPERTY_REGISTER_H

#include <meta/interface/property/intf_bind.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

class AnyBuilder {
public:
    virtual ~AnyBuilder() = default;
    virtual IAny::Ptr Construct() = 0;
    virtual ObjectId GetObjectId() const = 0;
};

class IPropertyRegister : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyRegister, "122b5241-78ee-458a-8bb7-ba8d0bedc863")
public:
    virtual IProperty::Ptr Create(const ObjectId& object, BASE_NS::string_view name) const = 0;
    virtual IBind::Ptr CreateBind() const = 0;

    virtual IAny& InvalidAny() const = 0;
    virtual IAny::Ptr ConstructAny(const ObjectId& id) const = 0;
    virtual bool IsAnyRegistered(const ObjectId& id) const = 0;
    virtual void RegisterAny(BASE_NS::shared_ptr<AnyBuilder> builder) = 0;
    virtual void UnregisterAny(const ObjectId& id) = 0;
};

META_END_NAMESPACE()

#endif
