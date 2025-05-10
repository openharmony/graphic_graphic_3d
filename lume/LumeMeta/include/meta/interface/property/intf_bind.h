/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Property Bind interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
 */

#ifndef META_INTERFACE_PROPERTY_BIND_H
#define META_INTERFACE_PROPERTY_BIND_H

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/types.h>
#include <meta/interface/intf_function.h>
#include <meta/interface/property/intf_property.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IBind, "29495e67-14a6-40aa-a16f-1923630af506")

class IBind : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IBind)
public:
    /**
     * @brief Set target for the bind object
     * @param prop/func Target to set (if property, function is created to get the value)
     * @getDeps if true, tries to find dependencies
     * @owner Property that uses this bind or nullptr, used to for checking circular dependencies
     * @return True on success
     */
    virtual bool SetTarget(const IProperty::ConstPtr& prop, bool getDeps, const IProperty* owner) = 0;
    virtual bool SetTarget(const IFunction::ConstPtr& func, bool getDeps, const IProperty* owner) = 0;
    /// Get the target function for this bind object
    virtual IFunction::ConstPtr GetTarget() const = 0;
    /// Explicitly add object this bind depends on (i.e. the value is re-evaluated if the dependency changes)
    virtual bool AddDependency(const INotifyOnChange::ConstPtr& dep) = 0;
    /// Remove dependency
    virtual bool RemoveDependency(const INotifyOnChange::ConstPtr& dep) = 0;
    /// Get all dependencies
    virtual BASE_NS::vector<INotifyOnChange::ConstPtr> GetDependencies() const = 0;
};

META_END_NAMESPACE()

#endif
