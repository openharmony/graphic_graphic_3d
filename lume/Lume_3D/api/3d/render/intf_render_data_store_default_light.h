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

#ifndef API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_LIGHT_H
#define API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_LIGHT_H

#include <cstdint>

#include <3d/namespace.h>
#include <base/containers/array_view.h>
#include <base/math/vector.h>
#include <base/util/uid.h>
#include <core/namespace.h>
#include <render/datastore/intf_render_data_store.h>

CORE3D_BEGIN_NAMESPACE()
struct RenderLight;
/** @ingroup group_render_irenderdatastoredefaultlight */
/** Interface to add lights to rendering.
    Not internally syncronized.
*/
class IRenderDataStoreDefaultLight : public RENDER_NS::IRenderDataStore {
public:
    static constexpr BASE_NS::Uid UID { "daa1bcf3-e1b7-4605-8c2a-01a6e6f5dbb6" };

    ~IRenderDataStoreDefaultLight() override = default;

    /** Shadow type. Default is PCF.
     */
    enum class ShadowType : uint8_t {
        /* Percentage Closer Filtering */
        PCF = 0,
        /* Variance Shadow Maps */
        VSM = 1,
    };

    /** Shadow quality. Default is NORMAL.
     */
    enum class ShadowQuality : uint8_t {
        /* Low quality */
        LOW = 0,
        /* Normal quality */
        NORMAL = 1,
        /* High quality */
        HIGH = 2,
        /* Ultra quality */
        ULTRA = 3,
    };

    /** Shadow smoothness. Default is NORMAL.
     */
    enum class ShadowSmoothness : uint8_t {
        /* Hard shadows */
        HARD = 0,
        /* Normal soft shadows */
        NORMAL = 1,
        /* Very soft shadows */
        SOFT = 2,
    };

    /** Lighting flags for rendering (and specialization)
     */
    enum LightingFlagBits : uint8_t {
        LIGHTING_SHADOW_TYPE_VSM_BIT = (1 << 0),
        LIGHTING_POINT_ENABLED_BIT = (1 << 1),
        LIGHTING_SPOT_ENABLED_BIT = (1 << 2),
    };
    using LightingFlags = uint32_t;

    struct LightCounts {
        uint32_t directional { 0u };
        uint32_t point { 0u };
        uint32_t spot { 0u };

        uint32_t dirShadow { 0u };
        uint32_t spotShadow { 0u };

        uint32_t shadowCount { 0u };
    };

    /** Shadow types.
     */
    struct ShadowTypes {
        ShadowType shadowType { ShadowType::PCF };
        ShadowQuality shadowQuality { ShadowQuality::NORMAL };
        ShadowSmoothness shadowSmoothness { ShadowSmoothness::NORMAL };
    };

    /** Shadow quality resolutions.
     */
    struct ShadowQualityResolutions {
        BASE_NS::Math::UVec2 low { 512u, 512u };
        BASE_NS::Math::UVec2 normal { 1024u, 1024u };
        BASE_NS::Math::UVec2 high { 2048u, 2048u };
        BASE_NS::Math::UVec2 ultra { 4096u, 4096u };
    };

    /** Set shadow type for all shadows.
     * @param shadowTypes Types for all shadows.
     * @param flags Additional flags reserved for future.
     */
    virtual void SetShadowTypes(const ShadowTypes& shadowTypes, const uint32_t flags) = 0;

    /** Get the type of all shadows.
     * @return Shadow type.
     */
    virtual ShadowTypes GetShadowTypes() const = 0;

    /** Set shadow quality resolutions for all shadows.
     * @param resolutions Resolutions for all qualities.
     * @param flags Additional flags reserved for future.
     */
    virtual void SetShadowQualityResolutions(const ShadowQualityResolutions& resolutions, const uint32_t flags) = 0;

    /** Get current shadow quality resolution.
     * @param resolutions Resolutions for all qualities.
     * @return Resolution of current shadows.
     */
    virtual BASE_NS::Math::UVec2 GetShadowQualityResolution() const = 0;

    /** Add light to scene. There can be only DefaultSceneRenderingConstants count of lights.
     * Lights that exceeds the limits are dropped.
     * @param light A light.
     */
    virtual void AddLight(const RenderLight& light) = 0;

    /** Get lights.
     * @return Array view of all lights.
     */
    virtual BASE_NS::array_view<const RenderLight> GetLights() const = 0;

    /** Get light counts.
     * @return Light count struct.
     */
    virtual LightCounts GetLightCounts() const = 0;

    /** Get lighting flags.
     * @return lighting flags.
     */
    virtual LightingFlags GetLightingFlags() const = 0;

protected:
    IRenderDataStoreDefaultLight() = default;
};
CORE3D_END_NAMESPACE()

#endif // API_3D_RENDER_IRENDER_DATA_STORE_DEFAULT_LIGHT_H
