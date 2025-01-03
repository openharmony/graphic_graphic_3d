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

#include <scene/ext/ecs_lazy_property_fwd.h>
#include <scene/ext/scene_property.h>
#include <scene/interface/intf_raycast.h>
#include <scene/interface/intf_scene.h>

#include <meta/ext/implementation_macros.h>
#include <meta/ext/object.h>
#include <meta/interface/intf_task_queue.h>

#include "core/internal_scene.h"

SCENE_BEGIN_NAMESPACE()

class SceneObject : public META_NS::IntroduceInterfaces<META_NS::MetaObject, IScene, IEnginePropertyInit, IRayCast> {
    META_OBJECT(SceneObject, SCENE_NS::ClassId::Scene, IntroduceInterfaces)

public:
    SceneObject() = default;
    ~SceneObject();

    META_BEGIN_STATIC_DATA()
    SCENE_STATIC_DYNINIT_PROPERTY_DATA(IScene, SCENE_NS::IRenderConfiguration::Ptr, RenderConfiguration, "")
    META_END_STATIC_DATA()

    META_IMPLEMENT_READONLY_PROPERTY(IRenderConfiguration::Ptr, RenderConfiguration)

public:
    bool Build(const META_NS::IMetadata::Ptr&) override;
    bool InitDynamicProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;
    bool AttachEngineProperty(const META_NS::IProperty::Ptr& p, BASE_NS::string_view path) override;

    Future<BASE_NS::vector<ICamera::Ptr>> GetCameras() const override;
    Future<BASE_NS::vector<META_NS::IAnimation::Ptr>> GetAnimations() const override;

    Future<INode::Ptr> GetRootNode() const override;
    Future<INode::Ptr> CreateNode(const BASE_NS::string_view path, META_NS::ObjectId id) override;
    Future<INode::Ptr> FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const override;
    Future<void> ReleaseNode(const INode::Ptr& node) override;

    using Super::CreateObject;
    Future<META_NS::IObject::Ptr> CreateObject(META_NS::ObjectId id) override;

    void StartAutoUpdate(META_NS::TimeSpan interval) override;

    Future<bool> SetRenderMode(RenderMode) override;
    Future<RenderMode> GetRenderMode() const override;
    Future<NodeHits> CastRay(
        const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const override;
public:
    IInternalScene::Ptr GetInternalScene() const override;

private:
    IInternalScene::Ptr internal_;
    META_NS::ITaskQueue::Token updateTask_ {};
};

SCENE_END_NAMESPACE()

#endif