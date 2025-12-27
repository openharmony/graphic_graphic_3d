/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef CORE3D_DM_FW_FRAG_H
#define CORE3D_DM_FW_FRAG_H

// includes

#include "3d/shaders/common/3d_dm_brdf_common.h"
#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_shadowing_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"
#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets and specializations
// clang-format off
#include "3d/shaders/common/3d_dm_frag_layout_common.h"
// clang-format on
#include "3d/shaders/common/3d_dm_env_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_lighting_common.h"
#define CORE3D_DM_FW_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"
#include "3d/shaders/common/3d_dm_inplace_fog_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

ClearcoatShadingVariables GetUnpackedAndSampledClearcoat(const uint matInstanceIdx, const vec3 ccNormal)
{
    const float cc = GetUnpackClearcoat(matInstanceIdx);
    const float ccRoughness = GetUnpackClearcoatRoughness(matInstanceIdx);
    ClearcoatShadingVariables ccsv;
    ccsv.cc = clamp(GetClearcoatSample(inUv, matInstanceIdx) * cc, 0.0, 1.0); // 0.0 - 1.0
    ccsv.ccNormal = ccNormal;
    ccsv.ccRoughness = GetClearcoatRoughnessSample(inUv, matInstanceIdx) * ccRoughness; // CORE_BRDF_MIN_ROUGHNESS - 1.0
    // geometric correction doesn't behave that well with clearcoat due to it being basically 0 roughness
    ccsv.ccRoughness = clamp(ccsv.ccRoughness, CORE_BRDF_MIN_ROUGHNESS, 1.0);
    CORE_RELAXEDP const float ccAlpha = ccsv.ccRoughness * ccsv.ccRoughness;
    ccsv.ccAlpha2 = ccAlpha * ccAlpha;
    return ccsv;
}

SheenShadingVariables GetUnpackedAndSampledSheen(const uint matInstanceIdx)
{
    const vec4 sheen = GetUnpackSheen(matInstanceIdx);
    SheenShadingVariables ssv;
    ssv.sheenColor = GetSheenSample(inUv, matInstanceIdx) * sheen.xyz;
    ssv.sheenRoughness = GetSheenRoughnessSample(inUv, matInstanceIdx) * sheen.w; // CORE_BRDF_MIN_ROUGHNESS - 1.0
    ssv.sheenColorMax = max(ssv.sheenColor.r, max(ssv.sheenColor.g, ssv.sheenColor.b));
    return ssv;
}

vec3 GetTransmissionRadianceSample(const vec2 fragUv, const vec3 worldReflect, const float roughness)
{
    // NOTE: this makes a pre color selection based on alpha
    // we would generally need an extra flag, the default texture is black with alpha zero
    const CORE_RELAXEDP float lod = CoreGetLodForRadianceSample(roughness);
    vec4 color = textureLod(uSampColorPrePass, fragUv, lod).rgba;
    if (color.a < 0.5f) {
        // sample environment if the default pre pass color was 0.0 alpha
        color.rgb = CoreGetRadianceSample(worldReflect, roughness);
    }
    return color.rgb;
}

// "main" functions

vec4 unlitBasic()
{
    const uint matInstanceIdx = GetMaterialInstanceIndex(inIndices);
    CORE_RELAXEDP vec4 baseColor =
        GetBaseColorSample(inUv, matInstanceIdx) * GetUnpackBaseColor(matInstanceIdx) * inColor;
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(matInstanceIdx)) {
            discard;
        }
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_OPAQUE_BIT) == CORE_MATERIAL_OPAQUE_BIT) {
        baseColor.a = 1.0;
    }

    // NOTE: no fog support yet
    if ((CORE_CAMERA_FLAGS & CORE_CAMERA_VELOCITY_OUT_BIT) == CORE_CAMERA_VELOCITY_OUT_BIT) {
        const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
        outVelocityNormal =
            GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), inNormal.xyz);
    }
    return baseColor;
}

vec4 unlitShadowAlpha()
{
    const uint matInstanceIdx = GetMaterialInstanceIndex(inIndices);
    CORE_RELAXEDP vec4 baseColor =
        GetBaseColorSample(inUv, matInstanceIdx) * GetUnpackBaseColor(matInstanceIdx) * inColor;
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(matInstanceIdx)) {
            discard;
        }
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_OPAQUE_BIT) == CORE_MATERIAL_OPAQUE_BIT) {
        baseColor.a = 1.0;
    }
    const vec3 N = normalize(inNormal.xyz);

    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    const vec4 atlasSizeInvSize = uLightData.atlasSizeInvSize;
    CORE_RELAXEDP float fullShadowCoeff = 1.0;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(N, L), 0.0, 1.0);

        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
            const uvec4 lightFlags = uLightData.lights[currLightIdx].flags;
            if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(inPos.xyz, 1.0);
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
        fullShadowCoeff *= shadowCoeff;
    }
    if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SPOT_ENABLED_BIT) == CORE_LIGHTING_SPOT_ENABLED_BIT) {
        const uint spotLightCount = uLightData.spotLightCount;
        const uint spotLightLightBeginIndex = uLightData.spotLightBeginIndex;
        for (uint spotIdx = 0; spotIdx < spotLightCount; ++spotIdx) {
            const uint currLightIdx = spotLightLightBeginIndex + spotIdx;

            const vec3 pointToLight = uLightData.lights[currLightIdx].pos.xyz - inPos.xyz;
            const float dist = length(pointToLight);
            const vec3 L = pointToLight / dist;
            const float NoL = clamp(dot(N, L), 0.0, 1.0);
            // NOTE: could check for NoL > 0.0 and NoV > 0.0
            CORE_RELAXEDP float shadowCoeff = 1.0;
            if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
                const uvec4 lightFlags = uLightData.lights[currLightIdx].flags;
                if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                    const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(inPos.xyz, 1.0);
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
            const float intensity = uLightData.lights[currLightIdx].color.w;
            const float fullAttenuation = min(1.0, angularAttenuation * angularAttenuation * attenuation * intensity);

            fullShadowCoeff *= (1.0 - (1.0 - shadowCoeff) * fullAttenuation);
        }
    }
    CORE_RELAXEDP vec4 color = baseColor * clamp(1.0 - fullShadowCoeff, 0.0, 1.0);

    // NOTE: no fog support yet
    if ((CORE_CAMERA_FLAGS & CORE_CAMERA_VELOCITY_OUT_BIT) == CORE_CAMERA_VELOCITY_OUT_BIT) {
        const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
        outVelocityNormal =
            GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), N);
    }
    return color;
}

vec4 pbrBasic()
{
    const uint matInstanceIdx = GetMaterialInstanceIndex(inIndices);
    const CORE_RELAXEDP vec4 baseColor = GetUnpackBaseColorFinalValue(inColor, inUv, matInstanceIdx);

    const vec3 normNormal = normalize(inNormal.xyz);
    vec3 N = normNormal;
    vec3 clearcoatN = normNormal;
    // clear coat normal is calculated if normal_map_bit and if clearcoat_bit
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_NORMAL_MAP_BIT) == CORE_MATERIAL_NORMAL_MAP_BIT) {
        N = GetNormalSample(inUv, matInstanceIdx);
        const float normalScale = GetUnpackNormalScale(matInstanceIdx);
        const mat3 tbn = CalcTbnMatrix(normNormal, inTangentW);
        N = CalcFinalNormal(tbn, N, normalScale);
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
            clearcoatN = GetClearcoatNormalSample(inUv, matInstanceIdx);
            const float ccNormalScale = GetUnpackClearcoatNormalScale(matInstanceIdx);
            clearcoatN = CalcFinalNormal(tbn, clearcoatN, ccNormalScale);
        }
    }
    // if no backface culling we flip automatically
    N = gl_FrontFacing ? N : -N;

    const CORE_RELAXEDP vec4 material = GetMaterialSample(inUv, matInstanceIdx) * GetUnpackMaterial(matInstanceIdx);
    InputBrdfData brdfData;
    if (CORE_MATERIAL_TYPE == CORE_MATERIAL_SPECULAR_GLOSSINESS) {
        brdfData = CalcBRDFSpecularGlossiness(baseColor, normNormal, material);
    } else if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SPECULAR_BIT) == CORE_MATERIAL_SPECULAR_BIT) {
        CORE_RELAXEDP vec4 specular = GetUnpackSpecular(matInstanceIdx);
        const CORE_RELAXEDP vec4 specularSample = GetSpecularSample(inUv, matInstanceIdx);
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SPECULAR_FACTOR_TEXTURE_BIT) ==
            CORE_MATERIAL_SPECULAR_FACTOR_TEXTURE_BIT) {
            specular.a *= specularSample.a;
        }
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SPECULAR_COLOR_TEXTURE_BIT) ==
            CORE_MATERIAL_SPECULAR_COLOR_TEXTURE_BIT) {
            specular.rgb *= specularSample.rgb;
        }
        brdfData = CalcBRDFSpecular(baseColor, normNormal, material, specular);
    } else {
        brdfData = CalcBRDFMetallicRoughness(baseColor, normNormal, material);
    }

    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    const vec3 camPos = uCameras[cameraIdx].viewInv[3].xyz;
    const vec3 V = normalize(camPos.xyz - inPos);
    const float NoV = clamp(dot(N, V), CORE3D_PBR_LIGHTING_EPSILON, 1.0);

    ClearcoatShadingVariables clearcoatSV;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
        clearcoatSV = GetUnpackedAndSampledClearcoat(matInstanceIdx, clearcoatN);
    }
    SheenShadingVariables sheenSV;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHEEN_BIT) == CORE_MATERIAL_SHEEN_BIT) {
        sheenSV = GetUnpackedAndSampledSheen(matInstanceIdx);
        sheenSV.sheenBRDFApprox = EnvBRDFApproxSheen(NoV, sheenSV.sheenRoughness * sheenSV.sheenRoughness);
    }

    CORE_RELAXEDP float transmission;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
        transmission = GetTransmissionSample(inUv, matInstanceIdx) * GetUnpackTransmission(matInstanceIdx);
        brdfData.diffuseColor *= (1.0 - transmission);
    }

    ShadingDataInplace shadingData;
    shadingData.pos = inPos.xyz;
    shadingData.N = N;
    shadingData.NoV = NoV;
    shadingData.V = V;
    shadingData.f0 = brdfData.f0;
    shadingData.alpha2 = brdfData.alpha2;
    shadingData.diffuseColor = brdfData.diffuseColor;
    shadingData.materialFlags = CORE_MATERIAL_FLAGS;
    const uint instanceIdx = GetInstanceIndex(inIndices);
    shadingData.layers = uMeshMatrix.mesh[instanceIdx].layers.xy;
    shadingData.cameraIdx = cameraIdx;
    CORE_RELAXEDP const float roughness = brdfData.roughness;

    vec3 color = vec3(0.0); // brdfData.diffuseColor
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) {
        color += CalculateLightingInplace(shadingData, clearcoatSV, sheenSV);
    }

    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) {
        const CORE_RELAXEDP float ao =
            clamp(1.0 + GetUnpackAO(matInstanceIdx) * (GetAOSample(inUv, matInstanceIdx) - 1.0), 0.0, 1.0);

        // lambert baked into irradianceSample (SH)
        CORE_RELAXEDP vec3 irradiance = vec3(0);
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_IRRADIANCE_BIT) ==
            CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_IRRADIANCE_BIT) {
            irradiance = CoreGetIrradianceSample(shadingData.N) * shadingData.diffuseColor * ao;
        } else {
            // get only the environment sample
            irradiance = GetIrradianceIndirectLambertSample() * shadingData.diffuseColor * ao;
        }

        const vec3 worldReflect = reflect(-shadingData.V, shadingData.N);
        const CORE_RELAXEDP vec3 fIndirect = EnvBRDFApprox(shadingData.f0.xyz, roughness, NoV);
        // ao applied after clear coat
        CORE_RELAXEDP vec3 radianceSample = CoreGetRadianceSample(worldReflect, roughness);
        CORE_RELAXEDP vec3 radiance = radianceSample * fIndirect;

        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHEEN_BIT) == CORE_MATERIAL_SHEEN_BIT) {
            AppendIndirectSheen(sheenSV, radianceSample, baseColor.a, irradiance, radiance);
        }
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
            const vec3 ccWorldReflect = reflect(-shadingData.V, clearcoatSV.ccNormal);
            const vec3 ccRadianceSample = CoreGetRadianceSample(ccWorldReflect, clearcoatSV.ccRoughness);
            AppendIndirectClearcoat(clearcoatSV, ccRadianceSample, baseColor.a, V, irradiance, radiance);
        }
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
            vec2 fragUv;
            CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);
            // Approximate double refraction by assuming a solid sphere beneath the point.
            const float ior = 1.0;
            const float eta = (1.0 / 1.5); // expecting glass
            // NOTE: ATM use direct refract (no sphere mapping)
            const vec3 rr = -V; // refract(-V, N, 1.0 / ior);
            const CORE_RELAXEDP vec3 transmissionRadianceSample = GetTransmissionRadianceSample(fragUv, rr, roughness);

            AppendIndirectTransmission(transmissionRadianceSample, baseColor.rgb, transmission, irradiance);
        }
        // apply ao for indirect specular as well (cheap version)
#if 1
        radiance *= ao * SpecularHorizonOcclusion(worldReflect, normNormal);
#else
        radiance *= EnvSpecularAo(ao, NoV, roughness) * SpecularHorizonOcclusion(worldReflect, normNormal);
#endif

        color += (irradiance + radiance);
    }

    // NOTE: emissive missing clearcoat darkening
    CORE_RELAXEDP vec3 emissive = GetEmissiveSample(inUv, matInstanceIdx) * GetUnpackEmissiveColor(matInstanceIdx);
    color += emissive;

    // fog handling
    InplaceFogBlock(CORE_CAMERA_FLAGS, inPos.xyz, camPos.xyz, vec4(color, baseColor.a), color);

    if ((CORE_CAMERA_FLAGS & CORE_CAMERA_VELOCITY_OUT_BIT) == CORE_CAMERA_VELOCITY_OUT_BIT) {
        outVelocityNormal =
            GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), N);
    }

    return GetPackPbrColor(color.rgb, baseColor.a);
}

#endif // CORE3D_DM_FW_FRAG_H
