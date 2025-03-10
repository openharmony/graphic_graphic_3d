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
#include <scene/interface/intf_scene.h>
#include <shared_mutex>

#include <render/intf_render_context.h>

#include <meta/api/make_callback.h>
#include <meta/ext/object.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/intf_task_queue.h>

#include "camera.h"
#include "ecs.h"
#include "ecs_object.h"
#include "intf_internal_raycast.h"

SCENE_BEGIN_NAMESPACE()

class InternalScene : public META_NS::IntroduceInterfaces<IInternalScene, IInternalRayCast> {
public:
    InternalScene(const IScene::Ptr& scene, META_NS::ITaskQueue::Ptr engine, META_NS::ITaskQueue::Ptr app,
        BASE_NS::shared_ptr<RENDER_NS::IRenderContext> context);

    ~InternalScene() override;

    void SetSelf(const IInternalScene::Ptr& s)
    {
        self_ = s;
    }

    bool Initialize() override;
    void Uninitialize() override;

    META_NS::ITaskQueue::Ptr GetEngineTaskQueue() const override;
    META_NS::ITaskQueue::Ptr GetAppTaskQueue() const override;

    INode::Ptr CreateNode(BASE_NS::string_view path, META_NS::ObjectId id) override;
    INode::Ptr FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const override;
    INode::Ptr FindNode(CORE_NS::Entity ent, META_NS::ObjectId id) const override;
    void ReleaseNode(const INode::Ptr& node) override;
    META_NS::IObject::Ptr CreateObject(META_NS::ObjectId id) override;
    BASE_NS::vector<INode::Ptr> GetChildren(const IEcsObject::Ptr&) const override;
    bool RemoveChild(
        const BASE_NS::shared_ptr<IEcsObject>& object, const BASE_NS::shared_ptr<IEcsObject>& child) override;
    bool AddChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child, size_t index) override;

    BASE_NS::vector<ICamera::Ptr> GetCameras() const override;
    BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations() const override;

    IEcsContext& GetEcsContext() override
    {
        return *ecs_;
    }
    RENDER_NS::IRenderContext& GetRenderContext() override
    {
        return *renderContext_;
    }
    CORE3D_NS::IGraphicsContext& GetGraphicsContext() override
    {
        return *graphicsContext3D_;
    }

    BASE_NS::shared_ptr<IScene> GetScene() const override;
    void SchedulePropertyUpdate(const IEcsObject::Ptr& obj) override;
    void SyncProperties() override;
    void Update() override;

    void RegisterComponent(const BASE_NS::Uid&, const IComponentFactory::Ptr&) override;
    void UnregisterComponent(const BASE_NS::Uid&) override;
    BASE_NS::shared_ptr<IComponentFactory> FindComponentFactory(const BASE_NS::Uid&) const override;

    bool SyncProperty(const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir) override;

    void AddRenderingCamera(const IInternalCamera::Ptr& camera) override;
    void RemoveRenderingCamera(const IInternalCamera::Ptr& camera) override;

    bool SetRenderMode(RenderMode) override;
    RenderMode GetRenderMode() const override;
    void SetSystemGraphUri(const BASE_NS::string& uri) override;
    BASE_NS::string GetSystemGraphUri() override;

#ifdef __PHYSICS_MODULE__
    void SetCustomRngGroup() override;
    void SetCustomRngGroupUri(const Core3D::IRenderUtil::CustomRngGroup& rngGroup) override;
    const Core3D::IRenderUtil::CustomRngGroup& GetCustomRngGroupUri() const override;
#endif

    void AppendCustomRenderNodeGraph(RENDER_NS::RenderHandleReference rng) override;
    void RenderFrame() override;
    bool HasPendingRender() const override;

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
    IComponent::Ptr CreateComponent(CORE_NS::IComponentManager* m, const IEcsObject::Ptr& ecsObject) const;
    void AttachComponents(const INode::Ptr& node, const IEcsObject::Ptr& ecsObject, CORE_NS::Entity ent) const;
    META_NS::ObjectId DeducePrimaryNodeType(CORE_NS::Entity ent) const;
    void RecursiveRemoveNodes(const INode::Ptr& node);
    void NotifyRenderingCameras();

    NodeHits MapHitResults(const BASE_NS::vector<CORE3D_NS::RayCastResult>& res, const RayCastOptions& options) const;
    bool UpdateSyncProperties(bool resetPending);

private:
    IScene::WeakPtr scene_;
    IInternalScene::WeakPtr self_;
    META_NS::ITaskQueue::Ptr engineQueue_;
    META_NS::ITaskQueue::Ptr appQueue_;
    BASE_NS::shared_ptr<RENDER_NS::IRenderContext> renderContext_;
    CORE3D_NS::IGraphicsContext::Ptr graphicsContext3D_;

    BASE_NS::unique_ptr<Ecs> ecs_;
    BASE_NS::unordered_map<BASE_NS::Uid, IComponentFactory::Ptr> componentFactories_;

    mutable BASE_NS::unordered_map<CORE_NS::Entity, INode::Ptr> nodes_;
    mutable BASE_NS::unordered_map<CORE_NS::Entity, META_NS::IAnimation::WeakPtr> animations_;
    mutable BASE_NS::vector<IInternalCamera::WeakPtr> renderingCameras_;

    uint64_t firstTime_ { ~0u };
    uint64_t previousFrameTime_ { ~0u };
    uint64_t deltaTime_ { 1u };
    RenderMode mode_ { RenderMode::ALWAYS };
    BASE_NS::string systemGraph_ = "rofs3D://systemGraph.json";
    BASE_NS::vector<RENDER_NS::RenderHandleReference> customRenderNodeGraphs_;
#ifdef __PHYSICS_MODULE__
    Core3D::IRenderUtil::CustomRngGroup customRngGroup_;
#endif

private: // locked bits
    mutable std::shared_mutex mutex_;
    bool pendingRender_ {};
    BASE_NS::unordered_map<void*, IEcsObject::WeakPtr> syncs_;
};

SCENE_END_NAMESPACE()

#endif
