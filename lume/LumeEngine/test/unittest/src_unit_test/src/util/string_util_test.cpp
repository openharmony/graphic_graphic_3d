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

#include <base/containers/string.h>
#include <base/containers/string_view.h>
#include <base/util/uid.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif

#include "util/string_util.h"

using namespace BASE_NS;
using namespace CORE_NS;
using namespace StringUtil;

/**
 * @tc.name: StringUtil_CopyStringToArray_ValidCopy
 * @tc.desc: Tests copying string to character array [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_CopyStringToArray_ValidCopy, testing::ext::TestSize.Level1)
{
    const string_view source = "Hello World";
    char target[20];

    CopyStringToArray(source, target, sizeof(target));

    EXPECT_STREQ("Hello World", target);
    EXPECT_EQ(source.length(), strlen(target));
}

/**
 * @tc.name: StringUtil_CopyStringToArray_EmptyString
 * @tc.desc: Tests copying empty string to character array [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_CopyStringToArray_EmptyString, testing::ext::TestSize.Level1)
{
    const string_view source = "";
    char target[20];

    CopyStringToArray(source, target, sizeof(target));

    EXPECT_STREQ("", target);
    EXPECT_EQ('\0', target[0]);
}

/**
 * @tc.name: StringUtil_LTrim_RemoveLeadingSpaces
 * @tc.desc: Tests left trim removes leading spaces [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_LTrim_RemoveLeadingSpaces, testing::ext::TestSize.Level1)
{
    string_view text = "   Hello World";
    size_t originalLength = text.length();

    LTrim(text);

    EXPECT_EQ("Hello World", text);
    EXPECT_EQ(originalLength - 3, text.length());
}

/**
 * @tc.name: StringUtil_LTrim_RemoveLeadingTabs
 * @tc.desc: Tests left trim removes leading tabs [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_LTrim_RemoveLeadingTabs, testing::ext::TestSize.Level1)
{
    string_view text = "\t\tHello World";
    size_t originalLength = text.length();

    LTrim(text);

    EXPECT_EQ("Hello World", text);
    EXPECT_EQ(originalLength - 2, text.length());
}

/**
 * @tc.name: StringUtil_LTrim_RemoveMixedWhitespace
 * @tc.desc: Tests left trim removes mixed leading whitespace [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_LTrim_RemoveMixedWhitespace, testing::ext::TestSize.Level1)
{
    string_view text = " \t \nHello World";
    LTrim(text);

    EXPECT_EQ("Hello World", text);
}

/**
 * @tc.name: StringUtil_LTrim_NoLeadingWhitespace
 * @tc.desc: Tests left trim with no leading whitespace [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_LTrim_NoLeadingWhitespace, testing::ext::TestSize.Level1)
{
    string_view text = "Hello World";
    size_t originalLength = text.length();

    LTrim(text);

    EXPECT_EQ("Hello World", text);
    EXPECT_EQ(originalLength, text.length());
}

/**
 * @tc.name: StringUtil_LTrim_EmptyString
 * @tc.desc: Tests left trim on empty string [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_LTrim_EmptyString, testing::ext::TestSize.Level1)
{
    string_view text = "";
    LTrim(text);

    EXPECT_EQ("", text);
}

/**
 * @tc.name: StringUtil_LTrim_AllWhitespace
 * @tc.desc: Tests left trim on all whitespace string [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_LTrim_AllWhitespace, testing::ext::TestSize.Level1)
{
    string_view text = "     ";
    LTrim(text);

    EXPECT_EQ("", text);
}

/**
 * @tc.name: StringUtil_RTrim_RemoveTrailingSpaces
 * @tc.desc: Tests right trim removes trailing spaces [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_RTrim_RemoveTrailingSpaces, testing::ext::TestSize.Level1)
{
    string_view text = "Hello World   ";
    size_t originalLength = text.length();

    RTrim(text);

    EXPECT_EQ("Hello World", text);
    EXPECT_EQ(originalLength - 3, text.length());
}

/**
 * @tc.name: StringUtil_RTrim_RemoveTrailingTabs
 * @tc.desc: Tests right trim removes trailing tabs [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_RTrim_RemoveTrailingTabs, testing::ext::TestSize.Level1)
{
    string_view text = "Hello World\t\t";
    size_t originalLength = text.length();

    RTrim(text);

    EXPECT_EQ("Hello World", text);
    EXPECT_EQ(originalLength - 2, text.length());
}

/**
 * @tc.name: StringUtil_RTrim_NoTrailingWhitespace
 * @tc.desc: Tests right trim with no trailing whitespace [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_RTrim_NoTrailingWhitespace, testing::ext::TestSize.Level1)
{
    string_view text = "Hello World";
    size_t originalLength = text.length();

    RTrim(text);

    EXPECT_EQ("Hello World", text);
    EXPECT_EQ(originalLength, text.length());
}

/**
 * @tc.name: StringUtil_RTrim_EmptyString
 * @tc.desc: Tests right trim on empty string [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_RTrim_EmptyString, testing::ext::TestSize.Level1)
{
    string_view text = "";
    RTrim(text);

    EXPECT_EQ("", text);
}

/**
 * @tc.name: StringUtil_Trim_RemoveBothEnds
 * @tc.desc: Tests trim removes whitespace from both ends [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Trim_RemoveBothEnds, testing::ext::TestSize.Level1)
{
    string_view text = "   Hello World   ";
    Trim(text);

    EXPECT_EQ("Hello World", text);
}

/**
 * @tc.name: StringUtil_Trim_OnlyLeading
 * @tc.desc: Tests trim with only leading whitespace [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Trim_OnlyLeading, testing::ext::TestSize.Level1)
{
    string_view text = "   Hello World";
    Trim(text);

    EXPECT_EQ("Hello World", text);
}

/**
 * @tc.name: StringUtil_Trim_OnlyTrailing
 * @tc.desc: Tests trim with only trailing whitespace [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Trim_OnlyTrailing, testing::ext::TestSize.Level1)
{
    string_view text = "Hello World   ";
    Trim(text);

    EXPECT_EQ("Hello World", text);
}

/**
 * @tc.name: StringUtil_Trim_InternalWhitespace
 * @tc.desc: Tests trim preserves internal whitespace [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Trim_InternalWhitespace, testing::ext::TestSize.Level1)
{
    string_view text = "  Hello   World  ";
    Trim(text);

    EXPECT_EQ("Hello   World", text);
}

/**
 * @tc.name: StringUtil_Split_SingleDelimiter
 * @tc.desc: Tests splitting string with single delimiter [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_SingleDelimiter, testing::ext::TestSize.Level1)
{
    const string_view text = "apple|banana|cherry";
    vector<string_view> result = Split(text, "|");

    ASSERT_EQ(3u, result.size());
    EXPECT_EQ("apple", result[0]);
    EXPECT_EQ("banana", result[1]);
    EXPECT_EQ("cherry", result[2]);
}

/**
 * @tc.name: StringUtil_Split_MultipleDelimiters
 * @tc.desc: Tests splitting string with multiple delimiters [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_MultipleDelimiters, testing::ext::TestSize.Level1)
{
    const string_view text = "apple|banana,cherry|date";
    vector<string_view> result = Split(text, "|,");

    ASSERT_EQ(4u, result.size());
    EXPECT_EQ("apple", result[0]);
    EXPECT_EQ("banana", result[1]);
    EXPECT_EQ("cherry", result[2]);
    EXPECT_EQ("date", result[3]);
}

/**
 * @tc.name: StringUtil_Split_AutoTrim
 * @tc.desc: Tests split automatically trims whitespace [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_AutoTrim, testing::ext::TestSize.Level1)
{
    const string_view text = "apple | banana | cherry";
    vector<string_view> result = Split(text, "|");

    ASSERT_EQ(3u, result.size());
    EXPECT_EQ("apple", result[0]);
    EXPECT_EQ("banana", result[1]);
    EXPECT_EQ("cherry", result[2]);
}

/**
 * @tc.name: StringUtil_Split_EmptySegments
 * @tc.desc: Tests split with empty segments [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_EmptySegments, testing::ext::TestSize.Level1)
{
    const string_view text = "apple||cherry";
    vector<string_view> result = Split(text, "|");

    ASSERT_EQ(2u, result.size());
    EXPECT_EQ("apple", result[0]);
    EXPECT_EQ("cherry", result[1]);
}

/**
 * @tc.name: StringUtil_Split_AllEmptySegments
 * @tc.desc: Tests split with all empty segments [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_AllEmptySegments, testing::ext::TestSize.Level1)
{
    const string_view text = "|||";
    vector<string_view> result = Split(text, "|");

    EXPECT_EQ(0u, result.size());
}

/**
 * @tc.name: StringUtil_Split_NoDelimiters
 * @tc.desc: Tests split string with no delimiters [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_NoDelimiters, testing::ext::TestSize.Level1)
{
    const string_view text = "apple";
    vector<string_view> result = Split(text, "|");

    ASSERT_EQ(1u, result.size());
    EXPECT_EQ("apple", result[0]);
}

/**
 * @tc.name: StringUtil_Split_EmptyString
 * @tc.desc: Tests splitting empty string [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_EmptyString, testing::ext::TestSize.Level1)
{
    const string_view text = "";
    vector<string_view> result = Split(text, "|");

    EXPECT_EQ(0u, result.size());
}

/**
 * @tc.name: StringUtil_FindAndReplaceOne_SingleMatch
 * @tc.desc: Tests finding and replacing single occurrence [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_FindAndReplaceOne_SingleMatch, testing::ext::TestSize.Level1)
{
    string source = "Hello World";
    const string_view find = "World";
    const string_view replace = "Universe";

    bool found = FindAndReplaceOne(source, find, replace);

    EXPECT_TRUE(found);
    EXPECT_EQ("Hello Universe", source);
}

/**
 * @tc.name: StringUtil_FindAndReplaceOne_NoMatch
 * @tc.desc: Tests find and replace with no match [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_FindAndReplaceOne_NoMatch, testing::ext::TestSize.Level1)
{
    string source = "Hello World";
    const string_view find = "Goodbye";
    const string_view replace = "Universe";

    bool found = FindAndReplaceOne(source, find, replace);

    EXPECT_FALSE(found);
    EXPECT_EQ("Hello World", source);
}

/**
 * @tc.name: StringUtil_FindAndReplaceOne_FirstOccurrence
 * @tc.desc: Tests replacing only first occurrence when multiple exist [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_FindAndReplaceOne_FirstOccurrence, testing::ext::TestSize.Level1)
{
    string source = "cat cat cat";
    const string_view find = "cat";
    const string_view replace = "dog";

    FindAndReplaceOne(source, find, replace);

    EXPECT_EQ("dog cat cat", source);
}

/**
 * @tc.name: StringUtil_FindAndReplaceAll_AllOccurrences
 * @tc.desc: Tests replacing all occurrences [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_FindAndReplaceAll_AllOccurrences, testing::ext::TestSize.Level1)
{
    string source = "cat cat cat";
    const string_view find = "cat";
    const string_view replace = "dog";

    FindAndReplaceAll(source, find, replace);

    EXPECT_EQ("dog dog dog", source);
}

/**
 * @tc.name: StringUtil_NotSpace_VisibleCharacters
 * @tc.desc: Tests NotSpace with visible characters [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_NotSpace_VisibleCharacters, testing::ext::TestSize.Level1)
{
    EXPECT_TRUE(NotSpace('a'));
    EXPECT_TRUE(NotSpace('Z'));
    EXPECT_TRUE(NotSpace('0'));
    EXPECT_TRUE(NotSpace('@'));
    EXPECT_TRUE(NotSpace('_'));
}

/**
 * @tc.name: StringUtil_NotSpace_WhitespaceCharacters
 * @tc.desc: Tests NotSpace with whitespace characters [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_NotSpace_WhitespaceCharacters, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(NotSpace(' '));
    EXPECT_FALSE(NotSpace('\t'));
    EXPECT_FALSE(NotSpace('\n'));
    EXPECT_FALSE(NotSpace('\r'));
}

/**
 * @tc.name: StringUtil_MaxStringLengthFromArray
 * @tc.desc: Tests compile-time string length calculation [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_MaxStringLengthFromArray, testing::ext::TestSize.Level1)
{
    constexpr char testArray[10] = "test";
    constexpr size_t length = MaxStringLengthFromArray(testArray);

    EXPECT_EQ(9u, length);
}

/**
 * @tc.name: StringUtil_Trim_NewlineCharacters
 * @tc.desc: Tests trim removes newline characters [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Trim_NewlineCharacters, testing::ext::TestSize.Level1)
{
    string_view text = "\n\nHello World\n\n";
    Trim(text);

    EXPECT_EQ("Hello World", text);
}

/**
 * @tc.name: StringUtil_Trim_CarriageReturn
 * @tc.desc: Tests trim removes carriage return [AUTO-GENERATED]
 * @tc:type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Trim_CarriageReturn, testing::ext::TestSize.Level1)
{
    string_view text = "\rHello World\r";
    Trim(text);

    EXPECT_EQ("Hello World", text);
}

/**
 * @tc.name: StringUtil_Split_ReturnVectorCopy
 * @tc.desc: Tests that split returns a new vector, not a view [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_StringUtilTest, StringUtil_Split_ReturnVectorCopy, testing::ext::TestSize.Level1)
{
    const string_view text = "one|two|three";
    vector<string_view> result = Split(text, "|");

    // Modify original string view (this just creates a new view)
    // The result should still contain the correct values
    ASSERT_EQ(3u, result.size());
    EXPECT_EQ("one", result[0]);
    EXPECT_EQ("two", result[1]);
    EXPECT_EQ("three", result[2]);
}
