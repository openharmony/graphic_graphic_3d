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

#include "animation_playback.h"

#include <3d/ecs/components/animation_component.h>
#include <3d/ecs/components/animation_input_component.h>
#include <3d/ecs/components/animation_state_component.h>
#include <3d/ecs/components/animation_track_component.h>
#include <3d/ecs/components/name_component.h>
#include <base/containers/fixed_string.h>
#include <base/math/mathf.h>
#include <base/math/quaternion_util.h>
#include <base/math/spline.h>
#include <base/math/vector_util.h>
#include <base/util/uid_util.h>
#include <core/ecs/intf_ecs.h>
#include <core/implementation_uids.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace CORE_NS;

AnimationPlayback::AnimationPlayback(
    const Entity animationEntity, const array_view<const Entity> targetEntities, IEcs& ecs)
    : animation_(animationEntity), ecs_(ecs), animationManager_(GetManager<IAnimationComponentManager>(ecs)),
      animationStateManager_(GetManager<IAnimationStateComponentManager>(ecs)),
      nameManager_(GetManager<INameComponentManager>(ecs))
{}

AnimationPlayback::~AnimationPlayback()
{
    if (auto animMgr = GetManager<IAnimationComponentManager>(ecs_); animMgr) {
        if (auto animHandle = animMgr->Write(animation_); animHandle) {
            animHandle->state = AnimationComponent::PlaybackState::STOP;
        }
    }
}

string_view AnimationPlayback::GetName() const
{
    string_view name;
    if (const auto nameData = nameManager_->Read(animation_); nameData) {
        name = nameData->name;
    }
    return name;
}

void AnimationPlayback::SetPlaybackState(AnimationComponent::PlaybackState state)
{
    if (auto handle = animationManager_->Write(animation_); handle) {
        handle->state = state;
    }
}

AnimationComponent::PlaybackState AnimationPlayback::GetPlaybackState() const
{
    if (auto handle = animationManager_->Read(animation_); handle) {
        return handle->state;
    }
    return AnimationComponent::PlaybackState::STOP;
}

void AnimationPlayback::SetRepeatCount(uint32_t repeatCount)
{
    if (auto handle = animationManager_->Write(animation_); handle) {
        handle->repeatCount = repeatCount;
    }
}

uint32_t AnimationPlayback::GetRepeatCount() const
{
    if (auto handle = animationManager_->Read(animation_); handle) {
        return handle->repeatCount;
    }
    return 0U;
}

void AnimationPlayback::SetWeight(float weight)
{
    if (auto handle = animationManager_->Write(animation_); handle) {
        handle->weight = weight;
    }
}

float AnimationPlayback::GetWeight() const
{
    if (auto handle = animationManager_->Read(animation_); handle) {
        return handle->weight;
    }
    return 0.f;
}

float AnimationPlayback::GetTimePosition() const
{
    if (auto handle = animationStateManager_->Read(animation_); handle) {
        return handle->time;
    }
    return 0.f;
}

void AnimationPlayback::SetTimePosition(float timePosition)
{
    if (auto handle = animationStateManager_->Write(animation_); handle) {
        handle->time = timePosition;
    }
}

float AnimationPlayback::GetAnimationLength() const
{
    float maxLength = 0.f;
    if (auto handle = animationManager_->Read(animation_); handle) {
        auto trackManager = GetManager<IAnimationTrackComponentManager>(animationManager_->GetEcs());
        auto inputManager = GetManager<IAnimationInputComponentManager>(animationManager_->GetEcs());
        for (const auto& trackEntity : handle->tracks) {
            if (auto track = trackManager->Read(trackEntity); track) {
                if (auto inputData = inputManager->Read(track->timestamps); inputData) {
                    if (!inputData->timestamps.empty()) {
                        maxLength = Math::max(maxLength, inputData->timestamps.back());
                    }
                }
            }
        }
    }
    return maxLength;
}

void AnimationPlayback::SetStartOffset(float startOffset)
{
    if (auto handle = animationManager_->Write(animation_); handle) {
        handle->startOffset = startOffset;
    }
}

float AnimationPlayback::GetStartOffset() const
{
    if (auto handle = animationManager_->Read(animation_); handle) {
        return handle->startOffset;
    }
    return 0.f;
}

void AnimationPlayback::SetDuration(float duration)
{
    if (auto handle = animationManager_->Write(animation_); handle) {
        handle->duration = duration;
    }
}

float AnimationPlayback::GetDuration() const
{
    if (auto handle = animationManager_->Read(animation_); handle) {
        return handle->duration;
    }
    return 0.f;
}

bool AnimationPlayback::IsCompleted() const
{
    if (auto handle = animationStateManager_->Read(animation_); handle) {
        return handle->completed;
    }
    return true;
}

void AnimationPlayback::SetSpeed(float speed)
{
    if (auto handle = animationManager_->Write(animation_); handle) {
        handle->speed = speed;
    }
}

float AnimationPlayback::GetSpeed() const
{
    if (auto handle = animationManager_->Read(animation_); handle) {
        return handle->speed;
    }
    return 0.f;
}

Entity AnimationPlayback::GetEntity() const
{
    return animation_;
}
CORE3D_END_NAMESPACE()
