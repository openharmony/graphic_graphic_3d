#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform sampler uSampler;
layout(set = 1, binding = 1) uniform texture2D uTex;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out float outColor;

void main(void)
{
    // expects to have downscale to half resolution
    const float d0 = texture(sampler2D(uTex, uSampler), inUv).x;
    const float d1 = textureOffset(sampler2D(uTex, uSampler), inUv, ivec2(1, 0)).x;
    const float d2 = textureOffset(sampler2D(uTex, uSampler), inUv, ivec2(0, 1)).x;
    const float d3 = textureOffset(sampler2D(uTex, uSampler), inUv, ivec2(1, 1)).x;

    const float depth = max(max(d0, d1), max(d2, d3));

    outColor = depth;
}
