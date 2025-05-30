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

/// Defines functions that can be used when exporting an object
class IExportFunctions : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IExportFunctions, "d7d2d7fe-649a-4b39-bc33-995097a396be")
public:
    /// Export given entity to serialization node
    virtual ReturnError ExportToNode(const IAny& entity, ISerNode::Ptr& out) = 0;
    ISerNode::Ptr ExportToNode(const IAny& entity)
    {
        ISerNode::Ptr node;
        return ExportToNode(entity, node) ? node : nullptr;
    }
    /// Export given value to serialisation node
    template<typename Type>
    ReturnError ExportValueToNode(const Type& value, ISerNode::Ptr& out)
    {
        return ExportToNode(static_cast<const IAny&>(Any<BASE_NS::remove_const_t<Type>>(value)), out);
    }
    template<typename Type>
    ISerNode::Ptr ExportValueToNode(const Type& value)
    {
        ISerNode::Ptr node;
        return ExportValueToNode(value, node) ? node : nullptr;
    }
};

/// Interface that used to export objects inside object's export function
class IExportContext : public IExportFunctions {
    META_INTERFACE(IExportFunctions, IExportContext, "a669af8b-58af-4468-ab64-4a38fcc80bf1")
public:
    /// Export given entity with name (exports the value inside the any)
    virtual ReturnError Export(BASE_NS::string_view name, const IAny& entity) = 0;
    /// Export given any with name (exports the actual any)
    virtual ReturnError ExportAny(BASE_NS::string_view name, const IAny::Ptr& any) = 0;
    /// Export weak pointer to object with name
    virtual ReturnError ExportWeakPtr(BASE_NS::string_view name, const IObject::ConstWeakPtr& ptr) = 0;
    /// Export itself as known interfaces, like IMetadata, IContainer, IAttach
    virtual ReturnError AutoExport() = 0;

    /// Export given value with name
    template<typename Type>
    ReturnError ExportValue(BASE_NS::string_view name, const Type& value)
    {
        return Export(name, static_cast<const IAny&>(Any<BASE_NS::remove_const_t<Type>>(value)));
    }
    /// Export given array of values with name
    template<typename Type>
    ReturnError ExportValue(BASE_NS::string_view name, const BASE_NS::vector<Type>& value)
    {
        return Export(name, static_cast<const IAny&>(ArrayAny<Type>(value)));
    }

    /// Get the context in which we are exporting, this is the IExporter typically
    virtual CORE_NS::IInterface* Context() const = 0;
    /// Get user set context object
    virtual META_NS::IObject::Ptr UserContext() const = 0;
    /// Substitute serialisation of the current object with given node (all exports for this object are discarded)
    virtual ReturnError SubstituteThis(ISerNode::Ptr) = 0;
    /// Get Metadata
    virtual SerMetadata GetMetadata() const = 0;
};

META_INTERFACE_TYPE(META_NS::IExportContext)

META_END_NAMESPACE()

#endif
