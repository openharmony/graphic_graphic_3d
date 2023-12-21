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

#if !defined(FOG_COMPONENT) || defined(IMPLEMENT_MANAGER)
#define FOG_COMPONENT

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>
#include <core/property/property_types.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Fog component can be used to configure per camera scene fogging.
 */
BEGIN_COMPONENT(IFogComponentManager, FogComponent)
    /** Global density factor. Fog layer's thickness.
     */
    DEFINE_PROPERTY(float, density, "Density factor", 0, VALUE(1.0f))

    /** Height density factor. Controls how the density increases as height decreases. Smaller values for larger
     * transition.
     */
    DEFINE_PROPERTY(float, heightFalloff, "Height falloff", 0, VALUE(1.0f))

    /** Offset to "ground" plane. Y value in world space.
     */
    DEFINE_PROPERTY(float, heightFogOffset, "Height fog offset (to ground plane)", 0, VALUE(1.0f))

    /** Second layer. Density of the second layer. Value of 0 will disable the second layer.
     */
    DEFINE_PROPERTY(float, layerDensity, "Second layer density", 0, VALUE(1.0f))

    /** Second layer. Height density factor.
     */
    DEFINE_PROPERTY(float, layerHeightFalloff, "Second layer height density", 0, VALUE(1.0f))

    /** Second layer. Offset to "ground" plane.
     */
    DEFINE_PROPERTY(float, layerHeightFogOffset, "Layer height fog offset", 0, VALUE(1.0f))

    /** Start distance of the fog from the camera.
     */
    DEFINE_PROPERTY(float, startDistance, "Start distance from the camera", 0, VALUE(1.0f))

    /** Scene objects past this distance will not have fog applied. Some skyboxes, environments might have baked fog.
     * Negative value applies fog to everything.
     */
    DEFINE_PROPERTY(float, cuttoffDistance, "Cut off distance, i.e. baked background environments", 0, VALUE(-1.0f))

    /** The maximum opacity of the fog. 1.0 is fully opaque. 0.0 is invisible, no fog at all.
     */
    DEFINE_PROPERTY(float, maxOpacity, "Height falloff", 0, VALUE(1.0f))

    /** Primary color of the fog. The w-value is multiplier.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, inscatteringColor, "Primary color of the fog with intensity in alpha", 0,
        ARRAY_VALUE(1.0f, 1.0f, 1.0f, 1.0f))

    /** Environment map factor with intensity in alpha.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, envMapFactor, "Environment Map Factor", 0, ARRAY_VALUE(1.0f, 1.0f, 1.0f, 1.0f))

    /** Additional factor for e.g. shader customization.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, additionalFactor, "Additional factor for customization", 0,
        ARRAY_VALUE(0.0f, 0.0f, 0.0f, 0.0f))

END_COMPONENT(IFogComponentManager, FogComponent, "7a5faa64-7e5e-4ec6-90c5-e7ce24eee7b1")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
