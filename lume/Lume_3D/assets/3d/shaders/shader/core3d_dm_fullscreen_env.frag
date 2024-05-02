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
    InplaceEnvironmentBlock(CORE_DEFAULT_ENV_TYPE, cameraIdx, inUv, uImgCubeSampler, uImgSampler, outColor);

    // specialization for post process
    if (CORE_POST_PROCESS_FLAGS > 0) {
        vec2 fragUv;
        CORE_GET_FRAGCOORD_UV(fragUv, gl_FragCoord.xy, uGeneralData.viewportSizeInvViewportSize.zw);

        InplacePostProcess(fragUv, outColor);
    }

    // TODO: calculate camera velocity
    outVelocityNormal = GetPackVelocityAndNormal(vec2(0.0), vec3(0.0));
}
