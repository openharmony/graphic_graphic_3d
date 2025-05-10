/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#if !defined(API_3D_ECS_COMPONENTS_WEATHER_COMPONENT_H) || defined(IMPLEMENT_MANAGER)
#define API_3D_ECS_COMPONENTS_WEATHER_COMPONENT_H

#if !defined(IMPLEMENT_MANAGER)
#include <3d/namespace.h>
#include <base/containers/string.h>
#include <base/math/vector.h>
#include <core/ecs/component_struct_macros.h>
#include <core/ecs/entity_reference.h>
#include <core/ecs/intf_component_manager.h>

CORE3D_BEGIN_NAMESPACE()
#endif
/** Weather component is used controlling visualization of clouds and other weather effects.
 */
BEGIN_COMPONENT(IWeatherComponentManager, WeatherComponent)
#if !defined(IMPLEMENT_MANAGER)
    static constexpr float SUN_MIN_DISTANCE = 1.4712E11f;
    static constexpr float SUN_MAX_DISTANCE = 1.5112E11f;
    static constexpr float SUN_RADIUS = 1392684000.0f;
    static constexpr float PLANET_RADIUS = 6371E3f;
    static constexpr float ATMOS_RADIUS = 6471e3f;
    static constexpr float ATMOSPHERE_RADIUS_OUTER = 50000.0f;

    /** Cloud rendering type enumeration */
    enum class CloudRenderingType : uint8_t {
        /** Full resolution */
        FULL = 0,
        /** Downscaled resolution */
        DOWNSCALED = 1,
        /** Reprojected */
        REPROJECTED = 2,
    };
#endif

    /** Combined with weather map R channel */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, windDir, "Wind Direction", 0U, ARRAY_VALUE(1.0f, 0.0f, 1.0f))
    /** Combined with weather map R channel */
    DEFINE_PROPERTY(float, windSpeed, "Wind Speed", 0U, VALUE(1000.0f))

    /** Combined with weather map R channel */
    DEFINE_PROPERTY(float, coverage, "Override Cloud Coverage", 0U, VALUE(0.0f))
    /** Combined with weather map G channel */
    DEFINE_PROPERTY(float, precipitation, "Override Precipitation", 0U, VALUE(-1.0f))
    /** Combined with weather map B channel */
    DEFINE_PROPERTY(float, cloudType, "Override Cloud Type", 0U, VALUE(0.0f))
    /** Density */
    DEFINE_PROPERTY(float, density, "Override Density", 0U, VALUE(0.2f))
    /** Altitude */
    DEFINE_PROPERTY(float, altitude, "Override Altitude", 0, VALUE(0.0f))
    /** SunPos */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, sunPos, "Sun Position", 0U, ARRAY_VALUE(0.0, SUN_MAX_DISTANCE, 0.0))

    /** SilverIntensity */
    DEFINE_PROPERTY(float, silverIntensity, "Silver Intensity", 0U, VALUE(0.2f))
    /** SilverSpread */
    DEFINE_PROPERTY(float, silverSpread, "Silver Spread", 0U, VALUE(1.0f))
    /** Direct */
    DEFINE_PROPERTY(float, direct, "Direct", 0U, VALUE(1.0f))
    /** Ambient */
    DEFINE_PROPERTY(float, ambient, "Ambient", 0U, VALUE(1.0f))
    /** LightStep */
    DEFINE_PROPERTY(float, lightStep, "LightStep", 0U, VALUE(0.021f))
    /** ScatteringDist */
    DEFINE_PROPERTY(float, scatteringDist, "Scattering Distance", 0U, VALUE(1.0f))

    /**
    0 CLOUD_DOMAIN_LOW_RES
    1 CLOUD_DOMAIN_CURL_RES
    2 CLOUD_DOMAIN_HIGH_RES
    3 not used
    4 CLOUD_DOMAIN_CURL_AMPLITUDE
    */
    DEFINE_ARRAY_PROPERTY(
        float, 5, domain, "Domain", 0, ARRAY_VALUE(58000.0f, 14909.0f, 50681.0f, 1136364.0f, 10000.0f))
    /** anvilBias */
    DEFINE_PROPERTY(float, anvilBias, "", 0, VALUE(0.13f))
    /** brightness */
    DEFINE_PROPERTY(float, brightness, "", 0, VALUE(1.0f))
    /** eccintricity */
    DEFINE_PROPERTY(float, eccintricity, "", 0, VALUE(0.6f))

    /** Cloud rendering type */
    DEFINE_PROPERTY(
        CloudRenderingType, cloudRenderingType, "Cloud Rendering Type", 0, VALUE(CloudRenderingType::REPROJECTED))

    /** lowFrequencyImage */
    DEFINE_PROPERTY(CORE_NS::EntityReference, lowFrequencyImage, "", 0, )
    /** highFrequencyImage */
    DEFINE_PROPERTY(CORE_NS::EntityReference, highFrequencyImage, "", 0, )
    /** curlNoiseImage */
    DEFINE_PROPERTY(CORE_NS::EntityReference, curlNoiseImage, "", 0, )
    /** cirrusImage */
    DEFINE_PROPERTY(CORE_NS::EntityReference, cirrusImage, "", 0, )
    /** weatherMapImage */
    DEFINE_PROPERTY(CORE_NS::EntityReference, weatherMapImage, "", 0, )

END_COMPONENT(IWeatherComponentManager, WeatherComponent, "aeb22dc1-b7c7-4fbe-85bb-96668b9420fb")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
