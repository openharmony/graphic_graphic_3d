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

/// Base class for constructing any objects
class AnyBuilder {
public:
    virtual ~AnyBuilder() = default;
    /// Construct any
    virtual IAny::Ptr Construct() = 0;
    /// Get the object id of the any
    virtual ObjectId GetObjectId() const = 0;
    /// Get type name of the any
    virtual BASE_NS::string GetTypeName() const = 0;
};

/**
 * @brief Register interface for the property related functions
 */
class IPropertyRegister : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyRegister, "122b5241-78ee-458a-8bb7-ba8d0bedc863")
public:
    /// Check if property has been registered with given object id
    virtual bool IsPropertyRegistered(const ObjectId& id) const = 0;
    /// Create property of given type and name
    virtual IProperty::Ptr Create(const ObjectId& object, BASE_NS::string_view name) const = 0;
    /// Create bind object
    virtual IBind::Ptr CreateBind() const = 0;
    /// Get invalid any object, this can be used to indicate error when IAny reference is returned
    virtual IAny& InvalidAny() const = 0;
    /// Construct any from object id
    virtual IAny::Ptr ConstructAny(const ObjectId& id) const = 0;
    /// Check if any has been registered with given object id
    virtual bool IsAnyRegistered(const ObjectId& id) const = 0;
    /// Register any type
    virtual void RegisterAny(BASE_NS::shared_ptr<AnyBuilder> builder) = 0;
    /// Unregister any type
    virtual void UnregisterAny(const ObjectId& id) = 0;
};

META_END_NAMESPACE()

#endif
