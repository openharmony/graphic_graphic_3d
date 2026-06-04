#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// Light probe receiving uses push constants in dm frag layout, env pass does
// not sample light probes and has its own push constants
#define PUSH_CONSTANT_IN_SHADER

// includes

#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"

// sets

#include "3d/shaders/common/3d_dm_env_frag_layout_common.h"
#define CORE3D_USE_SCENE_FOG_IN_ENV
#include "3d/shaders/common/3d_dm_inplace_env_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"

layout(set = 3, binding = 4, std140) uniform uLightProbeViewMatrices
{
    mat4 uViewInvMatrices[CORE_MAX_NUM_LIGHT_PROBE_BAKES_PER_ITERATION * 6u];
};

layout(push_constant, std430) uniform uPushConstant
{
    uint uViewMatrixIndex;
};

// in / out

layout(location = 0) in vec2 inUv;
layout(location = 1) in flat uint inIndices;

layout(location = 0) out vec4 outColor;

void InplaceEnvironmentBlockLightProbes(in uint environmentType, in uint cameraIdx, in vec2 uv, out vec4 color)
{
    color = vec4(0.0, 0.0, 0.0, 1.0);
    const DefaultMaterialEnvironmentStruct envData = uEnvironmentDataArray[0U];
    CORE_RELAXEDP const float lodLevel = envData.values.y;

    const mat4 viewInv = uViewInvMatrices[uViewMatrixIndex];

    // NOTE: would be nicer to calculate in the vertex shader

    // remove translation from view
    mat4 viewInvEnv = viewInv;
    viewInvEnv[3] = vec4(0.0, 0.0, 0.0, 1.0);
    if ((uCameras[cameraIdx].counts.y & CORE_DEFAULT_CAMERA_ENVIRONMENT_PROJECTION) ==
        CORE_DEFAULT_CAMERA_ENVIRONMENT_PROJECTION) {
        viewInvEnv = viewInvEnv * uCameras[cameraIdx].envProjInv;
    } else {
        viewInvEnv = viewInvEnv * uCameras[cameraIdx].projInv;
    }

    vec4 farPlane = viewInvEnv * vec4(uv.x, uv.y, 1.0, 1.0);
    farPlane.xyz = farPlane.xyz / farPlane.w;

    vec3 colorFactor = envData.envMapColorFactor.xyz;

    if ((environmentType == CORE_BACKGROUND_TYPE_CUBEMAP) ||
        (environmentType == CORE_BACKGROUND_TYPE_EQUIRECTANGULAR)) {
        vec4 nearPlane = viewInvEnv * vec4(uv.x, uv.y, 0.9999, 1.0);
        nearPlane.xyz = nearPlane.xyz / nearPlane.w;

        const vec3 worldView = mat3(envData.envRotation) * normalize(farPlane.xyz - nearPlane.xyz);

        if (environmentType == CORE_BACKGROUND_TYPE_CUBEMAP) {
            const uvec4 multiEnvIndices = envData.multiEnvIndices;
            if (multiEnvIndices.x > 0) {
                vec3 col = vec3(0.0);
                GetBlendedMultiEnvMapSample(
                    multiEnvIndices, worldView, lodLevel, envData.blendFactor.x, col, colorFactor);
                color.rgb = col;
            } else {
                color.rgb = GetEnvMapSample(uImgCubeSampler, worldView, lodLevel);
            }
        } else {
            const vec2 texCoord = vec2(atan(worldView.z, worldView.x) + CORE3D_DEFAULT_ENV_PI, acos(worldView.y)) /
                                  vec2(2.0 * CORE3D_DEFAULT_ENV_PI, CORE3D_DEFAULT_ENV_PI);
            color = textureLod(uImgSampler, texCoord, lodLevel);
        }
    } else if (environmentType == CORE_BACKGROUND_TYPE_IMAGE) {
        const vec2 texCoord = (uv + vec2(1.0)) * 0.5;
        color = textureLod(uImgSampler, texCoord, lodLevel);
    }

    color.xyz *= colorFactor.xyz;

    // fog
    const vec3 camPos = viewInv[3].xyz;
    vec3 fogColor = color.rgb;
    InplaceFogBlock(CORE_CAMERA_FLAGS, farPlane.xyz, camPos, color, fogColor);
    color.rgb = fogColor.rgb;
}

/*
fragment shader for environment sampling
*/
void main(void)
{
    outColor = vec4(0.0, 0.0, 0.0, 1.0);

    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    InplaceEnvironmentBlockLightProbes(CORE_DEFAULT_ENV_TYPE, cameraIdx, inUv, outColor);

    vec2 fragUv;
    CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

    // Post processing disabled, we are storing radiance map values directly
#if 0
    // specialization for post process
    if (CORE_POST_PROCESS_FLAGS > 0) {
        InplacePostProcess(fragUv, outColor);
    }
#endif
}
