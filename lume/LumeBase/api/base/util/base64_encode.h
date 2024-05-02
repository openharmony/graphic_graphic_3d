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

#ifndef API_BASE_UTIL_BASE64_ENCODE_H
#define API_BASE_UTIL_BASE64_ENCODE_H

#include <base/containers/array_view.h>
#include <base/containers/string.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
namespace Detail {
static constexpr const char TO_BASE64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

inline void ToBase64(const uint8_t* threeBytes, char* output)
{
    output[0U] = TO_BASE64[threeBytes[0u] >> 2u];
    output[1U] = TO_BASE64[((threeBytes[0u] << 4u) | (threeBytes[1u] >> 4u)) & 0x3f];
    output[2U] = TO_BASE64[((threeBytes[1u] << 2u) | (threeBytes[2u] >> 6u)) & 0x3f];
    output[3U] = TO_BASE64[threeBytes[2u] & 0x3f];
}

inline void ToBase64(const uint8_t* threeBytes, char* output, signed left)
{
    output[0U] = TO_BASE64[threeBytes[0u] >> 2u];
    output[1U] = TO_BASE64[((threeBytes[0u] << 4u) | (threeBytes[1u] >> 4u)) & 0x3f];

    output[2U] = left > 1 ? (TO_BASE64[((threeBytes[1u] << 2u) | (threeBytes[2u] >> 6u)) & 0x3f]) : '=';
    output[3U] = left > 2 ? TO_BASE64[threeBytes[2u] & 0x3f] : '=';
}

inline signed EncodeTriplets(char*& dst, const uint8_t*& src, signed left)
{
    for (; left >= 3; left -= 3) {
        Detail::ToBase64(src, dst);
        src += 3U;
        dst += 4U;
    }
    return left;
}

inline void FillTriplet(uint8_t* dst, const uint8_t*& src, signed left)
{
    switch (left) {
        case 2:
            *dst++ = *src++;
        case 1:
            *dst++ = *src++;
    }
}

inline void EncodeTail(char* dst, const uint8_t* src, signed left)
{
    if (left) {
        uint8_t rest[3] { 0, 0, 0 };
        FillTriplet(rest, src, left);
        Detail::ToBase64(rest, dst, left);
    }
}
} // namespace Detail

/** Base 64 encode data.
 * @param binaryData Data to encode.
 * @return Data as a base64 string.
 */
inline string Base64Encode(array_view<const uint8_t> binaryData)
{
    // The length of the encoded binary data will be about 4/3 of the encoded string.
    string encodedString((binaryData.size() + 2U) / 3U * 4U, '=');

    auto dst = encodedString.data();
    auto src = binaryData.data();
    signed left = static_cast<signed>(binaryData.size());

    // First write the full groups of three bytes
    left = Detail::EncodeTriplets(dst, src, left);

    // Add the rest of the bytes that was not divisible by three
    Detail::EncodeTail(dst, src, left);

    return encodedString;
}
BASE_END_NAMESPACE()
#endif // API_BASE_UTIL_BASE64_ENCODE_H