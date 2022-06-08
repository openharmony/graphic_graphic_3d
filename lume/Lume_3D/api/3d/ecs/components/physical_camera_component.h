/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#if !defined(PHYSICAL_CAMERA_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define PHYSICAL_CAMERA_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/property/property_types.h>

CORE3D_BEGIN_NAMESPACE()
#endif
BEGIN_COMPONENT(IPhysicalCameraComponentManager, PhysicalCameraComponent)
    /** Defines the size of the aperture */
    DEFINE_PROPERTY(float, aperture, "Aperture", 0, VALUE(16.f))
    /** Defines the shutter speed */
    DEFINE_PROPERTY(float, shutterSpeed, "Shutter Speed", 0, VALUE(1.0f / 125.0f))
    /** Sensitivity is similar to ISO value in photography,
     *  If the ISO value is high (film is more sensitive to the light), the image is brighter.
     *  Lower ISO values mean that the film is less sensitive and produces a darker image.
     */
    DEFINE_PROPERTY(float, sensitivity, "Sensitivity", 0, VALUE(100.f))
END_COMPONENT(IPhysicalCameraComponentManager, PhysicalCameraComponent, "4badb6a5-77c6-49ca-a5c3-f788f8ad5534")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
