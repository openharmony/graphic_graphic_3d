/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(PREVIOUS_JOINT_MATRICES_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define PREVIOUS_JOINT_MATRICES_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
namespace {
#include <3d/shaders/common/3d_dm_structures_common.h>
}
#endif
/** Contains the previous joint matrices of a skinned entity.
 */
BEGIN_COMPONENT(IPreviousJointMatricesComponentManager, PreviousJointMatricesComponent)

    /**
     * Number of valid entries in jointMatrices array.
     */
    DEFINE_PROPERTY(size_t, count, "Number of matrices", 0, 0)

    /**
     * Joint matrices for the skinned entity. SkinningSystem calculates the joint matrices
     */
    DEFINE_ARRAY_PROPERTY(
        BASE_NS::Math::Mat4X4, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointMatrices, "Joint matrices", 0, ARRAY_VALUE())

END_COMPONENT(
    IPreviousJointMatricesComponentManager, PreviousJointMatricesComponent, "7f1327fc-7869-4952-aa87-3b55f40c8a80")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif