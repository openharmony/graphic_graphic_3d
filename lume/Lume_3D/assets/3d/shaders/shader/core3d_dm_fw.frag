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
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

#include "3d/shaders/common/3d_dm_brdf_common.h"
#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_shadowing_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"
// sets and specializations

#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#define CORE3D_DM_FW_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"
#include "common/inplace_lighting_common.h"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

float GetLodForRadianceSample(const float roughness)
{
    return uEnvironmentData.values.x * roughness;
}

ClearcoatShadingVariables GetUnpackedAndSampledClearcoat()
{
    const uint instanceIdx = uint(inVelocityI.z + 0.5);
    const float cc = GetUnpackClearcoat(instanceIdx);
    const float ccRoughness = GetUnpackClearcoatRoughness(instanceIdx);
    ClearcoatShadingVariables ccsv;
    ccsv.cc = GetClearcoatSample(inUv) * cc;                                         // 0.0 - 1.0
    ccsv.ccRoughness = GetClearcoatRoughnessSample(inUv, instanceIdx) * ccRoughness; // CORE_BRDF_MIN_ROUGHNESS - 1.0
    CORE_RELAXEDP const float ccAlpha = ccsv.ccRoughness * ccsv.ccRoughness;
    ccsv.ccAlpha2 = ccAlpha * ccAlpha;
    return ccsv;
}

SheenShadingVariables GetUnpackedAndSampledSheen()
{
    const uint instanceIdx = uint(inVelocityI.z + 0.5);
    const vec4 sheen = GetUnpackSheen(instanceIdx);
    SheenShadingVariables ssv;
    ssv.sheenColor = GetSheenSample(inUv, instanceIdx) * sheen.xyz;
    ssv.sheenRoughness = GetSheenRoughnessSample(inUv, instanceIdx) * sheen.w; // CORE_BRDF_MIN_ROUGHNESS - 1.0
    ssv.sheenColorMax = max(ssv.sheenColor.r, max(ssv.sheenColor.g, ssv.sheenColor.b));
    return ssv;
}

vec3 GetIrradianceSample(const vec3 worldNormal)
{
    const vec3 worldNormalEnv = mat3(uEnvironmentData.envRotation) * worldNormal;
    return unpackIblIrradianceSH(worldNormalEnv, uEnvironmentData.shIndirectCoefficients) *
           uEnvironmentData.indirectDiffuseColorFactor.rgb;
}

vec3 GetRadianceSample(const vec3 worldReflect, const float roughness)
{
    const CORE_RELAXEDP float cubeLod = GetLodForRadianceSample(roughness);
    const vec3 worldReflectEnv = mat3(uEnvironmentData.envRotation) * worldReflect;
    return unpackIblRadiance(textureLod(uSampRadiance, worldReflectEnv, cubeLod)) *
           uEnvironmentData.indirectSpecularColorFactor.rgb;
}

vec3 GetTransmissionRadianceSample(const vec2 fragUv, const vec3 worldReflect, const float roughness)
{
    // NOTE: this makes a pre color selection based on alpha
    // we would generally need an extra flag, the default texture is black with alpha zero
    const CORE_RELAXEDP float lod = GetLodForRadianceSample(roughness);
    vec4 color = textureLod(uSampColorPrePass, fragUv, lod).rgba;
    if (color.a < 0.5f) {
        // sample environment if the default pre pass color was 0.0 alpha
        color.rgb = GetRadianceSample(worldReflect, roughness);
    }
    return color.rgb;
}

///////////////////////////////////////////////////////////////////////////////
// "main" functions

vec4 unlitBasic()
{
    const uint instanceIdx = uint(inVelocityI.z + 0.5);
    const CORE_RELAXEDP vec4 baseColor =
        GetBaseColorSample(inUv, instanceIdx) * GetUnpackBaseColor(instanceIdx) * inColor;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    }
    outVelocityNormal = GetPackVelocityAndNormal(inVelocityI.xy, inNormal.xyz);

    return baseColor;
}

vec4 unlitShadowAlpha()
{
    const uint instanceIdx = uint(inVelocityI.z + 0.5);
    const CORE_RELAXEDP vec4 baseColor =
        GetBaseColorSample(inUv, instanceIdx) * GetUnpackBaseColor(instanceIdx) * inColor;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    }
    const vec3 N = normalize(inNormal.xyz);

    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    CORE_RELAXEDP float fullShadowCoeff = 1.0;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(N, L), 0.0, 1.0);

        CORE_RELAXEDP float shadowCoeff = 1.0;
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHADOW_RECEIVER_BIT) == CORE_MATERIAL_SHADOW_RECEIVER_BIT) {
            const uvec2 lightFlags = uLightData.lights[currLightIdx].flags.xy;
            if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(inPos.xyz, 1.0);
                const vec4 shadowFactors = uLightData.lights[currLightIdx].shadowFactors;
                if ((CORE_LIGHTING_FLAGS & CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) == CORE_LIGHTING_SHADOW_TYPE_VSM_BIT) {
                    shadowCoeff = CalcVsmShadow(uSampColorShadow, shadowCoord, NoL, shadowFactors);
                } else {
                    shadowCoeff = CalcPcfShadow(uSampDepthShadow, shadowCoord, NoL, shadowFactors);
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
                const uvec2 lightFlags = uLightData.lights[currLightIdx].flags.xy;
                if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                    const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(inPos.xyz, 1.0);
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
            const float intensity = uLightData.lights[currLightIdx].color.w;
            const float fullAttenuation = min(1.0, angularAttenuation * angularAttenuation * attenuation * intensity);

            fullShadowCoeff *= (1.0 - (1.0 - shadowCoeff) * fullAttenuation);
        }
    }
    CORE_RELAXEDP vec4 color = baseColor * clamp(1.0 - fullShadowCoeff, 0.0, 1.0);

    outVelocityNormal = GetPackVelocityAndNormal(inVelocityI.xy, N);

    return color;
}

vec4 pbrBasic()
{
    const uint instanceIdx = uint(inVelocityI.z + 0.5);
    // NOTE: by the spec with blend mode opaque alpha should be 1.0 from this calculation
    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(inUv, instanceIdx) * GetUnpackBaseColor(instanceIdx) * inColor;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_OPAQUE_BIT) == CORE_MATERIAL_OPAQUE_BIT) {
        baseColor.a = 1.0;
    }

    const vec3 normNormal = normalize(inNormal.xyz);
    vec3 N = normNormal;
    vec3 clearcoatN = normNormal;
    // clear coat normal is calculated if normal_map_bit and if clearcoat_bit
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_NORMAL_MAP_BIT) == CORE_MATERIAL_NORMAL_MAP_BIT) {
        vec3 normal = GetNormalSample(inUv, instanceIdx);
        const float normalScale = GetUnpackNormalScale(instanceIdx);
        normal = normalize((2.0 * normal - 1.0) * vec3(normalScale, normalScale, 1.0f));
        const vec3 tangent = normalize(inTangentW.xyz);
        const vec3 bitangent = cross(normNormal, tangent.xyz) * inTangentW.w;
        const mat3 tbn = mat3(tangent.xyz, bitangent.xyz, normNormal);
        N = normalize(tbn * normal);
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
            vec3 ccNormal = GetClearcoatNormalSample(inUv, instanceIdx);
            // cc normal scale not yet supported
            ccNormal = normalize((2.0 * ccNormal - 1.0));
            clearcoatN = normalize(tbn * ccNormal);
        }
    }
    // if no backface culling we flip automatically
    N = gl_FrontFacing ? N : -N;

    InputBrdfData brdfData = GetInputBRDF(baseColor, normNormal, inUv, instanceIdx);

    const uint cameraIdx = GetUnpackCameraIndex();
    const vec3 camPos = uCameras[cameraIdx].viewInv[3].xyz;
    const vec3 V = normalize(camPos.xyz - inPos);
    const float NoV = clamp(dot(N, V), CORE_PBR_LIGHTING_EPSILON, 1.0);

    ClearcoatShadingVariables clearcoatSV;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
        clearcoatSV = GetUnpackedAndSampledClearcoat();
        clearcoatSV.ccNormal = clearcoatN;
        // compensate f0 with clear coat
        brdfData.f0.xyz = mix(brdfData.f0.xyz, f0ClearcoatToSurface(brdfData.f0.xyz), clearcoatSV.cc);
    }
    SheenShadingVariables sheenSV;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHEEN_BIT) == CORE_MATERIAL_SHEEN_BIT) {
        sheenSV = GetUnpackedAndSampledSheen();
        sheenSV.sheenBRDFApprox = EnvBRDFApproxSheen(NoV, sheenSV.sheenRoughness * sheenSV.sheenRoughness);
    }

    CORE_RELAXEDP float transmission;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
        transmission = GetTransmissionSample(inUv, instanceIdx) * GetUnpackTransmission(instanceIdx);
        brdfData.diffuseColor *= (1.0 - transmission);
    }

    ShadingData shadingData;
    shadingData.pos = inPos.xyz;
    shadingData.N = N;
    shadingData.NoV = NoV;
    shadingData.V = V;
    shadingData.f0 = brdfData.f0;
    shadingData.alpha2 = brdfData.alpha2;
    shadingData.diffuseColor = brdfData.diffuseColor;
    shadingData.materialFlags = CORE_MATERIAL_FLAGS;
    CORE_RELAXEDP const float roughness = brdfData.roughness;

    vec3 color = brdfData.diffuseColor;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) {
        color = CalculateLighting(shadingData, clearcoatSV, sheenSV);
    }

    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) {
        const CORE_RELAXEDP float ao = clamp(GetAOSample(inUv, instanceIdx) * GetUnpackAO(instanceIdx), 0.0, 1.0);
        // lambert baked into irradianceSample (SH)
        CORE_RELAXEDP vec3 irradiance = GetIrradianceSample(shadingData.N) * shadingData.diffuseColor * ao;

        const vec3 worldReflect = reflect(-shadingData.V, shadingData.N);
        const CORE_RELAXEDP vec3 fIndirect = EnvBRDFApprox(shadingData.f0.xyz, roughness, NoV);
        // ao applied after clear coat
        CORE_RELAXEDP vec3 radianceSample = GetRadianceSample(worldReflect, roughness);
        CORE_RELAXEDP vec3 radiance = radianceSample * fIndirect;

        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_SHEEN_BIT) == CORE_MATERIAL_SHEEN_BIT) {
            const vec3 sheenF = sheenSV.sheenColor * sheenSV.sheenBRDFApprox;
            const float sheenAttenuation = 1.0 - (sheenSV.sheenColorMax * sheenSV.sheenBRDFApprox);
            // energy compensation
            irradiance *= sheenAttenuation;
            radiance *= sheenAttenuation;

            radiance += sheenF * radianceSample;
        }
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
            const float ccNoV = clamp(dot(clearcoatSV.ccNormal, V), CORE_PBR_LIGHTING_EPSILON, 1.0);
            const vec3 ccWorldReflect = reflect(-shadingData.V, clearcoatSV.ccNormal);
            const float ccF = fSchlickSingle(0.04, ccNoV) * clearcoatSV.cc;
            const float ccAttenuation = 1.0 - ccF;

            // energy compensation
            irradiance *= ccAttenuation;
            radiance *= ccAttenuation * ccAttenuation;
            // add clear coat radiance
            radiance += GetRadianceSample(ccWorldReflect, clearcoatSV.ccRoughness) * ccF;
        }
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
            // Approximate double refraction by assuming a solid sphere beneath the point.
            const float ior = 1.0;
            const float Eta = (1.0 / 1.5); // expecting glass
            vec2 fragUv;
            CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);
            // NOTE: ATM use direct refract (no sphere mapping)
            const vec3 rr = -V; // refract(-V, N, 1.0 / ior);
            const vec3 Ft = GetTransmissionRadianceSample(fragUv, rr, roughness) * baseColor.rgb;
            irradiance *= (1.0 - transmission);
            irradiance = mix(irradiance, Ft, transmission);
        }
        // apply ao for indirect specular as well (cheap version)
#if 1
        radiance *= ao * SpecularHorizonOcclusion(worldReflect, normNormal);
#else
        radiance *= EnvSpecularAo(ao, NoV, roughness) * SpecularHorizonOcclusion(worldReflect, normNormal);
#endif

        color += (irradiance + radiance);
    }

    CORE_RELAXEDP vec3 emissive = GetEmissiveSample(inUv, instanceIdx) * GetUnpackEmissiveColor(instanceIdx);
    color += emissive * baseColor.a; // needs to be multiplied with alpha (premultiplied)

    outVelocityNormal = GetPackVelocityAndNormal(inVelocityI.xy, N);

    color.rgb = min(color.rgb, CORE_HDR_FLOAT_CLAMP_MAX_VALUE); // not expecting negative lights
    return vec4(color.rgb, baseColor.a);
}

/*
fragment shader for basic pbr materials.
*/
void main(void)
{
    if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT) {
        outColor = unlitBasic();
    } else if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT_SHADOW_ALPHA) {
        outColor = unlitShadowAlpha();
    } else {
        outColor = pbrBasic();
    }

    if (CORE_POST_PROCESS_FLAGS > 0) {
        vec2 fragUv;
        CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

        InplacePostProcess(fragUv, outColor);
    }
}
