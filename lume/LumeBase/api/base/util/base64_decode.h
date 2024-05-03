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

#ifndef API_BASE_UTIL_BASE64_DECODE_H
#define API_BASE_UTIL_BASE64_DECODE_H

#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
namespace Detail {
static constexpr const uint8_t FROM_BASE64[] = {
    62U,                                              // '+' (ascii 43)
    0, 0, 0,                                          // invalid
    63U,                                              // '/' (ascii 47)
    52U, 53U, 54U, 55U, 56U, 57U, 58U, 59U, 60U, 61U, //'0' '9' (ascii 48-57)
    0, 0, 0,                                          // invalid
    0,                                                // '=' padding (ascii 61)
    0, 0, 0,                                          // invalid
    0U, 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U, 13U, 14U, 15U, 16U, 17U, 18U, 19U, 20U, 21U, 22U, 23U, 24U,
    25U,              //'A' 'Z' (ascii 65-90)
    0, 0, 0, 0, 0, 0, // invalid
    26U, 27U, 28U, 29U, 30U, 31U, 32U, 33U, 34U, 35U, 36U, 37U, 38U, 39U, 40U, 41U, 42U, 43U, 44U, 45U, 46U, 47U, 48U,
    49U, 50U, 51U //'a' 'z' (ascii 97-122)
};

inline uint8_t FromBase64(uint8_t c)
{
    // Map characters to values from the table if they are in the 43..122 range. Invalid characters will return zeros.
    if (c >= 43 && c <= 122) {
        return FROM_BASE64[c - 43];
    } else {
        return 0;
    }
}

inline uint32_t FromBase64(const char*& src)
{
    uint32_t bits = static_cast<uint32_t>(Detail::FromBase64((uint8_t)(*src++))) << (3u * 6u);
    bits |= Detail::FromBase64((uint8_t)(*src++)) << (2u * 6u);
    bits |= Detail::FromBase64((uint8_t)(*src++)) << (1u * 6u);
    bits |= Detail::FromBase64((uint8_t)(*src++)) << (0u * 6u);
    return bits;
}

inline void DecodeQuartets(uint8_t* dst, const char* src, signed left)
{
    for (; left >= 4; left -= 4) {
        auto bits = FromBase64(src);
        *dst++ = uint8_t((bits >> 16u) & 0xff);
        *dst++ = uint8_t((bits >> 8u) & 0xff);
        *dst++ = uint8_t((bits >> 0u) & 0xff);
    }
}

inline size_t CountPadding(string_view encodedString)
{
    auto end = encodedString.end();
    auto first = --end;
    while (*first == '=') {
        --first;
    }
    return static_cast<size_t>(end - first);
}
} // namespace Detail

/** Decode base64 encoded data.
 * A base64 encoded string should be padded to the next four bytes with '=' characters.
 * @param encodedString String containing the base64 encoded data.
 * @return Decoded data.
 */
inline vector<uint8_t> Base64Decode(string_view encodedString)
{
    // the length of the decoded binary data will be 3/4 of the encoded string.
    vector<uint8_t> decodedBinary(encodedString.size() * 3u / 4u);

    Detail::DecodeQuartets(decodedBinary.data(), encodedString.data(), static_cast<signed>(encodedString.size()));
    decodedBinary.erase(
        decodedBinary.cend() - static_cast<ptrdiff_t>(Detail::CountPadding(encodedString)), decodedBinary.cend());
    return decodedBinary;
}
BASE_END_NAMESPACE()
#endif // API_BASE_UTIL_BASE64_DECODE_H
