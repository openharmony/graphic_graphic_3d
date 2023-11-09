/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#if !defined(ANIMATION_INPUT_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define ANIMATION_INPUT_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/**
 * Animation input component represents animation keyframe time stamps in seconds.
 */
BEGIN_COMPONENT(IAnimationInputComponentManager, AnimationInputComponent)

    /** Keyframe time stamps in seconds. */
    DEFINE_PROPERTY(BASE_NS::vector<float>, timestamps, "", 0,)

END_COMPONENT(IAnimationInputComponentManager, AnimationInputComponent, "a6e72690-4e42-47ba-9104-b0d16541838b")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif // __ANIMATION_INPUT_COMPONENT__
