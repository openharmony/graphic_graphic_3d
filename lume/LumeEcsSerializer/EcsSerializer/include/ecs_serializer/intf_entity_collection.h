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

#ifndef API_ECS_SERIALIZER_ENTITYCOLLECTION_H
#define API_ECS_SERIALIZER_ENTITYCOLLECTION_H

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_ecs.h>
#include <ecs_serializer/namespace.h>

ECS_SERIALIZER_BEGIN_NAMESPACE()

class IEntityCollection {
public:
    struct Deleter {
        constexpr Deleter() noexcept = default;
        void operator()(IEntityCollection* ptr) const
        {
            ptr->Destroy();
        }
    };
    using Ptr = unique_ptr<IEntityCollection, Deleter>;

    class IListener {
    public:
        virtual void ModifiedChanged(IEntityCollection& entityCollection, bool modified) = 0;

    protected:
        virtual ~IListener() = default;
    };

    virtual void AddListener(IListener& listener) = 0;
    virtual void RemoveListener(IListener& listener) = 0;

    // A list of property paths in a component to be serialized.
    using PropertyList = vector<string>;

    virtual IEcs& GetEcs() const = 0;
    virtual string GetUri() const = 0;
    virtual string GetContextUri() const = 0;

    virtual string GetSrc() const = 0;
    virtual void SetSrc(string_view src) = 0;

    virtual string GetType() const = 0;
    virtual void SetType(string_view type) = 0;

    // Entities.
    virtual size_t GetEntityCount() const = 0;
    virtual EntityReference GetEntity(size_t collectionIndex) const = 0;
    virtual EntityReference GetEntity(string_view localContextId) const = 0;
    virtual array_view<const EntityReference> GetEntities() const = 0;
    virtual void AddEntity(EntityReference entitity) = 0;
    virtual void AddEntities(array_view<const EntityReference> entities) = 0;
    virtual bool RemoveEntity(EntityReference entitity) = 0;
    virtual void RemoveEntities(array_view<const EntityReference> entities) = 0;
    virtual void SetId(string_view id, EntityReference entity) = 0;
    virtual string_view GetId(Entity entity) const = 0;

    // Subcollections.
    virtual size_t GetSubCollectionCount() const = 0;
    virtual IEntityCollection* GetSubCollection(size_t index) = 0;
    virtual const IEntityCollection* GetSubCollection(size_t index) const = 0;
    virtual int32_t GetSubCollectionIndex(string_view uri) const = 0;
    virtual int32_t GetSubCollectionIndexByRoot(Entity entity) const = 0;
    virtual IEntityCollection& AddSubCollection(string_view uri, string_view contextUri) = 0;
    virtual IEntityCollection& AddSubCollectionClone(IEntityCollection& collection, string_view uri) = 0;
    virtual void RemoveSubCollection(size_t index) = 0;

    // All entities recursively.
    virtual size_t GetEntityCountRecursive(bool includeDestroyed) const = 0;
    virtual void GetEntitiesRecursive(
        bool includeDestroyed, vector<EntityReference>& entitiesOut) const = 0;

    virtual bool Contains(Entity entity) const = 0;
    virtual bool IsExternal(Entity entity) const = 0;
    virtual bool isSubCollectionRoot(Entity entity) const = 0;
    virtual EntityReference GetReference(Entity entity) const = 0;

    virtual void SetActive(bool active) = 0;
    virtual bool IsActive() const = 0;

    virtual void MarkDestroyed(bool destroyed) = 0;
    virtual bool IsMarkedDestroyed() const = 0;

    virtual void MarkModified(bool modified) = 0;
    virtual void MarkModified(bool modified, bool recursive) = 0;
    virtual bool IsMarkedModified() const = 0;

    virtual void Clear() = 0;

    virtual void CopyContents(IEntityCollection& srcCollection) = 0;

    virtual vector<EntityReference> CopyContentsWithSerialization(
        IEntityCollection& srcCollection) = 0;
    virtual vector<EntityReference> CopyContentsWithSerialization(
        IEntityCollection& srcCollection, array_view<const EntityReference> entities) = 0;

    // Serialization state.
    virtual bool MarkComponentSerialized(Entity entity, Uid component, bool serialize) = 0;
    virtual bool MarkAllPropertiesSerialized(Entity entity, Uid component) = 0;
    virtual bool MarkPropertySerialized(
        Entity entity, Uid component, string_view propertyPath, bool serialize) = 0;
    virtual bool IsPropertySerialized(
        Entity entity, Uid component, string_view propertyPath) = 0;
    virtual const PropertyList* GetSerializedProperties(Entity entity, Uid component) const = 0;

protected:
    IEntityCollection() = default;
    virtual ~IEntityCollection() = default;
    virtual void Destroy() = 0;
};

ECS_SERIALIZER_END_NAMESPACE()

#endif // API_ECS_SERIALIZER_ENTITYCOLLECTION_H