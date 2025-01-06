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

#ifndef SCENE_INTERFACE_IENVIRONMENT_H
#define SCENE_INTERFACE_IENVIRONMENT_H

#include <scene/base/types.h>
#include <scene/interface/intf_bitmap.h>

SCENE_BEGIN_NAMESPACE()

enum class EnvBackgroundType : uint8_t {
    /** Background none */
    NONE = 0,
    /** Background image */
    IMAGE = 1,
    /** Background cubemap */
    CUBEMAP = 2,
    /** Background equirectangular */
    EQUIRECTANGULAR = 3,
};

class IEnvironment : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IEnvironment, "68341d9c-6252-4a72-9011-adda3a83b848")
public:
    META_PROPERTY(EnvBackgroundType, Background)

    /**
     * @brief Indirect diffuse factor with intensity in alpha.
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Vec4, IndirectDiffuseFactor)

    /**
     * @brief Indirect specular factor with intensity in alpha.
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Vec4, IndirectSpecularFactor)

    /**
     * @brief Environment map factor with intensity in alpha.
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Vec4, EnvMapFactor)

    /**
     * @brief Radiance cubemap.
     * @return property pointer
     */
    META_PROPERTY(IBitmap::Ptr, RadianceImage)

    /**
     * @brief Number of mip map levels in radiance cubemap, zero value indicates that full mip chain is available.
     * @return property pointer
     */
    META_PROPERTY(uint32_t, RadianceCubemapMipCount)

    /**
     * @brief  Environment map. (Cubemap, Equirect, Image).
     * @return property pointer
     */
    META_PROPERTY(IBitmap::Ptr, EnvironmentImage)

    /**
     * @brief Mip lod level for env map sampling, allows to have blurred / gradient background for the scene.
     * @return property pointer
     */
    META_PROPERTY(float, EnvironmentMapLodLevel)

    /**
     * @brief Irradiance lighting coefficients.
     * Values are expected to be prescaled with 1.0 / PI for Lambertian diffuse
     * @return property pointer
     */
    META_ARRAY_PROPERTY(BASE_NS::Math::Vec3, IrradianceCoefficients)

    /**
     * @brief IBL environment rotation.
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Quat, EnvironmentRotation)

    /**
     * @brief Additional factor for shader customization.
     * @return property pointer
     */

    // q: how to do?
    // META_PROPERTY(uint64_t, ShaderHandle)
};

META_REGISTER_CLASS(Environment, "e839fed0-d4d2-4521-9df9-6aeb5f62161e", META_NS::ObjectCategoryBits::NO_CATEGORY)

SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::EnvBackgroundType)
META_INTERFACE_TYPE(SCENE_NS::IEnvironment)

#endif
