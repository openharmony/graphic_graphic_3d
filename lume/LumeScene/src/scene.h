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

#ifndef SCENE_SRC_SCENE_H
#define SCENE_SRC_SCENE_H

#include <scene/ext/ecs_lazy_property.h>
#include <scene/ext/scene_property.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/resource/types.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/ext/resource/resource.h>
#include <meta/interface/intf_task_queue.h>

#include "core/internal_scene.h"
#include "resource/types/scene_type.h"

SCENE_BEGIN_NAMESPACE()

class SceneObject : public META_NS::IntroduceInterfaces<META_NS::MetaObject, META_NS::Resource, META_NS::INamed, IScene,
                        IEnginePropertyInit, IRayCast> {
    META_OBJECT(SceneObject, SCENE_NS::ClassId::Scene, IntroduceInterfaces)

public:
    SceneObject() = default;
    ~SceneObject();

    META_BEGIN_STATIC_DATA()
    // No serialization for RenderConfiguration, attached to root node (and serialized along with that)
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IScene, SCENE_NS::IRenderConfiguration::Ptr, RenderConfiguration, "")
    META_STATIC_PROPERTY_DATA(META_NS::INamed, BASE_NS::string, Name)
    META_END_STATIC_DATA()

    META_IMPLEMENT_READONLY_PROPERTY(IRenderConfiguration::Ptr, RenderConfiguration)
    META_IMPLEMENT_PROPERTY(BASE_NS::string, Name)

public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    BASE_NS::string GetName() const override
    {
        return META_NS::GetValue(Name());
    }

    IObject::Ptr Resolve(const META_NS::RefUri& uri) const override
    {
        if (uri.HasSegment()) {
            auto ref = uri;
            auto node = ref.TakeFirstNode();
            if (node.type == META_NS::RefUri::Node::OBJECT ||
                (node.type == META_NS::RefUri::Node::SPECIAL && node.name == "@")) {
                BASE_NS::string name = node.type == META_NS::RefUri::Node::SPECIAL ? "" : node.name;
                auto root = interface_pointer_cast<META_NS::IObjectInstance>(FindNode("/" + name, {}).GetResult());
                return root ? root->Resolve(ref) : nullptr;
            }
        }
        return Super::Resolve(uri);
    }

    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    bool AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    Future<BASE_NS::vector<ICamera::Ptr>> GetCameras() const override;
    Future<BASE_NS::vector<META_NS::IAnimation::Ptr>> GetAnimations() const override;

    Future<INode::Ptr> GetRootNode() const override;
    Future<INode::Ptr> CreateNode(const BASE_NS::string_view path, META_NS::ObjectId id) override;
    Future<INode::Ptr> CreateNode(
        const INode::ConstPtr& parent, BASE_NS::string_view name, META_NS::ObjectId id) override;
    Future<INode::Ptr> FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const override;
    Future<BASE_NS::vector<INode::Ptr>> FindNamedNodes(const FindNamedNodeParams& params) const override;
    Future<uint32_t> ReleaseNode(INode::Ptr&& node, bool recursive) override;
    Future<bool> RemoveNode(INode::Ptr&& node) override;
    Future<bool> RemoveObject(META_NS::IObject::Ptr&& object, const RemoveObjectOptions& options) override;

    IComponent::Ptr GetComponent(const INode::Ptr& node, BASE_NS::string_view componentName) const override;
    Future<IComponent::Ptr> CreateComponent(const INode::Ptr& node, BASE_NS::string_view componentName) override;

    using Super::CreateObject;
    Future<META_NS::IObject::Ptr> CreateObject(META_NS::ObjectId id) override;

    void StartAutoUpdate(META_NS::TimeSpan interval) override;

    Future<bool> SetRenderMode(RenderMode) override;
    Future<RenderMode> GetRenderMode() const override;

    Future<NodeHits> CastRay(
        const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const override;

    ResourceGroupBundle GetResourceGroups() const override;

public:
    IInternalScene::Ptr GetInternalScene() const override;

private:
    CORE_NS::ResourceType GetResourceType() const override
    {
        return ClassId::SceneResource.Id().ToUid();
    }

private:
    IInternalScene::Ptr internal_;
    META_NS::ITaskQueue::Token updateTask_ {};
};

SCENE_END_NAMESPACE()

#endif
