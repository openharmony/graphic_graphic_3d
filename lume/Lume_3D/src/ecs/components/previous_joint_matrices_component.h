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

#if !defined(API_3D_ECS_COMPONENTS_PREVIOUS_JOINT_MATRICES_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_PREVIOUS_JOINT_MATRICES_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

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
    DEFINE_PROPERTY(size_t, count, "Number Of Matrices", 0, 0)

    /**
     * Joint matrices for the skinned entity. SkinningSystem calculates the joint matrices
     */
    DEFINE_ARRAY_PROPERTY(
        BASE_NS::Math::Mat4X4, CORE_DEFAULT_MATERIAL_MAX_JOINT_COUNT, jointMatrices, "Joint Matrices", 0, ARRAY_VALUE())

END_COMPONENT(
    IPreviousJointMatricesComponentManager, PreviousJointMatricesComponent, "7f1327fc-7869-4952-aa87-3b55f40c8a80")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif