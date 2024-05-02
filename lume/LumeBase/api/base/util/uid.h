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

#ifndef API_BASE_UTIL_UID_H
#define API_BASE_UTIL_UID_H

#include <cstddef>
#include <cstdint>

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/hash.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
constexpr uint8_t HexToDec(char c) noexcept
{
    if (c <= '9' && c >= '0') {
        return static_cast<uint8_t>(c - '0');
    }
    if (c <= 'F' && c >= 'A') {
        return static_cast<uint8_t>(c - 'A' + 10);
    }
    if (c <= 'f' && c >= 'a') {
        return static_cast<uint8_t>(c - 'a' + 10);
    }
    return 0;
}

constexpr uint8_t HexToUint8(const char* c) noexcept
{
    return static_cast<uint8_t>((HexToDec(c[0]) << 4U) + HexToDec(c[1]));
}

constexpr bool IsUidString(string_view str) noexcept
{
    // UID string in the form 8-4-4-4-12. A total of 36 characters (32 hexadecimal characters and 4 hyphens).
    if (str.size() != 36U) {
        return false;
    }

    auto hexChars = [](string_view str) {
        for (const auto& c : str) {
            if (!((c <= '9' && c >= '0') || (c <= 'F' && c >= 'A') || (c <= 'f' && c >= 'a'))) {
                return false;
            }
        }
        return true;
    };
    return hexChars(str.substr(0U, 8U)) && (str[8U] == '-') && hexChars(str.substr(9U, 4U)) && (str[13U] == '-') &&
           hexChars(str.substr(14U, 4U)) && (str[18U] == '-') && hexChars(str.substr(19U, 4U)) && (str[23U] == '-') &&
           hexChars(str.substr(24U, 12U));
}

struct Uid {
    constexpr Uid() noexcept = default;

    explicit constexpr Uid(const uint8_t (&values)[16]) noexcept
    {
        uint64_t value = 0U;
        for (auto first = values, last = values + 8; first != last; ++first) {
            value = (value << 8) | *first; // 8: Multiply by 256
        }
        data[0] = value;

        value = 0U;
        for (auto first = values + 8, last = values + 16; first != last; ++first) {
            value = (value << 8) | *first; // 8: Multiply by 256
        }
        data[1] = value;
    }

    explicit constexpr Uid(const char (&str)[37])
    {
        if (IsUidString(str)) {
            auto src = str;

            uint64_t value = 0U;
            for (size_t i = 0; i < sizeof(uint32_t); ++i) {
                value = (value << 8) | HexToUint8(src); // 8: Multiply by 256
                src += 2;
            }
            ++src;
            for (size_t i = 0; i < sizeof(uint16_t); ++i) {
                value = (value << 8) | HexToUint8(src); // 8: Multiply by 256
                src += 2;
            }
            ++src;
            for (size_t i = 0; i < sizeof(uint16_t); ++i) {
                value = (value << 8) | HexToUint8(src); // 8: Multiply by 256
                src += 2;
            }
            ++src;
            data[0U] = value;

            value = 0U;
            for (size_t i = 0; i < sizeof(uint16_t); ++i) {
                value = (value << 8) | HexToUint8(src); // 8: Multiply by 256
                src += 2;
            }
            ++src;
            for (size_t i = 0; i < (sizeof(uint16_t) * 3); ++i) {
                value = (value << 8) | HexToUint8(src); // 8: Multiply by 256
                src += 2;
            }
            data[1U] = value;
        }
    }

    constexpr int compare(const Uid& rhs) const
    {
        if (data[0] < rhs.data[0]) {
            return -1;
        }
        if (data[0] > rhs.data[0]) {
            return 1;
        }
        if (data[1] < rhs.data[1]) {
            return -1;
        }
        if (data[1] > rhs.data[1]) {
            return 1;
        }
        return 0;
    }
    uint64_t data[2u] {};
};

inline constexpr bool operator<(const Uid& lhs, const Uid& rhs)
{
    if (lhs.data[0] > rhs.data[0]) {
        return false;
    }
    if (lhs.data[0] < rhs.data[0]) {
        return true;
    }
    if (lhs.data[1] < rhs.data[1]) {
        return true;
    }
    return false;
}

inline constexpr bool operator==(const Uid& lhs, const Uid& rhs)
{
    return (lhs.data[0U] == rhs.data[0U]) && (lhs.data[1U] == rhs.data[1U]);
}

inline constexpr bool operator!=(const Uid& lhs, const Uid& rhs)
{
    return !(lhs == rhs);
}

template<>
inline uint64_t hash(const Uid& value)
{
    return Hash(value.data[0], value.data[1]);
}
BASE_END_NAMESPACE()

#endif // API_BASE_UTIL_UID_H
