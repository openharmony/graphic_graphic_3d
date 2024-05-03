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

#if !defined(API_3D_ECS_COMPONENTS_ENVIRONMENT_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_ENVIRONMENT_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>
#include <core/namespace.h>

CORE3D_BEGIN_NAMESPACE()
#endif

/** Environment component can be used to configure per camera environment lighting.
 * With full customization one can use custom resources property
 */
BEGIN_COMPONENT(IEnvironmentComponentManager, EnvironmentComponent)
#if !defined(IMPLEMENT_MANAGER)
    enum class Background : uint8_t {
        /** Background none */
        NONE = 0,
        /** Background image */
        IMAGE = 1,
        /** Background cubemap */
        CUBEMAP = 2,
        /** Background equirectangular */
        EQUIRECTANGULAR = 3,
    };
#endif
    /** The type of the background fill when rendering.
     */
    DEFINE_PROPERTY(Background, background, "Background", 0, VALUE(Background::NONE))

    /** Indirect diffuse factor with intensity in alpha.
     */
    DEFINE_PROPERTY(
        BASE_NS::Math::Vec4, indirectDiffuseFactor, "Indirect Diffuse Factor", 0, ARRAY_VALUE(1.0f, 1.0f, 1.0f, 1.0f))

    /** Indirect specular factor with intensity in alpha.
     */
    DEFINE_PROPERTY(
        BASE_NS::Math::Vec4, indirectSpecularFactor, "Indirect Specular Factor", 0, ARRAY_VALUE(1.0f, 1.0f, 1.0f, 1.0f))

    /** Environment map factor with intensity in alpha.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, envMapFactor, "Environment Map Factor", 0, ARRAY_VALUE(1.0f, 1.0f, 1.0f, 1.0f))

    /** Radiance cubemap.
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, radianceCubemap, "Radiance Cubemap", 0,)

    /** Number of mip map levels in radiance cubemap, zero value indicates that full mip chain is available.
     */
    DEFINE_PROPERTY(uint32_t, radianceCubemapMipCount, "Radiance Cubemap Mip Count", 0, VALUE(0))

    /** Environment map. (Cubemap, Equirect, Image)
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, envMap, "Environment Map", 0,)

    /** Mip lod level for env map sampling, allows to have blurred / gradient background for the scene.
     */
    DEFINE_PROPERTY(float, envMapLodLevel, "Envmap Lod Level", 0, VALUE(0.0f))

    /** Irradiance lighting coefficients.
     * Values are expected to be prescaled with 1.0 / PI for Lambertian diffuse
     */
    DEFINE_ARRAY_PROPERTY(BASE_NS::Math::Vec3, 9, irradianceCoefficients, "Irradiance Coefficients", 0, ARRAY_VALUE())

    /** IBL environment rotation.
     */
    DEFINE_PROPERTY(
        BASE_NS::Math::Quat, environmentRotation, "IBL Environment Rotation", 0, ARRAY_VALUE(0.f, 0.f, 0.f, 1.f))

    /** Additional blend factor for multiple environments.
     */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, blendFactor, "Additional Blend Factor for Multiple Environments", 0,
        ARRAY_VALUE(1.0f, 1.0f, 1.0f, 1.0f))

    /** Shader. Prefer using automatic selection if no custom shaders.
     * Needs to match default material env layouts and specializations (api/3d/shaders/common).
     */
    DEFINE_PROPERTY(CORE_NS::EntityReference, shader, "Custom Environment Shader", 0,)

    /** Custom material extension resources. Deprecates and prevents MaterialExtensionComponent usage.
     * Are automatically bound to custom shader, custom pipeline layout custom descriptor set if they are in order.
     */
    DEFINE_PROPERTY(
        BASE_NS::vector<CORE_NS::EntityReference>, customResources, "Custom Material Extension Resources", 0, )

END_COMPONENT(IEnvironmentComponentManager, EnvironmentComponent, "a603b2e8-27f0-4c03-9538-70eaef88e3d3")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
