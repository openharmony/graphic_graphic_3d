/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Property interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
 */

#ifndef META_INTERFACE_PROPERTY_H
#define META_INTERFACE_PROPERTY_H

#include <meta/base/expected.h>
#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_notify_on_change.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_owner.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IProperty, "772086d4-b7eb-437e-b71b-83353caaffaf")

/**
 * @brief Typeless property interface, values are accessed as IAny.
 *
 * @Notice IProperty does not do any locking for its functions, one has to explicitly lock from outside.
 * Use one of the wrapper types to safely access properties (see Property<>, ArrayProperty<> and associated).
 */
class IProperty : public INotifyOnChange {
    META_INTERFACE(INotifyOnChange, IProperty)
public:
    /// Get the property's name
    virtual BASE_NS::string GetName() const = 0;
    /// Get the property's owner or nullptr
    virtual IOwner::WeakPtr GetOwner() const = 0;

    /// Set value, the value is copied from the given IAny if compatible. Returns result.
    virtual AnyReturnValue SetValue(const IAny& value) = 0;
    /// Get value, returns invalid any that is not compatible with anything in case of error.
    virtual const IAny& GetValue() const = 0;
    /// Check if property value is compatible with given type id.
    virtual bool IsCompatible(const TypeId& id) const = 0;
    /// Get the type id of the property value
    virtual TypeId GetTypeId() const = 0;
    /// Mark that the property has to be re-evaluated on GetValue and invoke the OnChanged handlers.
    virtual void NotifyChange() const = 0;
    /// Reset the property value to value-constructed value of the contained type. See IStackResetable for changing this
    /// behaviour.
    virtual void ResetValue() = 0;
    /// Check if this property has default value (the value has not been set explicitly by user)
    virtual bool IsDefaultValue() const = 0;
    /// Check if user has set value for this property
    bool IsValueSet() const
    {
        return !IsDefaultValue();
    }
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IProperty)

#endif
