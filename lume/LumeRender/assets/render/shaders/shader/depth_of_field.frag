
#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

// sets

struct DofConfig {
    vec4 factors0;
    vec4 factors1;
};

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    LocalPostProcessPushConstantStruct uPc;
};

layout(set = 0, binding = 0) uniform sampler2D uTex;
layout(set = 0, binding = 1) uniform sampler2D uNearTex;
layout(set = 0, binding = 2) uniform sampler2D uFarTex;
layout(set = 0, binding = 3) uniform sampler2D uDepth;
layout(set = 0, binding = 4, std140) uniform uDofConfigData
{
    DofConfig uLocalData;
};

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

float LinearDepth(float depth, float near, float far)
{
    return (far * near) / ((near - far) * depth + far);
}

CORE_RELAXEDP vec4 DepthOfField(vec2 texCoord, vec2 invTexSize, vec4 params1, vec4 params2)
{
    const CORE_RELAXEDP float depth = texture(uDepth, texCoord, 0).r;
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
    outColor = DepthOfField(inUv, uPc.viewportSizeInvSize.zw, uLocalData.factors0, uLocalData.factors1);
}
