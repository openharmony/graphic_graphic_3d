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

#ifndef META_INTERFACE_SERIALIZATION_ISER_NODE_H
#define META_INTERFACE_SERIALIZATION_ISER_NODE_H

#include <core/plugin/intf_interface.h>

#include <meta/base/interface_macros.h>
#include <meta/base/namespace.h>
#include <meta/base/ref_uri.h>
#include <meta/base/version.h>

META_BEGIN_NAMESPACE()

class ISerNodeVisitor;

class ISerNode : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerNode, "eea35313-5add-4408-ae62-0dd7464b986c")
public:
    virtual void Apply(ISerNodeVisitor&) = 0;
};

class IRootNode : public ISerNode {
    META_INTERFACE(ISerNode, IRootNode, "d42016dc-3941-463b-b332-b101ca2a9665")
public:
    virtual Version GetSerializerVersion() const = 0;
    virtual ISerNode::Ptr GetObject() const = 0;
};

class INilNode : public ISerNode {
    META_INTERFACE(ISerNode, INilNode, "d2712b6e-5c3c-4355-8c63-e039b946afdb")
public:
};

class IObjectNode : public ISerNode {
    META_INTERFACE(ISerNode, IObjectNode, "9c702ee2-6943-4cba-97c8-ac4cf9549cd2")
public:
    virtual BASE_NS::string GetObjectClassName() const = 0;
    virtual BASE_NS::string GetObjectName() const = 0;
    virtual ObjectId GetObjectId() const = 0;
    virtual InstanceId GetInstanceId() const = 0;
    virtual ISerNode::Ptr GetMembers() const = 0;

    virtual void SetObjectClassName(BASE_NS::string) = 0;
    virtual void SetObjectName(BASE_NS::string) = 0;
    virtual void SetObjectId(ObjectId) = 0;
    virtual void SetInstanceId(InstanceId) = 0;
    virtual void SetMembers(ISerNode::Ptr) = 0;
};

class IArrayNode : public ISerNode {
    META_INTERFACE(ISerNode, IArrayNode, "40230896-a813-45df-b342-892d5b80405d")
public:
    virtual BASE_NS::vector<ISerNode::Ptr> GetMembers() const = 0;
    virtual void AddNode(const ISerNode::Ptr& node) = 0;
};

struct NamedNode {
    BASE_NS::string name;
    ISerNode::Ptr node;
};

class IMapNode : public ISerNode {
    META_INTERFACE(ISerNode, IMapNode, "2fecfa69-652e-41cc-9724-3145b43a9ec5")
public:
    virtual BASE_NS::vector<NamedNode> GetMembers() const = 0;
    virtual ISerNode::Ptr FindNode(BASE_NS::string_view name) const = 0;
    virtual void AddNode(BASE_NS::string_view name, ISerNode::Ptr) = 0;
};

template<typename Type>
class IBuiltinValueNode : public ISerNode {
    META_INTERFACE(ISerNode, IBuiltinValueNode, MakeUid<Type>("SerNodes"))
public:
    virtual Type GetValue() const = 0;
    virtual void SetValue(const Type& value) = 0;
};

using IBoolNode = IBuiltinValueNode<bool>;
using IDoubleNode = IBuiltinValueNode<double>;
using IIntNode = IBuiltinValueNode<int64_t>;
using IUIntNode = IBuiltinValueNode<uint64_t>;
using IStringNode = IBuiltinValueNode<BASE_NS::string>;
using IRefUriNode = IBuiltinValueNode<RefUri>;

class ISerNodeVisitor : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ISerNodeVisitor, "147d085e-a0c3-480b-ad21-37cb8db0c120")
public:
    virtual void Visit(const IRootNode&) = 0;
    virtual void Visit(const INilNode&) = 0;
    virtual void Visit(const IObjectNode&) = 0;
    virtual void Visit(const IArrayNode&) = 0;
    virtual void Visit(const IMapNode&) = 0;
    virtual void Visit(const IBoolNode&) = 0;
    virtual void Visit(const IDoubleNode&) = 0;
    virtual void Visit(const IIntNode&) = 0;
    virtual void Visit(const IUIntNode&) = 0;
    virtual void Visit(const IStringNode&) = 0;
    virtual void Visit(const IRefUriNode&) = 0;
    virtual void Visit(const ISerNode&) = 0;
};

META_INTERFACE_TYPE(ISerNode);

META_END_NAMESPACE()

#endif
