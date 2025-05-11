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

#ifndef API_RENDER_SHADERS_COMMON_CORE_COLOR_CONVERSION_COMMON_H
#define API_RENDER_SHADERS_COMMON_CORE_COLOR_CONVERSION_COMMON_H

#ifndef VULKAN
#error "This is inteded to be included in shaders. Not fully ported to C/C++."
#endif

#include "render_compatibility_common.h"

// match with core/base color conversions

// No conversion and should be used as default
#define CORE_COLOR_CONVERSION_TYPE_NONE 0
// sRGB to linear conversion
#define CORE_COLOR_CONVERSION_TYPE_SRGB_TO_LINEAR 1
// Linear to sRGB conversion
#define CORE_COLOR_CONVERSION_TYPE_LINEAR_TO_SRGB 2

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

/**
 * Brightness matrix
 * https://docs.rainmeter.net/tips/colormatrix-guide/
 */
mat4 BrightnessMatrix(float brightness)
{
    mat4 matrix = mat4(1.0);
    matrix[3].xyz = vec3(brightness);
    return matrix;
}

/**
 * Contrast matrix
 * https://docs.rainmeter.net/tips/colormatrix-guide/
 */
mat4 ContrastMatrix(float contrast)
{
    const float t = (1.0 - contrast) / 2.0;

    mat4 matrix = mat4(1.0);
    matrix[0].x = contrast;
    matrix[1].y = contrast;
    matrix[2].z = contrast;

    matrix[3].xyz = vec3(t);

    return matrix;
}

/**
 * Saturation matrix
 * https://docs.rainmeter.net/tips/colormatrix-guide/
 */
mat4 SaturationMatrix(float saturation)
{
    const vec3 luminance = vec3(0.3086, 0.6094, 0.0820);

    const float t = (1 - saturation);
    const float sr = t * luminance.r;
    const float sg = t * luminance.g;
    const float sb = t * luminance.b;

    vec3 red = vec3(sr + saturation, sr, sr);
    vec3 green = vec3(sg, sg + saturation, sg);
    vec3 blue = vec3(sb, sb, sb + saturation);

    mat4 matrix = mat4(1.0);

    matrix[0].xyz = red;
    matrix[1].xyz = green;
    matrix[2].xyz = blue;

    return matrix;
}

/**
 * Hue shift color
 * https://gist.github.com/mairod/a75e7b44f68110e1576d77419d608786?permalink_comment_id=3195243#gistcomment-3195243
 */
vec3 HueShift(vec3 color, float hueShift)
{
    float hueRadians = radians(hueShift);
    const vec3 k = vec3(0.57735, 0.57735, 0.57735);
    float cosAngle = cos(hueRadians);
    return vec3(color * cosAngle + cross(k, color) * sin(hueRadians) + k * dot(k, color) * (1.0 - cosAngle));
}

/**
 * Kelvin to Rgb approximation for range between 1000K and 40000K
 * https://tannerhelland.com/2012/09/18/convert-temperature-rgb-algorithm-code.html
 */
vec3 KelvinToRgb(float kelvin)
{
    float temp = kelvin / 100.0;
    float r, g, b;

    if (temp <= 66.0) {
        r = 1.0;
        g = clamp(0.39008157876901960784 * log(temp) - 0.63184144378862745098, 0.0, 1.0);
        b = temp <= 19.0 ? 0.0 : clamp(0.54320678911019607843 * log(temp - 10.0) - 1.19625408914, 0.0, 1.0);
    } else {
        r = clamp(1.29293618606274509804 * pow(temp - 60.0, -0.1332047592), 0.0, 1.0);
        g = clamp(1.12989086089529411765 * pow(temp - 60.0, -0.0755148492), 0.0, 1.0);
        b = 1.0;
    }

    return vec3(r, g, b);
}

/**
 * Color grading for srgb color
 * https://www.shadertoy.com/view/lsdXRn
 */
vec3 ColorGrading(vec3 color, float colorTemperature, vec3 colorTemperatureStrength,
    float colorTemperatureBrightnessNormalization, vec3 presaturation, vec3 gain, vec3 lift, vec3 gamma)
{
    // Calculate original brightness using luminance coefficients
    float originalBrightness = dot(color, vec3(0.2126, 0.7152, 0.0722)); // For srgb colors
    // Adjust color based on color temperature and strength
    color = mix(color, color * KelvinToRgb(colorTemperature), colorTemperatureStrength);
    // Calculate new brightness after temperature adjustment
    float newBrightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    // Normalize brightness to maintain consistency
    color *= mix(1.0, (newBrightness > 1e-6) ? (originalBrightness / newBrightness) : 1.0,
        colorTemperatureBrightnessNormalization);
    // Adjust saturation by mixing with grayscale value
    color = mix(vec3(dot(color, vec3(0.2126, 0.7152, 0.0722))), color, presaturation);
    // Apply lift, gain and gamma adjustments
    return pow((gain * 2.0) * (color + (((lift * 2.0) - vec3(1.0)) * (vec3(1.0) - color))), vec3(0.5) / gamma);
}

#endif // API_RENDER_SHADERS_COMMON_CORE_COLOR_CONVERSION_COMMON_H
