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

#ifndef SCENE_SRC_NODE_NODE_H
#define SCENE_SRC_NODE_NODE_H

#include <scene/ext/component.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_node_notify.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_iterable.h>

#include "startable_handler.h"

SCENE_BEGIN_NAMESPACE()

struct ContainableTag {};
class NodeContaineable : public META_NS::IContainable {
    virtual META_NS::IObject::Ptr GetParent(ContainableTag) const = 0;
    META_NS::IObject::Ptr GetParent() const override
    {
        return GetParent(ContainableTag {});
    }
};

struct NodeTag {};
class NodeInterface : public INode {
    virtual Future<INode::Ptr> GetParent(NodeTag) const = 0;
    Future<INode::Ptr> GetParent() const override
    {
        return GetParent(NodeTag {});
    }
};

class Node : public META_NS::IntroduceInterfaces<NamedSceneObject, NodeInterface, INodeImport, META_NS::IIterable,
                 META_NS::IContainer, NodeContaineable, META_NS::IMetadataOwner, INodeNotify> {
    META_OBJECT(Node, SCENE_NS::ClassId::Node, IntroduceInterfaces)

    META_BEGIN_STATIC_DATA()
    META_STATIC_FORWARDED_PROPERTY_DATA(ITransform, BASE_NS::Math::Vec3, Position)
    META_STATIC_FORWARDED_PROPERTY_DATA(ITransform, BASE_NS::Math::Quat, Rotation)
    META_STATIC_FORWARDED_PROPERTY_DATA(ITransform, BASE_NS::Math::Vec3, Scale)
    META_STATIC_FORWARDED_PROPERTY_DATA(INode, bool, Enabled)
    META_STATIC_EVENT_DATA(META_NS::IContainer, META_NS::IOnChildChanged, OnContainerChanged)
    META_END_STATIC_DATA()

    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec3, Position, "TransformComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec3, Scale, "TransformComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Quat, Rotation, "TransformComponent")
    SCENE_USE_COMPONENT_PROPERTY(bool, Enabled, "NodeComponent")

    META_IMPLEMENT_EVENT(META_NS::IOnChildChanged, OnContainerChanged)
public:
    Future<bool> IsEnabledInHierarchy() const override;

    BASE_NS::string GetName() const override;
    Future<BASE_NS::string> GetPath() const override;
    using INode::GetParent;
    Future<INode::Ptr> GetParent(NodeTag) const override;
    Future<BASE_NS::vector<INode::Ptr>> GetChildren() const override;
    Future<bool> RemoveChild(const INode::Ptr&) override;
    Future<bool> AddChild(const INode::Ptr& child, size_t index = -1) override;

    Future<INode::Ptr> ImportChild(const INode::ConstPtr& node) override;
    Future<INode::Ptr> ImportChildScene(const IScene::ConstPtr& scene, BASE_NS::string_view nodeName) override;
    Future<INode::Ptr> ImportChildScene(BASE_NS::string_view uri, BASE_NS::string_view nodeName) override;
    bool SetEcsObject(const IEcsObject::Ptr& obj) override;

public:
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override;
    bool Detach(const IObject::Ptr& attachment) override;

public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    void Destroy() override;
    IScene::Ptr GetScene() const override;

    // IIterable
    META_NS::IterationResult Iterate(const META_NS::IterationParameters& params) override;
    META_NS::IterationResult Iterate(const META_NS::IterationParameters& params) const override;

    // IContainer
    BASE_NS::vector<IObject::Ptr> GetAll() const override;
    IObject::Ptr GetAt(SizeType index) const override;
    SizeType GetSize() const override;
    BASE_NS::vector<IObject::Ptr> FindAll(const FindOptions& options) const override;
    IObject::Ptr FindAny(const FindOptions& options) const override;
    IObject::Ptr FindByName(BASE_NS::string_view name) const override;
    bool Add(const IObject::Ptr& object) override;
    bool Insert(SizeType index, const IObject::Ptr& object) override;
    bool Remove(SizeType index) override;
    bool Remove(const IObject::Ptr& child) override;
    bool Move(SizeType fromIndex, SizeType toIndex) override;
    bool Move(const IObject::Ptr& child, SizeType toIndex) override;
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override;
    void RemoveAll() override;
    bool IsAncestorOf(const IObject::ConstPtr& object) const override;
    META_NS::IObject::Ptr GetParent(ContainableTag) const override;

    void OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i) override;
    void OnChildChanged(META_NS::ContainerChangeType type, const INode::Ptr& child, size_t index) override;
    bool IsListening() const override;
    void OnNodeActiveStateChanged(NodeActiteStateInfo state) override;

private:
    BASE_NS::unique_ptr<StartableHandler> startableHandler_;
};

SCENE_END_NAMESPACE()

#endif
