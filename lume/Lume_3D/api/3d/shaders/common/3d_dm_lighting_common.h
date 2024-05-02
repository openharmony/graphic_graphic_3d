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

#ifndef SHADERS__COMMON__3D_DM_LIGHTING_COMMON_H
#define SHADERS__COMMON__3D_DM_LIGHTING_COMMON_H

#include "3d/shaders/common/3d_dm_brdf_common.h"
#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_shadowing_common.h"
#include "render/shaders/common/render_compatibility_common.h"

/*
 * NOTE: Inplace data uCameras from descriptor sets
 * Needs to be included after the descriptor sets are bound.
 */

#define CORE3D_PBR_LIGHTING_EPSILON 0.0001
#define CORE_GEOMETRIC_SPECULAR_AA_ROUGHNESS 1

struct InputBrdfData {
    vec4 f0;
    vec3 diffuseColor;
    float roughness;
    float alpha2;
};

struct ShadingData {
    vec3 pos;
    vec3 N;
    float NoV;
    vec3 V;
    float alpha2;
    vec4 f0; // f0 = f0.xyz, f90 = f0.w
    vec3 diffuseColor;
};

struct ShadingDataInplace {
    vec3 pos;
    vec3 N;
    float NoV;
    vec3 V;
    float alpha2;
    vec4 f0; // f0 = f0.xyz, f90 = f0.w
    vec3 diffuseColor;
    uint materialFlags;
    uvec2 layers;
};

struct ClearcoatShadingVariables {
    vec3 ccNormal;
    float cc;
    CORE_RELAXEDP float ccRoughness;
    float ccAlpha2;
};

struct SheenShadingVariables {
    CORE_RELAXEDP vec3 sheenColor;
    CORE_RELAXEDP float sheenRoughness;
    CORE_RELAXEDP float sheenColorMax;
    CORE_RELAXEDP float sheenBRDFApprox;
};

struct AnisotropicShadingVariables {
    float roughness;
    float alpha;
    float anisotropy;
    float ToV;
    float BoV;
    vec3 anisotropicT;
    vec3 anisotropicB;
};

struct SubsurfaceScatterShadingVariables {
    vec3 scatterColor;
    float scatterDistance;
    float thickness;
};

#define CORE3D_GEOMETRIC_SPECULAR_AA_ROUGHNESS 1

void GetFinalCorrectedRoughness(CORE_RELAXEDP in vec3 polygonNormal, CORE_RELAXEDP inout float roughness)
{
#if (CORE_GEOMETRIC_SPECULAR_AA_ROUGHNESS == 1)
    // reduce shading aliasing by increasing roughness based on the curvature of the geometry
    const CORE_RELAXEDP vec3 normalDFdx = dFdx(polygonNormal);
    const CORE_RELAXEDP vec3 normalDdFdy = dFdy(polygonNormal);
    const CORE_RELAXEDP float geometricRoughness =
        pow(clamp(max(dot(normalDFdx, normalDFdx), dot(normalDdFdy, normalDdFdy)), 0.0, 1.0), 0.333);
    roughness = max(roughness, geometricRoughness);
#endif
    roughness = clamp(roughness, CORE_BRDF_MIN_ROUGHNESS, 1.0);
}

void AppendIndirectSheen(in SheenShadingVariables ssv, in CORE_RELAXEDP vec3 radianceSample,
    in CORE_RELAXEDP float alpha, inout CORE_RELAXEDP vec3 irradiance, inout CORE_RELAXEDP vec3 radiance)
{
    const vec3 sheenF = ssv.sheenColor * ssv.sheenBRDFApprox;
    const float sheenAttenuation = 1.0 - (ssv.sheenColorMax * ssv.sheenBRDFApprox);
    // energy compensation
    irradiance *= sheenAttenuation;
    radiance *= sheenAttenuation;

    radiance += sheenF * radianceSample;
}

void AppendIndirectClearcoat(in ClearcoatShadingVariables ccsv, in CORE_RELAXEDP vec3 radianceSample,
    in CORE_RELAXEDP float alpha, in vec3 V, inout CORE_RELAXEDP vec3 irradiance, inout CORE_RELAXEDP vec3 radiance)
{
    const float ccNoV = clamp(dot(ccsv.ccNormal, V), CORE3D_PBR_LIGHTING_EPSILON, 1.0);

    GetFinalCorrectedRoughness(ccsv.ccNormal, ccsv.ccAlpha2);

    float ccRadianceFactor = EnvBRDFApproxNonmetal(ccsv.ccAlpha2, ccNoV);
    float ccAttenuation = 1.0 - ccRadianceFactor;

    // energy compensation
    irradiance *= ccAttenuation;
    radiance *= ccAttenuation;

    // add clear coat radiance
    radiance += radianceSample * ccRadianceFactor;
}

void AppendIndirectTransmission(in CORE_RELAXEDP vec3 radianceSample, in CORE_RELAXEDP vec3 baseColor,
    in CORE_RELAXEDP float transmission, inout CORE_RELAXEDP vec3 irradiance)
{
    // Approximate double refraction by assuming a solid sphere beneath the point.
    const CORE_RELAXEDP vec3 Ft = radianceSample * baseColor.rgb;
    irradiance *= (1.0 - transmission);
    irradiance = mix(irradiance, Ft, transmission);
}

void GetFinalSampledBrdfGeometricCorrection(in vec3 polygonNormal, inout InputBrdfData ibd)
{
    // NOTE: diffuse color is already premultiplied
    CORE_RELAXEDP float roughness = ibd.roughness;
    GetFinalCorrectedRoughness(polygonNormal, roughness);

    const CORE_RELAXEDP float alpha = roughness * roughness;
    ibd.alpha2 = alpha * alpha;
    ibd.roughness = roughness;
}

void GetFinalSampledBrdfGeometricCorrection(inout InputBrdfData ibd)
{
    const CORE_RELAXEDP float roughness = ibd.roughness;
    const CORE_RELAXEDP float alpha = roughness * roughness;
    ibd.alpha2 = alpha * alpha;
}

InputBrdfData CalcBRDFMetallicRoughness(CORE_RELAXEDP vec4 baseColor, vec3 polygonNormal, CORE_RELAXEDP vec4 material)
{
    InputBrdfData bd;
    {
        const CORE_RELAXEDP float metallic = clamp(material.b, 0.0, 1.0);
        bd.f0.xyz = mix(vec3(material.a), baseColor.rgb, metallic); // f0 reflectance
        bd.f0.w = 1.0;                                              // f90
        // spec and glTF-Sample-Viewer don't use f0 here, but reflectance:
        bd.diffuseColor = mix(baseColor.rgb * (1.0 - bd.f0.xyz), vec3(0.0), vec3(metallic));
        bd.roughness = material.g;
    }
    // NOTE: diffuse color is already premultiplied
    GetFinalSampledBrdfGeometricCorrection(polygonNormal, bd);

    return bd;
}

InputBrdfData CalcBRDFMetallicRoughness(CORE_RELAXEDP vec4 baseColor, CORE_RELAXEDP vec4 material)
{
    InputBrdfData bd;
    {
        const CORE_RELAXEDP float metallic = clamp(material.b, 0.0, 1.0);
        bd.f0.xyz = mix(vec3(material.a), baseColor.rgb, metallic); // f0 reflectance
        bd.f0.w = 1.0;                                              // f90
        // spec and glTF-Sample-Viewer don't use f0 here, but reflectance:
        bd.diffuseColor = mix(baseColor.rgb * (1.0 - bd.f0.xyz), vec3(0.0), vec3(metallic));
        bd.roughness = material.g;
    }
    // NOTE: diffuse color is already premultiplied
    GetFinalSampledBrdfGeometricCorrection(bd);

    return bd;
}

InputBrdfData CalcBRDFSpecularGlossiness(CORE_RELAXEDP vec4 baseColor, vec3 polygonNormal, CORE_RELAXEDP vec4 material)
{
    InputBrdfData bd;
    {
        bd.f0.xyz = material.xyz; // f0 reflectance
        bd.f0.w = 1.0;            // f90
        bd.diffuseColor = baseColor.rgb * (1.0 - max(bd.f0.x, max(bd.f0.y, bd.f0.z)));
        bd.roughness = 1.0 - clamp(material.a, 0.0, 1.0);
    }
    // NOTE: diffuse color is already premultiplied
    GetFinalSampledBrdfGeometricCorrection(polygonNormal, bd);

    return bd;
}

InputBrdfData CalcBRDFSpecular(
    CORE_RELAXEDP vec4 baseColor, vec3 polygonNormal, CORE_RELAXEDP vec4 material, CORE_RELAXEDP vec4 specular)
{
    InputBrdfData bd;
    {
        const CORE_RELAXEDP float metallic = clamp(material.b, 0.0, 1.0);
        // start with metal-rough dielectricSpecularF0:
        bd.f0.xyz = mix(vec3(material.a), baseColor.rgb, metallic); // f0 reflectance

        // tint by specular color and multiply by strength:
        bd.f0.xyz = min(bd.f0.xyz * specular.rgb, vec3(1.0)) * specular.a;

        // spec: f0, glTF-Sample-Viewer: dielectricSpecularF0. later would be closer to the metal-rough case
        bd.diffuseColor = mix(baseColor.rgb * (1.0 - max(bd.f0.x, max(bd.f0.y, bd.f0.z))), vec3(0.0), vec3(metallic));

        // final f0
        bd.f0.xyz = mix(bd.f0.xyz, baseColor.rgb, metallic);
        bd.f0.w = mix(specular.a, 1.0, metallic); // f90
        bd.roughness = material.g;
    }
    // NOTE: diffuse color is already premultiplied
    GetFinalSampledBrdfGeometricCorrection(polygonNormal, bd);

    return bd;
}

mat3 CalcTbnMatrix(in vec3 polygonNormal, in vec4 tangentW)
{
    const vec3 tangent = normalize(tangentW.xyz);
    const vec3 bitangent = cross(polygonNormal, tangent.xyz) * tangentW.w;
    return mat3(tangent.xyz, bitangent.xyz, polygonNormal);
}

vec3 CalcFinalNormal(in mat3 tbn, in vec3 normal, in float normalScale)
{
    vec3 n = normalize((2.0 * normal - 1.0) * vec3(normalScale, normalScale, 1.0f));
    return normalize(tbn * n);
}

vec3 GetAnistropicReflectionVector(const vec3 V, const vec3 N, AnisotropicShadingVariables asv)
{
    // bent towards aniso direction should be re-evaluated with reference (based on filament)
    const vec3 anisoDir = asv.anisotropy >= 0.0 ? asv.anisotropicB : asv.anisotropicT;
    const vec3 anisoTangent = cross(anisoDir, V);
    const vec3 anisoNormal = cross(anisoTangent, anisoDir);
    // creates low bend factors for smooth surfaces
    const float bendFactor = abs(asv.anisotropy) * clamp(asv.roughness * 5.0, 0.0, 1.0);
    const vec3 bentNormal = normalize(mix(N, anisoNormal, bendFactor));

    return reflect(-V, bentNormal);
}

mat4 GetShadowMatrix(const uint shadowCamIdx)
{
    // NOTE: uCameras needs to be visible in the main shader which includes this file
    return uCameras[shadowCamIdx].shadowViewProj;
}

vec3 CalculateLight(
    uint currLightIdx, vec3 materialDiffuseBRDF, vec3 L, float NoL, ShadingData sd, const uint materialFlags)
{
    const vec3 H = normalize(L + sd.V);
    const float VoH = clamp(dot(sd.V, H), 0.0, 1.0);
    const float NoH = clamp(dot(sd.N, H), 0.0, 1.0);

    float extAttenuation = 1.0;
    vec3 calculatedColor = vec3(0.0);
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

vec3 CalculateLight(uint currLightIdx, vec3 materialDiffuseBRDF, vec3 L, float NoL, ShadingData sd,
    ClearcoatShadingVariables ccsv, SheenShadingVariables ssv, const uint materialFlags)
{
    const vec3 H = normalize(L + sd.V);
    const float VoH = clamp(dot(sd.V, H), 0.0, 1.0);
    const float NoH = clamp(dot(sd.N, H), 0.0, 1.0);

    float extAttenuation = 1.0;
    vec3 calculatedColor = vec3(0.0);
    if ((materialFlags & CORE_MATERIAL_SHEEN_BIT) == CORE_MATERIAL_SHEEN_BIT) {
        const float sheenD = dCharlie(ssv.sheenRoughness, NoH);
        const float sheenV = vAshikhmin(sd.NoV, NoL);
        const vec3 sheenSpec = ssv.sheenColor * (sheenD * sheenV); // F = 1.0

        extAttenuation *= (1.0 - (ssv.sheenColorMax * ssv.sheenBRDFApprox));
        calculatedColor += (sheenSpec * NoL);
    }
    if ((materialFlags & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
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

vec3 CalculateLighting(ShadingData sd, const uint materialFlags)
{
    const vec3 materialDiffuseBRDF = sd.diffuseColor * diffuseCoeff();
    vec3 color = vec3(0.0);
    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    const vec4 atlasSizeInvSize = uLightData.atlasSizeInvSize;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
        // NOTE: could check for NoL > 0.0 and NoV > 0.0
        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
        color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, materialFlags) * shadowCoeff;
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
            if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, materialFlags) *
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, materialFlags) * attenuation;
        }
    }

    return color;
}

vec3 CalculateLighting(
    ShadingData sd, ClearcoatShadingVariables ccsv, SheenShadingVariables ssv, const uint materialFlags)
{
    const vec3 materialDiffuseBRDF = sd.diffuseColor * diffuseCoeff();
    vec3 color = vec3(0.0);
    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    const vec4 atlasSizeInvSize = uLightData.atlasSizeInvSize;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
        // NOTE: could check for NoL > 0.0 and NoV > 0.0
        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
        color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv, materialFlags) * shadowCoeff;
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
            if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv, materialFlags) *
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
            color +=
                CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, ccsv, ssv, materialFlags) * attenuation;
        }
    }

    return color;
}

vec3 CalculateLight(uint currLightIdx, vec3 materialDiffuseBRDF, vec3 L, float NoL, ShadingData sd,
    AnisotropicShadingVariables asv, ClearcoatShadingVariables ccsv, SheenShadingVariables ssv,
    const uint materialFlags)
{
    const vec3 H = normalize(L + sd.V);
    const float VoH = dot(sd.V, H);
    const float NoH = dot(sd.N, H);

    vec3 calculatedColor = vec3(0.0);
    float extAttenuation = 1.0;
    if ((materialFlags & CORE_MATERIAL_SHEEN_BIT) == CORE_MATERIAL_SHEEN_BIT) {
        const float sheenD = dCharlie(ssv.sheenRoughness, NoH);
        const float sheenV = vAshikhmin(sd.NoV, NoL);
        const vec3 sheenSpec = ssv.sheenColor * (sheenD * sheenV); // F = 1.0

        extAttenuation *= (1.0 - (ssv.sheenColorMax * ssv.sheenBRDFApprox));
        calculatedColor += (sheenSpec * NoL);
    }
    if ((materialFlags & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
        const float ccNoL = clamp(dot(ccsv.ccNormal, L), CORE3D_PBR_LIGHTING_EPSILON, 1.0);
        const float ccNoH = clamp(dot(ccsv.ccNormal, H), 0.0, 1.0);
        const float ccf0 = 0.04;

        const float ccD = dGGX(ccsv.ccAlpha2, ccNoH);
        const float ccG = vKelemen(ccNoH);
        const float ccF = fSchlickSingle(ccf0, VoH) * ccsv.cc;
        const float ccSpec = ccF * ccD * ccG;

        extAttenuation *= (1.0 - ccF);
        calculatedColor += vec3(ccSpec * ccNoL);
    }

    const float ToL = dot(asv.anisotropicT, L);
    const float ToH = dot(asv.anisotropicT, H);
    const float BoL = dot(asv.anisotropicB, L);
    const float BoH = dot(asv.anisotropicB, H);
    const float at = max(asv.alpha * (1.0 + asv.anisotropy), CORE3D_PBR_LIGHTING_EPSILON);
    const float ab = max(asv.alpha * (1.0 - asv.anisotropy), CORE3D_PBR_LIGHTING_EPSILON);

    const float D = dGGXAnisotropic(at, ab, NoH, ToH, BoH, asv.anisotropy);
    const float G = vGGXAnisotropic(at, ab, NoL, sd.NoV, ToL, asv.ToV, BoL, asv.BoV, asv.anisotropy);
    const vec3 F = fSchlick(sd.f0, VoH);
    const vec3 specContrib = F * (D * G);

    // KHR_materials_specular: "In the diffuse component we have to account for the fact that F is now an RGB value.",
    // but it already was? const vec3 diffuseContrib = (1.0 - max(F.x, (max(F.y, F.z)))) * materialDiffuseBRDF;
    const vec3 diffuseContrib = (1.0 - F.xyz) * materialDiffuseBRDF;
    calculatedColor += (diffuseContrib + specContrib * extAttenuation) * extAttenuation * NoL;
    calculatedColor *= uLightData.lights[currLightIdx].color.xyz;
    return calculatedColor;
}

vec3 CalculateLighting(ShadingData sd, AnisotropicShadingVariables asv, ClearcoatShadingVariables ccsv,
    SheenShadingVariables ssv, const uint materialFlags)
{
    const vec3 materialDiffuseBRDF = sd.diffuseColor * diffuseCoeff();
    vec3 color = vec3(0.0);
    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    const vec4 atlasSizeInvSize = uLightData.atlasSizeInvSize;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
        // NOTE: could check for NoL > 0.0 and NoV > 0.0
        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
        color +=
            CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, asv, ccsv, ssv, materialFlags) * shadowCoeff;
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
            if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, asv, ccsv, ssv, materialFlags) *
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, asv, ccsv, ssv, materialFlags) *
                     attenuation;
        }
    }

    return color;
}

vec3 CalculateLight(uint currLightIdx, vec3 materialDiffuseBRDF, vec3 L, float NoL, ShadingData sd,
    SubsurfaceScatterShadingVariables ssssv, const uint materialFlags)
{
    const vec3 H = normalize(L + sd.V);
    const float VoH = clamp(dot(sd.V, H), 0.0, 1.0);
    const float NoH = clamp(dot(sd.N, H), 0.0, 1.0);

    float extAttenuation = 1.0;
    vec3 calculatedColor = vec3(0.0);
    const float D = dGGX(sd.alpha2, NoH);
    const float G = vGGXWithCombinedDenominator(sd.alpha2, sd.NoV, NoL);
    const vec3 F = fSchlick(sd.f0, VoH);
    const vec3 specContrib = F * (D * G);

    const vec3 diffuseContrib = (1.0 - F.xyz) * materialDiffuseBRDF;
    calculatedColor += (diffuseContrib + specContrib * extAttenuation) * extAttenuation * NoL;

    // subsurface scattering
    // Use a spherical gaussian approximation of pow() for forwardScattering
    // We could include distortion by adding shading_normal * distortion to light.l
    float scatterVoH = clamp(dot(sd.V, -L), 0.0, 1.0);
    // map distance to gaussian sharpness, the lower the distance, the higher the sharpness
    float sharpness = 10000.0 - 10000.0 * ssssv.scatterDistance;
    float forwardScatter = exp2(scatterVoH * sharpness - sharpness);
    float backScatter = clamp(NoL * ssssv.thickness + (1.0 - ssssv.thickness), 0.0, 1.0) * 0.5;
    float subsurface = mix(backScatter, 1.0, forwardScatter) * (1.0 - ssssv.thickness);
    calculatedColor += ssssv.scatterColor * (subsurface * dLambert());

    // TODO: apply attenuation to the transmitted light, i.e. uLightData.lights[currLightIdx].attenuation
    return (calculatedColor * uLightData.lights[currLightIdx].color.rgb);
}

vec3 CalculateLighting(ShadingData sd, SubsurfaceScatterShadingVariables sssv, const uint materialFlags)
{
    const vec3 materialDiffuseBRDF = sd.diffuseColor * diffuseCoeff();
    vec3 color = vec3(0.0);
    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    const vec4 atlasSizeInvSize = uLightData.atlasSizeInvSize;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(sd.N, L), 0.0, 1.0);
        // NOTE: could check for NoL > 0.0 and NoV > 0.0
        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
        color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, sssv, materialFlags) * shadowCoeff;
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
            if ((materialFlags & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, sssv, materialFlags) *
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
            color += CalculateLight(currLightIdx, materialDiffuseBRDF, L, NoL, sd, sssv, materialFlags) * attenuation;
        }
    }

    return color;
}

#endif // SHADERS__COMMON__3D_DM_LIGHTING_COMMON_H
