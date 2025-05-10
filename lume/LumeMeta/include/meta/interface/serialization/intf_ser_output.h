/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Serialisation intermediate format node
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-04
 */

#ifndef META_INTERFACE_SERIALIZATION_ISER_OUTPUT_H
#define META_INTERFACE_SERIALIZATION_ISER_OUTPUT_H

#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

/// Serialisation output interface
class ISerOutput : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerOutput, "6ec3b784-34a1-4fd6-964e-4af37de4186b")
public:
    // todo: have sink interface instead of returning string
    /// Process serialisation tree to string (e.g. to json string)
    virtual BASE_NS::string Process(const ISerNode::Ptr& tree) = 0;
};

META_INTERFACE_TYPE(META_NS::ISerOutput)

META_END_NAMESPACE()

#endif
