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

#if !defined(API_3D_ECS_COMPONENTS_LIGHT_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_LIGHT_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/ecs/components/layer_defines.h>
#include <3d/namespace.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif

BEGIN_COMPONENT(ILightComponentManager, LightComponent)
#if !defined(IMPLEMENT_MANAGER)
    enum class Type : uint8_t { DIRECTIONAL = 0, POINT = 1, SPOT = 2 };
#endif

    /** Type of the light.
     */
    DEFINE_PROPERTY(Type, type, "Type", 0, VALUE(Type::DIRECTIONAL))

    /** Diffuse color of the light. Values from 0.0 to 1.0
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, color, "Color", 0, ARRAY_VALUE(1.0f, 1.0f, 1.0f))

    /** Intensity of the light.
     */
    DEFINE_PROPERTY(float, intensity, "Intensity", 0, VALUE(1.0f))

    /** Range, distance cutoff for points and spots.
     */
    DEFINE_PROPERTY(float, range, "Range", 0, VALUE(0.0f))

    /** Near plane distance from the light source.
     */
    DEFINE_PROPERTY(float, nearPlane, "Shadow Near Plane", 0, VALUE(0.5f))

    /** Spotlight inner angle.
     */
    DEFINE_PROPERTY(float, spotInnerAngle, "Spot Cone Inner Angle", 0, VALUE(0.0f))

    /** Spotlight outer angle.
     */
    DEFINE_PROPERTY(float, spotOuterAngle, "Spot Cone Outer Angle", 0, VALUE(0.78539816339f))

    /** Shadow strength.
     */
    DEFINE_PROPERTY(float, shadowStrength, "Shadow Strength", 0, VALUE(1.0f))

    /** Shadow depth bias.
     */
    DEFINE_PROPERTY(float, shadowDepthBias, "Shadow Depth Bias", 0, VALUE(0.005f))

    /** Shadow normal bias.
     */
    DEFINE_PROPERTY(float, shadowNormalBias, "Shadow Normal Bias", 0, VALUE(0.025f))

    /** Shadow enabled.
     */
    DEFINE_PROPERTY(bool, shadowEnabled, "Shadow Enabled", 0, VALUE(false))

    /** Additional factor for e.g. shader customization.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, additionalFactor, "Additional Factor", 0, ARRAY_VALUE(0.0f, 0.0f, 0.0f, 0.0f))

    /** Defines a layer mask which affects lighting of layer objects. Default is all layer mask, and then the
     * light affects objects on all layers. */
    DEFINE_BITFIELD_PROPERTY(uint64_t, lightLayerMask, "Light Layer Mask", PropertyFlags::IS_BITFIELD,
        VALUE(LayerConstants::ALL_LAYER_MASK), LayerFlagBits)

    /** Defines a layer mask which affects shadow camera's rendering. Default is all layer mask, and then the
     * shadow camera renders objects from all layers. */
    DEFINE_BITFIELD_PROPERTY(uint64_t, shadowLayerMask, "Shadow Layer Mask", PropertyFlags::IS_BITFIELD,
        VALUE(LayerConstants::ALL_LAYER_MASK), LayerFlagBits)

END_COMPONENT(ILightComponentManager, LightComponent, "8047c9cd-4e83-45b3-91d9-dd32d643f0c8")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif

#endif
