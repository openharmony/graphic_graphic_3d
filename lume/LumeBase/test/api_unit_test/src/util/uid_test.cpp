/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>

#ifndef NDEBUG
#define NDEBUG // uid_util assert
#endif         // !NDEBUG

#include <base/containers/string_view.h>
#include <base/util/uid.h>
#include <base/util/uid_util.h>

using namespace BASE_NS;
using namespace testing::ext;
class UidTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(UidTest, Uid, TestSize.Level1)
{
    ASSERT_EQ(Uid(), Uid());

    constexpr Uid uid1 { "12345678-90ab-cdef-ABCD-EF1234567890" };
    constexpr Uid uid2 { "ac836534-faf7-4919-b65f-eb78df4b8d0d" };
    Uid uid3;

    // The Uid "12345678-90ab-cdef-ABCD-EF1234567890" as raw bytes.
    constexpr uint8_t raw[16] { 0x12, 0x34, 0x56, 0x78, 0x90, 0xab, 0xcd, 0xef, 0xAB, 0xCD, 0xEF, 0x12, 0x34, 0x56,
        0x78, 0x90 };

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

HWTEST_F(UidTest, UidUtil, TestSize.Level1)
{
    constexpr Uid uid { "12345678-90ab-cdef-abcd-ef1234567890" };
    const string_view str { "12345678-90ab-cdef-abcd-ef1234567890" };

    // Test Uid to string conversion.
    constexpr auto strB = to_string(uid);
    ASSERT_EQ(str, strB);

    // Test String to uid conversion.
    constexpr Uid uidB = StringToUid("12345678-90ab-cdef-abcd-ef1234567890");
    ASSERT_EQ(uid, uidB);
}

HWTEST_F(UidTest, Transforms, TestSize.Level1)
{
    ASSERT_EQ(HexToDec('a'), 10);
    ASSERT_EQ(HexToDec('A'), 10);
    ASSERT_EQ(HexToDec('5'), 5);
    ASSERT_EQ(HexToDec('k'), 0);
    ASSERT_EQ(HexToUint8("a5"), 165);  // only first 2 symbols
    ASSERT_EQ(HexToUint8("a57"), 165); // only first 2 symbols
    constexpr Uid uidA { "12345678-90ab-cdef-ABCD-EF1234567890" };
    constexpr Uid uidB { "12345678-90ab-cdef-BBCD-EF1234567890" };
    constexpr Uid uidC { "12345678-90ab-cdef-BBCD-EF1234567891" };
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
        static_assert(!IsUidString("12345678-90ab-cdef-gABC-DEF123456789"));
        static_assert(!IsUidString("12345678190ab-cdef-ABCD-EF1234567890"));
        static_assert(!IsUidString("12345678-90ab1cdef-ABCD-EF1234567890"));
        static_assert(!IsUidString("12345678-90ab-cdef1ABCD-EF1234567890"));
        static_assert(!IsUidString("12345678-90ab-cdef-ABCD1EF1234567890"));
        static_assert(!IsUidString("12345678-90ab-cdef-ABCD-EF123456789"));
        EXPECT_FALSE(IsUidString("12345678-90ab-cdef-ABCD-EF12345678900"));

        static constexpr Uid uid { "12345678-90ab-cdef-gABC-DEF123456789" };
        static constexpr Uid uidRaw {};
        static_assert(uid == uidRaw);
        ASSERT_EQ(uid, uidRaw);
    }
}
