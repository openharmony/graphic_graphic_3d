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

#if !defined(API_3D_ECS_COMPONENTS_ANIMATION_INPUT_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_ANIMATION_INPUT_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/**
 * Animation input component represents animation keyframe timestamps in seconds.
 */
BEGIN_COMPONENT(IAnimationInputComponentManager, AnimationInputComponent)

    /** Keyframe timestamps in seconds. */
    DEFINE_PROPERTY(BASE_NS::vector<float>, timestamps, "Keyframe Timestamps", 0, )

END_COMPONENT(IAnimationInputComponentManager, AnimationInputComponent, "a6e72690-4e42-47ba-9104-b0d16541838b")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif // API_3D_ECS_COMPONENTS_ANIMATION_INPUT_COMPONENT_H
