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

#include "boids_swarm_system.h"

#include <random>
#include <tuple>
#include <utility>

#include <base/math/mathf.h>
#include <base/math/quaternion_util.h>
#include <base/math/vector_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/log.h>
#include <core/namespace.h>
#include <core/perf/cpu_perf_scope.h>
#include <core/property/property_types.h>
#include <core/property_tools/property_api_impl.inl>

namespace {
static constexpr uint32_t BOIDSSWARM_PROFILER_DEFAULT_COLOR{0xff00ff};

static constexpr auto VELOCITY_COUNT{BOIDSSWARM_NS::BoidsSwarmStateComponent::VELOCITY_COUNT};
static constexpr auto VELOCITY_CURRENT_INDEX{BOIDSSWARM_NS::BoidsSwarmStateComponent::VELOCITY_CURRENT_INDEX};

struct GravityField {};
struct RepulsionField {};

void ValidateComponentHasMin(void* componentData, BASE_NS::array_view<const CORE_NS::Property> metadata)
{
    for (const auto& prop : metadata) {
        if (!(prop.flags & static_cast<uint32_t>(CORE_NS::PropertyFlags::HAS_MIN))) {
            continue;
        }

        bool isClmapable =
            prop.type == CORE_NS::PropertyType::FLOAT_T || prop.type == CORE_NS::PropertyType::VEC2_ARRAY_T ||
            prop.type == CORE_NS::PropertyType::VEC3_ARRAY_T || prop.type == CORE_NS::PropertyType::VEC4_ARRAY_T;
        if (!isClmapable) {
            continue;
        }
        auto* const floats = reinterpret_cast<float*>(reinterpret_cast<uintptr_t>(componentData) + prop.offset);
        const size_t floatCount = prop.size / sizeof(float);
        for (size_t i = 0; i < floatCount; ++i) {
            if (floats[i] < 0.0f) {
                floats[i] = 0.0f;
            }
        }
    }
}

struct QueryRowIndices {
    enum class SwarmEntity : uint32_t { BOIDS_SWARM_COMP = 0, TRANSFORM_COMP = 1, BOIDS_SWARM_STATE_COMP = 2 };
    enum class GravityEntity : uint32_t { BOIDS_SWARM_GRAVITY_COMP = 0, TRANSFORM_COMP = 1 };
    enum class RepulsionEntity : uint32_t { BOIDS_SWARM_REPULSION_COMP = 0, TRANSFORM_COMP = 1 };
};

template <typename T>
uint32_t GetQueryRowIndex(T IndexAsEnum)
{
    return static_cast<uint32_t>(IndexAsEnum);
}

BASE_NS::Math::Vec3 ClampMagnitude(const BASE_NS::Math::Vec3& v, float maxMagnitude)
{
    float sqrMag = BASE_NS::Math::SqrMagnitude(v);
    if (sqrMag > maxMagnitude * maxMagnitude) {
        return BASE_NS::Math::Normalize(v) * maxMagnitude;
    }
    return v;
}

BASE_NS::Math::Vec3 ComputeUpVector(const BASE_NS::Math::IVec3& axisMask)
{
    if (axisMask.x == 0 && axisMask.y != 0 && axisMask.z != 0) {
        return {1.f, 0.f, 0.f};
    }
    if (axisMask.x != 0 && axisMask.y != 0 && axisMask.z == 0) {
        return {0.f, 0.f, 1.f};
    }
    return {0.f, 1.f, 0.f};
}

BASE_NS::Math::Quat ClampRotationDelta(
    const BASE_NS::Math::Quat& source, const BASE_NS::Math::Quat& target, const BASE_NS::Math::Vec3& maxTurnRate)
{
    BASE_NS::Math::Quat delta = target * BASE_NS::Math::Inverse(source);
    BASE_NS::Math::Vec3 eulerDelta = BASE_NS::Math::ToEulerRad(delta);
    eulerDelta = BASE_NS::Math::Clamp(eulerDelta, -maxTurnRate, maxTurnRate);
    return BASE_NS::Math::Normalize(BASE_NS::Math::FromEulerRad(eulerDelta) * source);
}

CORE3D_NS::TransformComponent& operator>>(
    const BOIDSSWARM_NS::BoidsSwarmComponent& swarm, CORE3D_NS::TransformComponent& transform)
{
    if (!std::isnan(swarm.initialPosition.x) && !std::isnan(swarm.initialPosition.y) &&
        !std::isnan(swarm.initialPosition.z)) {
        transform.position = swarm.initialPosition;
    }
    if (!std::isnan(swarm.initialRotation.x) && !std::isnan(swarm.initialRotation.y) &&
        !std::isnan(swarm.initialRotation.z) && !std::isnan(swarm.initialRotation.w)) {
        transform.rotation = swarm.initialRotation;
    }
    return transform;
}

BOIDSSWARM_NS::BoidsSwarmSystem::BoidsSwarmFrameData& operator<<(
    BOIDSSWARM_NS::BoidsSwarmSystem::BoidsSwarmFrameData& frameData,
    const std::tuple<const CORE3D_NS::TransformComponent&, const BOIDSSWARM_NS::BoidsSwarmComponent&,
        const BOIDSSWARM_NS::BoidsSwarmStateComponent&>
        components)
{
    using namespace BASE_NS;

    const auto& transform = std::get<0>(components);
    const auto& swarm = std::get<1>(components);
    const auto& state = std::get<2>(components);

    frameData.rotation = transform.rotation;
    if (auto len = Math::Magnitude(state.velocities[VELOCITY_CURRENT_INDEX]); len > Math::EPSILON) {
        frameData.forward = state.velocities[VELOCITY_CURRENT_INDEX] / len;
    } else {
        frameData.forward = transform.rotation * Math::Vec3(0.f, 0.f, 1.f);
    }
    frameData.separationForce = Math::ZERO_VEC3;
    frameData.alignmentForce = Math::ZERO_VEC3;
    frameData.cohesionForce = Math::ZERO_VEC3;
    frameData.boundaryForce = Math::ZERO_VEC3;
    frameData.gravityForce = Math::ZERO_VEC3;
    frameData.repulsionForce = Math::ZERO_VEC3;
    frameData.boundaryMinPos = swarm.boundaryMinPos;
    frameData.boundaryMaxPos = swarm.boundaryMaxPos;
    frameData.maxTurnRate = swarm.maxTurnRate;
    for (size_t i = 0; i < VELOCITY_COUNT; ++i) {
        frameData.velocities[i + 1] = state.velocities[i];
    }
    frameData.totalWeight = 0.f;
    frameData.separationDistance = swarm.separationDistance;
    frameData.alignmentDistance = swarm.alignmentDistance;
    frameData.cohesionDistance = swarm.cohesionDistance;
    frameData.separationWeight = swarm.separationWeight;
    frameData.alignmentWeight = swarm.alignmentWeight;
    frameData.cohesionWeight = swarm.cohesionWeight;
    frameData.maxVelocityMag = swarm.maxVelocityMag;
    frameData.maxAccelerationMag = swarm.maxAccelerationMag;
    frameData.boundaryDistance = swarm.boundaryDistance;
    frameData.boundaryWeight = swarm.boundaryWeight;
    frameData.gravityWeight = swarm.gravityWeight;
    frameData.repulsionWeight = swarm.repulsionWeight;
    frameData.drivenForce = swarm.drivenForce;
    frameData.drivenForceWeight = swarm.drivenForceWeight;
    frameData.velDirIsFwd = swarm.velDirIsFwd;

    return frameData;
}

CORE3D_NS::TransformComponent& operator>>(
    const BOIDSSWARM_NS::BoidsSwarmSystem::BoidsSwarmFrameData& frameData, CORE3D_NS::TransformComponent& transform)
{
    transform.rotation = frameData.rotation;
    return transform;
}

BOIDSSWARM_NS::BoidsSwarmStateComponent& operator>>(
    const BOIDSSWARM_NS::BoidsSwarmSystem::BoidsSwarmFrameData& frameData,
    BOIDSSWARM_NS::BoidsSwarmStateComponent& state)
{
    for (size_t i = VELOCITY_COUNT - 1; i > 0; --i) {
        state.velocities[i] = state.velocities[i - 1];
    }
    state.velocities[VELOCITY_CURRENT_INDEX] = frameData.velocities[VELOCITY_CURRENT_INDEX];
    state.velocityMag = BASE_NS::Math::Magnitude(frameData.velocities[VELOCITY_CURRENT_INDEX]);
    return state;
}
}  // namespace

BOIDSSWARM_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

void BoidsSwarmSystem::SetActive(bool state)
{
    active_ = state;
}

bool BoidsSwarmSystem::IsActive() const
{
    return active_;
}

BoidsSwarmSystem::BoidsSwarmSystem(IEcs& ecs)
    : ecs_(ecs),
      boidsSwarmManager_(*(GetManager<IBoidsSwarmComponentManager>(ecs))),
      boidsSwarmGravityManager_(*(GetManager<IBoidsSwarmGravityComponentManager>(ecs))),
      boidsSwarmRepulsionManager_(*(GetManager<IBoidsSwarmRepulsionComponentManager>(ecs))),
      boidsSwarmStateManager_(*(GetManager<IBoidsSwarmStateComponentManager>(ecs))),
      transformManager_(*(GetManager<CORE3D_NS::ITransformComponentManager>(ecs))),
      randomEngine_(std::random_device{}())
{}

string_view BoidsSwarmSystem::GetName() const
{
    return BOIDSSWARM_NS::GetName(this);
}

Uid BoidsSwarmSystem::GetUid() const
{
    return UID;
}

const IPropertyHandle* BoidsSwarmSystem::GetProperties() const
{
    return nullptr;
}

IPropertyHandle* BoidsSwarmSystem::GetProperties()
{
    return nullptr;
}

void BoidsSwarmSystem::SetProperties(const IPropertyHandle&)
{}

const IEcs& BoidsSwarmSystem::GetECS() const
{
    return ecs_;
}

void BoidsSwarmSystem::SetTimeStepSec(float timeStepSec)
{
    timeStepSec_ = timeStepSec;
}

float BoidsSwarmSystem::GetTimeStepSec() const
{
    return timeStepSec_;
}

void BoidsSwarmSystem::SetAxisMask(const BASE_NS::Math::IVec3& axisMask)
{
    axisMask_ = {axisMask.x != 0 ? 1 : 0, axisMask.y != 0 ? 1 : 0, axisMask.z != 0 ? 1 : 0};
    axisMaskFloat_ = {axisMask.x != 0 ? 1.f : 0.f, axisMask.y != 0 ? 1.f : 0.f, axisMask.z != 0 ? 1.f : 0.f};
}

BASE_NS::Math::IVec3 BoidsSwarmSystem::GetAxisMask() const
{
    return axisMask_;
}

void BoidsSwarmSystem::Play()
{
    active_ = true;
}

void BoidsSwarmSystem::Pause()
{
    active_ = false;
}

void BoidsSwarmSystem::Stop()
{
    active_ = false;
    accumulatedTime_ = 0;

    swarmEntityQuery_.Execute();
    const auto& results = swarmEntityQuery_.GetResults();

    notPlayedEntities_.clear();
    for (const auto& row : results) {
        notPlayedEntities_[row.entity] = true;
    }
}

void BoidsSwarmSystem::SetPlaySpeed(float speed)
{
    speed = Math::Clamp(speed, MIN_PLAY_SPEED, MAX_PLAY_SPEED);
    playSpeed_ = speed;
}

void BoidsSwarmSystem::Initialize()
{
    ecs_.AddListener(boidsSwarmManager_, *this);
    ecs_.AddListener(boidsSwarmGravityManager_, *this);
    ecs_.AddListener(boidsSwarmRepulsionManager_, *this);

    const ComponentQuery::Operation operations[] = {
        {transformManager_, ComponentQuery::Operation::REQUIRE},
        {boidsSwarmStateManager_, ComponentQuery::Operation::REQUIRE},
    };
    swarmEntityQuery_.SetEcsListenersEnabled(true);
    swarmEntityQuery_.SetupQuery(boidsSwarmManager_, operations);

    const ComponentQuery::Operation gravityOperations[] = {
        {transformManager_, ComponentQuery::Operation::REQUIRE},
    };
    gravityEntityQuery_.SetEcsListenersEnabled(true);
    gravityEntityQuery_.SetupQuery(boidsSwarmGravityManager_, gravityOperations);

    const ComponentQuery::Operation repulsionOperations[] = {
        {transformManager_, ComponentQuery::Operation::REQUIRE},
    };
    repulsionEntityQuery_.SetEcsListenersEnabled(true);
    repulsionEntityQuery_.SetupQuery(boidsSwarmRepulsionManager_, repulsionOperations);
}

void BoidsSwarmSystem::Uninitialize()
{
    ecs_.RemoveListener(boidsSwarmManager_, *this);
    ecs_.RemoveListener(boidsSwarmGravityManager_, *this);
    ecs_.RemoveListener(boidsSwarmRepulsionManager_, *this);
    swarmEntityQuery_.SetEcsListenersEnabled(false);
    gravityEntityQuery_.SetEcsListenersEnabled(false);
    repulsionEntityQuery_.SetEcsListenersEnabled(false);
}

bool BoidsSwarmSystem::ValidateBoidsSwarmComponent(BoidsSwarmComponent& swarm)
{
    ValidateComponentHasMin(&swarm, boidsSwarmManager_.GetPropertyApi().MetaData());

    auto nanEq = [](float a, float b) -> bool { return (std::isnan(a) && std::isnan(b)) || (a == b); };
    bool changed = false;
    if (!nanEq(swarm.initialVelocity.x, swarm.prevInitialVelocity.x) ||
        !nanEq(swarm.initialVelocity.y, swarm.prevInitialVelocity.y) ||
        !nanEq(swarm.initialVelocity.z, swarm.prevInitialVelocity.z) ||
        !nanEq(swarm.initialPosition.x, swarm.prevInitialPosition.x) ||
        !nanEq(swarm.initialPosition.y, swarm.prevInitialPosition.y) ||
        !nanEq(swarm.initialPosition.z, swarm.prevInitialPosition.z) ||
        !nanEq(swarm.initialRotation.x, swarm.prevInitialRotation.x) ||
        !nanEq(swarm.initialRotation.y, swarm.prevInitialRotation.y) ||
        !nanEq(swarm.initialRotation.z, swarm.prevInitialRotation.z) ||
        !nanEq(swarm.initialRotation.w, swarm.prevInitialRotation.w)) {
        changed = true;
    }
    swarm.prevInitialVelocity = swarm.initialVelocity;
    swarm.prevInitialPosition = swarm.initialPosition;
    swarm.prevInitialRotation = swarm.initialRotation;
    return changed;
}

void BoidsSwarmSystem::ResetBoid(
    const BoidsSwarmComponent& swarm, CORE3D_NS::TransformComponent& transform, BoidsSwarmStateComponent& state)
{
    swarm >> transform;

    for (size_t i = 0; i < VELOCITY_COUNT; ++i) {
        state.velocities[i] = swarm.initialVelocity;
    }
    state.velocityMag = Math::Magnitude(swarm.initialVelocity);
}

void BoidsSwarmSystem::Reset()
{
    for (const auto& entry : notPlayedEntities_) {
        const auto& entity = entry.first;
        auto swarmComp = boidsSwarmManager_.Write(entity);
        const auto transformComp = transformManager_.Write(entity);
        const auto stateComp = boidsSwarmStateManager_.Write(entity);

        ResetBoid(*swarmComp, *transformComp, *stateComp);

        swarmComp->prevInitialVelocity = swarmComp->initialVelocity;
        swarmComp->prevInitialPosition = swarmComp->initialPosition;
        swarmComp->prevInitialRotation = swarmComp->initialRotation;
    }
}

void BoidsSwarmSystem::PreRun()
{
    using SwarmEntity = QueryRowIndices::SwarmEntity;

#if (BOIDSSWARM_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("BOIDSSWARM", "BoidsSwarmSystem", "PreRun", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif
    swarmEntityQuery_.Execute();
    gravityEntityQuery_.Execute();
    repulsionEntityQuery_.Execute();

    const auto& results = swarmEntityQuery_.GetResults();
    const size_t count = results.size();

    positions_.resize(count);
    frameDatas_.resize(count);
    entity2indices_.clear();
    for (size_t i = 0; i < count; ++i) {
        const auto& row = results[i];
        const uint32_t swarmCompId = row.components[GetQueryRowIndex(SwarmEntity::BOIDS_SWARM_COMP)];
        const uint32_t transformCompId = row.components[GetQueryRowIndex(SwarmEntity::TRANSFORM_COMP)];
        const uint32_t stateCompId = row.components[GetQueryRowIndex(SwarmEntity::BOIDS_SWARM_STATE_COMP)];
        const auto swarmComp = boidsSwarmManager_.Read(swarmCompId);
        const auto transformComp = transformManager_.Read(transformCompId);
        const auto stateComp = boidsSwarmStateManager_.Read(stateCompId);

        positions_[i] = transformComp->position;
        frameDatas_[i] << std::make_tuple(*transformComp, *swarmComp, *stateComp);

        entity2indices_[row.entity] = i;
    }
}

void BoidsSwarmSystem::Run()
{
#if (BOIDSSWARM_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("BOIDSSWARM", "BoidsSwarmSystem", "Run", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif

    auto computeNeighborForces = [&](size_t i) {
#if (BOIDSSWARM_DEV_ENABLED == 1)
        CORE_CPU_PERF_SCOPE(
            "BOIDSSWARM", "BoidsSwarmSystem", "ComputeNeighborForces", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif
        auto& frameData = frameDatas_[i];
        const size_t count = frameDatas_.size();

        auto swarmCompId = swarmEntityQuery_.GetResults()[i]
                               .components[GetQueryRowIndex(QueryRowIndices::SwarmEntity::BOIDS_SWARM_COMP)];
        auto swarmComp = boidsSwarmManager_.Read(swarmCompId);

        const Math::Vec3& position = positions_[i];
        const Math::Vec3 myPosProjected = position * axisMaskFloat_;

        const float separationDistSq = frameData.separationDistance * frameData.separationDistance;
        const float alignmentDistSq = frameData.alignmentDistance * frameData.alignmentDistance;
        const float cohesionDistSq = frameData.cohesionDistance * frameData.cohesionDistance;

        size_t separationCount = 0;
        size_t alignmentCount = 0;
        size_t cohesionCount = 0;

        auto processNeighbors = [&](const auto& targets, float distSqThreshold, auto&& processFunc) {
            if (!targets.empty()) {
                for (const auto& target : targets) {
                    auto it = entity2indices_.find(target);
                    if (it == entity2indices_.end()) {
                        continue;
                    }
                    size_t j = it->second;
                    const Math::Vec3 otherPosProjected = positions_[j] * axisMaskFloat_;
                    const Math::Vec3 delta = otherPosProjected - myPosProjected;
                    const float distSq = Math::Dot(delta, delta);
                    // specified targets will ignore distance threshold
                    processFunc(delta, distSq, j);
                }
            } else {
                for (size_t j = 0; j < count; ++j) {
                    if (i == j) {
                        continue;
                    }
                    const Math::Vec3 otherPosProjected = positions_[j] * axisMaskFloat_;
                    const Math::Vec3 delta = otherPosProjected - myPosProjected;
                    const float distSq = Math::Dot(delta, delta);
                    if (distSq <= distSqThreshold) {
                        processFunc(delta, distSq, j);
                    }
                }
            }
        };

        processNeighbors(
            swarmComp->separationTargets, separationDistSq, [&](const Math::Vec3& delta, float distSq, size_t j) {
                const float k = separationDistSq > Math::EPSILON ? 1.f - distSq / separationDistSq : 1.f;
                frameData.separationForce += -Normalize(delta) * k;
                ++separationCount;
            });

        processNeighbors(
            swarmComp->alignmentTargets, alignmentDistSq, [&](const Math::Vec3& delta, float distSq, size_t j) {
                frameData.alignmentForce += frameDatas_[j].forward;
                ++alignmentCount;
            });

        processNeighbors(
            swarmComp->cohesionTargets, cohesionDistSq, [&](const Math::Vec3& delta, float distSq, size_t j) {
                frameData.cohesionForce += positions_[j] * axisMaskFloat_;
                ++cohesionCount;
            });

        if (separationCount > 0) {
            frameData.separationForce *= frameData.maxAccelerationMag;
            frameData.separationForce = ClampMagnitude(frameData.separationForce, frameData.maxAccelerationMag);
            frameData.totalWeight += frameData.separationWeight;
        }
        if (alignmentCount > 0) {
            frameData.alignmentForce =
                ClampMagnitude(frameData.alignmentForce / alignmentCount, frameData.maxAccelerationMag);
            frameData.totalWeight += frameData.alignmentWeight;
        }
        if (cohesionCount > 0) {
            Math::Vec3 target = (frameData.cohesionForce / cohesionCount) - myPosProjected;
            frameData.cohesionForce =
                ClampMagnitude(target - frameData.velocities[VELOCITY_CURRENT_INDEX], frameData.maxAccelerationMag);
            frameData.totalWeight += frameData.cohesionWeight;
        }
    };

    auto computeFieldForces = [&](size_t i, auto field) -> Math::Vec3 {
#if (BOIDSSWARM_DEV_ENABLED == 1)
        CORE_CPU_PERF_SCOPE("BOIDSSWARM", "BoidsSwarmSystem", "ComputeFieldForces", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif
        auto& frameData = frameDatas_[i];
        const Math::Vec3& position = positions_[i];

        Math::Vec3 fieldForce = Math::ZERO_VEC3;
        auto processField = [&](const auto& query,
                                auto& manager,
                                auto compIndexEnum,
                                auto transformCompIndexEnum,
                                int directionSign,
                                float BoidsSwarmFrameData::*weightMember) {
            uint32_t fieldCount = 0;

            const auto& results = query.GetResults();
            for (const auto& row : results) {
                const uint32_t compId = row.components[GetQueryRowIndex(compIndexEnum)];
                const uint32_t transformCompId = row.components[GetQueryRowIndex(transformCompIndexEnum)];
                const auto comp = manager.Read(compId);
                const auto transformComp = transformManager_.Read(transformCompId);

                const Math::Vec3 delta = (transformComp->position - position) * axisMaskFloat_;
                const float distSq = Math::Dot(delta, delta);
                const float radiusSq = comp->radius * comp->radius;

                if (distSq <= radiusSq && distSq >= 0.f && comp->radius > 0.f) {
                    float falloff = Math::sqrt(distSq);
                    falloff = 1.0f - (falloff / comp->radius);
                    falloff *= falloff;
                    fieldForce += Math::Normalize(delta) * comp->accelerationMag * falloff * directionSign;
                    ++fieldCount;
                }
            }
            if (fieldCount > 0) {
                frameData.totalWeight += frameData.*weightMember;
            }
        };

        if constexpr (is_same_v<decltype(field), GravityField>) {
            processField(gravityEntityQuery_,
                boidsSwarmGravityManager_,
                QueryRowIndices::GravityEntity::BOIDS_SWARM_GRAVITY_COMP,
                QueryRowIndices::GravityEntity::TRANSFORM_COMP,
                +1,
                &BoidsSwarmFrameData::gravityWeight);
        } else if constexpr (is_same_v<decltype(field), RepulsionField>) {
            processField(repulsionEntityQuery_,
                boidsSwarmRepulsionManager_,
                QueryRowIndices::RepulsionEntity::BOIDS_SWARM_REPULSION_COMP,
                QueryRowIndices::RepulsionEntity::TRANSFORM_COMP,
                -1,
                &BoidsSwarmFrameData::repulsionWeight);
        }

        return ClampMagnitude(fieldForce, frameData.maxAccelerationMag);
    };

    auto computeBoundaryForce = [&](size_t i) -> Math::Vec3 {
#if (BOIDSSWARM_DEV_ENABLED == 1)
        CORE_CPU_PERF_SCOPE(
            "BOIDSSWARM", "BoidsSwarmSystem", "ComputeBoundaryForce", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif
        auto& frameData = frameDatas_[i];
        const Math::Vec3& position = positions_[i];

        Math::Vec3 boundaryForce = Math::ZERO_VEC3;
        uint32_t boundaryCount = 0;

        for (size_t xyz = 0; xyz < 3; ++xyz) {
            if (frameData.boundaryMinPos[xyz] >= frameData.boundaryMaxPos[xyz]) {
                return boundaryForce;
            }
        }

        if (frameData.boundaryWeight > 0.f && frameData.boundaryDistance >= 0.f) {
            const float margin = frameData.boundaryDistance;
            for (size_t xyz = 0; xyz < 3; ++xyz) {
                const float boundary = frameData.boundaryMinPos[xyz] + margin;
                if (axisMask_[xyz] == 0 || position[xyz] > boundary) {
                    continue;
                }
                boundaryForce[xyz] += (boundary - position[xyz]) / Math::max(margin, Math::EPSILON);
                ++boundaryCount;
            }
            for (size_t xyz = 0; xyz < 3; ++xyz) {
                const float boundary = frameData.boundaryMaxPos[xyz] - margin;
                if (axisMask_[xyz] == 0 || position[xyz] < boundary) {
                    continue;
                }
                boundaryForce[xyz] += (boundary - position[xyz]) / Math::max(margin, Math::EPSILON);
                ++boundaryCount;
            }
            boundaryForce *= frameData.maxAccelerationMag;
        }
        if (boundaryCount > 0) {
            frameData.totalWeight += frameData.boundaryWeight;
        }

        return boundaryForce;
    };

    size_t count = frameDatas_.size();

    for (size_t i = 0; i < count; ++i) {
        auto& frameData = frameDatas_[i];

        computeNeighborForces(i);
        frameData.gravityForce = computeFieldForces(i, GravityField());
        frameData.repulsionForce = computeFieldForces(i, RepulsionField());
        frameData.boundaryForce = computeBoundaryForce(i);

        frameData.drivenForce = ClampMagnitude(frameData.drivenForce, frameData.maxAccelerationMag);
        if (Math::Magnitude(frameData.drivenForce) > Math::EPSILON) {
            frameData.totalWeight += frameData.drivenForceWeight;
        }

        Math::Vec3 force =
            frameData.separationWeight * frameData.separationForce +
            frameData.alignmentWeight * frameData.alignmentForce + frameData.cohesionWeight * frameData.cohesionForce +
            frameData.boundaryWeight * frameData.boundaryForce + frameData.gravityWeight * frameData.gravityForce +
            frameData.repulsionWeight * frameData.repulsionForce + frameData.drivenForceWeight * frameData.drivenForce;
        if (frameData.totalWeight > Math::EPSILON) {
            force /= frameData.totalWeight;
        }
        force = ClampMagnitude(force, frameData.maxAccelerationMag);

        Math::Vec3& currVel = frameData.velocities[VELOCITY_CURRENT_INDEX];
        const Math::Vec3& prevVel = frameData.velocities[VELOCITY_CURRENT_INDEX + 1];
        currVel = prevVel + force * timeStepSec_;
        currVel = ClampMagnitude(currVel, frameData.maxVelocityMag);
    }
}

void BoidsSwarmSystem::PostRun()
{
#if (BOIDSSWARM_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("BOIDSSWARM", "BoidsSwarmSystem", "PostRun", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif
    const auto results = swarmEntityQuery_.GetResults();
    const size_t count = results.size();

    for (size_t i = 0; i < count; ++i) {
        using SwarmEntity = QueryRowIndices::SwarmEntity;

        auto& frameData = frameDatas_[i];
        Math::Vec3& position = positions_[i];

        const Math::Vec3 up = ComputeUpVector(axisMask_);
        LimitTurnRate(frameData, up);

        Math::Vec3 avgHistory = Math::ZERO_VEC3;
        for (size_t vi = VELOCITY_CURRENT_INDEX + 1; vi < VELOCITY_COUNT + 1; ++vi) {
            avgHistory += frameData.velocities[vi];
        }
        avgHistory /= static_cast<float>(VELOCITY_COUNT);
        frameData.velocities[VELOCITY_CURRENT_INDEX] =
            Math::Lerp(frameData.velocities[VELOCITY_CURRENT_INDEX], avgHistory, velocitySmoothingFactor_);

        // Separatedly set rotation here if velDirIsFwd
        if (float velMag = Math::Magnitude(frameData.velocities[VELOCITY_CURRENT_INDEX]);
            frameData.velDirIsFwd && velMag > Math::EPSILON) {
            frameData.rotation = Math::LookRotation(frameData.velocities[VELOCITY_CURRENT_INDEX], up);
        }

        positions_[i] += frameData.velocities[VELOCITY_CURRENT_INDEX] * timeStepSec_;

        const auto& row = results[i];
        const uint32_t transformCompId = row.components[GetQueryRowIndex(SwarmEntity::TRANSFORM_COMP)];
        const uint32_t stateCompId = row.components[GetQueryRowIndex(SwarmEntity::BOIDS_SWARM_STATE_COMP)];
        auto transformComp = transformManager_.Write(transformCompId);
        auto stateComp = boidsSwarmStateManager_.Write(stateCompId);

        frameData >> *stateComp;
        frameData >> *transformComp;
        transformComp->position = position;
    }
}

void BoidsSwarmSystem::LimitTurnRate(BoidsSwarmFrameData& frameData, const Math::Vec3& up)
{
#if (BOIDSSWARM_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("BOIDSSWARM", "BoidsSwarmSystem", "LimitTurnRate", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif
    Math::Vec3& currVel = frameData.velocities[VELOCITY_CURRENT_INDEX];
    const float currVelMag = Math::Magnitude(currVel);
    if (currVelMag <= Math::EPSILON) {
        return;
    }

    if (frameData.velDirIsFwd) {
        const Math::Quat curRot = Math::LookRotation(currVel / currVelMag, up);
        const Math::Vec3& prevVel = frameData.velocities[VELOCITY_CURRENT_INDEX + 1];
        const float prevVelMag = Math::Magnitude(prevVel);
        if (prevVelMag > Math::EPSILON) {
            const Math::Vec3 prevDir = prevVel / prevVelMag;
            Math::Quat clamped = ClampRotationDelta(Math::LookRotation(prevDir, up), curRot, frameData.maxTurnRate);
            currVel = clamped * Math::Vec3(0.f, 0.f, currVelMag);
            currVel *= axisMaskFloat_;  // avoid drifting on locked axes due to rotation
        }
    } else {
        frameData.rotation =
            ClampRotationDelta(frameData.rotation, Math::LookRotation(currVel, up), frameData.maxTurnRate);
    }
}

bool BoidsSwarmSystem::Update(bool frameRenderingQueued, uint64_t, uint64_t delta)
{
#if (BOIDSSWARM_DEV_ENABLED == 1)
    CORE_CPU_PERF_SCOPE("BOIDSSWARM", "BoidsSwarmSystem", "Update", BOIDSSWARM_PROFILER_DEFAULT_COLOR);
#endif
    if (!notPlayedEntities_.empty()) {
        Reset();
        notPlayedEntities_.clear();
        return true;
    }

    if (!active_) {
        return false;
    }

    accumulatedTime_ += delta;
    const uint64_t timeStepUs = static_cast<uint64_t>(timeStepSec_ / playSpeed_ * 1000000.0f);
    if (accumulatedTime_ < timeStepUs) {
        return false;
    }

    while (accumulatedTime_ >= timeStepUs) {
        PreRun();
        Run();
        PostRun();
        accumulatedTime_ -= timeStepUs;
    }

    return true;
}

void BoidsSwarmSystem::OnComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
    const CORE_NS::IComponentManager& componentManager, BASE_NS::array_view<const CORE_NS::Entity> entities)
{
    switch (type) {
        case CORE_NS::IEcs::ComponentListener::EventType::CREATED:
            if (&componentManager == static_cast<const CORE_NS::IComponentManager*>(&boidsSwarmManager_)) {
                for (const auto entity : entities) {
                    boidsSwarmStateManager_.Create(entity);
                    notPlayedEntities_[entity] = true;
                }
            }
            break;
        case CORE_NS::IEcs::ComponentListener::EventType::DESTROYED:
            if (&componentManager == static_cast<const CORE_NS::IComponentManager*>(&boidsSwarmManager_)) {
                for (const auto entity : entities) {
                    boidsSwarmStateManager_.Destroy(entity);
                    notPlayedEntities_.erase(entity);
                }
            }
            break;
        case CORE_NS::IEcs::ComponentListener::EventType::MODIFIED: {
            for (const auto entity : entities) {
                if (&componentManager == static_cast<const CORE_NS::IComponentManager*>(&boidsSwarmManager_)) {
                    auto swarmComp = boidsSwarmManager_.Write(entity);
                    bool initialPropChanged = ValidateBoidsSwarmComponent(*swarmComp);
                    if (swarmComp && initialPropChanged) {
                        notPlayedEntities_[entity] = true;
                    }
                } else if (&componentManager ==
                           static_cast<const CORE_NS::IComponentManager*>(&boidsSwarmGravityManager_)) {
                    auto gravComp = boidsSwarmGravityManager_.Write(entity);
                    if (gravComp) {
                        ValidateComponentHasMin(&*gravComp, boidsSwarmGravityManager_.GetPropertyApi().MetaData());
                    }
                } else if (&componentManager ==
                           static_cast<const CORE_NS::IComponentManager*>(&boidsSwarmRepulsionManager_)) {
                    auto repComp = boidsSwarmRepulsionManager_.Write(entity);
                    if (repComp) {
                        ValidateComponentHasMin(&*repComp, boidsSwarmRepulsionManager_.GetPropertyApi().MetaData());
                    }
                }
            }
            break;
        }
        default:
            break;
    }
}

CORE_NS::ISystem* IBoidsSwarmSystemInstance(CORE_NS::IEcs& ecs)
{
    return new BoidsSwarmSystem(ecs);
}

void IBoidsSwarmSystemDestroy(CORE_NS::ISystem* instance)
{
    delete static_cast<BoidsSwarmSystem*>(instance);
}

BOIDSSWARM_END_NAMESPACE()
