/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(JOINT_MATRICES_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define JOINT_MATRICES_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

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
    DEFINE_PROPERTY(size_t, count, "Number of matrices", 0, 0)

    /**
     * Joint matrices for the skinned entity. SkinningSystem calculates the joint matrices
     */
    DEFINE_ARRAY_PROPERTY(
        BASE_NS::Math::Mat4X4, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointMatrices, "Joint matrices", 0, ARRAY_VALUE())

    /**
     * Array of AABB minimum values for each joint in world space. Updated each frame by the SkinningSystem.
     */
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::Vec3, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointAabbMinArray,
        "Joint AABB min vectors", 0, ARRAY_VALUE())

    /**
     * Array of AABB maximium values for each joint in world space. Updated each frame by the SkinningSystem.
     */
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::Vec3, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointAabbMaxArray,
        "Joint AABB max vectors", 0, ARRAY_VALUE())

    /**
     * Minimum corner for the AABB that contains all the joints.
     */
    DEFINE_PROPERTY(
        BASE_NS::Math::Vec3, jointsAabbMin, "Combined joint AABB min values", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))

    /**
     * Maximum corner for the AABB that contains all the joints.
     */
    DEFINE_PROPERTY(
        BASE_NS::Math::Vec3, jointsAabbMax, "Combined joint AABB max values", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))

END_COMPONENT(IJointMatricesComponentManager, JointMatricesComponent, "59d701be-f741-4faa-b5d6-a4f20ad4e317")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif