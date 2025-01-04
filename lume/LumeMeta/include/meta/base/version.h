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
#ifndef META_BASE_VERSION_H
#define META_BASE_VERSION_H

#include <base/containers/string.h>
#include <base/util/uid_util.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

class Version {
public:
    constexpr Version(uint16_t major = 0, uint16_t minor = 0) : major_(major), minor_(minor) {}

    constexpr explicit Version(BASE_NS::string_view str)
    {
        size_t pos = ParseInt(str, major_);
        if (pos != 0 && pos + 1 < str.size()) {
            ParseInt(str.substr(pos + 1), minor_);
        }
    }

    constexpr uint16_t Major() const
    {
        return major_;
    }
    constexpr uint16_t Minor() const
    {
        return minor_;
    }

    BASE_NS::string ToString() const
    {
        return BASE_NS::string(BASE_NS::to_string(major_)) + "." + BASE_NS::string(BASE_NS::to_string(minor_));
    }

private:
    constexpr size_t ParseInt(BASE_NS::string_view str, uint16_t& out)
    {
        size_t i = 0;
        while (i < str.size() && str[i] >= '0' && str[i] <= '9') {
            if (i) {
                out *= 10;
            }
            out += str[i++] - '0';
        }
        return i;
    }

private:
    uint16_t major_ {};
    uint16_t minor_ {};
};

constexpr inline bool operator==(const Version& l, const Version& r)
{
    return l.Major() == r.Major() && l.Minor() == r.Minor();
}

constexpr inline bool operator!=(const Version& l, const Version& r)
{
    return !(l == r);
}

constexpr inline bool operator<(const Version& l, const Version& r)
{
    return l.Major() < r.Major() || (l.Major() == r.Major() && l.Minor() < r.Minor());
}

constexpr inline bool operator<=(const Version& l, const Version& r)
{
    return l == r || l < r;
}

constexpr inline bool operator>(const Version& l, const Version& r)
{
    return r < l;
}

constexpr inline bool operator>=(const Version& l, const Version& r)
{
    return r <= l;
}

META_END_NAMESPACE()

#endif
