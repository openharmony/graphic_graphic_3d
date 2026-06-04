#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

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
layout(location = 1) out vec4 outCameraDepth;

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

    return baseColor;
}

/*
fragment shader for basic pbr materials.
*/
void main(void)
{
    /*if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT) {
        outColor = unlitBasic();
    } else if (CORE_MATERIAL_TYPE == CORE_MATERIAL_UNLIT_SHADOW_ALPHA) {
        outColor = unlitShadowAlpha();
    } else {
        outColor = pbrBasic();
    }*/

    outColor = unlitBasic();

    if (CORE_POST_PROCESS_FLAGS > 0) {
        vec2 fragUv;
        CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

        InplacePostProcess(fragUv, outColor);
    }

    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    //CORE_RELAXEDP const vec4 projpos = uCameras[cameraIdx].viewProj * vec4(inPos.xyz, 1.0);
    //outCameraDepth = vec4((projpos.z + 1.0f) / 2.0f, (projpos.z + 1.0f) / 2.0f, (projpos.z + 1.0f) / 2.0f, outColor.a);

    const uint matInstanceIdx = GetMaterialInstanceIndex(inIndices);

    // use specular factor to pass the nearPlane and farPlane of the camera, .x for nearPlane, .y for farPlane 
    CORE_RELAXEDP vec4 specular = GetUnpackSpecular(matInstanceIdx);
    CORE_RELAXEDP vec4 coord = uCameras[cameraIdx].view * vec4(inPos.xyz, 1.0);
    CORE_RELAXEDP float curZatCamCoord = abs(coord.z / coord.w);
    float depth = (1.0 / curZatCamCoord - 1.0 / specular.y) / (1.0 / specular.x - 1.0 / specular.y);
    outCameraDepth = vec4(depth, depth , depth, outColor.a);
}
