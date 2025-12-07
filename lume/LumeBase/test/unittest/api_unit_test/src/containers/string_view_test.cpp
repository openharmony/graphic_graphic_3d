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

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string_view.h>

#include "test_framework.h"

/**
 * @tc.name: emptyStringView
 * @tc.desc: Tests for Empty String View. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, emptyStringView, testing::ext::TestSize.Level1)
{
    {
        const BASE_NS::string_view testString;

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(0, testString.size());

        EXPECT_EQ(testString.begin(), testString.end());
        EXPECT_EQ(testString.cbegin(), testString.cend());
        EXPECT_EQ(testString.rbegin(), testString.rend());
        EXPECT_EQ(testString.crbegin(), testString.crend());
    }
    {
        constexpr const char* TEST_STRING = "\0";
        constexpr const BASE_NS::string_view testString(TEST_STRING);

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(TEST_STRING), testString.size());

        const char* str = TEST_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
    {
        constexpr const char TEST_STRING[] = { '\0' };
        const BASE_NS::string_view testString(TEST_STRING);

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(TEST_STRING), testString.size());

        const char* str = TEST_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
}

/**
 * @tc.name: stringView
 * @tc.desc: Tests for String View. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, stringView, testing::ext::TestSize.Level1)
{
    {
        constexpr const char* TEST_STRING = "ABCDEF";
        constexpr const BASE_NS::string_view testString(TEST_STRING);

        EXPECT_FALSE(testString.empty());
        EXPECT_EQ(strlen(TEST_STRING), testString.size());
        EXPECT_EQ('B', testString[1]);
        EXPECT_EQ('B', testString.at(1));
        EXPECT_EQ(testString.max_size(), SIZE_MAX - 1);

        EXPECT_EQ(testString.front(), testString[0]);
        EXPECT_EQ(testString.back(), testString[testString.size() - 1]);

        EXPECT_NE(testString.begin(), testString.end());
        EXPECT_NE(testString.cbegin(), testString.cend());
        EXPECT_NE(testString.rbegin(), testString.rend());
        EXPECT_NE(testString.crbegin(), testString.crend());

        const char* str = TEST_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
    {
        constexpr const char TEST_STRING[] = { 'A', 'B', 'C', 'D', 'E', 'F' };
        const BASE_NS::string_view testString(TEST_STRING, BASE_NS::countof(TEST_STRING));

        EXPECT_FALSE(testString.empty());
        EXPECT_EQ(sizeof(TEST_STRING), testString.size());
        EXPECT_EQ('B', testString[1]);
        EXPECT_EQ('B', testString.at(1));
        EXPECT_EQ(testString.front(), testString[0]);
        EXPECT_EQ(testString.back(), testString[testString.size() - 1]);

        EXPECT_NE(testString.begin(), testString.end());
        EXPECT_NE(testString.cbegin(), testString.cend());
        EXPECT_NE(testString.rbegin(), testString.rend());
        EXPECT_NE(testString.crbegin(), testString.crend());

        const char* str = TEST_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
}

/**
 * @tc.name: swap
 * @tc.desc: Tests for Swap. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, swap, testing::ext::TestSize.Level1)
{
    constexpr const char* TEST_STRING = "ABCDEF";
    constexpr const BASE_NS::string_view testString(TEST_STRING);
    constexpr const char* TEST_STRING2 = "FEDCBA";
    constexpr const BASE_NS::string_view testString2(TEST_STRING2);
    BASE_NS::string_view t(TEST_STRING);
    BASE_NS::string_view t2(TEST_STRING2);
    EXPECT_EQ(testString, t);
    EXPECT_EQ(testString2, t2);
    t.swap(t2);
    EXPECT_EQ(testString, t2);
    EXPECT_EQ(testString2, t);
}

/**
 * @tc.name: subStringView
 * @tc.desc: Tests for Sub String View. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, subStringView, testing::ext::TestSize.Level1)
{
    // remove_prefix
    {
        const char* TEST_STRING = "ABCDEF";
        const auto PREFIX_LEN = 3u;
        const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_prefix(len);
            return a;
        }(TEST_STRING, PREFIX_LEN);

        EXPECT_EQ(strlen(TEST_STRING) - PREFIX_LEN, testString.size());

        const char* str = TEST_STRING + PREFIX_LEN;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // remove_prefix huge
    {
        constexpr const char* TEST_STRING = "ABCDEF";
        constexpr const auto PREFIX_LEN = 8u;
        constexpr const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_prefix(len);
            return a;
        }(TEST_STRING, PREFIX_LEN);

        EXPECT_EQ(0, testString.size());
    }
    // remove_suffix
    {
        constexpr const char* TEST_STRING = "ABCDEF";
        constexpr const auto SUFFIX_LEN = 3u;
        constexpr const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_suffix(len);
            return a;
        }(TEST_STRING, SUFFIX_LEN);

        EXPECT_EQ(strlen(TEST_STRING) - SUFFIX_LEN, testString.size());

        const char* str = TEST_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // remove_suffix huge
    {
        constexpr const char* TEST_STRING = "ABCDEF";
        constexpr const auto SUFFIX_LEN = 8u;
        constexpr const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_suffix(len);
            return a;
        }(TEST_STRING, SUFFIX_LEN);

        EXPECT_EQ(0, testString.size());
    }
    // substr
    {
        constexpr const char* TEST_STRING = "ABCDEF";
        constexpr const BASE_NS::string_view testString(TEST_STRING);

        constexpr const BASE_NS::string_view copy = testString.substr();
        constexpr const BASE_NS::string_view prefix = testString.substr(0, 3);
        constexpr const BASE_NS::string_view suffix = testString.substr(3);

        EXPECT_EQ(0, testString.compare(copy));
        EXPECT_TRUE(testString == copy);
        EXPECT_FALSE(testString != copy);
        EXPECT_FALSE(testString < copy);
        EXPECT_TRUE(testString <= copy);
        EXPECT_FALSE(testString > copy);
        EXPECT_TRUE(testString >= copy);

        EXPECT_GT(testString.compare(prefix), 0);
        EXPECT_FALSE(testString == prefix);
        EXPECT_TRUE(testString != prefix);
        EXPECT_FALSE(testString < prefix);
        EXPECT_FALSE(testString <= prefix);
        EXPECT_TRUE(testString > prefix);
        EXPECT_TRUE(testString >= prefix);

        EXPECT_LT(testString.compare(suffix), 0);
        EXPECT_FALSE(testString == suffix);
        EXPECT_TRUE(testString != suffix);
        EXPECT_TRUE(testString < suffix);
        EXPECT_TRUE(testString <= suffix);
        EXPECT_FALSE(testString > suffix);
        EXPECT_FALSE(testString >= suffix);

        EXPECT_GT(suffix.compare(testString), 0);
        EXPECT_GT(suffix.compare(prefix), 0);

        EXPECT_LT(prefix.compare(testString), 0);
        EXPECT_LT(prefix.compare(suffix), 0);

        EXPECT_EQ(prefix.compare(0, 3, testString), prefix.compare(testString));
        EXPECT_EQ(testString.compare(1, 3, prefix), -prefix.compare("BCD"));
        EXPECT_EQ(testString.compare(1, 3, "ABC"), -prefix.compare("BCD"));
        EXPECT_GT(suffix.compare("ABC"), 0);
        EXPECT_EQ(prefix.compare(0, 3, testString, 0, 3), prefix.compare("ABC"));
        EXPECT_EQ(prefix.compare(0, 3, testString, 1, 3), prefix.compare("BCD"));
        EXPECT_EQ(prefix.compare(0, 3, "ABCDEF", 6), prefix.compare("ABCDEF"));
        EXPECT_EQ(prefix.compare(0, 3, "ABCDEF", 1), prefix.compare("A"));

        EXPECT_FALSE(prefix == suffix);
        EXPECT_TRUE(prefix != suffix);
        EXPECT_TRUE(prefix < suffix);
        EXPECT_TRUE(prefix <= suffix);
        EXPECT_FALSE(prefix > suffix);
        EXPECT_FALSE(prefix >= suffix);

        {
            char array[6] = {};
            auto const size = testString.copy(array, 1);
            EXPECT_TRUE(testString.substr(0, 1) == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 7);
            EXPECT_TRUE(testString == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 2, 1);
            EXPECT_TRUE(testString.substr(1, 2) == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 7, 1);
            EXPECT_TRUE(testString.substr(1) == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 1, 7);
            EXPECT_TRUE(size == 0);
            EXPECT_TRUE(testString.substr(7) == BASE_NS::string_view({}));
        }
    }
    // fixed string copy test. Substr not implemented in fixed strings. Used string view instead.
    {
        constexpr const char* TEST_STRING = "ABCDEF";
        constexpr const BASE_NS::fixed_string<6> testString(TEST_STRING);
        constexpr const BASE_NS::string_view testStringSW(TEST_STRING);
        {
            char array[6] = {};
            auto const size = testString.copy(array, 1);
            EXPECT_TRUE(testStringSW.substr(0, 1) == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 7);
            EXPECT_TRUE(testStringSW == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 2, 1);
            EXPECT_TRUE(testStringSW.substr(1, 2) == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 7, 1);
            EXPECT_TRUE(testStringSW.substr(1) == BASE_NS::string_view(array, size));
        }
        {
            char array[6] = {};
            auto const size = testString.copy(array, 1, 7);
            EXPECT_TRUE(size == 0);
            EXPECT_TRUE(testStringSW.substr(7) == BASE_NS::string_view({}));
        }
    }
}

/**
 * @tc.name: toString
 * @tc.desc: Tests for To String. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, toString, testing::ext::TestSize.Level1)
{
    {
        constexpr uint64_t num = 1234567890123456789;
        constexpr auto str = BASE_NS::to_string(num);
        EXPECT_EQ(str, "1234567890123456789");
    }
    {
        constexpr int64_t num = -1234567890123456789;
        constexpr auto str = BASE_NS::to_string(num);
        EXPECT_EQ(str, "-1234567890123456789");
    }
    {
        constexpr auto someStr { "Test_" };

        constexpr auto str0 = BASE_NS::basic_fixed_string("Test_");
        constexpr auto str1 = BASE_NS::fixed_string<BASE_NS::constexpr_strlen(someStr)>(someStr);
        EXPECT_EQ(str0, str1);

        constexpr uint64_t num = 1234567890123456789;
        constexpr auto numStr = BASE_NS::to_string(num);
        constexpr auto result = str0 + numStr;
        EXPECT_EQ(result, "Test_1234567890123456789");
    }

    {
        constexpr auto someStr { "" };
        constexpr auto str0 = BASE_NS::basic_fixed_string("");
        constexpr auto str1 = BASE_NS::fixed_string<BASE_NS::constexpr_strlen(someStr)>(someStr);
        EXPECT_TRUE(str0.empty());
        EXPECT_EQ(str0, str1);
        constexpr char* emptyChar = nullptr;
        EXPECT_EQ(BASE_NS::constexpr_strlen(emptyChar), BASE_NS::constexpr_strlen(someStr));

        constexpr uint64_t num = 1234567890123456789;
        constexpr auto numStr = BASE_NS::to_string(num);
        constexpr auto result = str0 + numStr;
        EXPECT_EQ(result, "1234567890123456789");
    }
}

/**
 * @tc.name: toHex
 * @tc.desc: Tests for To Hex. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, toHex, testing::ext::TestSize.Level1)
{
    {
        constexpr uint64_t num = 0;
        constexpr auto str = BASE_NS::to_hex(num);
        EXPECT_EQ(str, "0");
    }
    {
        constexpr uint64_t num = 1234567890123456789;
        constexpr auto str = BASE_NS::to_hex(num);
        EXPECT_EQ(str, "112210F47DE98115");
    }
    {
        constexpr uint64_t num = static_cast<uint64_t>(-1234567890123456789);
        constexpr auto str = BASE_NS::to_hex(num);
        EXPECT_EQ(str, "EEDDEF0B82167EEB");
    }
    {
        constexpr int64_t num = static_cast<uint64_t>(-1234567890123456789);
        constexpr auto str = BASE_NS::to_hex(num);
        EXPECT_EQ(str, "-112210F47DE98115");
    }
}

/**
 * @tc.name: hash
 * @tc.desc: Tests for Hash. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, hash, testing::ext::TestSize.Level1)
{
    {
        constexpr uint64_t num = 0;
        constexpr auto str0 = BASE_NS::to_string(num);
        auto h1 = BASE_NS::hash<21u>(str0);
        auto h2 = BASE_NS::FNV1aHash("0", strlen("0"));
        EXPECT_EQ(h1, h2);
    }
    {
        constexpr uint64_t num = 1234567890123456789;
        constexpr auto str0 = BASE_NS::to_string(num);
        auto h1 = BASE_NS::hash<21u>(str0);
        auto h2 = BASE_NS::FNV1aHash("1234567890123456789", strlen("1234567890123456789"));
        EXPECT_EQ(h1, h2);
    }
    {
        constexpr uint64_t num = static_cast<uint64_t>(-1234567890123456789);
        constexpr auto str0 = BASE_NS::to_string(num);
        auto h1 = BASE_NS::hash<21u>(str0);
        auto h2 = BASE_NS::FNV1aHash("17212176183586094827", strlen("17212176183586094827"));
        EXPECT_EQ(h1, h2);
    }
    {
        constexpr int64_t num = static_cast<uint64_t>(-1234567890123456789);
        constexpr auto str0 = BASE_NS::to_string(num);
        auto h1 = BASE_NS::hash<21u>(str0);
        auto h2 = BASE_NS::FNV1aHash("-1234567890123456789", strlen("-1234567890123456789"));
        EXPECT_EQ(h1, h2);
    }
}

/**
 * @tc.name: replace
 * @tc.desc: Tests for Replace. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, replace, testing::ext::TestSize.Level1)
{
    {
        auto str0 = BASE_NS::basic_fixed_string("ABCDEFGHIJKLMNOPQRSTU");
        str0.replace(3, 6, "def");
        EXPECT_EQ(str0, "ABCdefGHIJKLMNOPQRSTU");
    }
    {
        auto str0 = BASE_NS::basic_fixed_string<char, sizeof("ABCDEFGHIJKLMNOPQRSTU") + 3u>("ABCDEFGHIJKLMNOPQRSTU");
        str0.replace(3, 4, "def");
        EXPECT_EQ(str0, "ABCdefEFGHIJKLMNOPQRSTU");
    }
    {
        auto str0 = BASE_NS::basic_fixed_string("ABCDEFGHIJKLMNOPQRSTU");
        str0.replace(3, 4, "def");
        EXPECT_EQ(str0, "ABCdefEFGHIJKLMNOPQRS");
    }
    {
        auto str0 = BASE_NS::basic_fixed_string("ABCDEFGHIJKLMNOPQRSTU");
        str0.replace(3, 8, "def");
        EXPECT_EQ(str0, "ABCdefIJKLMNOPQRSTU");
    }
}

/**
 * @tc.name: construct
 * @tc.desc: Tests for Construct. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, construct, testing::ext::TestSize.Level1)
{
    {
        constexpr auto str0 = BASE_NS::fixed_string<6>("ABC");
        constexpr auto str1 = str0;
        EXPECT_EQ(str1, "ABC");
    }

    {
        constexpr auto str0 = BASE_NS::fixed_string<6>("ABC");
        constexpr auto str1 = BASE_NS::fixed_string<6>("DEF");
        BASE_NS::fixed_string<6> str2 = str0 + str1;
        EXPECT_EQ(str2, "ABCDEF");
    }
    {
        constexpr auto str0 = BASE_NS::basic_fixed_string("ABC");
        constexpr auto str1 = BASE_NS::basic_fixed_string("DEF");
        BASE_NS::fixed_string<6> str2 = str0 + str1;
        EXPECT_EQ(str2, "ABCDEF");
    }
    {
        constexpr BASE_NS::basic_fixed_string<char, 7> str0("ABC");
        char str1[4] = "DEF";
        BASE_NS::basic_fixed_string<char, 7> str2 = str0 + str1;
        EXPECT_EQ(str2, "ABCDEF");
    }
    {
        char str0[4] = "ABC";
        constexpr auto str1 = BASE_NS::basic_fixed_string<char, 7>("DEF");
        BASE_NS::basic_fixed_string<char, 7> str2 = str0 + str1;
        EXPECT_EQ(str2, "ABCDEF");
    }
    {
        constexpr auto str0 = BASE_NS::basic_fixed_string<char, 7>("ABC");
        BASE_NS::basic_fixed_string<char, 7> str2 = 'D' + str0;
        EXPECT_EQ(str2, "DABC");
    }
    {
        auto str0 = BASE_NS::basic_fixed_string<char, 7>("ABC");
        const char* str1 = "DEF";
        str0.append(str1);
        EXPECT_EQ(str0, "ABCDEF");
    }
    {
        auto concat = []() {
            auto str0 = BASE_NS::fixed_string<6>("ABC");
            str0 += "DEF";
            return str0;
        };
        static constexpr const auto str0 = concat();
        EXPECT_EQ(str0, "ABCDEF");
    }
    {
        auto str0 = BASE_NS::basic_fixed_string<char, 7>('A');
        str0 += 'B';
        str0.append('C');
        EXPECT_EQ(str0, "ABC");
    }
    {
        auto concat = []() {
            auto str0 = BASE_NS::basic_fixed_string<char, 7>('A');
            str0 += 'B';
            str0.append('C');
            return str0;
        };
        static constexpr const auto str0 = concat();
        EXPECT_EQ(str0, "ABC");
    }
}

template<typename T, typename U>
void fixed_string_compare()
{
    {
        auto str0 = T("");
        auto str1 = U("");
        EXPECT_TRUE(str0 == str1);
        EXPECT_FALSE(str0 != str1);
        EXPECT_FALSE(str1 != str0);
        EXPECT_FALSE(str0 < str1);
        EXPECT_FALSE(str1 < str0);
    }
    {
        auto str0 = T("1234567890123456789");
        auto str1 = U("1234567890123456789");
        EXPECT_TRUE(str0 == str1);
        EXPECT_FALSE(str0 != str1);
        EXPECT_FALSE(str1 != str0);
        EXPECT_FALSE(str0 < str1);
        EXPECT_FALSE(str1 < str0);
    }
    {
        auto str0 = T("ABCDEFGHIJKLMNOPQRSTU");
        auto str1 = U("ABCDEFGHIJKLMNOPQRSTU");
        EXPECT_TRUE(str0 == str1);
        EXPECT_FALSE(str0 != str1);
        EXPECT_FALSE(str1 != str0);
        EXPECT_FALSE(str0 < str1);
        EXPECT_FALSE(str1 < str0);
    }
    {
        auto str0 = T("");
        auto str1 = U("123");
        EXPECT_FALSE(str0 == str1);
        EXPECT_TRUE(str0 != str1);
        EXPECT_TRUE(str1 != str0);
        EXPECT_TRUE(str0 < str1);
        EXPECT_FALSE(str1 < str0);
    }
    {
        auto str0 = T("1234567890123456789");
        auto str1 = U("123");
        EXPECT_FALSE(str0 == str1);
        EXPECT_TRUE(str0 != str1);
        EXPECT_TRUE(str1 != str0);
        EXPECT_FALSE(str0 < str1);
        EXPECT_TRUE(str1 < str0);
    }
    {
        auto str0 = T("1234567890123456789");
        auto str1 = U("1234567890123456780");
        EXPECT_FALSE(str0 == str1);
        EXPECT_TRUE(str0 != str1);
        EXPECT_TRUE(str1 != str0);
        EXPECT_FALSE(str0 < str1);
        EXPECT_TRUE(str1 < str0);
    }
    {
        auto str0 = T("ABCDEFGHIJKLMNOPQRSTU");
        auto str1 = U("ABCDEFGHIJKLMNOPQRSTA");
        EXPECT_FALSE(str0 == str1);
        EXPECT_TRUE(str0 != str1);
        EXPECT_TRUE(str1 != str0);
        EXPECT_FALSE(str0 < str1);
        EXPECT_TRUE(str1 < str0);
    }
    {
        auto str0 = T("ABCDEFGHIJKLMNOPQRSTU");
        auto str1 = U("ABCDEF");
        EXPECT_FALSE(str0 == str1);
        EXPECT_TRUE(str0 != str1);
        EXPECT_TRUE(str1 != str0);
        EXPECT_FALSE(str0 < str1);
        EXPECT_TRUE(str1 < str0);
    }
}

/**
 * @tc.name: compare
 * @tc.desc: Tests for Compare. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, compare, testing::ext::TestSize.Level1)
{
    constexpr const char* TEST_STRING = "ABCDEF";
    constexpr const BASE_NS::string_view testString(TEST_STRING);
    constexpr const char* TEST_STRING2 = "FEDCBA";
    constexpr const BASE_NS::string_view testString2(TEST_STRING2);
    constexpr const char* TEST_STRING3 = "FED";
    constexpr const BASE_NS::string_view testString3(TEST_STRING3);

    EXPECT_TRUE(testString == testString);
    EXPECT_TRUE(testString == TEST_STRING);
    EXPECT_TRUE(TEST_STRING == testString);
    EXPECT_FALSE(testString == testString2);
    EXPECT_FALSE(testString == TEST_STRING2);
    EXPECT_FALSE(TEST_STRING == testString2);
    EXPECT_FALSE(testString2 == testString3);
    EXPECT_FALSE(testString2 == TEST_STRING3);
    EXPECT_FALSE(TEST_STRING2 == testString3);

    EXPECT_FALSE(testString != testString);
    EXPECT_FALSE(testString != TEST_STRING);
    EXPECT_FALSE(TEST_STRING != testString);
    EXPECT_TRUE(testString != testString2);
    EXPECT_TRUE(testString != TEST_STRING2);
    EXPECT_TRUE(TEST_STRING != testString2);
    EXPECT_TRUE(testString2 != testString3);
    EXPECT_TRUE(testString2 != TEST_STRING3);
    EXPECT_TRUE(TEST_STRING2 != testString3);

    fixed_string_compare<BASE_NS::basic_fixed_string<char, 30>, BASE_NS::basic_fixed_string<char, 30>>();
    fixed_string_compare<BASE_NS::basic_fixed_string<char, 30>, BASE_NS::basic_string_view<char>>();
}

template<typename T>
void findTest()
{
    constexpr const char* TEST_STRING = "ABCDEF";
    constexpr const T testString(TEST_STRING);
    constexpr auto notFound = T::npos;
    {
        constexpr const char TO_FIND = 'A';
        constexpr const auto POS = 0U;
        static_assert(testString.find(TO_FIND, POS) == 0U);
        EXPECT_EQ(testString.find(TO_FIND, POS), 0U);
        constexpr const auto POS2 = 2U;
        static_assert(testString.find(TO_FIND, POS2) == notFound);
        EXPECT_EQ(testString.find(TO_FIND, POS2), notFound);
        constexpr const auto POS3 = 100U;
        static_assert(testString.find(TO_FIND, POS3) == notFound);
        EXPECT_EQ(testString.find(TO_FIND, POS3), notFound);
    }

    {
        constexpr const char* TO_FIND = "ABC";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString.find(toFind, POS) == 0U);
        EXPECT_EQ(testString.find(toFind, POS), 0U);
    }

    {
        constexpr const char* TO_FIND = "F";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString.find(toFind, POS) == 5U);
        EXPECT_EQ(testString.find(toFind, POS), 5U);
    }

    {
        constexpr const char* TEST_STRING2 = "BAAAAB";
        constexpr const T testString2(TEST_STRING2);
        constexpr const char* TO_FIND = "AA";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString2.find(toFind, POS) == 1U);
        EXPECT_EQ(testString2.find(toFind, POS), 1U);
        constexpr const auto POS2 = 8U;
        static_assert(testString2.rfind(toFind, POS2) == 3U);
        EXPECT_EQ(testString2.rfind(toFind, POS2), 3U);
        static_assert(testString2.rfind('A', POS2) == 4U);
        EXPECT_EQ(testString2.rfind('A', POS2), 4U);
        static_assert(testString2.rfind('A', POS) == notFound);
        EXPECT_EQ(testString2.rfind('A', POS), notFound);
    }

    {
        constexpr const char* TO_FIND = "ZZ";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString.find(toFind, POS) == notFound);
        EXPECT_EQ(testString.find(toFind, POS), notFound);
    }

    {
        constexpr const char* TEST_STRING2 = "AAAAAA";
        constexpr const T testString2(TEST_STRING2);
        constexpr const char* TO_FIND = "AAAB";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString2.find(toFind, POS) == notFound);
        EXPECT_EQ(testString2.find(toFind, POS), notFound);
    }

    {
        constexpr const char* TEST_STRING2 = "AAAAAA";
        constexpr const T testString2(TEST_STRING2);
        constexpr const char* TO_FIND = "AB";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString2.find(toFind, POS) == notFound);
        EXPECT_EQ(testString2.find(toFind, POS), notFound);
    }

    {
        constexpr const char* TO_FIND = "ABCDEFG";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString.find(toFind, POS) == notFound);
        EXPECT_EQ(testString.find(toFind, POS), notFound);
        static_assert(testString.rfind(toFind, POS) == notFound);
        EXPECT_EQ(testString.rfind(toFind, POS), notFound);
    }

    {
        constexpr const char* TO_FIND = "EFG";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString.find(toFind, POS) == notFound);
        EXPECT_EQ(testString.find(toFind, POS), notFound);
    }

    {
        constexpr const char* TO_FIND = "";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 8U;
        static_assert(testString.rfind(toFind, POS) == 6U);
        EXPECT_EQ(testString.rfind(toFind, POS), 6U);
    }
    {
        constexpr const char* TO_FIND = "CD";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0U;
        static_assert(testString.rfind(toFind, POS) == notFound);
        EXPECT_EQ(testString.rfind(toFind, POS), notFound);
        static_assert(testString.find_first_of('B', POS) == 1U);
        EXPECT_EQ(testString.find_first_of('B', POS), 1U);
        constexpr const auto POS2 = 8U;
        static_assert(testString.find_first_of('B', POS2) == notFound);
        EXPECT_EQ(testString.find_first_of('B', POS2), notFound);
        static_assert(testString.find_first_of('G', POS) == notFound);
        EXPECT_EQ(testString.find_first_of('G', POS), notFound);
    }

    {
        constexpr const char* TEST_STRING_SINGLE = "ABC";
        constexpr const T testStringSingle(TEST_STRING_SINGLE);
        constexpr const char* TO_FIND = "CD";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0u;
        static_assert(testStringSingle.find(toFind, POS) == notFound);
        EXPECT_EQ(testStringSingle.find(toFind, POS), notFound);
    }

    {
        constexpr const char* TEST_STRING_SINGLE = "";
        constexpr const T testStringSingle(TEST_STRING_SINGLE);
        constexpr const char* TO_FIND = "CD";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        constexpr const auto POS = 0u;
        static_assert(testStringSingle.rfind(toFind, POS) == notFound);
        EXPECT_EQ(testStringSingle.rfind(toFind, POS), notFound);
        static_assert(testStringSingle.find_first_of(toFind, POS) == notFound);
        EXPECT_EQ(testStringSingle.find_first_of(toFind, POS), notFound);
        static_assert(testStringSingle.find_last_of(toFind, POS) == notFound);
        EXPECT_EQ(testStringSingle.find_last_of(toFind, POS), notFound);
        static_assert(testStringSingle.rfind('A', POS) == notFound);
        EXPECT_EQ(testStringSingle.rfind('A', POS), notFound);
        static_assert(testStringSingle.find_first_of('A', POS) == notFound);
        EXPECT_EQ(testStringSingle.find_first_of('A', POS), notFound);
        static_assert(testStringSingle.find_last_of('A', POS) == notFound);
        EXPECT_EQ(testStringSingle.find_last_of('A', POS), notFound);
    }
}

/**
 * @tc.name: find
 * @tc.desc: Tests for Find. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, find, testing::ext::TestSize.Level1)
{
    findTest<BASE_NS::string_view>();
    findTest<BASE_NS::fixed_string<20>>();
}

template<typename T>
void testFindFirstOfAndLastOf()
{
    constexpr const char* TEST_STRING = "ABCDEF";
    constexpr const T testString(TEST_STRING);
    constexpr auto notFound = T::npos;
    {
        constexpr const char* TO_FIND = "ABC";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        static_assert(testString.find_first_of(toFind, 0U) == 0U);
        EXPECT_EQ(testString.find_first_of(toFind, 0U), 0U);
        static_assert(testString.find_last_of(toFind, 0U) == 0U);
        EXPECT_EQ(testString.find_last_of(toFind, 0U), 0U);
    }

    {
        constexpr const char* TO_FIND = "CDE";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        static_assert(testString.find_first_of(toFind, 1U) == 2U);
        EXPECT_EQ(testString.find_first_of(toFind, 1U), 2U);
        static_assert(testString.find_last_of(toFind, 1U) == notFound);
        EXPECT_EQ(testString.find_last_of(toFind, 1U), notFound);
    }

    {
        constexpr const char* TO_FIND = "ABC";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        static_assert(testString.find_first_of(toFind, 8U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 8U), notFound);
        static_assert(testString.find_last_of(toFind, 8U) == 2U);
        EXPECT_EQ(testString.find_last_of(toFind, 8U), 2U);
    }

    {
        constexpr const char* TO_FIND = "ZZZZZZZA";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        static_assert(testString.find_first_of(toFind, 8U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 8U), notFound);
        static_assert(testString.find_last_of(toFind, 8U) == 0U);
        EXPECT_EQ(testString.find_last_of(toFind, 8U), 0U);
    }

    {
        constexpr const char* TO_FIND = "";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        static_assert(testString.find_first_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 0U), notFound);
        static_assert(testString.find_last_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_last_of(toFind, 0U), notFound);
    }

    {
        constexpr const char* TO_FIND = "ZZZ";
        constexpr const BASE_NS::string_view toFind(TO_FIND);
        static_assert(testString.find_first_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 0U), notFound);
        static_assert(testString.find_last_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_last_of(toFind, 0U), notFound);
    }

    {
        constexpr const char TO_FIND = 'A';
        static_assert(testString.find_first_of(TO_FIND, 0U) == 0U);
        EXPECT_EQ(testString.find_first_of(TO_FIND, 0U), 0U);
        static_assert(testString.find_last_of(TO_FIND, 0U) == 0U);
        EXPECT_EQ(testString.find_last_of(TO_FIND, 0U), 0U);
    }

    {
        constexpr const char TO_FIND = 'Z';
        static_assert(testString.find_first_of(TO_FIND, 0U) == notFound);
        EXPECT_EQ(testString.find_first_of(TO_FIND, 0U), notFound);
        static_assert(testString.find_last_of(TO_FIND, 0U) == notFound);
        EXPECT_EQ(testString.find_last_of(TO_FIND, 0U), notFound);
    }
}

/**
 * @tc.name: findFirstOfAndLastOf
 * @tc.desc: Tests for Find First Of And Last Of. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersFixedString, findFirstOfAndLastOf, testing::ext::TestSize.Level1)
{
    testFindFirstOfAndLastOf<BASE_NS::string_view>();
    testFindFirstOfAndLastOf<BASE_NS::fixed_string<20>>();
}

template<typename T>
void startWithTest()
{
    constexpr const char* TEST_STRING = "ABCDEF";
    constexpr const T testString(TEST_STRING);
    {
        constexpr const char* STARTS_WITH = "A";
        constexpr const BASE_NS::string_view startsWith(STARTS_WITH);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.starts_with(STARTS_WITH);
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.starts_with(*STARTS_WITH);
        EXPECT_TRUE(res3);
    }
    {
        constexpr const char* STARTS_WITH = "ABC";
        constexpr const BASE_NS::string_view startsWith(STARTS_WITH);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.starts_with(STARTS_WITH);
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.starts_with(*STARTS_WITH);
        EXPECT_TRUE(res3);
    }

    {
        constexpr const char* STARTS_WITH = "ABCDEFG";
        constexpr const BASE_NS::string_view startsWith(STARTS_WITH);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.starts_with(STARTS_WITH);
        EXPECT_FALSE(res2);
    }
    {
        constexpr const char* STARTS_WITH = "AbC";
        constexpr const BASE_NS::string_view startsWith(STARTS_WITH);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.starts_with(STARTS_WITH);
        EXPECT_FALSE(res2);
    }
    {
        constexpr const char* STARTS_WITH = "C";
        constexpr const BASE_NS::string_view startsWith(STARTS_WITH);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.starts_with(STARTS_WITH);
        EXPECT_FALSE(res2);
        constexpr const bool res3 = testString.starts_with(*STARTS_WITH);
        EXPECT_FALSE(res3);
    }
    {
        constexpr const bool res1 = testString.starts_with(nullptr);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.starts_with("");
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.starts_with(BASE_NS::string_view {});
        EXPECT_TRUE(res3);
    }
}

/**
 * @tc.name: startsWith
 * @tc.desc: Tests for Starts With. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, startsWith, testing::ext::TestSize.Level1)
{
    startWithTest<BASE_NS::string_view>();
}

template<typename T>
void endsWithTest()
{
    constexpr const char* TEST_STRING = "ABCDEF";
    constexpr const T testString(TEST_STRING);
    {
        constexpr const char* ENDS_WITH = "F";
        constexpr const BASE_NS::string_view endsWith(ENDS_WITH);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.ends_with(ENDS_WITH);
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.ends_with(*ENDS_WITH);
        EXPECT_TRUE(res3);
    }
    {
        constexpr const char* ENDS_WITH = "DEF";
        constexpr const BASE_NS::string_view endsWith(ENDS_WITH);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.ends_with(ENDS_WITH);
        EXPECT_TRUE(res2);
    }

    {
        constexpr const char* ENDS_WITH = "ABCDEFG";
        constexpr const BASE_NS::string_view endsWith(ENDS_WITH);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.ends_with(ENDS_WITH);
        EXPECT_FALSE(res2);
    }
    {
        constexpr const char* ENDS_WITH = "DeF";
        constexpr const BASE_NS::string_view endsWith(ENDS_WITH);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.ends_with(ENDS_WITH);
        EXPECT_FALSE(res2);
    }
    {
        constexpr const char* ENDS_WITH = "E";
        constexpr const BASE_NS::string_view endsWith(ENDS_WITH);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.ends_with(ENDS_WITH);
        EXPECT_FALSE(res2);
        constexpr const bool res3 = testString.ends_with(*ENDS_WITH);
        EXPECT_FALSE(res3);
    }
    {
        constexpr const bool res1 = testString.ends_with(nullptr);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.ends_with("");
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.ends_with(BASE_NS::string_view {});
        EXPECT_TRUE(res3);
    }
}

/**
 * @tc.name: endsWith
 * @tc.desc: Tests for Ends With. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, endsWith, testing::ext::TestSize.Level1)
{
    endsWithTest<BASE_NS::string_view>();
}

/**
 * @tc.name: UserDefinedLiteral_QualifiedNamespace
 * @tc.desc: Tests for User Defined Literalqualified Namespace. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, UserDefinedLiteral_QualifiedNamespace, testing::ext::TestSize.Level1)
{
    using namespace BASE_NS::literals::string_literals;
    auto myString = "hello"_sv;
    static_assert(BASE_NS::is_same_v<BASE_NS::string_view, decltype(myString)>);
    ASSERT_EQ(BASE_NS::string_view { "hello" }, myString);
}

/**
 * @tc.name: UserDefinedLiteral
 * @tc.desc: Tests for User Defined Literal. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, UserDefinedLiteral, testing::ext::TestSize.Level1)
{
    using namespace BASE_NS::literals;
    auto myString = "hello"_sv;
    static_assert(BASE_NS::is_same_v<BASE_NS::string_view, decltype(myString)>);
    ASSERT_EQ(BASE_NS::string_view { "hello" }, myString);
}

/**
 * @tc.name: UserDefinedLiteral_zeroBytes
 * @tc.desc: Tests for User Defined Literalzero Bytes. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersStringView, UserDefinedLiteral_zeroBytes, testing::ext::TestSize.Level1)
{
    using namespace BASE_NS::literals;
    auto myString = "hel\0lo"_sv;
    static_assert(BASE_NS::is_same_v<BASE_NS::string_view, decltype(myString)>);
    ASSERT_EQ(6, myString.size());
    ASSERT_EQ(BASE_NS::string_view("hel\0lo", 6), myString);
    ASSERT_STREQ("hel", myString.data());
}
