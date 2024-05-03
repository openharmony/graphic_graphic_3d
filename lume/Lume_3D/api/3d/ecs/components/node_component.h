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

#if !defined(API_3D_ECS_COMPONENTS_HIERARCHY_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_HIERARCHY_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Node component can be used to create hierarchy for the objects in scene.
 */
BEGIN_COMPONENT(INodeComponentManager, NodeComponent)

    /** Id of the parent entity in hierarchy.
     */
    DEFINE_PROPERTY(CORE_NS::Entity, parent, "Parent", 0, VALUE(CORE_NS::Entity { CORE_NS::INVALID_ENTITY }))

    /** Defines whether this entity is enabled or disabled. */
    DEFINE_PROPERTY(bool, enabled, "Enabled", 0, true)

    /** Read-only property to check if this entity is enabled and belongs to enabled scene tree (all parents are also
     * enabled). */
    DEFINE_PROPERTY(bool, effectivelyEnabled, "Effectively Enabled", CORE_NS::PropertyFlags::IS_READONLY, true)

    /** Defines whether this entity is included in an exported scene. */
    DEFINE_PROPERTY(bool, exported, "Exported", 0, true)

END_COMPONENT(INodeComponentManager, NodeComponent, "d9d330b5-3900-4503-8c89-233c0c9184de")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
