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

#ifndef SCENE_EXT_NODE_FWD_H
#define SCENE_EXT_NODE_FWD_H

#include <scene/ext/get_parent_helper.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/ext/intf_engine_property_init.h>
#include <scene/ext/intf_node_notify.h>
#include <scene/interface/intf_layer.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>

#include <meta/ext/object_fwd.h>
#include <meta/interface/intf_containable.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_iterable.h>

SCENE_BEGIN_NAMESPACE()

class NodeFwd : public META_NS::IntroduceInterfaces<META_NS::ObjectFwd, IEcsObjectAccess, IEnginePropertyInit,
                    META_NS::INamed, NodeInterface, INodeImport, META_NS::IIterable, META_NS::IContainer,
                    NodeContaineable, META_NS::IMetadataOwner, INodeNotify, ILayer> {
    META_OBJECT_NO_CLASSINFO(NodeFwd, IntroduceInterfaces, ClassId::Node)

public:
    META_FORWARD_PROPERTY(BASE_NS::string, Name, EcsNameProperty())

    META_FORWARD_PROPERTY(BASE_NS::Math::Vec3, Position, META_EXT_CALL_BASE(ITransform, Position()))
    META_FORWARD_PROPERTY(BASE_NS::Math::Quat, Rotation, META_EXT_CALL_BASE(ITransform, Rotation()))
    META_FORWARD_PROPERTY(BASE_NS::Math::Vec3, Scale, META_EXT_CALL_BASE(ITransform, Scale()))
    META_FORWARD_PROPERTY(uint64_t, LayerMask, META_EXT_CALL_BASE(ILayer, LayerMask()))
    META_FORWARD_PROPERTY(bool, Enabled, META_EXT_CALL_BASE(INode, Enabled()))

    BASE_NS::shared_ptr<::META_NS::IEvent> EventOnContainerChanged(META_NS::MetadataQuery q) const override
    {
        return META_EXT_CALL_BASE(META_NS::IContainer, EventOnContainerChanged(q));
    }
    META_EVENT_TYPED_IMPL(META_NS::IOnChildChanged, OnContainerChanged)

    META_NS::IProperty::Ptr EcsNameProperty() const
    {
        auto i = interface_cast<META_NS::INamed>(GetEcsObject());
        return i ? i->Name() : nullptr;
    }

protected:
    BASE_NS::Math::Mat4X4 GetTransformMatrix() const override
    {
        return META_EXT_CALL_BASE(ITransform, GetTransformMatrix());
    }
    bool AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override
    {
        return META_EXT_CALL_BASE(IEnginePropertyInit, AttachEngineProperty(p, path));
    }
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override
    {
        return META_EXT_CALL_BASE(IEnginePropertyInit, InitDynamicProperty(p, path));
    }
    BASE_NS::string GetName() const override
    {
        return META_EXT_CALL_BASE(INode, GetName());
    }
    Future<bool> IsEnabledInHierarchy() const override
    {
        return META_EXT_CALL_BASE(INode, IsEnabledInHierarchy());
    }
    Future<BASE_NS::string> GetPath() const override
    {
        return META_EXT_CALL_BASE(INode, GetPath());
    }
    using INode::GetParent;
    Future<INode::Ptr> GetParent(NodeTag) const override
    {
        return META_EXT_CALL_BASE(INode, GetParent());
    }
    Future<BASE_NS::vector<INode::Ptr>> GetChildren() const override
    {
        return META_EXT_CALL_BASE(INode, GetChildren());
    }
    Future<bool> RemoveChild(const INode::Ptr& n) override
    {
        return META_EXT_CALL_BASE(INode, RemoveChild(n));
    }
    Future<bool> AddChild(const INode::Ptr& child, size_t index) override
    {
        return META_EXT_CALL_BASE(INode, AddChild(child, index));
    }
    Future<INode::Ptr> Clone(BASE_NS::string_view nodeName, const INode::Ptr& parent) override
    {
        return META_EXT_CALL_BASE(INode, Clone(nodeName, parent));
    }
    Future<INode::Ptr> ImportChild(const INode::ConstPtr& node) override
    {
        return META_EXT_CALL_BASE(INodeImport, ImportChild(node));
    }
    Future<INode::Ptr> ImportChildScene(
        const IScene::ConstPtr& scene, BASE_NS::string_view nodeName, BASE_NS::string_view resourceGroup) override
    {
        return META_EXT_CALL_BASE(INodeImport, ImportChildScene(scene, nodeName, resourceGroup));
    }
    Future<INode::Ptr> ImportChildScene(BASE_NS::string_view uri, BASE_NS::string_view nodeName) override
    {
        return META_EXT_CALL_BASE(INodeImport, ImportChildScene(uri, nodeName));
    }
    Future<INode::Ptr> ImportTemplate(const META_NS::IObjectTemplate::ConstPtr& templ) override
    {
        return META_EXT_CALL_BASE(INodeImport, ImportTemplate(templ));
    }
    bool SetEcsObject(const IEcsObject::Ptr& obj) override
    {
        return META_EXT_CALL_BASE(IEcsObjectAccess, SetEcsObject(obj));
    }
    IEcsObject::Ptr GetEcsObject() const override
    {
        return META_EXT_CALL_BASE(IEcsObjectAccess, GetEcsObject());
    }
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext) override
    {
        return META_EXT_CALL_BASE(IAttach, Attach(attachment, dataContext));
    }
    bool Detach(const IObject::Ptr& attachment) override
    {
        return META_EXT_CALL_BASE(IAttach, Detach(attachment));
    }
    IScene::Ptr GetScene() const override
    {
        return META_EXT_CALL_BASE(INode, GetScene());
    }
    Future<BASE_NS::string> GetUniqueChildName(BASE_NS::string_view name) const override
    {
        return META_EXT_CALL_BASE(INode, GetUniqueChildName(name));
    }

    META_NS::IterationResult Iterate(const META_NS::IterationParameters& params) override
    {
        return META_EXT_CALL_BASE(IIterable, Iterate(params));
    }
    META_NS::IterationResult Iterate(const META_NS::IterationParameters& params) const override
    {
        return META_EXT_CALL_BASE(IIterable, Iterate(params));
    }

    // IContainer
    BASE_NS::vector<IObject::Ptr> GetAll() const override
    {
        return META_EXT_CALL_BASE(IContainer, GetAll());
    }
    IObject::Ptr GetAt(SizeType index) const override
    {
        return META_EXT_CALL_BASE(IContainer, GetAt(index));
    }
    SizeType GetSize() const override
    {
        return META_EXT_CALL_BASE(IContainer, GetSize());
    }
    BASE_NS::vector<IObject::Ptr> FindAll(const FindOptions& options) const override
    {
        return META_EXT_CALL_BASE(IContainer, FindAll(options));
    }
    IObject::Ptr FindAny(const FindOptions& options) const override
    {
        return META_EXT_CALL_BASE(IContainer, FindAny(options));
    }
    IObject::Ptr FindByName(BASE_NS::string_view name) const override
    {
        return META_EXT_CALL_BASE(IContainer, FindByName(name));
    }
    bool Add(const IObject::Ptr& object) override
    {
        return META_EXT_CALL_BASE(IContainer, Add(object));
    }
    bool Insert(SizeType index, const IObject::Ptr& object) override
    {
        return META_EXT_CALL_BASE(IContainer, Insert(index, object));
    }
    bool Remove(SizeType index) override
    {
        return META_EXT_CALL_BASE(IContainer, Remove(index));
    }
    bool Remove(const IObject::Ptr& child) override
    {
        return META_EXT_CALL_BASE(IContainer, Remove(child));
    }
    bool Move(SizeType fromIndex, SizeType toIndex) override
    {
        return META_EXT_CALL_BASE(IContainer, Move(fromIndex, toIndex));
    }
    bool Move(const IObject::Ptr& child, SizeType toIndex) override
    {
        return META_EXT_CALL_BASE(IContainer, Move(child, toIndex));
    }
    bool Replace(const IObject::Ptr& child, const IObject::Ptr& replaceWith, bool addAlways) override
    {
        return META_EXT_CALL_BASE(IContainer, Replace(child, replaceWith, addAlways));
    }
    void RemoveAll() override
    {
        META_EXT_CALL_BASE(IContainer, RemoveAll());
    }
    bool IsAncestorOf(const IObject::ConstPtr& object) const override
    {
        return META_EXT_CALL_BASE(IContainer, IsAncestorOf(object));
    }
    META_NS::IObject::Ptr GetParent(ContainableTag) const override
    {
        return META_EXT_CALL_BASE(IContainable, GetParent());
    }
    void OnMetadataConstructed(const META_NS::StaticMetadata& m, CORE_NS::IInterface& i) override
    {
        return META_EXT_CALL_BASE(IMetadataOwner, OnMetadataConstructed(m, i));
    }
    void OnChildChanged(META_NS::ContainerChangeType type, const INode::Ptr& child, size_t index) override
    {
        META_EXT_CALL_BASE(INodeNotify, OnChildChanged(type, child, index));
    }
    bool IsListening() const override
    {
        return META_EXT_CALL_BASE(INodeNotify, IsListening());
    }
    void OnNodeActiveStateChanged(NodeActiteStateInfo state) override
    {
        META_EXT_CALL_BASE(INodeNotify, OnNodeActiveStateChanged(state));
    }

protected:
    NodeFwd() = default;
    ~NodeFwd() override = default;
    META_NO_COPY_MOVE(NodeFwd)
};

SCENE_END_NAMESPACE()

#endif
