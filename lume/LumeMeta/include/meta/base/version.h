/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * Description: Simple version type
 * Author: Mikael Kilpeläinen
 */
#ifndef META_BASE_VERSION_H
#define META_BASE_VERSION_H

#include <base/containers/string.h>
#include <base/util/uid_util.h>

#include <meta/base/namespace.h>

META_BEGIN_NAMESPACE()

/// Simple version class which supports major and minor version number
class Version {
public:
    /// Construct Versions from major and minor versions
    constexpr Version(uint16_t major = 0, uint16_t minor = 0) : major_(major), minor_(minor) {}

    /// Construct Version from string
    constexpr explicit Version(BASE_NS::string_view str)
    {
        size_t pos = ParseInt(str, major_);
        if (pos != 0 && pos + 1 < str.size()) {
            ParseInt(str.substr(pos + 1), minor_);
        }
    }

    /// Get major version
    constexpr uint16_t Major() const
    {
        return major_;
    }
    /// Get minor version
    constexpr uint16_t Minor() const
    {
        return minor_;
    }
    /// Get version as string
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
