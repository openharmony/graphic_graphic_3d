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

#ifndef CORE3D_DATASTORE_RENDER_DATA_STORE_WEATHER_H
#define CORE3D_DATASTORE_RENDER_DATA_STORE_WEATHER_H

#include <atomic>

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/containers/vector.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/ecs/entity.h>
#include <render/datastore/intf_render_data_store.h>
#include <render/resource_handle.h>

RENDER_BEGIN_NAMESPACE()
class IRenderContext;
RENDER_END_NAMESPACE()

CORE3D_BEGIN_NAMESPACE()
class RenderDataStoreWeather : public RENDER_NS::IRenderDataStore {
public:
    static constexpr const char* const TYPE_NAME = "RenderDataStoreWeather";
    static constexpr BASE_NS::Uid UID { "13BFD29D-D68A-466F-8577-9F09784DB901" };

    RenderDataStoreWeather(const RENDER_NS::IRenderContext& renderContext, BASE_NS::string_view name);
    ~RenderDataStoreWeather() override = default;

    // for clouds and sky
    enum class CloudRenderingType : uint8_t {
        /** Full resolution */
        FULL = 0,
        /** Downscaled resolution */
        DOWNSCALED = 1,
        /** Reprojected */
        REPROJECTED = 2,
    };

    /** Cloud optimiztion flags */
    enum CloudOptimizationFlagBits : uint32_t {
        /** Adaptive ray marching step sizes */
        ADAPTIVE_STEP_SIZE = (1 << 0),
        /** Dynamic LOD for lighting steps */
        DYNAMIC_LOD_LIGHTING = (1 << 1),
    };
    using CloudOptimizationFlags = uint32_t;

    struct WeatherSettings {
        static constexpr float SUN_MIN_DISTANCE = 1.4712E11f;
        static constexpr float SUN_MAX_DISTANCE = 1.5112E11f;
        static constexpr float SUN_RADIUS = 1392684000.0f;
        static constexpr float PLANET_RADIUS = 6371E3f;
        static constexpr float ATMOS_RADIUS = 6471e3f;

        static constexpr float ATMOSPHERE_RADIUS_OUTER = 50000.0f;

        float timeOfDay = 12.0f;
        float moonBrightness = 0.6f;
        float nightGlow = 0.025f;
        float skyViewBrightness = 0.7f;
        float worldScale = 100.0f;
        float maxAerialPerspective = 200.0f;
        BASE_NS::Math::Vec4 groundColor = { 0.3f, 0.3f, 0.3f, 0.f };

        BASE_NS::Math::Vec3 rayleighScatteringBase = { 5.802f, 13.558f, 33.1f };
        BASE_NS::Math::Vec3 ozoneAbsorptionBase = { 0.650f, 1.881f, 0.085f };
        float mieScatteringBase = 3.996f;
        float mieAbsorptionBase = 4.4f;

        BASE_NS::Math::Vec3 windDir = { 1.0f, 0.0f, 1.0f };
        float windSpeed = 100.0f;

        float coverage = 0.3f;
        float curliness = 4.0f;
        float crispiness = 3.0f;
        float cloudType = 1.0f;

        float density = 0.004f;
        float cloudHorizonFogHeight = 1.5f;
        float horizonFogFalloff = 1.0f;

        BASE_NS::Math::Vec4 ambientColorHueSunset = { 1.0f, 0.64f, 0.33f, 1.12f };
        BASE_NS::Math::Vec3 dayTopAmbientColor = { 1.0f, 1.0f, 1.0f };
        BASE_NS::Math::Vec4 directColorHueSunset = { 1.0f, 0.52f, 0.21f, 1.30f };

        BASE_NS::Math::Vec4 sunGlareColorDay = { 1.0f, 0.40f, 0.09f, 1.10f };
        BASE_NS::Math::Vec4 sunGlareColorSunset = { 1.0f, 0.44f, 0.11f, 0.90f };

        // --- Night / Moon params
        float ambientIntensityNight = 1.0f;      // Ambient at night
        float directLightIntensityNight = 0.60f; // Moon direct scale

        BASE_NS::Math::Vec3 nightTopAmbientColor = { 0.53f, 0.61f, 0.82f };
        BASE_NS::Math::Vec3 nightBottomAmbientColor = { 0.1f, 0.17f, 0.37f }; // Ground bounce at night

        BASE_NS::Math::Vec3 moonBaseColor = { 0.53f, 0.52f, 0.74f }; // Moonlight tint
        BASE_NS::Math::Vec4 moonGlareColor = { 0.95f, 1.0f, 1.0f, 0.6f };

        float cloudBottomAltitude = 4000.0f;
        float cloudTopAltitude = 12000.0f;
        float earthRadius = 6371000.0f;

        BASE_NS::Math::Vec4 sunDirElevation = { 0.0f, 0.0f, 0.0f, 0.0f };
        uint64_t sunId = CORE_NS::INVALID_ENTITY;
        float cloudTopOffset = 750.0f;
        float ambientIntensityDay = 1.35f;
        float softness = 1.4f;

        float weatherScale = 200.0f;
        float directIntensityDay = 1.3f;

        float maxSamples = 64.0f;

        CloudRenderingType cloudRenderingType { CloudRenderingType::REPROJECTED };

        CloudOptimizationFlags cloudOptimizationFlags { 0U };

        RENDER_NS::RenderHandle lowFrequencyImage;
        RENDER_NS::RenderHandle highFrequencyImage;
        RENDER_NS::RenderHandle curlNoiseImage;
        RENDER_NS::RenderHandle cirrusImage;
        RENDER_NS::RenderHandle weatherMapImage;
    };
    void SetWeatherSettings(uint64_t id, const WeatherSettings& settings);
    WeatherSettings GetWeatherSettings() const;

    // for water effects
    struct WaterEffectData {
        uint64_t id { 0xFFFFFFFFffffffff };
        BASE_NS::Math::Vec2 planeOffset { 0.0f, 0.0f };
        RENDER_NS::RenderHandle texture0;
        RENDER_NS::RenderHandle texture1;
        RENDER_NS::RenderHandle argsBuffer;
        uint32_t curIdx { 0U };
        bool isInitialized { false };
    };
    void SetWaterEffect(const WaterEffectData& waterEffectData);
    BASE_NS::array_view<const WaterEffectData> GetWaterEffectData() const;

    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(RENDER_NS::IRenderContext& renderContext, const char* name);
    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    // IRenderDataStore
    void PreRender() override;
    // clear in post render
    void PostRender() override;
    void Clear() override;

    void PreRenderBackend() override {}
    void PostRenderBackend() override {}

    uint32_t GetFlags() const override
    {
        return 0;
    }

    BASE_NS::string_view GetTypeName() const override
    {
        return TYPE_NAME;
    }

    BASE_NS::string_view GetName() const override
    {
        return name_;
    }

    const BASE_NS::Uid& GetUid() const override
    {
        return UID;
    }

private:
    const BASE_NS::string name_;
    std::atomic_int32_t refcnt_ { 0 };
    uint32_t targetIndex_ { 0 };

    BASE_NS::vector<WaterEffectData> waterEffectData_;

    WeatherSettings settings_ {};
    RENDER_NS::RenderHandleReference weatherMapTex_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_DATASTORE_RENDER_DATA_STORE_WEATHER_H
