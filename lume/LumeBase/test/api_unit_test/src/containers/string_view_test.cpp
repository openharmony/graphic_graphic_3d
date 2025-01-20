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

#include <gtest/gtest.h>

#include <base/containers/array_view.h>
#include <base/containers/fixed_string.h>
#include <base/containers/string_view.h>

using namespace testing::ext;

class StringViewTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(StringViewTest, emptyStringView, TestSize.Level1)
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
        constexpr const char* testStringStr = "\0";
        constexpr const BASE_NS::string_view testString(testStringStr);

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStringStr), testString.size());

        const char* str = testStringStr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
    {
        constexpr const char testStringStr[] = { '\0' };
        const BASE_NS::string_view testString(testStringStr);

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStringStr), testString.size());

        const char* str = testStringStr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
}

HWTEST_F(StringViewTest, stringView001, TestSize.Level1)
{
    {
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const BASE_NS::string_view testString(testStringStr);

        EXPECT_FALSE(testString.empty());
        EXPECT_EQ(strlen(testStringStr), testString.size());
        EXPECT_EQ('B', testString[1]);
        EXPECT_EQ('B', testString.at(1));
        EXPECT_EQ(testString.max_size(), SIZE_MAX - 1);

        EXPECT_EQ(testString.front(), testString[0]);
        EXPECT_EQ(testString.back(), testString[testString.size() - 1]);

        EXPECT_NE(testString.begin(), testString.end());
        EXPECT_NE(testString.cbegin(), testString.cend());
        EXPECT_NE(testString.rbegin(), testString.rend());
        EXPECT_NE(testString.crbegin(), testString.crend());

        const char* str = testStringStr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
}

HWTEST_F(StringViewTest, stringView002, TestSize.Level1)
{
    {
        constexpr const char testStringStr[] = { 'A', 'B', 'C', 'D', 'E', 'F' };
        const BASE_NS::string_view testString(testStringStr, BASE_NS::countof(testStringStr));

        EXPECT_FALSE(testString.empty());
        EXPECT_EQ(sizeof(testStringStr), testString.size());
        EXPECT_EQ('B', testString[1]);
        EXPECT_EQ('B', testString.at(1));
        EXPECT_EQ(testString.front(), testString[0]);
        EXPECT_EQ(testString.back(), testString[testString.size() - 1]);

        EXPECT_NE(testString.begin(), testString.end());
        EXPECT_NE(testString.cbegin(), testString.cend());
        EXPECT_NE(testString.rbegin(), testString.rend());
        EXPECT_NE(testString.crbegin(), testString.crend());

        const char* str = testStringStr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }

        for (auto begin = testString.rbegin(), end = testString.rend(); begin != end; ++begin) {
            EXPECT_EQ(*--str, *begin);
        }
    }
}

HWTEST_F(StringViewTest, swap, TestSize.Level1)
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const BASE_NS::string_view testString(testStringStr);
    constexpr const char* testStringStr2 = "FEDCBA";
    constexpr const BASE_NS::string_view testString2(testStringStr2);
    BASE_NS::string_view t(testStringStr);
    BASE_NS::string_view t2(testStringStr2);
    EXPECT_EQ(testString, t);
    EXPECT_EQ(testString2, t2);
    t.swap(t2);
    EXPECT_EQ(testString, t2);
    EXPECT_EQ(testString2, t);
}

HWTEST_F(StringViewTest, subStringView001, TestSize.Level1)
{
    // remove_prefix
    {
        const char* testStringStr = "ABCDEF";
        const auto prefixLen = 3u;
        const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_prefix(len);
            return a;
        }(testStringStr, prefixLen);

        EXPECT_EQ(strlen(testStringStr) - prefixLen, testString.size());

        const char* str = testStringStr + prefixLen;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // remove_prefix huge
    {
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const auto prefixLen = 8u;
        constexpr const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_prefix(len);
            return a;
        }(testStringStr, prefixLen);

        EXPECT_EQ(0, testString.size());
    }
}

HWTEST_F(StringViewTest, subStringView002, TestSize.Level1)
{
    // remove_suffix
    {
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const auto suffixLen = 3u;
        constexpr const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_suffix(len);
            return a;
        }(testStringStr, suffixLen);

        EXPECT_EQ(strlen(testStringStr) - suffixLen, testString.size());

        const char* str = testStringStr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // remove_suffix huge
    {
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const auto suffixLen = 8u;
        constexpr const BASE_NS::string_view testString = [](const char* str, size_t len) {
            BASE_NS::string_view a = str;
            a.remove_suffix(len);
            return a;
        }(testStringStr, suffixLen);

        EXPECT_EQ(0, testString.size());
    }
}

HWTEST_F(StringViewTest, subStringView003, TestSize.Level1)
{
    // substr
    {
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const BASE_NS::string_view testString(testStringStr);

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
    }
}

HWTEST_F(StringViewTest, subStringView004, TestSize.Level1)
{
    {
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const BASE_NS::string_view testString(testStringStr);
        constexpr const BASE_NS::string_view prefix = testString.substr(0, 3);
        constexpr const BASE_NS::string_view suffix = testString.substr(3);

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
}

HWTEST_F(StringViewTest, subStringView005, TestSize.Level1)
{
    // fixed string copy test. Substr not implemented in fixed strings. Used string view instead.
    {
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const BASE_NS::fixed_string<6> testString(testStringStr);
        constexpr const BASE_NS::string_view testStringSW(testStringStr);
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

HWTEST_F(StringViewTest, FixedString_ToString, TestSize.Level1)
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

HWTEST_F(StringViewTest, FixedString_ToHex, TestSize.Level1)
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

HWTEST_F(StringViewTest, FixedString_Hash, TestSize.Level1)
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

HWTEST_F(StringViewTest, FixedString_Replace, TestSize.Level1)
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

HWTEST_F(StringViewTest, FixedString_Construct001, TestSize.Level1)
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
}

HWTEST_F(StringViewTest, FixedString_Construct002, TestSize.Level1)
{
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
void FixedStringCompare001()
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
}

template<typename T, typename U>
void FixedStringCompare002()
{
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

HWTEST_F(StringViewTest, FixedString_Compare, TestSize.Level1)
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const BASE_NS::string_view testString(testStringStr);
    constexpr const char* testStringStr2 = "FEDCBA";
    constexpr const BASE_NS::string_view testString2(testStringStr2);
    constexpr const char* testStringStr3 = "FED";
    constexpr const BASE_NS::string_view testString3(testStringStr3);

    EXPECT_TRUE(testString == testString);
    EXPECT_TRUE(testString == testStringStr);
    EXPECT_TRUE(testStringStr == testString);
    EXPECT_FALSE(testString == testString2);
    EXPECT_FALSE(testString == testStringStr2);
    EXPECT_FALSE(testStringStr == testString2);
    EXPECT_FALSE(testString2 == testString3);
    EXPECT_FALSE(testString2 == testStringStr3);
    EXPECT_FALSE(testStringStr2 == testString3);

    EXPECT_FALSE(testString != testString);
    EXPECT_FALSE(testString != testStringStr);
    EXPECT_FALSE(testStringStr != testString);
    EXPECT_TRUE(testString != testString2);
    EXPECT_TRUE(testString != testStringStr2);
    EXPECT_TRUE(testStringStr != testString2);
    EXPECT_TRUE(testString2 != testString3);
    EXPECT_TRUE(testString2 != testStringStr3);
    EXPECT_TRUE(testStringStr2 != testString3);

    FixedStringCompare001<BASE_NS::basic_fixed_string<char, 30>, BASE_NS::basic_fixed_string<char, 30>>();
    FixedStringCompare001<BASE_NS::basic_fixed_string<char, 30>, BASE_NS::basic_string_view<char>>();
    FixedStringCompare002<BASE_NS::basic_fixed_string<char, 30>, BASE_NS::basic_fixed_string<char, 30>>();
    FixedStringCompare002<BASE_NS::basic_fixed_string<char, 30>, BASE_NS::basic_string_view<char>>();
}

template<typename T>
void TestFindChar()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    constexpr auto notFound = T::npos;
    {
        constexpr const char toFindStr = 'A';
        constexpr const auto pos = 0U;
        static_assert(testString.find(toFindStr, pos) == 0U);
        EXPECT_EQ(testString.find(toFindStr, pos), 0U);

        constexpr const auto pos2 = 2U;
        static_assert(testString.find(toFindStr, pos2) == notFound);
        EXPECT_EQ(testString.find(toFindStr, pos2), notFound);

        constexpr const auto pos3 = 100U;
        static_assert(testString.find(toFindStr, pos3) == notFound);
        EXPECT_EQ(testString.find(toFindStr, pos3), notFound);
    }
}

template<typename T>
void TestFindString()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);

    {
        constexpr const char* toFindStr = "ABC";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;
        static_assert(testString.find(toFind, pos) == 0U);
        EXPECT_EQ(testString.find(toFind, pos), 0U);
    }
}

template<typename T>
void TestFindCharEnd()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);

    {
        constexpr const char* toFindStr = "F";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;
        static_assert(testString.find(toFind, pos) == 5U);
        EXPECT_EQ(testString.find(toFind, pos), 5U);
    }
}

template<typename T>
void TestFindSubString()
{
    {
        constexpr const char* testStringStr2 = "BAAAAB";
        constexpr const T testString2(testStringStr2);
        constexpr auto notFound = T::npos;
        constexpr const char* toFindStr = "AA";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;

        static_assert(testString2.find(toFind, pos) == 1U);
        EXPECT_EQ(testString2.find(toFind, pos), 1U);

        constexpr const auto pos2 = 8U;
        static_assert(testString2.rfind(toFind, pos2) == 3U);
        EXPECT_EQ(testString2.rfind(toFind, pos2), 3U);

        static_assert(testString2.rfind('A', pos2) == 4U);
        EXPECT_EQ(testString2.rfind('A', pos2), 4U);

        static_assert(testString2.rfind('A', pos) == notFound);
        EXPECT_EQ(testString2.rfind('A', pos), notFound);
    }
}

template<typename T>
void TestFindNotFound()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    constexpr auto notFound = T::npos;
    {
        constexpr const char* toFindStr = "ZZ";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;
        static_assert(testString.find(toFind, pos) == notFound);
        EXPECT_EQ(testString.find(toFind, pos), notFound);
    }
}

template<typename T>
void TestFindPartialMatch()
{
    {
        constexpr const char* testStringStr2 = "AAAAAA";
        constexpr const T testString2(testStringStr2);
        constexpr auto notFound = T::npos;
        constexpr const char* toFindStr = "AAAB";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;

        static_assert(testString2.find(toFind, pos) == notFound);
        EXPECT_EQ(testString2.find(toFind, pos), notFound);
    }
}

template<typename T>
void TestFindSubStringNotFound()
{
    {
        constexpr const char* testStringStr2 = "AAAAAA";
        constexpr const T testString2(testStringStr2);
        constexpr auto notFound = T::npos;
        constexpr const char* toFindStr = "AB";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;

        static_assert(testString2.find(toFind, pos) == notFound);
        EXPECT_EQ(testString2.find(toFind, pos), notFound);
    }
}

template<typename T>
void TestFindStringNotFound()
{
        constexpr const char* testStringStr = "ABCDEF";
        constexpr const T testString(testStringStr);
        constexpr auto notFound = T::npos;
        constexpr const char* toFindStr = "ABCDEFG";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;

        static_assert(testString.find(toFind, pos) == notFound);
        EXPECT_EQ(testString.find(toFind, pos), notFound);

        static_assert(testString.rfind(toFind, pos) == notFound);
        EXPECT_EQ(testString.rfind(toFind, pos), notFound);
}

template<typename T>
void TestFindSubStringNotPresent()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    constexpr auto notFound = T::npos;
    {
        constexpr const char* toFindStr = "EFG";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;

        static_assert(testString.find(toFind, pos) == notFound);
        EXPECT_EQ(testString.find(toFind, pos), notFound);
    }
}

template<typename T>
void TestFindEmptyString()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    {
        constexpr const char* toFindStr = "";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 8U;

        static_assert(testString.rfind(toFind, pos) == 6U);
        EXPECT_EQ(testString.rfind(toFind, pos), 6U);
    }
}

template<typename T>
void TestFindFirstOf()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    constexpr auto notFound = T::npos;
    {
        constexpr const char* toFindStr = "CD";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0U;

        EXPECT_EQ(testString.rfind(toFind, pos), notFound);

        static_assert(testString.find_first_of('B', pos) == 1U);
        EXPECT_EQ(testString.find_first_of('B', pos), 1U);

        constexpr const auto pos2 = 8U;
        static_assert(testString.find_first_of('B', pos2) == notFound);
        EXPECT_EQ(testString.find_first_of('B', pos2), notFound);

        static_assert(testString.find_first_of('G', pos) == notFound);
        EXPECT_EQ(testString.find_first_of('G', pos), notFound);
    }
}

template<typename T>
void TestSingleStringNotFound()
{
    {
        constexpr const char* testStringSingleStr = "ABC";
        constexpr const T testStringSingle(testStringSingleStr);
        constexpr auto notFound = T::npos;
        constexpr const char* toFindStr = "CD";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0u;

        static_assert(testStringSingle.find(toFind, pos) == notFound);
        EXPECT_EQ(testStringSingle.find(toFind, pos), notFound);
    }
}

template<typename T>
void TestEmptyStringOperations()
{
    {
        constexpr const char* testStringSingleStr = "";
        constexpr const T testStringSingle(testStringSingleStr);
        constexpr auto notFound = T::npos;
        constexpr const char* toFindStr = "CD";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        constexpr const auto pos = 0u;

        static_assert(testStringSingle.rfind(toFind, pos) == notFound);
        EXPECT_EQ(testStringSingle.rfind(toFind, pos), notFound);
        static_assert(testStringSingle.find_first_of(toFind, pos) == notFound);
        EXPECT_EQ(testStringSingle.find_first_of(toFind, pos), notFound);
        static_assert(testStringSingle.find_last_of(toFind, pos) == notFound);
        EXPECT_EQ(testStringSingle.find_last_of(toFind, pos), notFound);
        static_assert(testStringSingle.rfind('A', pos) == notFound);
        EXPECT_EQ(testStringSingle.rfind('A', pos), notFound);
        static_assert(testStringSingle.find_first_of('A', pos) == notFound);
        EXPECT_EQ(testStringSingle.find_first_of('A', pos), notFound);
        static_assert(testStringSingle.find_last_of('A', pos) == notFound);
        EXPECT_EQ(testStringSingle.find_last_of('A', pos), notFound);
    }
}

template<typename T>
void FindTest()
{
    TestFindChar<T>();
    TestFindString<T>();
    TestFindCharEnd<T>();
    TestFindSubString<T>();
    TestFindNotFound<T>();
    TestFindPartialMatch<T>();
    TestFindSubStringNotFound<T>();
    TestFindStringNotFound<T>();
    TestFindSubStringNotPresent<T>();
    TestFindEmptyString<T>();
    TestFindFirstOf<T>();
    TestSingleStringNotFound<T>();
    TestEmptyStringOperations<T>();
}

HWTEST_F(StringViewTest, FixedString_Find, TestSize.Level1)
{
    FindTest<BASE_NS::string_view>();
    FindTest<BASE_NS::fixed_string<20>>();
}

template<typename T>
void TestFindFirstOfAndLastOf001()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    constexpr auto notFound = T::npos;
    {
        constexpr const char* toFindStr = "ABC";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        static_assert(testString.find_first_of(toFind, 0U) == 0U);
        EXPECT_EQ(testString.find_first_of(toFind, 0U), 0U);
        static_assert(testString.find_last_of(toFind, 0U) == 0U);
        EXPECT_EQ(testString.find_last_of(toFind, 0U), 0U);
    }
    {
        constexpr const char* toFindStr = "CDE";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        static_assert(testString.find_first_of(toFind, 1U) == 2U);
        EXPECT_EQ(testString.find_first_of(toFind, 1U), 2U);
        static_assert(testString.find_last_of(toFind, 1U) == notFound);
        EXPECT_EQ(testString.find_last_of(toFind, 1U), notFound);
    }
    {
        constexpr const char* toFindStr = "ABC";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        static_assert(testString.find_first_of(toFind, 8U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 8U), notFound);
        static_assert(testString.find_last_of(toFind, 8U) == 2U);
        EXPECT_EQ(testString.find_last_of(toFind, 8U), 2U);
    }
}

template<typename T>
void TestFindFirstOfAndLastOf002()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    constexpr auto notFound = T::npos;
    {
        constexpr const char* toFindStr = "ZZZZZZZA";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        static_assert(testString.find_first_of(toFind, 8U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 8U), notFound);
        static_assert(testString.find_last_of(toFind, 8U) == 0U);
        EXPECT_EQ(testString.find_last_of(toFind, 8U), 0U);
    }
    {
        constexpr const char* toFindStr = "";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        static_assert(testString.find_first_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 0U), notFound);
        static_assert(testString.find_last_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_last_of(toFind, 0U), notFound);
    }
    {
        constexpr const char* toFindStr = "ZZZ";
        constexpr const BASE_NS::string_view toFind(toFindStr);
        static_assert(testString.find_first_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFind, 0U), notFound);
        static_assert(testString.find_last_of(toFind, 0U) == notFound);
        EXPECT_EQ(testString.find_last_of(toFind, 0U), notFound);
    }
    {
        constexpr const char toFindStr = 'A';
        static_assert(testString.find_first_of(toFindStr, 0U) == 0U);
        EXPECT_EQ(testString.find_first_of(toFindStr, 0U), 0U);
        static_assert(testString.find_last_of(toFindStr, 0U) == 0U);
        EXPECT_EQ(testString.find_last_of(toFindStr, 0U), 0U);
    }
    {
        constexpr const char toFindStr = 'Z';
        static_assert(testString.find_first_of(toFindStr, 0U) == notFound);
        EXPECT_EQ(testString.find_first_of(toFindStr, 0U), notFound);
        static_assert(testString.find_last_of(toFindStr, 0U) == notFound);
        EXPECT_EQ(testString.find_last_of(toFindStr, 0U), notFound);
    }
}

HWTEST_F(StringViewTest, FixedString_FindFirstOfAndLastOf, TestSize.Level1)
{
    TestFindFirstOfAndLastOf001<BASE_NS::string_view>();
    TestFindFirstOfAndLastOf001<BASE_NS::fixed_string<20>>();

    TestFindFirstOfAndLastOf002<BASE_NS::string_view>();
    TestFindFirstOfAndLastOf002<BASE_NS::fixed_string<20>>();
}

template<typename T>
void StartWithTest001()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    {
        constexpr const char* startsWithStr = "A";
        constexpr const BASE_NS::string_view startsWith(startsWithStr);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.starts_with(startsWithStr);
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.starts_with(*startsWithStr);
        EXPECT_TRUE(res3);
    }
    {
        constexpr const char* startsWithStr = "ABC";
        constexpr const BASE_NS::string_view startsWith(startsWithStr);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.starts_with(startsWithStr);
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.starts_with(*startsWithStr);
        EXPECT_TRUE(res3);
    }
    {
        constexpr const char* startsWithStr = "ABCDEFG";
        constexpr const BASE_NS::string_view startsWith(startsWithStr);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.starts_with(startsWithStr);
        EXPECT_FALSE(res2);
    }
}

template<typename T>
void StartWithTest002()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    {
        constexpr const char* startsWithStr = "AbC";
        constexpr const BASE_NS::string_view startsWith(startsWithStr);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.starts_with(startsWithStr);
        EXPECT_FALSE(res2);
    }
    {
        constexpr const char* startsWithStr = "C";
        constexpr const BASE_NS::string_view startsWith(startsWithStr);
        constexpr const bool res1 = testString.starts_with(startsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.starts_with(startsWithStr);
        EXPECT_FALSE(res2);
        constexpr const bool res3 = testString.starts_with(*startsWithStr);
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

HWTEST_F(StringViewTest, startsWith, TestSize.Level1)
{
    StartWithTest001<BASE_NS::string_view>();
    StartWithTest002<BASE_NS::string_view>();
}

template<typename T>
void EndsWithTest001()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    {
        constexpr const char* endsWithStr = "F";
        constexpr const BASE_NS::string_view endsWith(endsWithStr);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.ends_with(endsWithStr);
        EXPECT_TRUE(res2);
        constexpr const bool res3 = testString.ends_with(*endsWithStr);
        EXPECT_TRUE(res3);
    }
    {
        constexpr const char* endsWithStr = "DEF";
        constexpr const BASE_NS::string_view endsWith(endsWithStr);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_TRUE(res1);
        constexpr const bool res2 = testString.ends_with(endsWithStr);
        EXPECT_TRUE(res2);
    }
    {
        constexpr const char* endsWithStr = "ABCDEFG";
        constexpr const BASE_NS::string_view endsWith(endsWithStr);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.ends_with(endsWithStr);
        EXPECT_FALSE(res2);
    }
}

template<typename T>
void EndsWithTest002()
{
    constexpr const char* testStringStr = "ABCDEF";
    constexpr const T testString(testStringStr);
    {
        constexpr const char* endsWithStr = "DeF";
        constexpr const BASE_NS::string_view endsWith(endsWithStr);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.ends_with(endsWithStr);
        EXPECT_FALSE(res2);
    }
    {
        constexpr const char* endsWithStr = "E";
        constexpr const BASE_NS::string_view endsWith(endsWithStr);
        constexpr const bool res1 = testString.ends_with(endsWith);
        EXPECT_FALSE(res1);
        constexpr const bool res2 = testString.ends_with(endsWithStr);
        EXPECT_FALSE(res2);
        constexpr const bool res3 = testString.ends_with(*endsWithStr);
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

HWTEST_F(StringViewTest, endsWith, TestSize.Level1)
{
    EndsWithTest001<BASE_NS::string_view>();
    EndsWithTest002<BASE_NS::string_view>();
}

HWTEST_F(StringViewTest, UserDefinedLiteral_QualifiedNamespace, TestSize.Level1)
{
    using namespace BASE_NS::literals::string_literals;
    auto myString = "hello"_sv;
    static_assert(BASE_NS::is_same_v<BASE_NS::string_view, decltype(myString)>);
    ASSERT_EQ(BASE_NS::string_view { "hello" }, myString);
}

HWTEST_F(StringViewTest, UserDefinedLiteral, TestSize.Level1)
{
    using namespace BASE_NS::literals;
    auto myString = "hello"_sv;
    static_assert(BASE_NS::is_same_v<BASE_NS::string_view, decltype(myString)>);
    ASSERT_EQ(BASE_NS::string_view { "hello" }, myString);
}

HWTEST_F(StringViewTest, UserDefinedLiteral_zeroBytes, TestSize.Level1)
{
    using namespace BASE_NS::literals;
    auto myString = "hel\0lo"_sv;
    static_assert(BASE_NS::is_same_v<BASE_NS::string_view, decltype(myString)>);
    ASSERT_EQ(6, myString.size());
    ASSERT_EQ(BASE_NS::string_view("hel\0lo", 6), myString);
    ASSERT_STREQ("hel", myString.data());
}
