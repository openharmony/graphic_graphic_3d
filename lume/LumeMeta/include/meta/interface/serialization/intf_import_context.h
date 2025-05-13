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

#ifndef META_INTERFACE_SERIALIZATION_IIMPORT_CONTEXT_H
#define META_INTERFACE_SERIALIZATION_IIMPORT_CONTEXT_H

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/interface/detail/any.h>
#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

/// Defines interface for functions that can be used when importing an object
class IImportFunctions : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImportFunctions, "b5415715-8eb7-4106-ab1e-2dfad7134936")
public:
    /// Try to resolve ref uri to an object
    virtual IObject::Ptr ResolveRefUri(const RefUri& uri) = 0;
    /// Import entity from serialisation node
    virtual ReturnError ImportFromNode(const ISerNode::ConstPtr&, IAny& entity) = 0;

    /// Import value from serialisation node
    template<typename Type>
    ReturnError ImportValueFromNode(const ISerNode::ConstPtr& node, Type& value)
    {
        Any<Type> any;
        auto res = ImportFromNode(node, any);
        if (res) {
            value = any.InternalGetValue();
        }
        return res;
    }
};

/// Interface that used to import objects inside object's import function
class IImportContext : public IImportFunctions {
    META_INTERFACE(IImportFunctions, IImportContext, "21dd3f27-d987-4ffd-93de-8d8c6d0585ef")
public:
    /// Check if something was serialised with given name
    virtual bool HasMember(BASE_NS::string_view name) const = 0;
    /// Import entity with given name, the given entity any much be compatible with the serialised type.
    virtual ReturnError Import(BASE_NS::string_view name, IAny& entity) = 0;
    /// Import any with given any, the any is constructed using global registry.
    virtual ReturnError ImportAny(BASE_NS::string_view name, IAny::Ptr& any) = 0;
    /// Import weak pointer with given name
    virtual ReturnError ImportWeakPtr(BASE_NS::string_view name, IObject::WeakPtr& ptr) = 0;
    /// Import itself as known interfaces, like IMetadata, IContainer, IAttach
    virtual ReturnError AutoImport() = 0;
    /// Import value with given name
    template<typename Type>
    ReturnError ImportValue(BASE_NS::string_view name, Type& value)
    {
        Any<Type> v(value);
        auto r = Import(name, static_cast<IAny&>(v));
        if (r) {
            value = v.InternalGetValue();
        }
        return r;
    }
    /// Import array of values with given name
    template<typename Type>
    ReturnError ImportValue(BASE_NS::string_view name, BASE_NS::vector<Type>& value)
    {
        ArrayAny<Type> v(value);
        auto r = Import(name, static_cast<IAny&>(v));
        if (r) {
            value = v.InternalGetValue();
        }
        return r;
    }

    /// Get the context in which we are importing, this is the IImporter typically
    virtual CORE_NS::IInterface* Context() const = 0;
    /// Get user set context object
    virtual IObject::Ptr UserContext() const = 0;
    /// Substitute the result of de-serialisation of the current object with given object
    virtual ReturnError SubstituteThis(IObject::Ptr) = 0;
    /// Get the entity name
    virtual BASE_NS::string GetName() const = 0;
    /// Get Metadata
    virtual SerMetadata GetMetadata() const = 0;
};

META_END_NAMESPACE()

#endif
