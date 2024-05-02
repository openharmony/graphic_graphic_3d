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

#ifndef API_BASE_MATH_MATRIX_UTIL_H
#define API_BASE_MATH_MATRIX_UTIL_H

#include <base/math/mathf.h>
#include <base/math/matrix.h>
#include <base/math/quaternion.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
/** \addtogroup group_math_matrixutil
 *  @{
 */
/** Returns transpose of this matrix */
static inline constexpr Mat3X3 Transpose(const Mat3X3& m)
{
    Mat3X3 result;

    result.x.x = m.x.x;
    result.y.x = m.x.y;
    result.z.x = m.x.z;

    result.x.y = m.y.x;
    result.y.y = m.y.y;
    result.z.y = m.y.z;

    result.x.z = m.z.x;
    result.y.z = m.z.y;
    result.z.z = m.z.z;

    return result;
}

/** Returns transpose of this matrix */
static inline constexpr Mat4X4 Transpose(const Mat4X4& m)
{
    Mat4X4 result;

    result.x.x = m.x.x;
    result.y.x = m.x.y;
    result.z.x = m.x.z;
    result.w.x = m.x.w;

    result.x.y = m.y.x;
    result.y.y = m.y.y;
    result.z.y = m.y.z;
    result.w.y = m.y.w;

    result.x.z = m.z.x;
    result.y.z = m.z.y;
    result.z.z = m.z.z;
    result.w.z = m.z.w;

    result.x.w = m.w.x;
    result.y.w = m.w.y;
    result.z.w = m.w.z;
    result.w.w = m.w.w;

    return result;
}

/** Set scale of this matrix */
static inline constexpr Mat3X3 PostScale(const Mat3X3& mat, const Vec2& vec)
{
    Mat3X3 result;
    result.x = { mat.x.x * vec.x, mat.x.y * vec.y, mat.x.z };
    result.y = { mat.y.x * vec.x, mat.y.y * vec.y, mat.y.z };
    result.z = { mat.z.x * vec.x, mat.z.y * vec.y, mat.z.z };
    return result;
}

/** Set scale of this matrix */
static inline constexpr Mat4X4 PostScale(const Mat4X4& mat, const Vec3& vec)
{
    Mat4X4 result;
    result.x = { mat.x.x * vec.x, mat.x.y * vec.y, mat.x.z * vec.z, mat.x.w };
    result.y = { mat.y.x * vec.x, mat.y.y * vec.y, mat.y.z * vec.z, mat.y.w };
    result.z = { mat.z.x * vec.x, mat.z.y * vec.y, mat.z.z * vec.z, mat.z.w };
    result.w = { mat.w.x * vec.x, mat.w.y * vec.y, mat.w.z * vec.z, mat.w.w };
    return result;
}

/** Set scale of this matrix */
static inline constexpr Mat3X3 Scale(const Mat3X3& mat, const Vec2& vec)
{
    Mat3X3 result;
    result.x = mat.x * vec.x;
    result.y = mat.y * vec.y;
    result.z = mat.z;
    return result;
}

/** Set scale of this matrix */
static inline constexpr Mat4X4 Scale(const Mat4X4& mat, const Vec3& vec)
{
    Mat4X4 result;
    result.x = mat.x * vec.x;
    result.y = mat.y * vec.y;
    result.z = mat.z * vec.z;
    result.w = mat.w;
    return result;
}

/** Get column of the matrix */
static inline constexpr Vec3 GetColumn(const Mat3X3& mat, int index)
{
    switch (index) {
            // the first column of the matrix: Vec3(m00, m10, m20)
        case 0:
            return mat.x;
            // the second column of the matrix:  Vec3(m01, m11, m21)
        case 1:
            return mat.y;
            // the third column of the matrix:  Vec3(m02, m12, m22)
        case 2:
            return mat.z;

        default:
            BASE_ASSERT(false);
            return Vec3(0.0f, 0.0f, 0.0f);
    }
}

/** Get column of the matrix */
static inline constexpr Vec4 GetColumn(const Mat4X4& mat, int index)
{
    switch (index) {
            // the first column of the matrix: Vec4(m00, m10, m20, m30)
        case 0:
            return mat.x;
            // the second column of the matrix:  Vec4(m01, m11, m21, m31)
        case 1:
            return mat.y;
            // the third column of the matrix:  Vec4(m02, m12, m22, m32)
        case 2:
            return mat.z;
            // the fourth column of the matrix:  Vec4(m03, m13, m23, m33)
        case 3:
            return mat.w;

        default:
            BASE_ASSERT(false);
            return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

/** Returns row of the matrix */
static inline constexpr Vec3 GetRow(const Mat3X3& mat, int index)
{
    switch (index) {
            // the first row of the matrix: Vec4(m00, m01, m02, m03)
        case 0:
            return Vec3(mat.x.x, mat.y.x, mat.z.x);
            // the second row of the matrix: Vec4(m10, m11, m12, m13)
        case 1:
            return Vec3(mat.x.y, mat.y.y, mat.z.y);
            // the third row of the matrix: Vec4(m20, m21, m22, m23)
        case 2:
            return Vec3(mat.x.z, mat.y.z, mat.z.z);
        default:
            BASE_ASSERT(false);
            return Vec3(0.0f, 0.0f, 0.0f);
    }
}

/** Returns row of the matrix */
static inline constexpr Vec4 GetRow(const Mat4X4& mat, int index)
{
    switch (index) {
            // the first row of the matrix: Vec4(m00, m01, m02, m03)
        case 0:
            return Vec4(mat.x.x, mat.y.x, mat.z.x, mat.w.x);
            // the second row of the matrix: Vec4(m10, m11, m12, m13)
        case 1:
            return Vec4(mat.x.y, mat.y.y, mat.z.y, mat.w.y);
            // the third row of the matrix: Vec4(m20, m21, m22, m23)
        case 2:
            return Vec4(mat.x.z, mat.y.z, mat.z.z, mat.w.z);
            // the fourth row of the matrix: Vec4(m30, m31, m32, m33)
        case 3:
            return Vec4(mat.x.w, mat.y.w, mat.z.w, mat.w.w);

        default:
            BASE_ASSERT(false);
            return Vec4(0.0f, 0.0f, 0.0f, 0.0f);
    }
}

/** Converts quaternion to matrix 3x3 */
static inline constexpr Mat3X3 Mat3Cast(const Quat& quaternion)
{
    Mat3X3 result;
    const float qXX(quaternion.x * quaternion.x);
    const float qYY(quaternion.y * quaternion.y);
    const float qZZ(quaternion.z * quaternion.z);
    const float qXZ(quaternion.x * quaternion.z);
    const float qXY(quaternion.x * quaternion.y);
    const float qYZ(quaternion.y * quaternion.z);
    const float qWX(quaternion.w * quaternion.x);
    const float qWY(quaternion.w * quaternion.y);
    const float qWZ(quaternion.w * quaternion.z);

    result.x.x = 1.0f - 2.0f * (qYY + qZZ);
    result.x.y = 2.0f * (qXY + qWZ);
    result.x.z = 2.0f * (qXZ - qWY);

    result.y.x = 2.0f * (qXY - qWZ);
    result.y.y = 1.0f - 2.0f * (qXX + qZZ);
    result.y.z = 2.0f * (qYZ + qWX);

    result.z.x = 2.0f * (qXZ + qWY);
    result.z.y = 2.0f * (qYZ - qWX);
    result.z.z = 1.0f - 2.0f * (qXX + qYY);
    return result;
}

/** Convert 3x3 matrix to 4x4 */
static inline constexpr Mat4X4 DimensionalShift(const Mat3X3& mat)
{
    Mat4X4 result;
    result.x = { mat.x.x, mat.x.y, 0, mat.x.z };
    result.y = { mat.y.x, mat.y.y, 0, mat.y.z };
    result.z = { 0, 0, 1, 0 };
    result.w = { mat.z.x, mat.z.y, 0, mat.z.z };
    return result;
}

/** Converts quaternion to matrix 4X4 */
static inline constexpr Mat4X4 Mat4Cast(const Quat& quaternion)
{
    return Mat4X4(Mat3Cast(quaternion));
}

/** Translate by vector2 */
static inline constexpr Mat3X3 Translate(const Mat3X3& mat, const Vec2& vec)
{
    Mat3X3 result(mat);
    result.z = mat.x * vec.x + mat.y * vec.y + mat.z;
    return result;
}

/** Translate by vector2 */
static inline constexpr Mat4X4 Translate(const Mat4X4& mat, const Vec2& vec)
{
    Mat4X4 result(mat);
    result.w = mat.x * vec.x + mat.y * vec.y + mat.w;
    return result;
}
/** Translate by vector3 */
static inline constexpr Mat4X4 Translate(const Mat4X4& mat, const Vec3& vec)
{
    Mat4X4 result(mat);
    result.w = mat.x * vec.x + mat.y * vec.y + mat.z * vec.z + mat.w;
    return result;
}
/** Skew angle in X and Y in radian */
static inline Mat4X4 SkewXY(const Mat4X4& mat, const Vec2& vec)
{
    Mat4X4 result(mat);
    result.x += mat.y * tanf(vec.y);
    result.y += mat.x * tanf(vec.x);
    return result;
}
/** Rotate on Z axis by angle in radian */
static inline Mat4X4 RotateZCWRadians(const Mat4X4& mat, float rot)
{
    Mat4X4 result(mat);
    float co = cosf(rot);
    float si = sinf(rot);
    result.x = mat.x * co + mat.y * si;
    result.y = mat.y * co - mat.x * si;
    return result;
}

/** Creates translation, rotation and scaling matrix from translation vector, rotation quaternion and scale vector */
static inline constexpr Mat4X4 Trs(
    const Vec3& translationVector, const Quat& rotationQuaternion, const Vec3& scaleVector)
{
    return Scale(Translate(Mat4X4(1.f), translationVector) * Mat4Cast(rotationQuaternion), scaleVector);
}

/** Transforms direction by this matrix */
static inline constexpr Vec3 MultiplyVector(const Mat4X4& mat, const Vec3& vec)
{
    Vec3 res;
    res.x = mat.x.x * vec.x + mat.y.x * vec.y + mat.z.x * vec.z;
    res.y = mat.x.y * vec.x + mat.y.y * vec.y + mat.z.y * vec.z;
    res.z = mat.x.z * vec.x + mat.y.z * vec.y + mat.z.z * vec.z;
    return res;
}

/** Transforms position by this matrix without perspective divide */
static inline constexpr Vec2 MultiplyPoint2X4(const Mat4X4& mat4X4, const Vec2& point)
{
    Vec2 result;
    result.x = mat4X4.x.x * point.x + mat4X4.y.x * point.y + mat4X4.w.x;
    result.y = mat4X4.x.y * point.x + mat4X4.y.y * point.y + mat4X4.w.y;

    return result;
}
/** Transforms direction by this matrix without perspective divide */
static inline constexpr Vec2 MultiplyVector2X4(const Mat4X4& mat4X4, const Vec2& point)
{
    Vec2 result;
    result.x = mat4X4.x.x * point.x + mat4X4.y.x * point.y;
    result.y = mat4X4.x.y * point.x + mat4X4.y.y * point.y;

    return result;
}
/** Transforms position by this matrix without perspective divide */
static inline constexpr Vec2 MultiplyPoint2X3(const Mat3X3& mat3X3, const Vec2& point)
{
    Vec2 result;
    result.x = mat3X3.x.x * point.x + mat3X3.y.x * point.y + mat3X3.z.x;
    result.y = mat3X3.x.y * point.x + mat3X3.y.y * point.y + mat3X3.z.y;

    return result;
}

/** Transforms position by this matrix without perspective divide */
static inline constexpr Vec3 MultiplyPoint3X4(const Mat4X4& mat4X4, const Vec3& point)
{
    Vec3 result;
    result.x = mat4X4.x.x * point.x + mat4X4.y.x * point.y + mat4X4.z.x * point.z + mat4X4.w.x;
    result.y = mat4X4.x.y * point.x + mat4X4.y.y * point.y + mat4X4.z.y * point.z + mat4X4.w.y;
    result.z = mat4X4.x.z * point.x + mat4X4.y.z * point.y + mat4X4.z.z * point.z + mat4X4.w.z;

    return result;
}

/** Inverse matrix with determinant output */
static inline constexpr Mat3X3 Inverse(Mat3X3 const& m, float& determinantOut)
{
    determinantOut = m.x.x * (m.y.y * m.z.z - m.z.y * m.y.z) - m.x.y * (m.y.x * m.z.z - m.y.z * m.z.x) +
                     m.x.z * (m.y.x * m.z.y - m.y.y * m.z.x);

    const float invdet = 1 / determinantOut;

    Mat3X3 inverse;
    inverse.x.x = (m.y.y * m.z.z - m.z.y * m.y.z) * invdet;
    inverse.x.y = (m.x.z * m.z.y - m.x.y * m.z.z) * invdet;
    inverse.x.z = (m.x.y * m.y.z - m.x.z * m.y.y) * invdet;
    inverse.y.x = (m.y.z * m.z.x - m.y.x * m.z.z) * invdet;
    inverse.y.y = (m.x.x * m.z.z - m.x.z * m.z.x) * invdet;
    inverse.y.z = (m.y.x * m.x.z - m.x.x * m.y.z) * invdet;
    inverse.z.x = (m.y.x * m.z.y - m.z.x * m.y.y) * invdet;
    inverse.z.y = (m.z.x * m.x.y - m.x.x * m.z.y) * invdet;
    inverse.z.z = (m.x.x * m.y.y - m.y.x * m.x.y) * invdet;
    return inverse;
}

/** Inverse matrix */
static inline constexpr Mat3X3 Inverse(const Mat3X3& mat3X3)
{
    float determinant = 0.0f;
    return Inverse(mat3X3, determinant);
}

/** Inverse matrix with determinant output */
static inline constexpr Mat4X4 Inverse(Mat4X4 const& m, float& determinantOut)
{
    const float coef00 = m.z.z * m.w.w - m.w.z * m.z.w;
    const float coef02 = m.y.z * m.w.w - m.w.z * m.y.w;
    const float coef03 = m.y.z * m.z.w - m.z.z * m.y.w;

    const float coef04 = m.z.y * m.w.w - m.w.y * m.z.w;
    const float coef06 = m.y.y * m.w.w - m.w.y * m.y.w;
    const float coef07 = m.y.y * m.z.w - m.z.y * m.y.w;

    const float coef08 = m.z.y * m.w.z - m.w.y * m.z.z;
    const float coef10 = m.y.y * m.w.z - m.w.y * m.y.z;
    const float coef11 = m.y.y * m.z.z - m.z.y * m.y.z;

    const float coef12 = m.z.x * m.w.w - m.w.x * m.z.w;
    const float coef14 = m.y.x * m.w.w - m.w.x * m.y.w;
    const float coef15 = m.y.x * m.z.w - m.z.x * m.y.w;

    const float coef16 = m.z.x * m.w.z - m.w.x * m.z.z;
    const float coef18 = m.y.x * m.w.z - m.w.x * m.y.z;
    const float coef19 = m.y.x * m.z.z - m.z.x * m.y.z;

    const float coef20 = m.z.x * m.w.y - m.w.x * m.z.y;
    const float coef22 = m.y.x * m.w.y - m.w.x * m.y.y;

    const float coef23 = m.y.x * m.z.y - m.z.x * m.y.y;

    const Vec4 fac0(coef00, coef00, coef02, coef03);
    const Vec4 fac1(coef04, coef04, coef06, coef07);
    const Vec4 fac2(coef08, coef08, coef10, coef11);
    const Vec4 fac3(coef12, coef12, coef14, coef15);
    const Vec4 fac4(coef16, coef16, coef18, coef19);
    const Vec4 fac5(coef20, coef20, coef22, coef23);

    const Vec4 vec0(m.y.x, m.x.x, m.x.x, m.x.x);
    const Vec4 vec1(m.y.y, m.x.y, m.x.y, m.x.y);
    const Vec4 vec2(m.y.z, m.x.z, m.x.z, m.x.z);
    const Vec4 vec3(m.y.w, m.x.w, m.x.w, m.x.w);

    const Vec4 inv0(vec1 * fac0 - vec2 * fac1 + vec3 * fac2);
    const Vec4 inv1(vec0 * fac0 - vec2 * fac3 + vec3 * fac4);
    const Vec4 inv2(vec0 * fac1 - vec1 * fac3 + vec3 * fac5);
    const Vec4 inv3(vec0 * fac2 - vec1 * fac4 + vec2 * fac5);

    constexpr Vec4 signA(+1.f, -1.f, +1.f, -1.f);
    constexpr Vec4 signB(-1.f, +1.f, -1.f, +1.f);
    const Mat4X4 inverse(inv0 * signA, inv1 * signB, inv2 * signA, inv3 * signB);

    const Vec4 row0(inverse.x.x, inverse.y.x, inverse.z.x, inverse.w.x);

    const Vec4 dot0(m.x * row0);
    determinantOut = (dot0.x + dot0.y) + (dot0.z + dot0.w);

    const float oneOverDeterminant = 1.0f / determinantOut;

    return inverse * oneOverDeterminant;
}

/** Get determinant out from matrix 4X4 */
static inline constexpr float Determinant(const Mat4X4& mat4X4)
{
    float determinant = 0.0f;
    Inverse(mat4X4, determinant);
    return determinant;
}

/** Inverse matrix */
static inline constexpr Mat4X4 Inverse(const Mat4X4& mat4X4)
{
    float determinant = 0.0f;
    return Inverse(mat4X4, determinant);
}

/** Inner product */
inline constexpr Vec4 operator*(const Mat4X4& m, const Vec4& v)
{
    return { v.x * m.x.x + v.y * m.y.x + v.z * m.z.x + v.w * m.w.x,
        v.x * m.x.y + v.y * m.y.y + v.z * m.z.y + v.w * m.w.y, v.x * m.x.z + v.y * m.y.z + v.z * m.z.z + v.w * m.w.z,
        v.x * m.x.w + v.y * m.y.w + v.z * m.z.w + v.w * m.w.w };
}

inline constexpr Vec3 operator*(const Mat3X3& m, const Vec3& v)
{
    return { v.x * m.x.x + v.y * m.y.x + v.z * m.z.x, v.x * m.x.y + v.y * m.y.y + v.z * m.z.y,
        v.x * m.x.z + v.y * m.y.z + v.z * m.z.z };
}

/** Creates matrix for left handed symmetric perspective view frustum, near and far clip planes correspond to z
 * normalized device coordinates of 0 and +1 */
static inline Mat4X4 PerspectiveLhZo(float fovy, float aspect, float zNear, float zFar)
{
    float const tanHalfFovy = tan(fovy / 2.0f);

    Mat4X4 result(0.f);
    if (const auto div = (aspect * tanHalfFovy); abs(div) > EPSILON) {
        result.x.x = 1.0f / div;
    } else {
        result.x.x = HUGE_VALF;
    }
    if (abs(tanHalfFovy) > EPSILON) {
        result.y.y = 1.0f / (tanHalfFovy);
    } else {
        result.y.y = HUGE_VALF;
    }

    if (const auto div = (zFar - zNear); abs(div) > EPSILON) {
        result.z.z = zFar / div;
        result.w.z = -(zFar * zNear) / div;
    } else {
        result.z.z = HUGE_VALF;
        result.w.z = HUGE_VALF;
    }
    result.z.w = 1.0f;
    return result;
}

/** Creates matrix for right handed symmetric perspective view frustum, near and far clip planes correspond to z
 * normalized device coordinates of 0 and +1 */
static inline Mat4X4 PerspectiveRhZo(float fovy, float aspect, float zNear, float zFar)
{
    Mat4X4 result = PerspectiveLhZo(fovy, aspect, zNear, zFar);
    result.z.z = -result.z.z;
    result.z.w = -result.z.w;

    return result;
}

/** Creates matrix for left handed symmetric perspective view frustum, near and far clip planes correspond to z
 * normalized device coordinates of -1 and +1 */
static inline Mat4X4 PerspectiveLhNo(float fovy, float aspect, float zNear, float zFar)
{
    float const tanHalfFovy = tan(fovy / 2.0f);

    Mat4X4 result(0.f);
    if (const auto div = (aspect * tanHalfFovy); abs(div) > EPSILON) {
        result.x.x = 1.0f / div;
    } else {
        result.x.x = HUGE_VALF;
    }
    if (abs(tanHalfFovy) > EPSILON) {
        result.y.y = 1.0f / (tanHalfFovy);
    } else {
        result.y.y = HUGE_VALF;
    }

    if (const auto div = (zFar - zNear); abs(div) > EPSILON) {
        result.z.z = (zFar + zNear) / div;
        result.w.z = -(2.0f * zFar * zNear) / div;
    } else {
        result.z.z = HUGE_VALF;
        result.w.z = HUGE_VALF;
    }
    result.z.w = 1.0f;
    return result;
}

/** Creates a matrix for right handed symmetric perspective view frustum, near and far clip planes correspond to z
 * normalized device coordinates of -1 and +1 */
static inline Mat4X4 PerspectiveRhNo(float fovy, float aspect, float zNear, float zFar)
{
    Mat4X4 result = PerspectiveLhNo(fovy, aspect, zNear, zFar);
    result.z.z = -result.z.z;
    result.z.w = -result.z.w;

    return result;
}

/** Creates matrix for orthographic parallel viewing volume using left handed coordinates, near and far clip planes
 * correspond to z normalized device coordinates of 0 and +1 */
static inline Mat4X4 OrthoLhZo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result(1.f);
    result.x.x = 2.0f / (right - left);
    result.y.y = 2.0f / (top - bottom);
    result.z.z = 1.0f / (zFar - zNear);
    result.w.x = -(right + left) / (right - left);
    result.w.y = -(top + bottom) / (top - bottom);
    result.w.z = -zNear / (zFar - zNear);
    return result;
}

/** Creates matrix for orthographic parallel viewing volume using left handed coordinates, near and far clip planes
 * correspond to z normalized device coordinates of -1 and +1 */
static inline Mat4X4 OrthoLhNo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result(1.f);
    result.x.x = 2.0f / (right - left);
    result.y.y = 2.0f / (top - bottom);
    result.z.z = 2.0f / (zFar - zNear);
    result.w.x = -(right + left) / (right - left);
    result.w.y = -(top + bottom) / (top - bottom);
    result.w.z = -(zFar + zNear) / (zFar - zNear);
    return result;
}

/** Creates matrix for orthographic parallel viewing volume using right handed coordinates, near and far clip planes
 * correspond to z normalized device coordinates of 0 and +1 */
static inline Mat4X4 OrthoRhZo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result = OrthoLhZo(left, right, bottom, top, zNear, zFar);
    result.z.z = -result.z.z;
    return result;
}

/** Creates matrix for orthographic parallel viewing volume using right handed coordinates, near and far clip planes
 * correspond to z normalized device coordinates of -1 and +1 */
static inline Mat4X4 OrthoRhNo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result = OrthoLhNo(left, right, bottom, top, zNear, zFar);
    result.z.z = -result.z.z;
    return result;
}

/** Creates matrix for left handed perspective view frustum, near and far clip planes correspond to z normalized device
 * coordinates of 0 and +1 */
static inline Mat4X4 PerspectiveLhZo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result(0.f);
    if (const float width = right - left; abs(width) > EPSILON) {
        result.x.x = 2.0f * zNear / width;
        result.z.x = (left + right) / width;
    } else {
        result.x.x = HUGE_VALF;
        result.z.x = HUGE_VALF;
    }

    if (const float height = top - bottom; abs(height) > EPSILON) {
        result.y.y = 2.0f * zNear / height;
        result.z.y = (top + bottom) / height;
    } else {
        result.y.y = HUGE_VALF;
        result.z.y = HUGE_VALF;
    }

    if (const float depth = zFar - zNear; abs(depth) > EPSILON) {
        result.z.z = zFar / depth;
        result.w.z = -(zFar * zNear) / depth;
    } else {
        result.z.z = HUGE_VALF;
        result.w.z = HUGE_VALF;
    }
    result.z.w = 1.0f;
    return result;
}

/** Creates matrix for right handed perspective view frustum, near and far clip planes correspond to z normalized device
 * coordinates of 0 and +1 */
static inline Mat4X4 PerspectiveRhZo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result = PerspectiveLhZo(left, right, bottom, top, zNear, zFar);
    result.z.z = -result.z.z;
    result.z.w = -result.z.w;

    return result;
}

/** Creates matrix for left handed perspective view frustum, near and far clip planes correspond to z normalized device
 * coordinates of -1 and +1 */
static inline Mat4X4 PerspectiveLhNo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result(0.f);
    if (const float width = right - left; abs(width) > EPSILON) {
        result.x.x = 2.0f * zNear / width;
        result.z.x = (left + right) / width;
    } else {
        result.x.x = HUGE_VALF;
        result.z.x = HUGE_VALF;
    }

    if (const float height = top - bottom; abs(height) > EPSILON) {
        result.y.y = 2.0f * zNear / height;
        result.z.y = (top + bottom) / height;
    } else {
        result.y.y = HUGE_VALF;
        result.z.y = HUGE_VALF;
    }

    if (const float depth = zFar - zNear; abs(depth) > EPSILON) {
        result.z.z = (zNear + zFar) / depth;
        result.w.z = -(2.0f * zFar * zNear) / depth;
    } else {
        result.z.z = HUGE_VALF;
        result.w.z = HUGE_VALF;
    }
    result.z.w = 1.0f;
    return result;
}

/** Creates matrix for right handed perspective view frustum, near and far clip planes correspond to z normalized device
 * coordinates of -1 and +1 */
static inline Mat4X4 PerspectiveRhNo(float left, float right, float bottom, float top, float zNear, float zFar)
{
    Mat4X4 result = PerspectiveLhNo(left, right, bottom, top, zNear, zFar);
    result.z.z = -result.z.z;
    result.z.w = -result.z.w;

    return result;
}

/** Right handed look at function */
static inline Mat4X4 LookAtRh(Vec3 const& eye, Vec3 const& center, Vec3 const& up)
{
    Vec3 const f(Normalize(center - eye));
    Vec3 const s(Normalize(Cross(f, up)));
    Vec3 const u(Cross(s, f));

    Mat4X4 result(1.f);
    result.x.x = s.x;
    result.y.x = s.y;
    result.z.x = s.z;
    result.x.y = u.x;
    result.y.y = u.y;
    result.z.y = u.z;
    result.x.z = -f.x;
    result.y.z = -f.y;
    result.z.z = -f.z;
    result.w.x = -Dot(s, eye);
    result.w.y = -Dot(u, eye);
    result.w.z = Dot(f, eye);
    return result;
}

/** Left handed look at function */
static inline Mat4X4 LookAtLh(Vec3 const& eye, Vec3 const& center, Vec3 const& up)
{
    Vec3 const f(Normalize(center - eye));
    Vec3 const s(Normalize(Cross(up, f)));
    Vec3 const u(Cross(f, s));

    Mat4X4 result(1.f);
    result.x.x = s.x;
    result.y.x = s.y;
    result.z.x = s.z;
    result.x.y = u.x;
    result.y.y = u.y;
    result.z.y = u.z;
    result.x.z = f.x;
    result.y.z = f.y;
    result.z.z = f.z;
    result.w.x = -Dot(s, eye);
    result.w.y = -Dot(u, eye);
    result.w.z = -Dot(f, eye);
    return result;
}

/** Decompose matrix */
static inline bool Decompose(
    Mat4X4 const& modelMatrix, Vec3& scale, Quat& orientation, Vec3& translation, Vec3& skew, Vec4& perspective)
{
    Mat4X4 localMatrix(modelMatrix);

    if (localMatrix.w.w != 1.f) {
        if (abs(localMatrix.w.w) < EPSILON) {
            return false;
        }

        for (size_t i = 0; i < 4U; ++i) {
            for (size_t j = 0; j < 4U; ++j) {
                localMatrix[i][j] /= localMatrix.w.w;
            }
        }
    }

    if (abs(localMatrix.x.w) >= EPSILON || abs(localMatrix.y.w) >= EPSILON || abs(localMatrix.z.w) >= EPSILON) {
        Vec4 rightHandSide;
        rightHandSide.x = localMatrix.x.w;
        rightHandSide.y = localMatrix.y.w;
        rightHandSide.z = localMatrix.z.w;
        rightHandSide.w = localMatrix.w.w;

        Mat4X4 perspectiveMatrix(localMatrix);
        for (size_t i = 0U; i < 3U; i++) {
            perspectiveMatrix[i].w = 0.0f;
        }
        perspectiveMatrix.w.w = 1.0f;

        float determinant;
        const Mat4X4 inversePerspectiveMatrix = Inverse(perspectiveMatrix, determinant);
        if (abs(determinant) < EPSILON) {
            return false;
        }

        const Mat4X4 transposedInversePerspectiveMatrix = Transpose(inversePerspectiveMatrix);

        perspective = transposedInversePerspectiveMatrix * rightHandSide;

        localMatrix.x.w = localMatrix.y.w = localMatrix.z.w = 0.0f;
        localMatrix.w.w = 1.0f;
    } else {
        perspective = Vec4(0.0f, 0.0f, 0.0f, 1.0f);
    }

    translation = Vec3(localMatrix.w);
    localMatrix.w = Vec4(0.0f, 0.0f, 0.0f, localMatrix.w.w);

    Vec3 row[3];
    for (size_t i = 0U; i < 3U; ++i) {
        for (size_t j = 0U; j < 3U; ++j) {
            row[i][j] = localMatrix[i][j];
        }
    }

    scale.x = sqrt(Dot(row[0], row[0]));

    row[0] = Math::Scale(row[0], 1.0f);

    skew.z = Dot(row[0], row[1]);
    row[1] = Combine(row[1], row[0], 1.0f, -skew.z);

    scale.y = sqrt(Dot(row[1], row[1]));
    row[1] = Math::Scale(row[1], 1.0f);
    skew.z /= scale.y;

    skew.y = Dot(row[0], row[2]);
    row[2] = Combine(row[2], row[0], 1.0f, -skew.y);
    skew.x = Dot(row[1], row[2]);
    row[2] = Combine(row[2], row[1], 1.0f, -skew.x);

    scale.z = sqrt(Dot(row[2], row[2]));
    row[2] = Math::Scale(row[2], 1.0f);
    skew.y /= scale.z;
    skew.x /= scale.z;

    const Vec3 pDum3 = Cross(row[1], row[2]);
    if (Dot(row[0], pDum3) < 0) {
        for (size_t i = 0U; i < 3U; i++) {
            scale[i] *= -1.0f;
            row[i] *= -1.0f;
        }
    }

    unsigned i, j, k = 0U;
    float root;
    const float trace = row[0].x + row[1].y + row[2].z;
    if (trace > 0.0f) {
        root = sqrt(trace + 1.0f);
        orientation.w = 0.5f * root;
        root = 0.5f / root; // root cannot be zero as it's square root of at least 1
        orientation.x = root * (row[1].z - row[2].y);
        orientation.y = root * (row[2].x - row[0].z);
        orientation.z = root * (row[0].y - row[1].x);
    } else { // End if > 0
        constexpr const unsigned next[3] = { 1U, 2U, 0U };
        i = 0U;
        if (row[1].y > row[0].x) {
            i = 1U;
        }
        if (row[2].z > row[i][i]) {
            i = 2U;
        }
        j = next[i];
        k = next[j];

        root = sqrt(row[i][i] - row[j][j] - row[k][k] + 1.0f);

        orientation[i] = 0.5f * root;
        root = (abs(root) > EPSILON) ? 0.5f / root : HUGE_VALF;
        orientation[j] = root * (row[i][j] + row[j][i]);
        orientation[k] = root * (row[i][k] + row[k][i]);
        orientation.w = root * (row[j][k] - row[k][j]);
    } // End if <= 0

    return true;
}
/** Check if matrix has any rotation/shear components */
static inline bool HasRotation(Mat3X3 const& m)
{
    return (m.x.y != 0.f || m.y.x != 0.f);
}

/** Check if matrix has any rotation/shear components */
static inline bool HasRotation(Mat4X4 const& m)
{
    return (m.x.y != 0.f || m.x.z != 0.f || m.y.x != 0.f || m.y.z != 0.f || m.z.x != 0.f || m.z.y != 0.f);
}

/** @} */
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_MATRIX_UTIL_H
