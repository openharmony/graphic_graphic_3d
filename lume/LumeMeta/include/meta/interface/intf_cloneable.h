/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of ICloneable interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-01-18
 */

#ifndef META_INTERFACE_ICLONEABLE_H
#define META_INTERFACE_ICLONEABLE_H

#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(ICloneable, "f40a850c-5034-4cb8-87ad-4e9a2ddf587b")

/**
 * @brief The ICloneable interface to indicate entity can be cloned
 */
class ICloneable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ICloneable)
public:
    /**
     * @brief Clone the current entity.
     */
    virtual BASE_NS::shared_ptr<CORE_NS::IInterface> GetClone() const = 0;
};

META_END_NAMESPACE()

#endif
