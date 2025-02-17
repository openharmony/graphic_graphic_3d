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

struct CustomEnvProperties {
    mat4 transMat;
};
layout(set = 1, binding = 0) uniform sampler2D uCustomSampler;
layout(set = 1, binding = 1) uniform uCustomBuffer
{
    CustomEnvProperties customProperties;
};
layout(set = 1, binding = 2) uniform uCustomBuffer2
{
    CustomEnvProperties customProperties2;
};
layout(set = 1, binding = 3) uniform uCustomBuffer3
{
    CustomEnvProperties customProperties3;
};
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
    vec4 texRes = texture(uCustomSampler, inUv);
    vec3 colorRes = SrgbToLinear(texRes.rgb);
    outColor = vec4(colorRes, 1.0f);
}
