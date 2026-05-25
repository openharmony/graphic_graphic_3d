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

#if !defined(BOIDS_SWARM_STATE_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define BOIDS_SWARM_STATE_COMPONENT

#include <cstddef>

#if !defined(IMPLEMENT_MANAGER)
#include <boids_swarm/namespace.h>

#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

BOIDSSWARM_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IBoidsSwarmStateComponentManager, BoidsSwarmStateComponent)

#if !defined(IMPLEMENT_MANAGER)
    static constexpr size_t VELOCITY_COUNT { 3 };
    static constexpr size_t VELOCITY_CURRENT_INDEX { 0 };
#endif

    // Velocity history ring buffer (3 entries). Index 0 is the current velocity.
    // Runtime-computed, read-only. Default: (0, 0, 0) per entry.
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::Vec3, VELOCITY_COUNT, velocities, "Velocities",
        CORE_NS::PropertyFlags::IS_READONLY, ARRAY_VALUE())
    // Current speed scalar (magnitude of velocity at VELOCITY_CURRENT_INDEX).
    // Runtime-computed, read-only. Default: 0.0f.
    DEFINE_PROPERTY(float, velocityMag, "Velocity Magnitude", CORE_NS::PropertyFlags::IS_READONLY, 0.0f)

END_COMPONENT(IBoidsSwarmStateComponentManager, BoidsSwarmStateComponent, "b2c3d4e5-f6a7-4890-b123-45678901cdef")

#if !defined(IMPLEMENT_MANAGER)
BOIDSSWARM_END_NAMESPACE()
#endif

#endif
