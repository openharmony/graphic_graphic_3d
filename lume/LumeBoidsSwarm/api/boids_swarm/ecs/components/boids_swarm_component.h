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

#if !defined(BOIDS_SWARM_ECS_COMPONENTS_BOIDS_SWARM_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define BOIDS_SWARM_ECS_COMPONENTS_BOIDS_SWARM_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <boids_swarm/ecs/systems/intf_boids_swarm_system.h>
#include <boids_swarm/namespace.h>
#include <limits>

#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

BOIDSSWARM_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(IBoidsSwarmComponentManager, BoidsSwarmComponent)
// Initial velocity of the boid. Default: (0, 0, 0). Range: any Vec3.
DEFINE_PROPERTY(BASE_NS::Math::Vec3, initialVelocity, "Initial Velocity", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))

// Initial world position. NaN per boid means use the entity's current transform position.
// Default: (NaN, NaN, NaN). Range: any Vec3.
DEFINE_PROPERTY(BASE_NS::Math::Vec3, initialPosition, "Initial Position", 0,
    ARRAY_VALUE(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()))
// Initial rotation (quaternion). NaN per boid means use the entity's current transform rotation.
// Default: (NaN, NaN, NaN, NaN). Range: any Quat.
DEFINE_PROPERTY(BASE_NS::Math::Quat, initialRotation, "Initial Rotation", 0,
    ARRAY_VALUE(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()))

DEFINE_PROPERTY(BASE_NS::Math::Vec3, prevInitialVelocity, "",
    CORE_NS::PropertyFlags::IS_HIDDEN | CORE_NS::PropertyFlags::NO_SERIALIZE | CORE_NS::PropertyFlags::IS_READONLY,
    ARRAY_VALUE(0.0f, 0.0f, 0.0f))
DEFINE_PROPERTY(BASE_NS::Math::Vec3, prevInitialPosition, "",
    CORE_NS::PropertyFlags::IS_HIDDEN | CORE_NS::PropertyFlags::NO_SERIALIZE | CORE_NS::PropertyFlags::IS_READONLY,
    ARRAY_VALUE(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN()))
DEFINE_PROPERTY(BASE_NS::Math::Quat, prevInitialRotation, "",
    CORE_NS::PropertyFlags::IS_HIDDEN | CORE_NS::PropertyFlags::NO_SERIALIZE | CORE_NS::PropertyFlags::IS_READONLY,
    ARRAY_VALUE(std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::quiet_NaN()))

// Lower and upper corners of the axis-aligned bounding box constraining boid movement.
// Note: When any component of boundaryMinPos is greater than or equal to the corresponding component of
// boundaryMaxPos, this boid is considered unbounded. Default: (0, 0, 0). Range: any Vec3.
DEFINE_PROPERTY(BASE_NS::Math::Vec3, boundaryMinPos, "Boundary Min Position", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))
DEFINE_PROPERTY(BASE_NS::Math::Vec3, boundaryMaxPos, "Boundary Max Position", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))

#if !defined(IMPLEMENT_MANAGER)
static constexpr float DEFAULT_MAX_VELOCITY_MAG = 0.01f / IBoidsSwarmSystem::DEFAULT_TIME_STEP_SEC;
static constexpr float DEFAULT_MAX_FORCE_MAG = DEFAULT_MAX_VELOCITY_MAG / IBoidsSwarmSystem::DEFAULT_TIME_STEP_SEC;
static constexpr float DEFAULT_MAX_TURN_RATE = BASE_NS::Math::PI * 0.75f * IBoidsSwarmSystem::DEFAULT_TIME_STEP_SEC;
#endif

// Maximum speed the boid can reach per simulation frame. Default: DEFAULT_MAX_VELOCITY_MAG (~0.625).
// Range: [0, +inf).
DEFINE_PROPERTY(
    float, maxVelocityMag, "Max Velocity Magnitude", CORE_NS::PropertyFlags::HAS_MIN, DEFAULT_MAX_VELOCITY_MAG)
// Maximum acceleration the boid can reach per simulation frame. Default: DEFAULT_MAX_FORCE_MAG (~39.06).
// Range: [0, +inf).
DEFINE_PROPERTY(
    float, maxAccelerationMag, "Max Acceleration Magnitude", CORE_NS::PropertyFlags::HAS_MIN, DEFAULT_MAX_FORCE_MAG)
// Per-axis rotation limit in radians per simulation frame. Default: DEFAULT_MAX_TURN_RATE (~0.0377) on all axes.
// Range: [0, +inf) per axis.
DEFINE_PROPERTY(BASE_NS::Math::Vec3, maxTurnRate, "Max Turn Rate (rad/sim_frame)", CORE_NS::PropertyFlags::HAS_MIN,
    ARRAY_VALUE(DEFAULT_MAX_TURN_RATE, DEFAULT_MAX_TURN_RATE, DEFAULT_MAX_TURN_RATE))

// How strongly the boid steers away from nearby neighbours within separationDistance.
// Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, separationWeight, "Separation Weight", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)
// Perception radius for the separation rule. Only boids strictly within this distance contribute to
// separation force (force is zero at the boundary). Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, separationDistance, "Separation Distance", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)

// How strongly the boid steers to match the average heading of neighbours within alignmentDistance.
// Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, alignmentWeight, "Alignment Weight", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)
// Perception radius for the alignment rule. Boids within this distance (inclusive) contribute to alignment
// force. Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, alignmentDistance, "Alignment Distance", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)

// How strongly the boid steers toward the average position of neighbours within cohesionDistance.
// Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, cohesionWeight, "Cohesion Weight", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)
// Perception radius for the cohesion rule. Boids within this distance (inclusive) contribute to cohesion
// force. Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, cohesionDistance, "Cohesion Distance", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)

// How strongly the boid is pushed back when approaching the boundary box edges.
// Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, boundaryWeight, "Boundary Weight", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)
// Distance from boundary walls where the boundary repulsion force begins to take effect.
// Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, boundaryDistance, "Boundary Distance", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)

// How strongly gravity field entities attract this boid. Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, gravityWeight, "Gravity Weight", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)

// How strongly repulsion field entities push this boid away. Default: 0.0f. Range: [0, +inf).
DEFINE_PROPERTY(float, repulsionWeight, "Repulsion Weight", CORE_NS::PropertyFlags::HAS_MIN, 0.0f)

// Constant external force vector added every frame (e.g. wind, current). Default: (0, 0, 0).
DEFINE_PROPERTY(BASE_NS::Math::Vec3, drivenForce, "Extra Driven Force", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f))
// Scales the drivenForce contribution in the weighted force sum. Default: 1.0f. Range: [0, +inf).
// Range: [0, +inf) per boid (clamped to magnitude <= maxAccelerationMag).
DEFINE_PROPERTY(float, drivenForceWeight, "Extra Driven Force Weight", CORE_NS::PropertyFlags::HAS_MIN, 1.0f)

DEFINE_PROPERTY(bool, velDirIsFwd, "Velocity Direction Is Forward", 0, false)

// Entity list used as separation neighbours instead of all swarm members.
// Default: empty. Range: any list of valid Entity references.
DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::Entity>, separationTargets, "Separation Targets", 0, )
// Entity list used as alignment neighbours instead of all swarm members.
// Default: empty. Range: any list of valid Entity references.
DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::Entity>, alignmentTargets, "Alignment Targets", 0, )
// Entity list used as cohesion neighbours instead of all swarm members.
// Default: empty. Range: any list of valid Entity references.
DEFINE_PROPERTY(BASE_NS::vector<CORE_NS::Entity>, cohesionTargets, "Cohesion Targets", 0, )

END_COMPONENT(IBoidsSwarmComponentManager, BoidsSwarmComponent, "fbd41220-246c-4ea4-ba05-ffd0fa6a3efe")
#if !defined(IMPLEMENT_MANAGER)
BOIDSSWARM_END_NAMESPACE()
#endif
#endif
