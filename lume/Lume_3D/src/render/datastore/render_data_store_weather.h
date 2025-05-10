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
    struct WeatherSettings {
        static constexpr float SUN_MIN_DISTANCE = 1.4712E11f;
        static constexpr float SUN_MAX_DISTANCE = 1.5112E11f;
        static constexpr float SUN_RADIUS = 1392684000.0f;
        static constexpr float PLANET_RADIUS = 6371E3f;
        static constexpr float ATMOS_RADIUS = 6471e3f;

        static constexpr float ATMOSPHERE_RADIUS_OUTER = 50000.0f;

        BASE_NS::Math::Vec3 windDir = { 1.0f, 0.0f, 1.0f };
        float windSpeed = 1000.0f;

        float coverage = 0.0f;
        float precipitation = -1.0f;
        float cloudType = 0.0f;

        float density = 0.2f;
        float altitude = 0.0f;

        /* Not really  used */
        BASE_NS::Math::Vec3 sunColor = { 1.0f, 1.0f, 0.0f };
        BASE_NS::Math::Vec3 ambientColor = { 1.0f, 0.0f, 0.0f };
        BASE_NS::Math::Vec3 atmosphereColor = { 0.6f, 0.7f, 0.95f };

        BASE_NS::Math::Vec3 sunPos = { 0.0f, SUN_MAX_DISTANCE, 0.0f };

        float silverIntensity = { 0.2f };
        float silverSpread = { 1.0f };
        float direct = { 1.0f };
        float ambient = { 1.0f };
        float lightStep = { 0.021f };
        float scatteringDist { 1.0f };

        float domain[5] = { 58000.0f, 14909.0f, 50681.0f, 1136364.0f, 10000.0f };
        float anvil_bias = { 0.13f };
        float brightness = { 1.0f };
        float eccintricity = { 0.6f };

        CloudRenderingType cloudRenderingType { CloudRenderingType::REPROJECTED };

        RENDER_NS::RenderHandle target0;
        RENDER_NS::RenderHandle target1;

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
    };
    void SetWaterEffect(uint64_t id, BASE_NS::Math::Vec2 offset);
    BASE_NS::array_view<const WaterEffectData> GetWaterEffectData() const;

    static BASE_NS::refcnt_ptr<IRenderDataStore> Create(RENDER_NS::IRenderContext& renderContext, const char* name);
    void Ref() override;
    void Unref() override;
    int32_t GetRefCount() override;

    // IRenderDataStore
    void PreRender() override {}
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
    const RENDER_NS::IRenderContext& renderContex_;

    BASE_NS::vector<WaterEffectData> waterEffectData_;

    WeatherSettings settings_ {};
    RENDER_NS::RenderHandleReference weatherMapTex_;
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_DATASTORE_RENDER_DATA_STORE_WEATHER_H
