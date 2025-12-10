#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler uSampler;
layout(set = 0, binding = 2) uniform texture2D uTex[2];
layout(set = 0, binding = 4) uniform UBO {
    vec4 a;
    vec2 b;
    float c;
} ubo[2];

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

void main(void)
{
    const vec2 uv = inUv;
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (uv.x < 0.5) {
        color = texture(sampler2D(uTex[0], uSampler), uv) + ubo[0].a;
    } else {
        color = texture(sampler2D(uTex[1], uSampler), uv) + ubo[1].a;
    }
    outColor = color;
}
