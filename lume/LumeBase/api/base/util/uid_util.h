/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2021-2022. All rights reserved.
 */

#ifndef API_BASE_UTIL_UID_UTIL_H
#define API_BASE_UTIL_UID_UTIL_H

#include <base/containers/fixed_string.h>
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

    auto* src = &value.data[0];
    auto* dst = str.data();
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        Uint8ToHex(*src++, dst, dst + 1);
        dst += 2;
    }
    *dst++ = '-';
    for (size_t i = 0; i < sizeof(uint16_t); ++i) {
        Uint8ToHex(*src++, dst, dst + 1);
        dst += 2;
    }
    *dst++ = '-';
    for (size_t i = 0; i < sizeof(uint16_t); ++i) {
        Uint8ToHex(*src++, dst, dst + 1);
        dst += 2;
    }
    *dst++ = '-';
    for (size_t i = 0; i < sizeof(uint16_t); ++i) {
        Uint8ToHex(*src++, dst, dst + 1);
        dst += 2;
    }
    *dst++ = '-';
    for (size_t i = 0; i < (sizeof(uint16_t) * 3); ++i) {
        Uint8ToHex(*src++, dst, dst + 1);
        dst += 2;
    }
    return str;
}
BASE_END_NAMESPACE()

#endif // API_BASE_UTIL_UID_UTIL_H
