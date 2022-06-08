/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef CORE_ECS_ENTITY_MANAGER_H
#define CORE_ECS_ENTITY_MANAGER_H

#include <base/containers/unordered_map.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class EntityManager final : public IEntityManager {
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
    BASE_NS::vector<Entity> GetAddedEntities() override;
    BASE_NS::vector<Entity> GetRemovedEntities() override;
    BASE_NS::vector<BASE_NS::pair<Entity, EventType>> GetEvents() override;

    uint32_t GetGenerationCounter() const override;

    Iterator::Ptr Begin(IteratorType type) const override;
    Iterator::Ptr End(IteratorType type) const override;

    void SetActive(const Entity entity, bool state) override;

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
    BASE_NS::vector<Entity> addedList_;
    BASE_NS::vector<Entity> removedList_;
    BASE_NS::vector<BASE_NS::pair<Entity, IEntityManager::EventType>> eventList_;
    uint32_t generationCounter_ { 0 };

    class IteratorImpl : public Iterator {
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
