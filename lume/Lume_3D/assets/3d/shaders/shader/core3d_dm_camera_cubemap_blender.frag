#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "3d/shaders/common/3d_dm_structures_common.h"
#include "render/shaders/common/render_compatibility_common.h"

// sets and specializations

// custom environment set
layout(set = 0, binding = 0, std140) uniform uEnvironmentStructDataArray
{
    DefaultMaterialEnvironmentStruct uEnvironmentDataArray[CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT];
};
layout(set = 0, binding = 1) uniform samplerCube uCubemapSampler1;
layout(set = 0, binding = 2) uniform samplerCube uCubemapSampler2;

struct EnvPushConstantStruct {
    // .x first index for target environment
    // .y lod level
    uvec4 indices;
};

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    EnvPushConstantStruct uPc;
};

// in / out

layout(location = 0) in vec3 inCubeUv;

layout(location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////

vec4 GetCombinedCubemap()
{
    const DefaultMaterialEnvironmentStruct envData = uEnvironmentDataArray[uPc.indices.x];
    const uvec2 envIdx = min(envData.multiEnvIndices.yz, uvec2(CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT - 1));
    const float lodLevel = float(uPc.indices.y);
    vec3 env0 =
        textureLod(uCubemapSampler1, mat3(uEnvironmentDataArray[envIdx.x].envRotation) * inCubeUv, lodLevel).xyz;
    vec3 env1 =
        textureLod(uCubemapSampler2, mat3(uEnvironmentDataArray[envIdx.y].envRotation) * inCubeUv, lodLevel).xyz;

    const float blendVal = envData.blendFactor.x;
    vec3 finalColor = mix(env0, env1, blendVal);
    return vec4(finalColor, 1.0);
}

/**
 * fragment shader for dynamic cubemap
 */
void main(void)
{
    outColor = GetCombinedCubemap();
}
