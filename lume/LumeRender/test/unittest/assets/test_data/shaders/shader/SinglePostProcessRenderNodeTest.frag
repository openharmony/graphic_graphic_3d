#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform UBO {
    vec4 b[16];
} ubo;
layout(set = 2, binding = 0) uniform UBO2 {
    vec4 b[16];
} ubo2;
layout(set = 1, binding = 0) uniform sampler uSampler;
layout(set = 1, binding = 1) uniform texture2D uTex;

layout(push_constant) uniform PC {
    vec4 viewportSizeInvSize;
    vec4 factor;
} pc;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

void main(void)
{
    const vec2 uv = inUv;
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    color = ubo.b[3] + pc.factor + ubo2.b[2];
    color = texture(sampler2D(uTex, uSampler), uv);
    color = vec4(color.g, color.r, color.b, 1.0);
    outColor = color;
}
