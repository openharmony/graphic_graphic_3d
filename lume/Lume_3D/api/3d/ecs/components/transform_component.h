/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(TRANSFORM_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define TRANSFORM_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Transform component is used to store and manipulate the position, rotation and scale of the object.
 */
BEGIN_COMPONENT(ITransformComponentManager, TransformComponent)

    /** Position of the transform relative to the parent object.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, position, "Position", 0, ARRAY_VALUE(0.f, 0.f, 0.f))

    /** Rotation of the transform relative to the parent object.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Quat, rotation, "Rotation", 0, ARRAY_VALUE(0.f, 0.f, 0.f, 1.f))

    /** Scale of the transform relative to the parent object.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, scale, "Scale", 0, ARRAY_VALUE(1.f, 1.f, 1.f))

END_COMPONENT(ITransformComponentManager, TransformComponent, "07786a48-a52b-4425-8229-8c0d955dc4cd")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
