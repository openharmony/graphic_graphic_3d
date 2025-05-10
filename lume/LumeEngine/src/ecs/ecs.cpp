/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include <algorithm>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/atomics.h>
#include <base/containers/iterator.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <base/util/uid_util.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_system.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/plugin/intf_plugin.h>
#include <core/plugin/intf_plugin_register.h>
#include <core/threading/intf_thread_pool.h>

#include "ecs/entity_manager.h"

CORE_BEGIN_NAMESPACE()
constexpr bool operator==(const CORE_NS::SystemTypeInfo* info, const BASE_NS::Uid& uid) noexcept
{
    return info->uid == uid;
}

inline bool operator==(
    const BASE_NS::unique_ptr<ISystem, SystemTypeInfo::DestroySystemFn>& ptr, const ISystem* system) noexcept
{
    return ptr.get() == system;
}

namespace {
using BASE_NS::array_view;
using BASE_NS::pair;
using BASE_NS::Uid;
using BASE_NS::unique_ptr;
using BASE_NS::unordered_map;
using BASE_NS::vector;

class Ecs final : public IEcs, IPluginRegister::ITypeInfoListener {
public:
    Ecs(IClassFactory&, const IThreadPool::Ptr& threadPool, uint64_t ecsId);
    ~Ecs() override;

    Ecs(const Ecs&) = delete;
    Ecs(const Ecs&&) = delete;
    Ecs& operator=(const Ecs&) = delete;
    Ecs& operator=(const Ecs&&) = delete;

    IEntityManager& GetEntityManager() override;
    const IEntityManager& GetEntityManager() const override;
    void GetComponents(Entity entity, vector<IComponentManager*>& result) const override;
    vector<ISystem*> GetSystems() const override;
    ISystem* GetSystem(const Uid& uid) const override;
    vector<IComponentManager*> GetComponentManagers() const override;
    IComponentManager* GetComponentManager(const Uid& uid) const override;
    Entity CloneEntity(Entity entity) override;
    void ProcessEvents() override;

    void Initialize() override;
    bool Update(uint64_t time, uint64_t delta) override;
    void Uninitialize() override;

    IComponentManager* CreateComponentManager(const ComponentManagerTypeInfo& componentManagerTypeInfo) override;
    ISystem* CreateSystem(const SystemTypeInfo& systemInfo) override;

    void AddListener(EntityListener& listener) override;
    void RemoveListener(EntityListener& listener) override;
    void AddListener(ComponentListener& listener) override;
    void RemoveListener(ComponentListener& listener) override;
    void AddListener(IComponentManager& manager, ComponentListener& listener) override;
    void RemoveListener(IComponentManager& manager, ComponentListener& listener) override;

    void RequestRender() override;
    void SetRenderMode(RenderMode renderMode) override;
    RenderMode GetRenderMode() override;

    bool NeedRender() const override;

    IClassFactory& GetClassFactory() const override;

    const IThreadPool::Ptr& GetThreadPool() const override;

    float GetTimeScale() const override;
    void SetTimeScale(float scale) override;

    void Ref() noexcept override;
    void Unref() noexcept override;

    uint64_t GetId() const override;

    using SystemPtr = unique_ptr<ISystem, SystemTypeInfo::DestroySystemFn>;
    using ManagerPtr = unique_ptr<IComponentManager, ComponentManagerTypeInfo::DestroyComponentManagerFn>;

protected:
    // IPluginRegister::ITypeInfoListener
    void OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos) override;

    void ProcessComponentEvents(
        IEcs::ComponentListener::EventType eventType, array_view<const Entity> removedEntities) const;

    void CleanupComponentManager(IComponentManager& manager);

    IThreadPool::Ptr threadPool_;

    // for storing systems and component managers in creation order
    vector<SystemPtr> systemOrder_;
    vector<ManagerPtr> managerOrder_;
    // for finding systems and component managers with UID
    unordered_map<Uid, ISystem*> systems_;
    unordered_map<Uid, IComponentManager*> managers_;

    vector<EntityListener*> entityListeners_;
    vector<ComponentListener*> componentListeners_;
    unordered_map<IComponentManager*, vector<ComponentListener*>> componentManagerListeners_;

    bool initialized_ { false };
    bool needRender_ { false };
    bool renderRequested_ { false };
    bool processingEvents_ { false };
    RenderMode renderMode_ { RENDER_ALWAYS };

    IClassFactory& pluginRegistry_;

    vector<pair<PluginToken, const IEcsPlugin*>> plugins_;
    float timeScale_ { 1.f };
    int32_t refcnt_ { 0 };

    uint64_t ecsId_ { 0xFFFFFFFFffffffff };
    EntityManager entityManager_;
};

template<typename ListType, typename ValueType>
auto Find(ListType& list, const ValueType& value)
{
    return std::find(list.begin(), list.end(), value);
}

void ProcessEntityListeners(const array_view<const pair<Entity, IEntityManager::EventType>> states,
    const array_view<IEcs::EntityListener*> entityListeners)
{
    // handle state changes (collect to groups of same kind of events)
    vector<Entity> res;
    res.reserve(states.size());
    auto type = states[0U].second;
    for (const auto& s : states) {
        if (s.second != type) {
            if (!res.empty()) {
                // Let listeners know that entity state has changed.
                for (auto* listener : entityListeners) {
                    if (listener) {
                        listener->OnEntityEvent(type, res);
                    }
                }
                // start collecting new events.
                res.clear();
            }
            type = s.second;
        }
        // add to event list.
        res.push_back(s.first);
    }
    if (!res.empty()) {
        // Send the final events.
        for (auto* listener : entityListeners) {
            if (listener) {
                listener->OnEntityEvent(type, res);
            }
        }
    }
}

template<class TypeInfo>
const TypeInfo* FindTypeInfo(const Uid& uid, const array_view<const ITypeInfo* const>& container)
{
    for (const auto& info : container) {
        if (static_cast<const TypeInfo* const>(info)->uid == uid) {
            return static_cast<const TypeInfo*>(info);
        }
    }

    return nullptr;
}

bool GatherComponents(vector<const ComponentManagerTypeInfo*>& componentManagerInfos,
    const array_view<const ITypeInfo* const>& componentMetadata, array_view<const Uid> componentDependencies)
{
    // Ensure we have all components required by this system.
    for (const auto& dependencyUid : componentDependencies) {
        if (dependencyUid == Uid {}) {
            continue;
        }
        const auto* componentTypeInfo = FindTypeInfo<ComponentManagerTypeInfo>(dependencyUid, componentMetadata);
        if (!componentTypeInfo) {
            return false;
        }
        if (std::find(componentManagerInfos.cbegin(), componentManagerInfos.cend(), componentTypeInfo) ==
            componentManagerInfos.cend()) {
            componentManagerInfos.push_back(componentTypeInfo);
        }
    }
    return true;
}

pair<vector<const SystemTypeInfo*>, vector<const ComponentManagerTypeInfo*>> GetAvailableComponentManagersAndSystems()
{
    auto& pluginRegister = GetPluginRegister();
    auto componentMetadata = pluginRegister.GetTypeInfos(ComponentManagerTypeInfo::UID);
    auto systemMetadata = pluginRegister.GetTypeInfos(SystemTypeInfo::UID);
    vector<const ComponentManagerTypeInfo*> componentManagerInfos;
    componentManagerInfos.reserve(componentMetadata.size());
    vector<const SystemTypeInfo*> systemInfos;
    systemInfos.reserve(systemMetadata.size());

    // Gather systems which have all components available
    for (const auto* info : systemMetadata) {
        if (!info || (info->typeUid != SystemTypeInfo::UID)) {
            continue;
        }
        auto* systemTypeInfo = static_cast<const SystemTypeInfo*>(info);
        if (!systemTypeInfo->createSystem) {
            continue;
        }

        // Ensure we have all components required by this system.
        if (!GatherComponents(componentManagerInfos, componentMetadata, systemTypeInfo->componentDependencies) ||
            !GatherComponents(
                componentManagerInfos, componentMetadata, systemTypeInfo->readOnlyComponentDependencies)) {
            CORE_LOG_W("Failed to resolve component dependencies: %s.", systemTypeInfo->typeName);
            continue;
        }
        // At least component managers should be available so add to list.
        systemInfos.push_back(systemTypeInfo);
    }
    return { BASE_NS::move(systemInfos), BASE_NS::move(componentManagerInfos) };
}

void OrderSystems(vector<const SystemTypeInfo*>& systemInfos)
{
    // Order systems based optional afterSystem UIDs.
    auto begin = systemInfos.begin();
    auto end = systemInfos.end();
    for (auto it = begin; it != end;) {
        if ((*it)->afterSystem == Uid {}) {
            ++it;
            continue;
        }
        auto pos = std::find(it, end, (*it)->afterSystem);
        if (pos != end) {
            // rotate it right after pos
            std::rotate(it, it + 1, (pos + 1));
        } else {
            ++it;
        }
    }

    // Order systems based optional beforeSystem UIDs.
    for (auto it = begin; it != end;) {
        if ((*it)->beforeSystem == Uid {}) {
            ++it;
            continue;
        }
        auto pos = std::find(begin, it, (*it)->beforeSystem);
        if (pos != it) {
            // rotate it left before pos
            std::rotate(pos, it, (it + 1));
        } else {
            ++it;
        }
    }
}

void Ecs::AddListener(EntityListener& listener)
{
    if (Find(entityListeners_, &listener) != entityListeners_.end()) {
        // already added.
        return;
    }
    entityListeners_.push_back(&listener);
}

void Ecs::RemoveListener(EntityListener& listener)
{
    if (auto it = Find(entityListeners_, &listener); it != entityListeners_.end()) {
        // Setting the listener to null instead of removing. This allows removing listeners from a listener callback.
        *it = nullptr;
        return;
    }
}

void Ecs::AddListener(ComponentListener& listener)
{
    if (Find(componentListeners_, &listener) != componentListeners_.end()) {
        // already added.
        return;
    }
    componentListeners_.push_back(&listener);
}

void Ecs::RemoveListener(ComponentListener& listener)
{
    if (auto it = Find(componentListeners_, &listener); it != componentListeners_.end()) {
        *it = nullptr;
        return;
    }
}

void Ecs::AddListener(IComponentManager& manager, ComponentListener& listener)
{
    auto list = componentManagerListeners_.find(&manager);
    if (list != componentManagerListeners_.end()) {
        if (auto it = Find(list->second, &listener); it != list->second.end()) {
            return;
        }
        list->second.push_back(&listener);
        return;
    }
    componentManagerListeners_[&manager].push_back(&listener);
}

void Ecs::RemoveListener(IComponentManager& manager, ComponentListener& listener)
{
    auto list = componentManagerListeners_.find(&manager);
    if (list == componentManagerListeners_.end()) {
        return;
    }
    if (auto it = Find(list->second, &listener); it != list->second.end()) {
        *it = nullptr;
        return;
    }
}

IComponentManager* Ecs::CreateComponentManager(const ComponentManagerTypeInfo& componentManagerTypeInfo)
{
    IComponentManager* manager = nullptr;
    if (componentManagerTypeInfo.createManager) {
        manager = GetComponentManager(componentManagerTypeInfo.uid);
        if (manager) {
            CORE_LOG_W("Duplicate component manager creation, returning existing instance");
        } else {
            manager = componentManagerTypeInfo.createManager(*this);
            if (manager) {
                managers_.insert({ componentManagerTypeInfo.uid, manager });
                managerOrder_.emplace_back(manager, componentManagerTypeInfo.destroyManager);
            }
        }
    }
    return manager;
}

ISystem* Ecs::CreateSystem(const SystemTypeInfo& systemInfo)
{
    ISystem* system = nullptr;
    if (!systemInfo.createSystem) {
        return system;
    }

    system = GetSystem(systemInfo.uid);
    if (system) {
        CORE_LOG_W("Duplicate system creation, returning existing instance.");
        return system;
    }

    system = systemInfo.createSystem(*this);
    if (!system) {
        CORE_LOG_E("Failed to create system.");
        return system;
    }

    systems_.insert({ systemInfo.uid, system });
    systemOrder_.emplace_back(system, systemInfo.destroySystem);

    if (initialized_) {
        system->Initialize();
    }

    return system;
}

Ecs::Ecs(IClassFactory& registry, const IThreadPool::Ptr& threadPool, const uint64_t ecsId)
    : threadPool_(threadPool), pluginRegistry_(registry), ecsId_(ecsId)
{
    for (auto info : CORE_NS::GetPluginRegister().GetTypeInfos(IEcsPlugin::UID)) {
        if (auto ecsPlugin = static_cast<const IEcsPlugin*>(info); ecsPlugin && ecsPlugin->createPlugin) {
            auto token = ecsPlugin->createPlugin(*this);
            plugins_.push_back({ token, ecsPlugin });
        }
    }
    GetPluginRegister().AddListener(*this);
}

Ecs::~Ecs()
{
    GetPluginRegister().RemoveListener(*this);

    Uninitialize();
    managerOrder_.clear();
    systemOrder_.clear();

    for (auto& plugin : plugins_) {
        if (plugin.second->destroyPlugin) {
            plugin.second->destroyPlugin(plugin.first);
        }
    }
}

IClassFactory& Ecs::GetClassFactory() const
{
    return pluginRegistry_;
}

IEntityManager& Ecs::GetEntityManager()
{
    return entityManager_;
}

const IEntityManager& Ecs::GetEntityManager() const
{
    return entityManager_;
}

void Ecs::GetComponents(Entity entity, vector<IComponentManager*>& result) const
{
    result.clear();
    result.reserve(managers_.size());
    for (auto& m : managerOrder_) {
        if (m->HasComponent(entity)) {
            result.push_back(m.get());
        }
    }
}

vector<ISystem*> Ecs::GetSystems() const
{
    vector<ISystem*> result;
    result.reserve(systemOrder_.size());
    for (auto& t : systemOrder_) {
        result.push_back(t.get());
    }
    return result;
}

ISystem* Ecs::GetSystem(const Uid& uid) const
{
    if (auto pos = systems_.find(uid); pos != systems_.end()) {
        return pos->second;
    }
    return nullptr;
}

vector<IComponentManager*> Ecs::GetComponentManagers() const
{
    vector<IComponentManager*> result;
    result.reserve(managerOrder_.size());
    for (auto& t : managerOrder_) {
        result.push_back(t.get());
    }
    return result;
}

IComponentManager* Ecs::GetComponentManager(const Uid& uid) const
{
    if (auto pos = managers_.find(uid); pos != managers_.end()) {
        return pos->second;
    }
    return nullptr;
}

Entity Ecs::CloneEntity(const Entity entity)
{
    if (!EntityUtil::IsValid(entity)) {
        return {};
    }

    const Entity clonedEntity = entityManager_.Create();
    if (entityManager_.IsAlive(entity)) {
        for (auto& cm : managerOrder_) {
            const auto id = cm->GetComponentId(entity);
            if (id != IComponentManager::INVALID_COMPONENT_ID) {
                cm->Create(clonedEntity);
                cm->SetData(clonedEntity, *cm->GetData(id));
            }
        }
    }
    return clonedEntity;
}

void Ecs::ProcessComponentEvents(
    ComponentListener::EventType eventType, const array_view<const Entity> removedEntities) const
{
    vector<Entity> (IComponentManager::*getter)();
    switch (eventType) {
        case ComponentListener::EventType::CREATED:
            getter = &IComponentManager::GetAddedComponents;
            break;
        case ComponentListener::EventType::MODIFIED:
            getter = &IComponentManager::GetUpdatedComponents;
            break;
        case ComponentListener::EventType::DESTROYED:
            getter = &IComponentManager::GetRemovedComponents;
            break;
        case ComponentListener::EventType::MOVED:
            getter = &IComponentManager::GetMovedComponents;
            break;
        default:
            return;
    }
    for (const auto& m : managerOrder_) {
        vector<Entity> affectedEntities = (*m.*getter)();
        if (!removedEntities.empty()) {
            affectedEntities.erase(
                std::remove_if(affectedEntities.begin(), affectedEntities.end(),
                    [removedEntities](const Entity& entity) {
                        const auto pos = std::lower_bound(removedEntities.cbegin(), removedEntities.cend(), entity,
                            [](const Entity& entity, const Entity& removed) { return entity < removed; });
                        return ((pos != removedEntities.cend()) && entity >= *pos);
                    }),
                affectedEntities.cend());
        }
        if (!affectedEntities.empty()) {
            // global listeners
            for (auto* listener : componentListeners_) {
                if (listener) {
                    listener->OnComponentEvent(eventType, *m, affectedEntities);
                }
            }
            // per manager listeners
            if (auto it = componentManagerListeners_.find(m.get()); it != componentManagerListeners_.cend()) {
                for (auto* listener : it->second) {
                    if (listener) {
                        listener->OnComponentEvent(eventType, *m, affectedEntities);
                    }
                }
            }
        }
    }
}

void Ecs::ProcessEvents()
{
    if (processingEvents_) {
        CORE_LOG_W("Calling ProcessEvents() from an event callback is not allowed");
        return;
    }
    processingEvents_ = true;

    vector<Entity> allRemovedEntities;
    bool deadEntities = false;
    do {
        // Let entity manager check entity reference counts
        entityManager_.UpdateDeadEntities();

        // Send entity related events
        if (const auto events = entityManager_.GetEvents(); !events.empty()) {
            ProcessEntityListeners(events, entityListeners_);
        }

        // Remove components for removed entities.
        const vector<Entity> removed = entityManager_.GetRemovedEntities();
        deadEntities = !removed.empty();
        if (deadEntities) {
            allRemovedEntities.append(removed.cbegin(), removed.cend());
        }
        for (auto& m : managerOrder_) {
            // Destroy all components related to these entities.
            if (deadEntities) {
                m->Destroy(removed);
            }
            m->Gc();
        }
        // Destroying components may release the last reference for some entity so we loop until there are no new
        // deaths reported.
    } while (deadEntities);

    if (!allRemovedEntities.empty()) {
        std::sort(allRemovedEntities.begin(), allRemovedEntities.end());
    }

    // Send component related events
    ProcessComponentEvents(ComponentListener::EventType::CREATED, allRemovedEntities);
    ProcessComponentEvents(ComponentListener::EventType::MOVED, allRemovedEntities);
    ProcessComponentEvents(ComponentListener::EventType::MODIFIED, allRemovedEntities);
    ProcessComponentEvents(ComponentListener::EventType::DESTROYED, {});

    // Clean-up removed listeners.
    entityListeners_.erase(
        std::remove(entityListeners_.begin(), entityListeners_.end(), nullptr), entityListeners_.cend());
    componentListeners_.erase(
        std::remove(componentListeners_.begin(), componentListeners_.end(), nullptr), componentListeners_.cend());

    for (auto it = componentManagerListeners_.begin(); it != componentManagerListeners_.cend();) {
        auto& listeners = it->second;
        listeners.erase(std::remove(listeners.begin(), listeners.end(), nullptr), listeners.cend());
        if (listeners.empty()) {
            it = componentManagerListeners_.erase(it);
        } else {
            ++it;
        }
    }

    processingEvents_ = false;
}

void Ecs::Initialize()
{
    if (systemOrder_.empty()) {
        auto [systemInfos, componentManagerInfos] = GetAvailableComponentManagersAndSystems();
        // Create all the required component managers.
        for (const auto* info : componentManagerInfos) {
            CreateComponentManager(*info);
        }

        OrderSystems(systemInfos);
        for (const auto* systemInfo : systemInfos) {
            ISystem* system = systemInfo->createSystem(*this);
            if (!system) {
                CORE_LOG_E("Failed to create system.");
                continue;
            }
            systems_.insert({ systemInfo->uid, system });
            systemOrder_.emplace_back(system, systemInfo->destroySystem);
        }
    }

    for (auto& s : systemOrder_) {
        s->Initialize();
    }
    initialized_ = true;
}

CORE_PROFILER_SYMBOL(escUpdate, "Update");

bool Ecs::Update(uint64_t time, uint64_t delta)
{
    CORE_PROFILER_MARK_FRAME_START(escUpdate);
    CORE_CPU_PERF_SCOPE("CORE", "Update", "Total_Cpu", CORE_PROFILER_DEFAULT_COLOR);
    bool frameRenderingQueued = false;
    if (GetRenderMode() == RENDER_ALWAYS || renderRequested_) {
        frameRenderingQueued = true;
    }

    // Update all systems.
    delta = static_cast<uint64_t>(static_cast<float>(delta) * timeScale_);
    for (auto& s : systemOrder_) {
        CORE_CPU_PERF_SCOPE("CORE", "SystemUpdate", s->GetName(), CORE_PROFILER_DEFAULT_COLOR);
        if (s->Update(frameRenderingQueued, time, delta)) {
            frameRenderingQueued = true;
        }
    }

    // Clear modification flags from component managers.
    for (auto& componentManager : managerOrder_) {
        componentManager->ClearModifiedFlags();
    }

    renderRequested_ = false;
    needRender_ = frameRenderingQueued;

    CORE_PROFILER_MARK_FRAME_END(escUpdate);
    return frameRenderingQueued;
}

void Ecs::Uninitialize()
{
    // Destroy all entities from scene.
    entityManager_.DestroyAllEntities();

    // Garbage-collect.
    ProcessEvents();

    // Uninitialize systems.
    for (auto it = systemOrder_.rbegin(); it != systemOrder_.rend(); ++it) {
        (*it)->Uninitialize();
    }

    initialized_ = false;
}

void Ecs::RequestRender()
{
    renderRequested_ = true;
}

void Ecs::SetRenderMode(RenderMode renderMode)
{
    renderMode_ = renderMode;
}

IEcs::RenderMode Ecs::GetRenderMode()
{
    return renderMode_;
}

bool Ecs::NeedRender() const
{
    return needRender_;
}

const IThreadPool::Ptr& Ecs::GetThreadPool() const
{
    return threadPool_;
}

float Ecs::GetTimeScale() const
{
    return timeScale_;
}

void Ecs::SetTimeScale(float scale)
{
    timeScale_ = scale;
}

uint64_t Ecs::GetId() const
{
    return ecsId_;
}

void Ecs::Ref() noexcept
{
    BASE_NS::AtomicIncrementRelaxed(&refcnt_);
}

void Ecs::Unref() noexcept
{
    if (BASE_NS::AtomicDecrementRelease(&refcnt_) == 0) {
        BASE_NS::AtomicFenceAcquire();
        delete this;
    }
}

template<typename Container>
inline auto RemoveUid(Container& container, const Uid& uid)
{
    container.erase(std::remove_if(container.begin(), container.end(),
                        [&uid](const auto& thing) { return thing->GetUid() == uid; }),
        container.cend());
}

void Ecs::OnTypeInfoEvent(EventType type, array_view<const ITypeInfo* const> typeInfos)
{
    if (type == EventType::ADDED) {
        // not really interesed in these events. systems and component managers are added when SystemGraphLoader parses
        // a configuration. we could store them in systems_ and managers_ and only define the order based on the graph.
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == IEcsPlugin::UID) {
                const auto ecsPlugin = static_cast<const IEcsPlugin*>(info);
                ecsPlugin->createPlugin(*this);
            }
        }
    } else if (type == EventType::REMOVED) {
        for (const auto* info : typeInfos) {
            if (info && info->typeUid == SystemTypeInfo::UID) {
                const auto systemInfo = static_cast<const SystemTypeInfo*>(info);
                // for systems Untinitialize should be called before destroying the instance
                if (const auto pos = systems_.find(systemInfo->uid); pos != systems_.cend()) {
                    pos->second->Uninitialize();
                    systems_.erase(pos);
                }
                RemoveUid(systemOrder_, systemInfo->uid);
            } else if (info && info->typeUid == ComponentManagerTypeInfo::UID) {
                const auto managerInfo = static_cast<const ComponentManagerTypeInfo*>(info);
                // BaseManager expects that the component list is empty when it's destroyed. might be also
                // nice to notify all the listeners that the components are being destroyed.
                if (const auto pos = managers_.find(managerInfo->uid); (pos != managers_.end()) && (pos->second)) {
                    auto* manager = pos->second;
                    CleanupComponentManager(*manager);
                    managers_.erase(pos);
                }
                RemoveUid(managerOrder_, managerInfo->uid);
            }
        }
    }
}

void Ecs::CleanupComponentManager(IComponentManager& manager)
{
    // remove all the components.
    const auto components = static_cast<IComponentManager::ComponentId>(manager.GetComponentCount());
    for (IComponentManager::ComponentId i = 0; i < components; ++i) {
        manager.Destroy(manager.GetEntity(i));
    }

    // check are there generic or specific component listeners to inform.
    if (const auto listenerIt = componentManagerListeners_.find(&manager);
        !componentListeners_.empty() ||
        ((listenerIt != componentManagerListeners_.end()) && !listenerIt->second.empty())) {
        if (const vector<Entity> removed = manager.GetRemovedComponents(); !removed.empty()) {
            const auto removedView = array_view<const Entity>(removed);
            for (auto* lister : componentListeners_) {
                if (lister) {
                    lister->OnComponentEvent(ComponentListener::EventType::DESTROYED, manager, removedView);
                }
            }
            if (listenerIt != componentManagerListeners_.end()) {
                for (auto* lister : listenerIt->second) {
                    if (lister) {
                        lister->OnComponentEvent(ComponentListener::EventType::DESTROYED, manager, removedView);
                    }
                }
                // remove all the listeners for this manager. RemoveListener won't do anything. this isn't neccessary,
                // but rather not leave invalid manager pointer even if it's just used as the key.
                componentManagerListeners_.erase(listenerIt);
            }
        }
    }
    // garbage collection will remove dead entries from the list and BaseManager is happy.
    manager.Gc();
}
} // namespace

IEcs* IEcsInstance(IClassFactory& registry, const IThreadPool::Ptr& threadPool, const uint64_t ecsId)
{
    return new Ecs(registry, threadPool, ecsId);
}
CORE_END_NAMESPACE()
