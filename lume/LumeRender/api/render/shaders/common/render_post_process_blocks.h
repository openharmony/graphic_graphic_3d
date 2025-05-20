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

#ifndef API_RENDER_SHADERS_COMMON_POST_PROCESS_BLOCKS_H
#define API_RENDER_SHADERS_COMMON_POST_PROCESS_BLOCKS_H

#include "render_color_conversion_common.h"
#include "render_post_process_structs_common.h"
#include "render_tonemap_common.h"

#define CORE_CLAMP_MAX_VALUE 64512.0

/**
 * returns tonemapped color
 */
void PostProcessTonemapBlock(in uint postProcessFlags, in vec4 tonemapFactor, in vec3 inCol, out vec3 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_TONEMAP_BIT) == POST_PROCESS_SPECIALIZATION_TONEMAP_BIT) {
        const float exposure = tonemapFactor.x;
        const vec3 x = outCol * exposure;
        const uint tonemapType = uint(tonemapFactor.w);
        if (tonemapType == CORE_POST_PROCESS_TONEMAP_ACES) {
            outCol = TonemapAces(x);
        } else if (tonemapType == CORE_POST_PROCESS_TONEMAP_ACES_2020) {
            outCol = TonemapAcesFilmRec2020(x);
        } else if (tonemapType == CORE_POST_PROCESS_TONEMAP_FILMIC) {
            const float exposureEstimate = 6.0f;
            outCol = TonemapFilmic(x * exposureEstimate);
        } else if (tonemapType == CORE_POST_PROCESS_TONEMAP_PBR_NEUTRAL) {
            outCol = TonemapPbrNeutral(x);
        }
    }
}

/**
 * returns vignette applied color with vignette values in the range 0-1
 */
void PostProcessVignetteBlock(
    in uint postProcessFlags, in vec4 vignetteFactor, in vec2 uv, in vec3 inCol, out vec3 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_VIGNETTE_BIT) == POST_PROCESS_SPECIALIZATION_VIGNETTE_BIT) {
        const vec2 uvVal = uv.xy * (vec2(1.0) - uv.yx);
        CORE_RELAXEDP float vignette = uvVal.x * uvVal.y * vignetteFactor.x * 40.0;
        vignette = clamp(pow(vignette, vignetteFactor.y), 0.0, 1.0);
        outCol.rgb *= vignette;
    }
}

/**
 * returns chroma applied color with two additional samples, coefficient in the range 0-1
 */
void PostProcessColorFringeBlock(in uint postProcessFlags, in vec4 chromaFactor, in vec2 uv, in vec2 uvSize,
    in sampler2D imgSampler, in vec3 inCol, out vec3 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_COLOR_FRINGE_BIT) ==
        POST_PROCESS_SPECIALIZATION_COLOR_FRINGE_BIT) {
        // this is cheap chroma
        const vec2 distUv = (uv - 0.5) * 2.0;
        const CORE_RELAXEDP float chroma = dot(distUv, distUv) * chromaFactor.y * chromaFactor.x;

        const vec2 uvDistToImageCenter = chroma * uvSize;
        const CORE_RELAXEDP float chromaRed =
            textureLod(imgSampler, uv - vec2(uvDistToImageCenter.x, uvDistToImageCenter.y), 0).x;
        const CORE_RELAXEDP float chromaBlue =
            textureLod(imgSampler, uv + vec2(uvDistToImageCenter.x, uvDistToImageCenter.y), 0).z;

        outCol.r = chromaRed;
        outCol.b = chromaBlue;
    }
}

/**
 * Array image version
 * returns chroma applied color with two additional samples, coefficient in the range 0-1
 */
void PostProcessColorFringeBlock(in uint postProcessFlags, in vec4 chromaFactor, in vec2 uv, in vec2 uvSize,
    in sampler2DArray imgSampler, in float layer, in vec3 inCol, out vec3 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_COLOR_FRINGE_BIT) ==
        POST_PROCESS_SPECIALIZATION_COLOR_FRINGE_BIT) {
        // this is cheap chroma
        const vec2 distUv = (uv - 0.5) * 2.0;
        const CORE_RELAXEDP float chroma = dot(distUv, distUv) * chromaFactor.y * chromaFactor.x;

        const vec2 uvDistToImageCenter = chroma * uvSize;
        const CORE_RELAXEDP float chromaRed =
            textureLod(imgSampler, vec3(uv - vec2(uvDistToImageCenter.x, uvDistToImageCenter.y), layer), 0).x;
        const CORE_RELAXEDP float chromaBlue =
            textureLod(imgSampler, vec3(uv + vec2(uvDistToImageCenter.x, uvDistToImageCenter.y), layer), 0).z;

        outCol.r = chromaRed;
        outCol.b = chromaBlue;
    }
}

/**
 * returns triangle noise
 * n must be normalized in [0..1] (e.g. texture coordinates)
 */

float TriangleNoise(vec2 n)
{
    // triangle noise, in [-1.0..1.0] range
    n = fract(n * vec2(5.3987, 5.4421));
    n += dot(n.yx, n.xy + vec2(21.5351, 14.3137));

    float xy = n.x * n.y;
    // compute in [0..2] and remap to [-1.0..1.0]
    return fract(xy * 95.4307) + fract(xy * 75.04961) - 1.0;
}

/**
 * returns dithered color
 */
void PostProcessDitherBlock(in uint postProcessFlags, in vec4 ditherFactor, in vec2 uv, in vec3 inCol, out vec3 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_DITHER_BIT) == POST_PROCESS_SPECIALIZATION_DITHER_BIT) {
        const uint ditherType = uint(ditherFactor.w);
        if (ditherType == CORE_POST_PROCESS_DITHER_INTERLEAVED) {
            outCol += (fract(sin(dot(uv.xy, vec2(12.9898, 78.233))) * 43758.5453) - 0.5) * ditherFactor.x;
        } else if (ditherType == CORE_POST_PROCESS_DITHER_TRIANGLE) {
            float noise = TriangleNoise(uv);
            outCol += noise * ditherFactor.x;
        } else if (ditherType == CORE_POST_PROCESS_DITHER_TRIANGLE_RGB) {
            vec3 noiseRGB = vec3(TriangleNoise(uv), TriangleNoise(uv + 0.1337), TriangleNoise(uv + 0.3141));
            outCol += noiseRGB * ditherFactor.x;
        }
    }
}

/**
 * returns additional color conversion
 */
void PostProcessColorConversionBlock(
    in uint postProcessFlags, in vec4 colorConversionFactor, in vec3 inCol, out vec3 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_COLOR_CONVERSION_BIT) ==
        POST_PROCESS_SPECIALIZATION_COLOR_CONVERSION_BIT) {
        const uint conversionFlags = uint(colorConversionFactor.w);
        if ((conversionFlags & CORE_POST_PROCESS_COLOR_CONVERSION_SRGB) != 0) {
            outCol.rgb = LinearToSrgb(outCol.rgb);
        }
    }
}
void PostProcessColorConversionBlock(
    in uint postProcessFlags, in vec4 colorConversionFactor, in vec4 inCol, out vec4 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_COLOR_CONVERSION_BIT) ==
        POST_PROCESS_SPECIALIZATION_COLOR_CONVERSION_BIT) {
        const uint conversionFlags = uint(colorConversionFactor.w);
        if ((conversionFlags & CORE_POST_PROCESS_COLOR_CONVERSION_ALPHA_MULTIPLY) != 0) {
            outCol.rgb *= outCol.a;
        }
        if ((conversionFlags & CORE_POST_PROCESS_COLOR_CONVERSION_SRGB) != 0) {
            outCol.rgb = LinearToSrgb(outCol.rgb);
        }
    }
}

/**
 * returns bloom combine
 */
void PostProcessBloomCombineBlock(in uint postProcessFlags, in vec4 bloomFactor, in vec2 uv, in sampler2D imgSampler,
    in sampler2D dirtImgSampler, in vec3 inCol, out vec3 outCol)
{
    outCol = inCol;
    if ((postProcessFlags & POST_PROCESS_SPECIALIZATION_BLOOM_BIT) == POST_PROCESS_SPECIALIZATION_BLOOM_BIT) {
        // NOTE: lower resolution, more samples might be needed
        const vec3 bloomColor = textureLod(imgSampler, uv, 0).rgb * bloomFactor.z;
        const vec3 dirtColor = textureLod(dirtImgSampler, uv, 0).rgb * bloomFactor.w;
        const vec3 bloomCombine = outCol + bloomColor + dirtColor * max(bloomColor.x, max(bloomColor.y, bloomColor.z));
        outCol.rgb = min(bloomCombine, CORE_CLAMP_MAX_VALUE);
    }
}

#endif // API_RENDER_SHADERS_COMMON_POST_PROCESS_BLOCKS_H
