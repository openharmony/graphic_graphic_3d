#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_tonemap_common.h"

// sets

layout(set = 1, binding = 0) uniform sampler uSampler;
layout(set = 1, binding = 1) uniform texture2D uTex;


// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

void main(void)
{
    const vec2 uv = inUv;
    vec4 color = vec4(0.0);
    color = texture(sampler2D(uTex, uSampler), uv);
    color = vec4(TonemapPbrNeutral(color.rgb), 1.0f);
    outColor = color;
}
