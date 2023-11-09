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

#ifndef API_BASE_UTIL_UID_H
#define API_BASE_UTIL_UID_H

#include <cstdint>

#include <base/namespace.h>
#include <base/util/compile_time_hashes.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
constexpr uint8_t HexToDec(char c)
{
    if ('f' >= c && c >= 'a') {
        return c - 'a' + 10;
    } else if ('F' >= c && c >= 'A') {
        return c - 'A' + 10;
    } else if ('9' >= c && c >= '0') {
        return c - '0';
    }

    BASE_ASSERT(false);
    return 0;
}

constexpr uint8_t HexToUint8(const char* c)
{
    return (HexToDec(c[0]) << 4) + HexToDec(c[1]);
}

struct Uid {
    constexpr Uid() noexcept = default;

    explicit constexpr Uid(const uint8_t (&values)[16]) noexcept
    {
        auto src = values;
        for (auto& dst : data) {
            dst = *src++;
        }
    }

    explicit constexpr Uid(const char (&str)[37])
    {
        auto dst = data;
        auto src = str;
        for (size_t i = 0; i < sizeof(uint32_t); ++i) {
            *dst++ = HexToUint8(src);
            src += 2;
        }
        ++src;
        for (size_t i = 0; i < sizeof(uint16_t); ++i) {
            *dst++ = HexToUint8(src);
            src += 2;
        }
        ++src;
        for (size_t i = 0; i < sizeof(uint16_t); ++i) {
            *dst++ = HexToUint8(src);
            src += 2;
        }
        ++src;
        for (size_t i = 0; i < sizeof(uint16_t); ++i) {
            *dst++ = HexToUint8(src);
            src += 2;
        }
        ++src;
        for (size_t i = 0; i < (sizeof(uint16_t) * 3); ++i) {
            *dst++ = HexToUint8(src);
            src += 2;
        }
    }

    uint8_t data[16u] {};
};

inline bool operator<(const Uid& lhs, const Uid& rhs)
{
    auto r = rhs.data;
    for (const auto& l : lhs.data) {
        if (l != *r) {
            return (l < *r);
        }
        ++r;
    }
    return false;
}

inline bool operator==(const Uid& lhs, const Uid& rhs)
{
    auto r = rhs.data;
    for (const auto& l : lhs.data) {
        if (l != *r) {
            return false;
        }
        ++r;
    }
    return true;
}

inline bool operator!=(const Uid& lhs, const Uid& rhs)
{
    return !(lhs == rhs);
}

template<typename T>
uint64_t hash(const T& b);

template<>
inline uint64_t hash(const Uid& value)
{
    return FNV1aHash(value.data, sizeof(value.data));
}
BASE_END_NAMESPACE()

#endif // API_BASE_UTIL_UID_H
