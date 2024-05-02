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

void PropertyAnimation::Evaluate()
{
    const PropertyAnimationState::EvaluationData data { currentValue_, from_, to_, META_ACCESS_PROPERTY_VALUE(Progress),
        META_ACCESS_PROPERTY_VALUE(Curve) };
    if (GetState().EvaluateValue(data) == AnyReturn::SUCCESS) {
        NotifyChanged();
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
    if (currentValue_ && GetState().IsRunning()) {
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
        // Temp shared_ptr<IAny> which does not delete our source IAny
        auto tmp = IAny::Ptr(&value, [](auto*) {});
        // Evaluate the animation with progress=1.f and pass the value down the property stack
        PropertyAnimationState::EvaluationData data { tmp, from_, to_, 1.f, META_ACCESS_PROPERTY_VALUE(Curve) };
        state.EvaluateValue(data);
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
