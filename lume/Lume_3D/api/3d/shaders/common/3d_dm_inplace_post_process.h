/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef SHADERS__COMMON__3D_DM_INPLACE_POST_PROCESS_H
#define SHADERS__COMMON__3D_DM_INPLACE_POST_PROCESS_H

// DECLARATIONS_CORE_POST_PROCESS
#include "render/shaders/common/render_post_process_blocks.h"

// uPostProcessData must be defined in pipeline layout

#ifdef VULKAN

void InplacePostProcess(in vec2 fragUv, inout vec4 color)
{
    // FUNCTIONS_CORE_POST_PROCESS
    PostProcessTonemapBlock(
        uPostProcessData.flags.x, uPostProcessData.factors[POST_PROCESS_INDEX_TONEMAP], color.rgb, color.rgb);

    const float tickDelta = uPostProcessData.renderTimings.y; // tick delta time (ms)
    const vec2 vecCoeffs = fragUv.xy * tickDelta;
    PostProcessDitherBlock(
        uPostProcessData.flags.x, uPostProcessData.factors[POST_PROCESS_INDEX_DITHER], vecCoeffs, color.rgb, color.rgb);
    PostProcessVignetteBlock(
        uPostProcessData.flags.x, uPostProcessData.factors[POST_PROCESS_INDEX_VIGNETTE], fragUv, color.rgb, color.rgb);
    PostProcessColorConversionBlock(
        uPostProcessData.flags.x, uPostProcessData.factors[POST_PROCESS_INDEX_COLOR_CONVERSION], color.rgb, color.rgb);
}

#endif

#endif // SHADERS__COMMON__3D_DM_INPLACE_POST_PROCESS_H
