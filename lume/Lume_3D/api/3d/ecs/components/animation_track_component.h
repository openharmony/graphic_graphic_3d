/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(ANIMATION_TRACK_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define ANIMATION_TRACK_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/util/uid.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/**
 * Properties of a single animation track.
 */
BEGIN_COMPONENT(IAnimationTrackComponentManager, AnimationTrackComponent)
#if !defined(IMPLEMENT_MANAGER)
    /**
     * Animation interpolation types. Can be one of the following:
     * step, linear and spline
     */
    enum class Interpolation : uint8_t {
        /** Interpolation type step */
        STEP,
        /** Interpolation type linear */
        LINEAR,
        /** Interpolation type spline */
        SPLINE
    };
#endif
    /** Animation target */
    DEFINE_PROPERTY(CORE_NS::EntityReference, target, "", 0,)
    /** Component type to animate */
    DEFINE_PROPERTY(BASE_NS::Uid, component, "", 0,)
    /** Property to animate. */
    DEFINE_PROPERTY(BASE_NS::string, property, "", 0,)
    /** Animation interpolation mode */
    DEFINE_PROPERTY(Interpolation, interpolationMode, "", 0,)
    /** Animation timestamps */
    DEFINE_PROPERTY(CORE_NS::EntityReference, timestamps, "", 0,)
    /** Animation data */
    DEFINE_PROPERTY(CORE_NS::EntityReference, data, "", 0,)

END_COMPONENT(IAnimationTrackComponentManager, AnimationTrackComponent, "42b5784a-44e6-4de1-8892-d0871ebca989")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif // __ANIMATION_TRACK_COMPONENT__
