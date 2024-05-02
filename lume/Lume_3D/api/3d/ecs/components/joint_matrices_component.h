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

#if !defined(API_3D_ECS_COMPONENTS_JOINT_MATRICES_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_JOINT_MATRICES_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
}
#endif
/** Contains the joint matrices of a skinned entity.
SkinComponent points the ISkin resources containing joint names and the inverse bind matrices of the joints.
SkinJointsComponent contains the entities which match the joint names in the hierarchy of the skinned entity.
*/
BEGIN_COMPONENT(IJointMatricesComponentManager, JointMatricesComponent)

    /**
     * Number of valid entries in jointMatrices array.
     */
    DEFINE_PROPERTY(size_t, count, "Number of Matrices", 0, 0)

    /**
     * Joint matrices for the skinned entity. SkinningSystem calculates the joint matrices
     */
    DEFINE_ARRAY_PROPERTY(
        BASE_NS::Math::Mat4X4, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointMatrices, "Joint Matrices", 0, ARRAY_VALUE())

    /**
     * Array of AABB minimum values for each joint in world space. Updated each frame by the SkinningSystem.
     */
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::Vec3, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointAabbMinArray,
        "Joint AABB Min Values", 0, ARRAY_VALUE())

    /**
     * Array of AABB maximium values for each joint in world space. Updated each frame by the SkinningSystem.
     */
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::Vec3, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointAabbMaxArray,
        "Joint AABB Max Values", 0, ARRAY_VALUE())

    /**
     * Minimum corner for the AABB that contains all the joints.
     */
    DEFINE_PROPERTY(
        BASE_NS::Math::Vec3, jointsAabbMin, "Combined Joint AABB Min Values", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))

    /**
     * Maximum corner for the AABB that contains all the joints.
     */
    DEFINE_PROPERTY(
        BASE_NS::Math::Vec3, jointsAabbMax, "Combined Joint AABB Max Values", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))

END_COMPONENT(IJointMatricesComponentManager, JointMatricesComponent, "59d701be-f741-4faa-b5d6-a4f20ad4e317")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif