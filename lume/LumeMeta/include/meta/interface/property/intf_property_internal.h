/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Property internal's interfaces
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
 */

#ifndef META_INTERFACE_PROPERTY_INTERNAL_H
#define META_INTERFACE_PROPERTY_INTERNAL_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_owner.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IPropertyInternal, "1757f95f-07ab-4d2a-af53-5fd6b2f3048a")
META_REGISTER_INTERFACE(IPropertyInternalAny, "65c7309e-21ee-4087-af33-76b297ab4f93")

/// Internal interface for setting things when property is constructed
class IPropertyInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyInternal)
public:
    virtual void SetOwner(IOwner::Ptr owner) = 0;
    virtual void SetSelf(IProperty::Ptr self) = 0;
};

/**
 * @brief Interface to get or set the internal any of property
 *
 * Properties are typeless on the interface and handles the values as IAny.
 * The internal any decides the type of the property, it (or clone there of) is
 * used to hold the value in the property.
 */
class IPropertyInternalAny : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyInternalAny)
public:
    /// Set the internal any. This is usually done by the library when constructing property.
    virtual AnyReturnValue SetInternalAny(IAny::Ptr any) = 0;
    /**
     * @brief Get the internal any, can be used to find out the type of the property.
     *
     * @Notice The value of this any is not specified (it depends hoe the property uses it),
     *         so only the dynamic type matters here.
     */
    virtual IAny::Ptr GetInternalAny() const = 0;
};

META_END_NAMESPACE()

#endif
