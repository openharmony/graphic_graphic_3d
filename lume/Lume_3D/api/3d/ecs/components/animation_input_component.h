/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
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
