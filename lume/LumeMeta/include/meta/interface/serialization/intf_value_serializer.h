/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Serialisation value exporter/importer
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-04
 */

#ifndef META_INTERFACE_SERIALIZATION_IVALUE_SERIALIZER_H
#define META_INTERFACE_SERIALIZATION_IVALUE_SERIALIZER_H

#include <meta/base/namespace.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/serialization/intf_export_context.h>
#include <meta/interface/serialization/intf_import_context.h>
#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

/// Interface to define serializer for values
class IValueSerializer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IValueSerializer, "c493f011-102e-4567-9d27-11c3237b15fa")
public:
    /// Get type id which this serialiser can export/import
    virtual TypeId GetTypeId() const = 0;
    /// Export given value as serialisation node (the value has to be compatible with the type)
    virtual ISerNode::Ptr Export(IExportFunctions&, const IAny& value) = 0;
    /// Import given node as value in any
    virtual IAny::Ptr Import(IImportFunctions&, const ISerNode::ConstPtr& node) = 0;
};

META_END_NAMESPACE()

#endif
