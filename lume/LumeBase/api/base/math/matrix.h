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

#ifndef API_BASE_MATH_MATRIX_H
#define API_BASE_MATH_MATRIX_H

#include <cstddef>
#if defined(_M_X64)
#include <immintrin.h>
#elif defined(_M_ARM64) || defined(__ARM_ARCH_ISA_A64)
#include <arm_neon.h>
#endif
#include <base/containers/type_traits.h>
#include <base/math/vector.h>
#include <base/math/vector_util.h>
#include <base/namespace.h>

// m00[0] m01[3] m02[6]
// m10[1] m11[4] m12[7]
// m20[2] m21[5] m22[8]

// m00[0] m01[4] m02[8]  m03[12]
// m10[1] m11[5] m12[9]  m13[13]
// m20[2] m21[6] m22[10] m23[14]
// m30[3] m31[7] m32[11] m33[15]

// m00[0] m01[4] m02[8]
// m10[1] m11[5] m12[9]
// m20[2] m21[6] m22[10]
// m30[3] m31[7] m32[11]
BASE_BEGIN_NAMESPACE()
namespace Math {
#include <base/math/disable_warning_4201_heading.h>

/** @ingroup group_math_matrix */
/** Matrix 3X3 presentation in column major format */
class Mat3X3 final {
public:
    union {
        struct {
            Vec3 x, y, z;
        };
        Vec3 base[3];
        float data[9];
    };

    /** Subscript operator */
    constexpr Vec3& operator[](size_t aIndex)
    {
        return base[aIndex];
    }

    /** Subscript operator */
    constexpr const Vec3& operator[](size_t aIndex) const
    {
        return base[aIndex];
    }

    // Constructors
    /** Default constructor */
    inline constexpr Mat3X3() noexcept : data { 0 } {}

    /** Identity constructor */
    inline explicit constexpr Mat3X3(float id) noexcept : data { id, 0.0f, 0.0f, 0.0f, id, 0.0f, 0.0f, 0.0f, id } {}

    /** Constructor for using Vector3's */
    inline constexpr Mat3X3(Vec3 const& v0, Vec3 const& v1, Vec3 const& v2) noexcept : x(v0), y(v1), z(v2) {}

    /** Constructor for array of floats */
    inline constexpr Mat3X3(const float d[9]) noexcept : data { d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8] }
    {}

    /** Add two matrices */
    inline constexpr Mat3X3 operator+(const Mat3X3& rhs) const
    {
        return Mat3X3(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    /** Add two matrices */
    inline constexpr Mat3X3& operator+=(const Mat3X3& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }

    /** Subtract two matrices */
    inline constexpr Mat3X3 operator-(const Mat3X3& rhs) const
    {
        return Mat3X3(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    /** Subtract two matrices */
    inline constexpr Mat3X3& operator-=(const Mat3X3& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    /** Multiply two matrices */
    inline constexpr Mat3X3 operator*(const Mat3X3& rhs) const
    {
        const Vec3& rha { rhs.x.x, rhs.y.x, rhs.z.x };
        const Vec3& rhb { rhs.x.y, rhs.y.y, rhs.z.y };
        const Vec3& rhc { rhs.x.z, rhs.y.z, rhs.z.z };

        return { { Dot(x, rha), Dot(x, rhb), Dot(x, rhc) }, { Dot(y, rha), Dot(y, rhb), Dot(y, rhc) },
            { Dot(z, rha), Dot(z, rhb), Dot(z, rhc) } };
    }

    /** Multiply columns by float scalar value */
    inline constexpr Mat3X3 operator*(const float& scalar) const
    {
        return Mat3X3(x * scalar, y * scalar, z * scalar);
    }

    /** Equality operator, returns true if matrices are equal */
    inline constexpr bool operator==(const Mat3X3& mat) const
    {
        for (size_t i = 0; i < countof(data); ++i) {
            if (data[i] != mat.data[i]) {
                return false;
            }
        }
        return true;
    }

    /** Inequality operator, returns true if matrices are inequal */
    inline constexpr bool operator!=(const Mat3X3& mat) const
    {
        for (size_t i = 0; i < countof(data); ++i) {
            if (data[i] != mat.data[i]) {
                return true;
            }
        }
        return false;
    }
};

// Assert that Mat3X3 is the same as 9 floats
static_assert(sizeof(Mat3X3) == 9 * sizeof(float));

static constexpr Mat3X3 IDENTITY_3X3(1.f);

/** @ingroup group_math_matrix */
/** Matrix 4X4 presentation in column major format */
class Mat4X4 final {
public:
    union {
        struct {
            Vec4 x, y, z, w;
        };
        Vec4 base[4]; // base[0] is X ,base [1] is Y, etc..
        float data[16];
    };

    // "For programming purposes, OpenGL matrices are 16-value arrays with base vectors laid out contiguously in memory.
    // The translation components occupy the 13th, 14th, and 15th elements of the 16-element matrix."
    // https://www.khronos.org/opengl/wiki/General_OpenGL:_Transformations#Are_OpenGL_matrices_column-major_or_row-major.3F
    // this is also the same as with glm.
    /** Subscript operator */
    constexpr Vec4& operator[](size_t aIndex)
    {
        return base[aIndex];
    }

    /** Subscript operator */
    constexpr const Vec4& operator[](size_t aIndex) const
    {
        return base[aIndex];
    }

    // Constructors
    /** Zero initializer constructor */
    inline constexpr Mat4X4() : data { 0 } {}

    /** Constructor for Vector4's */
    inline constexpr Mat4X4(Vec4 const& v0, Vec4 const& v1, Vec4 const& v2, Vec4 const& v3) : x(v0), y(v1), z(v2), w(v3)
    {}

    /** Constructor for array of floats */
    inline constexpr Mat4X4(const float d[16])
        : data { d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11], d[12], d[13], d[14], d[15] }
    {}

    /** Constructor for floats */
    inline constexpr Mat4X4(float d0, float d1, float d2, float d3, float d4, float d5, float d6, float d7, float d8,
        float d9, float d10, float d11, float d12, float d13, float d14, float d15)
        : data { d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11, d12, d13, d14, d15 }
    {}

    /** Identity constructor */
    inline explicit constexpr Mat4X4(float id)
        : data { id, 0.0f, 0.0f, 0.0f, 0.0f, id, 0.0f, 0.0f, 0.0f, 0.0f, id, 0.0f, 0.0f, 0.0f, 0.0f, id }
    {}

    /** Conversion constructor from Mat3X3 to Mat4X4 */
    explicit inline constexpr Mat4X4(const Mat3X3& mat3X3)
        : data { mat3X3.data[0], mat3X3.data[1], mat3X3.data[2], 0.0f, mat3X3.data[3], mat3X3.data[4], mat3X3.data[5],
              0.0f, mat3X3.data[6], mat3X3.data[7], mat3X3.data[8], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f }
    {}

    /** Multiply two matrices */
    inline Mat4X4 operator*(const Mat4X4& rhs) const
    {
        Mat4X4 res;
#if defined(BASE_SIMD) && defined(_M_X64)
#if USE_FMA_AVX
        // duplicate components of the first column of rhs.
        __m128 rhsX = _mm_broadcast_ss(rhs.x.data + 0);
        __m128 rhsY = _mm_broadcast_ss(rhs.x.data + 1);
        __m128 rhsZ = _mm_broadcast_ss(rhs.x.data + 2);
        __m128 rhsW = _mm_broadcast_ss(rhs.x.data + 3);
        // multiply each column of lhs with rhs's first columns components while adding the results
        __m128 rhsC = _mm_mul_ps(x.vec4, rhsX);        // res.x = lhs.x * rhs.x.x
        rhsC = _mm_fmadd_ps(y.vec4, rhsY, rhsC);       // res.x += lhs.y * rhs.x.y
        rhsC = _mm_fmadd_ps(z.vec4, rhsZ, rhsC);       // res.x += lhs.z * rhs.x.z
        res.x.vec4 = _mm_fmadd_ps(w.vec4, rhsW, rhsC); // res.x += lhs.w * rhs.x.w

        // repeat for second column of rhs
        rhsX = _mm_broadcast_ss(rhs.y.data + 0);
        rhsY = _mm_broadcast_ss(rhs.y.data + 1);
        rhsZ = _mm_broadcast_ss(rhs.y.data + 2);
        rhsW = _mm_broadcast_ss(rhs.y.data + 3);
        rhsC = _mm_mul_ps(x.vec4, rhsX);
        rhsC = _mm_fmadd_ps(y.vec4, rhsY, rhsC);
        rhsC = _mm_fmadd_ps(z.vec4, rhsZ, rhsC);
        res.y.vec4 = _mm_fmadd_ps(w.vec4, rhsW, rhsC);

        // repeat for third column of rhs
        rhsX = _mm_broadcast_ss(rhs.z.data + 0);
        rhsY = _mm_broadcast_ss(rhs.z.data + 1);
        rhsZ = _mm_broadcast_ss(rhs.z.data + 2);
        rhsW = _mm_broadcast_ss(rhs.z.data + 3);
        rhsC = _mm_mul_ps(x.vec4, rhsX);
        rhsC = _mm_fmadd_ps(y.vec4, rhsY, rhsC);
        rhsC = _mm_fmadd_ps(z.vec4, rhsZ, rhsC);
        res.z.vec4 = _mm_fmadd_ps(w.vec4, rhsW, rhsC);

        // repeat for fourth column of rhs
        rhsX = _mm_broadcast_ss(rhs.w.data + 0);
        rhsY = _mm_broadcast_ss(rhs.w.data + 1);
        rhsZ = _mm_broadcast_ss(rhs.w.data + 2);
        rhsW = _mm_broadcast_ss(rhs.w.data + 3);
        rhsC = _mm_mul_ps(x.vec4, rhsX);
        rhsC = _mm_fmadd_ps(y.vec4, rhsY, rhsC);
        rhsC = _mm_fmadd_ps(z.vec4, rhsZ, rhsC);
        res.w.vec4 = _mm_fmadd_ps(w.vec4, rhsW, rhsC);
#else
        // only SSE intrisics which should be fine with any x64
        // duplicate components of the first column of rhs.
        __m128 rhsX = _mm_set_ps1(rhs.x.x);
        __m128 rhsY = _mm_set_ps1(rhs.x.y);
        __m128 rhsZ = _mm_set_ps1(rhs.x.z);
        __m128 rhsW = _mm_set_ps1(rhs.x.w);
        // multiply each column of lhs with rhs's first columns components
        rhsX = _mm_mul_ps(x.vec4, rhsX); // lhs.x * rhs.x.x
        rhsY = _mm_mul_ps(y.vec4, rhsY); // lhs.y * rhs.x.y
        rhsZ = _mm_mul_ps(z.vec4, rhsZ); // lhs.z * rhs.x.z
        rhsW = _mm_mul_ps(w.vec4, rhsW); // lhs.w * rhs.x.w
        // sum the products
        rhsX = _mm_add_ps(rhsX, rhsY); // (lhs.x * rhs.x.x) + (lhs.y * rhs.x.y)
        rhsZ = _mm_add_ps(rhsZ, rhsW); // (lhs.z * rhs.x.z) + (lhs.w * rhs.x.w)
        res.x.vec4 = _mm_add_ps(rhsX, rhsZ);

        // repeat for second column of rhs
        rhsX = _mm_set_ps1(rhs.y.x);
        rhsY = _mm_set_ps1(rhs.y.y);
        rhsZ = _mm_set_ps1(rhs.y.z);
        rhsW = _mm_set_ps1(rhs.y.w);
        rhsX = _mm_mul_ps(x.vec4, rhsX);
        rhsY = _mm_mul_ps(y.vec4, rhsY);
        rhsZ = _mm_mul_ps(z.vec4, rhsZ);
        rhsW = _mm_mul_ps(w.vec4, rhsW);
        rhsX = _mm_add_ps(rhsX, rhsY);
        rhsZ = _mm_add_ps(rhsZ, rhsW);
        res.y.vec4 = _mm_add_ps(rhsX, rhsZ);

        // repeat for third column of rhs
        rhsX = _mm_set_ps1(rhs.z.x);
        rhsY = _mm_set_ps1(rhs.z.y);
        rhsZ = _mm_set_ps1(rhs.z.z);
        rhsW = _mm_set_ps1(rhs.z.w);
        rhsX = _mm_mul_ps(x.vec4, rhsX);
        rhsY = _mm_mul_ps(y.vec4, rhsY);
        rhsZ = _mm_mul_ps(z.vec4, rhsZ);
        rhsW = _mm_mul_ps(w.vec4, rhsW);
        rhsX = _mm_add_ps(rhsX, rhsY);
        rhsZ = _mm_add_ps(rhsZ, rhsW);
        res.z.vec4 = _mm_add_ps(rhsX, rhsZ);

        // repeat for fourth column of rhs
        rhsX = _mm_set_ps1(rhs.w.x);
        rhsY = _mm_set_ps1(rhs.w.y);
        rhsZ = _mm_set_ps1(rhs.w.z);
        rhsW = _mm_set_ps1(rhs.w.w);
        rhsX = _mm_mul_ps(x.vec4, rhsX);
        rhsY = _mm_mul_ps(y.vec4, rhsY);
        rhsZ = _mm_mul_ps(z.vec4, rhsZ);
        rhsW = _mm_mul_ps(w.vec4, rhsW);
        rhsX = _mm_add_ps(rhsX, rhsY);
        rhsZ = _mm_add_ps(rhsZ, rhsW);
        res.w.vec4 = _mm_add_ps(rhsX, rhsZ);
#endif
#elif defined(_M_ARM64) || defined(__ARM_ARCH_ISA_A64)
        // multiply each column of lhs with rhs's first columns components
        float32x4_t rhsC = vld1q_f32(rhs.x.data);
        res.x.vec4 = vmovq_n_f32(0);
        res.x.vec4 = vfmaq_laneq_f32(res.x.vec4, x.vec4, rhsC, 0); // res.x += lhs.x * rhs.x.x
        res.x.vec4 = vfmaq_laneq_f32(res.x.vec4, y.vec4, rhsC, 1); // res.x += lhs.y * rhs.x.y
        res.x.vec4 = vfmaq_laneq_f32(res.x.vec4, z.vec4, rhsC, 2); // res.x += lhs.z * rhs.x.z
        res.x.vec4 = vfmaq_laneq_f32(res.x.vec4, w.vec4, rhsC, 3); // res.x += lhs.w * rhs.x.w

        // repeat for second column of rhs
        rhsC = vld1q_f32(rhs.y.data);
        res.y.vec4 = vmovq_n_f32(0);
        res.y.vec4 = vfmaq_laneq_f32(res.y.vec4, x.vec4, rhsC, 0);
        res.y.vec4 = vfmaq_laneq_f32(res.y.vec4, y.vec4, rhsC, 1);
        res.y.vec4 = vfmaq_laneq_f32(res.y.vec4, z.vec4, rhsC, 2);
        res.y.vec4 = vfmaq_laneq_f32(res.y.vec4, w.vec4, rhsC, 3);

        // repeat for third column of rhs
        rhsC = vld1q_f32(rhs.z.data);
        res.z.vec4 = vmovq_n_f32(0);
        res.z.vec4 = vfmaq_laneq_f32(res.z.vec4, x.vec4, rhsC, 0);
        res.z.vec4 = vfmaq_laneq_f32(res.z.vec4, y.vec4, rhsC, 1);
        res.z.vec4 = vfmaq_laneq_f32(res.z.vec4, z.vec4, rhsC, 2);
        res.z.vec4 = vfmaq_laneq_f32(res.z.vec4, w.vec4, rhsC, 3);

        // repeat for fourth column of rhs
        rhsC = vld1q_f32(rhs.w.data);
        res.w.vec4 = vmovq_n_f32(0);
        res.w.vec4 = vfmaq_laneq_f32(res.w.vec4, x.vec4, rhsC, 0);
        res.w.vec4 = vfmaq_laneq_f32(res.w.vec4, y.vec4, rhsC, 1);
        res.w.vec4 = vfmaq_laneq_f32(res.w.vec4, z.vec4, rhsC, 2);
        res.w.vec4 = vfmaq_laneq_f32(res.w.vec4, w.vec4, rhsC, 3);
#else
#define d data
        res.d[0] = d[0] * rhs.d[0] + d[4] * rhs.d[1] + d[8] * rhs.d[2] + d[12] * rhs.d[3];
        res.d[4] = d[0] * rhs.d[4] + d[4] * rhs.d[5] + d[8] * rhs.d[6] + d[12] * rhs.d[7];
        res.d[8] = d[0] * rhs.d[8] + d[4] * rhs.d[9] + d[8] * rhs.d[10] + d[12] * rhs.d[11];
        res.d[12] = d[0] * rhs.d[12] + d[4] * rhs.d[13] + d[8] * rhs.d[14] + d[12] * rhs.d[15];

        res.d[1] = d[1] * rhs.d[0] + d[5] * rhs.d[1] + d[9] * rhs.d[2] + d[13] * rhs.d[3];
        res.d[5] = d[1] * rhs.d[4] + d[5] * rhs.d[5] + d[9] * rhs.d[6] + d[13] * rhs.d[7];
        res.d[9] = d[1] * rhs.d[8] + d[5] * rhs.d[9] + d[9] * rhs.d[10] + d[13] * rhs.d[11];
        res.d[13] = d[1] * rhs.d[12] + d[5] * rhs.d[13] + d[9] * rhs.d[14] + d[13] * rhs.d[15];

        res.d[2] = d[2] * rhs.d[0] + d[6] * rhs.d[1] + d[10] * rhs.d[2] + d[14] * rhs.d[3];
        res.d[6] = d[2] * rhs.d[4] + d[6] * rhs.d[5] + d[10] * rhs.d[6] + d[14] * rhs.d[7];
        res.d[10] = d[2] * rhs.d[8] + d[6] * rhs.d[9] + d[10] * rhs.d[10] + d[14] * rhs.d[11];
        res.d[14] = d[2] * rhs.d[12] + d[6] * rhs.d[13] + d[10] * rhs.d[14] + d[14] * rhs.d[15];

        res.d[3] = d[3] * rhs.d[0] + d[7] * rhs.d[1] + d[11] * rhs.d[2] + d[15] * rhs.d[3];
        res.d[7] = d[3] * rhs.d[4] + d[7] * rhs.d[5] + d[11] * rhs.d[6] + d[15] * rhs.d[7];
        res.d[11] = d[3] * rhs.d[8] + d[7] * rhs.d[9] + d[11] * rhs.d[10] + d[15] * rhs.d[11];
        res.d[15] = d[3] * rhs.d[12] + d[7] * rhs.d[13] + d[11] * rhs.d[14] + d[15] * rhs.d[15];
#undef d
#endif
        return res;
    }

    /** Add two matrices */
    inline constexpr Mat4X4 operator+(const Mat4X4& rhs) const
    {
        return Mat4X4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
    }

    /** Add two matrices */
    inline constexpr Mat4X4& operator+=(const Mat4X4& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        w += rhs.w;
        return *this;
    }

    /** Subtract two matrices */
    inline constexpr Mat4X4 operator-(const Mat4X4& rhs) const
    {
        return Mat4X4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
    }

    /** Subtract two matrices */
    inline constexpr Mat4X4& operator-=(const Mat4X4& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        w -= rhs.w;
        return *this;
    }

    /** Multiply columns by float scalar value */
    inline constexpr Mat4X4 operator*(const float& scalar) const
    {
        return Mat4X4(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    /** Equality operator, returns true if matrices are equal */
    inline constexpr bool operator==(const Mat4X4& mat) const
    {
        for (size_t i = 0; i < countof(data); ++i) {
            if (data[i] != mat.data[i]) {
                return false;
            }
        }
        return true;
    }

    /** Inequality operator, returns true if matrices are inequal */
    inline constexpr bool operator!=(const Mat4X4& mat) const
    {
        for (size_t i = 0; i < countof(data); ++i) {
            if (data[i] != mat.data[i]) {
                return true;
            }
        }
        return false;
    }
};

// Assert that Mat4X4 is the same as 16 floats
static_assert(sizeof(Mat4X4) == 16 * sizeof(float));

static constexpr Mat4X4 IDENTITY_4X4(1.f);

/** @ingroup group_math_matrix */
/** Matrix 4X3 presentation in column major format */
class Mat4X3 final {
public:
    union {
        struct {
            Vec3 x, y, z, w;
        };
        Vec3 base[4]; // base[0] is X ,base [1] is Y, etc..
        float data[12];
    };

    // "For programming purposes, OpenGL matrices are 16-value arrays with base vectors laid out contiguously in memory.
    // The translation components occupy the 13th, 14th, and 15th elements of the 16-element matrix."
    // https://www.khronos.org/opengl/wiki/General_OpenGL:_Transformations#Are_OpenGL_matrices_column-major_or_row-major.3F
    // this is also the same as with glm.
    /** Subscript operator */
    constexpr Vec3& operator[](size_t aIndex)
    {
        return base[aIndex];
    }

    /** Subscript operator */
    constexpr const Vec3& operator[](size_t aIndex) const
    {
        return base[aIndex];
    }

    // Constructors
    /** Zero initializer constructor */
    inline constexpr Mat4X3() : data { 0 } {}

    /** Constructor for Vector4's */
    inline constexpr Mat4X3(Vec3 const& v0, Vec3 const& v1, Vec3 const& v2, Vec3 const& v3) : x(v0), y(v1), z(v2), w(v3)
    {}

    /** Constructor for array of floats */
    inline constexpr Mat4X3(const float d[12])
        : data { d[0], d[1], d[2], d[3], d[4], d[5], d[6], d[7], d[8], d[9], d[10], d[11] }
    {}

    /** Constructor for floats */
    inline constexpr Mat4X3(float d0, float d1, float d2, float d3, float d4, float d5, float d6, float d7, float d8,
        float d9, float d10, float d11)
        : data { d0, d1, d2, d3, d4, d5, d6, d7, d8, d9, d10, d11 }
    {}

    /** Identity constructor */
    inline explicit constexpr Mat4X3(float id)
        : data { id, 0.0f, 0.0f, 0.0f, id, 0.0f, 0.0f, 0.0f, id, 0.0f, 0.0f, 0.0f }
    {}

    /** Multiply columns by float scalar value */
    inline constexpr Mat4X3 operator*(const float& scalar) const
    {
        return Mat4X3(x * scalar, y * scalar, z * scalar, w * scalar);
    }

    /** Equality operator, returns true if matrices are equal */
    inline constexpr bool operator==(const Mat4X3& mat) const
    {
        for (size_t i = 0; i < countof(data); ++i) {
            if (data[i] != mat.data[i]) {
                return false;
            }
        }
        return true;
    }

    /** Inequality operator, returns true if matrices are inequal */
    inline constexpr bool operator!=(const Mat4X3& mat) const
    {
        for (size_t i = 0; i < countof(data); ++i) {
            if (data[i] != mat.data[i]) {
                return true;
            }
        }
        return false;
    }
};

// Assert that Mat4X4 is the same as 12 floats
static_assert(sizeof(Mat4X3) == 12 * sizeof(float));

static constexpr Mat4X3 IDENTITY_4X3(1.f);

#include <base/math/disable_warning_4201_footer.h>
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_MATRIX_H
