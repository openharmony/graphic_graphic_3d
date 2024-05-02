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

#ifndef META_INTERFACE_SERIALIZATION_IEXPORT_CONTEXT_H
#define META_INTERFACE_SERIALIZATION_IEXPORT_CONTEXT_H

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/serialization/intf_ser_node.h>

META_BEGIN_NAMESPACE()

class IExportFunctions : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExportFunctions, "d7d2d7fe-649a-4b39-bc33-995097a396be")
public:
    virtual ReturnError ExportToNode(const IAny& entity, ISerNode::Ptr& out) = 0;
    ISerNode::Ptr ExportToNode(const IAny& entity)
    {
        ISerNode::Ptr node;
        return ExportToNode(entity, node) ? node : nullptr;
    }
};

class IExportContext : public IExportFunctions {
    META_INTERFACE(IExportFunctions, IExportContext, "a669af8b-58af-4468-ab64-4a38fcc80bf1")
public:
    virtual ReturnError Export(BASE_NS::string_view name, const IAny& entity) = 0;
    virtual ReturnError ExportAny(BASE_NS::string_view name, const IAny::Ptr& any) = 0;
    virtual ReturnError ExportWeakPtr(BASE_NS::string_view name, const IObject::ConstWeakPtr& ptr) = 0;
    // Export itself as known interfaces, like IMetadata, IContainer, IAttach
    virtual ReturnError AutoExport() = 0;

    template<typename Type>
    ReturnError ExportValue(BASE_NS::string_view name, const Type& value)
    {
        return Export(name, static_cast<const IAny&>(Any<Type>(value)));
    }

    template<typename Type>
    ReturnError ExportValue(BASE_NS::string_view name, const BASE_NS::vector<Type>& value)
    {
        return Export(name, static_cast<const IAny&>(ArrayAny<Type>(value)));
    }
};

META_INTERFACE_TYPE(IExportContext);

META_END_NAMESPACE()

#endif
