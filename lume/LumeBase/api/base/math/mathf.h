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

#ifndef API_BASE_MATH_MATHF_H
#define API_BASE_MATH_MATHF_H

#include <cmath>

#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
constexpr const float BASE_EPSILON = 1.192092896e-07f; // smallest such that 1.0+FLT_EPSILON != 1.0

/** \addtogroup group_math_mathf
 *  @{
 */
/** Returns Minimum value of 2 given parameter */
template<typename T>
static constexpr inline const T& min(const T& a, const T& b) noexcept
{
    return a < b ? a : b;
}

/** Returns Maximum value of 2 given parameter */
template<typename T>
static constexpr inline const T& max(const T& a, const T& b) noexcept
{
    return a > b ? a : b;
}

/** Clamps value between minimum and maximum value */
static constexpr inline float clamp(float value, float min, float max)
{
    if (value < min) {
        value = min;
    } else if (value > max) {
        value = max;
    }
    return value;
}

/** Clamps value between 0 and 1 and then returns it */
static constexpr inline float clamp01(float value)
{
    if (value < 0) {
        return 0;
    } else if (value > 1) {
        return 1;
    } else {
        return value;
    }
}

/** Returns Absolute value of given float */
static constexpr inline float abs(float value)
{
    return value < 0 ? -value : value;
}

/** Returns Absolute value of given integer */
static constexpr inline int abs(int value)
{
    return value < 0 ? -value : value;
}

/** Floor the given float value */
static float floor(float value)
{
    return floorf(value);
}

/** Ceil the given float value */
static float ceil(float value)
{
    return ceilf(value);
}

/** Returns largest integer smaller to or equal to value */
static int floorToInt(float value)
{
    return static_cast<int>(floor(value));
}

/** Returns smallest integer greater to or equal to value */
static int ceilToInt(float value)
{
    return static_cast<int>(ceil(value));
}

/** Interpolates between floats a and b by t and t is clamped value between 0 and 1 */
static constexpr inline float lerp(float a, float b, float t)
{
    return a + (b - a) * clamp01(t);
}

/** Returns value rounded to the nearest integer */
static float round(float value)
{
    return roundf(value);
}

/** Returns square root of value */
static float sqrt(float value)
{
    return sqrtf(value);
}

/** Returns the sine of angle value in radians */
static float sin(float value)
{
    return sinf(value);
}

/** Returns the cosine of angle value in radians */
static float cos(float value)
{
    return cosf(value);
}

/** Returns the tangent of angle value in radians */
static float tan(float value)
{
    return tanf(value);
}

/** Returns f raised to power p */
static float pow(float f, float p)
{
    return powf(f, p);
}

/** atan wrapper for float */
static float atan(float f)
{
    return atanf(f);
}

/** atan2 wrapper for float */
static float atan2(float a, float b)
{
    return atan2f(a, b);
}

/** asin wrapper for float */
static float asin(float value)
{
    return asinf(value);
}

/** acos wrapper for float */
static float acos(float value)
{
    return acosf(value);
}

/** Epsilon (1.192092896e-07F) */
static constexpr float EPSILON = BASE_EPSILON;

/** Positive infinity */
static float infinity = INFINITY;

/** 3.14... */
static constexpr float PI = 3.141592653f;

/** Degrees-to-radians conversion constant */
static constexpr float DEG2RAD = PI * 2.0f / 360.0f;

/** Radians-to-degrees conversion constant */
static constexpr float RAD2DEG = 1.0f / DEG2RAD;

/** Returns the sign of value */
static constexpr inline float sign(float value)
{
    return value >= 0.0f ? 1.0f : -1.0f;
}
/** @} */
}; // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_MATHF_H
