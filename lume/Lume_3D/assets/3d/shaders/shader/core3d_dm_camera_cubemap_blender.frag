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

layout(location = 0) in vec2 inUv;
layout(location = 1) in flat uint inIndices;

layout(location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////

vec3 GetCubemapCoord()
{
    const vec2 uv = inUv.xy * 2.0 - 1.0;
    const vec3 coords[] = {
        vec3(1.0, -uv.y, -uv.x),  // +X
        vec3(-1.0, -uv.y, uv.x),  // -X
        vec3(uv.x, 1.0, uv.y),    // +Y
        vec3(uv.x, -1.0, -uv.y),  // -Y
        vec3(uv.x, -uv.y, 1.0),   // +Z
        vec3(-uv.x, -uv.y, -1.0), // -Z
    };
    const uint index = min(inIndices, 5U);
    const vec3 cubeUv = normalize(coords[index]);
    return cubeUv;
}

vec4 GetCombinedCubemap()
{
    const vec3 uv = GetCubemapCoord();

    const DefaultMaterialEnvironmentStruct envData = uEnvironmentDataArray[uPc.indices.x];
    const float lodLevel = float(uPc.indices.y);

    const uvec4 multiEnvIndices = envData.multiEnvIndices;
    const uint env1Idx = min(multiEnvIndices.y, CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT - 1);
    const uint env2Idx = min(multiEnvIndices.z, CORE_DEFAULT_MATERIAL_MAX_ENVIRONMENT_COUNT - 1);
    const vec3 env0 =
        textureLod(uCubemapSampler1, uv, lodLevel).xyz * uEnvironmentDataArray[env1Idx].indirectSpecularColorFactor.xyz;
    const vec3 env1 =
        textureLod(uCubemapSampler2, uv, lodLevel).xyz * uEnvironmentDataArray[env2Idx].indirectSpecularColorFactor.xyz;

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
