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

#ifndef META_SRC_ANIMATION_INTERNAL_H
#define META_SRC_ANIMATION_INTERNAL_H

#include <meta/interface/animation/intf_animation.h>

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAnimationInternal, "ab8878c9-9784-4949-84a4-2a7988ec1b80")

/**
 * @brief The IAnimationInternal interface defins the internal interface used between
 *        AnimationState and Animation implementations.
 */
class IAnimationInternal : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IAnimationInternal)
public:
    enum class AnimationTargetState : uint32_t {
        /** Undefined state */
        UNDEFINED = 0,
        /** The animation is running. Progress property is between [0,1[. */
        RUNNING = 1,
        /** The animation is stopped.
            If the animation has not been started, its Progress is 0.
            If the animation has finished running, its Progress is 1. */
        STOPPED = 2,
        /** Finish the animation. */
        FINISHED = 3,
        /** A previously RUNNING animation has been paused. Its progress is between [0,1[. */
        PAUSED = 5
    };
    /**
     * @brief The AnimationStateChangedInfo structure defines the parameters for an event which is invoked when
     *        animation state was changed.
     */
    struct AnimationStateChangedInfo {
        /** The animation whose state changed. */
        BASE_NS::weak_ptr<const IAnimation> source;
        /** The new state. */
        AnimationTargetState state;
        /** Previous state. */
        AnimationTargetState previous;
    };
    /**
     * @brief Resets the animation clock to initial state.
     */
    virtual void ResetClock() = 0;
    /** Parameters for move operation */
    struct MoveParams {
        /** Step information */
        IAnimationModifier::StepData step;
        /** Target state. If AnimationState::UNDEFINED, a suitable state shall be
            decided automatically by the Move() operation. */
        AnimationTargetState state { AnimationTargetState::UNDEFINED };
        /** Returns a MoveParams object with given progress */
        constexpr static MoveParams FromProgress(float progress) noexcept
        {
            MoveParams params;
            params.step = IAnimationModifier::StepData(progress);
            return params;
        }
        /** Returns a MoveParams object with given progress and target state */
        constexpr static MoveParams FromProgress(float progress, AnimationTargetState state) noexcept
        {
            MoveParams params;
            params.step = IAnimationModifier::StepData(progress);
            params.state = state;
            return params;
        }
    };
    /**
     * @brief Move the animation to given status..
     * @param move Move operation parameters.
     * @return True if the state of the animation changed in some way as a result of the operation. False otherwise.
     */
    virtual bool Move(const MoveParams& move) = 0;
    /**
     * @brief Invoked by AnimationState when animation state has changed.
     */
    virtual void OnAnimationStateChanged(const AnimationStateChangedInfo& info) = 0;
    /**
     * @brief Invoked by AnimationState when owning animation should re-evaluate itself.
     */
    virtual void OnEvaluationNeeded() = 0;
};

META_END_NAMESPACE()

#endif // META_INTF_ANIMATION_INTERNAL_H
