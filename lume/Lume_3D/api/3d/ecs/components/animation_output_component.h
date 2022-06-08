/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(ANIMATION_OUTPUT_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define ANIMATION_OUTPUT_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>
#include <core/property/property.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/**
 * Animation output component represents animation keyframe data.
 */
BEGIN_COMPONENT(IAnimationOutputComponentManager, AnimationOutputComponent)
    /** Type hash of the keyframe data. If the property targetted by AnimationTrack
     * doesn't match with this type information (PropertyTypeDecl::compareHash), the track will be skipped. */
    DEFINE_PROPERTY(uint64_t, type, "Datatype Hash", 0,)
    /** Keyframe data. Data is stored as byte array but actual data type is specified in AnimationOutputComponent::type.
     */
    DEFINE_PROPERTY(BASE_NS::vector<uint8_t>, data, "Keyframe Data", 0,)

END_COMPONENT(IAnimationOutputComponentManager, AnimationOutputComponent, "aefd2f02-9178-46d1-8ef2-81a262f0a212")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif // __ANIMATION_OUTPUT_COMPONENT__
