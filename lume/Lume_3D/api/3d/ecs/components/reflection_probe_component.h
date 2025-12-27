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

#if !defined(API_3D_ECS_COMPONENTS_REFLECTION_PROBE_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_REFLECTION_PROBE_COMPONENT_H
#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IReflectionProbeComponentManager, ReflectionProbeComponent)
#if !defined(IMPLEMENT_MANAGER)
#endif
    /** Camera that will act as a probes rendering configuration
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, probeCamera, "Probe Camera", 0,)

END_COMPONENT(IReflectionProbeComponentManager, ReflectionProbeComponent, "714037fc-d0fa-48e0-9d93-7bbcabee6b7c")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
