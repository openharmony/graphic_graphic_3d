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

#if !defined(API_3D_ECS_COMPONENTS_FOG_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_FOG_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Fog component can be used to configure per camera scene fogging.
 */
BEGIN_COMPONENT(IFogComponentManager, FogComponent)
    /** Global density factor. Fog layer's thickness.
     */
    DEFINE_PROPERTY(float, density, "Density Factor", 0, VALUE(0.005f))

    /** Height density factor. Controls how the density increases as height decreases. Smaller values for larger
     * transition.
     */
    DEFINE_PROPERTY(float, heightFalloff, "Height Falloff", 0, VALUE(1.0f))

    /** Offset to "ground" plane. Y value in world space.
     */
    DEFINE_PROPERTY(float, heightFogOffset, "Height Offset (to ground plane)", 0, VALUE(0.0f))

    /** Second layer. Density of the second layer. Value of 0 will disable the second layer.
     */
    DEFINE_PROPERTY(float, layerDensity, "Second Layer Density", 0, VALUE(0.04f))

    /** Second layer. Height density factor.
     */
    DEFINE_PROPERTY(float, layerHeightFalloff, "Second Layer Height Falloff", 0, VALUE(1.0f))

    /** Second layer. Offset to "ground" plane.
     */
    DEFINE_PROPERTY(float, layerHeightFogOffset, "Second Layer Height Offset", 0, VALUE(0.0f))

    /** Start distance of the fog from the camera.
     */
    DEFINE_PROPERTY(float, startDistance, "Start Distance From Camera", 0, VALUE(1.0f))

    /** Scene objects past this distance will not have fog applied. Some skyboxes, environments might have baked fog.
     * Negative value applies fog to everything.
     */
    DEFINE_PROPERTY(float, cuttoffDistance, "Cutoff Distance", 0, VALUE(-1.0f))

    /** The opacity limit of both layers of the the fog. 1.0 is fully opaque. 0.0 is invisible, no fog at all.
     */
    DEFINE_PROPERTY(float, maxOpacity, "Maximum Opacity", 0, VALUE(0.2f))

    /** Primary color of the fog. The w-value is multiplier.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, inscatteringColor, "Fog Primary Color (Intensity In Alpha)", 0,
        ARRAY_VALUE(1.0f, 1.0f, 1.0f, 1.0f))

    /** Environment map factor with intensity in alpha.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, envMapFactor, "Environment Map Factor", 0, ARRAY_VALUE(1.0f, 1.0f, 1.0f, 0.1f))

    /** Additional factor for e.g. shader customization.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, additionalFactor, "Additional Factor For Customization", 0,
        ARRAY_VALUE(0.0f, 0.0f, 0.0f, 0.0f))

END_COMPONENT(IFogComponentManager, FogComponent, "7a5faa64-7e5e-4ec6-90c5-e7ce24eee7b1")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
