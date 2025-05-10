/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Serialisation intermediate format node
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-04
 */

#ifndef META_INTERFACE_SERIALIZATION_ISER_INPUT_H
#define META_INTERFACE_SERIALIZATION_ISER_INPUT_H

#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

/// Serialisation input interface
class ISerInput : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerInput, "c33cdbb9-9913-41ab-a525-96876b66eb30")
public:
    // todo: have source interface instead of string
    /// Process a string to serialisation tree (e.g. json)
    virtual ISerNode::Ptr Process(BASE_NS::string_view) = 0;
};

META_INTERFACE_TYPE(META_NS::ISerInput)

META_END_NAMESPACE()

#endif
