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

class IImportFunctions : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IImportFunctions, "b5415715-8eb7-4106-ab1e-2dfad7134936")
public:
    virtual IObject::Ptr ResolveRefUri(const RefUri& uri) = 0;
    virtual ReturnError ImportFromNode(const ISerNode::ConstPtr&, IAny& entity) = 0;
};

class IImportContext : public IImportFunctions {
    META_INTERFACE(IImportFunctions, IImportContext, "21dd3f27-d987-4ffd-93de-8d8c6d0585ef")
public:
    virtual bool HasMember(BASE_NS::string_view name) const = 0;
    virtual ReturnError Import(BASE_NS::string_view name, IAny& entity) = 0;
    virtual ReturnError ImportAny(BASE_NS::string_view name, IAny::Ptr& any) = 0;
    virtual ReturnError ImportWeakPtr(BASE_NS::string_view name, IObject::WeakPtr& ptr) = 0;
    virtual ReturnError AutoImport() = 0;

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
};

META_END_NAMESPACE()

#endif
