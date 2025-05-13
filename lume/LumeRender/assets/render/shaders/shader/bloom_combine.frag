#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_post_process_structs_common.h"

#include "common/bloom_common.h"

// sets

layout(set = 0, binding = 0) uniform texture2D uTex;
layout(set = 0, binding = 1) uniform texture2D uTexBloom;
layout(set = 0, binding = 2) uniform sampler uSampler;

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    LocalPostProcessPushConstantStruct uPc;
};

layout(constant_id = 0) const uint CORE_POST_PROCESS_FLAGS = 0;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////
// bloom combine
// bloom parameters:
// .x is thresholdHard, .y is thresholdSoft, .z is amountCoefficient, .w is dirtMaskCoefficient

void main()
{
    const vec2 uv = inUv;
    const vec3 baseColor = textureLod(sampler2D(uTex, uSampler), uv, 0).xyz;
    // NOTE: more samples (lower resolution)
    const vec3 bloomColor = textureLod(sampler2D(uTexBloom, uSampler), uv, 0).xyz;

    vec3 finalColor = min(BloomCombine(baseColor, bloomColor, uPc.factor), CORE_BLOOM_CLAMP_MAX_VALUE);
    outColor = vec4(finalColor, 1.0);
}
