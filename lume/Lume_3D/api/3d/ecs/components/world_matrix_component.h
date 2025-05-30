/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#if !defined(API_3D_ECS_COMPONENTS_WORLD_MATRIX_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_WORLD_MATRIX_COMPONENT_H

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
    /** World space transformation of the object from the previous frame.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Mat4X4, prevMatrix, "Previous World Matrix", 0, VALUE(BASE_NS::Math::IDENTITY_4X4))

END_COMPONENT(IWorldMatrixComponentManager, WorldMatrixComponent, "4f76b9cc-4586-434d-a4dd-3bd115188d48")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
