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

#ifndef META_INTERFACE_SERIALIZATION_IVALUE_SERIALIZER_H
#define META_INTERFACE_SERIALIZATION_IVALUE_SERIALIZER_H

#include <meta/base/namespace.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/serialization/intf_export_context.h>
#include <meta/interface/serialization/intf_import_context.h>
#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

class IValueSerializer : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IValueSerializer, "c493f011-102e-4567-9d27-11c3237b15fa")
public:
    virtual TypeId GetTypeId() const = 0;
    virtual ISerNode::Ptr Export(IExportFunctions&, const IAny& value) = 0;
    virtual IAny::Ptr Import(IImportFunctions&, const ISerNode::ConstPtr& node) = 0;
};

META_END_NAMESPACE()

#endif
