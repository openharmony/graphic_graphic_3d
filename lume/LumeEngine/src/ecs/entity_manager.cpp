/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#include "entity_manager.h"

#include <algorithm>
#include <atomic>

#include <core/log.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
using BASE_NS::pair;
using BASE_NS::vector;

namespace {
uint32_t GetId(const Entity& e)
{
    return e.id & 0xFFFFFFFF;
}

uint32_t GetGeneration(const Entity& e)
{
    return (e.id >> 32l) & 0xFFFFFFFF;
}

Entity MakeEntityId(uint32_t g, uint32_t i)
{
    return { (static_cast<uint64_t>(g) << 32l) | i };
}
class EntityReferenceCounter final : public IEntityReferenceCounter {
public:
    using Ptr = BASE_NS::refcnt_ptr<EntityReferenceCounter>;
    EntityReferenceCounter() = default;
    ~EntityReferenceCounter() override = default;

protected:
    void Ref() noexcept override
    {
        refcnt_.fetch_add(1, std::memory_order_relaxed);
    }

    void Unref() noexcept override
    {
        if (std::atomic_fetch_sub_explicit(&refcnt_, 1, std::memory_order_release) == 0) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete this;
        }
    }

    int32_t GetRefCount() const noexcept override
    {
        return refcnt_.load();
    }

private:
    std::atomic<int32_t> refcnt_ { -1 };
};
} // namespace

EntityManager::EntityManager() : EntityManager((const size_t)64) {}

EntityManager::EntityManager(const size_t entityCount)
{
    entities_.reserve(entityCount);
}

EntityManager::~EntityManager()
{
    // count live entities, should be zero
    int32_t liveEntities = 0;
    int32_t inactiveEntities = 0;
    for (const auto& e : entities_) {
        if (EntityState::State::ALIVE == e.state) {
            ++liveEntities;
        }
        if (EntityState::State::INACTIVE == e.state) {
            ++inactiveEntities;
        }
    }
    CORE_ASSERT(inactiveEntities == 0);
    CORE_ASSERT(liveEntities == 0);
}

Entity EntityManager::Create()
{
    // find first free, dead, or with zero refcnt.
    auto it = std::find_if(entities_.begin(), entities_.end(), [](const auto& b) {
        return (b.state == EntityState::State::FREE) || (b.state == EntityState::State::DEAD) ||
               (b.counter && (b.counter->GetRefCount() == 0));
    });
    Entity result;
    if (it == entities_.end()) {
        const auto generation = 1U;
        const auto id = (uint32_t)entities_.size();
        entities_.push_back({ EntityState::State::ALIVE, generation, nullptr });
        result = MakeEntityId(generation, id);
    } else {
        const auto id = (uint32_t)(it - entities_.begin());
        auto& slot = *it;
        // if the slot isn't free report a dead entity
        if (slot.state != EntityState::State::FREE) {
            const auto deadEntity = MakeEntityId(slot.generation, id);
            removedList_.push_back(deadEntity);
            eventList_.push_back({ deadEntity, EventType::DESTROYED });
        }
        slot.counter = nullptr; // NOTE: could push to a pool and recycle used often
        ++slot.generation;

        slot.state = EntityState::State::ALIVE;
        result = MakeEntityId(slot.generation, id);
    }
    addedList_.push_back(result);
    eventList_.push_back({ result, EventType::CREATED });
    ++generationCounter_;
    return result;
}

EntityReference EntityManager::CreateReferenceCounted()
{
    // find first free or dead.
    auto it = std::find_if(entities_.begin(), entities_.end(), [](const auto& b) {
        return (b.state == EntityState::State::FREE) || (b.state == EntityState::State::DEAD) ||
               (b.counter && (b.counter->GetRefCount() == 0));
    });
    Entity result;
    if (it == entities_.end()) {
        const auto generation = 1U;
        const auto id = (uint32_t)entities_.size();
        entities_.push_back(
            { EntityState::State::ALIVE, generation, IEntityReferenceCounter::Ptr { new EntityReferenceCounter } });
        result = MakeEntityId(generation, id);
    } else {
        const auto id = (uint32_t)(it - entities_.begin());
        auto& slot = *it;
        // if the slot isn't free report a dead entity
        if (slot.state != EntityState::State::FREE) {
            const auto deadEntity = MakeEntityId(slot.generation, id);
            removedList_.push_back(deadEntity);
            eventList_.push_back({ deadEntity, EventType::DESTROYED });
        }
        if (!slot.counter) {
            slot.counter.reset(new EntityReferenceCounter);
        }
        ++slot.generation;
        slot.state = EntityState::State::ALIVE;
        result = MakeEntityId(slot.generation, id);
    }
    addedList_.push_back(result);
    eventList_.push_back({ result, EventType::CREATED });
    ++generationCounter_;
    return EntityReference(result, entities_[GetId(result)].counter);
}

EntityReference EntityManager::GetReferenceCounted(const Entity entity)
{
    if (EntityUtil::IsValid(entity)) {
        if (const uint32_t id = GetId(entity); id < entities_.size()) {
            auto& e = entities_[id];
            // make sure the given entity id has the same generation and that the entity isn't dead or free.
            if ((e.generation == GetGeneration(entity)) &&
                ((e.state == EntityState::State::ALIVE) || (e.state == EntityState::State::INACTIVE))) {
                if (!e.counter) {
                    // entity wasn't yet reference counted so add a counter
                    e.counter.reset(new EntityReferenceCounter);
                    return { entity, e.counter };
                } else if (e.counter->GetRefCount() > 0) {
                    // reference count is still valid
                    return { entity, e.counter };
                } else {
                    // reference count has expired, but we won't revive the entity.
                }
            }
        }
    }
    return {};
}

void EntityManager::Destroy(const Entity entity)
{
    if (EntityUtil::IsValid(entity)) {
        if (const uint32_t id = GetId(entity); id < entities_.size()) {
            auto& e = entities_[id];
            if ((e.generation == GetGeneration(entity)) &&
                ((e.state == EntityState::State::ALIVE) || (e.state == EntityState::State::INACTIVE))) {
                e.state = EntityState::State::DEAD;
                e.counter = nullptr;
                removedList_.push_back(entity);
                eventList_.push_back({ entity, EventType::DESTROYED });
                ++generationCounter_;
            }
        }
    }
}

void EntityManager::DestroyAllEntities()
{
    for (uint32_t i = 0; i < (uint32_t)entities_.size(); ++i) {
        auto& e = entities_[i];
        if ((EntityState::State::ALIVE == e.state) || (EntityState::State::INACTIVE == e.state)) {
            auto entity = MakeEntityId(e.generation, i);
            removedList_.push_back(entity);
            eventList_.push_back({ entity, EventType::DESTROYED });
            e.counter = nullptr;
            e.state = EntityState::State::DEAD;
        }
    }
    ++generationCounter_;
}

bool EntityManager::IsAlive(const Entity entity) const
{
    if (EntityUtil::IsValid(entity)) {
        const uint32_t id = GetId(entity);
        if (id < entities_.size()) {
            const auto& state = entities_[id];
            if (state.generation == GetGeneration(entity)) {
                return (state.state == EntityState::State::ALIVE) &&
                       (!state.counter || (state.counter->GetRefCount() > 0));
            }
        }
    }
    return false;
}

vector<Entity> EntityManager::GetAddedEntities()
{
    return move(addedList_);
}

vector<Entity> EntityManager::GetRemovedEntities()
{
    for (const Entity& e : removedList_) {
        const uint32_t id = GetId(e);
        if (id < entities_.size()) {
            if (entities_[id].generation == GetGeneration(e)) {
                CORE_ASSERT(entities_[id].state == EntityState::State::DEAD);
                if (id < entities_.size() - 1) {
                    entities_[id].state = EntityState::State::FREE;
                } else {
                    entities_.resize(entities_.size() - 1);
                }
            }
        }
    }
    return move(removedList_);
}

uint32_t EntityManager::GetGenerationCounter() const
{
    return generationCounter_;
}

vector<pair<Entity, IEntityManager::EventType>> EntityManager::GetEvents()
{
    return move(eventList_);
}

void EntityManager::SetActive(const Entity entity, bool state)
{
    if (EntityUtil::IsValid(entity)) {
        EntityState::State oldState, newState;
        EventType event;
        if (state == true) {
            oldState = EntityState::State::INACTIVE;
            newState = EntityState::State::ALIVE;
            event = EventType::ACTIVATED;
        } else {
            oldState = EntityState::State::ALIVE;
            newState = EntityState::State::INACTIVE;
            event = EventType::DEACTIVATED;
        }

        uint32_t id = GetId(entity);
        if (id < entities_.size()) {
            if (entities_[id].generation == GetGeneration(entity)) {
                if (entities_[id].state == oldState) {
                    entities_[id].state = newState;
                    eventList_.push_back({ entity, event });
                    ++generationCounter_;
                }
            }
        }
    }
}

void EntityManager::UpdateDeadEntities()
{
    const auto removedCount = removedList_.size();
    for (uint32_t id = 0, count = (uint32_t)entities_.size(); id < count; ++id) {
        auto& e = entities_[id];
        if ((e.state != EntityState::State::FREE) && e.counter && (e.counter->GetRefCount() == 0)) {
            const Entity entity = MakeEntityId(e.generation, id);
            removedList_.push_back(entity);
            eventList_.push_back({ entity, EventType::DESTROYED });
            e.state = EntityState::State::DEAD;
        }
    }
    if (removedCount != removedList_.size()) {
        ++generationCounter_;
    }
}

EntityManager::IteratorImpl::IteratorImpl(const EntityManager& owner, size_t index, IteratorType type)
    : owner_(&owner), index_(static_cast<uint32_t>(index)), type_(type)
{
    const auto valid = (type == IteratorType::DEACTIVATED) ? EntityState::State::INACTIVE : EntityState::State::ALIVE;
    if (index < owner.entities_.size()) {
        const auto& e = owner.entities_[index];
        if ((e.state != valid) || (e.counter && e.counter->GetRefCount() == 0)) {
            Next();
        }
    }
}

const class IEntityManager* EntityManager::IteratorImpl::GetOwner() const
{
    return owner_;
}

bool EntityManager::IteratorImpl::Compare(const Iterator::Ptr& other) const
{
    if ((other == nullptr) || (other->GetOwner() != owner_)) {
        return false;
    }
    auto* otheri = static_cast<const EntityManager::IteratorImpl*>(other.get());
    return (index_ == otheri->index_) && (type_ == otheri->type_);
}

bool EntityManager::IteratorImpl::Next()
{
    const auto& entities = owner_->entities_;
    if (index_ < entities.size()) {
        const auto valid =
            (type_ == IteratorType::DEACTIVATED) ? EntityState::State::INACTIVE : EntityState::State::ALIVE;

        ++index_;
        while (index_ < entities.size()) {
            auto& state = entities[index_];
            if ((state.state == valid) && ((!state.counter) || (state.counter->GetRefCount() > 0))) {
                break;
            }
            ++index_;
        }
    }
    return (index_ < owner_->entities_.size());
}

Entity EntityManager::IteratorImpl::Get() const
{
    if (index_ >= owner_->entities_.size()) {
        return {};
    }
    return MakeEntityId(owner_->entities_[index_].generation, index_);
}

IEntityManager::Iterator::Ptr EntityManager::MakeIterator(uint32_t index, IteratorType type) const
{
    auto del = [](Iterator* it) { delete (EntityManager::IteratorImpl*)(it); };
    auto p = new EntityManager::IteratorImpl(*this, index, type);
    return { p, del };
}

IEntityManager::Iterator::Ptr EntityManager::IteratorImpl::Clone() const
{
    return owner_->MakeIterator(index_, type_);
}

IEntityManager::Iterator::Ptr EntityManager::Begin(IteratorType type) const
{
    return MakeIterator(0U, type);
}

IEntityManager::Iterator::Ptr EntityManager::End(IteratorType type) const
{
    return MakeIterator(static_cast<uint32_t>(entities_.size()), type);
}

CORE_END_NAMESPACE()
