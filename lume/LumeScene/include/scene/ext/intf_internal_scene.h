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

#include <scene/ext/intf_internal_camera.h>
#include <scene/interface/intf_camera.h>
#include <scene/interface/intf_node.h>

#include <3d/intf_graphics_context.h>
#include <render/intf_render_context.h>

#include <meta/api/task_queue.h>
#include <meta/ext/object.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/engine/intf_engine_value.h>

SCENE_BEGIN_NAMESPACE()

class IScene;
class IEcsObject;
class IComponentFactory;
class IEcsContext;

class IInternalScene : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IInternalScene, "24c69cfe-1d25-4546-a5cb-b63c2262065e")
public:
    virtual bool Initialize() = 0;
    virtual void Uninitialize() = 0;

    virtual void RegisterComponent(const BASE_NS::Uid&, const BASE_NS::shared_ptr<IComponentFactory>&) = 0;
    virtual void UnregisterComponent(const BASE_NS::Uid&) = 0;
    virtual BASE_NS::shared_ptr<IComponentFactory> FindComponentFactory(const BASE_NS::Uid&) const = 0;

    virtual META_NS::ITaskQueue::Ptr GetEngineTaskQueue() const = 0;
    virtual META_NS::ITaskQueue::Ptr GetAppTaskQueue() const = 0;

    //-- node handling
    virtual INode::Ptr CreateNode(BASE_NS::string_view path, META_NS::ObjectId id) = 0;
    virtual INode::Ptr FindNode(BASE_NS::string_view path, META_NS::ObjectId id) const = 0;
    virtual INode::Ptr FindNode(CORE_NS::Entity, META_NS::ObjectId id) const = 0;

    virtual void ReleaseNode(const INode::Ptr& node) = 0;
    virtual META_NS::IObject::Ptr CreateObject(META_NS::ObjectId id) = 0;
    virtual BASE_NS::vector<INode::Ptr> GetChildren(const BASE_NS::shared_ptr<IEcsObject>&) const = 0;
    virtual bool RemoveChild(
        const BASE_NS::shared_ptr<IEcsObject>& object, const BASE_NS::shared_ptr<IEcsObject>& child) = 0;
    virtual bool AddChild(const BASE_NS::shared_ptr<IEcsObject>& object, const INode::Ptr& child, size_t index) = 0;
    //--

    virtual BASE_NS::vector<ICamera::Ptr> GetCameras() const = 0;
    virtual BASE_NS::vector<META_NS::IAnimation::Ptr> GetAnimations() const = 0;

    virtual IEcsContext& GetEcsContext() = 0;
    virtual BASE_NS::shared_ptr<IScene> GetScene() const = 0;
    virtual RENDER_NS::IRenderContext& GetRenderContext() = 0;
    virtual CORE3D_NS::IGraphicsContext& GetGraphicsContext() = 0;

    virtual void SchedulePropertyUpdate(const BASE_NS::shared_ptr<IEcsObject>& obj) = 0;
    virtual void SyncProperties() = 0;
    virtual void Update() = 0;

    virtual bool SyncProperty(const META_NS::IProperty::ConstPtr& p, META_NS::EngineSyncDirection dir) = 0;

    virtual void AddRenderingCamera(const IInternalCamera::Ptr& camera) = 0;
    virtual void RemoveRenderingCamera(const IInternalCamera::Ptr& camera) = 0;

    virtual bool SetRenderMode(RenderMode) = 0;
    virtual RenderMode GetRenderMode() const = 0;

public:
    template<typename Func>
    auto AddTask(Func&& func)
    {
        return AddTask(BASE_NS::forward<Func>(func), GetEngineTaskQueue());
    }
    template<typename Func>
    auto AddTask(Func&& func, const META_NS::ITaskQueue::Ptr& queue)
    {
        return META_NS::AddFutureTaskOrRunDirectly(queue, BASE_NS::forward<Func>(func));
    }
};

inline Future<bool> SyncProperty(const IInternalScene::Ptr& scene, const META_NS::IProperty::ConstPtr& p,
    META_NS::EngineSyncDirection dir = META_NS::EngineSyncDirection::AUTO)
{
    return scene->AddTask([=] { return scene->SyncProperty(p, dir); });
}

SCENE_END_NAMESPACE()

META_INTERFACE_TYPE(SCENE_NS::IInternalScene)

#endif