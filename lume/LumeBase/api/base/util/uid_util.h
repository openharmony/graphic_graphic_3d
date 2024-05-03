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

#ifndef API_BASE_UTIL_UID_UTIL_H
#define API_BASE_UTIL_UID_UTIL_H

#include <cstddef>
#include <cstdint>

#include <base/containers/fixed_string.h>
#include <base/containers/string_view.h>
#include <base/util/log.h>
#include <base/util/uid.h>

BASE_BEGIN_NAMESPACE()
constexpr void Uint8ToHex(const uint8_t value, char* c0, char* c1)
{
    constexpr char chars[] = "0123456789abcdef";
    *c0 = chars[((value & 0xf0) >> 4u)];
    *c1 = chars[value & 0x0f];
}

constexpr Uid StringToUid(string_view value)
{
    constexpr size_t UID_LENGTH = 36;
    BASE_ASSERT(value.size() == UID_LENGTH);
    char str[UID_LENGTH + 1] {};
    str[UID_LENGTH] = '\0';
    value.copy(str, UID_LENGTH, 0);
    return Uid(str);
}

constexpr fixed_string<36u> to_string(const Uid& value)
{
    constexpr size_t UID_LENGTH = 36;
    fixed_string<UID_LENGTH> str(UID_LENGTH);

    auto* dst = str.data();
    uint64_t data = value.data[0];
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        Uint8ToHex(data >> 56U, dst, dst + 1);
        dst += 2;
        data <<= 8U;
    }
    *dst++ = '-';
    for (size_t i = 0; i < sizeof(uint16_t); ++i) {
        Uint8ToHex(data >> 56U, dst, dst + 1);
        dst += 2;
        data <<= 8U;
    }
    *dst++ = '-';
    for (size_t i = 0; i < sizeof(uint16_t); ++i) {
        Uint8ToHex(data >> 56U, dst, dst + 1);
        dst += 2;
        data <<= 8U;
    }
    *dst++ = '-';
    data = value.data[1];
    for (size_t i = 0; i < sizeof(uint16_t); ++i) {
        Uint8ToHex(data >> 56U, dst, dst + 1);
        dst += 2;
        data <<= 8U;
    }
    *dst++ = '-';
    for (size_t i = 0; i < (sizeof(uint16_t) * 3); ++i) {
        Uint8ToHex(data >> 56U, dst, dst + 1);
        dst += 2;
        data <<= 8U;
    }
    return str;
}
BASE_END_NAMESPACE()

#endif // API_BASE_UTIL_UID_UTIL_H
