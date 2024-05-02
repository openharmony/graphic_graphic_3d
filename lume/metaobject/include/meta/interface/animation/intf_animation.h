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

#ifndef META_INTERFACE_INTF_ANIMATION_H
#define META_INTERFACE_INTF_ANIMATION_H

#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <core/plugin/intf_interface.h>

#include <meta/base/ids.h>
#include <meta/base/namespace.h>
#include <meta/interface/animation/intf_animation_controller.h>
#include <meta/interface/animation/intf_animation_modifier.h>
#include <meta/interface/animation/intf_interpolator.h>
#include <meta/interface/curves/intf_curve_1d.h>
#include <meta/interface/intf_named.h>
#include <meta/interface/intf_object.h>
#include <meta/interface/property/intf_property.h>
#include <meta/interface/simple_event.h>

#include "meta/interface/property/property_events.h"

META_BEGIN_NAMESPACE()

META_REGISTER_INTERFACE(IAnimation, "37649f24-9ddd-4f8f-975b-9105db83cad0")
META_REGISTER_INTERFACE(IStartableAnimation, "1d1c7cac-0f13-421a-bbe8-f1aec751db6a")
META_REGISTER_INTERFACE(IPropertyAnimation, "501d9079-166e-4a59-a8f8-dfbae588bf80")
META_REGISTER_INTERFACE(IKeyframeAnimation, "75446215-498a-460f-8958-42f681c8becb")
META_REGISTER_INTERFACE(IStaggeredAnimation, "566ac43f-0408-403c-ab37-724687252ecd")
META_REGISTER_INTERFACE(ISequentialAnimation, "1c776172-f3d0-446c-8840-d487b916fbdd")
META_REGISTER_INTERFACE(IParallelAnimation, "5e9c463d-a442-46fe-8ee2-fb7396b1a90a")
META_REGISTER_INTERFACE(ITrackAnimation, "fdb5ee37-cd69-4591-8bc2-c13332baae18")
META_REGISTER_INTERFACE(ITimedAnimation, "4dacfc3d-747a-4bbb-88e7-505586d12c3f")

class IStaggeredAnimation;

/**
 * @brief IAnimation is the base interface for all animations.
 */
class IAnimation : public INamed {
    META_INTERFACE(INamed, IAnimation)
public:
    /**
     * @brief If true, the animation is enabled. If false, the animation
     *        remains stopped in the initial state.
     *        The default value is true.
     */
    META_PROPERTY(bool, Enabled)

    /**
     * @brief True when the animation is in a valid state to be run.
     *        For a PropertyAnimation this means that the animation points
     *        to a valid property. For a StaggeredAnimation this means
     *        that the animation has valid child animations.
     */
    META_READONLY_PROPERTY(bool, Valid)

    /**
     * @brief Duration of the animation after all animation modifiers have been applied.
     */
    META_READONLY_PROPERTY(TimeSpan, TotalDuration)

    /**
     * @brief True when the animation is running, false otherwise.
     */
    META_READONLY_PROPERTY(bool, Running)

    /**
     * @brief Animation state from [0,1] while the animation is running. When the animation
     *        has finished, the value will be 1.
     */
    META_READONLY_PROPERTY(float, Progress)

    /**
     * @brief Animation follows this curve.
     */
    META_PROPERTY(ICurve1D::Ptr, Curve)

    /**
     * @brief Controller which handles this animation.
     *        The default value is the controller returned by META_NS::GetAnimationController().
     * @note  To completely detach the animation from any controller, set the property to nullptr.
     *        In such a case the user is responsible for manually stepping the animation
     *        whenever needed.
     */
    META_PROPERTY(BASE_NS::weak_ptr<IAnimationController>, Controller)

    /**
     * @brief Steps the animation. Called by the framework, usually there is no
     *        need to call this manually.
     */
    virtual void Step(const IClock::ConstPtr& clock) = 0;

    /**
     * @brief Invoked when the animation has reached the end.
     */
    META_EVENT(IOnChanged, OnFinished)
    /**
     * @brief Invoked when the animation has started from the beginning.
     */
    META_EVENT(IOnChanged, OnStarted)
};

class IStartableAnimation : public CORE_NS::IInterface {
    META_INTERFACE2(CORE_NS::IInterface, IStartableAnimation)
public:
    /**
     * @brief Pauses the animation.
     */
    virtual void Pause() = 0;
    /**
     * @brief Stops the animation and starts it from the beginning.
     */
    virtual void Restart() = 0;
    /**
     * @brief Seeks animation to a specified position.
     *
     * @param position New Position of the animation. The value should be in
     *                 [0, 1] range, otherwise it will be clamped.
     */
    virtual void Seek(float position) = 0;
    /**
     * @brief Starts the animation.
     */
    virtual void Start() = 0;
    /**
     * @brief Stops the animation. It resets animation progress.
     */
    virtual void Stop() = 0;
    /**
     * @brief Jumps to the end of the animation.
     */
    virtual void Finish() = 0;
};

/**
 * @brief The IStaggeredAnimation defines an interface for an animation container which
 *        takes control of how it's child animations are run.
 */
class IStaggeredAnimation : public IAnimation {
    META_INTERFACE(IAnimation, IStaggeredAnimation)
public:
    /**
     * @brief Add a new animation to the container.
     */
    virtual void AddAnimation(const IAnimation::Ptr&) = 0;
    /**
     * @brief Remove an animation from the container.
     */
    virtual void RemoveAnimation(const IAnimation::Ptr&) = 0;
    /**
     * @brief Returns all direct child animations of this animation.
     */
    virtual BASE_NS::vector<IAnimation::Ptr> GetAnimations() const = 0;
};

/**
 * @brief The ISequentialAnimation interface defines an interface for an animation container
 *        which runs its child animations in sequence one after another.
 *
 *        Duration property is set by the container to reflect the total running time of all
 *        the animations added to the container. Setting the value has no effect.
 */
class ISequentialAnimation : public IStaggeredAnimation {
    META_INTERFACE(IStaggeredAnimation, ISequentialAnimation)
public:
};

/**
 * @brief The IParallelAnimation interface defines an interface for an animation container
 *        which runs its child animations parallelly.
 *
 *        Duration property is set by the container to reflect the running time of the longest
 *        running animation added to the container. Setting the value has no effect.
 */
class IParallelAnimation : public IStaggeredAnimation {
    META_INTERFACE(IStaggeredAnimation, IParallelAnimation)
public:
};

class ITimedAnimation : public IAnimation {
    META_INTERFACE(IAnimation, ITimedAnimation)
public:
    /**
     * @brief Duration of the animation before any modifiers are applied.
     */
    META_PROPERTY(TimeSpan, Duration)
};

/**
 * @brief IPropertyAnimation defines an interface for animations which can animate a property.
 */
class IPropertyAnimation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IPropertyAnimation, META_NS::InterfaceId::IPropertyAnimation)
public:
    /**
     * @brief Target property for this animation.
     *        Once target property is set, the type of the animation will be automatically
     *        set to match the type of the target property.
     */
    META_PROPERTY(IProperty::WeakPtr, Property)
};

/**
 * @brief IPropertyAnimation can be used to define an explicit keyframe animation on a property between two values.
 */
class IKeyframeAnimation : public ITimedAnimation {
    META_INTERFACE(ITimedAnimation, IKeyframeAnimation, META_NS::InterfaceId::IKeyframeAnimation)
public:
    /**
     * @brief A property containing the start value of the animation.
     *        Animation implementation will create a property of suitable type for the animation target property.
     * @note The property is null until target property is set or IPropertyAnimation::SetType is called explicitly.
     */
    META_PROPERTY(IAny::Ptr, From)
    /**
     * @brief A property containing the end value of the animation.
     *        Animation implementation will create a property of suitable type for the animation target property.
     * @note  The property is null until target property is set or IPropertyAnimation::SetType is called explicitly.
     */
    META_PROPERTY(IAny::Ptr, To)
};

/**
 * @brief ITrackAnimation defines the interface for an animation track, which can have multiple keyframes associated
 *        with a set of timestamps.
 *
 *        Functionality-wise a track animation is identical to a sequential animation with several keyframe animations
 *        all targetting the same property as it children. In such a case track animation is simpler to use and can
 *        also be implemented more efficiently by the framework.
 */
class ITrackAnimation : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, ITrackAnimation, META_NS::InterfaceId::ITrackAnimation)
public:
    /**
     * @brief Value of CurrentKeyframeIndex when the animation is not in valid timestamp range.
     */
    constexpr static uint32_t INVALID_INDEX = uint32_t(-1);
    /**
     * @brief TimeStamps for the animation. Timestamps are positions of the keyframes on the track, relative
     *        to the Duration of the animation.
     * @note Number of timestamps must match the number of keyframes.
     */
    META_ARRAY_PROPERTY(float, Timestamps)
    /**
     * @brief Keyframes for the animation.
     * @note Number of keyframes must match the number of timestamps.
     * @note KeyFrames property is invalid until target Property has been set, or IPropertyAnimation::SetType()
     *       has been called.
     */
    virtual IProperty::Ptr Keyframes() const = 0;
    /**
     * @brief Array of curves used for interpolating between each keyframe.
     * @note  As the curves are used to interpolate between keyframes, the number of curves needed to cover
     *        the entire animation is the number of keyframes - 1.
     * @note  The array can contain nullptrs, in which case the keyframe is interpolated linearly.
     * @note  The array size does not need to match keyframe count. Any keyframes for whom a matching curve
     *        is not found in the array are interpolated linearly.
     * @note  IAnimation::Curve can be used for controlling the progress over the full animation duration.
     */
    META_ARRAY_PROPERTY(ICurve1D::Ptr, KeyframeCurves)
    /**
     * @brief Array of functions that the track animation will call when the keyframe whose index corresponds
     *        to the index in the handler function array is reached.
     * @note  The array can also contain nullptrs if there is no handler for all frames.
     */
    META_ARRAY_PROPERTY(IFunction::Ptr, KeyframeHandlers)
    /**
     * @brief Index of the current keyframe.
     */
    META_READONLY_PROPERTY(uint32_t, CurrentKeyframeIndex)
    /**
     * @brief Adds a keyframe to the track animation.
     * @param timestamp Timestamp of the new keyframe. The keyframe will be added to such an index which maintains
     *                  ascending order of timestamps.
     * @param from A property containing the value to be added.
     * @return Index of the added keyframe or IMetaArrayProperty::N_POS in case of error.
     */
    virtual size_t AddKeyframe(float timestamp, const IAny::ConstPtr& value) = 0;
    /**
     * @brief Removes a keyframe and its associated timestamp from the track animation.
     * @param index Index to remove from.
     * @return True if a keyframe was removed, false otherwise.
     */
    virtual bool RemoveKeyframe(size_t index) = 0;
    /**
     * @brief Removes all keyframes and corresponding timestamps from the track animation.
     */
    virtual void RemoveAllKeyframes() = 0;
};

META_END_NAMESPACE()

META_INTERFACE_TYPE(META_NS::IAnimation)
META_INTERFACE_TYPE(META_NS::IPropertyAnimation)
META_INTERFACE_TYPE(META_NS::IKeyframeAnimation)
META_INTERFACE_TYPE(META_NS::IParallelAnimation)
META_INTERFACE_TYPE(META_NS::IStaggeredAnimation)

#endif
