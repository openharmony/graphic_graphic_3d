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

#ifndef BOIDS_SWARM_ECS_SYSTEMS_BOIDS_SWARM_SYSTEM_H
#define BOIDS_SWARM_ECS_SYSTEMS_BOIDS_SWARM_SYSTEM_H

#include <ComponentTools/component_query.h>
#include <boids_swarm/ecs/components/boids_swarm_component.h>
#include <boids_swarm/ecs/components/boids_swarm_gravity_component.h>
#include <boids_swarm/ecs/components/boids_swarm_repulsion_component.h>
#include <boids_swarm/ecs/components/boids_swarm_state_component.h>
#include <boids_swarm/ecs/systems/intf_boids_swarm_system.h>
#include <boids_swarm/namespace.h>
#include <random>
#include <unordered_set>

#include <3d/ecs/components/transform_component.h>
#include <base/containers/array_view.h>
#include <base/containers/unordered_map.h>
#include <base/containers/vector.h>
#include <core/ecs/intf_system.h>
#include <core/namespace.h>

CORE_BEGIN_NAMESPACE()
class IEcs;
CORE_END_NAMESPACE()

BOIDSSWARM_BEGIN_NAMESPACE()

class BoidsSwarmSystem final : public IBoidsSwarmSystem, CORE_NS::IEcs::ComponentListener {
public:
    static constexpr BASE_NS::Uid UID{"c3d4e5f6-a7b8-4901-c234-56789012def0"};

    struct BoidsSwarmFrameData {
        BASE_NS::Math::Quat rotation;
        BASE_NS::Math::Vec3 forward;
        BASE_NS::Math::Vec3 separationForce;
        BASE_NS::Math::Vec3 alignmentForce;
        BASE_NS::Math::Vec3 cohesionForce;
        BASE_NS::Math::Vec3 boundaryForce;
        BASE_NS::Math::Vec3 gravityForce;
        BASE_NS::Math::Vec3 repulsionForce;
        BASE_NS::Math::Vec3 drivenForce;
        BASE_NS::Math::Vec3 boundaryMinPos;
        BASE_NS::Math::Vec3 boundaryMaxPos;
        BASE_NS::Math::Vec3 maxTurnRate;
        BASE_NS::Math::Vec3 velocities[BoidsSwarmStateComponent::VELOCITY_COUNT + 1];
        float totalWeight;
        float separationDistance;
        float alignmentDistance;
        float cohesionDistance;
        float separationWeight;
        float alignmentWeight;
        float cohesionWeight;
        float maxVelocityMag;
        float maxAccelerationMag;
        float boundaryDistance;
        float boundaryWeight;
        float gravityWeight;
        float repulsionWeight;
        float drivenForceWeight;
        bool velDirIsFwd;
    };

    explicit BoidsSwarmSystem(CORE_NS::IEcs& ecs);
    ~BoidsSwarmSystem() override = default;

    BASE_NS::string_view GetName() const override;
    BASE_NS::Uid GetUid() const override;

    CORE_NS::IPropertyHandle* GetProperties() override;
    const CORE_NS::IPropertyHandle* GetProperties() const override;
    void SetProperties(const CORE_NS::IPropertyHandle&) override;

    bool IsActive() const override;
    void SetActive(bool state) override;

    void Initialize() override;
    bool Update(bool frameRenderingQueued, uint64_t time, uint64_t delta) override;
    void Uninitialize() override;

    const CORE_NS::IEcs& GetECS() const override;

    void SetTimeStepSec(float timeStepSec) override;
    float GetTimeStepSec() const override;

    void SetAxisMask(const BASE_NS::Math::IVec3& axisMask) override;
    BASE_NS::Math::IVec3 GetAxisMask() const override;

    void Play() override;
    void Pause() override;
    void Stop() override;
    bool IsPlaying() const override
    {
        return active_;
    }
    void SetPlaySpeed(float speed) override;
    float GetPlaySpeed() const override
    {
        return playSpeed_;
    }

    void SetVelocitySmoothingFactor(float factor) override
    {
        velocitySmoothingFactor_ = factor;
    }

    float GetVelocitySmoothingFactor() const override
    {
        return velocitySmoothingFactor_;
    }

private:
    void ResetBoid(
        const BoidsSwarmComponent& swarm, CORE3D_NS::TransformComponent& transform, BoidsSwarmStateComponent& state);
    void Reset();
    bool ValidateBoidsSwarmComponent(BoidsSwarmComponent& swarm);

    void PreRun();
    void Run();
    void PostRun();
    void LimitTurnRate(BoidsSwarmFrameData& frameData, const BASE_NS::Math::Vec3& up);

    void OnComponentEvent(CORE_NS::IEcs::ComponentListener::EventType type,
        const CORE_NS::IComponentManager& componentManager,
        BASE_NS::array_view<const CORE_NS::Entity> entities) override;

    bool active_{true};
    float velocitySmoothingFactor_{DEFAULT_VELOCITY_SMOOTHING_FACTOR};
    CORE_NS::IEcs& ecs_;

    IBoidsSwarmComponentManager& boidsSwarmManager_;
    IBoidsSwarmGravityComponentManager& boidsSwarmGravityManager_;
    IBoidsSwarmRepulsionComponentManager& boidsSwarmRepulsionManager_;
    IBoidsSwarmStateComponentManager& boidsSwarmStateManager_;
    CORE3D_NS::ITransformComponentManager& transformManager_;

    CORE_NS::ComponentQuery swarmEntityQuery_;
    CORE_NS::ComponentQuery gravityEntityQuery_;
    CORE_NS::ComponentQuery repulsionEntityQuery_;

    BASE_NS::vector<BASE_NS::Math::Vec3> positions_;
    BASE_NS::vector<BoidsSwarmFrameData> frameDatas_;
    BASE_NS::unordered_map<CORE_NS::Entity, bool> notPlayedEntities_;
    BASE_NS::unordered_map<CORE_NS::Entity, size_t> entity2indices_;

    float timeStepSec_{DEFAULT_TIME_STEP_SEC};
    float playSpeed_{DEFAULT_PLAY_SPEED};
    uint64_t accumulatedTime_{0};

    BASE_NS::Math::IVec3 axisMask_{IBoidsSwarmSystem::DEFAULT_AXIS_MASK.x != 0 ? 1 : 0,
        IBoidsSwarmSystem::DEFAULT_AXIS_MASK.y != 0 ? 1 : 0,
        IBoidsSwarmSystem::DEFAULT_AXIS_MASK.z != 0 ? 1 : 0};
    BASE_NS::Math::Vec3 axisMaskFloat_{IBoidsSwarmSystem::DEFAULT_AXIS_MASK.x != 0 ? 1.f : 0.f,
        IBoidsSwarmSystem::DEFAULT_AXIS_MASK.y != 0 ? 1.f : 0.f,
        IBoidsSwarmSystem::DEFAULT_AXIS_MASK.z != 0 ? 1.f : 0.f};

    std::mt19937 randomEngine_;
};

inline constexpr BASE_NS::string_view GetName(const BoidsSwarmSystem*)
{
    return "BoidsSwarmSystem";
}

BOIDSSWARM_END_NAMESPACE()

#endif
