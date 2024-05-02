#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "common/bloom_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform texture2D uTexBloom;
layout(set = 1, binding = 2) uniform sampler uSampler;

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

    vec3 finalColor = min(bloomCombine(baseColor, bloomColor, uPc.factor), CORE_BLOOM_CLAMP_MAX_VALUE);
    outColor = vec4(finalColor, 1.0);
}
