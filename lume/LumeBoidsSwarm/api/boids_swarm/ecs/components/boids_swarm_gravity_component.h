/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#if !defined(BOIDS_SWARM_ECS_COMPONENTS_BOIDS_SWARM_GRAVITY_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define BOIDS_SWARM_ECS_COMPONENTS_BOIDS_SWARM_GRAVITY_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <boids_swarm/namespace.h>

#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

BOIDSSWARM_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IBoidsSwarmGravityComponentManager, BoidsSwarmGravityComponent)
// Radius of influence; boids within this distance from the entity are attracted.
// Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, radius, "Radius", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)
// Magnitude of gravitational acceleration applied toward the entity.
// Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, accelerationMag, "Acceleration Magnitude", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)
END_COMPONENT(IBoidsSwarmGravityComponentManager, BoidsSwarmGravityComponent, "a1b2c3d4-e5f6-7890-abcd-ef1234567890")

#if !defined(IMPLEMENT_MANAGER)
BOIDSSWARM_END_NAMESPACE()
#endif
#endif
