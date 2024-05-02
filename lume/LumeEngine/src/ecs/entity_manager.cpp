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

#include "entity_manager.h"

#include <algorithm>
#include <atomic>

#include <base/containers/generic_iterator.h>
#include <base/containers/iterator.h>
#include <base/containers/refcnt_ptr.h>
#include <base/containers/type_traits.h>
#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_entity_manager.h>
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
    EntityReferenceCounter(const EntityReferenceCounter&) = delete;
    EntityReferenceCounter& operator=(const EntityReferenceCounter&) = delete;
    EntityReferenceCounter(EntityReferenceCounter&&) = delete;
    EntityReferenceCounter& operator=(EntityReferenceCounter&&) = delete;

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

EntityManager::EntityManager() : EntityManager(64u) {}

EntityManager::EntityManager(const size_t entityCount)
{
    entities_.reserve(entityCount);
}

EntityManager::~EntityManager()
{
    // count live entities, should be zero
    [[maybe_unused]] int32_t liveEntities = 0;
    [[maybe_unused]] int32_t inactiveEntities = 0;
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
    Entity result;
    if (freeList_.empty()) {
        const auto generation = 1U;
        const auto id = static_cast<uint32_t>(entities_.size());
        entities_.push_back({ EntityState::State::ALIVE, generation, nullptr });
        result = MakeEntityId(generation, id);
    } else {
        const auto id = freeList_.back();
        freeList_.pop_back();
        auto& slot = entities_[id];
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
    eventList_.push_back({ result, EventType::CREATED });
    ++generationCounter_;
    return result;
}

EntityReference EntityManager::CreateReferenceCounted()
{
    Entity result;
    if (freeList_.empty()) {
        const auto generation = 1U;
        const auto id = static_cast<uint32_t>(entities_.size());
        entities_.push_back(
            { EntityState::State::ALIVE, generation, IEntityReferenceCounter::Ptr { new EntityReferenceCounter } });
        result = MakeEntityId(generation, id);
    } else {
        const auto id = freeList_.back();
        freeList_.pop_back();
        auto& slot = entities_[id];

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
                }
                if (e.counter->GetRefCount() > 0) {
                    // reference count is still valid
                    return { entity, e.counter };
                }
                // reference count has expired, but we won't revive the entity.
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
    for (uint32_t i = 0, count = static_cast<uint32_t>(entities_.size()); i < count; ++i) {
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

vector<Entity> EntityManager::GetRemovedEntities()
{
    const auto freeSize = freeList_.size();
    for (const Entity& e : removedList_) {
        const uint32_t id = GetId(e);
        if (id < entities_.size()) {
            if (entities_[id].generation == GetGeneration(e)) {
                CORE_ASSERT(entities_[id].state == EntityState::State::DEAD);
                if (id < entities_.size() - 1) {
                    entities_[id].state = EntityState::State::FREE;
                    freeList_.push_back(id);
                } else {
                    entities_.resize(entities_.size() - 1);
                }
            }
        }
    }
    if (const auto finalFreeSize = freeList_.size()) {
        // by sorting with greater and using pop_back in creation we keep entities_ filled from the beginning.
        // could be removed not useful.
        if (finalFreeSize != freeSize) {
            std::sort(freeList_.begin(), freeList_.end(), std::greater {});
        }
        // check from the beginning that ids don't go out-of-bounds and remove problematic ones.
        // most likely they never are so linear is better than lower_bounds.
        auto count = 0U;
        while ((count < finalFreeSize) && (freeList_[count] >= entities_.size())) {
            ++count;
        }
        if (count) {
            freeList_.erase(freeList_.cbegin(), freeList_.cbegin() + count);
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
        EntityState::State oldState;
        EntityState::State newState;
        EventType event;
        if (state) {
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
    for (uint32_t id = 0, count = static_cast<uint32_t>(entities_.size()); id < count; ++id) {
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
    auto del = [](Iterator* it) { delete static_cast<EntityManager::IteratorImpl*>(it); };
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
