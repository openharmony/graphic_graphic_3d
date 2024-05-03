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

#ifndef SHADERS__COMMON__3D_DM_INPLACE_FOG_COMMON_H
#define SHADERS__COMMON__3D_DM_INPLACE_FOG_COMMON_H

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"

#ifdef VULKAN

struct FogLayer {
    float density;
    float fallOff;
    float offset;
};

float CalculateLineIntegral(in FogLayer layer, in float rayDirY, in float rayOrigY)
{
    const float epsilon = 0.005;

    const float exponent = max(-127.0f, layer.fallOff * (rayOrigY - layer.offset));

    const float falloff = max(-127.0f, layer.fallOff * rayDirY);

    const float lineIntegral = (1.0f - exp2(-falloff)) / falloff;
    const float lineIntegralTaylor = log(2.0) - (0.5 * (log(2.0) * log(2.0))) * falloff; // Taylor expansion around 0

    return layer.density * exp2(-exponent) * (abs(falloff) > epsilon ? lineIntegral : lineIntegralTaylor);
}

void InplaceFogBlock(in uint flags, in vec3 worldPos, in vec3 camWorldPos, in vec4 inCol, out vec3 outCol)
{
    outCol = inCol.rgb;
    if ((flags & CORE_CAMERA_FOG_BIT) == CORE_CAMERA_FOG_BIT) {
        const vec3 cameraToPointVector = worldPos - camWorldPos;

        float cameraToPointY = cameraToPointVector.y;
        float cameraY = camWorldPos.y;

        const float cameraToPointLengthSqr = dot(cameraToPointVector, cameraToPointVector);
        const float cameraToPointLengthInv = inversesqrt(cameraToPointLengthSqr);
        float worldDistanceToCamera = cameraToPointLengthSqr * cameraToPointLengthInv;

        const float minDistance = uFogData.baseFactors.x;
        if (minDistance > 0) {
            // recalculate fog starting point when minDistance is valid
            const float minIntersectionT = minDistance * cameraToPointLengthInv;
            const float cameraToMinIntersectionY = minIntersectionT * cameraToPointY;
            // shift the values accordingly
            cameraToPointY = cameraToPointY - cameraToMinIntersectionY;
            cameraY = cameraY + cameraToMinIntersectionY;
            worldDistanceToCamera = (1.0f - minIntersectionT) * worldDistanceToCamera;
        }

        const FogLayer L1 = FogLayer(uFogData.firstLayer.x, uFogData.firstLayer.y, uFogData.firstLayer.z);
        const float layer1Integral = CalculateLineIntegral(L1, cameraToPointY, cameraY);

        const FogLayer L2 = FogLayer(uFogData.secondLayer.x, uFogData.secondLayer.y, uFogData.secondLayer.z);
        const float layer2Integral = CalculateLineIntegral(L2, cameraToPointY, cameraY);

        const float exponentialHeightLineIntegral = (layer1Integral + layer2Integral) * worldDistanceToCamera;

        const float maxOpacity = uFogData.baseFactors.z;
        float expFogFactor = min(1.0 - clamp(exp2(-exponentialHeightLineIntegral), 0.0, 1.0), maxOpacity);

        const float cutOffDistance = uFogData.baseFactors.y;
        if (cutOffDistance > 0 && worldDistanceToCamera > cutOffDistance) {
            expFogFactor = 0;
        }

        const vec4 inscatteringColor = uFogData.inscatteringColor;
        const vec4 envMapColor = uFogData.envMapFactor;
        const vec3 fogColor = inscatteringColor.rgb * inscatteringColor.a + envMapColor.rgb * envMapColor.a;
        const vec3 baseColor = inCol.rgb;

        outCol.rgb = mix(baseColor, fogColor, expFogFactor);
    }
}

#else

#endif

#endif // SHADERS__COMMON__3D_DM_INPLACE_FOG_COMMON_H
