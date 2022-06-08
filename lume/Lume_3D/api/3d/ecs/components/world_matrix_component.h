/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(WORLD_MATRIX_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define WORLD_MATRIX_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** World Matrix component is used to store the final world-space transformation of the object and can be used for
 * example in rendering.
 */
BEGIN_COMPONENT(IWorldMatrixComponentManager, WorldMatrixComponent)

    /** World space transformation of the object.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Mat4X4, matrix, "World Matrix", 0, VALUE(BASE_NS::Math::IDENTITY_4X4))

END_COMPONENT(IWorldMatrixComponentManager, WorldMatrixComponent, "4f76b9cc-4586-434d-a4dd-3bd115188d48")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
