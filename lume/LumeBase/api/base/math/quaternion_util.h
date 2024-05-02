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

#ifndef API_BASE_MATH_QUATERNION_UTIL_H
#define API_BASE_MATH_QUATERNION_UTIL_H

#include <cmath>

#include <base/math/mathf.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
/** \addtogroup group_math_quaternionutil
 *  @{
 */
/** Create quaternion from vector3 (takes radians) */
static inline Quat FromEulerRad(const Vec3& euler)
{
    const float roll = euler.x;
    const float pitch = euler.y;
    const float yaw = euler.z;
    const float cy = Math::cos(yaw * 0.5f);
    const float sy = Math::sin(yaw * 0.5f);
    const float cp = Math::cos(pitch * 0.5f);
    const float sp = Math::sin(pitch * 0.5f);
    const float cr = Math::cos(roll * 0.5f);
    const float sr = Math::sin(roll * 0.5f);

    return Quat(cy * cp * sr - sy * sp * cr, sy * cp * sr + cy * sp * cr, sy * cp * cr - cy * sp * sr,
        cy * cp * cr + sy * sp * sr);
}

/** Get squared length */
static inline constexpr float LengthSquared(const Quat& quat)
{
    return quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w;
}

/** Get length */
static inline float Length(const Quat& quat)
{
    return Math::sqrt(quat.x * quat.x + quat.y * quat.y + quat.z * quat.z + quat.w * quat.w);
}

/** Get dot product of two quaternions */
static inline constexpr float Dot(const Quat& q1, const Quat& q2)
{
    return q1.x * q2.x + q1.y * q2.y + q1.z * q2.z + q1.w * q2.w;
}

/** Creates a rotation which rotates angle degrees around axis, takes in angle as radians */
static inline Quat AngleAxis(const float& angle, const Vec3& axis)
{
    const float sinHalfAngle = Math::sin(angle * 0.5f);
    const Vec3 axisMultiplied = axis * sinHalfAngle;
    return Quat(axisMultiplied.x, axisMultiplied.y, axisMultiplied.z, Math::cos(angle * 0.5f));
}

/** Inverse */
static inline constexpr Quat Inverse(const Quat& rotation)
{
    const float lengthSq = LengthSquared(rotation);
    if (lengthSq != 0.0f) {
        const float inverse = 1.0f / lengthSq;
        return Quat(rotation.x * -inverse, rotation.y * -inverse, rotation.z * -inverse, rotation.w * inverse);
    }
    return rotation;
}

/** Creates quaternion from three separate euler angles (degrees) */
static inline Quat Euler(const float& x, const float& y, const float& z)
{
    return FromEulerRad(Vec3(x, y, z) * Math::DEG2RAD);
}

/** Creates quaternion from euler angles (degrees) */
static inline Quat Euler(const Vec3& euler)
{
    return FromEulerRad(euler * Math::DEG2RAD);
}

/** Look rotation from forward and up vectors */
static inline Quat LookRotation(Vec3 forward, Vec3 up)
{
    forward = Math::Normalize(forward);
    const Vec3 right = Math::Normalize(Math::Cross(up, forward));
    up = Math::Cross(forward, right);
    const float m00 = right.x;
    const float m01 = right.y;
    const float m02 = right.z;
    const float m10 = up.x;
    const float m11 = up.y;
    const float m12 = up.z;
    const float m20 = forward.x;
    const float m21 = forward.y;
    const float m22 = forward.z;

    // discard values less than epsilon
    const auto discardEpsilon = [](const float value) { return value < Math::EPSILON ? 0.f : value; };

    // based on https://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/
    // Because quaternions cannot represent a reflection component, the matrix must be special orthogonal. For a special
    // orthogonal matrix, 1 + trace is always positive. The case switch is not needed, just do sqrt() on the 4 trace
    // variants and you are done:
    const float w = Math::sqrt(discardEpsilon(1.f + m00 + m11 + m22)) * 0.5f;
    float x = Math::sqrt(discardEpsilon(1.f + m00 - m11 - m22)) * 0.5f;
    float y = Math::sqrt(discardEpsilon(1.f - m00 + m11 - m22)) * 0.5f;
    float z = Math::sqrt(discardEpsilon(1.f - m00 - m11 + m22)) * 0.5f;

    // Recover signs
    x = std::copysign(x, m12 - m21);
    y = std::copysign(y, m20 - m02);
    z = std::copysign(z, m01 - m10);

    return { x, y, z, w };
}

/** Normalize single (degree) angle */
static inline constexpr float NormalizeAngle(float angle)
{
    while (angle > 360.0f) {
        angle -= 360.0f;
    }
    while (angle < 0.0f) {
        angle += 360.0f;
    }
    return angle;
}

/** Normalize vector3 angles (degree) */
static inline constexpr Vec3 NormalizeAngles(Vec3 angles)
{
    angles.x = NormalizeAngle(angles.x);
    angles.y = NormalizeAngle(angles.y);
    angles.z = NormalizeAngle(angles.z);
    return angles;
}

/** Convert quaternion to euler values (radians) */
static inline Vec3 ToEulerRad(const Quat& q)
{
    // roll (x-axis rotation)
    const float sinrCosp = +2.0f * (q.w * q.x + q.y * q.z);
    const float cosrCosp = +1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    const float roll = atan2(sinrCosp, cosrCosp);

    // pitch (y-axis rotation)
    const float sinp = +2.0f * (q.w * q.y - q.z * q.x);
    // use 90 degrees if out of range
    const float pitch = (fabs(sinp) >= 1.f) ? copysign(Math::PI / 2.0f, sinp) : asin(sinp);

    // yaw (z-axis rotation)
    const float sinyCosp = +2.0f * (q.w * q.z + q.x * q.y);
    const float cosyCosp = +1.0f - 2.0f * (q.y * q.y + q.z * q.z);
    const float yaw = atan2(sinyCosp, cosyCosp);

    Vec3 eulerRadRotations(roll, pitch, yaw);
    return eulerRadRotations;
}

/** Conjugates quaternion */
static inline Quat Conjugate(const Quat& q)
{
    return Quat(-q.x, -q.y, -q.z, q.w);
}

/** Normalizes quaternion */
static inline Quat Normalize(const Quat& q)
{
    const float len = Length(q);
    if (len <= 0.0f) {
        return Quat(0.0f, 0.0f, 0.0f, 1.0f);
    }
    const float oneOverLen = 1.0f / len;
    return Quat(q.x * oneOverLen, q.y * oneOverLen, q.z * oneOverLen, q.w * oneOverLen);
}

/** Multiply vector3 by quaternion(rotation) */
inline Vec3 operator*(const Quat& q, const Vec3& v)
{
    const Vec3 QuatVector(q.x, q.y, q.z);
    const Vec3 uv(Cross(QuatVector, v));
    const Vec3 uuv(Cross(QuatVector, uv));

    return v + ((uv * q.w) + uuv) * 2.0f;
}

/** Multiply vector3 by quaternion(rotation) */
inline Vec3 operator*(const Vec3& v, const Quat& q)
{
    return Inverse(q) * v;
}

/** Multiply quaternion with scalar */
inline constexpr Quat operator*(const float scalar, const Quat& quat)
{
    return Quat(quat.x * scalar, quat.y * scalar, quat.z * scalar, quat.w * scalar);
}

/** Multiply quaternion with scalar */
inline constexpr Quat operator*(const Quat& quat, const float scalar)
{
    return Quat(quat.x * scalar, quat.y * scalar, quat.z * scalar, quat.w * scalar);
}

/** Adds two quaternions */
inline constexpr Quat operator+(const Quat& q, const Quat& p)
{
    return Quat(q.x + p.x, q.y + p.y, q.z + p.z, q.w + p.w);
}

/** Spherically interpolate between x and y by a */
inline constexpr Quat Slerp(Quat const& x, Quat const& y, float a)
{
    Quat z = y;
    float cosTheta = Dot(x, z);
    if (cosTheta < 0.0f) {
        z.x = -z.x;
        z.y = -z.y;
        z.z = -z.z;
        z.w = -z.w;
        cosTheta = -cosTheta;
    }

    if (cosTheta > 1.0f - EPSILON) {
        return Quat(lerp(x.x, z.x, a), lerp(x.y, z.y, a), lerp(x.z, z.z, a), lerp(x.w, z.w, a));
    }

    // A Fast and Accurate Estimate for SLERP
    // sin(a * angle) / sin(angle) and sin((1-a) * angle) / sin(angle) are approximated with Chebyshev Polynomials
    constexpr float mu = 1.85298109240830f;
    constexpr float u[8] = // 1 / [i(2i + 1)] for i >= 1
        { 1.f / (1 * 3), 1.f / (2 * 5), 1.f / (3 * 7), 1.f / (4 * 9), 1.f / (5 * 11), 1.f / (6 * 13), 1.f / (7 * 15),
            mu / (8 * 17) };
    constexpr float v[8] = // i / (2i + 1) for i >= 1
        { 1.f / 3, 2.f / 5, 3.f / 7, 4.f / 9, 5.f / 11, 6.f / 13, 7.f / 15, mu * 8.f / 17 };

    const float xm1 = cosTheta - 1; // in [-1, 0]
    const float d = 1 - a;
    const float sqrA = a * a;
    const float sqrD = d * d;
    // bA[] stores a-related values, bD[] stores (1 - a)-related values
    float bA[8] {};
    float bD[8] {};
    for (int i = 7; i >= 0; --i) {
        bA[i] = (u[i] * sqrA - v[i]) * xm1;
        bD[i] = (u[i] * sqrD - v[i]) * xm1;
    }
    const float coeff1 =
        a *
        (1 + bA[0] * (1 + bA[1] * (1 + bA[2] * (1 + bA[3] * (1 + bA[4] * (1 + bA[5] * (1 + bA[6] * (1 + bA[7]))))))));
    const float coeff2 =
        d *
        (1 + bD[0] * (1 + bD[1] * (1 + bD[2] * (1 + bD[3] * (1 + bD[4] * (1 + bD[5] * (1 + bD[6] * (1 + bD[7]))))))));
    return (coeff2 * x) + (coeff1 * z);
}
/** @} */
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_QUATERNION_UTIL_H
