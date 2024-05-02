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

#ifndef META_SRC_ANIMATION_STATE_H
#define META_SRC_ANIMATION_STATE_H

#include <optional>

#include <meta/api/container/find_cache.h>
#include <meta/ext/implementation_macros.h>
#include <meta/interface/animation/intf_animation.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/intf_any.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_container.h>
#include <meta/interface/intf_manual_clock.h>
#include <meta/interface/property/property_events.h>

#include "intf_animation_internal.h"

META_BEGIN_NAMESPACE()

namespace Internal {

/**
 * @brief Base animation state implementation, handles animation state, evaluation.
 */
class AnimationState {
public:
    AnimationState() = default;
    ~AnimationState() = default;
    META_NO_COPY_MOVE(AnimationState)

public:
    struct AnimationStateParams {
        /** Owning animation */
        IAnimation::WeakPtr owner;
        /** Writeable IAnimation::Running property */
        Property<bool> runningProperty;
        /** Writeable IAnimation::Progress property */
        Property<float> progressProperty;
        /** Writeable IAnimation::TotalDuration property */
        Property<TimeSpan> totalDuration;
    };

    /**
     * @brief Initialize AnimationState.
     * @param owner Owning animation for AnimationState.
     */
    virtual bool Initialize(AnimationStateParams&& params);
    /**
     * @brief Uninitializes the state object.
     */
    virtual void Uninitialize();
    /** Result of a step operation */
    struct StepStatus {
        IAnimationInternal::AnimationTargetState state { IAnimationInternal::AnimationTargetState::UNDEFINED };
        float progress {};
        bool changed {};

        constexpr bool StatusChanged() const noexcept
        {
            return changed;
        }
    };
    /**
     * @brief Step the state based on clock.
     * @param clock Clock containing the step time.
     */
    virtual StepStatus Step(const IClock::ConstPtr& clock);
    /**
     * @brief Seeks the animation state to given position [0,1]
     */
    virtual void Seek(float position);
    /**
     * @brief Pauses the animation. Returns true if state changed.
     */
    virtual bool Pause();
    /**
     * @brief Starts the animation. Returns true if state changed.
     */
    virtual bool Start();
    /**
     * @brief Stops the animation. Returns true if state changed.
     */
    virtual bool Stop();
    /**
     * @brief Restarts the animation. Returns true if state changed.
     */
    virtual bool Restart();
    /**
     * @brief Finishes the animation. Returns true if state changed.
     */
    virtual bool Finish();
    /**
     * @brief Returns true if the animation is running.
     */
    bool IsRunning() const noexcept;
    /**
     * @brief REturns true if the animation is paused.
     */
    bool IsPaused() const noexcept;
    /**
     * @brief Returns the current progress of the animation.
     */
    float GetProgress() const noexcept;
    /**
     * @brief Move the animation to a requested step position
     * @param step
     * @param state
     * @return
     */
    StepStatus Move(const IAnimationInternal::MoveParams& move);
    /**
     * @brief Resets to the animation clock to initial state, even if the animation is running.
     */
    void ResetClock();

public:
    BASE_NS::vector<IAnimationModifier::Ptr> GetModifiers() const;
    bool Attach(const IObject::Ptr& attachment, const IObject::Ptr& dataContext);
    bool Detach(const IObject::Ptr& attachment);

    virtual void UpdateTotalDuration();

protected:
    virtual TimeSpan GetAnimationBaseDuration() const;

protected:
    AnimationStateParams& GetParams()
    {
        return params_;
    }

    constexpr IAnimationInternal::AnimationTargetState GetAnimationTargetState() const noexcept
    {
        return state_.animationState_;
    }

    /** Return the owning animation of this state object */
    IAnimation::Ptr GetOwner() const noexcept;
    /** Returns a duration with all modifiers applied */
    IAnimationModifier::DurationData ApplyDurationModifiers(TimeSpan duration) const;
    /** Returns step data with all modifiers applied */
    IAnimationModifier::StepData ApplyStepModifiers(float progress) const;

private:
    void NotifyEvaluationNeeded() const;
    void NotifyStateChanged(const IAnimationInternal::AnimationStateChangedInfo& info) const;
    void UpdateController();
    bool SetState(IAnimationInternal::AnimationTargetState state);
    void SetRunning(bool running) noexcept;
    void SetProgress(float progress) noexcept;

private:
    IAnimationController::WeakPtr controller_;         // Animation's controller
    IOnChanged::InterfaceTypePtr updateTotalDuration_; // Callback function for total duration update
    AnimationStateParams params_;                      // Animation parameters
    struct StateData {
        int32_t loops { 1 };
        TimeSpan totalDuration;
        IAnimationModifier::DurationData duration;
        bool shouldInit_ { false };
        IManualClock::Ptr clock_;
        std::optional<META_NS::TimeSpan> lastTick_;
        IAnimationInternal::AnimationTargetState animationState_ { IAnimationInternal::AnimationTargetState::STOPPED };

        void ResetLastTick() noexcept
        {
            lastTick_.reset();
        }
        void SetTime(const TimeSpan& time)
        {
            clock_->SetTime(time);
        }
        TimeSpan Tick(const TimeSpan& time)
        {
            clock_->IncrementTime(GetElapsed(time));
            lastTick_ = time;
            return clock_->GetTime();
        }
        TimeSpan GetCurrentTime() const
        {
            return clock_->GetTime();
        }
        constexpr TimeSpan GetElapsed(const TimeSpan& time) const noexcept
        {
            return lastTick_ ? time - *lastTick_ : TimeSpan::Zero();
        }
        constexpr TimeSpan GetBaseDuration() const noexcept
        {
            return duration.duration;
        }

    } state_;                                             // State data
    mutable FindCache<IAnimationModifier> modifierCache_; // Cached modifier query
};

/**
 * @brief State class implementation for animation targeting a single property.
 */
class PropertyAnimationState : public AnimationState {
    using Super = AnimationState;

public:
    PropertyAnimationState() = default;
    ~PropertyAnimationState() = default;

    /**
     * @brief Data needed for evaluating an animation state.
     */
    struct EvaluationData {
        /** Target value */
        const IAny::Ptr target;
        /** Start of interpolation range */
        const IAny::Ptr from;
        /** End of interpolation range */
        const IAny::Ptr to;
        /** Linear animation progress [0,1] */
        float progress;
        /** The (optional) curve to use for transforming the progress value */
        const ICurve1D::Ptr curve;
        /** Returns true if the data is valid */
        inline bool IsValid() const noexcept
        {
            return target && from && to;
        }
    };
    /**
     * @brief Initialize state object's internal interpolator to support given type id.
     */
    bool SetInterpolator(const TypeId& id);
    /**
     * @brief Return the interpolator initialized with SetInterpolator.
     */
    IInterpolator::Ptr GetInterpolator() const;
    /**
     * @brief Evaluates target value between [from,to] based on state.
     * @param data Evaluation data.
     */
    AnyReturnValue EvaluateValue(const EvaluationData& data) const;

private:
    IInterpolator::Ptr interpolator_;
};

} // namespace Internal

META_END_NAMESPACE()

#endif // META_SRC_ANIMATION_STATE_H
