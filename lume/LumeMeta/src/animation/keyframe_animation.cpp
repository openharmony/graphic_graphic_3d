/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Keyframe animation implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
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
                    PropertyLock lock { p.property };
                    lock->SetValueAny(*currentValue_);
                }
                break;
            case AnimationTargetState::RUNNING: {
                PropertyLock lock { p.property };
                currentValue_ = lock->GetValueAny().Clone(false);
            }
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
        if (auto prop = GetTargetProperty()) {
            PropertyLock lock { prop.property };
            prop.stack->EvaluateAndStore();
        }
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
