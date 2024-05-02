
#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"


// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform sampler2D uTex;
layout(set = 1, binding = 1) uniform sampler2D uNearTex;
layout(set = 1, binding = 2) uniform sampler2D uFarTex;
layout(set = 1, binding = 3) uniform sampler2D uDepth;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

float LinearDepth(float depth, float near, float far)
{
    return (far * near) / ((near - far) * depth + far);
}

CORE_RELAXEDP vec4 DepthOfField(vec2 texCoord, vec2 invTexSize, vec4 params1, vec4 params2)
{
    const CORE_RELAXEDP float depth = texture(uDepth,  texCoord, 0).r;
    const float linearDepth = LinearDepth(depth, params2.z, params2.w);

    const float nearBlur = params2.x;
    const float farBlur = params2.y;
    const CORE_RELAXEDP vec4 farPlane0 = textureLod(uFarTex, texCoord, 0);
    const CORE_RELAXEDP vec4 farPlane = textureLod(uFarTex, texCoord, farBlur);
    const CORE_RELAXEDP vec4 focusPlane = textureLod(uTex, texCoord, 0);
    const CORE_RELAXEDP vec4 nearPlane = textureLod(uNearTex, texCoord, nearBlur);

    CORE_RELAXEDP vec4 color;
    const CORE_RELAXEDP float farWeight = farPlane0.a;
    color.rgb = (farPlane.rgb * farWeight) + focusPlane.rgb * (1.0 - farWeight);

    const CORE_RELAXEDP float nearWeight = nearPlane.a;
    color.rgb = (nearPlane.rgb * nearWeight) + color.rgb * (1.0 - nearWeight);
    color.a = 1.0;
    return color;
}

void main(void)
{
    // nearTransitionStart, focusStart, focusEnd, farTransitionEnd
    // nearBlur, farBlur, nearPlane, farPlane
    outColor = DepthOfField(inUv, uPc.viewportSizeInvSize.zw, uLocalData.factors[0], uLocalData.factors[1]);
}
