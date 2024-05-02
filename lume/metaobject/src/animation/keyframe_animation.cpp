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

#include "keyframe_animation.h"

#include <meta/api/util.h>

META_BEGIN_NAMESPACE()

namespace Internal {

AnimationState::AnimationStateParams KeyframeAnimation::GetParams()
{
    AnimationState::AnimationStateParams params;
    params.owner = GetSelf<IAnimation>();
    params.runningProperty = META_ACCESS_PROPERTY(Running);
    params.progressProperty = META_ACCESS_PROPERTY(Progress);
    params.totalDuration = META_ACCESS_PROPERTY(TotalDuration);
    return params;
}

void KeyframeAnimation::Initialize()
{
    GetState().ResetClock();
    currentValue_.reset();
}

void KeyframeAnimation::OnAnimationStateChanged(const AnimationStateChangedInfo& info)
{
    using AnimationTargetState = IAnimationInternal::AnimationTargetState;
    if (auto p = GetTargetProperty()) {
        switch (info.state) {
            case AnimationTargetState::FINISHED:
                [[fallthrough]];
            case AnimationTargetState::STOPPED:
                if (currentValue_) {
                    // Evaluate current value
                    Evaluate();
                    // Then set the correct keyframe value to the underlying property
                    p.property->SetValue(*currentValue_);
                }
                break;
            case AnimationTargetState::RUNNING:
                currentValue_ = p.property->GetValue().Clone(false);
                // Evaluate current value
                Evaluate();
                break;
            default:
                break;
        }
    }
}

void KeyframeAnimation::Start()
{
    GetState().Start();
}

void KeyframeAnimation::Evaluate()
{
    const PropertyAnimationState::EvaluationData data { currentValue_, META_ACCESS_PROPERTY_VALUE(From),
        META_ACCESS_PROPERTY_VALUE(To), META_ACCESS_PROPERTY_VALUE(Progress), META_ACCESS_PROPERTY_VALUE(Curve) };
    if (GetState().EvaluateValue(data) == AnyReturn::SUCCESS) {
        NotifyChanged();
    }
}

void KeyframeAnimation::Stop()
{
    GetState().Stop();
}

void KeyframeAnimation::Pause()
{
    GetState().Pause();
}

void KeyframeAnimation::Restart()
{
    GetState().Restart();
}

void KeyframeAnimation::Finish()
{
    GetState().Finish();
}

void KeyframeAnimation::Seek(float position)
{
    GetState().Seek(position);
}

void KeyframeAnimation::OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous)
{
    auto me = GetSelf<IModifier>();
    if (previous) {
        previous->RemoveModifier(me);
        if (auto p = interface_cast<IProperty>(previous)) {
            p->SetValue(*currentValue_);
        }
        Stop();
    }
    if (property) {
        property.stack->AddModifier(me);
    }
    Initialize();
}

EvaluationResult KeyframeAnimation::ProcessOnGet(IAny& value)
{
    if (currentValue_ && (GetState().IsRunning() || GetState().IsPaused())) {
        if (auto result = value.CopyFrom(*currentValue_)) {
            return result == AnyReturn::NOTHING_TO_DO ? EvaluationResult::EVAL_CONTINUE
                                                      : EvaluationResult::EVAL_VALUE_CHANGED;
        }
    }
    return EvaluationResult::EVAL_CONTINUE;
}

} // namespace Internal
META_END_NAMESPACE()
