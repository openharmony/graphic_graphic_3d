/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "node_template_rewrite.h"

#include <meta/interface/builtin_objects.h>
#include <meta/interface/resource/intf_resource.h>
#include <meta/interface/serialization/intf_ser_node.h>

#include "scene_ser.h"

SCENE_BEGIN_NAMESPACE()

namespace {

class NodeVisitor : public META_NS::IntroduceInterfaces<META_NS::ISerNodeVisitor> {
public:
    NodeVisitor(BASE_NS::string_view oldGroup, BASE_NS::string_view newGroup, bool& changed)
        : oldGroup_(oldGroup), newGroup_(newGroup), changed_(changed)
    {}

    META_NS::ISerNode::Ptr VisitNode(META_NS::ISerNode::Ptr node) const
    {
        if (!node) {
            return nullptr;
        }
        NodeVisitor v(oldGroup_, newGroup_, changed_);
        node->Apply(v);
        return v.node;
    }

    void Visit(const META_NS::IRootNode& n) override
    {
        auto root = META_NS::GetObjectRegistry().Create<META_NS::IRootNode>(META_NS::ClassId::RootNode);
        if (root) {
            root->SetMetadata(n.GetMetadata());
            root->SetObject(VisitNode(n.GetObject()));
        }
        node = root;
    }
    META_NS::ISerNode::Ptr RewriteGroup(const META_NS::NamedNode& m)
    {
        auto sn = interface_cast<META_NS::IStringNode>(m.node);
        if (!sn) {
            return m.node;
        }
        if (sn->GetValue() == oldGroup_) {
            if (auto str = META_NS::GetObjectRegistry().Create<META_NS::IStringNode>(META_NS::ClassId::StringNode)) {
                str->SetValue(BASE_NS::string(newGroup_));
                changed_ = true;
                return str;
            }
        }
        return m.node;
    }
    void VisitObjectMembers(const META_NS::IMapNode& map, META_NS::IMapNode& mapNode)
    {
        for (auto&& m : map.GetMembers()) {
            auto node = m.node;
            if (m.name == "resourceId.group" || m.name == "baseResource.group") {
                node = RewriteGroup(m);
            }
            mapNode.AddNode(m.name, VisitNode(node));
        }
    }
    void Visit(const META_NS::IObjectNode& n) override
    {
        META_NS::ISerNode::Ptr snode;
        if (n.GetObjectId() == META_NS::ClassId::ResourcePlaceholder.Id() ||
            n.GetObjectId() == SCENE_NS::ClassId::SceneExternalNodeSer.Id() ||
            n.GetObjectId() == META_NS::ClassId::ObjectResourceOptions.Id()) {
            auto mapNode = META_NS::GetObjectRegistry().Create<META_NS::IMapNode>(META_NS::ClassId::MapNode);
            if (mapNode) {
                if (auto m = interface_cast<META_NS::IMapNode>(n.GetMembers())) {
                    VisitObjectMembers(*m, *mapNode);
                }
            }
            snode = mapNode;
        }
        if (!snode) {
            snode = VisitNode(n.GetMembers());
        }
        auto obj = META_NS::GetObjectRegistry().Create<META_NS::IObjectNode>(META_NS::ClassId::ObjectNode);
        if (obj) {
            obj->SetObjectClassName(n.GetObjectClassName());
            obj->SetObjectName(n.GetObjectName());
            obj->SetObjectId(n.GetObjectId());
            obj->SetInstanceId(n.GetInstanceId());
            obj->SetMembers(snode);
        }
        node = obj;
    }
    void Visit(const META_NS::IArrayNode& n) override
    {
        auto arr = META_NS::GetObjectRegistry().Create<META_NS::IArrayNode>(META_NS::ClassId::ArrayNode);
        if (arr) {
            for (auto&& m : n.GetMembers()) {
                if (auto n = VisitNode(m)) {
                    arr->AddNode(n);
                }
            }
        }
        node = arr;
    }

    void Visit(const META_NS::IMapNode& n) override
    {
        auto map = META_NS::GetObjectRegistry().Create<META_NS::IMapNode>(META_NS::ClassId::MapNode);
        if (map) {
            for (auto&& m : n.GetMembers()) {
                if (auto n = VisitNode(m.node)) {
                    map->AddNode(m.name, n);
                }
            }
        }
        node = map;
    }
    void Visit(const META_NS::INilNode& n) override
    {
        node = META_NS::GetObjectRegistry().Create<META_NS::INilNode>(META_NS::ClassId::NilNode);
    }

    template<typename NodeType, typename Type>
    void SetNode(META_NS::ObjectId id, const Type& v)
    {
        auto nn = META_NS::GetObjectRegistry().Create<NodeType>(id);
        if (nn) {
            nn->SetValue(v);
        }
        node = nn;
    }

    void Visit(const META_NS::IBoolNode& n) override
    {
        SetNode<META_NS::IBoolNode>(META_NS::ClassId::BoolNode, n.GetValue());
    }
    void Visit(const META_NS::IDoubleNode& n) override
    {
        SetNode<META_NS::IDoubleNode>(META_NS::ClassId::DoubleNode, n.GetValue());
    }
    void Visit(const META_NS::IIntNode& n) override
    {
        SetNode<META_NS::IIntNode>(META_NS::ClassId::IntNode, n.GetValue());
    }
    void Visit(const META_NS::IUIntNode& n) override
    {
        SetNode<META_NS::IUIntNode>(META_NS::ClassId::UIntNode, n.GetValue());
    }
    void Visit(const META_NS::IStringNode& n) override
    {
        SetNode<META_NS::IStringNode>(META_NS::ClassId::StringNode, n.GetValue());
    }
    void Visit(const META_NS::IRefUriNode& n) override
    {
        SetNode<META_NS::IRefUriNode>(META_NS::ClassId::RefNode, n.GetValue());
    }
    void Visit(const META_NS::ISerNode&) override
    {
        CORE_LOG_E("Unknown node type");
    }

    META_NS::ISerNode::Ptr node;
    BASE_NS::string_view oldGroup_;
    BASE_NS::string_view newGroup_;
    bool& changed_;
};
}  // namespace

META_NS::ISerNode::Ptr NodeTemplateGroupRewrite::Process(META_NS::ISerNode::ConstPtr tree)
{
    NodeVisitor v(oldGroup, newGroup, changed);
    tree->Apply(v);
    return v.node;
}

SCENE_END_NAMESPACE()
