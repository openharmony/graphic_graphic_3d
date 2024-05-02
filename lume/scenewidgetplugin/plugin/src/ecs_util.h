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

#ifndef SCENE_PLUGIN_ECSUTIL_H
#define SCENE_PLUGIN_ECSUTIL_H

#include <PropertyTools/property_data.h>

#include <base/containers/string.h>
#include <base/containers/unordered_map.h>
#include <base/math/vector.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/ecs/intf_ecs.h>
#include <core/ecs/intf_entity_manager.h>
#include <core/property/intf_property_api.h>
#include <core/property/property_types.h>
#include <core/property/scoped_handle.h>

CORE_BEGIN_NAMESPACE()

inline void CloneComponent(CORE_NS::Entity srcEntity, const CORE_NS::IComponentManager& srcManager,
    CORE_NS::IEcs& dstEcs, CORE_NS::Entity dstEntity)
{
    auto* dstManager = dstEcs.GetComponentManager(srcManager.GetUid());
    if (dstManager) {
        // Get copy destiantion property handle.
        auto componentId = dstManager->GetComponentId(dstEntity);
        if (componentId == CORE_NS::IComponentManager::INVALID_COMPONENT_ID) {
            dstManager->Create(dstEntity);
            componentId = dstManager->GetComponentId(dstEntity);
        }
        BASE_ASSERT(componentId != CORE_NS::IComponentManager::INVALID_COMPONENT_ID);
        const auto* srcHandle = srcManager.GetData(srcEntity);
        if (srcHandle) {
            dstManager->SetData(dstEntity, *srcHandle);
        }
    }
}

inline void CloneComponents(
    CORE_NS::IEcs& srcEcs, CORE_NS::Entity srcEntity, CORE_NS::IEcs& dstEcs, CORE_NS::Entity dstEntity)
{
    BASE_NS::vector<CORE_NS::IComponentManager*> managers;
    srcEcs.GetComponents(srcEntity, managers);
    for (auto* srcManager : managers) {
        CloneComponent(srcEntity, *srcManager, dstEcs, dstEntity);
    }
}

inline CORE_NS::Entity CloneEntity(CORE_NS::IEcs& srcEcs, CORE_NS::Entity src, CORE_NS::IEcs& dstEcs)
{
    CORE_NS::Entity dst = dstEcs.GetEntityManager().Create();
    CloneComponents(srcEcs, src, dstEcs, dst);
    return dst;
}

inline CORE_NS::EntityReference CloneEntityReference(CORE_NS::IEcs& srcEcs, CORE_NS::Entity src, CORE_NS::IEcs& dstEcs)
{
    CORE_NS::EntityReference dst = dstEcs.GetEntityManager().CreateReferenceCounted();
    CloneComponents(srcEcs, src, dstEcs, dst);
    return dst;
}

inline void GatherEntityReferences(BASE_NS::vector<CORE_NS::Entity*>& entities,
    BASE_NS::vector<CORE_NS::EntityReference*>& entityReferences, const CORE_NS::Property& property,
    uintptr_t offset = 0)
{
    if (property.type == CORE_NS::PropertyType::ENTITY_T) {
        entities.emplace_back(reinterpret_cast<CORE_NS::Entity*>(offset));
    } else if (property.type == CORE_NS::PropertyType::ENTITY_REFERENCE_T) {
        entityReferences.emplace_back(reinterpret_cast<CORE_NS::EntityReference*>(offset));
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

inline void RewriteEntityReferences(
    CORE_NS::IEcs& ecs, CORE_NS::Entity entity, BASE_NS::unordered_map<CORE_NS::Entity, CORE_NS::Entity>& oldToNew)
{
    // Go through the entity properties and update any entity references to point to the ones pointed by the oldToNew
    // map.
    auto managers = ecs.GetComponentManagers();
    for (auto cm : managers) {
        if (auto id = cm->GetComponentId(entity); id != CORE_NS::IComponentManager::INVALID_COMPONENT_ID) {
            auto* data = cm->GetData(id);
            if (data) {
                // Find all entity references from this component.
                BASE_NS::vector<CORE_NS::Entity*> entities;
                BASE_NS::vector<CORE_NS::EntityReference*> entityRefs;
                uintptr_t offset = (uintptr_t)data->RLock();
                if (offset) {
                    for (const auto& property : data->Owner()->MetaData()) {
                        GatherEntityReferences(entities, entityRefs, property, offset + property.offset);
                    }

                    // Rewrite old entity values with new ones. Assuming that the memory locations are the same as in
                    // the RLock. NOTE: Keeping the read access open and we must not change any container sizes.
                    if (!entities.empty() || !entityRefs.empty()) {
                        data->WLock();
                        for (CORE_NS::Entity* entity : entities) {
                            if (const auto it = oldToNew.find(*entity); it != oldToNew.end()) {
                                *entity = it->second;
                            }
                        }
                        for (CORE_NS::EntityReference* entity : entityRefs) {
                            if (const auto it = oldToNew.find(*entity); it != oldToNew.end()) {
                                *entity = ecs.GetEntityManager().GetReferenceCounted(it->second);
                            }
                        }
                        data->WUnlock();
                    }
                }

                data->RUnlock();
            }
        }
    }
}

inline BASE_NS::vector<CORE_NS::Entity> CloneEntities(
    CORE_NS::IEcs& srcEcs, BASE_NS::array_view<const CORE_NS::Entity> src, CORE_NS::IEcs& dstEcs)
{
    BASE_NS::vector<CORE_NS::Entity> clonedEntities;
    clonedEntities.reserve(src.size());
    for (const auto& srcEntity : src) {
        clonedEntities.emplace_back(CloneEntity(srcEcs, srcEntity, dstEcs));
    }
    return clonedEntities;
}

inline BASE_NS::vector<CORE_NS::EntityReference> CloneEntityReferences(
    CORE_NS::IEcs& srcEcs, BASE_NS::array_view<const CORE_NS::EntityReference> src, CORE_NS::IEcs& dstEcs)
{
    BASE_NS::vector<CORE_NS::EntityReference> clonedEntities;
    clonedEntities.reserve(src.size());
    for (const auto& srcEntity : src) {
        clonedEntities.emplace_back(CloneEntityReference(srcEcs, srcEntity, dstEcs));
    }
    return clonedEntities;
}

inline BASE_NS::vector<CORE_NS::EntityReference> CloneEntitiesUpdateRefs(
    CORE_NS::IEcs& srcEcs, BASE_NS::array_view<const CORE_NS::EntityReference> src, CORE_NS::IEcs& dstEcs)
{
    BASE_NS::unordered_map<CORE_NS::Entity, CORE_NS::Entity> oldToNew;

    BASE_NS::vector<CORE_NS::EntityReference> clonedEntities;
    clonedEntities.reserve(src.size());
    for (const auto& srcEntity : src) {
        clonedEntities.emplace_back(CloneEntityReference(srcEcs, srcEntity, dstEcs));
        oldToNew[srcEntity] = clonedEntities.back();
    }

    for (auto& entity : clonedEntities) {
        RewriteEntityReferences(dstEcs, entity, oldToNew);
    }
    return clonedEntities;
}

inline bool isPropertyContainer(const CORE_NS::Property& property)
{
    return property.type == PROPERTYTYPE(CORE_NS::IPropertyHandle*);
}

inline CORE_NS::IPropertyHandle* ResolveContainerProperty(const CORE_NS::IPropertyHandle& handle,
    const BASE_NS::string& propertyPath, BASE_NS::string& path, BASE_NS::string& name)
{
    // Extract property path.
    auto separatorPosition = propertyPath.find_first_of('.');
    if (separatorPosition == BASE_NS::string::npos) {
        return nullptr;
    }

    path = propertyPath.substr(0, separatorPosition);
    name = propertyPath.substr(separatorPosition + 1);

    CORE_NS::IPropertyHandle* result = nullptr;

    uintptr_t offset = uintptr_t(handle.RLock());

    // Get potential container.
    auto propertyData = CORE_NS::PropertyData::FindProperty(handle.Owner()->MetaData(), path, offset);
    if (propertyData) {
        // Ensure it is a container.
        if (CORE_NS::isPropertyContainer(*propertyData.property)) {
            // Try to flush value to container.
            result = *(CORE_NS::IPropertyHandle**)(propertyData.offset);
        }
    }

    handle.RUnlock();

    return result;
}

CORE_END_NAMESPACE()

#endif
