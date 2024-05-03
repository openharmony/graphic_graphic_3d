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

#ifndef CORE_ANIMATION_PLAYBACK_H
#define CORE_ANIMATION_PLAYBACK_H

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/systems/intf_animation_system.h>
#include <base/containers/array_view.h>
#include <core/ecs/entity.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
class IAnimationComponentManager;
class IAnimationStateComponentManager;
class INameComponentManager;

class AnimationPlayback final : public IAnimationPlayback {
public:
    AnimationPlayback(const CORE_NS::Entity animationEntity,
        const BASE_NS::array_view<const CORE_NS::Entity> targetEntities, CORE_NS::IEcs& ecs);
    ~AnimationPlayback() override;

    BASE_NS::string_view GetName() const override;

    void SetPlaybackState(AnimationComponent::PlaybackState state) override;
    AnimationComponent::PlaybackState GetPlaybackState() const override;

    void SetRepeatCount(uint32_t repeatCount) override;
    uint32_t GetRepeatCount() const override;

    void SetWeight(float weight) override;
    float GetWeight() const override;

    void SetTimePosition(float timePosition) override;
    float GetTimePosition() const override;

    float GetAnimationLength() const override;

    void SetStartOffset(float startOffset) override;
    float GetStartOffset() const override;

    void SetDuration(float duration) override;
    float GetDuration() const override;

    bool IsCompleted() const override;

    void SetSpeed(float speed) override;

    float GetSpeed() const override;

    CORE_NS::Entity GetEntity() const override;

private:
    // Animation resource.
    CORE_NS::Entity animation_;

    CORE_NS::IEcs& ecs_;

    IAnimationComponentManager* animationManager_ { nullptr };
    IAnimationStateComponentManager* animationStateManager_ { nullptr };
    INameComponentManager* nameManager_ { nullptr };
};
CORE3D_END_NAMESPACE()

#endif // CORE_ANIMATION_PLAYBACK_H
