/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#if !defined(SKIN_JOINTS_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define SKIN_JOINTS_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
}
#endif
/** Contains the entities which are the joints in the skinned entities hierarchy. SkinComponent points the ISkin
 * resources containing joint names and the inverse bind matrices of the joints. JointMatricesComponent contains the
 * joint matrices of the joint entities in this skin hierarchy.
 */
BEGIN_COMPONENT(ISkinJointsComponentManager, SkinJointsComponent)

    /** Number of valid entries in jointEntries array.
     */
    DEFINE_PROPERTY(size_t, count, "Number of Joints", 0, 0)

    /** Entities which are part of this skin hierarchy.
    SkinningSystem fills this and it's expected to be read-only afterwards.
    */
    DEFINE_ARRAY_PROPERTY(
        CORE_NS::Entity, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointEntities, "Joint Entities", 0, ARRAY_VALUE())

END_COMPONENT(ISkinJointsComponentManager, SkinJointsComponent, "e1a12201-a395-4579-b755-995c3c21d141")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
