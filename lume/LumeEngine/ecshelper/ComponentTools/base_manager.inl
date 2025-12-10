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

#include <core/log.h>
#ifndef NDEBUG
#include <base/containers/atomics.h>
#endif

CORE_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t MODIFIED = 0x80000000;

// Default initial reservation for 8 components/entities.
// Will resize as needed.
constexpr size_t INITIAL_COMPONENT_RESERVE_SIZE = 8;

template<typename Container>
inline auto ItemOrNull(Container&& v, IComponentManager::ComponentId index)
{
    if (index < v.size()) {
        return &(v[index]);
    }
    return (BASE_NS::remove_reference_t<decltype(v[index])>*)(nullptr);
}

template<typename EntityMap>
inline auto ItemId(EntityMap&& entities, Entity entity)
{
    if (EntityUtil::IsValid(entity)) {
        if (const auto it = entities.find(entity); it != entities.end()) {
            return it->second;
        }
    }
    return IComponentManager::INVALID_COMPONENT_ID;
}
} // namespace

// IPropertyApi
template<typename ComponentType, typename BaseClass>
IPropertyHandle* BaseManager<ComponentType, BaseClass>::Create() const
{
    return new BaseComponentHandle(const_cast<BaseManager<ComponentType, BaseClass>*>(this), {}, {});
}

template<typename ComponentType, typename BaseClass>
IPropertyHandle* BaseManager<ComponentType, BaseClass>::Clone(const IPropertyHandle* src) const
{
    if (src->Owner() == this) {
        auto* h = static_cast<const BaseComponentHandle*>(src);
        return new BaseComponentHandle(const_cast<BaseManager<ComponentType, BaseClass>*>(this), {}, h->data_);
    }
    return nullptr;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Release(IPropertyHandle* dst) const
{
    if (dst && (dst->Owner() == this)) {
        // we can only destroy things we "own" (know)
        auto* handle = static_cast<BaseComponentHandle*>(dst);
        if (auto id = GetComponentId(handle->entity_); id != IComponentManager::INVALID_COMPONENT_ID) {
            if (&components_[id] == handle) {
                // This is one of the components (bound to an entity) so do nothing
                return;
            }
        }
        delete handle;
    }
}

template<typename ComponentType, typename BaseClass>
uint64_t BaseManager<ComponentType, BaseClass>::Type() const
{
    return typeHash_;
}

// IComponentManager
template<typename ComponentType, typename BaseClass>
BASE_NS::string_view BaseManager<ComponentType, BaseClass>::GetName() const
{
    return name_;
}

template<typename ComponentType, typename BaseClass>
BASE_NS::Uid BaseManager<ComponentType, BaseClass>::GetUid() const
{
    return BaseClass::UID;
}

template<typename ComponentType, typename BaseClass>
size_t BaseManager<ComponentType, BaseClass>::GetComponentCount() const
{
    return components_.size();
}

template<typename ComponentType, typename BaseClass>
const IPropertyApi& BaseManager<ComponentType, BaseClass>::GetPropertyApi() const
{
    return *this;
}

template<typename ComponentType, typename BaseClass>
CORE_NS::Entity BaseManager<ComponentType, BaseClass>::GetEntity(ComponentId index) const
{
    if (index < components_.size()) {
        return components_[index].entity_;
    }
    return CORE_NS::Entity();
}

template<typename ComponentType, typename BaseClass>
uint32_t BaseManager<ComponentType, BaseClass>::GetComponentGeneration(ComponentId index) const
{
    if (index < components_.size()) {
        return components_[index].generation_;
    }
    return 0;
}

template<typename ComponentType, typename BaseClass>
bool BaseManager<ComponentType, BaseClass>::HasComponent(CORE_NS::Entity entity) const
{
    return GetComponentId(entity) != IComponentManager::INVALID_COMPONENT_ID;
}

template<typename ComponentType, typename BaseClass>
IComponentManager::ComponentId BaseManager<ComponentType, BaseClass>::GetComponentId(CORE_NS::Entity entity) const
{
    if (EntityUtil::IsValid(entity)) {
        if (auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            return it->second;
        }
    }
    return IComponentManager::INVALID_COMPONENT_ID;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Create(CORE_NS::Entity entity)
{
    if (EntityUtil::IsValid(entity)) {
        if (auto it = entityComponent_.find(entity); it == entityComponent_.end()) {
            entityComponent_.insert({ entity, static_cast<ComponentId>(components_.size()) });
            const auto oldCapacity = components_.capacity();
            components_.emplace_back(this, entity);
            if (components_.capacity() != oldCapacity) {
                moved_.reserve(moved_.size() + components_.size());
                for (const auto& handle : components_) {
                    moved_.push_back(handle.entity_);
                }
                modifiedFlags_ |= CORE_COMPONENT_MANAGER_COMPONENT_MOVED_BIT;
            }
            added_.push_back(entity);
            modifiedFlags_ |= CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT;
            ++generationCounter_;
        } else {
            if (auto dst = ScopedHandle<ComponentType>(&components_[it->second]); dst) {
                *dst = {};
            }
        }
    }
}

template<typename ComponentType, typename BaseClass>
bool BaseManager<ComponentType, BaseClass>::Destroy(CORE_NS::Entity entity)
{
    if (EntityUtil::IsValid(entity)) {
        if (auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            components_[it->second].entity_ = {}; // invalid entity. (marks it as ready for re-use)
            entityComponent_.erase(it);
            removed_.push_back(entity);
            modifiedFlags_ |= CORE_COMPONENT_MANAGER_COMPONENT_REMOVED_BIT;
            ++generationCounter_;
            return true;
        }
    }
    return false;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Gc()
{
    const bool hasRemovedComponents = modifiedFlags_ & CORE_COMPONENT_MANAGER_COMPONENT_REMOVED_BIT;
    if (!hasRemovedComponents) {
        return;
    }
    ComponentId componentCount = static_cast<ComponentId>(components_.size());
    for (ComponentId id = 0; id < componentCount;) {
        if (EntityUtil::IsValid(components_[id].entity_)) {
            ++id;
            continue;
        }
        // invalid entity.. if so clean garbage
        // find last valid and swap with it
        ComponentId rid = componentCount - 1;
        while ((rid > id) && !EntityUtil::IsValid(components_[rid].entity_)) {
            --rid;
        }
        if ((rid > id) && EntityUtil::IsValid(components_[rid].entity_)) {
            moved_.push_back(components_[rid].entity_);
            // fix the entityComponent_ map (update the component id for the entity)
            entityComponent_[components_[rid].entity_] = id;
            components_[id] = BASE_NS::move(components_[rid]);
        }
        --componentCount;
    }
    if (!moved_.empty()) {
        modifiedFlags_ |= CORE_COMPONENT_MANAGER_COMPONENT_MOVED_BIT;
    }
    if (components_.size() > componentCount) {
        auto diff = static_cast<typename decltype(components_)::difference_type>(componentCount);
        components_.erase(components_.cbegin() + diff, components_.cend());
    }
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Destroy(BASE_NS::array_view<const CORE_NS::Entity> gcList)
{
    for (const CORE_NS::Entity e : gcList) {
        Destroy(e);
    }
}

template<typename ComponentType, typename BaseClass>
BASE_NS::vector<CORE_NS::Entity> BaseManager<ComponentType, BaseClass>::GetAddedComponents()
{
    return BASE_NS::move(added_);
}

template<typename ComponentType, typename BaseClass>
BASE_NS::vector<CORE_NS::Entity> BaseManager<ComponentType, BaseClass>::GetRemovedComponents()
{
    return BASE_NS::move(removed_);
}

template<typename ComponentType, typename BaseClass>
BASE_NS::vector<CORE_NS::Entity> BaseManager<ComponentType, BaseClass>::GetUpdatedComponents()
{
    BASE_NS::vector<CORE_NS::Entity> updated;
    if (modifiedFlags_ & MODIFIED) {
        modifiedFlags_ &= ~MODIFIED;
        updated.reserve(components_.size() / 2U); // 2: approximation for vector reserve size
        for (auto& handle : components_) {
            if (handle.dirty_) {
                handle.dirty_ = false;
                updated.push_back(handle.entity_);
            }
        }
    }
    return updated;
}

template<typename ComponentType, typename BaseClass>
BASE_NS::vector<CORE_NS::Entity> BaseManager<ComponentType, BaseClass>::GetMovedComponents()
{
    return BASE_NS::move(moved_);
}

template<typename ComponentType, typename BaseClass>
CORE_NS::ComponentManagerModifiedFlags BaseManager<ComponentType, BaseClass>::GetModifiedFlags() const
{
    return modifiedFlags_ & ~MODIFIED;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::ClearModifiedFlags()
{
    modifiedFlags_ &= MODIFIED;
}

template<typename ComponentType, typename BaseClass>
uint32_t BaseManager<ComponentType, BaseClass>::GetGenerationCounter() const
{
    return generationCounter_;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::SetData(CORE_NS::Entity entity, const IPropertyHandle& dataHandle)
{
    if (!IsMatchingHandle(dataHandle)) {
        return;
    }
    if (const auto src = ScopedHandle<const ComponentType>(&dataHandle); src) {
        if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            if (auto dst = ScopedHandle<ComponentType>(&components_[it->second]); dst) {
                *dst = *src;
            }
        }
    }
}

template<typename ComponentType, typename BaseClass>
const IPropertyHandle* BaseManager<ComponentType, BaseClass>::GetData(CORE_NS::Entity entity) const
{
    return ItemOrNull(components_, ItemId(entityComponent_, entity));
}

template<typename ComponentType, typename BaseClass>
IPropertyHandle* BaseManager<ComponentType, BaseClass>::GetData(CORE_NS::Entity entity)
{
    return ItemOrNull(components_, ItemId(entityComponent_, entity));
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::SetData(ComponentId index, const IPropertyHandle& dataHandle)
{
    if (!IsMatchingHandle(dataHandle)) {
        // We could verify the metadata here.
        // And in copy only the matching properties one-by-one also.
        return;
    }
    if (index < components_.size()) {
        if (const auto src = ScopedHandle<const ComponentType>(&dataHandle); src) {
            if (auto dst = ScopedHandle<ComponentType>(&components_[index]); dst) {
                *dst = *src;
            }
        }
    }
}

template<typename ComponentType, typename BaseClass>
const IPropertyHandle* BaseManager<ComponentType, BaseClass>::GetData(ComponentId index) const
{
    return ItemOrNull(components_, index);
}

template<typename ComponentType, typename BaseClass>
IPropertyHandle* BaseManager<ComponentType, BaseClass>::GetData(ComponentId index)
{
    return ItemOrNull(components_, index);
}

template<typename ComponentType, typename BaseClass>
IEcs& BaseManager<ComponentType, BaseClass>::GetEcs() const
{
    return ecs_;
}

// "base class"
template<typename ComponentType, typename BaseClass>
ComponentType BaseManager<ComponentType, BaseClass>::Get(ComponentId index) const
{
    if (auto handle = ScopedHandle<const ComponentType>(ItemOrNull(components_, index))) {
        return *handle;
    }
    return ComponentType {};
}

template<typename ComponentType, typename BaseClass>
ComponentType BaseManager<ComponentType, BaseClass>::Get(CORE_NS::Entity entity) const
{
    if (auto handle = ScopedHandle<const ComponentType>(ItemOrNull(components_, ItemId(entityComponent_, entity)))) {
        return *handle;
    }
    return ComponentType {};
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Set(ComponentId index, const ComponentType& data)
{
    if (auto handle = ScopedHandle<ComponentType>(ItemOrNull(components_, index))) {
        *handle = data;
    }
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Set(CORE_NS::Entity entity, const ComponentType& data)
{
    if (EntityUtil::IsValid(entity)) {
        if (const auto it = entityComponent_.find(entity); it == entityComponent_.end()) {
            entityComponent_.insert({ entity, static_cast<ComponentId>(components_.size()) });
            const auto oldCapacity = components_.capacity();
            components_.emplace_back(this, entity, data).generation_ = 1;
            if (components_.capacity() != oldCapacity) {
                moved_.reserve(moved_.size() + components_.size());
                for (const auto& handle : components_) {
                    moved_.push_back(handle.entity_);
                }
                modifiedFlags_ |= CORE_COMPONENT_MANAGER_COMPONENT_MOVED_BIT;
            }
            added_.push_back(entity);
            modifiedFlags_ |= CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT;
            ++generationCounter_;
        } else {
            if (auto handle = ScopedHandle<ComponentType>(&components_[it->second]); handle) {
                *handle = data;
            }
        }
    }
}

template<typename ComponentType, typename BaseClass>
ScopedHandle<const ComponentType> BaseManager<ComponentType, BaseClass>::Read(ComponentId index) const
{
    return ScopedHandle<const ComponentType> { ItemOrNull(components_, index) };
}

template<typename ComponentType, typename BaseClass>
ScopedHandle<const ComponentType> BaseManager<ComponentType, BaseClass>::Read(CORE_NS::Entity entity) const
{
    return ScopedHandle<const ComponentType> { ItemOrNull(components_, ItemId(entityComponent_, entity)) };
}

template<typename ComponentType, typename BaseClass>
ScopedHandle<ComponentType> BaseManager<ComponentType, BaseClass>::Write(ComponentId index)
{
    return ScopedHandle<ComponentType> { ItemOrNull(components_, index) };
}

template<typename ComponentType, typename BaseClass>
ScopedHandle<ComponentType> BaseManager<ComponentType, BaseClass>::Write(CORE_NS::Entity entity)
{
    return ScopedHandle<ComponentType> { ItemOrNull(components_, ItemId(entityComponent_, entity)) };
}

// internal
template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Updated(CORE_NS::Entity entity)
{
    CORE_ASSERT_MSG(EntityUtil::IsValid(entity), "Invalid ComponentId, bound to INVALID_ENTITY");
    modifiedFlags_ |= CORE_COMPONENT_MANAGER_COMPONENT_UPDATED_BIT | MODIFIED;
    ++generationCounter_;
}

template<typename ComponentType, typename BaseClass>
BaseManager<ComponentType, BaseClass>::BaseManager(IEcs& ecs, const BASE_NS::string_view name) noexcept
    : BaseManager(ecs, name, INITIAL_COMPONENT_RESERVE_SIZE)
{}

template<typename ComponentType, typename BaseClass>
BaseManager<ComponentType, BaseClass>::BaseManager(
    IEcs& ecs, const BASE_NS::string_view name, const size_t preallocate) noexcept
    : ecs_(ecs), name_(name), typeHash_(BASE_NS::FNV1aHash(name.data(), name.size()))
{
    if (preallocate) {
        components_.reserve(preallocate);
        entityComponent_.reserve(preallocate);
    }
}

template<typename ComponentType, typename BaseClass>
BaseManager<ComponentType, BaseClass>::~BaseManager()
{
    CORE_ASSERT(GetComponentCount() == 0);
}

template<typename ComponentType, typename BaseClass>
bool BaseManager<ComponentType, BaseClass>::IsMatchingHandle(const IPropertyHandle& dataHandle)
{
    if (dataHandle.Owner() == this) {
        return true;
    }
    if (dataHandle.Owner() && (dataHandle.Owner()->Type() == typeHash_)) {
        return true;
    }
    return false;
}

// handle implementation
template<typename ComponentType, typename BaseClass>
BaseManager<ComponentType, BaseClass>::BaseComponentHandle::BaseComponentHandle(
    BaseManager* owner, CORE_NS::Entity entity) noexcept
    : manager_(owner), entity_(entity)
{}

template<typename ComponentType, typename BaseClass>
BaseManager<ComponentType, BaseClass>::BaseComponentHandle::BaseComponentHandle(
    BaseManager* owner, CORE_NS::Entity entity, const ComponentType& data) noexcept
    : manager_(owner), entity_(entity), data_(data)
{}

template<typename ComponentType, typename BaseClass>
BaseManager<ComponentType, BaseClass>::BaseComponentHandle::BaseComponentHandle(BaseComponentHandle&& other) noexcept
    :
#ifndef NDEBUG
      rLocked_(BASE_NS::exchange(other.rLocked_, 0U)), wLocked_(BASE_NS::exchange(other.wLocked_, false)),
#endif
      manager_(other.manager_), generation_(BASE_NS::exchange(other.generation_, 0U)),
      entity_(BASE_NS::exchange(other.entity_, {})), data_(BASE_NS::exchange(other.data_, {}))
{
#ifndef NDEBUG
    CORE_ASSERT((rLocked_ == 0U) && !wLocked_);
#endif
}

template<typename ComponentType, typename BaseClass>
typename BaseManager<ComponentType, BaseClass>::BaseComponentHandle&
BaseManager<ComponentType, BaseClass>::BaseComponentHandle::operator=(BaseComponentHandle&& other) noexcept
{
    if (this != &other) {
        CORE_ASSERT(manager_ == other.manager_);
#ifndef NDEBUG
        CORE_ASSERT((other.rLocked_ == 0U) && !other.wLocked_);
        rLocked_ = BASE_NS::exchange(other.rLocked_, 0U);
        wLocked_ = BASE_NS::exchange(other.wLocked_, false);
#endif
        generation_ = BASE_NS::exchange(other.generation_, 0U);
        entity_ = BASE_NS::exchange(other.entity_, {});
        data_ = BASE_NS::exchange(other.data_, {});
    }
    return *this;
}

template<typename ComponentType, typename BaseClass>
const IPropertyApi* BaseManager<ComponentType, BaseClass>::BaseComponentHandle::Owner() const
{
    return manager_;
}

template<typename ComponentType, typename BaseClass>
size_t BaseManager<ComponentType, BaseClass>::BaseComponentHandle::Size() const
{
    return sizeof(ComponentType);
}

template<typename ComponentType, typename BaseClass>
const void* BaseManager<ComponentType, BaseClass>::BaseComponentHandle::RLock() const
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(!wLocked_);
    BASE_NS::AtomicIncrementRelaxed(&rLocked_);
#endif
    return &data_;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::BaseComponentHandle::RUnlock() const
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(rLocked_ > 0U);
    BASE_NS::AtomicDecrementRelaxed(&rLocked_);
#endif
}

template<typename ComponentType, typename BaseClass>
void* BaseManager<ComponentType, BaseClass>::BaseComponentHandle::WLock()
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(rLocked_ <= 1U && !wLocked_);
    wLocked_ = true;
#endif
    return &data_;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::BaseComponentHandle::WUnlock()
{
    CORE_ASSERT(manager_);
#ifndef NDEBUG
    CORE_ASSERT(wLocked_);
    wLocked_ = false;
#endif
    // update generation etc..
    ++generation_;
    if (EntityUtil::IsValid(entity_)) {
        dirty_ = true;
        manager_->Updated(entity_);
    }
}
CORE_END_NAMESPACE()
