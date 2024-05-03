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

#if !defined(API_3D_ECS_COMPONENTS_TRANSFORM_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_TRANSFORM_COMPONENT_H

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
