#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler uSampler;
layout(set = 0, binding = 1) uniform texture2D uTex;

layout(constant_id = 0) const uint C1 = 0;

layout(push_constant) uniform constants {
    uint add;
} pc;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

void main(void)
{
    vec2 uv = inUv;
    vec4 color = texture(sampler2D(uTex, uSampler), uv);
    if ((C1 + pc.add) % 2 == 0) {
        outColor = vec4(color.g, color.b, color.r, color.a);
    } else {
        outColor = vec4(color.b, color.r, color.g, color.a);
    }
}
