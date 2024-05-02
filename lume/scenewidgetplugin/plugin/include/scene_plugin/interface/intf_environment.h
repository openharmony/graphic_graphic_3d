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
#ifndef SCENEPLUGIN_INTF_ENVIRONMENT_H
#define SCENEPLUGIN_INTF_ENVIRONMENT_H

#include <scene_plugin/namespace.h>
#include <scene_plugin/interface/intf_bitmap.h>

#include <meta/api/internal/object_api.h>
#include <meta/base/types.h>

SCENE_BEGIN_NAMESPACE()

// Implemented by SCENE_NS::ClassId::Environment
REGISTER_INTERFACE(IEnvironment, "8940546e-9717-475a-a799-60b93546dc9c")
class IEnvironment : public CORE_NS::IInterface {
public:
    META_INTERFACE(CORE_NS::IInterface, IEnvironment, InterfaceId::IEnvironment)

public:
    enum BackgroundType : uint8_t {
        /** Background none */
        NONE = 0,
        /** Background image */
        IMAGE = 1,
        /** Background cubemap */
        CUBEMAP = 2,
        /** Background equirectangular */
        EQUIRECTANGULAR = 3,
    };

    /**
     * @brief The type of the background fill when rendering.
     * @return property pointer
     */
    META_PROPERTY(BackgroundType, Background)

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
    META_ARRAY_PROPERTY(BASE_NS::Math::Vec3, IrradianceCoefficients);

    /**
     * @brief IBL environment rotation.
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Quat, EnvironmentRotation)

    /**
     * @brief Additional factor for shader customization.
     * @return property pointer
     */
    META_PROPERTY(BASE_NS::Math::Vec4, AdditionalFactor)

    /**
     * @brief Shader. Prefer using automatic selection if no custom shaders.
     * Needs to match default material env layouts and specializations (api/3d/shaders/common).
     * @return property pointer
     */
    META_PROPERTY(uint64_t, ShaderHandle)
};
SCENE_END_NAMESPACE()

META_TYPE(SCENE_NS::IEnvironment::BackgroundType)
META_TYPE(SCENE_NS::IEnvironment::WeakPtr);
META_TYPE(SCENE_NS::IEnvironment::Ptr);

#endif // SCENEPLUGIN_INTF_LIGHT_H
