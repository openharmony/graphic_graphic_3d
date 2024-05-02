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

#ifndef META_SRC_SERIALIZATION_SER_NODES_H
#define META_SRC_SERIALIZATION_SER_NODES_H

#include <base/containers/unordered_map.h>

#include <meta/base/ref_uri.h>
#include <meta/base/type_traits.h>
#include <meta/base/version.h>
#include <meta/ext/minimal_object.h>
#include <meta/ext/object_factory.h>
#include <meta/interface/intf_object_factory.h>
#include <meta/interface/serialization/intf_ser_node.h>

#include "../base_object.h"

META_BEGIN_NAMESPACE()
namespace Serialization {

class NilNode : public Internal::BaseObjectFwd<NilNode, ClassId::NilNode, INilNode> {
public:
    NilNode() = default;
    void Apply(ISerNodeVisitor& v) override
    {
        v.Visit(*this);
    }
};

class MapNode : public Internal::BaseObjectFwd<MapNode, ClassId::MapNode, IMapNode> {
public:
    MapNode() = default;
    MapNode(BASE_NS::vector<NamedNode> elements) : elements(BASE_NS::move(elements)) {}

    BASE_NS::vector<NamedNode> GetMembers() const override
    {
        return elements;
    }

    ISerNode::Ptr FindNode(BASE_NS::string_view name) const override
    {
        for (auto&& v : elements) {
            if (name == v.name) {
                return v.node;
            }
        }
        return nullptr;
    }

    void AddNode(BASE_NS::string_view name, ISerNode::Ptr n) override
    {
        elements.push_back(NamedNode { BASE_NS::string(name), BASE_NS::move(n) });
    }

    void Apply(ISerNodeVisitor& v) override
    {
        v.Visit(*this);
    }

public:
    BASE_NS::vector<NamedNode> elements;
};

class ArrayNode : public Internal::BaseObjectFwd<ArrayNode, ClassId::ArrayNode, IArrayNode> {
public:
    ArrayNode() = default;
    ArrayNode(BASE_NS::vector<ISerNode::Ptr> elements) : elements(BASE_NS::move(elements)) {}

    BASE_NS::vector<ISerNode::Ptr> GetMembers() const override
    {
        return elements;
    }

    void AddNode(const ISerNode::Ptr& node) override
    {
        elements.push_back(node);
    }

    void Apply(ISerNodeVisitor& v) override
    {
        v.Visit(*this);
    }

public:
    BASE_NS::vector<ISerNode::Ptr> elements;
};

class ObjectNode : public Internal::BaseObjectFwd<ObjectNode, ClassId::ObjectNode, IObjectNode> {
public:
    ObjectNode() = default;
    ObjectNode(BASE_NS::string className, BASE_NS::string name, const ObjectId& oid, const InstanceId& iid,
        ISerNode::Ptr members)
        : className(BASE_NS::move(className)), name(BASE_NS::move(name)), objectType(oid), instance(iid),
          members(BASE_NS::move(members))
    {}

    BASE_NS::string GetObjectClassName() const override
    {
        return className;
    }
    BASE_NS::string GetObjectName() const override
    {
        return name;
    }
    ObjectId GetObjectId() const override
    {
        return objectType;
    }
    InstanceId GetInstanceId() const override
    {
        return instance;
    }
    ISerNode::Ptr GetMembers() const override
    {
        return members;
    }

    void SetObjectClassName(BASE_NS::string name) override
    {
        className = BASE_NS::move(name);
    }
    void SetObjectName(BASE_NS::string name) override
    {
        className = BASE_NS::move(name);
    }
    void SetObjectId(ObjectId id) override
    {
        objectType = id;
    }
    void SetInstanceId(InstanceId id) override
    {
        instance = id;
    }
    void SetMembers(ISerNode::Ptr n) override
    {
        members = BASE_NS::move(n);
    }

    void Apply(ISerNodeVisitor& v) override
    {
        v.Visit(*this);
    }

public:
    BASE_NS::string className;
    BASE_NS::string name;
    ObjectId objectType;
    InstanceId instance;
    ISerNode::Ptr members;
};

class RootNode : public Internal::BaseObjectFwd<RootNode, ClassId::RootNode, IRootNode> {
public:
    RootNode() = default;
    RootNode(const Version& ver, ISerNode::Ptr obj) : serializerVersion_(ver), object(BASE_NS::move(obj)) {}

    Version GetSerializerVersion() const override
    {
        return serializerVersion_;
    }
    ISerNode::Ptr GetObject() const override
    {
        return object;
    }

    void Apply(ISerNodeVisitor& v) override
    {
        v.Visit(*this);
    }

public:
    Version serializerVersion_ { 1, 0 };
    ISerNode::Ptr object;
};

template<typename Type, const META_NS::ClassInfo& ClassInfo>
class BuiltinValueNode
    : public Internal::BaseObjectFwd<BuiltinValueNode<Type, ClassInfo>, ClassInfo, IBuiltinValueNode<Type>> {
public:
    using InterfaceType = IBuiltinValueNode<Type>;

    BuiltinValueNode() = default;
    BuiltinValueNode(const Type& v) : value(v) {}

    Type GetValue() const override
    {
        return value;
    }

    void SetValue(const Type& v) override
    {
        value = v;
    }

    void Apply(ISerNodeVisitor& v) override
    {
        v.Visit(*this);
    }

public:
    Type value {};
};

using BoolNode = BuiltinValueNode<bool, ClassId::BoolNode>;
using IntNode = BuiltinValueNode<int64_t, ClassId::IntNode>;
using UIntNode = BuiltinValueNode<uint64_t, ClassId::UIntNode>;
using DoubleNode = BuiltinValueNode<double, ClassId::DoubleNode>;
using StringNode = BuiltinValueNode<BASE_NS::string, ClassId::StringNode>;
using RefNode = BuiltinValueNode<RefUri, ClassId::RefNode>;

template<typename Type, typename Node>
struct SupportedType {
    using NodeType = Node;
    constexpr const static TypeId ID = UidFromType<Type>();

    static ISerNode::Ptr CreateNode(const IAny& any)
    {
        return ISerNode::Ptr(new NodeType(GetValue<Type>(any)));
    }

    static AnyReturnValue ExtractValue(const ISerNode::ConstPtr& n, IAny& any)
    {
        Type v {};
        if (auto node = interface_cast<typename NodeType::InterfaceType>(n)) {
            v = static_cast<Type>(node->GetValue());
        } else {
            if constexpr (BASE_NS::is_same_v<NodeType, IntNode> || BASE_NS::is_same_v<NodeType, DoubleNode>) {
                if (auto node = interface_cast<UIntNode::InterfaceType>(n)) {
                    v = static_cast<Type>(node->GetValue());
                } else if (auto node = interface_cast<IntNode::InterfaceType>(n)) {
                    v = static_cast<Type>(node->GetValue());
                }
            }
        }
        return any.SetValue(v);
    }
};

// clang-format off
using SupportedBuiltins = TypeList<
    SupportedType<bool, BoolNode>,
    SupportedType<double, DoubleNode>,
    SupportedType<uint8_t, UIntNode>,
    SupportedType<uint16_t, UIntNode>,
    SupportedType<uint32_t, UIntNode>,
    SupportedType<uint64_t, UIntNode>,
    SupportedType<int8_t, IntNode>,
    SupportedType<int16_t, IntNode>,
    SupportedType<int32_t, IntNode>,
    SupportedType<int64_t, IntNode>,
    SupportedType<BASE_NS::string, StringNode>,
    SupportedType<RefUri, RefNode>
    >;
// clang-format on

} // namespace Serialization
META_END_NAMESPACE()

#endif
