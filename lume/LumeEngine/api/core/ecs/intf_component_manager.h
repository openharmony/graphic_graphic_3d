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

#ifndef API_CORE_ECS_ICOMPONENT_MANAGER_H
#define API_CORE_ECS_ICOMPONENT_MANAGER_H

#include <cstddef>
#include <cstdint>

#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/uid.h>
#include <core/ecs/entity.h>
#include <core/namespace.h>

BASE_BEGIN_NAMESPACE()
template<class T>
class array_view;
BASE_END_NAMESPACE()

CORE_BEGIN_NAMESPACE()
class IPropertyApi;
class IPropertyHandle;
class IEcs;

/** \addtogroup group_ecs_icomponentmanager
 *  @{
 */
/** Component manager modified flag bits */
enum ComponentManagerModifiedFlagBits {
    /** Component added bit */
    CORE_COMPONENT_MANAGER_COMPONENT_ADDED_BIT = 0x00000001,
    /** Component removed bit */
    CORE_COMPONENT_MANAGER_COMPONENT_REMOVED_BIT = 0x00000002,
    /** Component updated bit */
    CORE_COMPONENT_MANAGER_COMPONENT_UPDATED_BIT = 0x00000004,
    /** Modified flag bits max enumeration */
    CORE_COMPONENT_MANAGER_MODIFIED_FLAG_BITS_MAX_ENUM = 0x7FFFFFFF
};
/** Container for component manager modified flag bits */
using ComponentManagerModifiedFlags = uint32_t;

/**
Component Manager.

*/
class IComponentManager {
public:
    using ComponentId = uint32_t;
    static constexpr ComponentId INVALID_COMPONENT_ID = 0xFFFFFFFF;

    /** Returns the name of the component type.
     */
    virtual BASE_NS::string_view GetName() const = 0;

    /** Returns the UID of the component type.
     */
    virtual BASE_NS::Uid GetUid() const = 0;

    /** Returns the total number of components. Includes components that are being destroyed but are not yet garbage
     *  collected.
     */
    virtual size_t GetComponentCount() const = 0;

    /** Access to metadata defining the contents of the component.
     */
    virtual const IPropertyApi& GetPropertyApi() const = 0;

    /** Returns entity for component index. May return invalid entity if the entity is being destroyed.
     */
    virtual Entity GetEntity(IComponentManager::ComponentId index) const = 0;

    /** Get component generation id, which is incremented every time the component data is changed.
     */
    virtual uint32_t GetComponentGeneration(IComponentManager::ComponentId index) const = 0;

    // Entity api.
    /** Checks if Entity has component.
     *  @param entity Entity to be checked.
     */
    virtual bool HasComponent(Entity entity) const = 0;

    /** Retrieves component id for a given entity.
     *  @param entity Entity to be used as a search key.
     */
    virtual IComponentManager::ComponentId GetComponentId(Entity entity) const = 0;

    /** Creates a default component for entity.
     *  @param entity Entity which for component is created.
     */
    virtual void Create(Entity entity) = 0;

    /** Removes component from entity.
     *  @param entity Entity where component is removed.
     */
    virtual bool Destroy(Entity entity) = 0;

    /** Garbage collect components.
     *  This is called automatically by the ECS, but can be called by the user too to force cleanup.
     */
    virtual void Gc() = 0;

    /** Remove components from entities.
     *  @param gcList List of entities to have their components removed
     */
    virtual void Destroy(BASE_NS::array_view<const Entity> gcList) = 0;

    /** Get list of entities that have new components of this type (since last call).
     */
    virtual BASE_NS::vector<Entity> GetAddedComponents() = 0;

    /** Get list of entities that no longer have components of this type (since last call).
     */
    virtual BASE_NS::vector<Entity> GetRemovedComponents() = 0;

    /** Get list of entities that have been modified (since last call).
     */
    virtual BASE_NS::vector<Entity> GetUpdatedComponents() = 0;

    /** Returns flags if component is added, removed or updated.
     */
    virtual ComponentManagerModifiedFlags GetModifiedFlags() const = 0;

    /** Clears all flags of component (Application developer should not call this since its been automatically called
     * after every frame).
     */
    virtual void ClearModifiedFlags() = 0;

    /** Number of changes occured in this component manager since start of its life.
     */
    virtual uint32_t GetGenerationCounter() const = 0;

    /** Set data for entity. Copies the data from "data" handle to the component for entity.
     *  @param entity Entity which is set-up.
     *  @param data property handle for entity.
     */
    virtual void SetData(Entity entity, const IPropertyHandle& data) = 0;

    /** Get data for entity. This handle can be used to directly read the component for entity.
     *  @param entity Entity where we get our property handle.
     */
    virtual const IPropertyHandle* GetData(Entity entity) const = 0;

    /** Get data for entity. This handle can be used to directly modify the component for entity.
     *  @param entity Entity where we get our property handle.
     */
    virtual IPropertyHandle* GetData(Entity entity) = 0;

    /** Set data for entity. Copies the data from "data" handle to the component.
     *  @param index Index what is used to add or update the component if index is same as before.
     *  @param data Data which is set to component.
     */
    virtual void SetData(ComponentId index, const IPropertyHandle& data) = 0;

    /** Get data for entity. This handle can be used to directly read the component for entity.
     *  @param index Index to get data from
     */
    virtual const IPropertyHandle* GetData(ComponentId index) const = 0;

    /** Get data for entity. This handle can be used to directly modify the component for entity.
     *  @param index Index to get data from
     */
    virtual IPropertyHandle* GetData(ComponentId index) = 0;

    /** Get the ECS instance using this manager.
     * @return Reference to owning ECS instance.
     */
    virtual IEcs& GetEcs() const = 0;

protected:
    IComponentManager() = default;
    IComponentManager(const IComponentManager&) = delete;
    IComponentManager(IComponentManager&&) = delete;
    IComponentManager& operator=(const IComponentManager&) = delete;
    virtual ~IComponentManager() = default;
};

/** Get name */
template<class T>
inline constexpr BASE_NS::string_view GetName()
{
    return GetName((const T*)nullptr);
}

/** Get UID */
template<class T>
inline constexpr BASE_NS::Uid GetUid()
{
    return GetUid((const T*)nullptr);
}

/** Create component */
template<class T>
inline auto CreateComponent(T& componentManager, const Entity& entity)
{
    componentManager.Create(entity);
    return componentManager.Get(entity);
}

/** @} */
CORE_END_NAMESPACE()

#endif // API_CORE_ECS_ICOMPONENT_MANAGER_H
