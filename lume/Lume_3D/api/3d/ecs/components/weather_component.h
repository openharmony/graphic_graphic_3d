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
    /** Cloud optimiztion mode flags */
    enum CloudOptimizationFlagBits : uint32_t {
        /** Adaptive ray marching step sizes */
        ADAPTIVE_STEP_SIZE = (1 << 0),
        /** Dynamic LOD for lighting steps */
        DYNAMIC_LOD_LIGHTING = (1 << 1),
    };
    using CloudOptimizationFlags = uint32_t;

#endif
    //------------- SKY RENDERING PARAMETERS ----------------//
    /** Time of the day*/
    DEFINE_PROPERTY(float, timeOfDay, "Time of Day", 0U, VALUE(12.0f))
    /** Moon brightness*/
    DEFINE_PROPERTY(float, moonBrightness, "Brightness of the Moon", 0U, VALUE(0.6f))
    /** Night blue tint glow*/
    DEFINE_PROPERTY(float, nightGlow, "Night Glow", 0U, VALUE(0.025f))
    /** Sky View Brightness **/
    DEFINE_PROPERTY(float, skyViewBrightness, "Sky View Brightness", 0U, VALUE(0.7f))
    /** World Scale **/
    DEFINE_PROPERTY(float, worldScale, "World Scale", 0U, VALUE(100.0f))
    /** Max Aerial Perspective **/
    DEFINE_PROPERTY(float, maxAerialPerspective, "Max Aerial Perspective", 0U, VALUE(200.0f))
    /** Rayleigh Scattering**/
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, rayleighScatteringBase, "Rayleigh Scattering Base", 0U,
        ARRAY_VALUE(5.802f, 13.558f, 33.1f))
    /** Ozone Absorption **/
    DEFINE_PROPERTY(
        BASE_NS::Math::Vec3, ozoneAbsorptionBase, "Ozone Absorption Base", 0U, ARRAY_VALUE(0.650f, 1.881f, 0.085f))
    /** Mie Scattering **/
    DEFINE_PROPERTY(float, mieScatteringBase, "Mie Scattering Base", 0U, VALUE(3.996f))
    /** Mie Absorption **/
    DEFINE_PROPERTY(float, mieAbsorptionBase, "Mie Absorption Base", 0U, VALUE(4.4f))
    /** Ground Color **/
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, groundColor, "Ground Color", 0U, ARRAY_VALUE(0.03f, 0.04f, 0.06f))

    //------------- CLOUDS SHAPE AND RENDERING PARAMETERS------------//
    /** Combined with weather map R channel */
    DEFINE_PROPERTY(float, coverage, "Override Cloud Coverage", 0U, VALUE(0.3f))
    /** High frequency noise tiling (curliness) */
    DEFINE_PROPERTY(float, curliness, "Curliness", 0U, VALUE(4.0f))
    /** Low frequency noise tiling (Crispiness) */
    DEFINE_PROPERTY(float, crispiness, "Crispiness", 0U, VALUE(2.2f))
    /** Cloud type (0 = stratus, 0.5 = stratoCumulus, 1.0 = cumulus)*/
    DEFINE_PROPERTY(float, cloudType, "Override Cloud Type", 0U, VALUE(1.0f))
    /** Cloud bottom altitude */
    DEFINE_PROPERTY(float, cloudBottomAltitude, "Cloud bottom altitude", 0U, VALUE(4000.0f))
    /** Cloud top altitude */
    DEFINE_PROPERTY(float, cloudTopAltitude, "Cloud top altitude", 0U, VALUE(12000.0f))
    /** WeatherMap tiling scale */
    DEFINE_PROPERTY(float, weatherScale, "Tiling of the weather map texture", 0, VALUE(200.0f))
    /** Max Samples for ray marching*/
    DEFINE_PROPERTY(float, maxSamples, "Override max samples", 0U, VALUE(64.0f))
    /** Softness */
    DEFINE_PROPERTY(float, softness, "Softness", 0U, VALUE(1.4f))
    /** Direct light intensity at day */
    DEFINE_PROPERTY(float, directIntensityDay, "Direct Intensity at day", 0, VALUE(1.4f))
    /** Ambient intensity at day */
    DEFINE_PROPERTY(float, ambientIntensityDay, "Ambient intensity at day", 0U, VALUE(1.35f))
    /** Top cloud ambient color at Day */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, dayTopAmbientColor, "Ambient color of the top clouds at day", 0U,
        ARRAY_VALUE(1.0f, 1.0f, 1.0f))
    /** Ambient color hue at sunset (rgb, a=strength) */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, ambientColorHueSunset, "Ambient color hue sunset", 0U,
        ARRAY_VALUE(1.0f, 0.6429f, 0.3393f, 1.12f))
    /** Direct color hue at sunset (rgb, a=strength) */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, directColorHueSunset, "Direct color hue sunset", 0U,
        ARRAY_VALUE(1.0f, 0.5231f, 0.2154f, 1.30f))
    /** Sun glare color at day (rgb, a=strength) */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, sunGlareColorDay, "Color of sun glare at day", 0U,
        ARRAY_VALUE(1.0f, 0.4444f, 0.1111f, 1.80f))
    /** Sun glare color at sunset (rgb, a=strength) */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, sunGlareColorSunset, "Color of sun glare at day", 0U,
        ARRAY_VALUE(1.0f, 0.4091f, 0.0909f, 1.0f))
    /** Top cloud ambient color at Night */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, nightTopAmbientColor, "Ambient color of the top clouds at night", 0U,
        ARRAY_VALUE(0.53f, 0.61f, 0.82f))
    /** Bottom cloud ambient color at Night */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, nightBottomAmbientColor, "Ambient color of the bottom clouds at night", 0U,
        ARRAY_VALUE(0.1f, 0.17f, 0.37f))
    /** Ambient intensity at Night */
    DEFINE_PROPERTY(float, ambientIntensityNight, "Ambient light intensity at night", 0U, 1.0f)
    /** Direct moon light intensity at Night */
    DEFINE_PROPERTY(float, directLightIntensityNight, "Direct moon light intensity at night", 0U, 0.60f)
    /** Base moonlight color (rgb) */
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, moonBaseColor, "Moonlight base color", 0U, ARRAY_VALUE(0.53f, 0.52f, 0.74f))
    /** Moon glare color (rgb, a=strength) */
    DEFINE_PROPERTY(BASE_NS::Math::Vec4, moonGlareColor, "Color of moon glare", 0U, ARRAY_VALUE(1.0f, 1.0f, 1.0f, 0.6f))
    /** WindSpeed used to animate clouds */
    DEFINE_PROPERTY(float, windSpeed, "cloud top offset", 0U, VALUE(100.0f))
    /** Wind direction*/
    DEFINE_PROPERTY(BASE_NS::Math::Vec3, windDir, "Wind Direction", 0U, ARRAY_VALUE(1.0f, 0.0f, 1.0f))
    /** Cloud Top Offset used in animation */
    DEFINE_PROPERTY(float, cloudTopOffset, "Cloud Top Offset", 0U, VALUE(750.0f))
    /** Height of the clouds' fog at the horizon*/
    DEFINE_PROPERTY(float, cloudHorizonFogHeight, "Cloud horizon fog height", 0U, VALUE(1.5f))
    /** Falloff of the clouds' fog at the horizon*/
    DEFINE_PROPERTY(float, horizonFogFalloff, "Cloud horizon fog falloff", 0U, VALUE(1.0f))
    /** Global Density of the clouds*/
    DEFINE_PROPERTY(float, density, "Override Density", 0U, VALUE(0.004f))
    /** Earth radius */
    DEFINE_PROPERTY(float, earthRadius, "Earth radius", 0U, VALUE(6371000.0f))

    /** Cloud rendering type */
    DEFINE_PROPERTY(
        CloudRenderingType, cloudRenderingType, "Cloud Rendering Type", 0, VALUE(CloudRenderingType::REPROJECTED))

    /** Cloud optimization flags*/
    DEFINE_BITFIELD_PROPERTY(CloudOptimizationFlags, cloudOptimizationFlags, "Cloud Optimization flags",
        PropertyFlags::IS_BITFIELD, 0U, WeatherComponent::CloudOptimizationFlagBits)

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

    DEFINE_PROPERTY(CORE_NS::Entity, directionalSun, "", 0, )

END_COMPONENT(IWeatherComponentManager, WeatherComponent, "aeb22dc1-b7c7-4fbe-85bb-96668b9420fb")
#if !defined(IMPLEMENT_MANAGER)
CORE3D_END_NAMESPACE()
#endif
#endif
