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

#ifndef SCENE_EXT_IINTERNAL_SCENE_H
#define SCENE_EXT_IINTERNAL_SCENE_H

#include <scene/ext/intf_component.h>
#include <scene/ext/intf_internal_camera.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_node.h>
#include <scene/interface/intf_render_context.h>
#include <scene/interface/resource/resource_group_bundle.h>
#include <scene/interface/scene_options.h>

#include <3d/intf_graphics_context.h>
#include <render/intf_render_context.h>

#include <meta/api/task_queue.h>
#include <meta/ext/object.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/engine/intf_engine_value_manager.h>
#include <meta/interface/intf_event.h>
#include <meta/interface/intf_startable_controller.h>

SCENE_BEGIN_NAMESPACE()

class IScene;
class IEcsObject;
class IComponentFactory;
class IEcsContext;

class IInternalSceneCore : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalSceneCore, "55523d8b-b1db-45a3-a0fc-64a53551d779")
public:
    virtual bool Initialize() = 0;
    virtual void Uninitialize() = 0;

    virtual void RegisterComponent(const BASE_NS::Uid&, const BASE_NS::shared_ptr<IComponentFactory>&) = 0;
    virtual void UnregisterComponent(const BASE_NS::Uid&) = 0;
    virtual BASE_NS::shared_ptr<IComponentFactory> FindComponentFactory(const BASE_NS::Uid&) const = 0;

    virtual IRenderContext::Ptr GetContext() const = 0;

    virtual IEcsContext& GetEcsContext() = 0;
    virtual BASE_NS::shared_ptr<IScene> GetScene() const = 0;
    virtual RENDER_NS::IRenderContext& GetRenderContext() = 0;
    virtual CORE3D_NS::IGraphicsContext& GetGraphicsContext() = 0;

    virtual void SchedulePropertyUpdate(const META_NS::IEnginePropertySync::Ptr& obj) = 0;
    virtual void SyncProperties() = 0;
    struct UpdateInfo {
        bool syncProperties { true }; /// If true, synchronize properties between ECS
        META_NS::IClock::Ptr clock;   /// Optional clock to use for ECS update
    };
    virtual void Update(const UpdateInfo& info) = 0;
    /// Updates using an internal clock. Update(const UpdateInfo& info) is preferred.
    virtual void Update(bool syncProperties = true) = 0;

    virtual bool SyncProperty(const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir) = 0;
    virtual void ListenNodeChanges(bool enabled) = 0;

    virtual SceneOptions GetOptions() const = 0;
    virtual ResourceGroupBundle GetResourceGroups() const = 0;
    virtual void SetResourceGroups(ResourceGroupBundle) = 0;

public:
    template<typename Func>
    [[deprecated(
        "use correct variant instead AddTaskFuture,AddTaskOrRunDirectly,RunDirectlyOrInTask")]] [[nodiscard]] auto
    AddTask(Func&& func)
    {
        return AddTaskOrRunDirectly(BASE_NS::forward<Func>(func));
    }
    template<typename Func>
    [[deprecated(
        "use correct variant instead AddTaskFuture,AddTaskOrRunDirectly,RunDirectlyOrInTask")]] [[nodiscard]] auto
    AddTask(Func&& func, const META_NS::ITaskQueue::Ptr& queue)
    {
        return AddTaskOrRunDirectly(BASE_NS::forward<Func>(func), queue);
    }

    template<typename Func>
    [[nodiscard]] auto AddTaskFuture(Func&& func) const
    {
        return AddTaskFuture(BASE_NS::forward<Func>(func), GetContext()->GetRenderQueue());
    }
    template<typename Func>
    [[nodiscard]] auto AddTaskFuture(Func&& func, const META_NS::ITaskQueue::Ptr& queue) const
    {
        return META_NS::Future<META_NS::PlainType_t<decltype(func())>>(
            queue->AddWaitableTask(META_NS::CreateWaitableTask(BASE_NS::move(func))));
    }

    template<typename Func>
    [[nodiscard]] auto AddTaskOrRunDirectly(Func&& func)
    {
        return GetContext()->AddTaskOrRunDirectly(BASE_NS::forward<Func>(func));
    }
    template<typename Func>
    [[nodiscard]] auto AddTaskOrRunDirectly(Func&& func, const META_NS::ITaskQueue::Ptr& queue)
    {
        return GetContext()->AddTaskOrRunDirectly(BASE_NS::forward<Func>(func), queue);
    }
    template<typename Func>
    auto RunDirectlyOrInTask(Func&& func)
    {
        return GetContext()->RunDirectlyOrInTask(BASE_NS::forward<Func>(func));
    }
    template<typename Func>
    auto RunDirectlyOrInTask(Func&& func, const META_NS::ITaskQueue::Ptr& queue)
    {
        return GetContext()->RunDirectlyOrInTask(BASE_NS::forward<Func>(func), queue);
    }

    template<typename Func>
    void PostUserNotification(Func&& func)
    {
        GetContext()->PostUserNotification(BASE_NS::forward<Func>(func));
    }
    template<typename MyEvent, typename... Args>
    void InvokeUserNotification(const META_NS::IEvent::Ptr& event, Args... args)
    {
        GetContext()->InvokeUserNotification<MyEvent>(event, BASE_NS::move(args)...);
    }
};

class SceneDebugInfo;

class IInternalScene : public IInternalSceneCore {
    META_INTERFACE(IInternalSceneCore, IInternalScene, "24c69cfe-1d25-4546-a5cb-b63c2262065e")
public:
    /// Create object, if the entity is invalid, new one is created
    virtual META_NS::IObject::Ptr CreateObject(META_NS::ObjectId id, CORE_NS::Entity) const = 0;
    META_NS::IObject::Ptr CreateObject(META_NS::ObjectId id) const
    {
        return CreateObject(id, CORE_NS::Entity {});
    }
    virtual META_NS::IObject::Ptr CreateObject(META_NS::ObjectId id, const CORE_NS::ResourceId&) const = 0;

    //-- node handling
    virtual INode::Ptr CreateNode(BASE_NS::string_view path, META_NS::ObjectId id) = 0;
    virtual INode::Ptr CreateNode(const INode::ConstPtr& parent, BASE_NS::string_view name, META_NS::ObjectId id) = 0;
    virtual INode::Ptr FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const = 0;
    virtual INode::Ptr FindNode(CORE_NS::Entity, META_NS::ObjectId id) const = 0;
    virtual BASE_NS::vector<INode::Ptr> FindNamedNodes(BASE_NS::string_view name, size_t maxCount,
        const INode::Ptr& root, META_NS::ObjectId id, META_NS::TraversalType traversalType) const = 0;
    virtual INode::Ptr GetRootNode() const = 0;
    /// Return a unique name for a child of given node entity
    virtual BASE_NS::string GetUniqueName(BASE_NS::string_view name, CORE_NS::Entity parent) const = 0;

    virtual uint32_t ReleaseNode(INode::Ptr&& node, bool recursive) = 0;
    virtual bool RemoveNode(INode::Ptr&& node, bool removeFromIndex = true) = 0;
    virtual bool RemoveObject(META_NS::IObject::Ptr&& object, bool removeFromIndex = true) = 0;

    virtual BASE_NS::vector<INode::Ptr> GetChildren(const BASE_NS::shared_ptr<IEcsObject>&) const = 0;
    virtual bool RemoveChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child) = 0;
    virtual bool AddChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child, size_t index) = 0;
    //--

    virtual void SetEntityActive(const BASE_NS::shared_ptr<IEcsObject>&, bool active) = 0;

    virtual IComponent::Ptr CreateEcsComponent(const INode::Ptr& node, BASE_NS::string_view componentName) = 0;

    virtual BASE_NS::vector<ICamera::Ptr> GetCameras() const = 0;
    virtual BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations() const = 0;
    virtual META_NS::IAnimation::Ptr FindAnimation(CORE_NS::Entity ent) const = 0;
    virtual META_NS::IAnimation::Ptr FindAnimation(const CORE_NS::ResourceId&) const = 0;

    virtual void AddRenderingCamera(const IInternalCamera::Ptr& camera) = 0;
    virtual void RemoveRenderingCamera(const IInternalCamera::Ptr& camera) = 0;

    virtual bool SetRenderMode(RenderMode) = 0;
    virtual RenderMode GetRenderMode() const = 0;
    virtual void RenderFrame() = 0;
    virtual bool HasPendingRender() const = 0;

    // Startable handling
    virtual void StartAllStartables(META_NS::IStartableController::ControlBehavior behavior) = 0;
    virtual void StopAllStartables(META_NS::IStartableController::ControlBehavior behavior) = 0;

    virtual SceneDebugInfo GetDebugInfo() const = 0;
};

[[nodiscard]] inline Future<bool> SyncPropertyFuture(const IInternalScene::Ptr& scene,
    const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir = META_NS::EngineSyncDirection::AUTO)
{
    return scene->AddTaskFuture([=] { return scene->SyncProperty(p, dir); });
}

[[nodiscard]] inline Future<bool> SyncPropertyDirect(const IInternalScene::Ptr& scene,
    const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir = META_NS::EngineSyncDirection::AUTO)
{
    return scene->AddTaskOrRunDirectly([=] { return scene->SyncProperty(p, dir); });
}

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IInternalScene)

#endif
