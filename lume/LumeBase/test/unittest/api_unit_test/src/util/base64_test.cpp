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
#include <base/util/base64_decode.h>
#include <base/util/base64_encode.h>

#include "test_framework.h"

constexpr const BASE_NS::string_view DATA[] = {
    "Many hands make light work.",
    "light work.",
    "light work",
    "light wor",
    "light wo",
    "light w",
};

constexpr const BASE_NS::string_view BASE64[] = {"TWFueSBoYW5kcyBtYWtlIGxpZ2h0IHdvcmsu",
    "bGlnaHQgd29yay4=",
    "bGlnaHQgd29yaw==",
    "bGlnaHQgd29y",
    "bGlnaHQgd28=",
    "bGlnaHQgdw=="};

constexpr const BASE_NS::string_view BAD_DATA[] = {"bad   input"};

constexpr const BASE_NS::string_view BAD_BASE64[] = {
    "YmFkICAgaW5wdXQ=    "  // bad ending
};

/**
 * @tc.name: Encode
 * @tc.desc: Tests for Encode. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Base64, Encode, testing::ext::TestSize.Level1)
{
    auto result = BASE64;
    for (const auto& str : DATA) {
        BASE_NS::string encoded = BASE_NS::Base64Encode(BASE_NS::array_view<const uint8_t>(
            static_cast<const uint8_t*>(static_cast<const void*>(str.data())), str.size()));
        EXPECT_EQ(*result++, encoded);
    }
    auto resultBad = BAD_BASE64;
    for (const auto& str : BAD_DATA) {
        BASE_NS::string encoded = BASE_NS::Base64Encode(BASE_NS::array_view<const uint8_t>(
            static_cast<const uint8_t*>(static_cast<const void*>(str.data())), str.size()));
        BASE_NS::string_view compareValue = *resultBad++;
        auto trim_pos = compareValue.find(' ');
        if (trim_pos != compareValue.npos)
            compareValue.remove_suffix(compareValue.size() - trim_pos);
        EXPECT_EQ(compareValue, encoded);
    }
}

/**
 * @tc.name: Decode
 * @tc.desc: Tests for Decode. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Base64, Decode, testing::ext::TestSize.Level1)
{
    auto result = DATA;
    for (const auto& str : BASE64) {
        BASE_NS::vector<uint8_t> decoded = BASE_NS::Base64Decode(str);
        EXPECT_EQ(*result++,
            BASE_NS::string_view(static_cast<const char*>(static_cast<const void*>(decoded.data())), decoded.size()));
    }
    auto resultBad = BAD_DATA;
    for (const auto& str : BAD_BASE64) {
        BASE_NS::vector<uint8_t> decoded = BASE_NS::Base64Decode(str);
        BASE_NS::string_view compareValue = *resultBad++;
        auto compareU = BASE_NS::array_view<const uint8_t>(
            static_cast<const uint8_t*>(static_cast<const void*>(compareValue.data())), compareValue.size());
        for (size_t i = 0; i < decoded.size(); i += 3) {
            if (decoded[i] + decoded[i + 1] + decoded[i + 2] != 0) {
                for (size_t j = 0; j < 3; j++) {
                    EXPECT_EQ(compareU[j], decoded[j]);
                }
            }
        }
    }
}

/**
 * @tc.name: EmptyInput
 * @tc.desc: VULN-051 regression — Base64Decode with empty input must not crash or produce output.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Base64, EmptyInput, testing::ext::TestSize.Level1)
{
    auto result = BASE_NS::Base64Decode(BASE_NS::string_view{});
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: AllPaddingInput
 * @tc.desc: VULN-051 regression — CountPadding must not decrement the iterator past the start of an
 *           all-padding string, which would be undefined behaviour.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Base64, AllPaddingInput, testing::ext::TestSize.Level1)
{
    auto result = BASE_NS::Base64Decode(BASE_NS::string_view{"===="});
    EXPECT_TRUE(result.empty());
}

/**
 * @tc.name: DecodeNonEmpty
 * @tc.desc: VULN-031 regression — loop counter must not go negative for non-empty input strings,
 *           which could cause the loop body to execute with an out-of-range index.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Base64, DecodeNonEmpty, testing::ext::TestSize.Level1)
{
    BASE_NS::string encoded(100u, 'A');
    auto result = BASE_NS::Base64Decode(BASE_NS::string_view{encoded});
    EXPECT_FALSE(result.empty());
}

/**
 * @tc.name: DecodeNonPadded
 * @tc.desc: Non-padded base64 input (length not a multiple of 4) must decode correctly
 *           without out-of-bounds writes. Tests the remainder-byte handling path.
 * @tc.type: FUNC
 */
UNIT_TEST(API_UnitTest_Base64, DecodeNonPadded, testing::ext::TestSize.Level1)
{
    // "light wor" padded   = "bGlnaHQgd29y"   (12 chars, 0 remainder)
    // "light wor" unpadded = "bGlnaHQgd29y"   (same, multiple of 4 — no remainder)
    // "light wo"  padded   = "bGlnaHQgd28="   (12 chars, 1 pad byte)
    // "light wo"  unpadded = "bGlnaHQgd28"    (11 chars, 3 remainder)
    // "light w"   padded   = "bGlnaHQgdw=="   (12 chars, 2 pad bytes)
    // "light w"   unpadded = "bGlnaHQgdw"     (10 chars, 2 remainder)

    // 2-byte remainder (strip both '=' from "bGlnaHQgdw==")
    {
        const BASE_NS::string_view input = "bGlnaHQgdw";  // "light w"
        const BASE_NS::string_view expected = "light w";
        auto decoded = BASE_NS::Base64Decode(input);
        BASE_NS::string_view result(static_cast<const char*>(static_cast<const void*>(decoded.data())), decoded.size());
        EXPECT_EQ(expected, result);
    }

    // 3-byte remainder (strip single '=' from "bGlnaHQgd28=")
    {
        const BASE_NS::string_view input = "bGlnaHQgd28";  // "light wo"
        const BASE_NS::string_view expected = "light wo";
        auto decoded = BASE_NS::Base64Decode(input);
        BASE_NS::string_view result(static_cast<const char*>(static_cast<const void*>(decoded.data())), decoded.size());
        EXPECT_EQ(expected, result);
    }

    // Minimal 2-char input (no full quartets, only remainder)
    {
        const BASE_NS::string_view input = "YQ";  // "a"
        const BASE_NS::string_view expected = "a";
        auto decoded = BASE_NS::Base64Decode(input);
        BASE_NS::string_view result(static_cast<const char*>(static_cast<const void*>(decoded.data())), decoded.size());
        EXPECT_EQ(expected, result);
    }

    // Minimal 3-char input (no full quartets, only remainder)
    {
        const BASE_NS::string_view input = "YWI";  // "ab"
        const BASE_NS::string_view expected = "ab";
        auto decoded = BASE_NS::Base64Decode(input);
        BASE_NS::string_view result(static_cast<const char*>(static_cast<const void*>(decoded.data())), decoded.size());
        EXPECT_EQ(expected, result);
    }
}
