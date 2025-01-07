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

#ifndef API_BASE_UTIL_UTF8_DECODE_H
#define API_BASE_UTIL_UTF8_DECODE_H

#include <cstdint>

#include <base/containers/string_view.h>
#include <base/namespace.h>
#include <base/util/log.h>

BASE_BEGIN_NAMESPACE()
namespace {

constexpr uint32_t UTF8_ACCEPT = 0U;
constexpr uint32_t UTF8_REJECT = 12U;

// The first table maps bytes to character classes that to reduce the size of the transition table and
// create bitmasks.
static constexpr const uint8_t CHAR_MAP[] = { 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U,
    0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U, 1U, 1U,
    1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 1U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 9U, 7U, 7U, 7U,
    7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U, 7U,
    8U, 8U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U, 2U,
    2U, 2U, 2U, 10U, 3U, 3U, 3U, 3U, 3U, 3U, 3U, 3U, 3U, 3U, 3U, 3U, 4U, 3U, 3U, 11U, 6U, 6U, 6U, 5U, 8U, 8U, 8U, 8U,
    8U, 8U, 8U, 8U, 8U, 8U, 8 };

// The second transition table that maps a combination of a state of the automaton and a character class to a state.
// These have been premultiplied with 12 to save the operation from runtime.
static constexpr const uint8_t STATE[] = { 0U, 12U, 24U, 36U, 60U, 96U, 84U, 12U, 12U, 12U, 48U, 72U, 12U, 12U, 12U,
    12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 0U, 12U, 12U, 12U, 12U, 12U, 0U, 12U, 0U, 12U, 12U, 12U, 24U, 12U,
    12U, 12U, 12U, 12U, 24U, 12U, 24U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 24U, 12U, 12U, 12U, 12U, 12U, 24U,
    12U, 12U, 12U, 12U, 12U, 12U, 12U, 24U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 36U, 12U, 36U, 12U, 12U, 12U,
    36U, 12U, 12U, 12U, 12U, 12U, 36U, 12U, 36U, 12U, 12U, 12U, 36U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12U, 12 };

constexpr inline uint32_t decode(uint32_t* state, uint32_t* codep, unsigned char byte)
{
    uint32_t type = CHAR_MAP[byte];
    uint32_t prevCodep = (byte & 0x3fU) | (*codep << 6U);
    uint32_t codepoint = (0xffU >> type) & (byte);
    *codep = (*state > UTF8_REJECT) ? prevCodep : codepoint;

    *state = STATE[*state + type];
    return *state;
}
} // namespace

/** Decode utf8 encoded string.
 * @param buf Utf8 encoded string pointer, moved to next codepoint on success.
 * @return Next unicode codepoint on success, 0 otherwise.
 */
constexpr uint32_t GetCharUtf8(const char** buf)
{
    uint32_t state = UTF8_ACCEPT;
    uint32_t codepoint = 0U;

    while (**buf) {
        decode(&state, &codepoint, static_cast<unsigned char>(**buf));
        (*buf)++;
        switch (state) {
            case UTF8_ACCEPT:
                return codepoint;
            case UTF8_REJECT:
                BASE_LOG_E("invalid utf8 sequence\n");
                return 0;
        }
    }
    return 0;
}

/** Count valid character in provided utf8 encoded string.
 * @param string Utf8 encoded string.
 * @return Valid unicode codepoint count in provided utf8 string.
 */
constexpr uint32_t CountGlyphsUtf8(const BASE_NS::string_view string)
{
    uint32_t state = UTF8_ACCEPT;
    uint32_t codepoint = 0U;
    uint32_t count = 0U;
    const char* s = string.data();
    const char* sEnd = string.data() + string.length();

    for (; (s < sEnd) && *s; ++s) {
        if (!decode(&state, &codepoint, static_cast<unsigned char>(*s))) {
            count += 1U;
        }
    }
    if (state != UTF8_ACCEPT) {
        BASE_LOG_E("malformed utf8 string\n");
    }
    return count;
}
BASE_END_NAMESPACE()
#endif // API_BASE_UTIL_UTF8_DECODE_H
