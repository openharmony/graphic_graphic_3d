/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(LOCAL_MATRIX_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define LOCAL_MATRIX_COMPONENT

#if !defined(IMPLEMENT_MANAGER)

#include <3d/namespace.h>
#include <base/math/matrix.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
BEGIN_COMPONENT(ILocalMatrixComponentManager, LocalMatrixComponent)
    /** Local matrix component */
    DEFINE_PROPERTY(BASE_NS::Math::Mat4X4, matrix, "Matrix", 0, VALUE(BASE_NS::Math::IDENTITY_4X4))
END_COMPONENT(ILocalMatrixComponentManager, LocalMatrixComponent, "06dce540-17f6-4446-ad43-2bfd7c61aa0d")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
