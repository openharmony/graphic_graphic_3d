/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of IRecyclable interface
 * Author: Mikael Kilpeläinen
 * Create: 2023-05-26
 */

#ifndef META_INTERFACE_IRECYCLABLE_H
#define META_INTERFACE_IRECYCLABLE_H

#include <meta/interface/intf_metadata.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IRecyclable, "47ded8f2-b790-47af-a576-88e5b157a179")

/**
 * @brief The IRecyclable interface to indicate entity can be recycled
 */
class IRecyclable : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IRecyclable)
public:
    /**
     * @brief Build the object again when recycling (see ILifecycle for Build).
     */
    virtual bool ReBuild(const IMetadata::Ptr&) = 0;
    /**
     * @brief Dispose the object before it can be re-used.
     */
    virtual void Dispose() = 0;
};

META_END_NAMESPACE()

#endif
