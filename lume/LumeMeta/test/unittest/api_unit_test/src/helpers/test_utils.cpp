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

#include "helpers/test_utils.h"

#include <iomanip>
#include <sstream>

#include <base/containers/string.h>

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

namespace BASE_NS {

std::ostream& operator<<(std::ostream& os, const BASE_NS::Uid& uid)
{
    std::stringstream ss;

    ss << "{" << std::setfill('0') << std::setw(2) << std::hex;
    for (size_t i = 0; i < sizeof(uid.data); i++) {
        ss << static_cast<uint16_t>(uid.data[i]);
        if (i == 3 || i == 5 || i == 7 || i == 9) {
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

std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec3& vec)
{
    std::stringstream ss;
    ss << std::setprecision(6) << "BASE_NS::Math::Vec3 {" << vec.x << ", " << vec.y << ", " << vec.z << "}";
    return os << ss.str();
}

std::ostream& operator<<(std::ostream& os, const BASE_NS::Math::Vec2& vec)
{
    std::stringstream ss;
    ss << std::setprecision(6) << "BASE_NS::Math::Vec2 {" << vec.x << ", " << vec.y << "}";
    return os << ss.str();
}

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

} // namespace Math
} // namespace BASE_NS

namespace META_NS {
std::ostream& operator<<(std::ostream& os, const META_NS::TimeSpan& ts)
{
    return os << ts.ToMicroseconds() << "us";
}

} // namespace META_NS
