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

#include "entity_collection.h"

#include <cinttypes>
#include <PropertyTools/property_data.h>

#include <base/containers/fixed_string.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/log.h>
#include <core/property/intf_property_api.h>

#include "ecs_util.h"

using namespace BASE_NS;
using namespace CORE_NS;

SCENE_BEGIN_NAMESPACE()

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
            listeners_.erase(listeners_.begin() + i);
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

void EntityCollection::SetUri(const BASE_NS::string& uri)
{
    uri_ = uri;
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
    const auto it = entityIdentifiers_.find(localContextId);
    if (it != entityIdentifiers_.end()) {
        return it->second;
    }
    const auto itt = namedEntities_.find(localContextId);
    if (itt != namedEntities_.end()) {
        return itt->second;
    }
    return EntityReference {};
}

EntityReference EntityCollection::GetEntityRecursive(string_view localContextId) const
{
    auto ret = GetEntity(localContextId);

    if (!CORE_NS::EntityUtil::IsValid(ret)) {
        for (auto& it : collections_) {
            ret = it->GetEntityRecursive(localContextId);
            if (CORE_NS::EntityUtil::IsValid(ret)) {
                break;
            }
        }
    }

    return ret;
}

void EntityCollection::RemoveEntityRecursive(CORE_NS::Entity entity)
{
    for (auto entityRef = entities_.cbegin(); entityRef != entities_.cend();) {
        if (entity == *entityRef) {
            entityRef = entities_.erase(entityRef);
        } else {
            entityRef++;
        }
    }

    for (auto entityRef = namedEntities_.cbegin(); entityRef != namedEntities_.cend();) {
        if (entityRef->second == entity) {
            entityRef = namedEntities_.erase(entityRef);
        } else {
            ++entityRef;
        }
    }

    auto uniqueIdentifier = GetUniqueIdentifier(entity);
    if (!uniqueIdentifier.empty()) {
        entityIdentifiers_.erase(uniqueIdentifier);
    }

    for (auto& it : collections_) {
        it->RemoveEntityRecursive(entity);
    }
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
    for (const auto& entity : entities) {
        if (entity != Entity {}) {
            // TODO: make sure that the same entity is not added twice.
            entities_.emplace_back(entity);
        } else {
            CORE_LOG_D("Trying to add null entity reference to a collection");
        }
    }
    MarkModified(true);
}

bool EntityCollection::RemoveEntity(EntityReference entity)
{
    for (size_t i = 0; i < entities_.size(); ++i) {
        if (entities_[i] == entity) {
            entities_.erase(entities_.begin() + i);

            // Also remove any related id mappings.
            for (auto it = namedEntities_.begin(); it != namedEntities_.end(); ++it) {
                if (it->second == entity) {
                    namedEntities_.erase(it);
                    break;
                }
            }

            auto uniqueIdentifier = GetUniqueIdentifier(entity);
            if (!uniqueIdentifier.empty()) {
                entityIdentifiers_.erase(uniqueIdentifier);
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
    for (auto& entity : entities) {
        RemoveEntity(entity);
    }
}

void EntityCollection::SetId(string_view id, EntityReference entity)
{
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

string_view EntityCollection::GetIdRecursive(Entity entity) const
{
    auto ret = GetId(entity);
    if (ret.empty()) {
        for (auto& it : collections_) {
            ret = it->GetIdRecursive(entity);
            if (!ret.empty()) {
                break;
            }
        }
    }
    return ret;
}

void EntityCollection::SetUniqueIdentifier(string_view id, EntityReference entity)
{
    entityIdentifiers_[id] = entity;
}

string_view EntityCollection::GetUniqueIdentifier(Entity entity) const
{
    for (auto& it : entityIdentifiers_) {
        if (it.second == entity) {
            return it.first;
        }
    }
    return {};
}

string_view EntityCollection::GetUniqueIdentifierRecursive(Entity entity) const
{
    auto ret = GetUniqueIdentifier(entity);
    if (ret.empty()) {
        for (auto& it : collections_) {
            ret = it->GetUniqueIdentifierRecursive(entity);
            if (!ret.empty()) {
                break;
            }
        }
    }
    return ret;
}

size_t EntityCollection::GetSubCollectionCount() const
{
    return collections_.size();
}

IEntityCollection* EntityCollection::GetSubCollection(size_t index)
{
    if (index < 0 || index >= collections_.size()) {
        return nullptr;
    }
    return collections_.at(index).get();
}

const IEntityCollection* EntityCollection::GetSubCollection(size_t index) const
{
    if (index < 0 || index >= collections_.size()) {
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

IEntityCollection& EntityCollection::AddSubCollection(string_view uri, string_view contextUri, bool serializable)
{
    auto collection = EntityCollection::Ptr { new EntityCollection(ecs_, uri, contextUri) };
    collection->SetSerialized(serializable);
    collections_.emplace_back(move(collection));

    // listen to changes in subcollection
    collections_.back()->AddListener(*this);

    if (serializable) {
        MarkModified(true);
    }
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

        collections_.erase(collections_.begin() + index);
        MarkModified(true);
    }
}

size_t EntityCollection::GetEntityCountRecursive(bool includeDestroyed, bool includeNonSerialized) const
{
    if (!includeDestroyed && IsMarkedDestroyed()) {
        return 0;
    }

    if (!includeNonSerialized && !IsSerialized()) {
        return 0;
    }

    auto size = entities_.size();
    for (const auto& collection : collections_) {
        BASE_ASSERT(collection);
        size += collection->GetEntityCountRecursive(includeDestroyed, includeNonSerialized);
    }
    return size;
}

void EntityCollection::GetEntitiesRecursive(
    bool includeDestroyed, vector<EntityReference>& entitiesOut, bool includeNonSerialized) const
{
    // NOTE: Cloning depends on ordering of entitiesOut.
    entitiesOut.reserve(entitiesOut.size() + GetEntityCountRecursive(includeDestroyed, includeNonSerialized));
    DoGetEntitiesRecursive(includeDestroyed, includeNonSerialized, entitiesOut);
}

void EntityCollection::DoGetEntitiesRecursive(
    bool includeDestroyed, bool includeNonSerialized, vector<EntityReference>& entitiesOut) const
{
    if (!includeDestroyed && IsMarkedDestroyed()) {
        return;
    }

    if (!includeNonSerialized && !IsSerialized()) {
        return;
    }

    entitiesOut.insert(entitiesOut.end(), entities_.begin(), entities_.end());
    for (const auto& collection : collections_) {
        BASE_ASSERT(collection);
        collection->DoGetEntitiesRecursive(includeDestroyed, includeNonSerialized, entitiesOut);
    }
}

bool EntityCollection::Contains(Entity entity) const
{
    for (auto& it : entities_) {
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
    for (auto& it : entities_) {
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
    if (!IsSerialized()) {
        return;
    }

    if (isMarkedModified_ != modified) {
        isMarkedModified_ = modified;
        for (auto* l : listeners_) {
            l->ModifiedChanged(*this, modified);
        }
    }
}

void EntityCollection::MarkModified(bool modified, bool recursive)
{
    if (!IsSerialized()) {
        return;
    }

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
    entityIdentifiers_.clear();
    entities_.clear();
    collections_.clear();
    listeners_.clear();

    MarkModified(true);
}

void EntityCollection::CopyContents(IEntityCollection& srcCollection)
{
    // TODO: use just the public api
    static_cast<EntityCollection&>(srcCollection).ClonePrivate(*this);
    MarkModified(true);
}

void EntityCollection::AddEntityToSubcollection(
    BASE_NS::string_view collection, BASE_NS::string_view name, CORE_NS::Entity entity, bool makeUnique)
{
    auto collectionIx = GetSubCollectionIndex(collection);
    if (collectionIx == -1) {
        AddSubCollection(collection, {}, !makeUnique);
        collectionIx = GetSubCollectionCount() - 1;
    }

    if (auto targetCollection = GetSubCollection(collectionIx)) {
        BASE_NS::string postFixed(name.data(), name.size());
        if (makeUnique) {
            postFixed.append(":");
            postFixed.append(BASE_NS::to_hex(entity.id));
        }
        if (!CORE_NS::EntityUtil::IsValid(targetCollection->GetEntity(postFixed))) {
            auto ref = ecs_.GetEntityManager().GetReferenceCounted(entity);
            targetCollection->AddEntity(ref);
            targetCollection->SetUniqueIdentifier(postFixed, ref);
        }
    }
}

bool EntityCollection::IsSerialized() const
{
    return isSerialized_;
}

void EntityCollection::SetSerialized(bool serialize)
{
    isSerialized_ = serialize;
}

// TODO: clean up copying.
namespace {
void CloneEntitiesFromCollection(IEntityCollection& srcCollection, IEntityCollection& dstCollection,
    IEntityCollection& srcSerializationCollection, IEntityCollection& dstSerializationCollection,
    array_view<const EntityReference> entities, unordered_map<Entity, Entity>& oldToNew,
    vector<EntityReference>& clonedOut)
{
    const size_t entityCount = entities.size();
    for (size_t i = 0; i < entityCount; ++i) {
        auto srcEntity = entities[i];

        // Copy the serialization and subcollection info.
        auto subCollectionIndex = srcCollection.GetSubCollectionIndexByRoot(srcEntity);
        if (subCollectionIndex >= 0) {
            auto* srcSubCollection = srcCollection.GetSubCollection(subCollectionIndex);
            auto& dstSubCollection =
                dstCollection.AddSubCollection(srcSubCollection->GetUri(), srcSubCollection->GetContextUri());
            dstSubCollection.SetSrc(srcSubCollection->GetSrc());

            // Recursively copy sub collection.
            CloneEntitiesFromCollection(*srcSubCollection, dstSubCollection, srcSerializationCollection,
                dstSerializationCollection, srcSubCollection->GetEntities(), oldToNew, clonedOut);

            // Need to find the collection roots manually.
            // TODO: a bit messy that the colelctions are iterated separately.
            // TODO: no easy way to get the root entity.
            const size_t collectionCount = srcSubCollection->GetSubCollectionCount();
            for (size_t j = 0; j < collectionCount; ++j) {
                auto* sc = srcSubCollection->GetSubCollection(j);
                auto root = sc->GetEntity("/");
                if (root != Entity {}) {
                    CloneEntitiesFromCollection(*srcSubCollection, dstSubCollection, srcSerializationCollection,
                        dstSerializationCollection, { &root, 1 }, oldToNew, clonedOut);
                }
            }

        } else if (!srcCollection.IsExternal(srcEntity)) {
            auto dstEntity = CloneEntityReference(srcCollection.GetEcs(), srcEntity, dstCollection.GetEcs());

            clonedOut.emplace_back(dstEntity);
            oldToNew[srcEntity] = dstEntity;

            dstCollection.AddEntity(dstEntity);
            auto id = srcCollection.GetId(srcEntity);
            if (!id.empty()) {
                dstCollection.SetId(id, dstEntity);
            }

            // Note that the root collections contain all the serialization info.
            for (auto& cm : srcCollection.GetEcs().GetComponentManagers()) {
                auto* properties = srcSerializationCollection.GetSerializedProperties(srcEntity, cm->GetUid());
                if (properties) {
                    dstSerializationCollection.MarkComponentSerialized(dstEntity, cm->GetUid(), true);
                    for (auto& property : *properties) {
                        dstSerializationCollection.MarkPropertySerialized(dstEntity, cm->GetUid(), property, true);
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
    CloneEntitiesFromCollection(srcCollection, *this, srcCollection, *this, entities, oldToNew, entitiesOut);

    for (auto& entity : entitiesOut) {
        RewriteEntityReferences(GetEcs(), entity, oldToNew);
    }
    return entitiesOut;
}

vector<EntityReference> EntityCollection::CopyContentsWithSerialization(IEntityCollection& srcCollection)
{
    vector<EntityReference> entitiesIn;
    srcCollection.GetEntitiesRecursive(false, entitiesIn);
    return CopyContentsWithSerialization(srcCollection, entitiesIn);
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
            pl.erase(pl.begin() + i);
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
    // subcollection changed, propagate modified status
    if (modified) {
        MarkModified(modified);
    }
}

IEntityCollection::Ptr EntityCollection::CreateNewEntityCollection(string_view uri, string_view contextUri)
{
    return EntityCollection::Ptr { new EntityCollection(ecs_, uri, contextUri) };
}

SCENE_END_NAMESPACE()
