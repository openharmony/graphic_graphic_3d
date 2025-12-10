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

#include "test_framework.h"

#ifndef NDEBUG
#define NDEBUG // uid_util assert
#endif         // !NDEBUG

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <base/util/uid_util.h>

using namespace BASE_NS;

/**
 * @tc.name: Uid
 * @tc.desc: Tests for Uid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilUidTest, Uid, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(Uid(), Uid());

    constexpr Uid uid1 { "a55f9712-f8ad-47e5-987d-23e160d83f75" };
    constexpr Uid uid2 { "ac836534-faf7-4919-b65f-eb78df4b8d0d" };
    Uid uid3;

    // The uid "a55f9712-f8ad-47e5-987d-23e160d83f75" as raw bytes.
    constexpr uint8_t raw[16] { 0xa5, 0x5f, 0x97, 0x12, 0xf8, 0xad, 0x47, 0xe5, 0x98, 0x7d, 0x23, 0xe1, 0x60, 0xd8,
        0x3f, 0x75 };

    constexpr Uid uid1Raw { raw };
    ASSERT_EQ(uid1, uid1Raw);
    ASSERT_NE(uid2, uid1Raw);

    // Test equality and inequality.
    ASSERT_EQ(uid1, uid1);
    ASSERT_NE(uid1, uid2);
    ASSERT_NE(uid1, uid3);

    // Test assignment.
    uid3 = uid1;
    ASSERT_EQ(uid1, uid3);

    // Test cpy cntsr.
    constexpr Uid uid1Copy(uid1);
    ASSERT_EQ(uid1, uid1Copy);
}

/**
 * @tc.name: UidUtil
 * @tc.desc: Tests for Uid Util. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilUidTest, UidUtil, testing::ext::TestSize.Level1)
{
    constexpr Uid uid { "a55f9712-f8ad-47e5-987d-23e160d83f75" };
    const string_view str { "a55f9712-f8ad-47e5-987d-23e160d83f75" };

    // Test Uid to string conversion.
    constexpr auto strB = to_string(uid);
    ASSERT_EQ(str, strB);

    // Test String to uid conversion.
    constexpr Uid uidB = StringToUid("a55f9712-f8ad-47e5-987d-23e160d83f75");
    ASSERT_EQ(uid, uidB);
}

/**
 * @tc.name: Transforms
 * @tc.desc: Tests for Transforms. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_UtilUidTest, Transforms, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(HexToDec('a'), 10);
    ASSERT_EQ(HexToDec('A'), 10);
    ASSERT_EQ(HexToDec('5'), 5);
    ASSERT_EQ(HexToDec('k'), 0);
    ASSERT_EQ(HexToDec('/'), 0);
    ASSERT_EQ(HexToDec(':'), 0);
    ASSERT_EQ(HexToDec('@'), 0);
    ASSERT_EQ(HexToDec('G'), 0);
    ASSERT_EQ(HexToDec('`'), 0);
    ASSERT_EQ(HexToDec('g'), 0);

    ASSERT_EQ(HexToUint8("a5"), 165);  // only first 2 symbols
    ASSERT_EQ(HexToUint8("a57"), 165); // only first 2 symbols
    constexpr Uid uidA { "a55f9712-f8ad-47e4-987d-23e160d83f75" };
    constexpr Uid uidB { "a55f9712-f8ad-47e5-987d-23e160d83f74" };
    constexpr Uid uidC { "a55f9712-f8ad-47e5-987d-23e160d83f75" };
    ASSERT_TRUE(uidA < uidB);
    ASSERT_TRUE(uidB < uidC);
    ASSERT_TRUE(uidA < uidC);
    ASSERT_FALSE(uidB < uidB);
    ASSERT_LT(uidA.compare(uidB), 0);
    ASSERT_LT(uidB.compare(uidC), 0);
    ASSERT_GT(uidB.compare(uidA), 0);
    ASSERT_GT(uidC.compare(uidB), 0);
    ASSERT_EQ(uidB.compare(uidB), 0);

    {
        static_assert(IsUidString("12345678-90ab-cdef-ABCD-EF1234567890"));
        const bool ok = IsUidString("12345678-90ab-cdef-ABCD-EF1234567890");
        EXPECT_TRUE(ok);

        static constexpr Uid uid { "12345678-90ab-cdef-ABCD-EF1234567890" };
        static constexpr uint8_t raw[16] { 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0xAB, 0xCD, 0xEF, 0x12, 0x34,
            0x56, 0x78, 0x90 };
        static constexpr Uid uidRaw { raw };
        static_assert(uid == uidRaw);
        ASSERT_EQ(uid, uidRaw);
    }
    {
        // Compile-time:
        static_assert(!IsUidString("12345678-90ab-cdef-gABC-DEF123456789"));
        static_assert(!IsUidString("12345678190ab-cdef-ABCD-EF1234567890"));
        static_assert(!IsUidString("12345678-90ab1cdef-ABCD-EF1234567890"));
        static_assert(!IsUidString("12345678-90ab-cdef1ABCD-EF1234567890"));
        static_assert(!IsUidString("12345678-90ab-cdef-ABCD1EF1234567890"));
        static_assert(!IsUidString("12345678-90ab-cdef-ABCD-EF123456789"));
        static_assert(!IsUidString("12345678-90ab-cdef-ABCD-EF12345678900"));

        // As at runtime:
        EXPECT_FALSE(IsUidString("12345678-90ab-cdef-gABC-DEF123456789"));
        EXPECT_FALSE(IsUidString("12345678190ab-cdef-ABCD-EF1234567890"));
        EXPECT_FALSE(IsUidString("12345678-90ab1cdef-ABCD-EF1234567890"));
        EXPECT_FALSE(IsUidString("12345678-90ab-cdef1ABCD-EF1234567890"));
        EXPECT_FALSE(IsUidString("12345678-90ab-cdef-ABCD1EF1234567890"));
        EXPECT_FALSE(IsUidString("12345678-90ab-cdef-ABCD-EF123456789"));
        EXPECT_FALSE(IsUidString("12345678-90ab-cdef-ABCD-EF12345678900"));

        static constexpr auto base = string_view("12345678-90ab-cdef-ABCD-EF1234567890");
        // '/' is the first ASCII character before '0'
        BASE_NS::string test(base);
        for (size_t i = 0; i < test.size(); ++i) {
            test = base;
            test[i] = '/';
            EXPECT_FALSE(IsUidString(test));
        }
        // ':' is the first ASCII character after '9'
        for (size_t i = 0; i < test.size(); ++i) {
            test = base;
            test[i] = ':';
            EXPECT_FALSE(IsUidString(test));
        }
        // '@' is the first ASCII character before 'A'
        for (size_t i = 0; i < test.size(); ++i) {
            test = base;
            test[i] = '@';
            EXPECT_FALSE(IsUidString(test));
        }
        // 'G' is the first ASCII character after 'F'
        for (size_t i = 0; i < test.size(); ++i) {
            test = base;
            test[i] = 'G';
            EXPECT_FALSE(IsUidString(test));
        }
        // '`' is the first ASCII character before 'a'
        for (size_t i = 0; i < test.size(); ++i) {
            test = base;
            test[i] = '`';
            EXPECT_FALSE(IsUidString(test));
        }
        // 'g' is the first ASCII character after 'g'
        for (size_t i = 0; i < test.size(); ++i) {
            test = base;
            test[i] = 'g';
            EXPECT_FALSE(IsUidString(test));
        }

        static constexpr Uid uid { "12345678-90ab-cdef-gABC-DEF123456789" };
        static constexpr Uid uidRaw {};
        static_assert(uid == uidRaw);
        ASSERT_EQ(uid, uidRaw);
    }
}
