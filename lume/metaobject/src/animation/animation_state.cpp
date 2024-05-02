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

#include "animation_state.h"

#include <meta/api/make_callback.h>
#include <meta/api/util.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/builtin_objects.h>
#include <meta/interface/interface_macros.h>
#include <meta/interface/intf_attachment_container.h>
#include <meta/interface/property/property_events.h>

META_BEGIN_NAMESPACE()

namespace Internal {

bool AnimationState::Initialize(AnimationStateParams&& params)
{
    params_ = BASE_NS::move(params);
    auto owner = GetOwner();
    if (!owner) {
        CORE_LOG_F("AnimationState: Invalid target animation");
        return false;
    }
    auto controllerProp = owner->Controller();
    if (!controllerProp) {
        return false;
    }
    if (!controllerProp->GetValue().lock()) {
        // No controller set, init to default
        controllerProp->SetValue(META_NS::GetAnimationController());
    }

    updateTotalDuration_ = MakeCallback<IOnChanged>(this, &AnimationState::UpdateTotalDuration);
    if (auto timed = interface_cast<ITimedAnimation>(owner)) {
        timed->Duration()->OnChanged()->AddHandler(updateTotalDuration_, uintptr_t(this));
    }

    auto updateController = MakeCallback<IOnChanged>(this, &AnimationState::UpdateController);
    controllerProp->OnChanged()->AddHandler(updateController);
    state_.clock_ = META_NS::GetObjectRegistry().Create<META_NS::IManualClock>(META_NS::ClassId::ManualClock);
    CORE_ASSERT(state_.clock_);
    UpdateTotalDuration();
    UpdateController();
    return true;
}

void AnimationState::Uninitialize()
{
    if (auto timed = interface_pointer_cast<ITimedAnimation>(GetOwner())) {
        timed->Duration()->OnChanged()->RemoveHandler(uintptr_t(this));
    }
}

void AnimationState::NotifyEvaluationNeeded() const
{
    if (auto internal = interface_pointer_cast<IAnimationInternal>(GetOwner())) {
        internal->OnEvaluationNeeded();
    }
}

void AnimationState::NotifyStateChanged(const IAnimationInternal::AnimationStateChangedInfo& info) const
{
    if (auto internal = interface_pointer_cast<IAnimationInternal>(GetOwner())) {
        internal->OnAnimationStateChanged(info);
        // Need also evaluation
        internal->OnEvaluationNeeded();
    }
}

void AnimationState::UpdateController()
{
    IAnimation::Ptr animation = GetOwner();
    if (!animation) {
        CORE_LOG_E("Invalid target animation");
        return;
    }
    auto oldController = controller_.lock();
    auto newController = animation->Controller()->GetValue().lock();

    if (oldController != newController) {
        if (oldController) {
            oldController->RemoveAnimation(animation);
        }
        if (newController) {
            newController->AddAnimation(animation);
        }
        controller_ = newController;
    }
}

void AnimationState::ResetClock()
{
    state_.ResetLastTick();
    state_.SetTime(TimeSpan::Zero());
}

AnimationState::StepStatus AnimationState::Step(const IClock::ConstPtr& clock)
{
    if (!IsRunning()) {
        return {};
    }

    const auto time = clock ? clock->GetTime() : TimeSpan::Zero();
    float progress = static_cast<float>(state_.Tick(time).ToMilliseconds()) /
                     static_cast<float>(state_.GetBaseDuration().ToMilliseconds());

    return Move(IAnimationInternal::MoveParams::FromProgress(progress));
}

constexpr IAnimationInternal::AnimationTargetState GetTargetState(const IAnimationInternal::MoveParams& move) noexcept
{
    using AnimationTargetState = IAnimationInternal::AnimationTargetState;
    const auto& step = move.step;
    const auto& state = move.state;
    if (state == AnimationTargetState::UNDEFINED) {
        // Figure out target state based on step data automatically
        const float progress = step.progress;
        const bool reverse = step.reverse;
        if (progress >= 1.f) {
            return reverse ? AnimationTargetState::RUNNING : AnimationTargetState::FINISHED;
        }
        if (progress <= 0.f) {
            return reverse ? AnimationTargetState::FINISHED : AnimationTargetState::RUNNING;
        }
        return AnimationTargetState::RUNNING;
    }

    // Just go to the state defined by the caller
    return state;
}

AnimationState::StepStatus AnimationState::Move(const IAnimationInternal::MoveParams& move)
{
    using AnimationTargetState = IAnimationInternal::AnimationTargetState;
    auto animationState = GetTargetState(move);
    const auto& step = move.step;
    float progress = step.progress;

    if (state_.shouldInit_) {
        state_.loops = state_.duration.loopCount;
        state_.shouldInit_ = false;
    }

    if (animationState == AnimationTargetState::FINISHED) {
        // Check if we need to loop
        if (state_.loops && (state_.loops < 0 || --state_.loops)) {
            animationState = AnimationTargetState::RUNNING;
            const auto overflow = progress - BASE_NS::Math::floor(progress);
            state_.SetTime(overflow * state_.GetBaseDuration());
            if (overflow > 0.f) {
                // If progress based on clock would be e.g. 1.2, jump to 0.2 to not jank the animation
                progress = overflow;
            }
        }
    }

    AnimationState::StepStatus status;
    if (progress = BASE_NS::Math::clamp01(progress); progress != GetProgress()) {
        SetProgress(progress);
        status.changed = true;
    }

    status.changed |= SetState(animationState);
    status.state = state_.animationState_;
    status.progress = GetProgress();
    if (status.changed) {
        NotifyEvaluationNeeded();
    }
    return status;
}

void AnimationState::Seek(float position)
{
    auto animation = GetOwner();
    if (!animation) {
        CORE_LOG_E("Invalid target animation");
        return;
    }

    position = BASE_NS::Math::clamp01(position);
    state_.ResetLastTick();
    const auto seekedTime = state_.GetBaseDuration().ToSecondsFloat() * position;
    state_.SetTime(TimeSpan::Seconds(seekedTime));
    auto state = state_.animationState_;
    if (position >= 1.f) {
        state = IAnimationInternal::AnimationTargetState::FINISHED;
    }
    Move(IAnimationInternal::MoveParams::FromProgress(position, state));
}

bool AnimationState::Pause()
{
    return SetState(IAnimationInternal::AnimationTargetState::PAUSED);
}

bool AnimationState::Start()
{
    return SetState(IAnimationInternal::AnimationTargetState::RUNNING);
}

bool AnimationState::Stop()
{
    return Move(IAnimationInternal::MoveParams::FromProgress(0.f, IAnimationInternal::AnimationTargetState::STOPPED))
        .StatusChanged();
}

bool AnimationState::Finish()
{
    return Move(IAnimationInternal::MoveParams::FromProgress(1.f, IAnimationInternal::AnimationTargetState::FINISHED))
        .StatusChanged();
}

bool AnimationState::Restart()
{
    if (Stop()) {
        return Start();
    }
    return false;
}

IAnimation::Ptr AnimationState::GetOwner() const noexcept
{
    return params_.owner.lock();
}

bool AnimationState::IsRunning() const noexcept
{
    return GetValue(params_.runningProperty, false);
}

bool AnimationState::IsPaused() const noexcept
{
    return state_.animationState_ == IAnimationInternal::AnimationTargetState::PAUSED;
}

void AnimationState::SetRunning(bool running) noexcept
{
    SetValue(params_.runningProperty, running);
}

float AnimationState::GetProgress() const noexcept
{
    return GetValue(params_.progressProperty, 0.f);
}

void AnimationState::SetProgress(float progress) noexcept
{
    SetValue(params_.progressProperty, progress);
}

bool AnimationState::SetState(IAnimationInternal::AnimationTargetState state)
{
    using AnimationTargetState = IAnimationInternal::AnimationTargetState;
    const auto previous = state_.animationState_;
    if (previous == state) {
        return false;
    }
    if (const auto owner = GetOwner()) {
        bool notifyStarted = false;
        switch (state) {
            case AnimationTargetState::RUNNING:
                if (previous != AnimationTargetState::PAUSED) {
                    state_.shouldInit_ = true;
                    notifyStarted = true;
                    if (previous == AnimationTargetState::FINISHED) {
                        SetProgress(0.f);
                        ResetClock();
                    }
                }
                break;
            case AnimationTargetState::PAUSED:
                state_.ResetLastTick();
                break;
            case AnimationTargetState::FINISHED:
                [[fallthrough]];
            case AnimationTargetState::STOPPED:
                ResetClock();
                break;
            default:
                CORE_LOG_E("Invalid target state for animation: AnimationTargetState::UNDEFINED");
                ResetClock();
                break;
        }
        SetRunning(state == AnimationTargetState::RUNNING);

        state_.animationState_ = state;
        IAnimationInternal::AnimationStateChangedInfo info;
        info.source = GetOwner();
        info.state = state;
        info.previous = previous;
        NotifyStateChanged(info);

        if (state == AnimationTargetState::FINISHED) {
            Invoke<IOnChanged>(owner->OnFinished());
        }
        if (notifyStarted) {
            Invoke<IOnChanged>(owner->OnStarted());
        }
        return true;
    }
    return false;
}

BASE_NS::vector<IAnimationModifier::Ptr> AnimationState::GetModifiers() const
{
    if (!modifierCache_.HasTarget()) {
        // Do not create an attachment container unless one has already been created by someone
        if (const auto attach = interface_pointer_cast<IAttach>(params_.owner)) {
            if (const auto container = attach->GetAttachmentContainer(false)) {
                modifierCache_.SetTarget(
                    container, { "", TraversalType::NO_HIERARCHY, { IAnimationModifier::UID }, true });
            }
        }
    }

    return modifierCache_.FindAll();
}

IAnimationModifier::StepData AnimationState::ApplyStepModifiers(float progress) const
{
    IAnimationModifier::StepData step(progress);
    for (auto&& mod : GetModifiers()) {
        mod->ProcessOnStep(step);
    }
    return step;
}

IAnimationModifier::DurationData AnimationState::ApplyDurationModifiers(TimeSpan duration) const
{
    using DurationData = IAnimationModifier::DurationData;
    DurationData durationData;
    durationData.duration = duration;
    for (auto&& mod : GetModifiers()) {
        DurationData data = durationData;
        if (mod->ProcessOnGetDuration(data)) {
            durationData = data;
        }
    }
    return durationData;
}

TimeSpan AnimationState::GetAnimationBaseDuration() const
{
    if (auto timed = interface_cast<ITimedAnimation>(GetOwner())) {
        return GetValue(timed->Duration());
    }
    CORE_LOG_W("Cannot update total duration of an animation that does not implement ITimedAnimation");
    return TimeSpan::Zero();
}

void AnimationState::UpdateTotalDuration()
{
    if (!params_.totalDuration) {
        return;
    }
    auto durationData = ApplyDurationModifiers(GetAnimationBaseDuration());
    state_.duration = durationData;
    state_.totalDuration =
        durationData.loopCount > 0 ? durationData.duration * durationData.loopCount : TimeSpan::Infinite();
    SetValue(params_.totalDuration, state_.totalDuration);
}

bool AnimationState::Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext)
{
    bool success = false;
    if (auto attach = interface_pointer_cast<IAttach>(params_.owner)) {
        if (const auto attachments = interface_cast<IAttachmentContainer>(attach->GetAttachmentContainer(true))) {
            if (const auto modifier = interface_cast<IAnimationModifier>(attachment)) {
                if (success = attachments->Attach(attachment, dataContext); success) {
                    if (auto notifyChanged = interface_cast<INotifyOnChange>(modifier)) {
                        notifyChanged->OnChanged()->AddHandler(updateTotalDuration_, uintptr_t(this));
                    }
                }
                UpdateTotalDuration();
            } else {
                // Attaching something else than a modifier
                return attachments->Attach(attachment, dataContext);
            }
        }
    }
    return success;
}

bool AnimationState::Detach(const IObject::Ptr& attachment)
{
    bool success = false;
    if (auto attach = interface_pointer_cast<IAttach>(params_.owner)) {
        success = attach->Detach(attachment);
        if (const auto modifier = interface_cast<IAnimationModifier>(attachment)) {
            if (auto notifyChanged = interface_cast<INotifyOnChange>(modifier)) {
                notifyChanged->OnChanged()->RemoveHandler(uintptr_t(this));
            }
        }
        UpdateTotalDuration();
    }
    return success;
}

// PropertyAnimationState

IInterpolator::Ptr PropertyAnimationState::GetInterpolator() const
{
    return interpolator_;
}

bool PropertyAnimationState::SetInterpolator(const TypeId& id)
{
    interpolator_ = id != TypeId {} ? GetObjectRegistry().CreateInterpolator(id) : nullptr;
    return interpolator_ != nullptr;
}

AnyReturnValue PropertyAnimationState::EvaluateValue(const EvaluationData& data) const
{
    if (!data.IsValid()) {
        return AnyReturn::FAIL;
    }

    auto step = ApplyStepModifiers(data.progress);
    auto progress = step.progress;

    if (progress <= 0.f) {
        return data.target->CopyFrom(*data.from);
    }
    if (progress >= 1.f) {
        return data.target->CopyFrom(*data.to);
    }
    if (data.curve) {
        progress = data.curve->Transform(progress);
    }
    if (interpolator_) {
        return interpolator_->Interpolate(*data.target, *data.from, *data.to, progress);
    }
    CORE_LOG_W("No interpolator set for animation state");
    return AnyReturn::FAIL;
}

} // namespace Internal

META_END_NAMESPACE()
