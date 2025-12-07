#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// From Core:
#include "render/shaders/common/render_color_conversion_common.h"

layout(set = 0, binding = 0) uniform sampler2D uColorSampler;
layout(set = 0, binding = 1) uniform sampler2D uDepthSampler;

// in / out
layout(location = 0) in vec2 inUv;
layout(location = 0) out float outDepth;
layout(location = 1) out vec4 outColor;

float DownsampleDepth(const vec2 uv, const vec2 texelSize) {
    const vec2 halfTexel = texelSize * 0.5f;
    
    const float depth0 = textureLod(uDepthSampler, uv + vec2(-halfTexel.x, -halfTexel.y), 0).r;
    const float depth1 = textureLod(uDepthSampler, uv + vec2(halfTexel.x, -halfTexel.y), 0).r;
    const float depth2 = textureLod(uDepthSampler, uv + vec2(-halfTexel.x, halfTexel.y), 0).r;
    const float depth3 = textureLod(uDepthSampler, uv + vec2(halfTexel.x, halfTexel.y), 0).r;

    return min(min(depth0, depth1), min(depth2, depth3));
}

vec4 DownsampleColor(const vec2 uv, const vec2 texelSize) {
    const vec2 halfTexel = texelSize * 0.5f;
    
    const vec4 color0 = textureLod(uColorSampler, uv + vec2(-halfTexel.x, -halfTexel.y), 0);
    const vec4 color1 = textureLod(uColorSampler, uv + vec2(halfTexel.x, -halfTexel.y), 0);
    const vec4 color2 = textureLod(uColorSampler, uv + vec2(-halfTexel.x, halfTexel.y), 0);
    const vec4 color3 = textureLod(uColorSampler, uv + vec2(halfTexel.x, halfTexel.y), 0);

    return (color0 + color1 + color2 + color3) * 0.25f;
}

void main() {
    const vec2 texelSize = 1.0f / vec2(textureSize(uColorSampler, 0));

    outDepth = DownsampleDepth(inUv, texelSize);
    outColor = DownsampleColor(inUv, texelSize);
}
