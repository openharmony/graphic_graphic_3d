#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"

#include "common/bloom_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform texture2D uTex;
layout(set = 1, binding = 1) uniform sampler uSampler;

// in / out

layout (location = 0) in vec2 inUv;

layout (location = 0) out vec4 outColor;

///////////////////////////////////////////////////////////////////////////////
// bloom threshold downscale

void main()
{
    const vec2 uv = inUv;

    vec3 color = vec3(0.0);
    if ((CORE_BLOOM_QUALITY_NORMAL & CORE_POST_PROCESS_FLAGS) == CORE_BLOOM_QUALITY_NORMAL) {
        color = bloomDownscaleWeighted9(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    } else {
        color = bloomDownscaleWeighted(uv, uPc.viewportSizeInvSize.zw, uTex, uSampler);
    }

    const float luma = CalcLuma(color);
    if (luma < uPc.factor.x)
    {
        color = vec3(0.0);
    } else if (luma < uPc.factor.y)
    {
        const float divisor = uPc.factor.y - uPc.factor.x; // cannot be zero -> if equal, should go to first if
        const float coeff = 1.0 / divisor;
        const float lumaCoeff = (luma - uPc.factor.x) * coeff;
        color *= max(0.0, lumaCoeff);
    }

    color = min(color, CORE_BLOOM_CLAMP_MAX_VALUE);
    outColor = vec4(color, 1.0);
}
