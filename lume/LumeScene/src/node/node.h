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

#include <scene/ext/component_fwd.h>
#include <scene/ext/intf_ecs_object_access.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_node_import.h>
#include <scene/interface/intf_scene.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>

SCENE_BEGIN_NAMESPACE()

class Node : public META_NS::IntroduceInterfaces<META_NS::MetaObject, INode, INodeImport, IEcsObjectAccess> {
    META_OBJECT(Node, SCENE_NS::ClassId::Node, IntroduceInterfaces)

public:
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec3, Position, "TransformComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Vec3, Scale, "TransformComponent")
    SCENE_USE_COMPONENT_PROPERTY(BASE_NS::Math::Quat, Rotation, "TransformComponent")
    SCENE_USE_COMPONENT_PROPERTY(bool, Enabled, "NodeComponent")

    Future<BASE_NS::string> GetPath() const override;
    Future<INode::Ptr> GetParent() const override;
    Future<BASE_NS::vector<INode::Ptr>> GetChildren() const override;
    Future<bool> RemoveChild(const INode::Ptr&) override;
    Future<bool> AddChild(const INode::Ptr& child, size_t index = -1) override;
    Future<bool> SetName(const BASE_NS::string&) override;

    Future<INode::Ptr> ImportChild(const INode::ConstPtr& node) override;
    Future<INode::Ptr> ImportChildScene(const IScene::ConstPtr& scene, const BASE_NS::string& nodeName) override;

public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    BASE_NS::string GetName() const override;
    bool SetEcsObject(const IEcsObject::Ptr&) override;
    IEcsObject::Ptr GetEcsObject() const override;

    IScene::Ptr GetScene() const override;

protected:
    IEcsObject::Ptr object_;
};

SCENE_END_NAMESPACE()

#endif