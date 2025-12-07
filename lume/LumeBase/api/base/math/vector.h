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

#ifndef API_BASE_MATH_VECTOR_H
#define API_BASE_MATH_VECTOR_H

#include <cstddef>
#include <cstdint>
#if defined(BASE_SIMD) && defined(_M_X64)
#include <immintrin.h>
#elif defined(_M_ARM64) || defined(__ARM_ARCH_ISA_A64)
#include <arm_neon.h>
#endif
#include <base/math/mathf.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
#include <base/math/disable_warning_4201_heading.h>

class Vec2;
class Vec3;
class Vec4;

/** @ingroup group_math_vector */
/** Vector2 presentation */
class Vec2 final {
public:
    union {
        struct {
            float x;
            float y;
        };
        float data[2];
    };
    /** Subscript operator */
    constexpr float& operator[](size_t index)
    {
        return data[index];
    }

    /** Subscript operator */
    constexpr const float& operator[](size_t index) const
    {
        return data[index];
    }

    // Constructors
    /** Default constructor */
    inline constexpr Vec2() noexcept : data {} {}
    /** Constructor for using single float as input for all components, marked explicit to avoid implicit conversion
     * from float to vector. For example Normalize(1.0f) would be illegal. */
    inline constexpr explicit Vec2(float value) noexcept : x(value), y(value) {}
    /** Constructor for using floats as input */
    inline constexpr Vec2(float xParameter, float yParameter) noexcept : x(xParameter), y(yParameter) {}
    /** Constructor for using array of floats as input */
    inline constexpr Vec2(const float parameter[2]) noexcept : x(parameter[0]), y(parameter[1]) {}

    /** Add operator */
    inline constexpr Vec2 operator+(const Vec2& v2) const
    {
        return Vec2(x + v2.x, y + v2.y);
    }
    /** Add operator */
    inline constexpr Vec2& operator+=(const Vec2& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    /** Negate operator */
    inline constexpr Vec2 operator-() const
    {
        return Vec2(-x, -y);
    }

    /** Subtract operator */
    inline constexpr Vec2 operator-(const Vec2& v2) const
    {
        return Vec2(x - v2.x, y - v2.y);
    }
    /** Subtract operator */
    inline constexpr Vec2& operator-=(const Vec2& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    /** Multiply operator */
    inline constexpr Vec2 operator*(const Vec2& v2) const
    {
        return Vec2(x * v2.x, y * v2.y);
    }
    /** Multiply operator */
    inline constexpr Vec2& operator*=(const Vec2& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    /** Divide operator */
    inline constexpr Vec2 operator/(const Vec2& v2) const
    {
        return Vec2(x / v2.x, y / v2.y);
    }
    /** Divide operator */
    inline constexpr Vec2& operator/=(const Vec2& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    /** Multiplies vector by float value */
    inline constexpr Vec2 operator*(float d) const
    {
        return Vec2(x * d, y * d);
    }
    /** Multiplies vector by float value */
    friend inline constexpr Vec2 operator*(float d, const Vec2& v2)
    {
        return v2 * d;
    }
    /** Multiplies vector by float value */
    inline constexpr Vec2& operator*=(float d)
    {
        x *= d;
        y *= d;
        return *this;
    }

    /** Divides vector by float value */
    inline constexpr Vec2 operator/(float d) const
    {
        return Vec2(x / d, y / d);
    }
    /** Divides vector by float value */
    inline constexpr Vec2& operator/=(float d)
    {
        x /= d;
        y /= d;
        return *this;
    }

    /** Add float from vector */
    inline constexpr Vec2 operator+(float d) const
    {
        return Vec2(x + d, y + d);
    }

    /** Add float from vector */
    inline constexpr Vec2& operator+=(float d)
    {
        x += d;
        y += d;
        return *this;
    }

    /** Subtract float from vector */
    inline constexpr Vec2 operator-(float d) const
    {
        return Vec2(x - d, y - d);
    }
    /** Subtract float from vector */
    inline constexpr Vec2& operator-=(float d)
    {
        x -= d;
        y -= d;
        return *this;
    }

    // Equality operators
    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const Vec2& rhs) const
    {
        const Vec2 temp = *this - rhs;
        const float sqmgt = temp.x * temp.x + temp.y * temp.y;

        // Returns false in the presence of NaN values
        return sqmgt < Math::EPSILON * Math::EPSILON;
    }

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const Vec2& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that Vec2 is the same as 2 floats
static_assert(sizeof(Vec2) == 2 * sizeof(float));

static constexpr Vec2 ZERO_VEC2(0.0f, 0.0f);

/** @ingroup group_math_vector */
/** Vector3 presentation */
class Vec3 final {
public:
    union {
        struct {
            float x;
            float y;
            float z;
        };
        float data[3];
    };
    /** Subscript operator */
    constexpr float& operator[](size_t index);
    /** Subscript operator */
    constexpr const float& operator[](size_t index) const;

    // Constructors
    /** Default constructor */
    inline constexpr Vec3() noexcept;
    /** Constructor for using single float as input for all components, marked explicit to avoid implicit conversion
     * from float to vector. For example Normalize(1.0f) would be illegal. */
    inline constexpr explicit Vec3(float value) noexcept;
    /** Constructor for using floats as input */
    inline constexpr Vec3(float xParameter, float yParameter, float zParameter) noexcept;
    /** Constructor for using array of floats as input */
    inline constexpr Vec3(const float d[]) noexcept;
    /** Constructor for using vector2 and float as input */
    inline constexpr Vec3(const Vec2& vec, float zParameter) noexcept;
    /** Constructor for using vector4 as input, discards w component */
    inline constexpr Vec3(const Vec4& vec) noexcept;

    // Vec3 to Vec3 operations
    /** Add operator */
    inline constexpr Vec3 operator+(const Vec3& v2) const;
    /** Add operator */
    inline constexpr Vec3& operator+=(const Vec3& rhs);

    /** Negate operator */
    inline constexpr Vec3 operator-() const;

    /** Subtract operator */
    inline constexpr Vec3 operator-(const Vec3& v2) const;
    /** Subtract operator */
    inline constexpr Vec3& operator-=(const Vec3& rhs);

    /** Multiply operator */
    inline constexpr Vec3 operator*(const Vec3& v2) const;
    /** Multiply operator */
    inline constexpr Vec3& operator*=(const Vec3& rhs);

    /** Divide operator */
    inline constexpr Vec3 operator/(const Vec3& v2) const;
    /** Divide operator */
    inline constexpr Vec3& operator/=(const Vec3& rhs);

    // Equality operators
    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const Vec3& rhs) const;

    /** Inequality operator, returns true if vectors are inequal */
    constexpr bool operator!=(const Vec3& rhs) const;

    // Vec3 to float operations

    /** Add operator */
    inline constexpr Vec3 operator+(float d) const;
    /** Add operator */
    inline constexpr Vec3& operator+=(float d);

    /** Subtract operator */
    inline constexpr Vec3 operator-(float d) const;
    /** Subtract operator */
    inline constexpr Vec3& operator-=(float d);

    /** Multiplies vector by float value */
    inline constexpr Vec3 operator*(float d) const;
    /** Multiplies vector by float value */
    friend inline constexpr Vec3 operator*(float d, const Vec3& v2);
    /** Multiplies vector by float value */
    inline constexpr Vec3& operator*=(float d);

    /** Divides vector by float value */
    inline constexpr Vec3 operator/(float d) const;
    /** Divides vector by float value */
    inline constexpr Vec3& operator/=(float d);
};

// Assert that Vec3 is the same as 3 floats
static_assert(sizeof(Vec3) == 3 * sizeof(float));

/** @ingroup group_math_vector */
/** Vector4 presentation */
class Vec4 final {
public:
    union {
        struct {
            float x;
            float y;
            float z;
            float w;
        };
        float data[4];
#if defined(BASE_SIMD) && defined(_M_X64)
        __m128 vec4;
#elif defined(_M_ARM64) || defined(__ARM_ARCH_ISA_A64)
        float32x4_t vec4;
#endif
    };
    /** Subscript operator */
    constexpr float& operator[](size_t index);
    /** Subscript operator */
    constexpr const float& operator[](size_t index) const;

    // Constructors
    /** Default constructor */
    inline constexpr Vec4() noexcept;
    /** Constructor for using single float as input for all components, marked explicit to avoid implicit conversion
     * from float to vector. For example Normalize(1.0f) would be illegal. */
    inline constexpr explicit Vec4(float value) noexcept;
    /** Constructor for using floats as input */
    inline constexpr Vec4(float xParameter, float yParameter, float zParameter, float wParameter) noexcept;
    /** Constructor for using array of floats as input */
    inline constexpr Vec4(const float d[4]) noexcept;
    /** Constructor for using vector2 and 2 float as input */
    inline constexpr Vec4(const Vec2& vec, float zParameter, float wParameter) noexcept;
    /** Constructor for using vector3 and float as input (float as w component) */
    inline constexpr Vec4(const Vec3& vec, float w) noexcept;

    /** Add operator */
    inline constexpr Vec4 operator+(const Vec4& v2) const;
    /** Add operator */
    inline constexpr Vec4& operator+=(const Vec4& rhs);

    /** Negate operator */
    inline constexpr Vec4 operator-() const;

    /** Subtract operator */
    inline constexpr Vec4 operator-(const Vec4& v2) const;
    /** Subtract operator */
    inline constexpr Vec4& operator-=(const Vec4& rhs);

    /** Multiply operator */
    inline constexpr Vec4 operator*(const Vec4& v2) const;
    /** Multiply operator */
    inline constexpr Vec4& operator*=(const Vec4& rhs);

    /** Divide operator */
    inline constexpr Vec4 operator/(const Vec4& v2) const;
    /** Divide operator */
    inline constexpr Vec4& operator/=(const Vec4& rhs);

    /** Add operator */
    inline constexpr Vec4 operator+(float d) const;
    /** Add operator */
    inline constexpr Vec4& operator+=(float d);

    /** Subtract operator */
    inline constexpr Vec4 operator-(float d) const;
    /** Subtract operator */
    inline constexpr Vec4& operator-=(float d);

    /** Multiplies a vector by a float value */
    inline constexpr Vec4 operator*(float d) const;
    /** Multiplies vector by float value */
    friend inline constexpr Vec4 operator*(float d, const Vec4& v2);
    /** Multiplies a vector by a float value */
    inline constexpr Vec4& operator*=(float d);

    /** Divides a vector by a float value */
    inline constexpr Vec4 operator/(float d) const;
    /** Divides a vector by a float value */
    inline constexpr Vec4& operator/=(float d);

    // Equality operators
    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const Vec4& rhs) const;

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const Vec4& rhs) const;
};

// Assert that Vec4 is the same as 4 floats
static_assert(sizeof(Vec4) == 4 * sizeof(float));

/** @ingroup group_math_vector */
/** Unsigned integer vector2 presentation */
class UVec2 final {
public:
    union {
        struct {
            uint32_t x;
            uint32_t y;
        };
        uint32_t data[2];
    };

    /** Subscript operator */
    constexpr uint32_t& operator[](size_t index)
    {
        return data[index];
    }
    /** Subscript operator */
    constexpr const uint32_t& operator[](size_t index) const
    {
        return data[index];
    }

    // Constructors
    /** Default constructor */
    inline constexpr UVec2() : data {} {}
    /** Constructor for using two uint32_t's as input */
    inline constexpr UVec2(uint32_t xParameter, uint32_t yParameter) : x(xParameter), y(yParameter) {}

    /** Add operator */
    inline constexpr UVec2 operator+(const UVec2& v2) const
    {
        return UVec2(x + v2.x, y + v2.y);
    }
    /** Add operator */
    inline constexpr UVec2& operator+=(const UVec2& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    /** Subtract operator */
    inline constexpr UVec2 operator-(const UVec2& v2) const
    {
        return UVec2(x - v2.x, y - v2.y);
    }
    /** Subtract operator */
    inline constexpr UVec2& operator-=(const UVec2& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    /** Multiply operator */
    inline constexpr UVec2 operator*(const UVec2& v2) const
    {
        return UVec2(x * v2.x, y * v2.y);
    }
    /** Multiply operator */
    inline constexpr UVec2& operator*=(const UVec2& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    /** Divide operator */
    inline constexpr UVec2 operator/(const UVec2& v2) const
    {
        return UVec2(x / v2.x, y / v2.y);
    }
    /** Divide operator */
    inline constexpr UVec2& operator/=(const UVec2& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    /** Multiplies vector by uint32_t value */
    inline constexpr UVec2 operator*(uint32_t d) const
    {
        return UVec2(x * d, y * d);
    }
    /** Multiplies vector by uint32_t value */
    friend inline constexpr UVec2 operator*(uint32_t d, const UVec2& v2)
    {
        return v2 * d;
    }
    /** Multiplies vector by uint32_t value */
    inline constexpr UVec2& operator*=(uint32_t d)
    {
        x *= d;
        y *= d;
        return *this;
    }

    /** Divides vector by uint32_t value */
    inline constexpr UVec2 operator/(uint32_t d) const
    {
        return UVec2(x / d, y / d);
    }
    /** Divides vector by uint32_t value */
    inline constexpr UVec2& operator/=(uint32_t d)
    {
        if (d == 0) {
            x = y = UINT32_MAX;
            return *this;
        }

        x /= d;
        y /= d;
        return *this;
    }

    /** Add uint to uvector2 */
    inline constexpr UVec2 operator+(uint32_t d) const
    {
        return UVec2(x + d, y + d);
    }
    /** Add uint to uvector2 */
    inline constexpr UVec2& operator+=(uint32_t d)
    {
        x += d;
        y += d;
        return *this;
    }

    /** Subtract uint32_t from uvector2 */
    inline constexpr UVec2 operator-(uint32_t d) const
    {
        return UVec2(x - d, y - d);
    }
    /** Subtract uint32_t from uvector2 */
    inline constexpr UVec2& operator-=(uint32_t d)
    {
        x -= d;
        y -= d;
        return *this;
    }

    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const UVec2& rhs) const
    {
        if (x != rhs.x) {
            return false;
        }
        if (y != rhs.y) {
            return false;
        }
        return true;
    }

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const UVec2& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that UVec2 is the same as 2 uint32_t's
static_assert(sizeof(UVec2) == 2 * sizeof(uint32_t));

static constexpr UVec2 ZERO_UVEC2(0, 0);

/** @ingroup group_math_vector */
/** Unsigned integer vector3 presentation */
class UVec3 {
public:
    union {
        struct {
            uint32_t x;
            uint32_t y;
            uint32_t z;
        };
        uint32_t data[3];
    };

    // Constructors
    /** Default constructor */
    inline constexpr UVec3() : data {} {}
    /** Constructor for using three uint32_t's as input */
    inline constexpr UVec3(uint32_t x, uint32_t y, uint32_t z) : x(x), y(y), z(z) {}

    /** Subscript operator */
    constexpr uint32_t& operator[](size_t index)
    {
        return data[index];
    }

    /** Subscript operator */
    constexpr const uint32_t& operator[](size_t index) const
    {
        return data[index];
    }

    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const UVec3& rhs) const
    {
        if (x != rhs.x) {
            return false;
        }
        if (y != rhs.y) {
            return false;
        }
        if (z != rhs.z) {
            return false;
        }
        return true;
    }

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const UVec3& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that UVec3 is the same as 3 uint32_t's
static_assert(sizeof(UVec3) == 3 * sizeof(uint32_t));

/** @ingroup group_math_vector */
/** Unsigned integer vector4 presentation */
class UVec4 {
public:
    union {
        struct {
            uint32_t x;
            uint32_t y;
            uint32_t z;
            uint32_t w;
        };
        uint32_t data[4];
    };
    // Constructors
    /** Default constructor */
    inline constexpr UVec4() : data {} {}
    /** Constructor for using four uint32_t's as input */
    inline constexpr UVec4(uint32_t x, uint32_t y, uint32_t z, uint32_t w) : x(x), y(y), z(z), w(w) {}

    /** Subscript operator */
    constexpr uint32_t& operator[](size_t index)
    {
        return data[index];
    }

    /** Subscript operator */
    constexpr const uint32_t& operator[](size_t index) const
    {
        return data[index];
    }

    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const UVec4& rhs) const
    {
        if (x != rhs.x) {
            return false;
        }
        if (y != rhs.y) {
            return false;
        }
        if (z != rhs.z) {
            return false;
        }
        if (w != rhs.w) {
            return false;
        }
        return true;
    }

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const UVec4& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that UVec4 is the same as 4 uint32_t's
static_assert(sizeof(UVec4) == 4 * sizeof(uint32_t));

/** @ingroup group_math_vector */
/** Signed integer vector2 presentation */
class IVec2 final {
public:
    union {
        struct {
            int32_t x;
            int32_t y;
        };
        int32_t data[2];
    };

    /** Subscript operator */
    constexpr int32_t& operator[](size_t index)
    {
        return data[index];
    }
    /** Subscript operator */
    constexpr const int32_t& operator[](size_t index) const
    {
        return data[index];
    }

    // Constructors
    /** Default constructor */
    inline constexpr IVec2() : data {} {}
    /** Constructor for using two int32_t's as input */
    inline constexpr IVec2(int32_t xParameter, int32_t yParameter) : x(xParameter), y(yParameter) {}
    ~IVec2() = default;

    /** Add operator */
    inline constexpr IVec2 operator+(const IVec2& v2) const
    {
        return IVec2(x + v2.x, y + v2.y);
    }
    /** Add operator */
    inline constexpr IVec2& operator+=(const IVec2& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }

    /** Subtract operator */
    inline constexpr IVec2 operator-(const IVec2& v2) const
    {
        return IVec2(x - v2.x, y - v2.y);
    }
    /** Subtract operator */
    inline constexpr IVec2& operator-=(const IVec2& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }

    /** Multiply operator */
    inline constexpr IVec2 operator*(const IVec2& v2) const
    {
        return IVec2(x * v2.x, y * v2.y);
    }
    /** Multiply operator */
    inline constexpr IVec2& operator*=(const IVec2& rhs)
    {
        x *= rhs.x;
        y *= rhs.y;
        return *this;
    }

    /** Divide operator */
    inline constexpr IVec2 operator/(const IVec2& v2) const
    {
        return IVec2(x / v2.x, y / v2.y);
    }
    /** Divide operator */
    inline constexpr IVec2& operator/=(const IVec2& rhs)
    {
        x /= rhs.x;
        y /= rhs.y;
        return *this;
    }

    /** Multiplies vector by int value */
    inline constexpr IVec2 operator*(int32_t d) const
    {
        return IVec2(x * d, y * d);
    }
    /** Multiplies vector by int value */
    friend inline constexpr IVec2 operator*(int32_t d, const IVec2& v2)
    {
        return v2 * d;
    }
    /** Multiplies vector by float value */
    inline constexpr IVec2& operator*=(int32_t d)
    {
        x *= d;
        y *= d;
        return *this;
    }

    /** Divides vector by float value */
    inline constexpr IVec2 operator/(int32_t d) const
    {
        return IVec2(x / d, y / d);
    }
    /** Divides vector by float value */
    inline constexpr IVec2& operator/=(int32_t d)
    {
        if (d == 0) {
            x = y = INT32_MAX;
            return *this;
        }

        x /= d;
        y /= d;
        return *this;
    }

    /** Add int32_t to IVector2 */
    inline constexpr IVec2 operator+(int32_t d) const
    {
        return IVec2(x + d, y + d);
    }
    /** Add int32_t to IVector2 */
    inline constexpr IVec2& operator+=(int32_t d)
    {
        x += d;
        y += d;
        return *this;
    }

    /** Subtract int32_t from IVector2 */
    inline constexpr IVec2 operator-(int32_t d) const
    {
        return IVec2(x - d, y - d);
    }
    /** Subtract int32_t from IVector2 */
    inline constexpr IVec2& operator-=(int32_t d)
    {
        x -= d;
        y -= d;
        return *this;
    }

    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const IVec2& rhs) const
    {
        if (x != rhs.x) {
            return false;
        }
        if (y != rhs.y) {
            return false;
        }
        return true;
    }

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const IVec2& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that IVec2 is the same as 2 int32_t's
static_assert(sizeof(IVec2) == 2 * sizeof(int32_t));

/** @ingroup group_math_vector */
/** Signed integer vector3 presentation */
class IVec3 {
public:
    union {
        struct {
            int32_t x;
            int32_t y;
            int32_t z;
        };
        int32_t data[3];
    };

    // Constructors
    /** Default constructor */
    inline constexpr IVec3() : data {} {}
    /** Constructor for using three int32_t's as input */
    inline constexpr IVec3(int32_t x, int32_t y, int32_t z) : x(x), y(y), z(z) {}
    ~IVec3() = default;

    /** Subscript operator */
    constexpr int32_t& operator[](size_t index)
    {
        return data[index];
    }

    /** Subscript operator */
    constexpr const int32_t& operator[](size_t index) const
    {
        return data[index];
    }

    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const IVec3& rhs) const
    {
        if (x != rhs.x) {
            return false;
        }
        if (y != rhs.y) {
            return false;
        }
        if (z != rhs.z) {
            return false;
        }
        return true;
    }

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const IVec3& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that IVec3 is the same as 3 int32_t's
static_assert(sizeof(IVec3) == 3 * sizeof(int32_t));

/** @ingroup group_math_vector */
/** Signed integer vector4 presentation */
class IVec4 {
public:
    union {
        struct {
            int32_t x;
            int32_t y;
            int32_t z;
            int32_t w;
        };
        int32_t data[4];
    };
    // Constructors
    /** Default constructor */
    inline constexpr IVec4() : data {} {}
    /** Constructor for using four int32_t's as input */
    inline constexpr IVec4(int32_t x, int32_t y, int32_t z, int32_t w) : x(x), y(y), z(z), w(w) {}
    ~IVec4() = default;

    /** Subscript operator */
    constexpr int32_t& operator[](size_t index)
    {
        return data[index];
    }

    /** Subscript operator */
    constexpr const int32_t& operator[](size_t index) const
    {
        return data[index];
    }

    /** Equality operator, returns true if the vectors are equal */
    constexpr bool operator==(const IVec4& rhs) const
    {
        if (x != rhs.x) {
            return false;
        }
        if (y != rhs.y) {
            return false;
        }
        if (z != rhs.z) {
            return false;
        }
        if (w != rhs.w) {
            return false;
        }
        return true;
    }

    /** Inequality operator, returns true if vectors are different */
    constexpr bool operator!=(const IVec4& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that IVec4 is the same as 4 int32_t's
static_assert(sizeof(IVec4) == 4 * sizeof(int32_t));

constexpr float& Vec3::operator[](size_t index)
{
    return data[index];
}
constexpr const float& Vec3::operator[](size_t index) const
{
    return data[index];
}

// Constructors
inline constexpr Vec3::Vec3() noexcept : data {} {}
inline constexpr Vec3::Vec3(float value) noexcept : x(value), y(value), z(value) {}
inline constexpr Vec3::Vec3(float xParameter, float yParameter, float zParameter) noexcept
    : x(xParameter), y(yParameter), z(zParameter)
{}
inline constexpr Vec3::Vec3(const float d[]) noexcept : x(d[0]), y(d[1]), z(d[2]) {}
inline constexpr Vec3::Vec3(const Vec2& vec, float zParameter) noexcept : x(vec.x), y(vec.y), z(zParameter) {}
inline constexpr Vec3::Vec3(const Vec4& vec) noexcept : x(vec.x), y(vec.y), z(vec.z) {}

// Vec3 to Vec3 operations
// Add
inline constexpr Vec3 Vec3::operator+(const Vec3& v2) const
{
    return Vec3(x + v2.x, y + v2.y, z + v2.z);
}
// Add
inline constexpr Vec3& Vec3::operator+=(const Vec3& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    return *this;
}

// Negate
inline constexpr Vec3 Vec3::operator-() const
{
    return Vec3(-x, -y, -z);
}

// Subtract
inline constexpr Vec3 Vec3::operator-(const Vec3& v2) const
{
    return Vec3(x - v2.x, y - v2.y, z - v2.z);
}
// Subtract
inline constexpr Vec3& Vec3::operator-=(const Vec3& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    return *this;
}

// Multiply
inline constexpr Vec3 Vec3::operator*(const Vec3& v2) const
{
    return Vec3(x * v2.x, y * v2.y, z * v2.z);
}
// Multiply
inline constexpr Vec3& Vec3::operator*=(const Vec3& rhs)
{
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    return *this;
}

// Divide
inline constexpr Vec3 Vec3::operator/(const Vec3& v2) const
{
    return Vec3(x / v2.x, y / v2.y, z / v2.z);
}
// Divide
inline constexpr Vec3& Vec3::operator/=(const Vec3& rhs)
{
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    return *this;
}

// Equality operators
// Returns true if the vectors are equal
constexpr bool Vec3::operator==(const Vec3& rhs) const
{
    const Vec3 temp = *this - rhs;
    const float sqmgt = temp.x * temp.x + temp.y * temp.y + temp.z * temp.z;

    // Returns false in the presence of NaN values
    return sqmgt < Math::EPSILON * Math::EPSILON;
}

// Returns true if vectors are different.
constexpr bool Vec3::operator!=(const Vec3& rhs) const
{
    // Returns true in the presence of NaN values
    return !(*this == rhs);
}

// Vec3 to float operations

// Add
inline constexpr Vec3 Vec3::operator+(float d) const
{
    return Vec3(x + d, y + d, z + d);
}
// Add
inline constexpr Vec3& Vec3::operator+=(float d)
{
    x += d;
    y += d;
    z += d;
    return *this;
}
// Subtract
inline constexpr Vec3 Vec3::operator-(float d) const
{
    return Vec3(x - d, y - d, z - d);
}
// Subtract
inline constexpr Vec3& Vec3::operator-=(float d)
{
    x -= d;
    y -= d;
    z -= d;
    return *this;
}

// Multiplies vector by float value
inline constexpr Vec3 Vec3::operator*(float d) const
{
    return Vec3(x * d, y * d, z * d);
}
constexpr Vec3 operator*(float d, const Vec3& v2)
{
    return v2 * d;
}
inline constexpr Vec3& Vec3::operator*=(float d)
{
    x *= d;
    y *= d;
    z *= d;
    return *this;
}

// Divides vector by float value
inline constexpr Vec3 Vec3::operator/(float d) const
{
    return Vec3(x / d, y / d, z / d);
}
inline constexpr Vec3& Vec3::operator/=(float d)
{
    x /= d;
    y /= d;
    z /= d;
    return *this;
}

constexpr float& Vec4::operator[](size_t index)
{
    return data[index];
}
constexpr const float& Vec4::operator[](size_t index) const
{
    return data[index];
}

static constexpr Vec3 ZERO_VEC3(0.0f, 0.0f, 0.0f);

// Constructors
inline constexpr Vec4::Vec4() noexcept : data {} {}
inline constexpr Vec4::Vec4(float value) noexcept : x(value), y(value), z(value), w(value) {}
inline constexpr Vec4::Vec4(float xParameter, float yParameter, float zParameter, float wParameter) noexcept
    : x(xParameter), y(yParameter), z(zParameter), w(wParameter)
{}
inline constexpr Vec4::Vec4(const float d[4]) noexcept : x(d[0]), y(d[1]), z(d[2]), w(d[3]) {}
inline constexpr Vec4::Vec4(const Vec3& vec, float w) noexcept : x(vec.x), y(vec.y), z(vec.z), w(w) {}
inline constexpr Vec4::Vec4(const Vec2& vec, float zParameter, float wParameter) noexcept
    : x(vec.x), y(vec.y), z(zParameter), w(wParameter)
{}

// Add
inline constexpr Vec4 Vec4::operator+(const Vec4& v2) const
{
    return Vec4(x + v2.x, y + v2.y, z + v2.z, w + v2.w);
}

inline constexpr Vec4 Vec4::operator+(float d) const
{
    return Vec4(x + d, y + d, z + d, w + d);
}

inline constexpr Vec4& Vec4::operator+=(const Vec4& rhs)
{
    x += rhs.x;
    y += rhs.y;
    z += rhs.z;
    w += rhs.w;
    return *this;
}

inline constexpr Vec4& Vec4::operator+=(float d)
{
    x += d;
    y += d;
    z += d;
    w += d;
    return *this;
}

// Negate
inline constexpr Vec4 Vec4::operator-() const
{
    return Vec4(-x, -y, -z, -w);
}

// Subtract
inline constexpr Vec4 Vec4::operator-(const Vec4& v2) const
{
    return Vec4(x - v2.x, y - v2.y, z - v2.z, w - v2.w);
}

inline constexpr Vec4 Vec4::operator-(float d) const
{
    return Vec4(x - d, y - d, z - d, w - d);
}

inline constexpr Vec4& Vec4::operator-=(const Vec4& rhs)
{
    x -= rhs.x;
    y -= rhs.y;
    z -= rhs.z;
    w -= rhs.w;
    return *this;
}

inline constexpr Vec4& Vec4::operator-=(float d)
{
    x -= d;
    y -= d;
    z -= d;
    w -= d;
    return *this;
}

// Multiply
inline constexpr Vec4 Vec4::operator*(const Vec4& v2) const
{
    return Vec4(x * v2.x, y * v2.y, z * v2.z, w * v2.w);
}

inline constexpr Vec4& Vec4::operator*=(const Vec4& rhs)
{
    x *= rhs.x;
    y *= rhs.y;
    z *= rhs.z;
    w *= rhs.w;
    return *this;
}

// Divide
inline constexpr Vec4 Vec4::operator/(const Vec4& v2) const
{
    return Vec4(x / v2.x, y / v2.y, z / v2.z, w / v2.w);
}

inline constexpr Vec4& Vec4::operator/=(const Vec4& rhs)
{
    x /= rhs.x;
    y /= rhs.y;
    z /= rhs.z;
    w /= rhs.w;
    return *this;
}

// Multiplies a vector by a float value
inline constexpr Vec4 Vec4::operator*(float d) const
{
    return Vec4(x * d, y * d, z * d, w * d);
}

constexpr Vec4 operator*(float d, const Vec4& v2)
{
    return v2 * d;
}

inline constexpr Vec4& Vec4::operator*=(float d)
{
    x *= d;
    y *= d;
    z *= d;
    w *= d;
    return *this;
}

// Divides a vector by a float value
inline constexpr Vec4 Vec4::operator/(float d) const
{
    return Vec4(x / d, y / d, z / d, w / d);
}
inline constexpr Vec4& Vec4::operator/=(float d)
{
    x /= d;
    y /= d;
    z /= d;
    w /= d;
    return *this;
}

// Equality operators
// Returns true if the vectors are equal.
constexpr bool Vec4::operator==(const Vec4& rhs) const
{
    const Vec4 temp = *this - rhs;
    const float sqmgt = temp.x * temp.x + temp.y * temp.y + temp.z * temp.z + temp.w * temp.w;

    // Returns false in the presence of NaN values
    return sqmgt < Math::EPSILON * Math::EPSILON;
}

// Returns true if vectors are different.
constexpr bool Vec4::operator!=(const Vec4& rhs) const
{
    // Returns true in the presence of NaN values
    return !(*this == rhs);
}

static constexpr Vec4 ZERO_VEC4(0.0f, 0.0f, 0.0f, 0.0f);

#include <base/math/disable_warning_4201_footer.h>
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_VECTOR_H
