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
#ifndef SCENE_SRC_CORE_INTERNAL_SCENE_H
#define SCENE_SRC_CORE_INTERNAL_SCENE_H

#include <scene/base/types.h>
#include <scene/ext/intf_component_factory.h>
#include <scene/ext/intf_internal_scene.h>
#include <scene/ext/intf_node_notify.h>
#include <scene/interface/intf_scene.h>
#include <scene/interface/scene_options.h>
#include <shared_mutex>

#include <render/intf_render_context.h>

#include <meta/api/make_callback.h>
#include <meta/ext/object.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_startable_controller.h>
#include <meta/interface/intf_task_queue.h>

#include "camera.h"
#include "ecs.h"
#include "intf_internal_raycast.h"

SCENE_BEGIN_NAMESPACE()

class IOnNodeChanged : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IOnNodeChanged, "8a4ea0ec-deb5-487f-8a7b-b55c00ed048a")
public:
    virtual void OnChildChanged(
        META_NS::ContainerChangeType, CORE_NS::Entity parent, CORE_NS::Entity child, size_t index) = 0;
};

class InternalScene : public META_NS::IntroduceInterfaces<IInternalScene, IInternalRayCast, IOnNodeChanged> {
public:
    InternalScene(const IScene::Ptr& scene, IRenderContext::Ptr context, SceneOptions opts);

    ~InternalScene() override;

    void SetSelf(const IInternalScene::Ptr& s)
    {
        self_ = s;
    }

    bool Initialize() override;
    void Uninitialize() override;

    IRenderContext::Ptr GetContext() const override
    {
        return context_;
    }

    INode::Ptr CreateNode(BASE_NS::string_view path, META_NS::ObjectId id) override;
    INode::Ptr FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const override;
    INode::Ptr FindNode(CORE_NS::Entity ent, META_NS::ObjectId id) const override;
    bool ReleaseNode(INode::Ptr&& node, bool recursive) override;
    bool RemoveNode(const INode::Ptr& node) override;
    META_NS::IObject::Ptr CreateObject(META_NS::ObjectId id) override;
    BASE_NS::vector<INode::Ptr> GetChildren(const IEcsObject::Ptr&) const override;
    bool RemoveChild(
        const BASE_NS::shared_ptr<IEcsObject>& object, const BASE_NS::shared_ptr<IEcsObject>& child) override;
    bool AddChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child, size_t index) override;
    IComponent::Ptr CreateEcsComponent(const INode::Ptr& node, BASE_NS::string_view componentName) override;

    BASE_NS::vector<ICamera::Ptr> GetCameras() const override;
    BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations() const override;

    IEcsContext& GetEcsContext() override
    {
        return *ecs_;
    }
    RENDER_NS::IRenderContext& GetRenderContext() override
    {
        return *context_->GetRenderer();
    }
    CORE3D_NS::IGraphicsContext& GetGraphicsContext() override
    {
        return *graphicsContext3D_;
    }

    BASE_NS::shared_ptr<IScene> GetScene() const override;
    void SchedulePropertyUpdate(const IEcsObject::Ptr& obj) override;
    void SyncProperties() override;
    void Update(bool syncProperties = true) override;

    void RegisterComponent(const BASE_NS::Uid&, const IComponentFactory::Ptr&) override;
    void UnregisterComponent(const BASE_NS::Uid&) override;
    BASE_NS::shared_ptr<IComponentFactory> FindComponentFactory(const BASE_NS::Uid&) const override;

    bool SyncProperty(const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir) override;

    void AddRenderingCamera(const IInternalCamera::Ptr& camera) override;
    void RemoveRenderingCamera(const IInternalCamera::Ptr& camera) override;

    bool SetRenderMode(RenderMode) override;
    RenderMode GetRenderMode() const override;
    void RenderFrame() override;
    bool HasPendingRender() const override;

    void ListenNodeChanges(bool enabled) override;
    SceneOptions GetOptions() const override;

public:
    NodeHits CastRay(
        const BASE_NS::Math::Vec3& pos, const BASE_NS::Math::Vec3& dir, const RayCastOptions& options) const override;
    NodeHits CastRay(const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec2& pos,
        const RayCastOptions& options) const override;
    BASE_NS::Math::Vec3 ScreenPositionToWorld(
        const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const override;
    BASE_NS::Math::Vec3 WorldPositionToScreen(
        const IEcsObject::ConstPtr& entity, const BASE_NS::Math::Vec3& pos) const override;

private:
    INode::Ptr ConstructNode(CORE_NS::Entity, META_NS::ObjectId id) const;
    INode::Ptr ConstructNodeImpl(CORE_NS::Entity ent, INode::Ptr node) const;
    IComponent::Ptr CreateComponent(
        CORE_NS::IComponentManager* m, const IEcsObject::Ptr& ecsObject, bool createEcsComponent) const;
    void AttachComponents(const INode::Ptr& node, const IEcsObject::Ptr& ecsObject, CORE_NS::Entity ent) const;
    META_NS::ObjectId DeducePrimaryNodeType(CORE_NS::Entity ent) const;
    void NotifyRenderingCameras();

    NodeHits MapHitResults(const BASE_NS::vector<CORE3D_NS::RayCastResult>& res, const RayCastOptions& options) const;
    bool UpdateSyncProperties(bool resetPending);
    void ReleaseChildNodes(const IEcsObject::Ptr& eobj);

    void OnChildChanged(
        META_NS::ContainerChangeType, CORE_NS::Entity parent, CORE_NS::Entity child, size_t index) override;

    using NodesType = BASE_NS::unordered_map<CORE_NS::Entity, INode::Ptr>;
    INode::Ptr ReleaseCached(NodesType::iterator);

    void SetEntityActive(const BASE_NS::shared_ptr<IEcsObject>&, bool active) override;

private:
    IScene::WeakPtr scene_;
    IInternalScene::WeakPtr self_;
    IRenderContext::Ptr context_;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext3D_;

    SceneOptions options_;

    BASE_NS::unique_ptr<Ecs> ecs_;
    BASE_NS::unordered_map<BASE_NS::Uid, IComponentFactory::Ptr> componentFactories_;

    mutable NodesType nodes_;
    mutable BASE_NS::unordered_map<CORE_NS::Entity, META_NS::IAnimation::WeakPtr> animations_;
    mutable BASE_NS::vector<IInternalCamera::WeakPtr> renderingCameras_;

    uint64_t firstTime_ { ~0u };
    uint64_t previousFrameTime_ { ~0u };
    uint64_t deltaTime_ { 1u };

    RenderMode mode_ { RenderMode::ALWAYS };
    size_t nodeListening_ {};

    BASE_NS::vector<INodeNotify::Ptr> GetNotifiableNodesFromHierarchy(CORE_NS::Entity root);

private: // locked bits
    mutable std::shared_mutex mutex_;
    bool pendingRender_ {};
    BASE_NS::unordered_map<void*, IEcsObject::WeakPtr> syncs_;
};

SCENE_END_NAMESPACE()

#endif
