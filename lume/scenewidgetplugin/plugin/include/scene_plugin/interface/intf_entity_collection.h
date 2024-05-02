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

#ifndef SCENEPLUGIN_INTF_ENTITYCOLLECTION_H
#define SCENEPLUGIN_INTF_ENTITYCOLLECTION_H

#include <scene_plugin/namespace.h>

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_ecs.h>

SCENE_BEGIN_NAMESPACE()

class IEntityCollection {
public:
    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IEntityCollection* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = BASE_NS::unique_ptr<IEntityCollection, Deleter>;

    class IListener {
    public:
        virtual void ModifiedChanged(IEntityCollection& entityCollection, bool modified) = 0;
    };
    virtual void AddListener(IListener& listener) = 0;
    virtual void RemoveListener(IListener& listener) = 0;

    // A list of property paths in a component to be serialized.
    using PropertyList = BASE_NS::vector<BASE_NS::string>;

    // Creates a new Empty Entity Collection with the same ECS as source
    virtual IEntityCollection::Ptr CreateNewEntityCollection(
        BASE_NS::string_view uri, BASE_NS::string_view contextUri) = 0;

    virtual CORE_NS::IEcs& GetEcs() const = 0;
    virtual BASE_NS::string GetUri() const = 0;
    virtual void SetUri(const BASE_NS::string& uri) = 0;
    virtual BASE_NS::string GetContextUri() const = 0;

    virtual BASE_NS::string GetSrc() const = 0;
    virtual void SetSrc(BASE_NS::string_view src) = 0;

    virtual BASE_NS::string GetType() const = 0;
    virtual void SetType(BASE_NS::string_view type) = 0;

    // Entities.
    virtual size_t GetEntityCount() const = 0;
    virtual CORE_NS::EntityReference GetEntity(size_t collectionIndex) const = 0;
    virtual CORE_NS::EntityReference GetEntity(BASE_NS::string_view localContextId) const = 0;
    virtual CORE_NS::EntityReference GetEntityRecursive(BASE_NS::string_view localContextId) const = 0;
    virtual BASE_NS::array_view<const CORE_NS::EntityReference> GetEntities() const = 0;
    virtual void AddEntity(CORE_NS::EntityReference entitity) = 0;
    virtual void AddEntities(BASE_NS::array_view<const CORE_NS::EntityReference> entities) = 0;
    virtual bool RemoveEntity(CORE_NS::EntityReference entitity) = 0;
    virtual void RemoveEntities(BASE_NS::array_view<const CORE_NS::EntityReference> entities) = 0;
    virtual void RemoveEntityRecursive(CORE_NS::Entity entity) = 0;
    virtual void SetId(BASE_NS::string_view id, CORE_NS::EntityReference entity) = 0;
    virtual BASE_NS::string_view GetId(CORE_NS::Entity entity) const = 0;
    virtual BASE_NS::string_view GetIdRecursive(CORE_NS::Entity entity) const = 0;

    virtual void SetUniqueIdentifier(BASE_NS::string_view id, CORE_NS::EntityReference entity) = 0;
    virtual BASE_NS::string_view GetUniqueIdentifier(CORE_NS::Entity entity) const = 0;
    virtual BASE_NS::string_view GetUniqueIdentifierRecursive(CORE_NS::Entity entity) const = 0;

    // Sub-collections.
    virtual size_t GetSubCollectionCount() const = 0;
    virtual IEntityCollection* GetSubCollection(size_t index) = 0;
    virtual const IEntityCollection* GetSubCollection(size_t index) const = 0;
    virtual int32_t GetSubCollectionIndex(BASE_NS::string_view uri) const = 0;
    virtual int32_t GetSubCollectionIndexByRoot(CORE_NS::Entity entity) const = 0;
    virtual IEntityCollection& AddSubCollection(
        BASE_NS::string_view uri, BASE_NS::string_view contextUri, bool serializable = true) = 0;
    virtual IEntityCollection& AddSubCollectionClone(IEntityCollection& collection, BASE_NS::string_view uri) = 0;
    virtual void RemoveSubCollection(size_t index) = 0;

    // All entities recursively.
    virtual size_t GetEntityCountRecursive(bool includeDestroyed, bool includeNonSerialized = true) const = 0;
    virtual void GetEntitiesRecursive(bool includeDestroyed, BASE_NS::vector<CORE_NS::EntityReference>& entitiesOut,
        bool includeNonSerialized = true) const = 0;

    virtual bool Contains(CORE_NS::Entity entity) const = 0;
    virtual bool IsExternal(CORE_NS::Entity entity) const = 0;
    virtual bool isSubCollectionRoot(CORE_NS::Entity entity) const = 0;
    virtual CORE_NS::EntityReference GetReference(CORE_NS::Entity entity) const = 0;

    virtual void SetActive(bool active) = 0;
    virtual bool IsActive() const = 0;

    virtual void MarkDestroyed(bool destroyed) = 0;
    virtual bool IsMarkedDestroyed() const = 0;

    virtual void MarkModified(bool modified) = 0;
    virtual void MarkModified(bool modified, bool recursive) = 0;
    virtual bool IsMarkedModified() const = 0;

    virtual void Clear() = 0;

    virtual void CopyContents(IEntityCollection& srcCollection) = 0;

    virtual BASE_NS::vector<CORE_NS::EntityReference> CopyContentsWithSerialization(
        IEntityCollection& srcCollection) = 0;
    virtual BASE_NS::vector<CORE_NS::EntityReference> CopyContentsWithSerialization(
        IEntityCollection& srcCollection, BASE_NS::array_view<const CORE_NS::EntityReference> entities) = 0;

    virtual void AddEntityToSubcollection(
        BASE_NS::string_view collection, BASE_NS::string_view name, CORE_NS::Entity entity, bool makeUnique = true) = 0;

    // Serialization state.
    virtual bool MarkComponentSerialized(CORE_NS::Entity entity, BASE_NS::Uid component, bool serialize) = 0;
    virtual bool MarkAllPropertiesSerialized(CORE_NS::Entity entity, BASE_NS::Uid component) = 0;
    virtual bool MarkPropertySerialized(
        CORE_NS::Entity entity, BASE_NS::Uid component, BASE_NS::string_view propertyPath, bool serialize) = 0;
    virtual bool IsPropertySerialized(
        CORE_NS::Entity entity, BASE_NS::Uid component, BASE_NS::string_view propertyPath) = 0;
    virtual const PropertyList* GetSerializedProperties(CORE_NS::Entity entity, BASE_NS::Uid component) const = 0;

    virtual bool IsSerialized() const = 0;
    virtual void SetSerialized(bool serialize) = 0;

protected:
    IEntityCollection() = default;
    virtual ~IEntityCollection() = default;
    virtual void Destroy() = 0;
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_INTF_ENTITYCOLLECTION_H
