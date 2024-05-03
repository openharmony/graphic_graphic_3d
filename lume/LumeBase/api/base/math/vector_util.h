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

#ifndef API_BASE_MATH_VECTOR_UTIL_H
#define API_BASE_MATH_VECTOR_UTIL_H

#include <base/math/mathf.h>
#include <base/math/vector.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
/** \addtogroup group_math_vectorutil
 *  @{
 */
// Vector2
/** Dot product of two vector2's */
static inline constexpr float Dot(const Vec2& lhs, const Vec2& rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

/** Cross product of two vector2's */
static inline constexpr float Cross(const Vec2& lhs, const Vec2& rhs)
{
    return lhs.x * rhs.y - lhs.y * rhs.x;
}

/** Linearly interpolate between two vector2's */
static inline constexpr Vec2 Lerp(Vec2 v1, Vec2 v2, float t)
{
    t = Math::clamp01(t);
    return Vec2(v1.x + (v2.x - v1.x) * t, v1.y + (v2.y - v1.y) * t);
}

/** Return squared magnitude of two vector2's */
static inline constexpr float SqrMagnitude(const Vec2& vec)
{
    return vec.x * vec.x + vec.y * vec.y;
}

/** Return magnitude of two vector2's */
static inline float Magnitude(const Vec2& vec)
{
    return Math::sqrt(vec.x * vec.x + vec.y * vec.y);
}

/** Return distance between vector2's */
static inline float distance(const Vec2& v0, const Vec2& v1)
{
    return Magnitude(v1 - v0);
}

/** Return two component min of vector2's */
static constexpr inline Vec2 min(const Vec2& lhs, const Vec2& rhs)
{
    return Vec2(min(lhs.x, rhs.x), min(lhs.y, rhs.y));
}

/** Return two component max of vector2's */
static constexpr inline Vec2 max(const Vec2& lhs, const Vec2& rhs)
{
    return Vec2(max(lhs.x, rhs.x), max(lhs.y, rhs.y));
}

/** Return normalized vector2 (if magnitude is not larger than epsilon, returns zero vector) */
static inline Vec2 Normalize(const Vec2& value)
{
    const float mag = Magnitude(value);
    if (mag > Math::EPSILON)
        return value / mag;
    else
        return Vec2(0.0f, 0.0f);
}

/** Return vector2 perpendicular to input in clock-wise direction */
static inline Vec2 PerpendicularCW(const Vec2& value)
{
    return Vec2(-value.y, value.x);
}

/** Return vector2 perpendicular to input in counter clock-wise direction */
static inline Vec2 PerpendicularCCW(const Vec2& value)
{
    return Vec2(value.y, -value.x);
}

/** Return vector2 rotated clock wise by angle radians (Y down)*/
static inline Vec2 RotateCW(const Vec2& value, float angle)
{
    const float s = Math::sin(angle);
    const float c = Math::cos(angle);
    return Vec2(value.x * c - value.y * s, value.y * c + value.x * s);
}

/** Return vector2 rotated counter clock wise by angle radians (Y down) */
static inline Vec2 RotateCCW(const Vec2& value, float angle)
{
    const float s = Math::sin(angle);
    const float c = Math::cos(angle);
    return Vec2(value.x * c + value.y * s, value.y * c - value.x * s);
}

/** Return intersection of two Vec2 start points and direction vectors. Returns boolean flag indicating if the vectors
 * intersect. */
static inline Vec2 Intersect(
    const Vec2& aStart, const Vec2& aDir, const Vec2& bStart, const Vec2& bDir, bool infinite, bool& intersected)
{
    const auto denominator = Cross(aDir, bDir);

    if (Math::abs(denominator) < BASE_EPSILON) {
        // The lines are parallel
        intersected = false;
        return {};
    }

    // Solve the intersection positions
    const auto originDist = bStart - aStart;
    const auto uNumerator = Cross(originDist, aDir);
    const auto u = uNumerator / denominator;
    const auto t = Cross(originDist, bDir) / denominator;

    if (!infinite && (t < 0 || t > 1 || u < 0 || u > 1)) {
        // The intersection lies outside of the line segments
        intersected = false;
        return {};
    }

    intersected = true;

    // Calculate the intersection point
    return aStart + aDir * t;
}

// Vector3
/** Dot product of two vector3's */
static inline constexpr float Dot(const Vec3& lhs, const Vec3& rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

/** Cross product of two vector3's */
static inline constexpr Vec3 Cross(const Vec3& lhs, const Vec3& rhs)
{
    return Vec3(lhs.y * rhs.z - lhs.z * rhs.y, lhs.z * rhs.x - lhs.x * rhs.z, lhs.x * rhs.y - lhs.y * rhs.x);
}

/** Linearly interpolate between v1 and v2 vector3's by value t */
static inline constexpr Vec3 Lerp(const Vec3& v1, const Vec3& v2, float t)
{
    t = Math::clamp01(t);
    return Vec3(v1.x + (v2.x - v1.x) * t, v1.y + (v2.y - v1.y) * t, v1.z + (v2.z - v1.z) * t);
}

/** Return squared magnitude of vector3's */
static inline constexpr float SqrMagnitude(const Vec3& vec)
{
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

/** Return magnitude of vector3's */
static inline float Magnitude(const Vec3& vec)
{
    return Math::sqrt(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

/** Return squared distance of two vector3's */
static inline float Distance2(const Vec3& v0, const Vec3& v1)
{
    return SqrMagnitude(v1 - v0);
}

/** Return normalized vector3 (if magnitude is not larger than epsilon, returns zero vector) */
static inline Vec3 Normalize(const Vec3& value)
{
    const float mag = Magnitude(value);
    if (mag > Math::EPSILON) {
        return value / mag;
    } else {
        return Vec3(0.0f, 0.0f, 0.0f);
    }
}

/** Return three component min of vector3's */
static constexpr inline Vec3 min(const Vec3& lhs, const Vec3& rhs)
{
    return Vec3(min(lhs.x, rhs.x), min(lhs.y, rhs.y), min(lhs.z, rhs.z));
}

/** Return three component max of vector3's */
static constexpr inline Vec3 max(const Vec3& lhs, const Vec3& rhs)
{
    return Vec3(max(lhs.x, rhs.x), max(lhs.y, rhs.y), max(lhs.z, rhs.z));
}

/** Return scaled value of vector3 */
static inline Vec3 Scale(Vec3 const& v, float desiredLength)
{
    return v * desiredLength / Magnitude(v);
}

/** Combine vector3's which have been multiplied with scalar (a * ascl) + (b * bscl) */
static inline constexpr Vec3 Combine(Vec3 const& a, Vec3 const& b, float ascl, float bscl)
{
    return (a * ascl) + (b * bscl);
}

// Vector4
/** Return squared magnitude of vector4 */
static inline constexpr float SqrMagnitude(const Vec4& vec)
{
    return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
}

/** Linearly interpolate between v1 and v2 vector4's by value t */
static inline constexpr Vec4 Lerp(const Vec4& v1, const Vec4& v2, float t)
{
    t = Math::clamp01(t);
    return Vec4(v1.x + (v2.x - v1.x) * t, v1.y + (v2.y - v1.y) * t, v1.z + (v2.z - v1.z) * t, v1.w + (v2.w - v1.w) * t);
}

/** @} */
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_VECTOR_UTIL_H
