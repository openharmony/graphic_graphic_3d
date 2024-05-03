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

#ifndef API_BASE_MATH_FLOAT_PACKER_H
#define API_BASE_MATH_FLOAT_PACKER_H

#include <cstdint>

#include <base/math/mathf.h>
#include <base/math/vector.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
constexpr const uint32_t F32_EXPONENT_BITS = 0xFF;
constexpr const uint32_t F32_EXPONENT_SHIFT = 23;
constexpr const uint32_t F32_SIGN_BIT = 31;
constexpr const uint32_t F32_INFINITY = (F32_EXPONENT_BITS << F32_EXPONENT_SHIFT);

constexpr const uint32_t F16_EXPONENT_BITS = 0x1F;
constexpr const uint32_t F16_EXPONENT_SHIFT = 10;
constexpr const uint32_t F16_SIGN_BIT = 15;
constexpr const uint32_t F16_SIGN_SHIFT = (F32_SIGN_BIT - F16_SIGN_BIT);
constexpr const uint32_t F16_MANTISSA_SHIFT = (F32_EXPONENT_SHIFT - F16_EXPONENT_SHIFT);
constexpr const uint32_t F16_INFINITY = (F16_EXPONENT_BITS << F16_EXPONENT_SHIFT);

/** \addtogroup group_math_floatpacker
 *  @{
 */
/** Converts 32 bit floating point number to 16 bit float value
 */
inline uint16_t F32ToF16(float val)
{
    union {
        float f;
        uint32_t ui;
    } f32 { val };

    uint32_t noSign = f32.ui & 0x7fffffff;     // Non-sign bits
    uint32_t sign = f32.ui & 0x80000000;       // Sign bit
    uint32_t exponent = f32.ui & F32_INFINITY; // Exponent

    noSign >>= F16_MANTISSA_SHIFT; // Align mantissa on MSB
    sign >>= F16_SIGN_SHIFT;       // Shift sign bit into position

    // 16bit bias = 15, 32bit bias = 127
    // (-127 + 15) << 10 = (-112) << 10 = -0x1c000
    noSign -= 0x1c000; // Adjust bias

    // 16bit min exponent = -14, 32bit bias 127
    // (-14 + 127) << 23 = 0x38800000
    noSign = (exponent < 0x38800000) ? 0 : noSign; // Flush-to-zero
    // 16bit max exponent = 15, 32bit bias 127
    // (15 + 127) << 23 = 0x47000000
    noSign = (exponent > 0x47000000) ? F16_INFINITY : noSign; // Clamp-to-inf

    // Re-insert sign bit
    return static_cast<uint16_t>(noSign | sign);
}

/** Converts 16 bit floating point number to 32 bit float
 */
inline float F16ToF32(uint16_t val)
{
    union {
        float f = 0.f;
        uint32_t ui;
    } f32;

    uint32_t noSign = val & 0x7fffU;        // Non-sign bits
    uint32_t sign = val & 0x8000U;          // Sign bit
    uint32_t exponent = val & F16_INFINITY; // Exponent

    noSign <<= F16_MANTISSA_SHIFT; // Align mantissa on MSB
    sign <<= F16_SIGN_SHIFT;       // Shift sign bit into position

    // 16bit bias = 15, 32bit bias = 127
    // (-15 + 127) << 23 = 0x38000000
    noSign += 0x38000000; // Adjust bias

    noSign = (exponent == 0 ? 0 : noSign);                       // Denormals-as-zero
    noSign = (exponent == F16_INFINITY ? F32_INFINITY : noSign); // Clamp-to-inf

    f32.ui = noSign | sign; // Re-insert sign bit

    return f32.f;
}

/** Pack single vector2(32bit x 2) to 32 bit integer (unsigned packed values) */
inline uint32_t PackUnorm2X16(const Vec2& v)
{
    union {
        uint16_t in[2];
        uint32_t out;
    } u;

    u.in[0] = uint16_t(round(clamp(v[0], 0.f, +1.f) * 65535.0f));
    u.in[1] = uint16_t(round(clamp(v[1], 0.f, +1.f) * 65535.0f));

    return u.out;
}

/** Unpack 32 bit integer to default lume vector2
 */
inline Vec2 UnpackUnorm2X16(uint32_t p)
{
    const union {
        uint32_t in;
        uint16_t out[2];
    } u { p };

    return Vec2(u.out[0] * 1.5259021896696421759365224689097e-5f, u.out[1] * 1.5259021896696421759365224689097e-5f);
}

/** Pack single vector2(32bit x 2) to 32 bit integer (signed packed values)
 */
inline uint32_t PackSnorm2X16(const Vec2& v)
{
    union {
        int16_t in[2];
        uint32_t out;
    } u;

    u.in[0] = (int16_t)(round(clamp(v.x, -1.0f, +1.0f) * 32767.0f));
    u.in[1] = (int16_t)(round(clamp(v.y, -1.0f, +1.0f) * 32767.0f));

    return u.out;
}

/** Unpack 32 bit integer to default lume vector2
 */
inline Vec2 UnpackSnorm2X16(uint32_t p)
{
    const union {
        uint32_t in;
        int16_t out[2];
    } u { p };

    return Vec2(clamp(u.out[0] * 3.0518509475997192297128208258309e-5f, -1.0f, 1.0f),
        clamp(u.out[1] * 3.0518509475997192297128208258309e-5f, -1.0f, 1.0f));
}

/** Pack vector2 to 32 bit integer with half precision
 */
inline uint32_t PackHalf2X16(const Vec2& v)
{
    const union {
        uint16_t in[2];
        uint32_t out;
    } u { { F32ToF16(v.x), F32ToF16(v.y) } };

    return u.out;
}

/** Unpack 32 bit integer to normal lume vector2 and rise precision from 16 bit to 32 bits
 */
inline Vec2 UnpackHalf2X16(uint32_t v)
{
    const union {
        uint32_t in;
        uint16_t out[2];
    } u { v };

    return Vec2(F16ToF32(u.out[0]), F16ToF32(u.out[1]));
}
/** @} */
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_FLOAT_PACKER_H
