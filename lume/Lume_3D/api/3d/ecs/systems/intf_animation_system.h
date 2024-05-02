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

#ifndef API_3D_ECS_SYSTEMS_IANIMATION_SYSTEM_H
#define API_3D_ECS_SYSTEMS_IANIMATION_SYSTEM_H

#include <3d/ecs/components/animation_component.h>
#include <base/containers/string_view.h>
#include <core/ecs/intf_system.h>

CORE_BEGIN_NAMESPACE()
struct Entity;
CORE_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class ISceneNode;
/** @ingroup group_ecs_systems_ianimation */
/**
 * Animation Playback.
 * Object which includes ways to control animation.
 */
class IAnimationPlayback {
public:
    /** Returns playback name. */
    virtual BASE_NS::string_view GetName() const = 0;

    /** Sets playback state.
     *  @param state can be one of the following states: STOP, PLAY and PAUSE.
     */
    virtual void SetPlaybackState(AnimationComponent::PlaybackState state) = 0;

    /** Returns playback state of this track.
     */
    virtual AnimationComponent::AnimationComponent::PlaybackState GetPlaybackState() const = 0;

    /** Sets number of times animation is repeated.
     *  @param repeatCount number of repetition count or infinite if AnimationComponent::REPEAT_COUNT_INFINITE is
     * passed.
     */
    virtual void SetRepeatCount(uint32_t repeatCount) = 0;

    /** Returns repetition count for playback clip.
     */
    virtual uint32_t GetRepeatCount() const = 0;

    /** Sets animation weight, used when multiple animations are blended together.
     *  @param weight weight for the animation, in range from zero to one.
     */
    virtual void SetWeight(float weight) = 0;

    /** Returns animation weight, where 0.0 means that animation is not applied and 1.0 means that animation is applied
     * in full strength.
     */
    virtual float GetWeight() const = 0;

    /** Set playback time position in seconds.
     *  @param timePosition New time position for animation.
     */
    virtual void SetTimePosition(float timePosition) = 0;

    /** Returns current playback position in seconds. */
    virtual float GetTimePosition() const = 0;

    /** Returns the length of the animation data in seconds. */
    virtual float GetAnimationLength() const = 0;

    /** Set animation start time offset in seconds. */
    virtual void SetStartOffset(float startOffset) = 0;

    /** Returns animation start time in seconds. */
    virtual float GetStartOffset() const = 0;

    /** Set animation playback duration in seconds. */
    virtual void SetDuration(float duration) = 0;

    /** Returns animation duration in seconds. */
    virtual float GetDuration() const = 0;

    /** Returns boolean flag if animation playback is completed.
     */
    virtual bool IsCompleted() const = 0;

    /** Set speed for animation, can be also negative for reverse playback
     */
    virtual void SetSpeed(float) = 0;

    /** Get speed of animation currently in use
     */
    virtual float GetSpeed() const = 0;

    /** Get entity owning the animation component.
     */
    virtual CORE_NS::Entity GetEntity() const = 0;

protected:
    virtual ~IAnimationPlayback() = default;
};

/** @ingroup group_ecs_systems_ianimation */
/**
 * Animation System.
 * System that has Creation and Destroying functions for Animation Playbacks(objects that control animation).
 */
class IAnimationSystem : public CORE_NS::ISystem {
public:
    static constexpr BASE_NS::Uid UID { "3937c464-0073-4900-865d-89da078b9daa" };

    /** Creates a playback object that allows to control animation state (play, pause etc).
     *  This is intended for animations imported from glTF, which are limited to the node hierarchy.
     *  @param animationEntity Entity owning the animation resource.
     *  @param node Node to animate.
     *  @return Playback object or nullptr if no animation is found.
     */
    virtual IAnimationPlayback* CreatePlayback(CORE_NS::Entity const& animationEntity, ISceneNode const& node) = 0;

    /** Creates a playback object that allows to control animation state (play, pause etc).
     *  @param animationEntity Entity owning the animation resource.
     *  @param targetEntities Target entity for each of the animation tracks.
     *  @return Playback object or nullptr if no animation is found.
     */
    virtual IAnimationPlayback* CreatePlayback(
        const CORE_NS::Entity animationEntity, const BASE_NS::array_view<const CORE_NS::Entity> targetEntities) = 0;

    /** Destroys given playback object
     *  @param playback Playback object to be destroyed
     */
    virtual void DestroyPlayback(IAnimationPlayback* playback) = 0;

    /** Retrieve number of currently existing playback objects. */
    virtual size_t GetPlaybackCount() const = 0;

    /** Retrieve playback object in given index. */
    virtual IAnimationPlayback* GetPlayback(size_t index) const = 0;

protected:
    IAnimationSystem() = default;
    IAnimationSystem(const IAnimationSystem&) = delete;
    IAnimationSystem(IAnimationSystem&&) = delete;
    IAnimationSystem& operator=(const IAnimationSystem&) = delete;
    IAnimationSystem& operator=(IAnimationSystem&&) = delete;
};

CORE3D_END_NAMESPACE()

#endif // API_3D_ECS_SYSTEMS_IANIMATION_SYSTEM_H
