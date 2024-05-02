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

#ifndef API_CORE_ECS_IECS_H
#define API_CORE_ECS_IECS_H

#include <cstdint>

#include <base/containers/refcnt_ptr.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/namespace.h>
#include <core/threading/intf_thread_pool.h>

BASE_BEGIN_NAMESPACE()
struct Uid;
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IClassFactory;
class ISystem;
class IComponentManager;
struct SystemTypeInfo;
struct ComponentManagerTypeInfo;

/** \addtogroup group_ecs_iecs
 *  @{
 */
/** IEcs */
class IEcs {
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

    /** Listener for ECS Entity events. */
    class EntityListener {
    public:
        using EventType = IEntityManager::EventType;
        /** Entities added to processing. */
        virtual void OnEntityEvent(EventType type, BASE_NS::array_view<const Entity> entities) = 0;

    protected:
        virtual ~EntityListener() = default;
    };

    /** Listener for ECS Component events. */
    class ComponentListener {
    public:
        enum class EventType : uint8_t {
            /** Component created to entity
             */
            CREATED,
            /** Component in entity modified
             */
            MODIFIED,
            /** Component destroyed from entity
             */
            DESTROYED
        };
        /** Entities added to processing. */
        virtual void OnComponentEvent(
            EventType type, const IComponentManager& componentManager, BASE_NS::array_view<const Entity> entities) = 0;

    protected:
        virtual ~ComponentListener() = default;
    };

    /** Gets entity manager for ECS.
     */
    virtual IEntityManager& GetEntityManager() = 0;

    /** Get components of entity.
        @param entity Entity where we get components.
        @param result List where entitys components are written in.
    */
    virtual void GetComponents(Entity entity, BASE_NS::vector<IComponentManager*>& result) = 0;

    /** Get systems of this ECS.
     */
    virtual BASE_NS::vector<ISystem*> GetSystems() const = 0;

    /** Get one particular system from systems list.
     *  @param uid UID of the system we want to get.
     */
    virtual ISystem* GetSystem(const BASE_NS::Uid& uid) const = 0;

    /** Get component managers of this ECS.
     */
    virtual BASE_NS::vector<IComponentManager*> GetComponentManagers() const = 0;

    /** Get one particular component manager from component managers list.
     *  @param uid UID of the component manager we want to get.
     */
    virtual IComponentManager* GetComponentManager(const BASE_NS::Uid& uid) const = 0;

    /** Clone an entity. Creates a new entity with copies of all components of this entity.
     *  NOTE: Does not clone children of the entity. NodeSystem can be used to clone whole hierarchies
     *  @param entity Entity to be cloned.
     */
    virtual Entity CloneEntity(Entity entity) = 0;

    /** Garbage collect entities.
     */
    virtual void ProcessEvents() = 0;

    /** Starts all systems.
     */
    virtual void Initialize() = 0;

    /** Updates all systems.
     *  @param time Current program time (in us).
     *  @param delta Time it took since last frame (in us).
     *  @return True if rendering is required.
     */
    virtual bool Update(uint64_t time, uint64_t delta) = 0;

    /** Stops all systems and removes all entities and components.
     */
    virtual void Uninitialize() = 0;

    /** Add an instance of "componentManagerTypeInfo" to "system.
    @param componentManagerTypeInfo Component manager type that is added to manager list.
    */
    virtual IComponentManager* CreateComponentManager(const ComponentManagerTypeInfo& componentManagerTypeInfo) = 0;

    /** Add an instance of "systemInfo" to "system".
     *  @param systemInfo System type that is instantiated to systems list.
     */
    virtual ISystem* CreateSystem(const SystemTypeInfo& systemInfo) = 0;

    /** Add listener for Entity events.
     */
    virtual void AddListener(EntityListener& listener) = 0;

    /** Remove Entity event listener.
     */
    virtual void RemoveListener(EntityListener& listener) = 0;

    /** Add listener for global component events.
     */
    virtual void AddListener(ComponentListener& listener) = 0;

    /** Remove global component event listener.
     */
    virtual void RemoveListener(ComponentListener& listener) = 0;

    /** Add listener for specific component manager events.
     */
    virtual void AddListener(IComponentManager& manager, ComponentListener& listener) = 0;

    /** Remove specific component manager event listener.
     */
    virtual void RemoveListener(IComponentManager& manager, ComponentListener& listener) = 0;

    /** Mark ECS dirty and queue current frame for rendering.
     */
    virtual void RequestRender() = 0;

    /** Set render mode, either render only when dirty or render always.
     * @param renderMode Rendering mode.
     */
    virtual void SetRenderMode(RenderMode renderMode) = 0;

    /** Retrieve current render mode.
     */
    virtual RenderMode GetRenderMode() = 0;

    /** Check if scene requires rendering after Update() has been called.
     */
    virtual bool NeedRender() const = 0;

    /** Get access to interface registry
     */
    virtual IClassFactory& GetClassFactory() const = 0;

    /** Get access to a thread pool.
     */
    virtual const IThreadPool::Ptr& GetThreadPool() const = 0;

    /** Get scaling factor used for the delta time passed to ISystem::Update.
     */
    virtual float GetTimeScale() const = 0;

    /** Set scaling factor used for the delta time passed to ISystem::Update.
     * @param scale Time scaling factor.
     */
    virtual void SetTimeScale(float scale) = 0;

    using Ptr = BASE_NS::refcnt_ptr<IEcs>;

protected:
    virtual void Ref() noexcept = 0;
    virtual void Unref() noexcept = 0;

    friend Ptr;

    IEcs() = default;
    IEcs(const IEcs&) = delete;
    IEcs(IEcs&&) = delete;
    IEcs& operator=(const IEcs&) = delete;
    IEcs& operator=(IEcs&&) = delete;
    virtual ~IEcs() = default;
};

// Helper method to fetch managers by name
/** Get manager */
template<class T>
T* GetManager(const CORE_NS::IEcs& ecs)
{
    return static_cast<T*>(ecs.GetComponentManager(T::UID));
}

/** Get system */
template<class T>
T* GetSystem(const CORE_NS::IEcs& ecs)
{
    return static_cast<T*>(ecs.GetSystem(T::UID));
}
/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_ECS_IECS_H
