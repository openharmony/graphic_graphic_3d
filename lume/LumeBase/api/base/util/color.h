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

/** Color values are in linear space (non premultiplied value)
 * Linear color space (non-srgb values)
 * Default initialized to ones (opaque white).
 */
class Color {
public:
    union {
        struct {
            float x;
            float y;
            float z;
            float w;
        };
        struct {
            float r;
            float g;
            float b;
            float a;
        };

        float data[4] { 0.0f };
    };

    constexpr Color() noexcept : x(1.f), y(1.f), z(1.f), w(1.f) {}

    constexpr explicit Color(float val) noexcept : x(val), y(val), z(val), w(val) {}

    /** Will set the alpha channel to 1.0
     */
    constexpr Color(float x, float y, float z) noexcept : x(x), y(y), z(z), w(1.0f) {}

    constexpr Color(float x, float y, float z, float w) noexcept : x(x), y(y), z(z), w(w) {}

    /** Constructs Color from ARGB format (uint32_t)
     */
    constexpr explicit Color(const uint32_t color) noexcept
        : x(float((color >> COLOR_SHIFT_16) & UINT8_MAX) / 255.0f),
          y(float((color >> COLOR_SHIFT_8) & UINT8_MAX) / 255.0f),
          z(float((color >> COLOR_SHIFT_0) & UINT8_MAX) / 255.0f),
          w(float((color >> COLOR_SHIFT_24) & UINT8_MAX) / 255.0f)
    {}

    constexpr explicit Color(const BASE_NS::Math::Vec4& other) noexcept : x(other.x), y(other.y), z(other.z), w(other.w)
    {}

    constexpr void operator=(const BASE_NS::Math::Vec4& other) noexcept
    {
        x = other.x;
        y = other.y;
        z = other.z;
        w = other.w;
    }

    constexpr Color operator+(const Color& other) const noexcept
    {
        Color result;
        result.x = x + other.x;
        result.y = y + other.y;
        result.z = z + other.z;
        result.w = w + other.w;

        return result;
    }

    constexpr Color operator-(const Color& other) const noexcept
    {
        Color result;
        result.x = x - other.x;
        result.y = y - other.y;
        result.z = z - other.z;
        result.w = w - other.w;

        return result;
    }

    constexpr Color operator*(const Color& other) const noexcept
    {
        Color result;
        result.x = x * other.x;
        result.y = y * other.y;
        result.z = z * other.z;
        result.w = w * other.w;

        return result;
    }

    constexpr Color operator/(const Color& other) const noexcept
    {
        Color result;
        result.x = other.x != 0.f ? x / other.x : 0.f;
        result.y = other.y != 0.f ? y / other.y : 0.f;
        result.z = other.z != 0.f ? z / other.z : 0.f;
        result.w = other.w != 0.f ? w / other.w : 0.f;

        return result;
    }

    constexpr Color operator*(const float scalar) const noexcept
    {
        Color result;
        result.x = x * scalar;
        result.y = y * scalar;
        result.z = z * scalar;
        result.w = w * scalar;

        return result;
    }

    constexpr Color operator/(const float scalar) const noexcept
    {
        if (scalar != 0.f) {
            Color result;
            result.x = x / scalar;
            result.y = y / scalar;
            result.z = z / scalar;
            result.w = w / scalar;

            return result;
        }

        return *this;
    }

    constexpr Color& operator*=(const float scalar) noexcept
    {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        w *= scalar;

        return *this;
    }

    constexpr Color& operator/=(const float scalar) noexcept
    {
        if (scalar != 0.f) {
            x /= scalar;
            y /= scalar;
            z /= scalar;
            w /= scalar;
        }

        return *this;
    }

    constexpr bool operator==(const Color& other) const noexcept
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }

    constexpr bool operator!=(const Color& other) const noexcept
    {
        return x != other.x || y != other.y || z != other.z || w != other.w;
    }

    constexpr float& operator[](const uint32_t index) noexcept
    {
        switch (index) {
            case 0: // 0: index
                return x;
            case 1: // 1: index
                return y;
            case 2: // 2: index
                return z;
            case 3: // 3: index
                return w;
            default: // do nothing
                break;
        }

        return x; // dummy
    }

    constexpr const float& operator[](const uint32_t index) const noexcept
    {
        switch (index) {
            case 0: // 0: index
                return x;
            case 1: // 1: index
                return y;
            case 2: // 2: index
                return z;
            case 3: // 3: index
                return w;
            default: // do nothing
                break;
        }

        return x; // dummy
    }

    /** Returns Color's members as Math::Vec4
     */
    constexpr operator Math::Vec4() const noexcept
    {
        return Math::Vec4(x, y, z, w);
    }

    constexpr Color GetLerp(const Color& other, float weight) const noexcept
    {
        Color result = *this;
        result.x = Math::lerp(result.x, other.x, weight);
        result.y = Math::lerp(result.y, other.y, weight);
        result.z = Math::lerp(result.z, other.z, weight);
        result.w = Math::lerp(result.w, other.w, weight);

        return result;
    }

    inline uint32_t GetRgba32() const noexcept
    {
        uint32_t c = (uint8_t)Math::round(x * 255.0f);
        c <<= COLOR_SHIFT_8;
        c |= (uint8_t)Math::round(y * 255.0f);
        c <<= COLOR_SHIFT_8;
        c |= (uint8_t)Math::round(z * 255.0f);
        c <<= COLOR_SHIFT_8;
        c |= (uint8_t)Math::round(w * 255.0f);

        return c;
    }

    inline uint64_t GetRgba64() const noexcept
    {
        uint64_t c = (uint16_t)Math::round(x * 65535.0f);
        c <<= COLOR_SHIFT_16;
        c |= (uint16_t)Math::round(y * 65535.0f);
        c <<= COLOR_SHIFT_16;
        c |= (uint16_t)Math::round(z * 65535.0f);
        c <<= COLOR_SHIFT_16;
        c |= (uint16_t)Math::round(w * 65535.0f);

        return c;
    }

    inline uint32_t GetArgb32() const noexcept
    {
        uint32_t c = (uint8_t)Math::round(w * 255.0f);
        c <<= COLOR_SHIFT_8;
        c |= (uint8_t)Math::round(x * 255.0f);
        c <<= COLOR_SHIFT_8;
        c |= (uint8_t)Math::round(y * 255.0f);
        c <<= COLOR_SHIFT_8;
        c |= (uint8_t)Math::round(z * 255.0f);

        return c;
    }

    inline uint64_t GetArgb64() const noexcept
    {
        uint64_t c = (uint16_t)Math::round(w * 65535.0f);
        c <<= COLOR_SHIFT_16;
        c |= (uint16_t)Math::round(x * 65535.0f);
        c <<= COLOR_SHIFT_16;
        c |= (uint16_t)Math::round(y * 65535.0f);
        c <<= COLOR_SHIFT_16;
        c |= (uint16_t)Math::round(z * 65535.0f);

        return c;
    }

    constexpr float GetLuminanceLinear() const noexcept
    {
        return x * 0.299f + y * 0.587f + z * 0.114f;
    }

    constexpr float GetLuminanceSrgb() const noexcept
    {
        return x * 0.2126f + y * 0.7152f + z * 0.0722f;
    }

    inline Color GetGamma() const
    {
        Color result = *this;
        result.x = Math::pow(result.x, 1.f / 2.2f);
        result.y = Math::pow(result.y, 1.f / 2.2f);
        result.z = Math::pow(result.z, 1.f / 2.2f);

        return result;
    }

    constexpr Color GetGrayscaleSrgb() const noexcept
    {
        float luminance = GetLuminanceSrgb();
        return Color(luminance, luminance, luminance, w);
    }

    constexpr Color GetGrayscaleLinear() const noexcept
    {
        float luminance = GetLuminanceLinear();
        return Color(luminance, luminance, luminance, w);
    }
};

/** Color conversion types
 */
enum ColorConversionType {
    /** No conversion and should be used as default */
    COLOR_CONVERSION_TYPE_NONE = 0,
    /** sRGB to linear conversion */
    COLOR_CONVERSION_TYPE_SRGB_TO_LINEAR = 1,
    /** Linear to sRGB conversion */
    COLOR_CONVERSION_TYPE_LINEAR_TO_SRGB = 2,
};

/** Color space flag bits
 * Defaults are linear
 */
enum ColorSpaceFlagBits {
    /** Add hint to treat typical sRGB data as linear with input data and with final output (swapchain)
     * This value should be retrieved from the engine and use with all (or some of) the plugins
     * NOTE: mainly affects the input formats where data is written into (and typically results as non-srgb formats),
     * and swapchain formats.
     */
    COLOR_SPACE_SRGB_AS_LINEAR_BIT = 0x00000001,
};
/** Container for color space flag bits */
using ColorSpaceFlags = uint32_t;

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
    return Color(color);
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
    uint32_t g = ((uint32_t)color.y & UINT8_MAX) << COLOR_SHIFT_8;
    uint32_t b = ((uint32_t)color.z & UINT8_MAX) << COLOR_SHIFT_16;
    uint32_t a = ((uint32_t)color.w & UINT8_MAX) << COLOR_SHIFT_24;

    return a | b | g | r;
}

/** Input color in linear (Vec4)
 * Output color in sRGB (uint32_t)
 */
inline uint32_t FromColorRGBAToSRGB(Color color)
{
    uint32_t r = ((uint32_t)(LinearToSRGBConv(color.x) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_0;
    uint32_t g = ((uint32_t)(LinearToSRGBConv(color.y) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_8;
    uint32_t b = ((uint32_t)(LinearToSRGBConv(color.z) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_16;
    uint32_t a = ((uint32_t)(color.w * 255.0f) & UINT8_MAX) << COLOR_SHIFT_24;

    return a | b | g | r;
}

inline uint32_t FromColorRGBA(const Color color, const ColorSpaceFlags flags)
{
    if (flags == 0U) {
        return FromColorRGBAToLinear(color);
    }
    return FromColorRGBAToSRGB(color);
}

inline constexpr uint32_t PackPremultiplyColorUnorm(Color color)
{
    // premultiply colors by alpha and scale the components from 0..1 to 0..255 and pack them in a 32 bit value RGBA
    // where R is the lowest byte and A the highest.
    return (((uint32_t(BASE_NS::Math::clamp01(color.w) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_24) |
            ((uint32_t(BASE_NS::Math::clamp01(color.w * color.z) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_16) |
            ((uint32_t(BASE_NS::Math::clamp01(color.w * color.y) * 255.0f) & UINT8_MAX) << COLOR_SHIFT_8) |
            ((uint32_t(BASE_NS::Math::clamp01(color.w * color.x) * 255.0f) & UINT8_MAX)));
}

static const Color BLACK_COLOR = MakeColorFromLinear(0xFF000000);
static const Color WHITE_COLOR = MakeColorFromLinear(0xFFFFFFFF);
BASE_END_NAMESPACE()

#endif // API_BASE_UTIL_COLOR_H
