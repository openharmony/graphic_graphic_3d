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

#if !defined(ANIMATION_STATE_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define ANIMATION_STATE_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/ecs/components/animation_component.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Internal state of animation playback. Inteded to be used by the AnimationSystem to track playback state changes. */
BEGIN_COMPONENT(IAnimationStateComponentManager, AnimationStateComponent)
#if !defined(IMPLEMENT_MANAGER)
    // A description of a state of track that is animated.
    struct TrackState {
        // Target entity.
        CORE_NS::Entity entity;

        // Number of frames.
        size_t length;
    };
    /** Playback states for an animation. */
    enum FlagBits : uint8_t { MANUAL_PROGRESS = 0x01 };
    using Flags = uint8_t;
#endif
    // Animated entities (matches tracks).
    DEFINE_PROPERTY(BASE_NS::vector<TrackState>, trackStates, "", CORE_NS::PropertyFlags::IS_HIDDEN, )

    DEFINE_PROPERTY(AnimationComponent::PlaybackState, state, "", CORE_NS::PropertyFlags::IS_HIDDEN,
        VALUE(AnimationComponent::PlaybackState::STOP))

    // Playback time.
    DEFINE_PROPERTY(float, time, "Playback Time", 0, VALUE(0.0f))

    // current repeat loop.
    DEFINE_PROPERTY(uint32_t, currentLoop, "", CORE_NS::PropertyFlags::IS_HIDDEN, VALUE(0U))

    // Completion flag.
    DEFINE_PROPERTY(bool, completed, "", CORE_NS::PropertyFlags::IS_HIDDEN, VALUE(false))

    DEFINE_PROPERTY(bool, dirty, "", CORE_NS::PropertyFlags::IS_HIDDEN, VALUE(false))
    /** Additional controls for animation playback. */
    DEFINE_BITFIELD_PROPERTY(
        Flags, options, "Options", CORE_NS::PropertyFlags::IS_BITFIELD, VALUE(0), AnimationStateComponent::FlagBits)

END_COMPONENT(IAnimationStateComponentManager, AnimationStateComponent, "a590f16c-723d-421e-a9f6-451a4ac49907")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
