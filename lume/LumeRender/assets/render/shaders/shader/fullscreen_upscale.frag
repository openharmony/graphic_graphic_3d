#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"

// sets

layout(set = 0, binding = 0) uniform sampler uSampler;
layout(set = 0, binding = 1) uniform texture2D uTex;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

struct PushConstantStruct
{
    vec4 texSizeInvTexSize;
};

layout(push_constant, std430) uniform uPushConstant
{
    PushConstantStruct uPc;
};

void main(void)
{
    const vec2 ts = uPc.texSizeInvTexSize.zw * 2.0;
    const vec2 uv = inUv;

    // center
    vec4 color = textureLod(sampler2D(uTex, uSampler), uv, 0) * 0.5;
    // corners
    // 1.0 / 8.0 = 0.125
    color += textureLod(sampler2D(uTex, uSampler), uv - ts, 0) * 0.125;
    color += textureLod(sampler2D(uTex, uSampler), uv + vec2(ts.x, -ts.y), 0) * 0.125;
    color += textureLod(sampler2D(uTex, uSampler), uv + vec2(-ts.x, ts.y), 0) * 0.125;
    color += textureLod(sampler2D(uTex, uSampler), uv + ts, 0) * 0.125;

    outColor = color.rgba;
}
