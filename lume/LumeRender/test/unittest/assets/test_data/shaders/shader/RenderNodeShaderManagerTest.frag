#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler uSampler;
layout(set = 0, binding = 1) uniform texture2D uTex[2];
layout(set = 0, binding = 2) uniform UBO {
    vec4 a;
    vec2 b;
    float c;
} ubo;

layout(push_constant) uniform PC {
    vec4 x;
    vec4 y;
} pc;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

void main(void)
{
    const vec2 uv = inUv;
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    color = texture(sampler2D(uTex[0], uSampler), uv);
    color += texture(sampler2D(uTex[1], uSampler), uv);
    color *= pc.x;
    color += pc.y;
    outColor = color;
}
