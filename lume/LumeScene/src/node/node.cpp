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

#include "node.h"

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/util.h>

#include "core/ecs.h"

SCENE_BEGIN_NAMESPACE()

bool Node::Build(const META_NS::IMetadata::Ptr& d)
{
    return Super::Build(d);
}

BASE_NS::string Node::GetName() const
{
    return object_ ? BASE_NS::string(EntityName(object_->GetPath())) : "";
}

bool Node::SetEcsObject(const IEcsObject::Ptr& o)
{
    object_ = o;
    return true;
}

IEcsObject::Ptr Node::GetEcsObject() const
{
    return object_;
}

IScene::Ptr Node::GetScene() const
{
    return object_->GetScene()->GetScene();
}

Future<INode::Ptr> Node::GetParent() const
{
    if (auto s = object_->GetScene()) {
        return s->AddTask([=] {
            if (s->GetEcsContext().GetRootEntity() == object_->GetEntity()) {
                return INode::Ptr {};
            }
            return s->FindNode(ParentPath(object_->GetPath()), {});
        });
    }
    return {};
}

Future<BASE_NS::vector<INode::Ptr>> Node::GetChildren() const
{
    if (auto s = object_->GetScene()) {
        return s->AddTask([=] { return s->GetChildren(object_); });
    }
    return {};
}

Future<bool> Node::RemoveChild(const INode::Ptr& child)
{
    if (auto s = object_->GetScene()) {
        if (auto ecsobj = interface_pointer_cast<IEcsObjectAccess>(child)) {
            return s->AddTask([=] { return s->RemoveChild(object_, ecsobj->GetEcsObject()); });
        }
    }
    return {};
}
Future<bool> Node::AddChild(const INode::Ptr& child, size_t index)
{
    if (auto s = object_->GetScene()) {
        return s->AddTask([=] { return s->AddChild(object_, child, index); });
    }
    return {};
}
Future<bool> Node::SetName(const BASE_NS::string& name)
{
    if (auto s = object_->GetScene()) {
        return s->AddTask([=] { return object_->SetName(name); });
    }
    return {};
}
Future<BASE_NS::string> Node::GetPath() const
{
    if (auto s = object_->GetScene()) {
        return s->AddTask([=] { return object_->GetPath(); });
    }
    return {};
}

Future<INode::Ptr> Node::ImportChild(const INode::ConstPtr& node)
{
    IEcsObject::Ptr eobj;
    if (auto i = interface_cast<IEcsObjectAccess>(node)) {
        eobj = i->GetEcsObject();
    }
    if (eobj) {
        if (auto s = object_->GetScene()) {
            return s->AddTask([=] {
                auto ent = CopyExternalAsChild(*object_, *eobj);
                return ent ? s->FindNode(ent, {}) : nullptr;
            });
        }
    }
    return {};
}

Future<INode::Ptr> Node::ImportChildScene(const IScene::ConstPtr& scene, const BASE_NS::string& nodeName)
{
    if (auto s = object_->GetScene()) {
        return s->AddTask([=] {
            auto ent = CopyExternalAsChild(*object_, *scene);
            auto node = ent ? s->FindNode(ent, {}) : nullptr;
            if (node) {
                node->SetName(nodeName);
            }
            return node;
        });
    }
    return {};
}

SCENE_END_NAMESPACE()