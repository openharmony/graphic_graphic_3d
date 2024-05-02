#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"
#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets and specializations

#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#define CORE3D_DM_FW_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_brdf_common.h"
#include "3d/shaders/common/3d_dm_inout_common.h"
#include "3d/shaders/common/3d_dm_inplace_fog_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"
#include "common/inplace_lighting_common.h"

// in / out

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

///////////////////////////////////////////////////////////////////////////////
// "main" functions

struct ReflectionDataStruct {
    CORE_RELAXEDP float mipCount;
    CORE_RELAXEDP float screenPercentage;
    CORE_RELAXEDP float imageResX;
    CORE_RELAXEDP float imageResY;
};

ReflectionDataStruct GetUnpackReflectionPlaneData()
{
    const uint instanceIdx = 0;
    // clearcoat roughness used as a slot to pass in the factor data
    vec4 factor = uMaterialData.material[instanceIdx].factors[CORE_MATERIAL_FACTOR_CLEARCOAT_ROUGHNESS_IDX];
    ReflectionDataStruct rds;
    rds.mipCount = factor.x;
    rds.screenPercentage = factor.y;
    rds.imageResX = max(1.0, factor.z);
    rds.imageResY = max(1.0, factor.w);
    return rds;
}

float GetLodForReflectionSample(const float roughness, const float mipCount)
{
    return mipCount * roughness;
}

float GetLodForRadianceSample(const float roughness)
{
    return uEnvironmentData.values.x * roughness;
}

vec3 GetReflectionSample(const vec2 inputUv, const float lod)
{
    return textureLod(uSampTextures[CORE_MATERIAL_TEX_CLEARCOAT_ROUGHNESS_IDX], inputUv.xy, lod).xyz;
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

uint GetInstanceIndex()
{
    return 0U;
}

vec4 PlaneReflector(const vec2 fragUv)
{
    // GPU instancing not supported
    const uint instanceIdx = GetInstanceIndex();

    const CORE_RELAXEDP vec4 baseColor = GetUnpackBaseColorFinalValue(inColor, inUv, instanceIdx);

    const vec3 normNormal = normalize(inNormal.xyz);
    vec3 N = normNormal;
    // clear coat normal is calculated if normal_map_bit and if clearcoat_bit
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_NORMAL_MAP_BIT) == CORE_MATERIAL_NORMAL_MAP_BIT) {
        N = GetNormalSample(inUv, instanceIdx);
        const float normalScale = GetUnpackNormalScale(instanceIdx);
        const mat3 tbn = CalcTbnMatrix(normNormal, inTangentW);
        N = CalcFinalNormal(tbn, N, normalScale);
    }
    // if no backface culling we flip automatically
    N = gl_FrontFacing ? N : -N;

    const CORE_RELAXEDP vec4 material = GetMaterialSample(inUv, instanceIdx) * GetUnpackMaterial(instanceIdx);
    InputBrdfData brdfData = CalcBRDFMetallicRoughness(baseColor, normNormal, material);

    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    const vec3 camPos = uCameras[cameraIdx].viewInv[3].xyz;
    const vec3 V = normalize(camPos.xyz - inPos);
    const float NoV = clamp(dot(N, V), CORE3D_PBR_LIGHTING_EPSILON, 1.0);

    CORE_RELAXEDP float transmission;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
        transmission = GetTransmissionSample(inUv) * GetUnpackTransmission(instanceIdx);
        brdfData.diffuseColor *= (1.0 - transmission);
    }

    ClearcoatShadingVariables clearcoatSV;
    SheenShadingVariables sheenSV;
    ShadingDataInplace shadingData;
    shadingData.pos = inPos.xyz;
    shadingData.N = N;
    shadingData.NoV = NoV;
    shadingData.V = V;
    shadingData.f0 = brdfData.f0;
    shadingData.alpha2 = brdfData.alpha2;
    shadingData.diffuseColor = brdfData.diffuseColor;
    shadingData.layers = uMeshMatrix.mesh[instanceIdx].layers.xy;
    CORE_RELAXEDP const float roughness = brdfData.roughness;

    vec3 color = vec3(0.0); // brdfData.diffuseColor
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) {
        color += CalculateLightingInplace(shadingData, clearcoatSV, sheenSV);
    }

    // reflection
    const ReflectionDataStruct rds = GetUnpackReflectionPlaneData();
    const CORE_RELAXEDP float reflLod = GetLodForReflectionSample(roughness, rds.mipCount);

    vec2 scaledFragUv;
    CORE_GET_FRAGCOORD_UV(
        scaledFragUv, gl_FragCoord.xy * rds.screenPercentage, 1.0 / vec2(rds.imageResX, rds.imageResY));
    const CORE_RELAXEDP vec3 radianceSample = GetReflectionSample(scaledFragUv, reflLod);

    CORE_RELAXEDP vec3 radiance = radianceSample;
    CORE_RELAXEDP vec3 irradiance = vec3(0.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) ==
        CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) {
        const CORE_RELAXEDP float ao = clamp(GetAOSample(inUv, instanceIdx) * GetUnpackAO(instanceIdx), 0.0, 1.0);
        // lambert baked into irradianceSample (SH)
        irradiance = GetIrradianceSample(N) * shadingData.diffuseColor * ao;
        const CORE_RELAXEDP vec3 fIndirect = EnvBRDFApprox(shadingData.f0.xyz, roughness, NoV);
        radiance *= fIndirect;
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_TRANSMISSION_BIT) == CORE_MATERIAL_TRANSMISSION_BIT) {
            // NOTE: ATM use direct refract (no sphere mapping)
            const vec3 rr = -V; // refract(-V, N, 1.0 / ior);
            const vec3 transmissionRadianceSample = GetTransmissionRadianceSample(fragUv, rr, roughness);

            AppendIndirectTransmission(transmissionRadianceSample, baseColor.rgb, transmission, irradiance);
        }
    }
    color += irradiance + radiance;

    // fog handling
    InplaceFogBlock(CORE_CAMERA_FLAGS, inPos.xyz, camPos.xyz, vec4(color, baseColor.a), color);

    outVelocityNormal = GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), N);

    return GetPackPbrColor(color.rgb, baseColor.a);
}

/**
 * fragment shader for plane reflector
 */
void main(void)
{
    vec2 fragUv;
    CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, GetUnpackViewport().zw);
    outColor = PlaneReflector(fragUv);
    if (CORE_POST_PROCESS_FLAGS > 0) {
        InplacePostProcess(fragUv, outColor);
    }
}
