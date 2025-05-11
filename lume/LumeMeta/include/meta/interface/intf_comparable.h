/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of Comparable interfaces
 * Author: Mikael Kilpeläinen
 * Create: 2023-01-24
 */

#ifndef META_INTERFACE_ICOMPARABLE_H
#define META_INTERFACE_ICOMPARABLE_H

#include <core/plugin/intf_interface.h>

#include <meta/base/types.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IComparable, "aae8a240-8a99-4edc-869e-3b87f42e37a0")

/**
 * @brief The IComparable interface to indicate entity can be compared
 */
class IComparable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IComparable)
public:
    /**
     * @brief Compare given object to this one.
     *        0 on equal, negative if this is less than, positive if this is greater than.
     */
    virtual int Compare(const IComparable::Ptr&) const = 0;
};

META_END_NAMESPACE()

#endif
