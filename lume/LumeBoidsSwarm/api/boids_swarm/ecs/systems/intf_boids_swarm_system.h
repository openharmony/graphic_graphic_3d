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

#ifndef BOIDS_SWARM_ECS_SYSTEMS_IBOIDS_SWARM_SYSTEM_H
#define BOIDS_SWARM_ECS_SYSTEMS_IBOIDS_SWARM_SYSTEM_H

#include <boids_swarm/namespace.h>

#include <base/containers/array_view.h>
#include <base/math/vector.h>
#include <core/ecs/intf_system.h>

BOIDSSWARM_BEGIN_NAMESPACE()

class IBoidsSwarmSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "c3d4e5f6-a7b8-4901-c234-56789012def0" };

    static constexpr float DEFAULT_TIME_STEP_SEC = 0.016f;
    static constexpr float DEFAULT_PLAY_SPEED = 1.f;
    static constexpr float MIN_PLAY_SPEED = 0.125f;
    static constexpr float MAX_PLAY_SPEED = 8.f;
    static constexpr BASE_NS::Math::IVec3 DEFAULT_AXIS_MASK { 1, 0, 1 };
    static constexpr float DEFAULT_VELOCITY_SMOOTHING_FACTOR = 0.5f;

    virtual void SetTimeStepSec(float timeStepSec) = 0;
    virtual float GetTimeStepSec() const = 0;

    virtual void SetAxisMask(const BASE_NS::Math::IVec3& axisMask) = 0;
    virtual BASE_NS::Math::IVec3 GetAxisMask() const = 0;

    virtual void Play() = 0;
    virtual void Pause() = 0;
    virtual void Stop() = 0;
    virtual bool IsPlaying() const = 0;
    virtual void SetPlaySpeed(float speed) = 0;
    virtual float GetPlaySpeed() const = 0;

    virtual void SetVelocitySmoothingFactor(float factor) = 0;
    virtual float GetVelocitySmoothingFactor() const = 0;

protected:
    IBoidsSwarmSystem() = default;
    IBoidsSwarmSystem(const IBoidsSwarmSystem&) = delete;
    IBoidsSwarmSystem(IBoidsSwarmSystem&&) = delete;
    IBoidsSwarmSystem& operator=(const IBoidsSwarmSystem&) = delete;
    IBoidsSwarmSystem& operator=(IBoidsSwarmSystem&&) = delete;
};

inline constexpr BASE_NS::string_view GetName(const IBoidsSwarmSystem*)
{
    return "BoidsSwarmSystem";
}

BOIDSSWARM_END_NAMESPACE()

#endif
