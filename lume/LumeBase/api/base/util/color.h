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

#ifndef API_BASE_UTIL_COLOR_H
#define API_BASE_UTIL_COLOR_H

#include <cstdint>

#include <base/math/vector.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
constexpr const uint8_t COLOR_SHIFT_0 = 0;
constexpr const uint8_t COLOR_SHIFT_8 = 8;
constexpr const uint8_t COLOR_SHIFT_16 = 16;
constexpr const uint8_t COLOR_SHIFT_24 = 24;

constexpr const bool MAKE_COLOR_AS_LINEAR = true;

/** Color values are in linear space (non premultiplied value)
 * Linear color space (non-srgb values)
 * Prefer using 32-bit color (uint32_t) in sRGB format
 */
using Color = Math::Vec4;

inline float SRGBToLinearConv(const float srgb)
{
    float col = srgb;
    if (srgb <= 0.04045f) {
        col *= (1.f / 12.92f);
    } else {
        col = Math::pow((col + 0.055f) * (1.f / 1.055f), 2.4f);
    }
    return col;
}

inline float LinearToSRGBConv(const float linear)
{
    float srgb = linear;
    if (srgb <= 0.0031308f) {
        srgb *= 12.92f;
    } else {
        srgb = 1.0550000667572021484375f * Math::pow(srgb, 1.f / 2.4f) - 0.05500000715255737305f;
    }
    return srgb;
}

/** Input color in linear ARGB format (uint32_t)
 * Output color in linear (Vec4)
 */
constexpr Color MakeColorFromLinear(const uint32_t color)
{
    uint8_t b = (color >> COLOR_SHIFT_0) & UINT8_MAX;
    uint8_t g = (color >> COLOR_SHIFT_8) & UINT8_MAX;
    uint8_t r = (color >> COLOR_SHIFT_16) & UINT8_MAX;
    uint8_t a = (color >> COLOR_SHIFT_24) & UINT8_MAX;

    return { r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
}

/** Input color in sRGB ARGB format (uint32_t)
 * Output color in linear (Vec4)
 */
inline Color MakeColorFromSRGB(const uint32_t color)
{
    uint8_t b = (color >> COLOR_SHIFT_0) & UINT8_MAX;
    uint8_t g = (color >> COLOR_SHIFT_8) & UINT8_MAX;
    uint8_t r = (color >> COLOR_SHIFT_16) & UINT8_MAX;
    uint8_t a = (color >> COLOR_SHIFT_24) & UINT8_MAX;

    return { SRGBToLinearConv(r / 255.0f), SRGBToLinearConv(g / 255.0f), SRGBToLinearConv(b / 255.0f), a / 255.0f };
}

/** Input color in linear (Vec4)
 * Output color in linear (uint32_t)
 */
constexpr uint32_t FromColorToLinear(Color color)
{
    color *= 255.f;
    uint32_t b = ((uint32_t)color.z & UINT8_MAX) << COLOR_SHIFT_0;
    uint32_t g = ((uint32_t)color.y & UINT8_MAX) << COLOR_SHIFT_8;
    uint32_t r = ((uint32_t)color.x & UINT8_MAX) << COLOR_SHIFT_16;
    uint32_t a = ((uint32_t)color.w & UINT8_MAX) << COLOR_SHIFT_24;

    return a | r | g | b;
}

/** Input color in linear (Vec4)
 * Output color in sRGB (uint32_t)
 */
inline uint32_t FromColorToSRGB(Color color)
{
    // rounding should be handled in some smart way to get actual min and max values too
    uint32_t b = ((uint32_t)(LinearToSRGBConv(color.z) * 255.f) & UINT8_MAX) << COLOR_SHIFT_0;
    uint32_t g = ((uint32_t)(LinearToSRGBConv(color.y) * 255.f) & UINT8_MAX) << COLOR_SHIFT_8;
    uint32_t r = ((uint32_t)(LinearToSRGBConv(color.x) * 255.f) & UINT8_MAX) << COLOR_SHIFT_16;
    uint32_t a = ((uint32_t)(color.w * 255.f) & UINT8_MAX) << COLOR_SHIFT_24;

    return a | r | g | b;
}

/** Input color in linear (Vec4)
 * Output color in linear (uint32_t)
 */
constexpr uint32_t FromColorRGBAToLinear(Color color)
{
    color *= 255.f;
    uint32_t r = ((uint32_t)color.x & UINT8_MAX) << COLOR_SHIFT_0;
    uint32_t b = ((uint32_t)color.z & UINT8_MAX) << COLOR_SHIFT_16;
    uint32_t g = ((uint32_t)color.y & UINT8_MAX) << COLOR_SHIFT_8;
    uint32_t a = ((uint32_t)color.w & UINT8_MAX) << COLOR_SHIFT_24;

    return a | b | g | r;
}

/** Input color in linear (Vec4)
 * Output color in sRGB (uint32_t)
 */
inline uint32_t FromColorRGBAToSRGB(Color color)
{
    uint32_t r = ((uint32_t)(LinearToSRGBConv(color.x) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_0;
    uint32_t b = ((uint32_t)(LinearToSRGBConv(color.z) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_16;
    uint32_t g = ((uint32_t)(LinearToSRGBConv(color.y) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_8;
    uint32_t a = ((uint32_t)(color.w * 255.0f) & UINT8_MAX) << COLOR_SHIFT_24;

    return a | b | g | r;
}

inline uint32_t FromColorRGBA(const Color color)
{
    if (MAKE_COLOR_AS_LINEAR) {
        return FromColorRGBAToLinear(color);
    } else {
        return FromColorRGBAToSRGB(color);
    }
}

static const Color BLACK_COLOR = MakeColorFromLinear(0xFF000000);
static const Color WHITE_COLOR = MakeColorFromLinear(0xFFFFFFFF);
BASE_END_NAMESPACE()

#endif // API_BASE_UTIL_COLOR_H
