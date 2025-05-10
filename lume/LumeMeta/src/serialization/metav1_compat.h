/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2023-2024. All rights reserved.
 * Description: Compatibility for Meta v1
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-26
 */

#ifndef META_SRC_SERIALIZATION_METAV1_COMPAT_H
#define META_SRC_SERIALIZATION_METAV1_COMPAT_H

#include <meta/base/namespace.h>
#include <meta/interface/serialization/intf_ser_transformation.h>

#include "ser_nodes.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class MetaMigrateV1 : public IntroduceInterfaces<BaseObject, ISerTransformation> {
    META_OBJECT(MetaMigrateV1, ClassId::MetaMigrateV1, IntroduceInterfaces)

public:
    BASE_NS::string GetName() const override
    {
        return "Meta v1 Migration";
    }
    Version ApplyForLessThanVersion() const override
    {
        return Version(2, 0);
    }
    ISerNode::Ptr Process(ISerNode::Ptr) override;
};

} // namespace Serialization
META_END_NAMESPACE()

#endif
