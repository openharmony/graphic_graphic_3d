/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Helper to create value serializers
 * Author: Mikael Kilpeläinen
 * Create: 2024-01-15
 */

#ifndef META_EXT_SERIALIZATION_VALUE_SERIALIZER_H
#define META_EXT_SERIALIZATION_VALUE_SERIALIZER_H

#include <meta/base/meta_types.h>
#include <meta/ext/serialization/value_serializer.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/interface_helpers.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/serialization/intf_global_serialization_data.h>
#include <meta/interface/serialization/intf_value_serializer.h>

META_BEGIN_NAMESPACE()

/// Helper class to use export/import functions to construct value serializer
template<typename Type, typename ExportFunc, typename ImportFunc>
struct ValueSerializer : IntroduceInterfaces<IValueSerializer> {
    ValueSerializer(ExportFunc e, ImportFunc i) : export_(BASE_NS::move(e)), import_(BASE_NS::move(i)) {}
    TypeId GetTypeId() const override
    {
        return UidFromType<Type>();
    }
    ISerNode::Ptr Export(IExportFunctions& f, const IAny& value) override
    {
        Type v {};
        if (value.GetValue(v)) {
            return export_(f, v);
        }
        return nullptr;
    }
    IAny::Ptr Import(IImportFunctions& f, const ISerNode::ConstPtr& node) override
    {
        Type v {};
        return import_(f, node, v) ? IAny::Ptr(new Any<Type>(v)) : nullptr;
    }

private:
    ExportFunc export_;
    ImportFunc import_;
};

template<typename Type, typename Export, typename Import>
void RegisterSerializer(IGlobalSerializationData& data, Export e, Import i)
{
    data.RegisterValueSerializer(
        CreateShared<ValueSerializer<Type, Export, Import>>(BASE_NS::move(e), BASE_NS::move(i)));
}

template<typename Type, typename Export, typename Import>
void RegisterSerializer(Export e, Import i)
{
    RegisterSerializer<Type>(GetObjectRegistry().GetGlobalSerializationData(), BASE_NS::move(e), BASE_NS::move(i));
}

template<typename Type>
void UnregisterSerializer(IGlobalSerializationData& data)
{
    data.UnregisterValueSerializer(UidFromType<Type>());
}

template<typename Type>
void UnregisterSerializer()
{
    UnregisterSerializer<Type>(GetObjectRegistry().GetGlobalSerializationData());
}

META_END_NAMESPACE()

#endif
