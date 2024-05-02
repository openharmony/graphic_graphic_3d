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
#include "metav1_compat.h"

#include <meta/ext/serialization/common_value_serializers.h>

META_BEGIN_NAMESPACE()
namespace Serialization {

using BasicMetaTypes = TypeList<float, double, bool, uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t,
    int64_t, BASE_NS::Uid, BASE_NS::string, BASE_NS::string_view, BASE_NS::Math::Vec2, BASE_NS::Math::UVec2,
    BASE_NS::Math::IVec2, BASE_NS::Math::Vec3, BASE_NS::Math::UVec3, BASE_NS::Math::IVec3, BASE_NS::Math::Vec4,
    BASE_NS::Math::UVec4, BASE_NS::Math::IVec4, BASE_NS::Math::Quat, BASE_NS::Math::Mat3X3, BASE_NS::Math::Mat4X4>;

static bool IsV1Property(ObjectId oid)
{
    static const BASE_NS::Uid propertyUid("00000000-0000-0000-5072-6f7065727479");
    return oid.ToUid().data[1] == propertyUid.data[1];
}

template<typename... Types>
static bool CheckBasicMetaTypes(ObjectId oid, ObjectId& out, TypeList<Types...>)
{
    return (false || ... ||
            (UidFromType<Types[]>().data[0] == oid.ToUid().data[0] ? (out = UidFromType<Types>(), true) : false));
}

static bool IsV1BasicArray(ObjectId oid, ObjectId& out)
{
    return CheckBasicMetaTypes(oid, out, BasicMetaTypes {});
}

static bool IsV1Any(ObjectId oid)
{
    static const BASE_NS::Uid anyUid("00000000-0000-0000-5479-706564416e79");
    return oid.ToUid().data[1] == anyUid.data[1];
}

static ObjectId MakeAny(ObjectId oid)
{
    BASE_NS::Uid uid("00000000-0000-0000-4275-696c74416e79");
    uid.data[0] = oid.ToUid().data[0];
    return uid;
}

static ObjectId MakeArrayAny(ObjectId oid)
{
    BASE_NS::Uid uid("00000000-0000-0000-4172-726179416e79");
    uid.data[0] = oid.ToUid().data[0];
    return uid;
}

class NodeVisitor : public IntroduceInterfaces<ISerNodeVisitor> {
public:
    static ISerNode::Ptr VisitNode(ISerNode::Ptr node)
    {
        if (!node) {
            return nullptr;
        }
        NodeVisitor v;
        node->Apply(v);
        return v.node;
    }

    void Visit(const IRootNode& n) override
    {
        node.reset(new RootNode(n.GetSerializerVersion(), VisitNode(n.GetObject())));
    }
    void Visit(const IObjectNode& n) override
    {
        node.reset(new ObjectNode(
            n.GetObjectClassName(), n.GetObjectName(), n.GetObjectId(), n.GetInstanceId(), VisitNode(n.GetMembers())));
    }
    void Visit(const IArrayNode& n) override
    {
        BASE_NS::vector<ISerNode::Ptr> arr;
        for (auto&& m : n.GetMembers()) {
            if (auto n = VisitNode(m)) {
                arr.push_back(n);
            }
        }
        node.reset(new ArrayNode(BASE_NS::move(arr)));
    }
    ISerNode::Ptr RewriteValueToAny(ObjectId property, ISerNode::Ptr node)
    {
        if (auto n = interface_cast<IObjectNode>(node)) {
            if (IsV1Any(n->GetObjectId())) {
                return ISerNode::Ptr(new ObjectNode("Any", n->GetObjectName(), MakeAny(n->GetObjectId()),
                    n->GetInstanceId(), VisitNode(n->GetMembers())));
            }
        }
        BASE_NS::vector<NamedNode> m;
        if (auto n = VisitNode(node)) {
            m.push_back(NamedNode { "value", n });
        }
        ObjectId any = MakeAny(property);
        ObjectId uid;
        if (IsV1BasicArray(property, uid)) {
            any = MakeArrayAny(uid);
        }
        return ISerNode::Ptr(new ObjectNode("Any", "", any, {}, CreateShared<MapNode>(m)));
    }

    ISerNode::Ptr RewritePropertyFlags(ISerNode::Ptr n)
    {
        uint64_t value {};
        uint64_t converted { uint64_t(ObjectFlagBits::SERIALIZE) };
        if (ExtractNumber(n, value)) {
            if (value & 8) {
                converted |= 8;
            }
            if (value & 128) {
                converted |= 16;
            }
            if (converted != uint64_t(ObjectFlagBits::SERIALIZE)) {
                return ISerNode::Ptr(new UIntNode(converted));
            }
        }
        return nullptr;
    }

    ISerNode::Ptr RewriteProperty(BASE_NS::string name, const IObjectNode& n)
    {
        BASE_NS::vector<NamedNode> map;
        ISerNode::Ptr value;
        bool hasDefaultValue = false;
        if (auto m = interface_cast<IMapNode>(n.GetMembers())) {
            for (auto&& node : m->GetMembers()) {
                auto nn = node.node;
                if (node.name == "flags") {
                    node.name = "__flags";
                    nn = RewritePropertyFlags(nn);
                } else if (node.name == "defaultValue") {
                    hasDefaultValue = true;
                    nn = RewriteValueToAny(n.GetObjectId(), nn);
                } else if (node.name == "value" || node.name == "valueObject") {
                    node.name = "values";
                    value = RewriteValueToAny(n.GetObjectId(), nn);
                    nn = ISerNode::Ptr(new ArrayNode({ value }));
                } else {
                    nn = VisitNode(nn);
                }
                if (nn) {
                    map.push_back(NamedNode { node.name, nn });
                }
            }
            if (!value) {
                map.push_back(NamedNode { "values", ISerNode::Ptr(new ArrayNode(BASE_NS::vector<ISerNode::Ptr> {})) });
                if (!hasDefaultValue) {
                    CORE_LOG_E("Invalid json file, property doesn't have value or defaultValue");
                }
            } else if (!hasDefaultValue) {
                map.push_back(NamedNode { "defaultValue", value });
            }
            map.push_back(NamedNode { "modifiers", ISerNode::Ptr(new ArrayNode(BASE_NS::vector<ISerNode::Ptr> {})) });
        }
        auto mapNode = CreateShared<MapNode>(BASE_NS::move(map));
        return ISerNode::Ptr(new ObjectNode("Property", name, ClassId::StackProperty, n.GetInstanceId(), mapNode));
    }
    ISerNode::Ptr RewriteObject(BASE_NS::string name, IObjectNode& n)
    {
        if (IsV1Property(n.GetObjectId())) {
            return RewriteProperty(name, n);
        }
        return ISerNode::Ptr(new ObjectNode(
            n.GetObjectClassName(), n.GetObjectName(), n.GetObjectId(), n.GetInstanceId(), VisitNode(n.GetMembers())));
    }
    ISerNode::Ptr RewritePropertyMap(const NamedNode& node)
    {
        BASE_NS::vector<ISerNode::Ptr> arr;
        auto n = VisitNode(node.node);
        if (auto map = interface_cast<IMapNode>(n)) {
            for (auto&& m : map->GetMembers()) {
                // the rewrite was already done by the above VisitNode call
                arr.push_back(m.node);
            }
        }
        return ISerNode::Ptr(new ArrayNode(arr));
    }
    void AddProperties(BASE_NS::vector<ISerNode::Ptr> properties, IMapNode& map)
    {
        if (auto p = interface_cast<IArrayNode>(map.FindNode("__properties"))) {
            for (auto&& n : properties) {
                p->AddNode(n);
            }
        } else {
            map.AddNode("__properties", IArrayNode::Ptr(new ArrayNode(properties)));
        }
    }
    void Visit(const IMapNode& n) override
    {
        BASE_NS::vector<NamedNode> map;
        BASE_NS::vector<ISerNode::Ptr> properties;
        for (auto&& m : n.GetMembers()) {
            ISerNode::Ptr p;
            if (auto obj = interface_cast<IObjectNode>(m.node)) {
                p = RewriteObject(m.name, *obj);
                if (IsV1Property(obj->GetObjectId())) {
                    properties.push_back(p);
                    p = nullptr;
                }
            } else if (m.name == "__properties") {
                p = RewritePropertyMap(m);
            } else {
                p = VisitNode(m.node);
            }
            if (p) {
                map.push_back(NamedNode { m.name, p });
            }
        }
        auto mapNode = IMapNode::Ptr(new MapNode(BASE_NS::move(map)));
        if (!properties.empty()) {
            AddProperties(properties, *mapNode);
        }
        node = mapNode;
    }
    void Visit(const INilNode& n) override
    {
        node.reset(new NilNode);
    }
    void Visit(const IBoolNode& n) override
    {
        node.reset(new BoolNode(n.GetValue()));
    }
    void Visit(const IDoubleNode& n) override
    {
        node.reset(new DoubleNode(n.GetValue()));
    }
    void Visit(const IIntNode& n) override
    {
        node.reset(new IntNode(n.GetValue()));
    }
    void Visit(const IUIntNode& n) override
    {
        node.reset(new UIntNode(n.GetValue()));
    }
    void Visit(const IStringNode& n) override
    {
        node.reset(new StringNode(n.GetValue()));
    }
    void Visit(const IRefUriNode& n) override
    {
        node.reset(new RefNode(n.GetValue()));
    }
    void Visit(const ISerNode&) override
    {
        CORE_LOG_E("Unknown node type");
    }

    ISerNode::Ptr node;
};

ISerNode::Ptr RewriteMetaV1NodeTree(ISerNode::Ptr tree)
{
    NodeVisitor v;
    tree->Apply(v);
    return v.node;
}

} // namespace Serialization
META_END_NAMESPACE()
