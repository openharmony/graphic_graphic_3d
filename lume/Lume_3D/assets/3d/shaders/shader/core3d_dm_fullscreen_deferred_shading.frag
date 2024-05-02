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

#define ENABLE_INPUT_ATTACHMENTS 1
#include "3d/shaders/common/3d_dm_deferred_shading_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_lighting_common.h"
#include "3d/shaders/common/3d_dm_inplace_fog_common.h"

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

// unpack gbuffer

struct FullGBufferData {
    CORE_RELAXEDP vec4 baseColor;
    vec3 normal;
    float ao;
    vec4 material;
    uint materialType;
    uint materialFlags;
};

FullGBufferData GetUnpackMaterialValues(const vec2 uv)
{
    FullGBufferData fd;
    fd.material = vec4(0.0, 1.0, 1.0, 0.04);
    fd.baseColor = vec4(0.0, 0.0, 0.0, 1.0);
    fd.normal = vec3(0.0, 1.0, 0.0);
    fd.ao = 1.0;
    fd.materialType = 0;
    fd.materialFlags = 0;

#if (ENABLE_INPUT_ATTACHMENTS == 1)
    GetUnpackMaterialWithFlags(subpassLoad(uGBufferMaterial), fd.material, fd.materialType, fd.materialFlags);
#else
    GetUnpackMaterialWithFlags(textureLod(uGBufferMaterial, uv, 0), fd.material, fd.materialType, fd.materialFlags);
#endif
    return fd;
}

void GetSampledGBuffer(const vec2 uv, inout FullGBufferData fd)
{
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    GetUnpackBaseColorWithAo(subpassLoad(uGBufferBaseColor), fd.baseColor.rgb, fd.ao);

    vec2 vel;
    GetUnpackVelocityAndNormal(subpassLoad(uGBufferVelocityNormal).xyzw, vel, fd.normal);
    fd.normal = normalize(fd.normal);
#else
    GetUnpackBaseColorWithAo(textureLod(uGBufferBaseColor, uv, 0), fd.baseColor.rgb, fd.ao);

    vec2 vel;
    GetUnpackVelocityAndNormal(textureLod(uGBufferVelocityNormal, uv, 0), vel, fd.normal);
    fd.normal = normalize(fd.normal);
#endif
}

void GetSimpleSampledGBuffer(const vec2 uv, inout FullGBufferData fd)
{
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    vec2 vel;
    GetUnpackVelocityAndNormal(subpassLoad(uGBufferVelocityNormal).xyzw, vel, fd.normal);
    fd.normal = normalize(fd.normal);
#else
    vec2 vel;
    GetUnpackVelocityAndNormal(textureLod(uGBufferVelocityNormal, uv, 0), vel, fd.normal);
    fd.normal = normalize(fd.normal);
#endif
}

float GetSampledDepthBuffer(const vec2 uv)
{
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    return GetUnpackDepthBuffer(subpassLoad(uGBufferDepthBuffer).x);
#else
    return GetUnpackDepthBuffer(textureLod(uGBufferDepthBuffer, uv, 0).x);
#endif
}

vec4 GetSampledBaseColor(const vec2 uv)
{
    vec4 color = vec4(0.0);
#if (ENABLE_INPUT_ATTACHMENTS == 1)
    GetUnpackBaseColorWithAo(subpassLoad(uGBufferBaseColor), color.rgb, color.a);
#else
    GetUnpackBaseColorWithAo(textureLod(uGBufferBaseColor, uv, 0), color.rgb, color.a);
#endif
    return color;
}

// end gbuffer

vec3 GetWorldPos(const uint cameraIdx, const float depthSample, const vec2 uv)
{
    mat4 projInv = uCameras[cameraIdx].projInv;

    vec4 sceneProj = vec4(uv.xy * 2.0 - 1.0, depthSample, 1.0);
    vec4 sceneView = uCameras[cameraIdx].viewProjInv * sceneProj;
    return sceneView.xyz / sceneView.w;
}

float GetLodForRadianceSample(const float roughness)
{
    return uEnvironmentData.values.x * roughness;
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

vec4 UnlitBasic()
{
    const vec4 color = GetSampledBaseColor(inUv);

    // NOTE: fog missing

    return vec4(color.rgb, 1.0);
}

vec4 UnlitShadowAlpha(float depthBufferSample, FullGBufferData fd)
{
    GetSimpleSampledGBuffer(inUv, fd);

    const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
    const vec3 worldPos = GetWorldPos(cameraIdx, depthBufferSample, inUv.xy);
    const vec3 camWorldPos = uCameras[cameraIdx].viewInv[3].xyz;

    const uint directionalLightCount = uLightData.directionalLightCount;
    const uint directionalLightBeginIndex = uLightData.directionalLightBeginIndex;
    const vec4 atlasSizeInvSize = uLightData.atlasSizeInvSize;
    CORE_RELAXEDP float fullShadowCoeff = 1.0;
    for (uint lightIdx = 0; lightIdx < directionalLightCount; ++lightIdx) {
        const uint currLightIdx = directionalLightBeginIndex + lightIdx;
        const vec3 L = -uLightData.lights[currLightIdx].dir.xyz; // normalization already done in c-code
        const float NoL = clamp(dot(fd.normal, L), 0.0, 1.0);

        CORE_RELAXEDP float shadowCoeff = 1.0;
        const bool shadowReceiver = true;
        if (shadowReceiver) {
            const uvec4 lightFlags = uLightData.lights[currLightIdx].flags;
            if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(worldPos.xyz, 1.0);
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
        const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
        const uint spotLightCount = uLightData.spotLightCount;
        const uint spotLightLightBeginIndex = uLightData.spotLightBeginIndex;
        for (uint spotIdx = 0; spotIdx < spotLightCount; ++spotIdx) {
            const uint currLightIdx = spotLightLightBeginIndex + spotIdx;

            const vec3 pointToLight = uLightData.lights[currLightIdx].pos.xyz - worldPos.xyz;
            const float dist = length(pointToLight);
            const vec3 L = pointToLight / dist;
            const float NoL = clamp(dot(fd.normal, L), 0.0, 1.0);
            // NOTE: could check for NoL > 0.0 and NoV > 0.0
            CORE_RELAXEDP float shadowCoeff = 1.0;
            const bool shadowReceiver = true;
            if (shadowReceiver) {
                const uvec4 lightFlags = uLightData.lights[currLightIdx].flags;
                if ((lightFlags.x & CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) == CORE_LIGHT_USAGE_SHADOW_LIGHT_BIT) {
                    const vec4 shadowCoord = GetShadowMatrix(lightFlags.y) * vec4(worldPos.xyz, 1.0);
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
    CORE_RELAXEDP vec3 color = fd.baseColor.rgb * clamp(1.0 - fullShadowCoeff, 0.0, 1.0);

    // fog handling
    InplaceFogBlock(CORE_CAMERA_FLAGS, worldPos, camWorldPos.xyz, vec4(color, 1.0), color);

    return vec4(color.xyz, 1.0);
}

vec4 PbrBasic(float depthBufferSample, FullGBufferData fd)
{
    GetSampledGBuffer(inUv, fd);

    // should always be metallic roughness
    InputBrdfData brdfData = CalcBRDFMetallicRoughness(fd.baseColor, fd.material);

    const uint cameraIdx = GetUnpackCameraIndex(uGeneralData);
    const vec3 worldPos = GetWorldPos(cameraIdx, depthBufferSample, inUv.xy);
    const vec3 camWorldPos = uCameras[cameraIdx].viewInv[3].xyz;

    const vec3 V = normalize(camWorldPos - worldPos);
    const float NoV = clamp(dot(fd.normal, V), CORE3D_PBR_LIGHTING_EPSILON, 1.0);

    ShadingData shadingData;
    shadingData.pos = worldPos;
    shadingData.N = fd.normal;
    shadingData.NoV = NoV;
    shadingData.V = V;
    shadingData.f0 = brdfData.f0;
    shadingData.alpha2 = brdfData.alpha2;
    shadingData.diffuseColor = brdfData.diffuseColor;
    CORE_RELAXEDP const float roughness = brdfData.roughness;

    vec3 color = vec3(0.0); // brdfData.diffuseColor
    if ((fd.materialFlags & CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) == CORE_MATERIAL_PUNCTUAL_LIGHT_RECEIVER_BIT) {
        color = CalculateLighting(shadingData, fd.materialFlags);
    }

    if ((fd.materialFlags & CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) == CORE_MATERIAL_INDIRECT_LIGHT_RECEIVER_BIT) {
        // lambert baked into irradianceSample (SH)
        CORE_RELAXEDP vec3 irradiance = GetIrradianceSample(shadingData.N) * shadingData.diffuseColor * fd.ao;

        const vec3 worldReflect = reflect(-shadingData.V, shadingData.N);
        const CORE_RELAXEDP vec3 fIndirect = EnvBRDFApprox(shadingData.f0.xyz, roughness, NoV);
        // ao applied after clear coat
        CORE_RELAXEDP vec3 radianceSample = GetRadianceSample(worldReflect, roughness);
        CORE_RELAXEDP vec3 radiance = radianceSample * fIndirect;
        // apply ao for indirect specular as well (cheap version)
#if 1
        radiance *= fd.ao * SpecularHorizonOcclusion(worldReflect, fd.normal);
#else
        radiance *= EnvSpecularAo(fd.ao, NoV, roughness) * SpecularHorizonOcclusion(worldReflect, normNormal);
#endif

        color += (irradiance + radiance);
    }

    // fog handling
    InplaceFogBlock(CORE_CAMERA_FLAGS, worldPos.xyz, camWorldPos.xyz, vec4(color, 1.0), color);

    color.rgb = clamp(color.rgb, 0.0, CORE_HDR_FLOAT_CLAMP_MAX_VALUE); // zero to hdr max
    return vec4(color.rgb, 1.0);
}

/*
fragment shader for basic pbr materials.
*/
void main(void)
{
    const float depthBufferSample = GetSampledDepthBuffer(inUv);
    if (depthBufferSample < 1.0) {
        FullGBufferData fd = GetUnpackMaterialValues(inUv);
        if (fd.materialType == CORE_MATERIAL_UNLIT) {
            outColor = UnlitBasic();
        } else if (fd.materialType == CORE_MATERIAL_UNLIT_SHADOW_ALPHA) {
            outColor = UnlitShadowAlpha(depthBufferSample, fd);
        } else {
            outColor = PbrBasic(depthBufferSample, fd);
        }
    } else {
        outColor = vec4(0.0);
    }
}
