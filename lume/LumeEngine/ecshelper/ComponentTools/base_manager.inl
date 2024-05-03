/*
 * Copyright (C) 2023-2024 Huawei Device Co., Ltd.
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

CORE_BEGIN_NAMESPACE()
namespace {
constexpr uint32_t MODIFIED = 0x80000000;
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
            components_.emplace_back(this, entity);
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
        auto* it = &components_[id];
        // invalid entity.. if so clean garbage
        if (!EntityUtil::IsValid(it->entity_)) {
            // find last valid and swap with it
            for (ComponentId rid = componentCount - 1; rid > id; rid--) {
                auto* rit = &components_[rid];
                // valid entity? if so swap the components.
                if (EntityUtil::IsValid(rit->entity_)) {
                    // fix the entityComponent_ map (update the component id for the entity)
                    entityComponent_[rit->entity_] = id;
                    *it = BASE_NS::move(*rit);
                    break;
                }
            }
            --componentCount;
            continue;
        }
        ++id;
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
        updated.reserve(components_.size() / 2); // 2: aproximation for vector reserve size
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
    if (EntityUtil::IsValid(entity)) {
        if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            if (it->second < components_.size()) {
                return &components_[it->second];
            }
        }
    }
    return nullptr;
}

template<typename ComponentType, typename BaseClass>
IPropertyHandle* BaseManager<ComponentType, BaseClass>::GetData(CORE_NS::Entity entity)
{
    if (EntityUtil::IsValid(entity)) {
        if (const auto it = entityComponent_.find(entity); it != entityComponent_.end()) {
            if (it->second < components_.size()) {
                return &components_[it->second];
            }
        }
    }
    return nullptr;
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
    if (index < components_.size()) {
        return &components_[index];
    }
    return nullptr;
}

template<typename ComponentType, typename BaseClass>
IPropertyHandle* BaseManager<ComponentType, BaseClass>::GetData(ComponentId index)
{
    if (index < components_.size()) {
        return &components_[index];
    }
    return nullptr;
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
    CORE_ASSERT_MSG(index < components_.size(), "Invalid ComponentId");
    if (auto handle = ScopedHandle<const ComponentType>(GetData(index)); handle) {
        return *handle;
    }
    return ComponentType {};
}

template<typename ComponentType, typename BaseClass>
ComponentType BaseManager<ComponentType, BaseClass>::Get(CORE_NS::Entity entity) const
{
    if (auto handle = ScopedHandle<const ComponentType>(GetData(entity)); handle) {
        return *handle;
    }
    return ComponentType {};
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Set(ComponentId index, const ComponentType& data)
{
    if (auto handle = ScopedHandle<ComponentType>(GetData(index)); handle) {
        *handle = data;
    }
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::Set(CORE_NS::Entity entity, const ComponentType& data)
{
    if (EntityUtil::IsValid(entity)) {
        if (const auto it = entityComponent_.find(entity); it == entityComponent_.end()) {
            entityComponent_.insert({ entity, static_cast<ComponentId>(components_.size()) });
            components_.emplace_back(this, entity, data).generation_ = 1;
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
    return ScopedHandle<const ComponentType> { GetData(index) };
}

template<typename ComponentType, typename BaseClass>
ScopedHandle<const ComponentType> BaseManager<ComponentType, BaseClass>::Read(CORE_NS::Entity entity) const
{
    return ScopedHandle<const ComponentType> { GetData(entity) };
}

template<typename ComponentType, typename BaseClass>
ScopedHandle<ComponentType> BaseManager<ComponentType, BaseClass>::Write(ComponentId index)
{
    return ScopedHandle<ComponentType> { GetData(index) };
}

template<typename ComponentType, typename BaseClass>
ScopedHandle<ComponentType> BaseManager<ComponentType, BaseClass>::Write(CORE_NS::Entity entity)
{
    return ScopedHandle<ComponentType> { GetData(entity) };
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
    : ecs_(ecs), name_(name)
{
    // Initial reservation for 64 components/entities.
    // Will resize as needed.
    constexpr size_t INITIAL_COMPONENT_RESERVE_SIZE = 64;
    components_.reserve(INITIAL_COMPONENT_RESERVE_SIZE);
    entityComponent_.reserve(INITIAL_COMPONENT_RESERVE_SIZE);
    typeHash_ = BASE_NS::FNV1aHash(name_.data(), name_.size());
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
    : rLocked_(other.rLocked_.exchange(0U)), wLocked_(BASE_NS::exchange(other.wLocked_, false)),
      manager_(other.manager_), generation_(BASE_NS::exchange(other.generation_, 0U)),
      entity_(BASE_NS::exchange(other.entity_, {})), data_(BASE_NS::exchange(other.data_, {}))
{
    CORE_ASSERT((rLocked_ == 0U) && !wLocked_);
}

template<typename ComponentType, typename BaseClass>
typename BaseManager<ComponentType, BaseClass>::BaseComponentHandle&
BaseManager<ComponentType, BaseClass>::BaseComponentHandle::operator=(BaseComponentHandle&& other) noexcept
{
    if (this != &other) {
        CORE_ASSERT(manager_ == other.manager_);
        CORE_ASSERT((other.rLocked_ == 0U) && !other.wLocked_);
        rLocked_ = other.rLocked_.exchange(0U);
        wLocked_ = BASE_NS::exchange(other.wLocked_, false);
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
    CORE_ASSERT(!wLocked_);
    ++rLocked_;
    return &data_;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::BaseComponentHandle::RUnlock() const
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(rLocked_ > 0U);
    --rLocked_;
}

template<typename ComponentType, typename BaseClass>
void* BaseManager<ComponentType, BaseClass>::BaseComponentHandle::WLock()
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(rLocked_ <= 1U && !wLocked_);
    wLocked_ = true;
    return &data_;
}

template<typename ComponentType, typename BaseClass>
void BaseManager<ComponentType, BaseClass>::BaseComponentHandle::WUnlock()
{
    CORE_ASSERT(manager_);
    CORE_ASSERT(wLocked_);
    wLocked_ = false;
    // update generation etc..
    ++generation_;
    if (EntityUtil::IsValid(entity_)) {
        dirty_ = true;
        manager_->Updated(entity_);
    }
}
CORE_END_NAMESPACE()
