#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_brdf_common.h"
#include "3d/shaders/common/3d_dm_shadowing_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"
#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets and specializations

#include "3d/shaders/common/3d_dm_frag_layout_common.h"
#include "3d/shaders/common/3d_dm_lighting_common.h"
#define CORE3D_DM_DF_FRAG_INPUT 1
#include "3d/shaders/common/3d_dm_inout_common.h"
#include "3d/shaders/common/3d_dm_inplace_sampling_common.h"

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;
layout(location = 2) out vec4 outBaseColor;
layout(location = 3) out vec4 outMaterial;

uint GetInstanceIndex()
{
    uint instanceIdx = 0U;
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_GPU_INSTANCING_BIT) == CORE_MATERIAL_GPU_INSTANCING_BIT) {
        instanceIdx = GetUnpackFlatIndicesInstanceIdx(inIndices);
    }
    return instanceIdx;
}

///////////////////////////////////////////////////////////////////////////////
// "main" functions

void UnlitBasic()
{
    const uint instanceIdx = GetInstanceIndex();
    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(inUv) * GetUnpackBaseColor(instanceIdx) * inColor;
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_OPAQUE_BIT) == CORE_MATERIAL_OPAQUE_BIT) {
        baseColor.a = 1.0;
    }
    // write out only values which are needed
    // outColor = vec4(0.0);
    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    outVelocityNormal =
        GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), inNormal);
    outBaseColor = baseColor;

    const vec4 material = vec4(0.0);
    outMaterial = GetPackMaterialWithFlags(material, CORE_MATERIAL_TYPE, CORE_MATERIAL_FLAGS);
}

void UnlitShadowAlpha()
{
    const uint instanceIdx = GetInstanceIndex();
    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(inUv, instanceIdx) * GetUnpackBaseColor(instanceIdx) * inColor;
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) ==
        CORE_MATERIAL_ADDITIONAL_SHADER_DISCARD_BIT) {
        if (baseColor.a < GetUnpackAlphaCutoff(instanceIdx)) {
            discard;
        }
    }
    if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_OPAQUE_BIT) == CORE_MATERIAL_OPAQUE_BIT) {
        baseColor.a = 1.0;
    }
    // NOTE: add material flags to baseColor alpha
    // write out only values which are needed
    // outColor = vec4(0.0);
    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    outVelocityNormal =
        GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), inNormal);
    outBaseColor = baseColor;

    const vec4 material = vec4(0.0);
    outMaterial = GetPackMaterialWithFlags(material, CORE_MATERIAL_TYPE, CORE_MATERIAL_FLAGS);
}

void PbrBasic()
{
    const uint instanceIdx = GetInstanceIndex();
    // NOTE: by the spec with blend mode opaque alpha should be 1.0 from this calculation
    CORE_RELAXEDP vec4 baseColor = GetBaseColorSample(inUv, instanceIdx) * GetUnpackBaseColor(instanceIdx) * inColor;
    baseColor.a = clamp(baseColor.a, 0.0, 1.0);
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
        N = GetNormalSample(inUv, instanceIdx);
        const float normalScale = GetUnpackNormalScale(instanceIdx);
        const mat3 tbn = CalcTbnMatrix(normNormal, inTangentW);
        N = CalcFinalNormal(tbn, N, normalScale);
        if ((CORE_MATERIAL_FLAGS & CORE_MATERIAL_CLEARCOAT_BIT) == CORE_MATERIAL_CLEARCOAT_BIT) {
            clearcoatN = GetClearcoatNormalSample(inUv, instanceIdx);
            const float ccNormalScale = GetUnpackClearcoatNormalScale(instanceIdx);
            clearcoatN = CalcFinalNormal(tbn, clearcoatN, ccNormalScale);
        }
    }
    // if no backface culling we flip automatically
    N = gl_FrontFacing ? N : -N;

    CORE_RELAXEDP vec4 material = GetMaterialSample(inUv, instanceIdx) * GetUnpackMaterial(instanceIdx);
    GetFinalCorrectedRoughness(normNormal, material.g);
    const CORE_RELAXEDP float ao = clamp(GetAOSample(inUv, instanceIdx) * GetUnpackAO(instanceIdx), 0.0, 1.0);

    // NOTE: one should write emissive to target and use it additively in lighting
    CORE_RELAXEDP vec3 emissive = GetEmissiveSample(inUv, instanceIdx) * GetUnpackEmissiveColor(instanceIdx);
    emissive = emissive * baseColor.a; // needs to be multiplied with alpha (premultiplied)

    // write out only values which are needed
    outColor = GetPackColor(vec4(emissive, 1.0));
    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    outVelocityNormal = GetPackVelocityAndNormal(GetFinalCalculatedVelocity(inPos.xyz, inPrevPosI.xyz, cameraIdx), N);
    outBaseColor = GetPackBaseColorWithAo(baseColor.xyz, ao);
    outMaterial = GetPackMaterialWithFlags(material, CORE_MATERIAL_TYPE, CORE_MATERIAL_FLAGS);
}

/*
 * fragment shader for basic pbr materials.
 */
void main(void)
{
    if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT) {
        UnlitBasic();
    } else if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT_SHADOW_ALPHA) {
        UnlitShadowAlpha();
    } else {
        PbrBasic();
    }
}
