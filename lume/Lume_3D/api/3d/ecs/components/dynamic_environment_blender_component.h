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

#if !defined(API_3D_ECS_COMPONENTS_DYNAMIC_ENVIRONMENT_BLENDER_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_DYNAMIC_ENVIRONMENT_BLENDER_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IDynamicEnvironmentBlenderComponentManager, DynamicEnvironmentBlenderComponent)

    /** Entities containing EnvironmentComponents that are pushed to camera buffers and can be blended.
     *  Controls indirect and environment lighting. Currently there is a limit of 4 blend environments.
     */
    DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::Entity>, environments, "Environments", 0,)

    /** Environment blend weights in a Vec4(Vector4).
     *  Each component of the vector decides weight in the total of 100% blend (for example vec4(0.25f, 0.75f, 0, 0)
     *  would result in environment1 having the weight of 25% and environment2 having 75%).
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, entryFactor, "Entry Factor", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f, 0.0f))

    /** Environment blend interpolator target values as Vec4.
     *  Each vector4 component acts as a weight (for example entry of 0.25, 0.25, 0.25, 0.25 with switch of 0.9, 0, 0, 0
     *  would make entry of 0.9, 0.033, 0.033, 0.033)
     *  Can also be set as full 100% (The values form full 1.0).
     *  User is responsible for keeping the 100% value (Total not exceeding 100%).
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, switchFactor, "Switch Factor", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f, 0.0f))

END_COMPONENT(IDynamicEnvironmentBlenderComponentManager, DynamicEnvironmentBlenderComponent,
    "0e1d7883-4609-427e-a05c-9aa7ad42e188")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
