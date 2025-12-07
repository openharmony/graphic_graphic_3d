#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

#include "render/shaders/common/render_color_conversion_common.h"

layout(set = 0, binding = 0) uniform sampler2D uSSAOSampler;

layout(location = 0) in vec2 inUv;
layout(location = 0) out float outSsao;

void main() {
    // gather-optimized gaussian blur

    const vec2 texelSize = 1.0f / vec2(textureSize(uSSAOSampler, 0));

    float result = 0.0f;
    float weightSum = 0.0f;

    const float centerSample = texture(uSSAOSampler, inUv).r;
    result += centerSample * 0.25f;
    weightSum += 0.25f;

    const vec4 topLeft = textureGather(uSSAOSampler, inUv + vec2(-0.75f, -0.75f) * texelSize, 0);
    result += (topLeft.x + topLeft.y + topLeft.z + topLeft.w) * 0.0625f;
    weightSum += 0.25f;

    const vec4 topRight = textureGather(uSSAOSampler, inUv + vec2(0.75f, -0.75f) * texelSize, 0);
    result += (topRight.x + topRight.y + topRight.z + topRight.w) * 0.0625f;
    weightSum += 0.25f;

    const vec4 bottomLeft = textureGather(uSSAOSampler, inUv + vec2(-0.75f, 0.75f) * texelSize, 0);
    result += (bottomLeft.x + bottomLeft.y + bottomLeft.z + bottomLeft.w) * 0.0625f;
    weightSum += 0.25f;

    const vec4 bottomRight = textureGather(uSSAOSampler, inUv + vec2(0.75f, 0.75f) * texelSize, 0);
    result += (bottomRight.x + bottomRight.y + bottomRight.z + bottomRight.w) * 0.0625f;
    weightSum += 0.25f;

    // top
    result += texture(uSSAOSampler, inUv + vec2(0.0f, -1.5f) * texelSize).r * 0.125f;
    // bottom
    result += texture(uSSAOSampler, inUv + vec2(0.0f, 1.5f) * texelSize).r * 0.125f;
    // left
    result += texture(uSSAOSampler, inUv + vec2(-1.5f, 0.0f) * texelSize).r * 0.125f;
    // right
    result += texture(uSSAOSampler, inUv + vec2(1.5f, 0.0f) * texelSize).r * 0.125f;

    weightSum += 0.5f;

    outSsao = result / weightSum;
}