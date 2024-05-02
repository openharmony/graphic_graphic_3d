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

#include "render_data_store_default_light.h"

#include <cstdint>

#include <3d/render/default_material_constants.h>
#include <3d/render/render_data_defines_3d.h>
#include <base/containers/array_view.h>
#include <core/log.h>
#include <render/resource_handle.h>

CORE3D_BEGIN_NAMESPACE()
using namespace BASE_NS;
using namespace RENDER_NS;

RenderDataStoreDefaultLight::RenderDataStoreDefaultLight(const string_view name) : name_(name) {}

void RenderDataStoreDefaultLight::PostRender()
{
    Clear();
}

void RenderDataStoreDefaultLight::Clear()
{
    lights_.clear();
    lightCounts_ = {};
}

void RenderDataStoreDefaultLight::SetShadowTypes(const ShadowTypes& shadowTypes, const uint32_t flags)
{
    shadowTypes_ = shadowTypes;
}

IRenderDataStoreDefaultLight::ShadowTypes RenderDataStoreDefaultLight::GetShadowTypes() const
{
    return shadowTypes_;
}

void RenderDataStoreDefaultLight::SetShadowQualityResolutions(
    const ShadowQualityResolutions& resolutions, const uint32_t flags)
{
    resolutions_ = resolutions;
}

Math::UVec2 RenderDataStoreDefaultLight::GetShadowQualityResolution() const
{
    if (shadowTypes_.shadowQuality == ShadowQuality::LOW) {
        return resolutions_.low;
    } else if (shadowTypes_.shadowQuality == ShadowQuality::NORMAL) {
        return resolutions_.normal;
    } else if (shadowTypes_.shadowQuality == ShadowQuality::HIGH) {
        return resolutions_.high;
    } else {
        return resolutions_.ultra;
    }
}

void RenderDataStoreDefaultLight::AddLight(const RenderLight& light)
{
    // drop light that has all components below zero (negative light)
    constexpr float lightIntensityEpsilon { 0.0001f };
    if (((light.color.x < 0.0f) && (light.color.y < 0.0f) && (light.color.z < 0.0f)) ||
        (light.color.w < lightIntensityEpsilon)) {
        return;
    }

    RenderLight renderLight = light; // copy
    // we do not support negative color values for lights
    renderLight.color.x = Math::max(0.0f, renderLight.color.x);
    renderLight.color.y = Math::max(0.0f, renderLight.color.y);
    renderLight.color.z = Math::max(0.0f, renderLight.color.z);
    const uint32_t lightCount = lightCounts_.directional + lightCounts_.spot + lightCounts_.point;
#if (CORE3D_VALIDATION_ENABLED == 1)
    if (lightCount >= DefaultMaterialLightingConstants::MAX_LIGHT_COUNT) {
        CORE_LOG_ONCE_W("drop_light_count_", "CORE3D_VALIDATION: light dropped (max count: %u)",
            DefaultMaterialLightingConstants::MAX_LIGHT_COUNT);
    }
#endif
    if (lightCount < DefaultMaterialLightingConstants::MAX_LIGHT_COUNT) {
        if (renderLight.lightUsageFlags & RenderLight::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT) {
            lightCounts_.directional++;
        } else if (renderLight.lightUsageFlags & RenderLight::LIGHT_USAGE_SPOT_LIGHT_BIT) {
            lightCounts_.spot++;
        } else if (renderLight.lightUsageFlags & RenderLight::LIGHT_USAGE_POINT_LIGHT_BIT) {
            lightCounts_.point++;
        }
        if (renderLight.lightUsageFlags & RenderLight::LIGHT_USAGE_SHADOW_LIGHT_BIT) {
            const uint32_t shadowCount = lightCounts_.dirShadow + lightCounts_.spotShadow;
            if (shadowCount < DefaultMaterialLightingConstants::MAX_SHADOW_COUNT) {
                if (renderLight.lightUsageFlags & RenderLight::LIGHT_USAGE_DIRECTIONAL_LIGHT_BIT) {
                    lightCounts_.dirShadow++;
                } else if (renderLight.lightUsageFlags & RenderLight::LIGHT_USAGE_SPOT_LIGHT_BIT) {
                    lightCounts_.spotShadow++;
                }
                renderLight.shadowIndex = shadowCount; // shadow index in atlas
                lightCounts_.shadowCount = lightCounts_.spotShadow + lightCounts_.dirShadow;
            }
        }
        lights_.push_back(move(renderLight));
    }
}

array_view<const RenderLight> RenderDataStoreDefaultLight::GetLights() const
{
    return array_view<const RenderLight>(lights_);
}

IRenderDataStoreDefaultLight::LightCounts RenderDataStoreDefaultLight::GetLightCounts() const
{
    return lightCounts_;
}

IRenderDataStoreDefaultLight::LightingFlags RenderDataStoreDefaultLight::GetLightingFlags() const
{
    LightingFlags lightingSpecializationFlags = 0u;
    if (shadowTypes_.shadowType == IRenderDataStoreDefaultLight::ShadowType::VSM) {
        lightingSpecializationFlags |= LightingFlagBits::LIGHTING_SHADOW_TYPE_VSM_BIT;
    }
    if (lightCounts_.point > 0u) {
        lightingSpecializationFlags |= LightingFlagBits::LIGHTING_POINT_ENABLED_BIT;
    }
    if (lightCounts_.spot > 0u) {
        lightingSpecializationFlags |= LightingFlagBits::LIGHTING_SPOT_ENABLED_BIT;
    }
    return lightingSpecializationFlags;
}

// for plugin / factory interface
RENDER_NS::IRenderDataStore* RenderDataStoreDefaultLight::Create(RENDER_NS::IRenderContext&, char const* name)
{
    // engine not needed
    return new RenderDataStoreDefaultLight(name);
}

void RenderDataStoreDefaultLight::Destroy(IRenderDataStore* instance)
{
    delete static_cast<RenderDataStoreDefaultLight*>(instance);
}
CORE3D_END_NAMESPACE()
