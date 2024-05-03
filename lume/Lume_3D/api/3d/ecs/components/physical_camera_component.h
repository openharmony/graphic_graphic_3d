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

#if !defined(API_3D_ECS_COMPONENTS_PHYSICAL_CAMERA_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_PHYSICAL_CAMERA_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

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
