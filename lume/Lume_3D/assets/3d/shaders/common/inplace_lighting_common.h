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

#ifndef SHADERS__COMMON__INPLANE_LIGHTING_COMMON_H
#define SHADERS__COMMON__INPLANE_LIGHTING_COMMON_H

#include "3d/shaders/common/3d_dm_shadowing_common.h"
#include "3d/shaders/common/3d_dm_brdf_common.h"
// NOTE: inplace
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"

/*
 * Needs to be included after the descriptor sets are bound.
 */

#define CORE_PBR_LIGHTING_EPSILON 0.0001

struct InputBrdfData {
    CORE_RELAXEDP vec4 f0;
    CORE_RELAXEDP vec3 diffuseColor;
    CORE_RELAXEDP float roughness;
    CORE_RELAXEDP float alpha2;
};

struct ShadingData {
    vec3 pos;
    vec3 N;
    float NoV;
    vec3 V;
    float alpha2;
    vec4 f0; // f0 = f0.xyz, f90 = f0.w
    vec3 diffuseColor;
    uint materialFlags;
};

struct ClearcoatShadingVariables {
    vec3 ccNormal;
    CORE_RELAXEDP float cc;
    CORE_RELAXEDP float ccRoughness;
    CORE_RELAXEDP float ccAlpha2;
};

struct SheenShadingVariables {
    CORE_RELAXEDP vec3 sheenColor;
    CORE_RELAXEDP float sheenRoughness;
    CORE_RELAXEDP float sheenColorMax;
    CORE_RELAXEDP float sheenBRDFApprox;
};

#define CORE_GEOMETRIC_SPECULAR_AA_ROUGHNESS 1

InputBrdfData GetInputBRDF(vec4 baseColor, vec3 polygonNormal, vec4 uv, uint instanceIdx)
{
    const CORE_RELAXEDP vec4 material =
        GetMaterialSample(uv, instanceIdx) * GetUnpackMaterial(uMaterialData, instanceIdx);
    InputBrdfData bd;
    if (CORE_MATERIAL_TYPE == CORE_MATERIAL_SPECULAR_GLOSSINESS) {
        bd.f0.xyz = material.xyz * baseColor.a; // f0 reflectance multiplied for transparent
        bd.f0.w = 1.0;                          // f90
        bd.diffuseColor = baseColor.rgb * (1.0 - max(bd.f0.x, max(bd.f0.y, bd.f0.z)));
        bd.roughness = 1.0 - clamp(material.a, 0.0, 1.0);
    } else if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SPECULAR_BIT) == CORE_MATERIAL_SPECULAR_BIT) {
        const CORE_RELAXEDP float metallic = clamp(material.b, 0.0, 1.0);
        // start with metal-rough dielectricSpecularF0:
        // f0 reflectance multiplied for transparents
        bd.f0.xyz = mix(vec3(material.a), baseColor.rgb, metallic) * baseColor.a;

        // tint by specular color and multiply by strength:
        const CORE_RELAXEDP vec4 specular =
            GetSpecularSample(uv, instanceIdx) * GetUnpackSpecular(uMaterialData, instanceIdx);
        bd.f0.xyz = min(bd.f0.xyz * specular.rgb, vec3(1.0)) * specular.a;

        // spec: f0, glTF-Sample-Viewer: dielectricSpecularF0. later would be closer to the metal-rough case
        bd.diffuseColor = mix(baseColor.rgb * (1.0 - max(bd.f0.x, max(bd.f0.y, bd.f0.z))), vec3(0.0), vec3(metallic));

        // final f0
        bd.f0.xyz = mix(bd.f0.xyz, baseColor.rgb, metallic);
        bd.f0.w = mix(specular.a, 1.0, metallic); // f90
        bd.roughness = material.g;
    } else {
        const CORE_RELAXEDP float metallic = clamp(material.b, 0.0, 1.0);
        // f0 reflectance multiplied for transparents
        bd.f0.xyz = mix(vec3(material.a), baseColor.rgb, metallic) * baseColor.a;
        bd.f0.w = 1.0; // f90
        // spec and glTF-Sample-Viewer don't use f0 here, but reflectance:
        bd.diffuseColor = mix(baseColor.rgb * (1.0 - bd.f0.xyz), vec3(0.0), vec3(metallic));
        bd.roughness = material.g;
    }
    // NOTE: diffuse color is already premultiplied

#if (CORE_GEOMETRIC_SPECULAR_AA_ROUGHNESS == 1)
    // reduce shading aliasing by increasing roughness based on the curvature of the geometry
    const vec3 normalDFdx = dFdx(polygonNormal);
    const vec3 normalDdFdy = dFdy(polygonNormal);
    const float geometricRoughness =
        pow(clamp(max(dot(normalDFdx, normalDFdx), dot(normalDdFdy, normalDdFdy)), 0.0, 1.0), 0.333);
    bd.roughness = max(bd.roughness, geometricRoughness);
#endif
    bd.roughness = clamp(bd.roughness, CORE_BRDF_MIN_ROUGHNESS, 1.0);
    const CORE_RELAXEDP float alpha = bd.roughness * bd.roughness;
    bd.alpha2 = alpha * alpha;

    return bd;
}

mat4 GetShadowMatrix(const uint shadowCamIdx)
{
    return uCameras[shadowCamIdx].shadowViewProj;
}

vec3 CalculateLight(uint currLightIdx, vec3 materialDiffuseBRDF, vec3 L, float NoL, ShadingData sd,
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
        const float ccNoL = clamp(dot(ccsv.ccNormal, L), CORE_PBR_LIGHTING_EPSILON, 1.0);
        const float ccNoH = clamp(dot(ccsv.ccNormal, H), 0.0, 1.0);
        const float ccf0 = 0.04;

        const float ccD = dGGX(ccsv.ccAlpha2, ccNoH);
        const float ccG = vKelemen(ccNoH);
        const float ccF = fSchlickSingle(ccf0, VoH) * ccsv.cc;
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

vec3 CalculateLighting(ShadingData sd, ClearcoatShadingVariables ccsv, SheenShadingVariables ssv)
{
    const vec3 materialDiffuseBRDF = sd.diffuseColor * diffuseCoeff();
    vec3 color = vec3(0.0);
    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
        // NOTE: could check for NoL > 0.0 and NoV > 0.0
        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
            const uvec2 lightFlags = uLightData.lights[currLightIdx].flags.xy;
            if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(sd.pos.xyz, 1.0);
                const vec4 shadowFactors = uLightData.lights[currLightIdx].shadowFactors;
                if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) == CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) {
                    shadowCoeff = CalcVsmShadow(uSampColorShadow, shadowCoord, NoL, shadowFactors);
                } else {
                    shadowCoeff = CalcPcfShadow(uSampDepthShadow, shadowCoord, NoL, shadowFactors);
                }
            }
        }
        color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv) * shadowCoeff;
    }

    if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SPOT_ENABLED_BIT) == CORE_LIGHTING_SPOT_ENABLED_BIT) {
        const uint spotLightCount = uLightData.spotLightCount;
        const uint spotLightLightBeginIndex = uLightData.spotLightBeginIndex;
        for (uint spotIdx = 0; spotIdx < spotLightCount; ++spotIdx) {
            const uint currLightIdx = spotLightLightBeginIndex + spotIdx;

            const vec3 pointToLight = uLightData.lights[currLightIdx].pos.xyz - sd.pos.xyz;
            const float dist = length(pointToLight);
            const vec3 L = pointToLight / dist;
            const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
            // NOTE: could check for NoL > 0.0 and NoV > 0.0
            CORE_RELAXEDP float shadowCoeff = 1.0;
            if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
                const uvec2 lightFlags = uLightData.lights[currLightIdx].flags.xy;
                if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                    const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(sd.pos.xyz, 1.0);
                    const vec4 shadowFactors = uLightData.lights[currLightIdx].shadowFactors;
                    if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) ==
                        CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) {
                        shadowCoeff = CalcVsmShadow(uSampColorShadow, shadowCoord, NoL, shadowFactors);
                    } else {
                        shadowCoeff = CalcPcfShadow(uSampDepthShadow, shadowCoord, NoL, shadowFactors);
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv) *
                (angularAttenuation * angularAttenuation * attenuation) * shadowCoeff;
        }
    }
    if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_POINT_ENABLED_BIT) == CORE_LIGHTING_POINT_ENABLED_BIT) {
        const uint pointLightCount = uLightData.pointLightCount;
        const uint pointLightBeginIndex = uLightData.pointLightBeginIndex;
        for (uint pointIdx = 0; pointIdx < pointLightCount; ++pointIdx) {
            const uint currLightIdx = pointLightBeginIndex + pointIdx;

            const vec3 pointToLight = uLightData.lights[currLightIdx].pos.xyz - sd.pos.xyz;
            const float dist = length(pointToLight);
            const vec3 L = pointToLight / dist;
            const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
            const float range = uLightData.lights[currLightIdx].dir.w;
            const float attenuation = max(min(1.0 - pow(dist / range, 4.0), 1.0), 0.0) / (dist * dist);
            // NOTE: could check for NoL > 0.0 and NoV > 0.0
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv) * attenuation;
        }
    }

    return color;
}

#endif // SHADERS__COMMON__INPLANE_LIGHTING_COMMON_H
