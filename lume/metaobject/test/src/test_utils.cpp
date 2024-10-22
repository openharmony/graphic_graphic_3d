/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <iomanip>
#include <sstream>

#include <base/containers/string.h>

#include "test_utils.h"

namespace {
    // The width of the output field
    const int TWO = 2;

    // Insert '-' separator at a specific location
    const int THREE = 3;
    const int FIVE = 5;
    const int SEVEN = 7;
    const int NINE = 9;
    
    // Floating-point precision
    const int SIX = 6;
}

namespace BASE_NS {
std::ostream& operator<<(std::ostream& os, const BASE_NS::Uid& uid)
{
    std::stringstream ss;
    ss << "{" << std::setfill('0') << std::setw(TWO) << std::hex;
    for (size_t i = 0; i < sizeof(uid.data); i++) {
        ss << static_cast<uint16_t>(uid.data[i]);
        if (i == THREE || i == FIVE || i == SEVEN || i == NINE) {
            ss << "-";
        }
    }
    ss << "}";
    return os << ss.str();
}

std::ostream& operator<<(std::ostream& os, const BASE_NS::string& s)
{
    return os << s.c_str();
}

std::ostream& operator<<(std::ostream& os, const BASE_NS::string_view& s)
{
    return os << s.data();
}

namespace Math {
bool operator>(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x > rhs.x) && (lhs.y > rhs.y));
}

bool operator<(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x < rhs.x) && (lhs.y < rhs.y));
}

bool operator>(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x > rhs.x) && (lhs.y > rhs.y));
}

bool operator<(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x < rhs.x) && (lhs.y < rhs.y));
}

bool operator>=(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x >= rhs.x) && (lhs.y >= rhs.y));
}

bool operator<=(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return ((lhs.x <= rhs.x) && (lhs.y <= rhs.y));
}

bool operator>=(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x >= rhs.x) && (lhs.y >= rhs.y));
}

bool operator<=(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return ((lhs.x <= rhs.x) && (lhs.y <= rhs.y));
}

bool AreEqual(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs, float epsilon)
{
    return BASE_NS::Math::abs(lhs.x - rhs.x) < epsilon && BASE_NS::Math::abs(lhs.y - rhs.y) < epsilon &&
           BASE_NS::Math::abs(lhs.z - rhs.z) < epsilon;
}

bool AreNear(const BASE_NS::Math::Vec3& lhs, const BASE_NS::Math::Vec3& rhs)
{
    return AreEqual(lhs, rhs, 0.01f);
}

bool AreEqual(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs, float epsilon)
{
    return BASE_NS::Math::abs(lhs.x - rhs.x) < epsilon && BASE_NS::Math::abs(lhs.y - rhs.y) < epsilon;
}

bool AreNear(const BASE_NS::Math::Vec2& lhs, const BASE_NS::Math::Vec2& rhs)
{
    return AreEqual(lhs, rhs, 0.01f);
}

std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec3& vec)
{
    std::stringstream ss;
    ss << std::setprecision(SIX) << "BASE_NS::Math::Vec3 {" << vec.x << ", " << vec.y << ", " << vec.z << "}";
    return os << ss.str();
}
std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec2& vec)
{
    std::stringstream ss;
    ss << std::setprecision(SIX) << "BASE_NS::Math::Vec2 {" << vec.x << ", " << vec.y << "}";
    return os << ss.str();
}

} // namespace Math
} // namespace BASE_NS

namespace META_NS {

std::ostream& operator<<(std::ostream& os, const META_NS::TimeSpan& ts)
{
    return os << ts.ToMicroseconds() << "us";
}

} // namespace META_NS