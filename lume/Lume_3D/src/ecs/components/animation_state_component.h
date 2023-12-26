/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include <core/property/property_types.h>

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
#endif
    // Animated entities (matches tracks).
    DEFINE_PROPERTY(BASE_NS::vector<TrackState>, trackStates, "", 0,)

    DEFINE_PROPERTY(AnimationComponent::PlaybackState, state, "", 0, VALUE(AnimationComponent::PlaybackState::STOP))

    // Playback time.
    DEFINE_PROPERTY(float, time, "", 0, VALUE(0.0f))

    // Animation length (length of the undelying animation data).
    DEFINE_PROPERTY(float, length, "", 0, VALUE(0.0f))

    // current repeat loop.
    DEFINE_PROPERTY(uint32_t, currentLoop, "", 0, VALUE(0U))

    // Completion flag.
    DEFINE_PROPERTY(bool, completed, "", 0, VALUE(false))

    DEFINE_PROPERTY(bool, dirty, "", 0, VALUE(false))

END_COMPONENT(IAnimationStateComponentManager, AnimationStateComponent, "a590f16c-723d-421e-a9f6-451a4ac49907")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
