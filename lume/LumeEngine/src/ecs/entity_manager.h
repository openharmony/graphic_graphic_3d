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

#ifndef CORE_ECS_ENTITY_MANAGER_H
#define CORE_ECS_ENTITY_MANAGER_H

#include <cstddef>
#include <cstdint>

#include <base/containers/generic_iterator.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T1, class T2>
struct pair;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE() class EntityManager final : public IEntityManager {
public:
    EntityManager();
    explicit EntityManager(const size_t entityCount);
    ~EntityManager() override;

    Entity Create() override;
    EntityReference CreateReferenceCounted() override;
    EntityReference GetReferenceCounted(Entity entity) override;
    void Destroy(const Entity entity) override;
    void DestroyAllEntities() override;

    bool IsAlive(const Entity entity) const override;

    uint32_t GetGenerationCounter() const override;

    Iterator::Ptr Begin(IteratorType type) const override;
    Iterator::Ptr End(IteratorType type) const override;

    void SetActive(const Entity entity, bool state) override;

    BASE_NS::vector<Entity> GetRemovedEntities();
    BASE_NS::vector<BASE_NS::pair<Entity, EventType>> GetEvents();

    // Marks all entities with zero refcnt as DEAD.
    void UpdateDeadEntities();

private:
    struct EntityState {
        enum class State : uint32_t {
            /* Entity is ready for re-use.
             */
            FREE = 0,
            /* Entity is alive and active.
             */
            ALIVE = 1,
            /* Entity is alive and de-activated.
             */
            INACTIVE = 2,
            /* Entity is destroyed and waiting for GC
             */
            DEAD = 3,
        } state { 0 };
        uint32_t generation { 0 };
        IEntityReferenceCounter::Ptr counter;
    };
    BASE_NS::vector<EntityState> entities_;
    BASE_NS::vector<Entity> removedList_;
    BASE_NS::vector<uint32_t> freeList_;
    BASE_NS::vector<BASE_NS::pair<Entity, IEntityManager::EventType>> eventList_;
    uint32_t generationCounter_ { 0 };

    class IteratorImpl final : public Iterator {
        const EntityManager* owner_ { nullptr };
        uint32_t index_ { 0 };
        IteratorType type_;

    public:
        IteratorImpl(const EntityManager&, size_t, IteratorType type);
        const class IEntityManager* GetOwner() const override;
        bool Compare(const Iterator::Ptr&) const override;
        bool Next() override;
        Entity Get() const override;
        Iterator::Ptr Clone() const override;
    };
    Iterator::Ptr MakeIterator(uint32_t index, IteratorType type) const;
};
CORE_END_NAMESPACE()

#endif // CORE_ECS_ENTITY_MANAGER_H
