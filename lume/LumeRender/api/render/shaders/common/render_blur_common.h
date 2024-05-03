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

#ifndef API_RENDER_SHADERS_COMMON_CORE_BLUR_COMMON_H
#define API_RENDER_SHADERS_COMMON_CORE_BLUR_COMMON_H

// includes
#include "render_compatibility_common.h"

// specialization for CORE_BLUR_COLOR_TYPE.
// These are defines instead of contants to avoid extra code generation in shaders.
#define CORE_BLUR_TYPE_RGBA 0
#define CORE_BLUR_TYPE_R 1
#define CORE_BLUR_TYPE_RG 2
#define CORE_BLUR_TYPE_RGB 3
#define CORE_BLUR_TYPE_A 4
#define CORE_BLUR_TYPE_SOFT_DOWNSCALE_RGB 5
#define CORE_BLUR_TYPE_DOWNSCALE_RGBA 6
#define CORE_BLUR_TYPE_DOWNSCALE_RGBA_DOF 7
#define CORE_BLUR_TYPE_RGBA_DOF 8
#define CORE_BLUR_FILTER_SIZE 3

#ifndef VULKAN
#include <render/namespace.h>
RENDER_BEGIN_NAMESPACE()
#endif

#ifndef VULKAN
RENDER_END_NAMESPACE()
#endif

#ifdef VULKAN
/**
 http://rastergrid.com/blog/2010/09/efficient-gaussian-blur-with-linear-sampling/
*/

const float CORE_BLUR_OFFSETS[CORE_BLUR_FILTER_SIZE] = { 0.0, 1.3846153846, 3.2307692308 };
const float CORE_BLUR_WEIGHTS[CORE_BLUR_FILTER_SIZE] = { 0.2270270270, 0.3162162162, 0.0702702703 };

vec4 GaussianBlurRGBA(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    vec4 color = textureLod(sampler2D(tex, sampl), uv, 0) * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dir.xy;

        color +=
            textureLod(sampler2D(tex, sampl), (vec2(fragCoord) + currOffset) * invTexSize, 0) * CORE_BLUR_WEIGHTS[idx];
        color +=
            textureLod(sampler2D(tex, sampl), (vec2(fragCoord) - currOffset) * invTexSize, 0) * CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

vec3 GaussianBlurRGB(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    vec3 color = textureLod(sampler2D(tex, sampl), uv, 0).xyz * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dir.xy;

        color += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) + currOffset) * invTexSize, 0).xyz *
                 CORE_BLUR_WEIGHTS[idx];
        color += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) - currOffset) * invTexSize, 0).xyz *
                 CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

vec2 GaussianBlurRG(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    vec2 color = textureLod(sampler2D(tex, sampl), uv, 0).xy * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dir.xy;

        color += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) + currOffset) * invTexSize, 0).xy *
                 CORE_BLUR_WEIGHTS[idx];
        color += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) - currOffset) * invTexSize, 0).xy *
                 CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

float GaussianBlurR(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    float color = textureLod(sampler2D(tex, sampl), uv, 0).x * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dir.xy;

        color += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) + currOffset) * invTexSize, 0).x *
                 CORE_BLUR_WEIGHTS[idx];
        color += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) - currOffset) * invTexSize, 0).x *
                 CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

float GaussianBlurA(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    float alpha = textureLod(sampler2D(tex, sampl), uv, 0).a * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dir.xy;

        alpha += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) + currOffset, 0) * invTexSize, 0).a *
                 CORE_BLUR_WEIGHTS[idx];
        alpha += textureLod(sampler2D(tex, sampl), (vec2(fragCoord) - currOffset, 0) * invTexSize, 0).a *
                 CORE_BLUR_WEIGHTS[idx];
    }

    return alpha;
}

vec4 GaussianBlurRGBALayer(
    texture2DArray tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec3 dirLayer, const vec2 invTexSize)
{
    vec4 color = textureLod(sampler2DArray(tex, sampl), vec3(uv, dirLayer.z), 0) * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dirLayer.xy;

        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) + currOffset) * invTexSize, dirLayer.z), 0) *
            CORE_BLUR_WEIGHTS[idx];
        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) - currOffset) * invTexSize, dirLayer.z), 0) *
            CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

vec3 GaussianBlurRGBLayer(
    texture2DArray tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec3 dirLayer, const vec2 invTexSize)
{
    vec3 color = textureLod(sampler2DArray(tex, sampl), vec3(uv, dirLayer.z), 0).xyz * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dirLayer.xy;

        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) + currOffset) * invTexSize, dirLayer.z), 0)
                .xyz *
            CORE_BLUR_WEIGHTS[idx];
        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) - currOffset) * invTexSize, dirLayer.z), 0)
                .xyz *
            CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

vec2 GaussianBlurRGLayer(
    texture2DArray tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec3 dirLayer, const vec2 invTexSize)
{
    vec2 color = textureLod(sampler2DArray(tex, sampl), vec3(uv, dirLayer.z), 0).xy * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dirLayer.xy;

        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) + currOffset) * invTexSize, dirLayer.z), 0)
                .xy *
            CORE_BLUR_WEIGHTS[idx];
        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) - currOffset) * invTexSize, dirLayer.z), 0)
                .xy *
            CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

float GaussianBlurRLayer(
    texture2DArray tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec3 dirLayer, const vec2 invTexSize)
{
    float color = textureLod(sampler2DArray(tex, sampl), vec3(uv, dirLayer.z), 0).x * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dirLayer.xy;

        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) + currOffset) * invTexSize, dirLayer.z), 0).x *
            CORE_BLUR_WEIGHTS[idx];
        color +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) - currOffset) * invTexSize, dirLayer.z), 0).x *
            CORE_BLUR_WEIGHTS[idx];
    }

    return color;
}

float GaussianBlurALayer(
    texture2DArray tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec3 dirLayer, const vec2 invTexSize)
{
    float alpha = textureLod(sampler2DArray(tex, sampl), vec3(uv, dirLayer.z), 0).a * CORE_BLUR_WEIGHTS[0];

    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dirLayer.xy;

        alpha +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) + currOffset, 0) * invTexSize, dirLayer.z), 0)
                .a *
            CORE_BLUR_WEIGHTS[idx];
        alpha +=
            textureLod(sampler2DArray(tex, sampl), vec3((vec2(fragCoord) - currOffset, 0) * invTexSize, dirLayer.z), 0)
                .a *
            CORE_BLUR_WEIGHTS[idx];
    }

    return alpha;
}

#define CORE_BLUR_SOFT_HEAVY_SAMPLES 0
// NOTE: dir not used
vec3 SoftDownscaleRGB(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
#if (CORE_BLUR_SOFT_HEAVY_SAMPLES == 1)

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
    vec3 color = textureLod(sampler2D(tex, sampl), uv + ths, 0).xyz * 0.5;
    // corners
    // 1.0 / 8.0 = 0.125
    color = textureLod(sampler2D(tex, sampl), uv - ths, 0).xyz * 0.125 + color;
    color = textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y - ths.y), 0).xyz * 0.125 + color;
    color = textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y + ths.y), 0).xyz * 0.125 + color;
    color = textureLod(sampler2D(tex, sampl), uv + ths, 0).xyz * 0.125 + color;

#endif

    return color;
}

vec4 DownscaleRGBA(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    return textureLod(sampler2D(tex, sampl), uv, 0);
}

vec4 DownscaleRGBADof(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    const vec2 ths = invTexSize * 0.5; // 0.5: half

    vec4 color = vec4(0);

    // 1.0 / 8.0 = 0.125
    float weights[5] = { 0.5, 0.125, 0.125, 0.125, 0.125 }; // 0.5: weight, 0.125: weight
    vec4 samples[5] = {
        // center
        textureLod(sampler2D(tex, sampl), uv, 0),
        // corners
        textureLod(sampler2D(tex, sampl), uv - ths, 0),
        textureLod(sampler2D(tex, sampl), vec2(uv.x + ths.x, uv.y - ths.y), 0),
        textureLod(sampler2D(tex, sampl), vec2(uv.x - ths.x, uv.y + ths.y), 0),
        textureLod(sampler2D(tex, sampl), uv + ths, 0),
    };
    float weight = 0.0;
    for (int i = 0; i < 5; ++i) { // 5: size
        weight += samples[i].a;
    }
    if (weight > 0.0) {
        for (int i = 0; i < 5; ++i) { // 5:size
            color += samples[i] * weights[i];
        }
    } else {
        color = samples[0];
    }

    return color;
}

vec4 BlurRGBADof(
    texture2D tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec2 dir, const vec2 invTexSize)
{
    const vec2 ths = invTexSize * 0.5; // 0.5:half

    CORE_RELAXEDP vec4 color = vec4(0);

    CORE_RELAXEDP vec4 samples[1 + 2 * CORE_BLUR_FILTER_SIZE]; // 2: idx
    samples[0] = textureLod(sampler2D(tex, sampl), uv, 0);
    float weight = samples[0].a;
    for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
        vec2 currOffset = vec2(CORE_BLUR_OFFSETS[idx]) * dir.xy;

        samples[idx * 2 - 1] = textureLod(sampler2D(tex, sampl), // 2 : idx
            (vec2(fragCoord) + currOffset) * invTexSize, 0); // 2: index
        weight += samples[idx * 2 - 1].a; // 2: size
        samples[idx * 2] = textureLod(sampler2D(tex, sampl), (vec2(fragCoord) - currOffset) * invTexSize, 0); // 2:idx
        weight += samples[idx * 2].a; // 2: size
    }
    if (weight > 0.0) {
        weight = 1.0 / weight;
        color = samples[0] * CORE_BLUR_WEIGHTS[0] * weight;
        for (int idx = 1; idx < CORE_BLUR_FILTER_SIZE; ++idx) {
            color += samples[idx * 2 - 1] * CORE_BLUR_WEIGHTS[idx] * weight; // 2: idx
            color += samples[idx * 2] * CORE_BLUR_WEIGHTS[idx] * weight; // 2 : idex
        }
    } else {
        color = samples[0];
    }

    return color;
}

vec3 SoftDownscaleRGBLayer(
    texture2DArray tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec3 dirLayer, const vec2 invTexSize)
{
#if (CORE_BLUR_SOFT_HEAVY_SAMPLES == 1)

    // first, 9 samples (calculate coefficients)
    const float diagCoeff = (1.0f / 32.0f); // 32.0: param
    const float stepCoeff = (2.0f / 32.0f); // 2.0: param, 32.0: param
    const float centerCoeff = (4.0f / 32.0f); //4.0: param, 32.0: param

    const vec2 ts = invTexSize;

    vec3 color = textureLod(sampler2DArray(tex, sampl), vec3(uv.x - ts.x, uv.y - ts.y, dirLayer.z), 0).xyz * diagCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x, uv.y - ts.y, dirLayer.z), 0).xyz * stepCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x + ts.x, uv.y - ts.y, dirLayer.z), 0).xyz * diagCoeff;

    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x - ts.x, uv.y, dirLayer.z), 0).xyz * stepCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x, uv.y, dirLayer.z), 0).xyz * centerCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x + ts.x, uv.y, dirLayer.z), 0).xyz * stepCoeff;

    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x - ts.x, uv.y + ts.y, dirLayer.z), 0).xyz * diagCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x, uv.y + ts.y, dirLayer.z), 0).xyz * centerCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x + ts.x, uv.y + ts.y, dirLayer.z), 0).xyz * diagCoeff;

    // then center square
    const vec2 ths = ts * 0.5;

    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x - ths.x, uv.y - ths.y, dirLayer.z), 0).xyz * centerCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x + ths.x, uv.y - ths.y, dirLayer.z), 0).xyz * centerCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x - ths.x, uv.y + ths.y, dirLayer.z), 0).xyz * centerCoeff;
    color += textureLod(sampler2DArray(tex, sampl), vec3(uv.x + ths.x, uv.y + ths.y, dirLayer.z), 0).xyz * centerCoeff;

#else

    const vec2 ths = invTexSize * 0.5;

    // center
    vec3 color = textureLod(sampler2DArray(tex, sampl), vec3(uv + ths, dirLayer.z), 0).xyz * 0.5;
    // corners
    // 1.0 / 8.0 = 0.125
    color = textureLod(sampler2DArray(tex, sampl), vec3(uv - ths, dirLayer.z), 0).xyz * 0.125 + color;
    color = textureLod(sampler2DArray(tex, sampl), vec3(uv.x + ths.x, uv.y - ths.y, dirLayer.z), 0).xyz * 0.125 + color;
    color = textureLod(sampler2DArray(tex, sampl), vec3(uv.x - ths.x, uv.y + ths.y, dirLayer.z), 0).xyz * 0.125 + color;
    color = textureLod(sampler2DArray(tex, sampl), vec3(uv + ths, dirLayer.z), 0).xyz * 0.125 + color;

#endif

    return color;
}

vec4 DownscaleRGBALayer(
    texture2DArray tex, sampler sampl, const vec2 fragCoord, const vec2 uv, const vec3 dirLayer, const vec2 invTexSize)
{
    return textureLod(sampler2DArray(tex, sampl), vec3(uv, dirLayer.z), 0);
}
#endif

#endif // API_RENDER_SHADERS_COMMON_CORE_BLUR_COMMON_H
