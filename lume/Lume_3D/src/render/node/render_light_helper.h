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

#ifndef CORE3D_RENDER__NODE__RENDER_LIGHT_HELPER_H
#define CORE3D_RENDER__NODE__RENDER_LIGHT_HELPER_H

// NOTE: do not include this helper file in header

#include <algorithm>

#include <3d/namespace.h>
#include <3d/render/intf_render_data_store_default_light.h>

CORE3D_BEGIN_NAMESPACE()
class RenderLightHelper final {
public:
    RenderLightHelper() = default;
    ~RenderLightHelper() = default;

    // NOTE: temporary, no cluster calculations yet
    static constexpr uint32_t DEFAULT_CLUSTER_INDEX_COUNT { 256u };
    // offset to DefaultMaterialSingleLightStruct
    static constexpr uint32_t LIGHT_LIST_OFFSET { 16u * 6u };

    struct LightCounts {
        uint32_t directionalLightCount { 0u };
        uint32_t pointLightCount { 0u };
        uint32_t spotLightCount { 0u };
    };

    static BASE_NS::Math::Vec4 GetShadowAtlasSizeInvSize(const IRenderDataStoreDefaultLight& dsLight)
    {
        const BASE_NS::Math::UVec2 shadowQualityRes = dsLight.GetShadowQualityResolution();
        const uint32_t shadowCount = dsLight.GetLightCounts().shadowCount;
        BASE_NS::Math::Vec2 size = { float(shadowQualityRes.x * shadowCount), float(shadowQualityRes.y) };
        size.x = BASE_NS::Math::max(1.0f, size.x);
        size.y = BASE_NS::Math::max(1.0f, size.y);
        return { size.x, size.y, 1.0f / size.x, 1.0f / size.y };
    }

    struct SortData {
        RenderLight::LightUsageFlags lightUsageFlags { 0u };
        uint32_t index { 0u };
    };

    static BASE_NS::vector<SortData> SortLights(
        const BASE_NS::array_view<const RenderLight> lights, const uint32_t lightCount)
    {
        BASE_NS::vector<SortData> sortedFlags(lightCount);
        for (uint32_t idx = 0; idx < lightCount; ++idx) {
            sortedFlags[idx].lightUsageFlags = lights[idx].lightUsageFlags;
            sortedFlags[idx].index = idx;
        }
        std::sort(sortedFlags.begin(), sortedFlags.end(), [](const auto& lhs, const auto& rhs) {
            return ((lhs.lightUsageFlags & 0x7u) < (rhs.lightUsageFlags & 0x7u));
        });
        return sortedFlags;
    }

    static void CopySingleLight(
        const RenderLight& currLight, const uint32_t shadowCount, DefaultMaterialSingleLightStruct* memLight)
    {
        const float shadowStepSize = 1.0f / BASE_NS::Math::max(1.0f, static_cast<float>(shadowCount));
        const BASE_NS::Math::Vec4 pos = currLight.pos;
        const BASE_NS::Math::Vec4 dir = currLight.dir;
        memLight->pos = pos;
        memLight->dir = dir;
        constexpr float epsilonForMinDivisor { 0.0001f };
        memLight->dir.w = BASE_NS::Math::max(epsilonForMinDivisor, currLight.range);
        memLight->color =
            BASE_NS::Math::Vec4(BASE_NS::Math::Vec3(currLight.color) * currLight.color.w, currLight.color.w);
        memLight->spotLightParams = currLight.spotLightParams;
        memLight->shadowFactors = { currLight.shadowFactors.x, currLight.shadowFactors.y, currLight.shadowFactors.z,
            shadowStepSize };
        memLight->flags = { currLight.lightUsageFlags, currLight.shadowCameraIndex, currLight.shadowIndex,
            shadowCount };
        memLight->indices = { static_cast<uint32_t>(currLight.id >> 32U),
            static_cast<uint32_t>(currLight.id & 0xFFFFffff), static_cast<uint32_t>(currLight.layerMask >> 32U),
            static_cast<uint32_t>(currLight.layerMask & 0xFFFFffff) };
    }
};
CORE3D_END_NAMESPACE()

#endif // CORE3D_RENDER__NODE__RENDER_LIGHT_HELPER_H
