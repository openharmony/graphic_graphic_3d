/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of IDerived interface
 * Author: Jani Kattelus
 * Create: 2022-04-07
 */

#ifndef META_INTERFACE_IDERIVED_H
#define META_INTERFACE_IDERIVED_H

#include <meta/interface/intf_object.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IDerived, "d471a8d7-16fe-4b3e-9ff9-fa8787271e3f")
/**
 * @brief Defines that the object type is derived from another object type.
 *        This is part of the ObjectRegistry object construction machinery.
 */
class IDerived : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IDerived)
public:
    /**
     * @brief Called by the framework. (part of the aggregate part creation). Implementer receives the actual object
     * "aggr" and the requested superclass implementation "impl".
     */
    virtual void SetSuperInstance(const IObject::Ptr& aggr, const IObject::Ptr& impl) = 0;
};

META_END_NAMESPACE()

#endif
