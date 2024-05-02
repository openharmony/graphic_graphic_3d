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

#ifndef SCENEPLUGIN_ENTITYCOLLECTION_H
#define SCENEPLUGIN_ENTITYCOLLECTION_H

#include <scene_plugin/interface/intf_entity_collection.h>

#include <base/containers/unique_ptr.h>
#include <base/containers/unordered_map.h>

SCENE_BEGIN_NAMESPACE()

class EntityCollection : public IEntityCollection, private IEntityCollection::IListener {
public:
    using Ptr = BASE_NS::unique_ptr<EntityCollection, Deleter>;

    EntityCollection(CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri);

    void AddListener(IEntityCollection::IListener& listener) override;
    void RemoveListener(IEntityCollection::IListener& listener) override;

    //
    // From IEntityCollection
    //

    IEntityCollection::Ptr CreateNewEntityCollection(
        BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;

    CORE_NS::IEcs& GetEcs() const override;
    BASE_NS::string GetUri() const override;
    void SetUri(const BASE_NS::string& uri) override;

    BASE_NS::string GetContextUri() const override;

    BASE_NS::string GetSrc() const override;
    void SetSrc(BASE_NS::string_view src) override;

    BASE_NS::string GetType() const override;
    void SetType(BASE_NS::string_view type) override;

    size_t GetEntityCount() const override;
    CORE_NS::EntityReference GetEntity(size_t collectionIndex) const override;
    CORE_NS::EntityReference GetEntity(BASE_NS::string_view localContextId) const override;
    CORE_NS::EntityReference GetEntityRecursive(BASE_NS::string_view localContextId) const override;
    BASE_NS::array_view<const CORE_NS::EntityReference> GetEntities() const override;
    void AddEntity(CORE_NS::EntityReference entitity) override;
    void AddEntities(BASE_NS::array_view<const CORE_NS::EntityReference> entities) override;
    bool RemoveEntity(CORE_NS::EntityReference entitity) override;
    void RemoveEntities(BASE_NS::array_view<const CORE_NS::EntityReference> entities) override;
    void RemoveEntityRecursive(CORE_NS::Entity entity) override;

    void SetId(BASE_NS::string_view id, CORE_NS::EntityReference entity) override;
    BASE_NS::string_view GetId(CORE_NS::Entity entity) const override;
    BASE_NS::string_view GetIdRecursive(CORE_NS::Entity entity) const override;

    void SetUniqueIdentifier(BASE_NS::string_view id, CORE_NS::EntityReference entity) override;
    BASE_NS::string_view GetUniqueIdentifier(CORE_NS::Entity entity) const override;
    BASE_NS::string_view GetUniqueIdentifierRecursive(CORE_NS::Entity entity) const override;

    size_t GetSubCollectionCount() const override;
    IEntityCollection* GetSubCollection(size_t index) override;
    const IEntityCollection* GetSubCollection(size_t index) const override;
    int32_t GetSubCollectionIndex(BASE_NS::string_view uri) const override;
    int32_t GetSubCollectionIndexByRoot(CORE_NS::Entity entity) const override;
    IEntityCollection& AddSubCollection(
        BASE_NS::string_view uri, BASE_NS::string_view contextUri, bool serializable) override;
    IEntityCollection& AddSubCollectionClone(IEntityCollection& collection, BASE_NS::string_view uri) override;
    void RemoveSubCollection(size_t index) override;

    size_t GetEntityCountRecursive(bool includeDestroyed, bool includeNonSerialized = true) const override;
    void GetEntitiesRecursive(bool includeDestroyed, BASE_NS::vector<CORE_NS::EntityReference>& entitiesOut,
        bool includeNonSerialized = true) const override;

    bool Contains(CORE_NS::Entity entity) const override;
    bool IsExternal(CORE_NS::Entity entity) const override;
    bool isSubCollectionRoot(CORE_NS::Entity entity) const override;
    CORE_NS::EntityReference GetReference(CORE_NS::Entity entity) const override;

    void SetActive(bool active) override;
    bool IsActive() const override;

    void MarkDestroyed(bool destroyed) override;
    bool IsMarkedDestroyed() const override;

    void MarkModified(bool modified) override;
    void MarkModified(bool modified, bool recursive) override;
    bool IsMarkedModified() const override;

    void Clear() override;

    void CopyContents(IEntityCollection& srcCollection) override;

    BASE_NS::vector<CORE_NS::EntityReference> CopyContentsWithSerialization(IEntityCollection& srcCollection) override;
    BASE_NS::vector<CORE_NS::EntityReference> CopyContentsWithSerialization(
        IEntityCollection& srcCollection, BASE_NS::array_view<const CORE_NS::EntityReference> entities) override;

    bool MarkComponentSerialized(CORE_NS::Entity entity, BASE_NS::Uid component, bool serialize) override;
    bool MarkAllPropertiesSerialized(CORE_NS::Entity entity, BASE_NS::Uid component) override;
    bool MarkPropertySerialized(
        CORE_NS::Entity entity, BASE_NS::Uid component, BASE_NS::string_view propertyPath, bool serialize) override;
    bool IsPropertySerialized(
        CORE_NS::Entity entity, BASE_NS::Uid component, BASE_NS::string_view propertyPath) override;
    const PropertyList* GetSerializedProperties(CORE_NS::Entity entity, BASE_NS::Uid component) const override;

    void AddEntityToSubcollection(
        BASE_NS::string_view collection, BASE_NS::string_view name, CORE_NS::Entity entity, bool makeUnique) override;

    bool IsSerialized() const override;
    void SetSerialized(bool serialize) override;

protected:
    void Destroy() override;
    ~EntityCollection() override;

private:
    // Prevent copying.
    EntityCollection(const EntityCollection&) = delete;
    EntityCollection& operator=(const EntityCollection&) = delete;

    // From IEntityCollection::IListener
    void ModifiedChanged(IEntityCollection& entityCollection, bool modified) override;

    void DoGetEntitiesRecursive(
        bool includeDestroyed, bool includeNonSerialized, BASE_NS::vector<CORE_NS::EntityReference>& entitiesOut) const;

    void ClonePrivate(EntityCollection& dst) const;
    void DoCloneRecursive(EntityCollection& dst) const;

    // TODO: Create a better data structure for this information.
    // Components to be serialized in an entity.
    using ComponentMap = BASE_NS::unordered_map<BASE_NS::Uid, PropertyList>;
    BASE_NS::unordered_map<CORE_NS::Entity, ComponentMap> serializationInfo_;

    CORE_NS::IEcs& ecs_;
    BASE_NS::string uri_;
    BASE_NS::string contextUri_;

    BASE_NS::string src_ {};
    BASE_NS::string type_ {};

    BASE_NS::unordered_map<BASE_NS::string, CORE_NS::EntityReference> namedEntities_;
    BASE_NS::unordered_map<BASE_NS::string, CORE_NS::EntityReference> entityIdentifiers_;
    BASE_NS::vector<CORE_NS::EntityReference> entities_;

    BASE_NS::vector<EntityCollection::Ptr> collections_;

    bool isActive_ { true };
    bool isMarkedDestroyed_ { false };
    bool isMarkedModified_ { false };
    bool isSerialized_ { true };

    BASE_NS::vector<IEntityCollection::IListener*> listeners_;
};

SCENE_END_NAMESPACE()

#endif // SCENEPLUGIN_ENTITYCOLLECTION_H
