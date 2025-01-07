#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_indirect_lighting_common.h"
#include "3d/shaders/common/3d_dm_structures_common.h"
#include "3d/shaders/common/3d_dm_target_packing_common.h"

// sets

#include "3d/shaders/common/3d_dm_env_frag_layout_common.h"
#define CORE3D_USE_SCENE_FOG_IN_ENV
#include "3d/shaders/common/3d_dm_inplace_env_common.h"
#include "3d/shaders/common/3d_dm_inplace_post_process.h"

// in / out

layout(location = 0) in vec2 inUv;
layout(location = 1) in flat uint inIndices;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outVelocityNormal;

/*
fragment shader for environment sampling
*/
void main(void)
{
    outColor = vec4(0.0, 0.0, 0.0, 1.0);

    const uint cameraIdx = GetUnpackFlatIndicesCameraIdx(inIndices);
    InplaceEnvironmentBlock(CORE_DEFAULT_ENV_TYPE, cameraIdx, inUv, outColor);

    vec2 fragUv;
    CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

    // specialization for post process
    if (CORE_POST_PROCESS_FLAGS > 0) {
        InplacePostProcess(fragUv, outColor);
    }

    // ray from camera to an infinity point (NDC) for the current frame
    const vec2 resolution = uGeneralData.viewportSizeInvViewportSize.xy;

    const vec4 currentClipSpacePos = vec4(
        fragUv.x / resolution.x * 2.0 - 1.0,
        (resolution.y - fragUv.y) / resolution.y * 2.0 - 1.0,
        1.0, 1.0);
    vec4 currentWorldSpaceDir = uCameras[cameraIdx].viewProjInv * currentClipSpacePos;
    currentWorldSpaceDir /= currentWorldSpaceDir.w;

    // ray from camera to an infinity point (NDC) for the previous frame
    vec4 prevClipSpacePos = uCameras[cameraIdx].viewProjPrevFrame * currentWorldSpaceDir;
    prevClipSpacePos /= prevClipSpacePos.w;
    const vec2 prevScreenSpacePos = vec2((prevClipSpacePos.x + 1.0) * 0.5 * resolution.x, (1.0 - prevClipSpacePos.y) * 0.5 * resolution.y);

    const vec2 velocity = (fragUv.xy - prevScreenSpacePos) * vec2(1.0, -1.0);
    outVelocityNormal = GetPackVelocityAndNormal(velocity, vec3(0.0));
}
