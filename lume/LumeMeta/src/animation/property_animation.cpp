/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
 * Description: Property animation implementations
 * Author: Lauri Jaaskela
 * Create: 2023-12-20
 */

#include "property_animation.h"

#include <meta/api/make_callback.h>
#include <meta/interface/intf_any.h>

META_BEGIN_NAMESPACE()

namespace Internal {

bool PropertyAnimation::Build(const IMetadata::Ptr& meta)
{
    return Super::Build(meta);
}

void PropertyAnimation::Step(const IClock::ConstPtr& clock)
{
    Super::Step(clock);
}

void PropertyAnimation::OnAnimationStateChanged(const IAnimationInternal::AnimationStateChangedInfo& info)
{
    if (auto p = GetTargetProperty()) {
        switch (info.state) {
            case AnimationTargetState::FINISHED:
                [[fallthrough]];
            case AnimationTargetState::STOPPED:
                // Evaluate current value
                Evaluate();
                break;
            case AnimationTargetState::RUNNING:
                break;
            default:
                break;
        }
    }
}

void PropertyAnimation::Evaluate()
{
    const PropertyAnimationState::EvaluationData data { currentValue_, from_, to_, META_ACCESS_PROPERTY_VALUE(Progress),
        META_ACCESS_PROPERTY_VALUE(Curve) };
    if (GetState().EvaluateValue(data) == AnyReturn::SUCCESS) {
        evalChanged_ = true;
        NotifyChanged();
        if (auto prop = GetTargetProperty()) {
            PropertyLock lock { prop.property };
            prop.stack->EvaluateAndStore();
        }
    }
}

void PropertyAnimation::OnPropertyChanged(const TargetProperty& property, const IStackProperty::Ptr& previous)
{
    auto me = GetSelf<IModifier>();
    if (previous) {
        previous->RemoveModifier(me);
    }
    if (property) {
        property.stack->AddModifier(me);
    }
}

EvaluationResult PropertyAnimation::ProcessOnGet(IAny& value)
{
    if (currentValue_ && (evalChanged_ || GetState().IsRunning())) {
        evalChanged_ = false;
        if (auto result = value.CopyFrom(*currentValue_)) {
            return result == AnyReturn::NOTHING_TO_DO ? EvaluationResult::EVAL_CONTINUE
                                                      : EvaluationResult::EVAL_VALUE_CHANGED;
        }
    }

    return EvaluationResult::EVAL_CONTINUE;
}

EvaluationResult PropertyAnimation::ProcessOnSet(IAny& value, const IAny& current)
{
    if (META_ACCESS_PROPERTY_VALUE(Valid) || META_ACCESS_PROPERTY_VALUE(Enabled)) {
        from_ = current.Clone();
        to_ = value.Clone();
        currentValue_ = current.Clone();
        auto& state = GetState();
        // Start animating
        state.Start();
        // Propagate initial value
        value.CopyFrom(*from_);
    }
    return EvaluationResult::EVAL_CONTINUE;
}

AnimationState::AnimationStateParams PropertyAnimation::GetParams()
{
    AnimationState::AnimationStateParams params;
    params.owner = GetSelf<IAnimation>();
    params.runningProperty = META_ACCESS_PROPERTY(Running);
    params.progressProperty = META_ACCESS_PROPERTY(Progress);
    params.totalDuration = META_ACCESS_PROPERTY(TotalDuration);
    return params;
}

} // namespace Internal
META_END_NAMESPACE()
