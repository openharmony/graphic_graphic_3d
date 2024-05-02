/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
