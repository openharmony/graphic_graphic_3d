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

#ifndef API_RENDER_SHADERS_COMMON_CORE_COLOR_CONVERSION_COMMON_H
#define API_RENDER_SHADERS_COMMON_CORE_COLOR_CONVERSION_COMMON_H

#ifndef VULKAN
#error "This is inteded to be included in shaders. Not fully ported to C/C++."
#endif

#include "render_compatibility_common.h"

/**
 * Calculate luma.
 */
float CalcLuma(const vec3 color)
{
    // Rec. 601 luma
    return 0.299 * color.r + 0.587 * color.g + 0.114 * color.b;
}

/**
 * Calculate simplified luma.
 */
float CalcLumaFxaa(const vec3 color)
{
    // Rec. 601 luma based approximation
    return color.g * (0.587 / 0.299) + color.r;
}

/**
 * Luma weight.
 */
float LumaWeight(const float luma)
{
    return (1.0 / (1.0 + luma));
}

/**
 * Tonemap based on luma.
 */
vec3 TonemapLuma(const vec3 color, const float luma, const float range)
{
    return color / (1.0 + luma / range);
}

/**
 * Inverse tonemap based on luma.
 */
vec3 TonemapLumaInv(const vec3 color, const float luma, const float range)
{
    return color / (1.0 - luma / range);
}

/**
 * Convert sRGB to linear RGB.
 * https://en.wikipedia.org/wiki/SRGB
 */
vec3 SrgbToLinear(const vec3 srgb)
{
    const float mlow = 1.0f / 12.92f;
    const float mhigh = 1.0f / 1.055f;

    const vec3 high = pow((srgb + 0.055f) * mhigh, vec3(2.4f));
    const vec3 low = srgb * mlow;
    const bvec3 cutoff = lessThan(srgb, vec3(0.04045f));
    return mix(high, low, cutoff);
}

/**
 * Convert linear RGB to sRGB.
 * https://en.wikipedia.org/wiki/SRGB
 */
vec3 LinearToSrgb(const vec3 linear)
{
    const float mlow = 12.92f;
    const float mhigh = 1.055f;

    const vec3 high = pow(linear, vec3(0.416f)) * mhigh - 0.055f;
    const vec3 low = linear * mlow;
    const bvec3 cutoff = lessThan(linear, vec3(0.0031308f));
    return mix(high, low, cutoff);
}

/**
 * Convert RGB to YCoCg.
 * https://en.wikipedia.org/wiki/YCoCg
 */
vec3 rgbToYCoCg(const vec3 rgb)
{
    const float y = dot(rgb, vec3(0.25, 0.5, 0.25));
    const float co = dot(rgb, vec3(0.5, 0.0, -0.5));
    const float cg = dot(rgb, vec3(-0.25, 0.5, -0.25));
    return vec3(y, co, cg);
}

/**
 * Convert YCoCg to RGB
 * https://en.wikipedia.org/wiki/YCoCg
 */
vec3 yCoCgToRgb(const vec3 ycocg)
{
    const float y = ycocg.r;
    const float co = ycocg.g;
    const float cg = ycocg.b;
    return vec3(y + co - cg, y + cg, y - co - cg);
}

#endif // API_RENDER_SHADERS_COMMON_CORE_COLOR_CONVERSION_COMMON_H
