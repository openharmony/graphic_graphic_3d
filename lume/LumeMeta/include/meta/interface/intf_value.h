/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Value interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-10-18
 */

#ifndef META_INTERFACE_VALUE_H
#define META_INTERFACE_VALUE_H

#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IValue, "23c3c0c7-9937-468c-9199-28f982b40fe5")

/// Interface for values
class IValue : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IValue)
public:
    /// Set value as given any (the any has to be compatible)
    virtual AnyReturnValue SetValue(const IAny& value) = 0;
    /// Get value as any or invalid any in case of error
    virtual const IAny& GetValue() const = 0;
    /// Check if given type id if compatible with this value
    virtual bool IsCompatible(const TypeId& id) const = 0;
};

META_INTERFACE_TYPE(META_NS::IValue)

META_END_NAMESPACE()

#endif
