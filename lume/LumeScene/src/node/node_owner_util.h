/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SCENE_SRC_NODE_NODE_OWNER_UTIL_H
#define SCENE_SRC_NODE_NODE_OWNER_UTIL_H

#include <scene/ext/intf_ecs_context.h>
#include <scene/ext/intf_ecs_object.h>
#include <scene/ext/intf_internal_scene.h>

#include "../ecs_component/entity_owner_component.h"

SCENE_BEGIN_NAMESPACE()

/**
 * @brief Make a node's entity own another entity, so the owned entity (e.g. a node-generated mesh) is
 *        destroyed together with the node. Shared by node types that own engine-side resources.
 */
inline void SetEntityOwner(const IEcsObject::Ptr& nodeObject, CORE_NS::Entity owned)
{
    if (!nodeObject) {
        return;
    }
    auto scene = nodeObject->GetScene();
    if (!scene) {
        return;
    }
    auto ecs = scene->GetEcsContext().GetNativeEcs();
    auto ownerManager = CORE_NS::GetManager<IEntityOwnerComponentManager>(*ecs);
    if (!ownerManager) {
        return;
    }
    auto nodeEntity = nodeObject->GetEntity();
    ownerManager->Create(nodeEntity);
    if (auto ownerHandle = ownerManager->Write(nodeEntity)) {
        ownerHandle->entity = ecs->GetEntityManager().GetReferenceCounted(owned);
    }
}

SCENE_END_NAMESPACE()

#endif
