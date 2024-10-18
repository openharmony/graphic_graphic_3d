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

#include <cinttypes>

#include <core/ecs/intf_entity_manager.h>
#include <core/log.h>
#include <core/property/intf_property_api.h>

#include "ecs_serializer/ecs_clone_util.h"
#include "ecs_serializer/intf_entity_collection.h"

using namespace BASE_NS;
using namespace CORE_NS;

ECS_SERIALIZER_BEGIN_NAMESPACE()

class EntityCollection : public IEntityCollection, private IEntityCollection::IListener {
public:
    using Ptr = BASE_NS::unique_ptr<EntityCollection, Deleter>;

    EntityCollection(CORE_NS::IEcs& ecs, BASE_NS::string_view uri, BASE_NS::string_view contextUri);

    void AddListener(IEntityCollection::IListener& listener) override;
    void RemoveListener(IEntityCollection::IListener& listener) override;

    //
    // From IEntityCollection
    //
    CORE_NS::IEcs& GetEcs() const override;
    BASE_NS::string GetUri() const override;
    BASE_NS::string GetContextUri() const override;

    BASE_NS::string GetSrc() const override;
    void SetSrc(BASE_NS::string_view src) override;

    BASE_NS::string GetType() const override;
    void SetType(BASE_NS::string_view type) override;

    size_t GetEntityCount() const override;
    CORE_NS::EntityReference GetEntity(size_t collectionIndex) const override;
    CORE_NS::EntityReference GetEntity(BASE_NS::string_view localContextId) const override;
    BASE_NS::array_view<const CORE_NS::EntityReference> GetEntities() const override;
    void AddEntity(CORE_NS::EntityReference entitity) override;
    void AddEntities(BASE_NS::array_view<const CORE_NS::EntityReference> entities) override;
    bool RemoveEntity(CORE_NS::EntityReference entitity) override;
    void RemoveEntities(BASE_NS::array_view<const CORE_NS::EntityReference> entities) override;
    void SetId(BASE_NS::string_view id, CORE_NS::EntityReference entity) override;
    BASE_NS::string_view GetId(CORE_NS::Entity entity) const override;

    size_t GetSubCollectionCount() const override;
    IEntityCollection* GetSubCollection(size_t index) override;
    const IEntityCollection* GetSubCollection(size_t index) const override;
    int32_t GetSubCollectionIndex(BASE_NS::string_view uri) const override;
    int32_t GetSubCollectionIndexByRoot(CORE_NS::Entity entity) const override;
    IEntityCollection& AddSubCollection(BASE_NS::string_view uri, BASE_NS::string_view contextUri) override;
    IEntityCollection& AddSubCollectionClone(IEntityCollection& collection, BASE_NS::string_view uri) override;
    void RemoveSubCollection(size_t index) override;

    size_t GetEntityCountRecursive(bool includeDestroyed) const override;
    void GetEntitiesRecursive(
        bool includeDestroyed, BASE_NS::vector<CORE_NS::EntityReference>& entitiesOut) const override;

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

protected:
    void Destroy() override;
    ~EntityCollection() override;

private:
    // Prevent copying.
    EntityCollection(const EntityCollection&) = delete;
    EntityCollection& operator=(const EntityCollection&) = delete;

    // From IEntityCollection::IListener
    void ModifiedChanged(IEntityCollection& entityCollection, bool modified) override;

    void DoGetEntitiesRecursive(bool includeDestroyed, BASE_NS::vector<CORE_NS::EntityReference>& entitiesOut) const;

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
    BASE_NS::vector<CORE_NS::EntityReference> entities_;

    BASE_NS::vector<EntityCollection::Ptr> collections_;

    bool isActive_ { true };
    bool isMarkedDestroyed_ { false };
    bool isMarkedModified_ { false };

    BASE_NS::vector<IEntityCollection::IListener*> listeners_;
};

EntityCollection::EntityCollection(IEcs& ecs, string_view uri, string_view contextUri)
    : ecs_(ecs), uri_(uri), contextUri_(contextUri)
{}

void EntityCollection::AddListener(IEntityCollection::IListener& listener)
{
    BASE_ASSERT(&listener);
    listeners_.emplace_back(&listener);
}

void EntityCollection::RemoveListener(IEntityCollection::IListener& listener)
{
    BASE_ASSERT(&listener);
    for (size_t i = 0; i < listeners_.size(); ++i) {
        if (&listener == listeners_[i]) {
            listeners_.erase(listeners_.begin() + static_cast<int64_t>(i));
            return;
        }
    }

    // trying to remove a non-existent listener.
    BASE_ASSERT(true);
}

IEcs& EntityCollection::GetEcs() const
{
    return ecs_;
}

string EntityCollection::GetUri() const
{
    return uri_;
}

string EntityCollection::GetContextUri() const
{
    return contextUri_;
}

string EntityCollection::GetSrc() const
{
    return src_;
}

void EntityCollection::SetSrc(string_view src)
{
    src_ = src;
    MarkModified(true);
}

string EntityCollection::GetType() const
{
    return type_;
}

void EntityCollection::SetType(string_view type)
{
    type_ = type;
    MarkModified(true);
}

size_t EntityCollection::GetEntityCount() const
{
    return entities_.size();
}

EntityReference EntityCollection::GetEntity(size_t collectionIndex) const
{
    BASE_ASSERT(collectionIndex < entities_.size());
    if (collectionIndex >= entities_.size()) {
        return EntityReference {};
    }
    return entities_[collectionIndex];
}

EntityReference EntityCollection::GetEntity(string_view localContextId) const
{
    const auto it = namedEntities_.find(localContextId);
    if (it != namedEntities_.end()) {
        return it->second;
    }
    return EntityReference {};
}

array_view<const EntityReference> EntityCollection::GetEntities() const
{
    return entities_;
}

void EntityCollection::AddEntity(EntityReference entity)
{
    AddEntities({ &entity, 1 });
}

void EntityCollection::AddEntities(array_view<const EntityReference> entities)
{
    entities_.reserve(entities_.size() + entities.size());
    for (auto entity : entities) {
        if (entity != Entity {}) {
            // TODO: make sure that the same entity is not added twice.
            entities_.emplace_back(entity);
        } else {
            CORE_LOG_W("Trying to add empty entity to a collection '%s'", uri_.c_str());
        }
    }
    MarkModified(true);
}

bool EntityCollection::RemoveEntity(EntityReference entity)
{
    for (size_t i = 0; i < entities_.size(); ++i) {
        if (entities_[i] == entity) {
            entities_.erase(entities_.begin() + static_cast<int64_t>(i));

            // Also remove any related id mappings.
            for (auto it = namedEntities_.begin(); it != namedEntities_.end(); ++it) {
                if (it->second == entity) {
                    namedEntities_.erase(it);
                    break;
                }
            }

            // TODO:  If this collection is overriding another "template" collection, when removing entities, we
            // need to remember that and the information about deletion needs to be serialized. Maybe check if
            // (src_.empty()). However this also needs to work with undo

            MarkModified(true);
            return true;
        }
    }

    // Not found. Check the sub-collections.
    for (auto& collection : collections_) {
        BASE_ASSERT(collection);
        if (collection->RemoveEntity(entity)) {
            MarkModified(true);
            return true;
        }
    }

    return false;
}

void EntityCollection::RemoveEntities(array_view<const EntityReference> entities)
{
    for (auto entity : entities) {
        RemoveEntity(entity);
    }
}

void EntityCollection::SetId(string_view id, EntityReference entity)
{
#if 0
    CORE_LOG_V("EntityCollection '%s': Set entity id: %" PRIx64 " = '%s'", src_.c_str(), entity.operator Entity().id, string(id).c_str());
#endif
    namedEntities_[id] = entity;
}
string_view EntityCollection::GetId(Entity entity) const
{
    for (auto& it : namedEntities_) {
        if (it.second == entity) {
            return it.first;
        }
    }
    return {};
}

size_t EntityCollection::GetSubCollectionCount() const
{
    return collections_.size();
}

IEntityCollection* EntityCollection::GetSubCollection(size_t index)
{
    if (index >= collections_.size()) {
        return nullptr;
    }
    return collections_.at(index).get();
}

const IEntityCollection* EntityCollection::GetSubCollection(size_t index) const
{
    if (index >= collections_.size()) {
        return nullptr;
    }
    return collections_.at(index).get();
}

int32_t EntityCollection::GetSubCollectionIndex(string_view uri) const
{
    for (size_t i = 0; i < collections_.size(); ++i) {
        BASE_ASSERT(collections_[i]);
        if (collections_[i]->GetUri() == uri) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

int32_t EntityCollection::GetSubCollectionIndexByRoot(Entity entity) const
{
    if (entity != Entity {}) {
        for (size_t i = 0; i < collections_.size(); ++i) {
            BASE_ASSERT(collections_[i]);
            if (collections_[i]->GetEntity("/") == entity) {
                return static_cast<int32_t>(i);
            }
        }
    }
    return -1;
}

IEntityCollection& EntityCollection::AddSubCollection(string_view uri, string_view contextUri)
{
    collections_.emplace_back(EntityCollection::Ptr { new EntityCollection(ecs_, uri, contextUri) });

    // listen to changes in subcollection
    collections_.back()->AddListener(*this);

    MarkModified(true);
    return *collections_.back();
}

IEntityCollection& EntityCollection::AddSubCollectionClone(IEntityCollection& collection, string_view uri)
{
    // TODO: use just the public api
    collections_.emplace_back(EntityCollection::Ptr { new EntityCollection(ecs_, uri, collection.GetContextUri()) });
    auto& ec = *collections_.back();
    static_cast<EntityCollection&>(collection).ClonePrivate(ec);

    // listen to changes in subcollection
    ec.AddListener(*this);

    MarkModified(true);
    return ec;
}

void EntityCollection::RemoveSubCollection(size_t index)
{
    BASE_ASSERT(index < collections_.size());
    if (index < collections_.size()) {
        // stop listening to changes in subcollection
        auto& ec = collections_.at(index);
        ec->RemoveListener(*this);

        collections_.erase(collections_.begin() + static_cast<int64_t>(index));
        MarkModified(true);
    }
}

size_t EntityCollection::GetEntityCountRecursive(bool includeDestroyed) const
{
    if (!includeDestroyed && IsMarkedDestroyed()) {
        return 0;
    }

    auto size = entities_.size();
    for (const auto& collection : collections_) {
        BASE_ASSERT(collection);
        size += collection->GetEntityCountRecursive(includeDestroyed);
    }
    return size;
}

void EntityCollection::GetEntitiesRecursive(bool includeDestroyed, vector<EntityReference>& entitiesOut) const
{
    // NOTE: Cloning depends on ordering of entitiesOut.
    entitiesOut.reserve(entitiesOut.size() + GetEntityCountRecursive(includeDestroyed));
    DoGetEntitiesRecursive(includeDestroyed, entitiesOut);
}

void EntityCollection::DoGetEntitiesRecursive(bool includeDestroyed, vector<EntityReference>& entitiesOut) const
{
    if (!includeDestroyed && IsMarkedDestroyed()) {
        return;
    }

    entitiesOut.insert(entitiesOut.end(), entities_.begin(), entities_.end());
    for (const auto& collection : collections_) {
        BASE_ASSERT(collection);
        collection->DoGetEntitiesRecursive(includeDestroyed, entitiesOut);
    }
}

bool EntityCollection::Contains(Entity entity) const
{
    for (auto it : entities_) {
        if (it == entity) {
            return true;
        }
    }
    for (const auto& collection : collections_) {
        if (!collection->IsMarkedDestroyed()) {
            if (collection->Contains(entity)) {
                return true;
            }
        }
    }
    return false;
}

bool EntityCollection::IsExternal(Entity entity) const
{
    for (auto it : entities_) {
        if (it == entity) {
            return false;
        }
    }
    return true;
}

bool EntityCollection::isSubCollectionRoot(Entity entity) const
{
    if (entity == Entity {}) {
        return false;
    }

    for (const auto& collection : collections_) {
        if (collection->GetEntity("/") == entity) {
            return true;
        }
    }

    return false;
}

CORE_NS::EntityReference EntityCollection::GetReference(CORE_NS::Entity entity) const
{
    if (Contains(entity)) {
        auto ref = GetEcs().GetEntityManager().GetReferenceCounted(entity);

        // Check that this entity was reference counted already (it should be part of the collection).
        CORE_ASSERT(ref.GetRefCount() > 1);

        return ref;
    }
    return {};
}

void EntityCollection::SetActive(bool active)
{
    isActive_ = active;
    const bool effectivelyActive = isActive_ && !isMarkedDestroyed_;

    auto& em = ecs_.GetEntityManager();
    for (auto& entity : entities_) {
        em.SetActive(entity, effectivelyActive);
    }

    for (auto& collection : collections_) {
        BASE_ASSERT(collection);
        collection->SetActive(active);
    }
}

bool EntityCollection::IsActive() const
{
    return isActive_;
}

void EntityCollection::MarkDestroyed(bool destroyed)
{
    MarkModified(true);
    isMarkedDestroyed_ = destroyed;
    const bool effectivelyActive = isActive_ && !isMarkedDestroyed_;

    // Change the active state of entities without changing the active state of the collection itself.
    auto& em = ecs_.GetEntityManager();
    for (auto& entity : entities_) {
        em.SetActive(entity, effectivelyActive);
    }

    for (auto& collection : collections_) {
        BASE_ASSERT(collection);
        collection->MarkDestroyed(destroyed);
    }
}

bool EntityCollection::IsMarkedDestroyed() const
{
    return isMarkedDestroyed_;
}

void EntityCollection::MarkModified(bool modified)
{
    if (isMarkedModified_ != modified) {
        if (uri_ == "project://assets/prefabs/boxes.prefab") {
            CORE_LOG_D("Modified uri=%s, src=%s, active=%i, address=%p, modified=%i", uri_.c_str(), src_.c_str(),
                isActive_, (void*)this, modified);
        }

        isMarkedModified_ = modified;
        for (auto* l : listeners_) {
            l->ModifiedChanged(*this, modified);
        }
    }
}

void EntityCollection::MarkModified(bool modified, bool recursive)
{
    if (recursive && !collections_.empty()) {
        for (auto& c : collections_) {
            c->MarkModified(modified, true);
        }
    }
    MarkModified(modified);
}

bool EntityCollection::IsMarkedModified() const
{
    return isMarkedModified_;
}

void EntityCollection::Clear()
{
    serializationInfo_.clear();
    namedEntities_.clear();
    entities_.clear();
    collections_.clear();

    MarkModified(true);
}

void EntityCollection::CopyContents(IEntityCollection& srcCollection)
{
    // TODO: use just the public api
    static_cast<EntityCollection&>(srcCollection).ClonePrivate(*this);
    MarkModified(true);
}

namespace {
void CopyFrom(IEntityCollection& srcCollection, array_view<const EntityReference> srcEntities,
    IEntityCollection& dstCollection, unordered_map<Entity, Entity>& oldToNew, vector<EntityReference>& clonedOut,
    bool copyCollections)
{
    BASE_NS::unordered_map<IEntityCollection*, bool> copiedCollections;

    for (auto& srcEntity : srcEntities) {
        auto index = srcCollection.GetSubCollectionIndexByRoot(srcEntity);
        if (index >= 0) {
            auto* col = srcCollection.GetSubCollection(static_cast<size_t>(index));
            auto& copy = dstCollection.AddSubCollection(col->GetUri(), col->GetContextUri());
            copy.SetSrc(col->GetSrc());
            vector<EntityReference> allEntities;
            col->GetEntitiesRecursive(false, allEntities);
            CopyFrom(*col, allEntities, copy, oldToNew, clonedOut, true);
            copiedCollections[col] = true;
        } else if (!srcCollection.IsExternal(srcEntity)) {
            auto dstEntity = CloneEntityReference(srcCollection.GetEcs(), srcEntity, dstCollection.GetEcs());
            clonedOut.emplace_back(dstEntity);
            oldToNew[srcEntity] = dstEntity;
            dstCollection.AddEntity(dstEntity);
            auto id = srcCollection.GetId(srcEntity);
            if (!id.empty()) {
                dstCollection.SetId(id, dstEntity);
            }
        }
    }

    if (copyCollections) {
        auto subCollectionCount = srcCollection.GetSubCollectionCount();
        for (size_t subIndex = 0; subIndex < subCollectionCount; subIndex++) {
            auto* subCollection = srcCollection.GetSubCollection(subIndex);
            if (!copiedCollections.contains(subCollection)) {
                auto& copy = dstCollection.AddSubCollection(subCollection->GetUri(), subCollection->GetContextUri());
                copy.SetSrc(subCollection->GetSrc());
                vector<EntityReference> allEntities;
                subCollection->GetEntitiesRecursive(false, allEntities);
                CopyFrom(*subCollection, allEntities, copy, oldToNew, clonedOut, false);
            }
        }
    }
}

CORE_NS::Entity GetOldEntity(unordered_map<Entity, Entity>& oldToNew, CORE_NS::Entity newEntity)
{
    for (auto& e : oldToNew) {
        if (e.second == newEntity) {
            return e.first;
        }
    }
    return {};
}

void CopySerializationInfo(IEntityCollection& srcCollection, IEntityCollection& dstCollection,
    unordered_map<Entity, Entity>& oldToNew, vector<EntityReference>& entitiesOut)
{
    // Copy all serialization info for copied entites from their original entities
    // Note that the root collections contain all the serialization info.
    for (auto& cm : srcCollection.GetEcs().GetComponentManagers()) {
        for (auto& entityOut : entitiesOut) {
            auto srcEntity = GetOldEntity(oldToNew, entityOut);
            if (CORE_NS::EntityUtil::IsValid(srcEntity)) {
                auto* properties = srcCollection.GetSerializedProperties(srcEntity, cm->GetUid());
                if (properties) {
                    dstCollection.MarkComponentSerialized(entityOut, cm->GetUid(), true);
                    for (auto property : *properties) {
                        dstCollection.MarkPropertySerialized(entityOut, cm->GetUid(), property, true);
                    }
                }
            }
        }
    }
}
} // namespace

vector<EntityReference> EntityCollection::CopyContentsWithSerialization(
    IEntityCollection& srcCollection, array_view<const EntityReference> entities)
{
    unordered_map<Entity, Entity> oldToNew;

    vector<EntityReference> entitiesOut;
    entitiesOut.reserve(entities.size());
    CopyFrom(srcCollection, entities, *this, oldToNew, entitiesOut, false);
    CopySerializationInfo(srcCollection, *this, oldToNew, entitiesOut);

    for (auto& entity : entitiesOut) {
        RewriteEntityReferences(GetEcs(), entity, oldToNew);
    }
    return entitiesOut;
}

vector<EntityReference> EntityCollection::CopyContentsWithSerialization(IEntityCollection& srcCollection)
{
    vector<EntityReference> allEntities;
    srcCollection.GetEntitiesRecursive(false, allEntities);
    return CopyContentsWithSerialization(srcCollection, allEntities);
}

void EntityCollection::ClonePrivate(EntityCollection& dst) const
{
    // Clone all collections recursively.
    DoCloneRecursive(dst);

    //
    // Remap entity properties that are pointing to the src entities to point to cloned ones.
    //
    unordered_map<Entity, Entity> oldToNew;

    vector<EntityReference> sourceEntities;
    GetEntitiesRecursive(false, sourceEntities);
    vector<EntityReference> clonedEntities;
    dst.GetEntitiesRecursive(false, clonedEntities);

    // NOTE: Assuming the order in GetEntitiesRecursive is consistent.
    BASE_ASSERT(sourceEntities.size() == clonedEntities.size());
    const auto entityCount = sourceEntities.size();
    for (size_t i = 0; i < entityCount; ++i) {
        oldToNew[sourceEntities[i]] = clonedEntities[i];
    }
    for (auto& entity : clonedEntities) {
        RewriteEntityReferences(dst.GetEcs(), entity, oldToNew);
    }
}

void EntityCollection::DoCloneRecursive(EntityCollection& dst) const
{
    // Clone entities.
    dst.entities_ = CloneEntityReferences(ecs_, { entities_.data(), entities_.size() }, dst.GetEcs());

    // Create id mapping but reference cloned entities instead of the original
    BASE_ASSERT(entities_.size() == dst.entities_.size());
    dst.namedEntities_.clear();
    dst.namedEntities_.reserve(namedEntities_.size());
    const auto entityCount = entities_.size();
    for (const auto& it : namedEntities_) {
        for (size_t i = 0; i < entityCount; ++i) {
            if (it.second == entities_[i]) {
                dst.SetId(it.first, dst.entities_[i]);
                break;
            }
        }
    }

    // Recurse.
    dst.collections_.reserve(collections_.size());
    for (auto& collection : collections_) {
        BASE_ASSERT(collection);

        if (!collection->IsMarkedDestroyed()) {
            dst.collections_.emplace_back(EntityCollection::Ptr {
                new EntityCollection(dst.GetEcs(), collection->GetUri(), collection->GetContextUri()) });
            auto& clonedChild = *dst.collections_.back();
            collection->DoCloneRecursive(clonedChild);
        }
    }
}

namespace {

bool SetPropertyDefined(EntityCollection::PropertyList& pl, string_view propertyPath)
{
    for (const auto& prop : pl) {
        if (prop == propertyPath) {
            // Already marked as a property defined by this node.
            return false;
        }

        // check if the property we are trying to set is a sub-property of an already defined property
        auto len1 = prop.length();
        auto len2 = propertyPath.length();
        if (len2 > len1) {
            auto view1 = prop.substr(0, len1);
            auto view2 = propertyPath.substr(0, len1);
            if (view1 == view2) {
                // already defined in a higher level, so no need to define this sub-property
                return false;
            }
        }
    }
    pl.push_back(string(propertyPath));
    return true;
}
bool SetPropertyUndefined(EntityCollection::PropertyList& pl, string_view propertyPath)
{
    for (size_t i = 0; i < pl.size(); ++i) {
        if (pl[i] == propertyPath) {
            pl.erase(pl.begin() + static_cast<int64_t>(i));
            return true;
        }
    }
    return false;
}

} // namespace

bool EntityCollection::MarkComponentSerialized(Entity entity, Uid component, bool serialize)
{
    bool changed = false;
    const auto entityInfo = serializationInfo_.find(entity);
    const bool entityFound = (entityInfo != serializationInfo_.end());
    if (serialize) {
        if (!entityFound) {
            serializationInfo_[entity][component] = {};
            changed = true;
        } else {
            const auto componentInfo = entityInfo->second.find(component);
            const bool componentFound = (componentInfo != entityInfo->second.end());
            if (!componentFound) {
                entityInfo->second[component] = {};
                changed = true;
            }
        }
    } else {
        if (entityFound) {
            entityInfo->second.erase(component);
            changed = true;
        }
    }

    if (changed) {
        MarkModified(true);
    }

    return changed;
}

bool EntityCollection::MarkAllPropertiesSerialized(Entity entity, Uid component)
{
    bool changed = false;

    auto cm = GetEcs().GetComponentManager(component);
    if (!cm) {
        CORE_LOG_W("Set modified: Unrecognized component");
        return false;
    }

    auto info = serializationInfo_.find(entity);
    if (info == serializationInfo_.end()) {
        serializationInfo_[entity] = {};
        info = serializationInfo_.find(entity);
        changed = true;
    }

    const auto& propertyApi = cm->GetPropertyApi();
    const auto propertyCount = propertyApi.PropertyCount();
    for (size_t i = 0; i < propertyCount; ++i) {
        auto* property = propertyApi.MetaData(i);
        changed = changed | SetPropertyDefined(info->second[component], property->name);
    }

    if (changed) {
        MarkModified(true);
    }

    return changed;
}

bool EntityCollection::MarkPropertySerialized(Entity entity, Uid component, string_view propertyPath, bool serialize)
{
    bool changed = false;

    auto* cm = GetEcs().GetComponentManager(component);
    if (cm) {
        auto info = serializationInfo_.find(entity);
        if (serialize) {
            if (info == serializationInfo_.end()) {
                serializationInfo_[entity][component] = {};
                info = serializationInfo_.find(entity);
                changed = true;
            }
            changed = changed | SetPropertyDefined(info->second[component], propertyPath);
        } else {
            if (info != serializationInfo_.end()) {
                changed = changed | SetPropertyUndefined(info->second[component], propertyPath);
            }
        }
    }

    if (changed) {
        MarkModified(true);
    }

    return changed;
}

const IEntityCollection::PropertyList* EntityCollection::GetSerializedProperties(Entity entity, Uid component) const
{
    const auto info = serializationInfo_.find(entity);
    if (info != serializationInfo_.end()) {
        const auto props = info->second.find(component);
        if (props != info->second.end()) {
            return &props->second;
        }
    }
    return nullptr;
}

bool EntityCollection::IsPropertySerialized(Entity entity, Uid component, string_view propertyPath)
{
    auto info = serializationInfo_.find(entity);
    if (info == serializationInfo_.end()) {
        return false;
    } else {
        const auto props = info->second.find(component);
        if (props == info->second.end()) {
            return false;
        } else {
            for (auto& prop : props->second) {
                if (prop == propertyPath) {
                    return true;
                }
            }
        }
    }
    return false;
}

void EntityCollection::Destroy()
{
    delete this;
}

EntityCollection::~EntityCollection()
{
    Clear();
}

void EntityCollection::ModifiedChanged(IEntityCollection& entityCollection, bool modified)
{
    CORE_UNUSED(entityCollection);

    // subcollection changed, propagate modified status
    if (modified) {
        MarkModified(modified);
    }
}

IEntityCollection::Ptr CreateEntityCollection(IEcs& ecs, string_view uri, string_view contextUri)
{
    return EntityCollection::Ptr { new EntityCollection(ecs, uri, contextUri) };
}

ECS_SERIALIZER_END_NAMESPACE()
