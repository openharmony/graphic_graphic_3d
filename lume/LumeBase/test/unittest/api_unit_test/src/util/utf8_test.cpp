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

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/containers/vector.h>
#include <base/util/utf8_decode.h>

#include "test_framework.h"

namespace {
// clang-format off
constexpr const char str_lat[] =
    u8"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut "
    u8"labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco ";

constexpr const char str_math[] = u8"\u2B30 \u2B48";
constexpr const uint32_t str_math_cps[] = { 0x2b30, ' ', 0x2b48 };

constexpr const char str_ch[] = u8"快速的棕色狐狸跳过懒狗";
constexpr const uint32_t str_ch_cps[] =
    { 0x5FEB, 0x901F, 0x7684, 0x68D5, 0x8272, 0x72D0, 0x72F8, 0x8DF3, 0x8FC7, 0x61D2, 0x72D7 };

constexpr const char str_run[] = u8"ᚠᛇᚻ᛫ᛒᛦᚦ᛫ᚠᚱᚩᚠᚢᚱ᛫ᚠᛁᚱᚪ᛫ᚷᛖᚻᚹᛦᛚᚳᚢᛗ"
                                 u8"ᛋᚳᛖᚪᛚ᛫ᚦᛖᚪᚻ᛫ᛗᚪᚾᚾᚪ᛫ᚷᛖᚻᚹᛦᛚᚳ᛫ᛗᛁᚳᛚᚢᚾ᛫ᚻᛦᛏ᛫ᛞᚫᛚᚪᚾ"
                                 u8"ᚷᛁᚠ᛫ᚻᛖ᛫ᚹᛁᛚᛖ᛫ᚠᚩᚱ᛫ᛞᚱᛁᚻᛏᚾᛖ᛫ᛞᚩᛗᛖᛋ᛫ᚻᛚᛇᛏᚪᚾ";
constexpr const uint32_t str_run_cps[] = { 0x16A0, 0x16C7, 0x16BB, 0x16EB, 0x16D2, 0x16E6, 0x16A6, 0x16EB, 0x16A0,
    0x16B1, 0x16A9, 0x16A0, 0x16A2, 0x16B1, 0x16EB, 0x16A0, 0x16C1, 0x16B1, 0x16AA, 0x16EB, 0x16B7, 0x16D6, 0x16BB,
    0x16B9, 0x16E6, 0x16DA, 0x16B3, 0x16A2, 0x16D7, 0x16CB, 0x16B3, 0x16D6, 0x16AA, 0x16DA, 0x16EB, 0x16A6, 0x16D6,
    0x16AA, 0x16BB, 0x16EB, 0x16D7, 0x16AA, 0x16BE, 0x16BE, 0x16AA, 0x16EB, 0x16B7, 0x16D6, 0x16BB, 0x16B9, 0x16E6,
    0x16DA, 0x16B3, 0x16EB, 0x16D7, 0x16C1, 0x16B3, 0x16DA, 0x16A2, 0x16BE, 0x16EB, 0x16BB, 0x16E6, 0x16CF, 0x16EB,
    0x16DE, 0x16AB, 0x16DA, 0x16AA, 0x16BE, 0x16B7, 0x16C1, 0x16A0, 0x16EB, 0x16BB, 0x16D6, 0x16EB, 0x16B9, 0x16C1,
    0x16DA, 0x16D6, 0x16EB, 0x16A0, 0x16A9, 0x16B1, 0x16EB, 0x16DE, 0x16B1, 0x16C1, 0x16BB, 0x16CF, 0x16BE, 0x16D6,
    0x16EB, 0x16DE, 0x16A9, 0x16D7, 0x16D6, 0x16CB, 0x16EB, 0x16BB, 0x16DA, 0x16C7, 0x16CF, 0x16AA, 0x16BE };

constexpr const char str_fin[] = u8"Törkylempijävongahdus";
constexpr const uint32_t str_fin_cps[] = { 0x0054, 0x00F6, 0x0072, 0x006B, 0x0079, 0x006C, 0x0065, 0x006D, 0x0070,
    0x0069, 0x006A, 0x00E4, 0x0076, 0x006F, 0x006E, 0x0067, 0x0061, 0x0068, 0x0064, 0x0075, 0x0073 };

constexpr const char str_boundaries[] = u8"\u0080\u0800\u007F\u07FF\uFFFF\uD7FF\uE000\uFFFD";
constexpr const uint32_t str_boundaries_cps[] = { 0x0080, 0x0800, 0x007F, 0x07FF, 0xFFFF, 0xD7FF, 0xE000, 0xFFFD };

//https://www.w3.org/2001/06/utf-8-wrong/UTF-8-test.html
constexpr const char str_invalid[][7] = {
    // Unexpected continuation bytes
    { '\x80' },
    { '\xBF' },
    // The following two bytes cannot appear in a correct UTF-8 string
    { '\xFE' },
    { '\xFF' },
    { '\xFE', '\xFE', '\xFF', '\xFF' },

    // ==================
    // Overlong sequences
    // Maximum overlong sequences
    { '\xC1', '\xBF' },
    { '\xE0', '\x9F', '\xBF'},
    { '\xfc', '\x83', '\xbf', '\xbf', '\xbf', '\xbf' },

    // Examples of an overlong ASCII character
    { '\xC0', '\xAF' },
    { '\xFC', '\x80', '\x80', '\x80', '\x80', '\xAF' },

    // Overlong representation of the NUL character
    { '\xC0', '\x80' },
    { '\xE0', '\x80', '\x80' },
    { '\xF0', '\x80', '\x80', '\x80' },
    { '\xF8', '\x80', '\x80', '\x80', '\x80' },
    { '\xFC', '\x80', '\x80', '\x80', '\x80', '\x80' },

    // ==================
    // Illegal code positions
    // Single UTF-16 surrogates
    { '\xED', '\xA0', '\x80' },
    { '\xED', '\xAD', '\xBF' },
    { '\xED', '\xAE', '\x80' },
    { '\xED', '\xBF', '\xBF' },
    // Paired UTF-16 surrogates
    { '\xED', '\xA0', '\x80', '\xED', '\xB0', '\x80' },
    { '\xED', '\xAF', '\xBF', '\xED', '\xBF', '\xBF' },
};
// clang-format on
} // namespace

UNIT_TEST(API_UnitTest_utf8, Count, testing::ext::TestSize.Level1)
{
    EXPECT_EQ(BASE_NS::CountGlyphsUtf8(str_lat), 183U);
    EXPECT_EQ(BASE_NS::CountGlyphsUtf8(str_ch), 11U);
    EXPECT_EQ(BASE_NS::CountGlyphsUtf8(str_run), 106U);
    EXPECT_EQ(BASE_NS::CountGlyphsUtf8(str_fin), 21U);
    EXPECT_EQ(BASE_NS::CountGlyphsUtf8(str_boundaries), 8U);

    for (const auto& invalid : str_invalid) {
        EXPECT_EQ(BASE_NS::CountGlyphsUtf8(invalid), 0U);
    }
}

UNIT_TEST(API_UnitTest_utf8, Decode, testing::ext::TestSize.Level1)
{
    uint32_t codepoint = 0;

    const char* ptr = str_math;
    for (uint32_t i = 0; (codepoint = BASE_NS::GetCharUtf8(&ptr), codepoint); i++) {
        EXPECT_EQ(codepoint, str_math_cps[i]);
    }
    ptr = str_ch;
    for (uint32_t i = 0; (codepoint = BASE_NS::GetCharUtf8(&ptr), codepoint); i++) {
        EXPECT_EQ(codepoint, str_ch_cps[i]);
    }
    ptr = str_run;
    for (uint32_t i = 0; (codepoint = BASE_NS::GetCharUtf8(&ptr), codepoint); i++) {
        EXPECT_EQ(codepoint, str_run_cps[i]);
    }
    ptr = str_fin;
    for (uint32_t i = 0; (codepoint = BASE_NS::GetCharUtf8(&ptr), codepoint); i++) {
        EXPECT_EQ(codepoint, str_fin_cps[i]);
    }
    ptr = str_boundaries;
    for (uint32_t i = 0; (codepoint = BASE_NS::GetCharUtf8(&ptr), codepoint); i++) {
        EXPECT_EQ(codepoint, str_boundaries_cps[i]);
    }
    for (size_t i = 0; i < 21; i++) {
        ptr = str_invalid[i];
        EXPECT_EQ(BASE_NS::GetCharUtf8(&ptr), 0);
    }
}
