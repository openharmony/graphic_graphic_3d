/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2023. All rights reserved.
 * Description: Definition of IMetadata interface
 * Author: Jani Kattelus
 * Create: 2021-08-26
 */

#ifndef META_INTERFACE_ISTATIC_METADATA_H
#define META_INTERFACE_ISTATIC_METADATA_H

#include <meta/base/interface_macros.h>
#include <meta/interface/static_object_metadata.h>

META_BEGIN_NAMESPACE()

/// Interface to get registered static metadata for object type
class IStaticMetadata : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IStaticMetadata, "de9eee18-b2cc-4060-8bd8-5fabd3457e59")
public:
    /// Get static metadata
    virtual const StaticObjectMetadata* GetStaticMetadata() const = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IStaticMetadata)

#endif
