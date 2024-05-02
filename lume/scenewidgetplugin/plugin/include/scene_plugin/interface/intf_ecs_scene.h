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
#ifndef SCENEPLUGIN_INTF_ECS_SCENE_H
#define SCENEPLUGIN_INTF_ECS_SCENE_H

#include <scene_plugin/interface/intf_ecs_object.h>
#include <scene_plugin/interface/intf_node.h>
#include <scene_plugin/namespace.h>

#include <core/ecs/intf_ecs.h>

#include <meta/base/interface_macros.h>
#include <meta/base/types.h>
#include <meta/interface/intf_task_queue.h>

SCENE_BEGIN_NAMESPACE()

class IAssetManager;

// Implemented by SCENE_NS::ClassId::Scene, not intended to direct application use
REGISTER_INTERFACE(IEcsScene, "d2b4295b-d06d-401e-a82d-b167a3c51a92")
class IEcsScene : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEcsScene, InterfaceId::IEcsScene)
public:
    /**
     * Mode that controls how the engine will be used to render a frame. This can be used to skip rendering altogether
     * when the scene contents have not changed, lowering the GPU and CPU load and saving battery life.
     */
    enum RenderMode {
        /**
         * In addition to render requests also re-render when the scene is detected to be dirty. The scene is
         * considered dirty if any component in an entity has been changed, added or removed or rendering is
         * explicitly requestedthrough RequestRender().
         */
        RENDER_IF_DIRTY,

        /**
         * Render every frame regardless of calls to RequestRender(). This mode will still take v-sync count into
         * account.
         */
        RENDER_ALWAYS,
    };
    META_PROPERTY(uint8_t, RenderMode)

    /**
     * @brief Get Ecs instance. Raw access to ECS. This should be used in the tasks from UI_NS::JobQueues::ENGINE_THREAD
     * @return Strong pointer to ECS. Can be null.
     */
    virtual CORE_NS::IEcs::Ptr GetEcs() = 0;

    class IPrepareSceneForInitialization : public META_NS::ICallable {
    public:
        constexpr static BASE_NS::Uid UID { "d4000af2-3dc2-455e-86f4-3b5124cc026c" };
        using Ptr = BASE_NS::shared_ptr<IPrepareSceneForInitialization>;
        using WeakPtr = BASE_NS::weak_ptr<IPrepareSceneForInitialization>;
        virtual bool Invoke(CORE_NS::IEcs::Ptr ecs) = 0;
        using FunctionType = bool(CORE_NS::IEcs::Ptr ecs);

    protected:
        friend Ptr;
        META_NO_COPY_MOVE_INTERFACE(IPrepareSceneForInitialization)
    };

    /**
     * @brief Set Ecs instance. Raw access to ECS. Previous instance is destroyed and the existing scene
     * will be reset. Runs ecs initialization with the given instance.
     * @param ecs The instance to (re)set.
     */
    virtual void SetEcsInitializationCallback(IPrepareSceneForInitialization::WeakPtr callback) = 0;

    /**
     * @brief Create new node or get existing object from the scene cache.
     * @param path to Ecs object on engine scene
     * @return Always returns object, if the object does not exist on scene yet, the assumption is that it will be
     * created by someone else.
     */
    virtual IEcsObject::Ptr GetEcsObject(const BASE_NS::string_view& path) = 0;

    /**
     * @brief Update EcsObject on existing node
     * @param node Node instance
     * @param name including the full path on esc
     * @param createEngineObject boolean value to define if the object should be created by function or someone else
     */
    virtual void BindNodeToEcs(
        SCENE_NS::INode::Ptr& node, const BASE_NS::string_view name, bool createEngineObject) = 0;

    /**
     * @brief Queue task in Engine job queue. This may use other thread to run the task.
     * @param task TaskQueue-item
     * @return a token that can be used to cancel the task before it runs (again)
     */
    virtual META_NS::ITaskQueue::Token AddEngineTask(
        const META_NS::ITaskQueue::CallableType::Ptr& task, bool runDeferred) = 0;

    /**
     * @brief Queue task in Application job queue. This may use other thread to run the task.
     * @param task TaskQueue-item
     * @return a token that can be used to cancel the task before it runs (again)
     */
    virtual META_NS::ITaskQueue::Token AddApplicationTask(
        const META_NS::ITaskQueue::CallableType::Ptr& task, bool runDeferred) = 0;

    /**
     * @brief Cancel Engine Task
     * @param token that was received when task was added
     */
    virtual void CancelEngineTask(META_NS::ITaskQueue::Token token) = 0;

    /**
     * @brief Cancel App Task
     * @param token that was received when task was added
     */
    virtual void CancelAppTask(META_NS::ITaskQueue::Token token) = 0;

    /**
     * @brief Retrieve root entity collection with scene contents.
     */
    virtual IEntityCollection* GetEntityCollection() = 0;

    /**
     * @brief Retrieve asset manager for the scene.
     */
    virtual IAssetManager* GetAssetManager() = 0;
};
SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IEcsScene::WeakPtr);
META_TYPE(SCENE_NS::IEcsScene::Ptr);

#endif // SCENEPLUGIN_INTF_ECS_SCENE_H
