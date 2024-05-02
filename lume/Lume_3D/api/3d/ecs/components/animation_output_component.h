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

#if !defined(API_3D_ECS_COMPONENTS_ANIMATION_OUTPUT_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_ANIMATION_OUTPUT_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/**
 * Animation output component represents animation keyframe data.
 */
BEGIN_COMPONENT(IAnimationOutputComponentManager, AnimationOutputComponent)
    /** Type hash of the keyframe data. If the property targetted by AnimationTrack
     * doesn't match with this type information (PropertyTypeDecl::compareHash), the track will be skipped. */
    DEFINE_PROPERTY(uint64_t, type, "Keyframe Datatype Hash", 0, )
    /** Keyframe data. Data is stored as byte array but actual data type is specified in AnimationOutputComponent::type.
     */
    DEFINE_PROPERTY(BASE_NS::vector<uint8_t>, data, "Keyframe Data", 0, )

END_COMPONENT(IAnimationOutputComponentManager, AnimationOutputComponent, "aefd2f02-9178-46d1-8ef2-81a262f0a212")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif // API_3D_ECS_COMPONENTS_ANIMATION_OUTPUT_COMPONENT_H
