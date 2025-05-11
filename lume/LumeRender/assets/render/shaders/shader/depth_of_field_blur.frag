
#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// specialization

// includes
#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

// sets

//#include "render/shaders/common/render_post_process_layout_common.h"
struct DofConfig {
    vec4 factors0;
    vec4 factors1;
};

layout(push_constant, std430) uniform uPostProcessPushConstant
{
    LocalPostProcessPushConstantStruct uPc;
};

layout(set = 0, binding = 0) uniform sampler2D uTex;
layout(set = 0, binding = 1) uniform sampler2D uDepth;
layout(set = 0, binding = 2, std140) uniform uDofConfigData
{
    DofConfig uLocalData;
};

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outNearColor;
layout (location = 1) out vec4 outFarColor;

float LinearDepth(float depth, float near, float far)
{
    return (far * near) / ((near - far) * depth + far);
}

void main(void)
{
    CORE_RELAXEDP float depth = texture(uDepth,  inUv, 0).r;

    const float linearDepth = LinearDepth(depth, uLocalData.factors1.z, uLocalData.factors1.w);

    // uLocalData.factors0 = nearTransitionStart, focusStart, focusEnd, farTransitionEnd
    float nearTransitionStart = uLocalData.factors0.x;
    float focusStart = uLocalData.factors0.y;
    float focusEnd = uLocalData.factors0.z;
    float farTransitionEnd = uLocalData.factors0.w;
    const float nearTransitionRange = focusStart - nearTransitionStart;
    const float farTransitionRange = farTransitionEnd - focusEnd;

    float nearBlur = uLocalData.factors1.x;
    float farBlur = uLocalData.factors1.y;    

    vec4 color = textureLod(uTex, inUv, 0);

    float coc;
    if (linearDepth < nearTransitionStart) {
        // near blur
        coc = -1.0;
    } else if (linearDepth < focusStart) {
        // transition near to focus
        coc = (linearDepth - focusStart) / nearTransitionRange;
    } else if (linearDepth < focusEnd) {
        // focus
        coc = 0;
    } else if (linearDepth < farTransitionEnd) {
        // transition focus to far
        coc = (linearDepth - focusEnd) / farTransitionRange;
    } else {
        // far
        coc = 1.0;
    }

    outNearColor.a = -1.0 * coc;
    outNearColor.rgb = color.rgb;
    outFarColor.a = coc;
    outFarColor.rgb = color.rgb;
}
