#version 460 core
#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

// includes

#include "render/shaders/common/render_color_conversion_common.h"

#include "render/shaders/common/render_post_process_structs_common.h"
#include "render/shaders/common/render_post_process_common.h"
#include "render/shaders/common/render_color_conversion_common.h"

#include "common/bloom_common.h"

// sets

#include "render/shaders/common/render_post_process_layout_common.h"

layout(set = 1, binding = 0) uniform sampler2DArray uImgSampler;
layout(set = 1, binding = 1) uniform sampler2D uBloomSampler;
layout(set = 1, binding = 2) uniform sampler2D uDirtSampler;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

//>DECLARATIONS_CORE_POST_PROCESS
#include "render/shaders/common/render_post_process_blocks.h"

#define TIME_SCALE 0.07f

/*
 * fragment shader for post process and tonemapping
 */
void main(void)
{
    const float layer = uPc.factor.x;
    outColor = textureLod(uImgSampler, vec3(inUv, layer), 0);

    //>FUNCTIONS_CORE_POST_PROCESS
    PostProcessColorFringeBlock(uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_COLOR_FRINGE], inUv,
        uPc.viewportSizeInvSize.zw, uImgSampler, layer, outColor.rgb, outColor.rgb);

    PostProcessBloomCombineBlock(uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_BLOOM], inUv,
        uBloomSampler, uDirtSampler, outColor.rgb, outColor.rgb);

    PostProcessTonemapBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_TONEMAP], outColor.rgb, outColor.rgb);
    const float tickDelta = uGlobalData.renderTimings.y; // tick delta time (ms)
    const vec2 vecCoeffs =
        inUv.xy +
        TIME_SCALE * tickDelta; // Offset uv randomly by a small number based on delta time to create temporal noise
    PostProcessDitherBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_DITHER], vecCoeffs, outColor.rgb, outColor.rgb);
    PostProcessVignetteBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_VIGNETTE], inUv, outColor.rgb, outColor.rgb);
    PostProcessColorConversionBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_COLOR_CONVERSION], outColor.rgba, outColor.rgba);
}
