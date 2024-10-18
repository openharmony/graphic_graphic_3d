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

#ifndef API_ECS_SERIALIZER_ECSUTIL_H
#define API_ECS_SERIALIZER_ECSUTIL_H

#include <PropertyTools/property_data.h>
#include <base/containers/unordered_map.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/property/intf_property_api.h>
#include <core/property/property_types.h>
#include <ecs_serializer/namespace.h>

ECS_SERIALIZER_BEGIN_NAMESPACE()

void CloneComponent(Entity srcEntity, const IComponentManager& srcManager,
    IEcs& dstEcs, Entity dstEntity)
{
    auto* dstManager = dstEcs.GetComponentManager(srcManager.GetUid());
    if (dstManager) {
        // Get copy destiantion property handle.
        auto componentId = dstManager->GetComponentId(dstEntity);
        if (componentId == IComponentManager::INVALID_COMPONENT_ID) {
            dstManager->Create(dstEntity);
            componentId = dstManager->GetComponentId(dstEntity);
        }
        BASE_ASSERT(componentId != IComponentManager::INVALID_COMPONENT_ID);
        const auto* srcHandle = srcManager.GetData(srcEntity);
        if (srcHandle) {
            dstManager->SetData(dstEntity, *srcHandle);
        }
    }
}

inline void CloneComponents(
    IEcs& srcEcs, Entity srcEntity, IEcs& dstEcs, Entity dstEntity)
{
    vector<IComponentManager*> managers;
    srcEcs.GetComponents(srcEntity, managers);
    for (auto* srcManager : managers) {
        CloneComponent(srcEntity, *srcManager, dstEcs, dstEntity);
    }
}

inline Entity CloneEntity(IEcs& srcEcs, Entity src, IEcs& dstEcs)
{
    Entity dst = dstEcs.GetEntityManager().Create();
    CloneComponents(srcEcs, src, dstEcs, dst);
    return dst;
}

inline EntityReference CloneEntityReference(IEcs& srcEcs, Entity src, IEcs& dstEcs)
{
    EntityReference dst = dstEcs.GetEntityManager().CreateReferenceCounted();
    CloneComponents(srcEcs, src, dstEcs, dst);
    return dst;
}

void GatherEntityReferences(vector<Entity*>& entities,
    vector<EntityReference*>& entityReferences, const Property& property,
    uintptr_t offset = 0)
{
    if (property.type == PropertyType::ENTITY_T) {
        entities.emplace_back(reinterpret_cast<Entity*>(offset));
    } else if (property.type == PropertyType::ENTITY_REFERENCE_T) {
        entityReferences.emplace_back(reinterpret_cast<EntityReference*>(offset));
    } else if (property.metaData.containerMethods) {
        auto& containerProperty = property.metaData.containerMethods->property;
        if (property.type.isArray) {
            // Array of properties.
            for (size_t i = 0; i < property.count; i++) {
                uintptr_t ptr = offset + i * containerProperty.size;
                GatherEntityReferences(entities, entityReferences, containerProperty, ptr);
            }
        } else {
            // This is a "non trivial container"
            // (So it needs to read the data and not just the metadata to figure out the data structure).
            const auto count = property.metaData.containerMethods->size(offset);
            for (size_t i = 0; i < count; i++) {
                uintptr_t ptr = property.metaData.containerMethods->get(offset, i);
                GatherEntityReferences(entities, entityReferences, containerProperty, ptr);
            }
        }
    } else if (!property.metaData.memberProperties.empty()) {
        // Custom type (struct). Process sub properties recursively.
        for (size_t i = 0; i < property.count; i++) {
            for (const auto& child : property.metaData.memberProperties) {
                GatherEntityReferences(entities, entityReferences, child, offset + child.offset);
            }
            offset += property.size / property.count;
        }
    }
}

void RewriteEntityReferences(
    IEcs& ecs, Entity entity, unordered_map<Entity, Entity>& oldToNew)
{
    auto managers = ecs.GetComponentManagers();
    for (auto cm : managers) {
        auto* data = cm->GetData(id);
        if (data) {
            // Find all entity references from this component.
            vector<Entity*> entities;
            vector<EntityReference*> entityRefs;
            uintptr_t offset = (uintptr_t)data->RLock();
            if (offset) {
                for (const auto& property : data->Owner()->MetaData()) {
                    GatherEntityReferences(entities, entityRefs, property, offset + property.offset);
                }

                // Rewrite old entity values with new ones. Assuming that the memory locations are the same as in
                // the RLock. NOTE: Keeping the read access open and we must not change any container sizes.
                if (!entities.empty()) {
                    data->WLock();
                    data->WUnlock();
                }
            }

            data->RUnlock();
        }
    }
}

inline vector<Entity> CloneEntities(
    IEcs& srcEcs, array_view<const Entity> src, IEcs& dstEcs)
{
    vector<Entity> clonedEntities;
    clonedEntities.reserve(src.size());
    for (auto srcEntity : src) {
        clonedEntities.emplace_back(CloneEntity(srcEcs, srcEntity, dstEcs));
    }
    return clonedEntities;
}

inline vector<EntityReference> CloneEntityReferences(
    IEcs& srcEcs, array_view<const EntityReference> src, IEcs& dstEcs)
{
    vector<EntityReference> clonedEntities;
    clonedEntities.reserve(src.size());
    for (const auto& srcEntity : src) {
        clonedEntities.emplace_back(CloneEntityReference(srcEcs, srcEntity, dstEcs));
    }
    return clonedEntities;
}

inline vector<EntityReference> CloneEntitiesUpdateRefs(
    IEcs& srcEcs, array_view<const EntityReference> src, IEcs& dstEcs)
{
    unordered_map<Entity, Entity> oldToNew;
    vector<EntityReference> clonedEntities;
    clonedEntities.reserve(src.size());

    for (const auto& entity : clonedEntities) {
        RewriteEntityReferences(dstEcs, entity, oldToNew);
    }
    return clonedEntities;
}

ECS_SERIALIZER_END_NAMESPACE()

#endif
