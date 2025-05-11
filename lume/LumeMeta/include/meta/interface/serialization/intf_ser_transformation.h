/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Serialisation tree transformation
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-04
 */

#ifndef META_INTERFACE_SERIALIZATION_ISER_TRANSFORMATION_H
#define META_INTERFACE_SERIALIZATION_ISER_TRANSFORMATION_H

#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

/// Transformation for serialisation tree
class ISerTransformation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerTransformation, "dea20f12-e257-4204-9f38-3cb8eb4f4ced")
public:
    /// Get name of this transformation
    virtual BASE_NS::string GetName() const = 0;
    /// Get lowest version you should not apply this, that is, apply for everything less than this version.
    virtual Version ApplyForLessThanVersion() const = 0;
    /// Apply transformation to serialisation tree (might create totally new tree)
    virtual ISerNode::Ptr Process(ISerNode::Ptr) = 0;
};

META_INTERFACE_TYPE(META_NS::ISerTransformation)

META_END_NAMESPACE()

#endif
