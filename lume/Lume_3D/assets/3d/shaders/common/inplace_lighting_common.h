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

#ifndef SHADERS__COMMON__INPLANE_LIGHTING_COMMON_H
#define SHADERS__COMMON__INPLANE_LIGHTING_COMMON_H

#include "3d/shaders/common/3d_dm_brdf_common.h"
#include "3d/shaders/common/3d_dm_shadowing_common.h"
#include "3d/shaders/common/3d_dm_lighting_common.h"
// NOTE: inplace
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"

/*
 * Needs to be included after the descriptor sets are bound.
 */

vec3 CalculateLightInplace(uint currLightIdx, vec3 materialDiffuseBRDF, vec3 L, float NoL, ShadingDataInplace sd,
    ClearcoatShadingVariables ccsv, SheenShadingVariables ssv)
{
    const vec3 H = normalize(L + sd.V);
    const float VoH = clamp(dot(sd.V, H), 0.0, 1.0);
    const float NoH = clamp(dot(sd.N, H), 0.0, 1.0);

    float extAttenuation = 1.0;
    vec3 calculatedColor = vec3(0.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHEEN_BIT) == CORE_MATERIAL_SHEEN_BIT) {
        const float sheenD = dCharlie(ssv.sheenRoughness, NoH);
        const float sheenV = vAshikhmin(sd.NoV, NoL);
        const vec3 sheenSpec = ssv.sheenColor * (sheenD * sheenV); // F = 1.0

        extAttenuation *= (1.0 - (ssv.sheenColorMax * ssv.sheenBRDFApprox));
        calculatedColor += (sheenSpec * NoL);
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
        const float ccNoL = clamp(dot(ccsv.ccNormal, L), CORE3D_PBR_LIGHTING_EPSILON, 1.0);
        const float ccNoH = clamp(dot(ccsv.ccNormal, H), CORE3D_PBR_LIGHTING_EPSILON, 1.0);
        const float ccLoH = clamp(dot(L, H), CORE3D_PBR_LIGHTING_EPSILON, 1.0);
        const float ccNoV = clamp(dot(ccsv.ccNormal, sd.V), CORE3D_PBR_LIGHTING_EPSILON, 1.0);
        const float ccf0 = 0.04;

        const float ccD = dGGX(ccsv.ccAlpha2, ccNoH);
        const float ccG = vKelemen(ccLoH);
        const float ccF = fSchlickSingle(ccf0, ccNoV) * ccsv.cc; // NOTE: cc in ccF
        const float ccSpec = ccF * ccD * ccG;

        extAttenuation *= (1.0 - ccF);
        calculatedColor += vec3(ccSpec * ccNoL);
    }
    const float D = dGGX(sd.alpha2, NoH);
    const float G = vGGXWithCombinedDenominator(sd.alpha2, sd.NoV, NoL);
    const vec3 F = fSchlick(sd.f0, VoH);
    const vec3 specContrib = F * (D * G);

    // KHR_materials_specular: "In the diffuse component we have to account for the fact that F is now an RGB value.",
    // but it already was? const vec3 diffuseContrib = (1.0 - max(F.x, (max(F.y, F.z)))) * materialDiffuseBRDF;
    const vec3 diffuseContrib = (1.0 - F.xyz) * materialDiffuseBRDF;
    calculatedColor += (diffuseContrib + specContrib * extAttenuation) * extAttenuation * NoL;
    calculatedColor *= uLightData.lights[currLightIdx].color.xyz;
    return calculatedColor;
}

bool CheckLightLayerMask(uint currLightIdx, uvec2 layers) {
    // Check that the light is enabled for this specific object.
    // TODO: It seems like the mask bits are in .wz order when it should be .zw?
    const uvec2 lightLayerMask = uLightData.lights[currLightIdx].indices.wz;
    // If any of the layer bits match the light layer mask -> return true (i.e. use this light).
    return (layers & lightLayerMask) != uvec2(0, 0);
}

vec3 CalculateLightingInplace(ShadingDataInplace sd, ClearcoatShadingVariables ccsv, SheenShadingVariables ssv)
{
    const vec3 materialDiffuseBRDF = sd.diffuseColor * diffuseCoeff();
    vec3 color = vec3(0.0);
    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    const vec4 atlasSizeInvSize = uLightData.atlasSizeInvSize;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;

        if (!CheckLightLayerMask(currLightIdx, sd.layers)) {
            continue;
        }

        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
        // NOTE: could check for NoL > 0.0 and NoV > 0.0
        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
            const uvec4 lightFlags = uLightData.lights[currLightIdx].flags;
            if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(sd.pos.xyz, 1.0);
                const vec4 shadowFactors = uLightData.lights[currLightIdx].shadowFactors;
                if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) == CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) {
                    shadowCoeff = CalcVsmShadow(
                        uSampColorShadow, shadowCoord, NoL, shadowFactors, atlasSizeInvSize, lightFlags.zw);
                } else {
                    shadowCoeff = CalcPcfShadow(
                        uSampDepthShadow, shadowCoord, NoL, shadowFactors, atlasSizeInvSize, lightFlags.zw);
                }
            }
        }
        color += CalculateLightInplace(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv) * shadowCoeff;
    }

    if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SPOT_ENABLED_BIT) == CORE_LIGHTING_SPOT_ENABLED_BIT) {
        const uint spotLightCount = uLightData.spotLightCount;
        const uint spotLightLightBeginIndex = uLightData.spotLightBeginIndex;
        for (uint spotIdx = 0; spotIdx < spotLightCount; ++spotIdx) {
            const uint currLightIdx = spotLightLightBeginIndex + spotIdx;

            if (!CheckLightLayerMask(currLightIdx, sd.layers)) {
                continue;
            }

            const vec3 pointToLight = uLightData.lights[currLightIdx].pos.xyz - sd.pos.xyz;
            const float dist = length(pointToLight);
            const vec3 L = pointToLight / dist;
            const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
            // NOTE: could check for NoL > 0.0 and NoV > 0.0
            CORE_RELAXEDP float shadowCoeff = 1.0;
            if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
                const uvec4 lightFlags = uLightData.lights[currLightIdx].flags;
                if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                    const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(sd.pos.xyz, 1.0);
                    const vec4 shadowFactors = uLightData.lights[currLightIdx].shadowFactors;
                    if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) ==
                        CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) {
                        shadowCoeff = CalcVsmShadow(
                            uSampColorShadow, shadowCoord, NoL, shadowFactors, atlasSizeInvSize, lightFlags.zw);
                    } else {
                        shadowCoeff = CalcPcfShadow(
                            uSampDepthShadow, shadowCoord, NoL, shadowFactors, atlasSizeInvSize, lightFlags.zw);
                    }
                }
            }

            const float lightAngleScale = uLightData.lights[currLightIdx].spotLightParams.x;
            const float lightAngleOffset = uLightData.lights[currLightIdx].spotLightParams.y;
            // See: https://github.com/KhronosGroup/glTF/tree/master/extensions/2.0/Khronos/KHR_lights_punctual
            const float cd = dot(uLightData.lights[currLightIdx].dir.xyz, -L);
            const float angularAttenuation = clamp(cd * lightAngleScale + lightAngleOffset, 0.0, 1.0);

            const float range = uLightData.lights[currLightIdx].dir.w;
            const float attenuation = max(min(1.0 - pow(dist / range, 4.0), 1.0), 0.0) / (dist * dist);
            color += CalculateLightInplace(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv) *
                     (angularAttenuation * angularAttenuation * attenuation) * shadowCoeff;
        }
    }
    if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_POINT_ENABLED_BIT) == CORE_LIGHTING_POINT_ENABLED_BIT) {
        const uint pointLightCount = uLightData.pointLightCount;
        const uint pointLightBeginIndex = uLightData.pointLightBeginIndex;
        for (uint pointIdx = 0; pointIdx < pointLightCount; ++pointIdx) {
            const uint currLightIdx = pointLightBeginIndex + pointIdx;

            if (!CheckLightLayerMask(currLightIdx, sd.layers)) {
                continue;
            }

            const vec3 pointToLight = uLightData.lights[currLightIdx].pos.xyz - sd.pos.xyz;
            const float dist = length(pointToLight);
            if (dist == 0) {
                return color;
            }
            const vec3 L = pointToLight / dist;
            const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
            const float range = uLightData.lights[currLightIdx].dir.w;
            const float attenuation = max(min(1.0 - pow(dist / range, 4.0), 1.0), 0.0) / (dist * dist);
            // NOTE: could check for NoL > 0.0 and NoV > 0.0
            color += CalculateLightInplace(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv) * attenuation;
        }
    }

    return color;
}

#endif // SHADERS__COMMON__INPLANE_LIGHTING_COMMON_H
