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

#ifndef SHADERS__COMMON__BLOOM_COMMON_H
#define SHADERS__COMMON__BLOOM_COMMON_H

#include "render/shaders/common/render_compatibility_common.h"
#include "render/shaders/common/render_color_conversion_common.h"
#include "render/shaders/common/render_post_process_structs_common.h"

#define CORE_BLOOM_CLAMP_MAX_VALUE 64512.0

/*
Combines bloom color with the given base color.
*/
vec3 bloomCombine(vec3 baseColor, vec3 bloomColor, vec4 bloomParameters)
{
    return baseColor + bloomColor * bloomParameters.z;
}

#define CORE_ENABLE_HEAVY_SAMPLES 1

/*
 * Downscales samples.
 * "Firefly" filter with weighting.
 */
vec3 bloomDownscaleWeighted(vec2 uv, vec2 invTexSize, texture2D tex, sampler sampl)
{
    // NOTE: should change to 9 tap filter with bilinears

#if (CORE_ENABLE_HEAVY_SAMPLES == 1)
    // first, 9 samples (calculate coefficients)
    const float diagCoeff = (1.0f / 32.0f);
    const float stepCoeff = (2.0f / 32.0f);
    const float centerCoeff = (4.0f / 32.0f);

    const vec2 ts = invTexSize;

    float fullWeight = 0.00001;
    //
    vec3 colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x - ts.x, uv.y - ts.y), 0).xyz;
    float weight = 1.0 / (1 + CalcLuma(colSample));
    vec3 color = colSample * diagCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x, uv.y - ts.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * stepCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x + ts.x, uv.y - ts.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * diagCoeff * weight;
    fullWeight += weight;

    //
    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x - ts.x, uv.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * stepCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x, uv.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * centerCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x + ts.x, uv.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * stepCoeff * weight;
    fullWeight += weight;

    //
    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x - ts.x, uv.y + ts.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * diagCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x, uv.y + ts.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * centerCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x + ts.x, uv.y + ts.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * diagCoeff * weight;
    fullWeight += weight;

    // then center square
    const vec2 ths = ts * 0.5;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y - ths.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * centerCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y - ths.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * centerCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y + ths.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * centerCoeff * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y + ths.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * centerCoeff * weight;
    fullWeight += weight;

    color *= (13.0 / fullWeight);

#else

    const vec2 ths = invTexSize * 0.5;

    float fullWeight = 0.00001;

    // center
    vec3 colSample = textureLod(sampler2D(tex, sampl), uv, 0).xyz;
    float weight = 1.0 / (1 + CalcLuma(colSample));
    vec3 color = colSample * 0.5 * weight;
    fullWeight += weight;

    // corners
    // 1.0 / 8.0 = 0.125
    colSample = textureLod(sampler2D(tex, sampl), uv - ths, 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * 0.125 * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y - ths.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * 0.125 * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y + ths.y), 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * 0.125 * weight;
    fullWeight += weight;

    colSample = textureLod(sampler2D(tex, sampl), uv + ths, 0).xyz;
    weight = 1.0 / (1 + CalcLuma(colSample));
    color += colSample * 0.125 * weight;
    fullWeight += weight;

    color *= (5.0 / fullWeight);
#endif

    return color;
}

vec3 bloomDownscale(vec2 uv, vec2 invTexSize, texture2D tex, sampler sampl)
{
#if (CORE_ENABLE_HEAVY_SAMPLES == 1)
    // first, 9 samples (calculate coefficients)
    const float diagCoeff = (1.0f / 32.0f);
    const float stepCoeff = (2.0f / 32.0f);
    const float centerCoeff = (4.0f / 32.0f);

    const vec2 ts = invTexSize;

    vec3 color = textureLod(sampler2D(tex, sampl), vec2(uv.x - ts.x, uv.y - ts.y), 0).xyz * diagCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x, uv.y - ts.y), 0).xyz * stepCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x + ts.x, uv.y - ts.y), 0).xyz * diagCoeff;

    color += textureLod(sampler2D(tex, sampl), vec2(uv.x - ts.x, uv.y), 0).xyz * stepCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x, uv.y), 0).xyz * centerCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x + ts.x, uv.y), 0).xyz * stepCoeff;

    color += textureLod(sampler2D(tex, sampl), vec2(uv.x - ts.x, uv.y + ts.y), 0).xyz * diagCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x, uv.y + ts.y), 0).xyz * centerCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x + ts.x, uv.y + ts.y), 0).xyz * diagCoeff;

    // then center square
    const vec2 ths = ts * 0.5;

    color += textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y - ths.y), 0).xyz * centerCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y - ths.y), 0).xyz * centerCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y + ths.y), 0).xyz * centerCoeff;
    color += textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y + ths.y), 0).xyz * centerCoeff;

#else

    const vec2 ths = invTexSize * 0.5;

    // center
    vec3 color = textureLod(sampler2D(tex, sampl), uv, 0).xyz * 0.5;
    // corners
    // 1.0 / 8.0 = 0.125
    color = textureLod(sampler2D(tex, sampl), uv - ths, 0).xyz * 0.125 + color;
    color = textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y - ths.y), 0).xyz * 0.125 + color;
    color = textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y + ths.y), 0).xyz * 0.125 + color;
    color = textureLod(sampler2D(tex, sampl), uv + ths, 0).xyz * 0.125 + color;

#endif

    return color;
}

/*
Upscale samples.
*/
vec3 bloomUpscale(vec2 uv, vec2 invTexSize, texture2D tex, sampler sampl)
{
    const vec2 ts = invTexSize * 2.0;

    // center
    vec3 color = texture(sampler2D(tex, sampl), uv).xyz * (1.0 / 2.0);
    // corners
    color = texture(sampler2D(tex, sampl), uv - ts).xyz
        * (1.0 / 8.0) + color;
    color = texture(sampler2D(tex, sampl), uv + vec2(ts.x, -ts.y)).xyz
        * (1.0 / 8.0) + color;
    color = texture(sampler2D(tex, sampl), uv + vec2(-ts.x, ts.y)).xyz
        * (1.0 / 8.0) + color;
    color = texture(sampler2D(tex, sampl), uv + ts).xyz
        * (1.0 / 8.0) + color;

    return color;
}

#endif // SHADERS__COMMON__BLOOM_COMMON_H
