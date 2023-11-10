/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

layout(set = 1, binding = 0) uniform sampler2D uImgSampler;
layout(set = 1, binding = 1) uniform sampler2D uBloomSampler;
layout(set = 1, binding = 2) uniform sampler2D uDirtSampler;

// in / out

layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

//>DECLARATIONS_CORE_POST_PROCESS
#include "render/shaders/common/render_post_process_blocks.h"

/*
 * fragment shader for post process and tonemapping
 */
void main(void)
{
    outColor = texture(uImgSampler, inUv);

    //>FUNCTIONS_CORE_POST_PROCESS
    PostProcessColorFringeBlock(uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_COLOR_FRINGE], inUv,
        uPc.viewportSizeInvSize.zw, uImgSampler, outColor.rgb, outColor.rgb);

    PostProcessBloomCombineBlock(uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_BLOOM], inUv,
        uBloomSampler, uDirtSampler, outColor.rgb, outColor.rgb);

    PostProcessTonemapBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_TONEMAP], outColor.rgb, outColor.rgb);
    const float tickDelta = uGlobalData.renderTimings.y; // tick delta time (ms)
    const vec2 vecCoeffs = inUv.xy * tickDelta;
    PostProcessDitherBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_DITHER], vecCoeffs, outColor.rgb, outColor.rgb);
    PostProcessVignetteBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_VIGNETTE], inUv, outColor.rgb, outColor.rgb);
    PostProcessColorConversionBlock(
        uGlobalData.flags.x, uGlobalData.factors[POST_PROCESS_INDEX_COLOR_CONVERSION], outColor.rgb, outColor.rgb);
}
