/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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


#if !defined(SCENE_SRC_ECS_COMPONENT_ENTITY_OWNER_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define SCENE_SRC_ECS_COMPONENT_ENTITY_OWNER_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <scene/base/namespace.h>

#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

SCENE_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IEntityOwnerComponentManager, EntityOwnerComponent)
    /** Owned entity. */
    DEFINE_PROPERTY(CORE_NS::EntityReference, entity, "Entity", 0,)
END_COMPONENT(IEntityOwnerComponentManager, EntityOwnerComponent, "aa74aebf-58d5-46bf-b6f2-bdcb55274e9e")
#if !defined(IMPLEMENT_MANAGER)
SCENE_END_NAMESPACE()
#endif

#endif