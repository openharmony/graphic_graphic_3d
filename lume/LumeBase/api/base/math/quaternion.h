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

#ifndef API_BASE_MATH_QUATERNION_H
#define API_BASE_MATH_QUATERNION_H

#include <cmath>
#include <cstddef>

#include <base/math/mathf.h>
#include <base/namespace.h>

BASE_BEGIN_NAMESPACE()
namespace Math {
#include <base/math/disable_warning_4201_heading.h>

/** @ingroup group_math_quaternion */
/** Quaternion */
class Quat final {
public:
    union {
        struct {
            float x;
            float y;
            float z;
            float w;
        };
        float data[4];
    };

    /** Subscript operator */
    constexpr float& operator[](size_t aIndex)
    {
        return data[aIndex];
    }

    /** Subscript operator */
    constexpr const float& operator[](size_t aIndex) const
    {
        return data[aIndex];
    }

    // Constructors
    /** Default constructor */
    inline constexpr Quat() noexcept : data {} {}

    /** Constructor for floats */
    inline constexpr Quat(float xParameter, float yParameter, float zParameter, float wParameter) noexcept
        : x(xParameter), y(yParameter), z(zParameter), w(wParameter)
    {}

    /** Constructor for float array */
    inline constexpr Quat(const float d[]) noexcept : x(d[0]), y(d[1]), z(d[2]), w(d[3]) {}

    // Quaternion to quaternion operations
    /** Multiply quaternion by quaternion */
    inline constexpr Quat operator*(const Quat& quat) const
    {
        return Quat(w * quat.x + x * quat.w + y * quat.z - z * quat.y,
            w * quat.y + y * quat.w + z * quat.x - x * quat.z, w * quat.z + z * quat.w + x * quat.y - y * quat.x,
            w * quat.w - x * quat.x - y * quat.y - z * quat.z);
    } // Add

    inline ~Quat() = default;

    /** Divide quaternion by float */
    inline constexpr Quat operator/(float d) const
    {
        return Quat(x / d, y / d, z / d, w / d);
    }

    /** Divide quaternion by float */
    inline constexpr Quat& operator/=(float d)
    {
        if (d != 0.f) {
            x /= d;
            y /= d;
            z /= d;
            w /= d;
        } else {
            x = y = z = w = HUGE_VALF;
        }
        return *this;
    }

    // Returns true if the quaternions are equal.
    /** Equality operator */
    inline constexpr bool operator==(const Quat& rhs) const
    {
        auto const temp = Quat(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
        const float sqmgt = temp.x * temp.x + temp.y * temp.y + temp.z * temp.z + temp.w * temp.w;

        // Returns false in the presence of NaN values
        return sqmgt < Math::EPSILON * Math::EPSILON;
    }

    // Returns true if quaternions are different.
    /** Inequality operator */
    inline constexpr bool operator!=(const Quat& rhs) const
    {
        // Returns true in the presence of NaN values
        return !(*this == rhs);
    }
};

// Assert that Quat is the same as 4 floats
static_assert(sizeof(Quat) == 4 * sizeof(float));

#include <base/math/disable_warning_4201_footer.h>
} // namespace Math
BASE_END_NAMESPACE()

#endif // API_BASE_MATH_QUATERNION_H
