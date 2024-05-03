/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
