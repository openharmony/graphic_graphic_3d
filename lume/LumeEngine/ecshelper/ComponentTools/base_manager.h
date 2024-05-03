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
#ifndef CORE__ECS_HELPER__COMPONENT_TOOLS__BASE_MANAGER_H
#define CORE__ECS_HELPER__COMPONENT_TOOLS__BASE_MANAGER_H

#include <atomic>
#include <cstddef>
#include <cstdint>

#include <base/containers/array_view.h>
#include <base/containers/string_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>
#include <core/property/intf_property_api.h>
#include <core/property/intf_property_handle.h>
#include <core/property/scoped_handle.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
struct Property;

// BaseClass should inherit IComponentManager AND should follow the basic form of component managers
// (this will be enforced later)
template<typename ComponentType, typename BaseClass>
class BaseManager : public BaseClass, public IPropertyApi {
    using ComponentId = IComponentManager::ComponentId;

public:
    // IPropertyApi
    size_t PropertyCount() const override = 0;
    const Property* MetaData(size_t index) const override = 0;
    BASE_NS::array_view<const Property> MetaData() const override = 0;
    IPropertyHandle* Create() const override;
    IPropertyHandle* Clone(const IPropertyHandle*) const override;
    void Release(IPropertyHandle*) const override;
    uint64_t Type() const override;

    // IComponentManager
    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;
    size_t GetComponentCount() const override;
    const IPropertyApi& GetPropertyApi() const override;
    Entity GetEntity(ComponentId index) const override;
    uint32_t GetComponentGeneration(ComponentId index) const override;
    bool HasComponent(Entity entity) const override;
    IComponentManager::ComponentId GetComponentId(Entity entity) const override;
    void Create(Entity entity) override;
    bool Destroy(Entity entity) override;
    void Gc() override;
    void Destroy(BASE_NS::array_view<const Entity> gcList) override;
    BASE_NS::vector<Entity> GetAddedComponents() override;
    BASE_NS::vector<Entity> GetRemovedComponents() override;
    BASE_NS::vector<Entity> GetUpdatedComponents() override;
    CORE_NS::ComponentManagerModifiedFlags GetModifiedFlags() const override;
    void ClearModifiedFlags() override;
    uint32_t GetGenerationCounter() const override;
    void SetData(Entity entity, const IPropertyHandle& dataHandle) override;
    const IPropertyHandle* GetData(Entity entity) const override;
    IPropertyHandle* GetData(Entity entity) override;
    void SetData(ComponentId index, const IPropertyHandle& dataHandle) override;
    const IPropertyHandle* GetData(ComponentId index) const override;
    IPropertyHandle* GetData(ComponentId index) override;
    IEcs& GetEcs() const override;

    // "base class"
    ComponentType Get(ComponentId index) const override;
    ComponentType Get(Entity entity) const override;
    void Set(ComponentId index, const ComponentType& aData) override;
    void Set(Entity entity, const ComponentType& aData) override;
    ScopedHandle<const ComponentType> Read(ComponentId index) const override;
    ScopedHandle<const ComponentType> Read(Entity entity) const override;
    ScopedHandle<ComponentType> Write(ComponentId index) override;
    ScopedHandle<ComponentType> Write(Entity entity) override;

    // internal, non-public
    void Updated(Entity entity);

    BaseManager(const BaseManager&) = delete;
    BaseManager(BaseManager&&) = delete;
    BaseManager& operator=(const BaseManager&) = delete;
    BaseManager& operator=(BaseManager&&) = delete;

protected:
    BaseManager(IEcs& ecs, BASE_NS::string_view) noexcept;
    virtual ~BaseManager();
    IEcs& ecs_;
    BASE_NS::string_view name_;

    bool IsMatchingHandle(const IPropertyHandle& handle);

    class BaseComponentHandle : public IPropertyHandle {
    public:
        BaseComponentHandle() = delete;
        BaseComponentHandle(BaseManager* owner, Entity entity) noexcept;
        BaseComponentHandle(BaseManager* owner, Entity entity, const ComponentType& data) noexcept;
        ~BaseComponentHandle() override = default;
        BaseComponentHandle(const BaseComponentHandle& other) = delete;
        BaseComponentHandle(BaseComponentHandle&& other) noexcept;
        BaseComponentHandle& operator=(const BaseComponentHandle& other) = delete;
        BaseComponentHandle& operator=(BaseComponentHandle&& other) noexcept;
        const IPropertyApi* Owner() const override;
        size_t Size() const override;
        const void* RLock() const override;
        void RUnlock() const override;
        void* WLock() override;
        void WUnlock() override;

        mutable std::atomic_uint32_t rLocked_ { 0 };
        mutable bool wLocked_ { false };
        bool dirty_ { false };
        BaseManager* manager_ { nullptr };
        uint32_t generation_ { 0 };
        Entity entity_;
        ComponentType data_;
    };
    uint32_t generationCounter_ { 0 };
    uint32_t modifiedFlags_ { 0 };
    BASE_NS::unordered_map<Entity, ComponentId> entityComponent_;
    BASE_NS::vector<BaseComponentHandle> components_;
    BASE_NS::vector<Entity> added_;
    BASE_NS::vector<Entity> removed_;
    BASE_NS::vector<Entity> updated_;
    uint64_t typeHash_;
};
CORE_END_NAMESPACE()
#endif
