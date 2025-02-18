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

#include <base/containers/string.h>
#include <base/containers/type_traits.h>
#include <base/math/mathf.h>
#include <cstring>

using namespace testing::ext;

namespace {
constexpr const char* SHORT_STRING = "ABCDEF";
constexpr const char* LONG_STRING = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890"; // must exceed 30!!!
constexpr const char* SHORT_STRING_LOWERCASE = "abcdef";
constexpr const char* LONG_STRING_LOWERCASE = "abcdefghijklmnopqrstuvwxyz1234567890";

class CustomAllocator {
public:
    // simple linear allocator.
    uint8_t buf[1024u * sizeof(char)];
    size_t pos { 0 };
    void Reset()
    {
        pos = 0;
        memset_s(buf, sizeof(buf), 0, sizeof(buf));
    }
    void* alloc(BASE_NS::allocator::size_type size)
    {
        void* ret = &buf[pos];
        pos += size;
        return ret;
    }
    void Free(void*)
    {
        return;
    }
    static void* alloc(void* instance, BASE_NS::allocator::size_type size)
    {
        return ((CustomAllocator*)instance)->alloc(size);
    }
    static void Free(void* instance, void* ptr)
    {
        ((CustomAllocator*)instance)->Free(ptr);
    }
};

class ShortAllocator {
public:
    // simple linear allocator.
    uint8_t buf[64u * sizeof(char)];
    size_t pos { 0 };
    void Reset()
    {
        pos = 0;
        memset_s(buf, sizeof(buf), 0, sizeof(buf));
    }
    void* alloc(BASE_NS::allocator::size_type size)
    {
        void* ret = &buf[pos];
        pos += size;
        if (pos > 64u) {
            return nullptr;
        }
        return ret;
    }
    void Free(void*)
    {
        return;
    }
    static void* alloc(void* instance, BASE_NS::allocator::size_type size)
    {
        return ((ShortAllocator*)instance)->alloc(size);
    }
    static void Free(void* instance, void* ptr)
    {
        ((ShortAllocator*)instance)->Free(ptr);
    }
};
} // namespace

class StringTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(StringTest, EmptyString, TestSize.Level1)
{
    {
        const BASE_NS::string testString;

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(0, testString.size());

        EXPECT_EQ(testString.begin(), testString.end());
        EXPECT_EQ(testString.cbegin(), testString.cend());
    }
    {
        constexpr const char* testStrArr = "\0";
        const BASE_NS::string testString(testStrArr);

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStrArr), testString.size());

        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        constexpr const char testStrArr[] = { '\0' };
        const BASE_NS::string testString(testStrArr);

        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStrArr), testString.size());

        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        constexpr const char testStrArr[] = { '\0' };
        constexpr const char nonEmptyStrArr[] = { "AAAAA" };
        BASE_NS::string testString(nonEmptyStrArr);
        testString.clear();
        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStrArr), testString.size());

        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        constexpr const char testStrArr[] = { '\0' };
        constexpr const char nonEmptyStrArr[] = { "AAAAA" };
        BASE_NS::string testString(nonEmptyStrArr);
        testString.assign(0, 'A');
        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStrArr), testString.size());

        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        constexpr const char testStrArr[] = { '\0' };
        constexpr const char nonEmptyStrArr[] = { "AAAAA" };
        BASE_NS::string testString(nonEmptyStrArr);
        testString.resize(0);
        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStrArr), testString.size());

        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        constexpr const char testStrArr[] = { '\0' };
        BASE_NS::string testString(testStrArr);
        testString.resize(0);
        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(strlen(testStrArr), testString.size());

        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
}

HWTEST_F(StringTest, NonEmptyString, TestSize.Level1)
{
    {
        constexpr const char* testStrArr = "ABCDEF";
        const BASE_NS::string testString(testStrArr);

        EXPECT_FALSE(testString.empty());
        EXPECT_EQ(strlen(testStrArr), testString.size());
        EXPECT_EQ('B', testString[1]);
        EXPECT_EQ(testString.front(), testStrArr[0]);
        EXPECT_EQ(testString.back(), testStrArr[strlen(testStrArr) - 1]);
        {
            BASE_NS::string testString2(testStrArr);
            EXPECT_EQ('B', testString2[1]);
            EXPECT_EQ(testString2.front(), testStrArr[0]);
            EXPECT_EQ(testString2.back(), testStrArr[strlen(testStrArr) - 1]);
        }

        EXPECT_NE(testString.begin(), testString.end());
        EXPECT_NE(testString.cbegin(), testString.cend());
        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        constexpr const char* testStrArr = "ABCDEF";
        const BASE_NS::string testString(testStrArr, 1, 1);
        EXPECT_EQ(1, testString.length());
        EXPECT_EQ(*(testStrArr + 1), testString[0]);
    }
    {
        constexpr const char baseStrArr[] = { "CD" };
        constexpr const char testStrArr[] = { "CDAAA" };
        BASE_NS::string testString(baseStrArr);
        testString.resize(5, 'A');
        EXPECT_EQ(strlen(testStrArr), testString.size());
        const char* str = testStrArr;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
}

void AssingString(
    const char* initialValue, const char* assignedValue, BASE_NS::allocator& initial, BASE_NS::allocator& assigned)
{
    // copy assign
    {
        BASE_NS::string testString(initialValue, initial);
        BASE_NS::string testString2(assignedValue, assigned);
        testString = testString2;
        const char* str = assignedValue;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // move assign
    {
        BASE_NS::string testString(initialValue, initial);
        BASE_NS::string testString2(assignedValue, assigned);
        testString = BASE_NS::move(testString2);
        const char* str = assignedValue;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // c-str assign
    {
        BASE_NS::string testString(initialValue, initial);
        testString = assignedValue;
        const char* str = assignedValue;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // char assign
    {
        BASE_NS::string testString(initialValue, initial);
        testString = 'a';
        const char* str = "a";
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // string_view assign
    {
        BASE_NS::string testString(initialValue, initial);
        testString = BASE_NS::string_view(assignedValue);
        const char* str = assignedValue;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    // assign(c-str)
    {
        BASE_NS::string testString(initialValue, initial);
        testString.assign(assignedValue);
        const char* str = assignedValue;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
}

void AssingString(BASE_NS::allocator& initial, BASE_NS::allocator& assigned)
{
    // assignment operators to an empty string
    AssingString("", SHORT_STRING, initial, assigned);

    // assignment operators to an empty string
    AssingString("", LONG_STRING, initial, assigned);

    // assignment operators to a non-empty short string
    AssingString(SHORT_STRING_LOWERCASE, SHORT_STRING, initial, assigned);

    // assignment operators to a non-empty short string
    AssingString(SHORT_STRING_LOWERCASE, LONG_STRING, initial, assigned);

    // assignment operators to a non-empty long string
    AssingString(LONG_STRING_LOWERCASE, SHORT_STRING, initial, assigned);

    // assignment operators to a non-empty long string
    AssingString(LONG_STRING_LOWERCASE, LONG_STRING, initial, assigned);
}

HWTEST_F(StringTest, Assign, TestSize.Level1)
{
    AssingString(BASE_NS::default_allocator(), BASE_NS::default_allocator());
    {
        CustomAllocator custom;
        BASE_NS::allocator alloc { &custom, CustomAllocator::alloc, CustomAllocator::Free };
        AssingString(alloc, BASE_NS::default_allocator());
    }
    {
        CustomAllocator custom;
        BASE_NS::allocator alloc { &custom, CustomAllocator::alloc, CustomAllocator::Free };
        AssingString(BASE_NS::default_allocator(), alloc);
    }
    {
        CustomAllocator custom;
        BASE_NS::allocator alloc { &custom, CustomAllocator::alloc, CustomAllocator::Free };
        AssingString(alloc, alloc);
    }
    {
        ShortAllocator customShort;
        char hugeCharArray[256] = "";
        BASE_NS::allocator alloc { &customShort, ShortAllocator::alloc, ShortAllocator::Free };
        memset_s(hugeCharArray, sizeof(hugeCharArray), 'A', 200);
        const char* p = &hugeCharArray[0];
        const BASE_NS::string valDefault = BASE_NS::string(p, 200);
        BASE_NS::string valCustom = BASE_NS::string(valDefault, alloc);
        EXPECT_EQ(valCustom.size(), 0);
        EXPECT_EQ(valCustom.capacity(), 14); // const empty short string capacity
    }
    {
        ShortAllocator customShort;
        char hugeCharArray[256] = "";
        BASE_NS::allocator alloc { &customShort, ShortAllocator::alloc, ShortAllocator::Free };
        memset_s(hugeCharArray, sizeof(hugeCharArray), 'A', 200);
        const char* p = &hugeCharArray[0];
        const BASE_NS::string valDefault = BASE_NS::string(p, 200);
        BASE_NS::string valCustom(hugeCharArray, alloc);
        EXPECT_EQ(valCustom.size(), 0);
        EXPECT_EQ(valCustom.capacity(), 14); // const empty short string capacity
    }
}

HWTEST_F(StringTest, ReserveAndShrink, TestSize.Level1)
{
    constexpr const BASE_NS::string::size_type newCapacity = 100u;
    {
        BASE_NS::string testString;
        EXPECT_LT(testString.capacity(), newCapacity);
        testString.reserve(newCapacity);
        EXPECT_GE(testString.capacity(), newCapacity);
        testString = SHORT_STRING;
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        EXPECT_LT(testString.capacity(), newCapacity);
        testString.reserve(newCapacity);
        EXPECT_GE(testString.capacity(), newCapacity);
        // short content is preserved after reserve
        const char* str = SHORT_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        BASE_NS::string testString(LONG_STRING);
        EXPECT_LT(testString.capacity(), newCapacity);
        testString.reserve(newCapacity);
        EXPECT_GE(testString.capacity(), newCapacity);
        // long content is preserved after reserve
        const char* str = LONG_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        EXPECT_LT(testString.capacity(), newCapacity);
        testString.reserve(newCapacity);
        const auto newCapacity = testString.capacity();
        EXPECT_GE(newCapacity, newCapacity);
        // assigning content that fits in capacity doesn't increase capacity
        ASSERT_LT(strlen(LONG_STRING), newCapacity);
        testString = LONG_STRING;
        EXPECT_EQ(newCapacity, testString.capacity());
        const char* str = LONG_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
    {
        BASE_NS::string testString;
        testString.reserve(newCapacity);
        testString = LONG_STRING;
        EXPECT_GE(testString.capacity(), newCapacity);

        // verify that the capacity of a copy constructed string is based on the strlen instead of the 'other' capacity.
        BASE_NS::string testString2 = testString;
        EXPECT_LT(testString2.capacity(), testString.capacity());
        EXPECT_GE(testString2.capacity(), testString2.size());
    }
}

void VerifyInsertResult(
    BASE_NS::string const& testString, const char* initialValue, const char* appendValue, int pos = 0, int len = -1)
{
    auto const initialSize = static_cast<int>(strlen(initialValue));
    auto const appendSize = static_cast<int>(strlen(appendValue));
    if (len == -1) {
        len = appendSize;
    }
    if (pos > initialSize) {
        pos = initialSize;
    }
    const char* initialStr = initialValue;
    const char* appendStr = appendValue;
    const char* newStr = testString.c_str();
    for (int i = 0; i < pos; ++i) {
        EXPECT_EQ(*initialStr++, *newStr++);
    }
    for (int i = 0; i < len; ++i) {
        EXPECT_EQ(*appendStr++, *newStr++);
    }
    for (int i = pos; i < initialSize; ++i) {
        EXPECT_EQ(*initialStr++, *newStr++);
    }
}

HWTEST_F(StringTest, Insert, TestSize.Level1)
{
    // NOTE: need real tests. this is just a smoke test to call each insert overload once.
    const BASE_NS::string insertString(SHORT_STRING_LOWERCASE);
    constexpr BASE_NS::string_view insertStringView(SHORT_STRING_LOWERCASE);
    {
        BASE_NS::string testString(SHORT_STRING);
        testString.insert(3, insertStringView.data());
        VerifyInsertResult(testString, SHORT_STRING, SHORT_STRING_LOWERCASE, 3);
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        testString.insert(3, LONG_STRING_LOWERCASE);
        VerifyInsertResult(testString, SHORT_STRING, LONG_STRING_LOWERCASE, 3);
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        testString.insert(3, SHORT_STRING_LOWERCASE, 2);
        VerifyInsertResult(testString, SHORT_STRING, SHORT_STRING_LOWERCASE, 3, 2);
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        testString.insert(3, LONG_STRING_LOWERCASE, 16);
        VerifyInsertResult(testString, SHORT_STRING, LONG_STRING_LOWERCASE, 3, 16);
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        testString.insert(99, SHORT_STRING_LOWERCASE, 2);
        VerifyInsertResult(testString, SHORT_STRING, SHORT_STRING_LOWERCASE, 99, 2);
    }
    {
        BASE_NS::string testString(LONG_STRING);
        testString.insert(99, LONG_STRING_LOWERCASE, 16);
        VerifyInsertResult(testString, LONG_STRING, LONG_STRING_LOWERCASE, 99, 16);
    }
}

void VerifyEraseResult(
    BASE_NS::string const& testString, const char* initialValue, const size_t index, const size_t count)
{
    auto const initialSize = strlen(initialValue);
    const char* originalStr = initialValue;
    const char* newStr = testString.c_str();

    for (size_t i = 0; i < index; ++i) {
        EXPECT_EQ(*originalStr++, *newStr++);
    }
    if (count != BASE_NS::string::npos) {
        originalStr += count;
        for (size_t i = index + count; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
}

void Erase(const char* initialValue)
{
    auto const initialSize = strlen(initialValue);

    {
        BASE_NS::string testString(initialValue);
        // default index is 0 and count npos so should result in empty string
        testString.erase();
        EXPECT_TRUE(testString.empty());
        EXPECT_EQ(0, testString.size());
        VerifyEraseResult(testString, initialValue, 0, BASE_NS::string::npos);
    }
    for (size_t index = 0; index < initialSize; ++index) {
        {
            BASE_NS::string testString(initialValue);
            // index + npos should result in size equal to index
            testString.erase(index);
            EXPECT_EQ(index, testString.size());
            VerifyEraseResult(testString, initialValue, index, BASE_NS::string::npos);
        }
        {
            BASE_NS::string testString(initialValue);
            // erasing index..size should result in size equal to index
            const size_t count = testString.size() - index;
            testString.erase(index, count);
            EXPECT_EQ(index, testString.size());
            VerifyEraseResult(testString, initialValue, index, count);
        }
        {
            BASE_NS::string testString(initialValue);
            // erasing index..size - 1 should result in size equal to index + 1
            const size_t count = testString.size() - index - 1;
            testString.erase(index, count);
            EXPECT_EQ(index + 1, testString.size());
            VerifyEraseResult(testString, initialValue, index, count);
        }
        {
            BASE_NS::string testString(initialValue);
            // same as erase(index, 1)
            const auto it =
                testString.erase(testString.begin() + static_cast<std::string::iterator::difference_type>(index));
            EXPECT_EQ(initialSize - 1, testString.size());
            VerifyEraseResult(testString, initialValue, index, 1);
            // iterator poinst to the character after the erased character
            EXPECT_EQ(*(initialValue + index + 1), *it);
        }
        {
            BASE_NS::string testString(initialValue);
            // erasing index..size - 1 should result in size equal to index + 1
            const auto begin = testString.begin() + static_cast<std::string::iterator::difference_type>(index);
            const auto end = testString.end() - 1;
            const auto len = (end - begin);
            const auto it = testString.erase(begin, end);
            EXPECT_EQ(index + 1, testString.size());
            VerifyEraseResult(testString, initialValue, static_cast<size_t>(index), static_cast<size_t>(len));
            EXPECT_EQ(*(initialValue + index + len), *it);
        }
    }
    {
        {
            BASE_NS::string testString(initialValue);
            const auto it = testString.erase(initialSize + 100, initialSize + 200);
            EXPECT_EQ(initialSize, testString.size());
        }
    }
}

HWTEST_F(StringTest, Erase, TestSize.Level1)
{
    Erase(SHORT_STRING);
    Erase(LONG_STRING);
}

void AppendString(const char* initialValue, const char* assignedValue)
{
    // append n x char
    {
        BASE_NS::string testString(initialValue);
        auto const initialSize = testString.size();
        // appending 3/2 of capacity caused
        auto const cnt = 3 * testString.capacity() / 2;
        testString.append(cnt, 'a');
        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ('a', *newStr++);
        }
    }
    // append string
    {
        BASE_NS::string testString(initialValue);
        BASE_NS::string testString2(assignedValue);

        auto const initialSize = testString.size();
        auto const cnt = testString2.size();

        testString.append(testString2);
        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        originalStr = assignedValue;
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
    // append substring
    {
        BASE_NS::string testString(initialValue);
        BASE_NS::string testString2(assignedValue);

        auto const initialSize = testString.size();
        auto const posValue = 1u;
        auto const assingLen = strlen(assignedValue);
        auto const cnt = BASE_NS::Math::min(assingLen, size_t(2u));

        testString.append(testString2, posValue, cnt);

        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        originalStr = assignedValue + posValue;
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
    // append subsring size = -1
    {
        BASE_NS::string testString(initialValue);
        BASE_NS::string testString2(assignedValue);

        auto const initialSize = testString.size();
        auto const posValue = 1u;
        auto const assingLen = strlen(assignedValue);
        auto const cnt = (assingLen >= posValue) ? (assingLen - posValue) : assingLen;

        testString.append(testString2, posValue, static_cast<BASE_NS::string::size_type>(-1));
        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        originalStr = assignedValue + posValue;
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
    // append c-str
    {
        BASE_NS::string testString(initialValue);
        auto const initialSize = testString.size();
        auto const assingLen = strlen(assignedValue);
        auto const cnt = BASE_NS::Math::min(assingLen, size_t(2u));

        testString.append(assignedValue, cnt);

        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        originalStr = assignedValue;
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
    // append c-str
    {
        BASE_NS::string testString(initialValue);
        auto const initialSize = testString.size();
        auto const cnt = strlen(assignedValue);

        testString.append(assignedValue);
        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        originalStr = assignedValue;
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
    // append string_view
    {
        BASE_NS::string testString(initialValue);
        BASE_NS::string_view testString2(assignedValue);
        auto const initialSize = testString.size();
        auto const cnt = testString2.size();

        testString.append(testString2);
        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        originalStr = assignedValue;
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
    // append string_view
    {
        BASE_NS::string testString(initialValue);
        BASE_NS::string_view testString2(assignedValue);
        auto const initialSize = testString.size();
        auto const posValue = 1u;
        auto const cnt = BASE_NS::Math::min(testString2.size(), size_t(2u));

        testString.append(testString2, posValue, cnt);

        ASSERT_EQ(initialSize + cnt, testString.size());

        const char* originalStr = initialValue;
        const char* newStr = testString.c_str();
        for (size_t i = 0; i < initialSize; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
        originalStr = assignedValue + posValue;
        for (size_t i = 0; i < cnt; ++i) {
            EXPECT_EQ(*originalStr++, *newStr++);
        }
    }
}

HWTEST_F(StringTest, MemberAppend, TestSize.Level1)
{
    BASE_NS::string testString;
    BASE_NS::string testString2;
    testString.append(testString2, 0);
    testString.append(testString2, 0, 0);
    testString.append(testString2, 0, 1);
    testString.append(testString2, 1);
    testString.append(testString2, 1, 0);
    testString.append(testString2, 1, 1);
    testString2 = 'A';

    testString.append(testString2, 0);
    testString.append(testString2, 0, 0);
    testString.append(testString2, 0, 1);
    testString.append(testString2, 1);
    testString.append(testString2, 1, 0);
    testString.append(testString2, 1, 1);

    AppendString("", "");
    AppendString("", SHORT_STRING);
    AppendString("", LONG_STRING_LOWERCASE);

    AppendString(SHORT_STRING, "");
    AppendString(SHORT_STRING, SHORT_STRING);
    AppendString(SHORT_STRING, LONG_STRING_LOWERCASE);

    AppendString(LONG_STRING, "");
    AppendString(LONG_STRING, SHORT_STRING);
    AppendString(LONG_STRING, LONG_STRING_LOWERCASE);
}

void VerifyAppendResult(
    BASE_NS::string const& testString, const char* initialValue, const char* appendValue, const bool result = true)
{
    auto const initialSize = strlen(initialValue);
    auto const appendSize = strlen(appendValue);
    const char* originalStr = initialValue;
    const char* newStr = testString.c_str();
    int errCnt = 0;
    for (size_t i = 0; i < initialSize; ++i) {
        if (result) {
            EXPECT_EQ(*originalStr++, *newStr++);
        } else {
            errCnt += (*originalStr++ != *newStr++);
        }
    }
    originalStr = appendValue;
    for (size_t i = 0; i < appendSize; ++i) {
        if (result) {
            EXPECT_EQ(*originalStr++, *newStr++);
        } else {
            errCnt += (*originalStr++ != *newStr++);
        }
    }
    EXPECT_TRUE((errCnt == 0) == result);
}

void NonMemberAppend(const char* initialValue, const char* appendValue)
{
    //  string lref + string lref
    {
        const BASE_NS::string lhs(initialValue);
        const BASE_NS::string rhs(appendValue);
        auto const testString = lhs + rhs;
        VerifyAppendResult(testString, initialValue, appendValue);
    }
    //  string lref + const char
    {
        const BASE_NS::string lhs(initialValue);
        BASE_NS::string rhs(appendValue);
        auto testString = lhs;
        for (const char c : rhs) {
            testString += c;
        }
        VerifyAppendResult(testString, initialValue, appendValue);
    }
    //  string lref + char pointer
    {
        const BASE_NS::string lhs(initialValue);
        const char* rhs(appendValue);
        auto testString = lhs;
        testString += rhs;
        VerifyAppendResult(testString, initialValue, appendValue);
    }
    //  string lref + c-str
    {
        const BASE_NS::string lhs(initialValue);
        auto const testString = lhs + appendValue;
        VerifyAppendResult(testString, initialValue, appendValue);
    }
    /*
    basic_string<CharT> operator+(const basic_string<CharT>& lhs, CharT rhs);
    basic_string<CharT> operator+(const CharT* lhs, const basic_string<CharT>& rhs);
    basic_string<CharT> operator+(CharT lhs, const basic_string<CharT>& rhs);
    */
    //  string rref + string rref
    {
        auto const testString = BASE_NS::string(initialValue) + BASE_NS::string(appendValue);
        VerifyAppendResult(testString, initialValue, appendValue);
    }
    //  string rref + string lref
    {
        const BASE_NS::string rhs(appendValue);
        auto const testString = BASE_NS::string(initialValue) + rhs;
        VerifyAppendResult(testString, initialValue, appendValue);
    }
    //  string rref + c-str
    {
        auto const testString = BASE_NS::string(initialValue) + appendValue;
        VerifyAppendResult(testString, initialValue, appendValue);
    }
    //  string rref + char
    {
        auto const testString = BASE_NS::string(initialValue) + 'A';
        VerifyAppendResult(testString, initialValue, "A");
    }
    /*
    basic_string<CharT> operator+(const basic_string<CharT>& lhs, basic_string<CharT>&& rhs);
    basic_string<CharT> operator+(const CharT* lhs, basic_string<CharT>&& rhs);
    basic_string<CharT> operator+(CharT lhs, basic_string<CharT>&& rhs);
    */
}

HWTEST_F(StringTest, NonMemberAppend, TestSize.Level1)
{
    NonMemberAppend("", "");
    NonMemberAppend("", SHORT_STRING);
    NonMemberAppend("", LONG_STRING);

    NonMemberAppend(SHORT_STRING, "");
    NonMemberAppend(SHORT_STRING, SHORT_STRING);
    NonMemberAppend(SHORT_STRING, LONG_STRING);

    NonMemberAppend(LONG_STRING, "");
    NonMemberAppend(LONG_STRING, SHORT_STRING);
    NonMemberAppend(LONG_STRING, LONG_STRING);
}
template<typename AllT>
void CustomAllocatorAppend(const char* initialValue, const char* appendValue, BASE_NS::allocator alloc,
    AllT* customAllocator, bool expectedResult = true)
{
    //  append
    {
        customAllocator->Reset();
        const BASE_NS::string lhs(initialValue);
        const BASE_NS::string rhs(appendValue);
        BASE_NS::string testString = BASE_NS::string(lhs, alloc);
        testString += rhs;
        VerifyAppendResult(testString, initialValue, appendValue, expectedResult);
    }
    //  push_back
    {
        if (strlen(appendValue)) {
            customAllocator->Reset();
            const BASE_NS::string lhs(initialValue);
            BASE_NS::string testString = BASE_NS::string(lhs, alloc);
            testString.push_back('A');
            VerifyAppendResult(testString, initialValue, "A", expectedResult);
        }
    }
    //  append chars
    {
        if (strlen(appendValue)) {
            customAllocator->Reset();
            const BASE_NS::string lhs(initialValue);
            BASE_NS::string testString = BASE_NS::string(lhs, alloc);
            testString.append(5, 'A'); // 5: index
            VerifyAppendResult(testString, initialValue, "AAAAA", expectedResult);
        }
    }
}

HWTEST_F(StringTest, CustomAllocatorAppend, TestSize.Level1)
{
    ShortAllocator customShort;
    BASE_NS::allocator alloc { &customShort, ShortAllocator::alloc, ShortAllocator::Free };
    CustomAllocatorAppend("", "", alloc, &customShort);
    CustomAllocatorAppend("", SHORT_STRING, alloc, &customShort);
    CustomAllocatorAppend("", LONG_STRING, alloc, &customShort);

    CustomAllocatorAppend(SHORT_STRING, "", alloc, &customShort);
    CustomAllocatorAppend(SHORT_STRING, SHORT_STRING, alloc, &customShort);
    CustomAllocatorAppend(SHORT_STRING, LONG_STRING, alloc, &customShort); // 6 + 55 < 64

    CustomAllocatorAppend(LONG_STRING, "", alloc, &customShort);
    CustomAllocatorAppend(LONG_STRING, SHORT_STRING, alloc, &customShort, false); // 36 + 55 > 64
    CustomAllocatorAppend(LONG_STRING, LONG_STRING, alloc, &customShort, false);  // 36 + 36 + 36 > 64
}

void Compare(BASE_NS::string const& lhs, BASE_NS::string_view rhs)
{
    ASSERT_EQ(lhs.size(), rhs.size());

    auto iRhs = rhs.begin();

    for (auto const c : lhs) {
        EXPECT_EQ(*iRhs++, c);
    }
}

template<typename Lhs, typename Rhs>
void CompareTest(Lhs lhs, Rhs rhs)
{
    EXPECT_TRUE(lhs == lhs);
    EXPECT_FALSE(lhs == rhs);
    EXPECT_TRUE(lhs != rhs);
    EXPECT_TRUE(lhs < rhs);
    EXPECT_TRUE(lhs <= lhs);
    EXPECT_TRUE(lhs <= rhs);
    EXPECT_TRUE(rhs > lhs);
    EXPECT_TRUE(rhs >= rhs);
    EXPECT_TRUE(rhs >= lhs);
}

HWTEST_F(StringTest, ComparisonOperators, TestSize.Level1)
{
    const auto a = BASE_NS::string("A");
    const auto b = BASE_NS::string("B");
    // string - string comparison
    CompareTest(a, b);

    // string - string_view comparison
    CompareTest(a, BASE_NS::string_view(b));
    CompareTest(BASE_NS::string_view(a), b);

    // string_view - string_view comparison
    CompareTest(BASE_NS::string_view(a), BASE_NS::string_view(b));

    // string - c-string comparison
    CompareTest(a, b.data());
    CompareTest(a.data(), b);

    // string_view - c-string comparison
    CompareTest(BASE_NS::string_view(a), b.data());
    CompareTest(a.data(), BASE_NS::string_view(b));
}

HWTEST_F(StringTest, Compare, TestSize.Level1)
{
    constexpr const char* testStrArr = "ABCDEF";
    const BASE_NS::string testString(testStrArr);

    const BASE_NS::string copy = testString;
    const BASE_NS::string prefix = BASE_NS::string(testString.substr(0, 3));
    const BASE_NS::string suffix = BASE_NS::string(testString.substr(3));

    EXPECT_EQ(testString.compare(copy), 0);

    EXPECT_GT(testString.compare(prefix), 0);

    EXPECT_LT(testString.compare(suffix), 0);

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
}

void ReplaceString(
    const char* initialValue, const char* replacedValue, size_t index, size_t count, const char* resultValue)
{
    BASE_NS::string testString(initialValue);
    testString.replace(testString.begin() + static_cast<std::string::iterator::difference_type>(index),
        testString.begin() + static_cast<std::string::iterator::difference_type>(index + count), replacedValue);
    Compare(testString, resultValue);
}

void Replace(const char* initialValue, const char* replacedValue)
{
    const auto initialSize = strlen(initialValue);
    const auto replaceSize = strlen(replacedValue);
    char result[256];
    for (size_t index = 0; index <= initialSize; ++index) {
        BASE_NS::CloneData(result, sizeof(result), initialValue, index);
        BASE_NS::CloneData(result + index, sizeof(result) - index, replacedValue, replaceSize);

        for (size_t count = 0; count <= initialSize - index; ++count) {
            BASE_NS::CloneData(result + index + replaceSize, sizeof(result) - index - replaceSize,
                initialValue + index + count, initialSize - index - count);
            result[initialSize - count + replaceSize] = '\0';
            ReplaceString(initialValue, replacedValue, index, count, result);
        }
    }
}

HWTEST_F(StringTest, Replace, TestSize.Level1)
{
    Replace("", SHORT_STRING_LOWERCASE);
    Replace("", LONG_STRING_LOWERCASE);

    Replace(SHORT_STRING, SHORT_STRING_LOWERCASE);
    Replace(SHORT_STRING, LONG_STRING_LOWERCASE);

    Replace(LONG_STRING, SHORT_STRING_LOWERCASE);
    Replace(LONG_STRING, LONG_STRING_LOWERCASE);
}

HWTEST_F(StringTest, Copy, TestSize.Level1)
{
    {
        BASE_NS::string testString;
        char buffer[256];
        EXPECT_EQ(testString.copy(buffer, 0), 0);
        EXPECT_EQ(testString.copy(buffer, 1), 0);
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        char buffer[256] {};
        EXPECT_EQ(testString.copy(buffer, 3), 3);
        const char* str = SHORT_STRING;
        for (size_t i = 0; i < 3; ++i) {
            EXPECT_EQ(*str++, testString[i]);
        }
    }
    {
        BASE_NS::string testString(SHORT_STRING);
        char buffer[256] {};
        EXPECT_EQ(testString.copy(buffer, BASE_NS::countof(buffer)), testString.length());
        const char* str = SHORT_STRING;
        for (const auto c : testString) {
            EXPECT_EQ(*str++, c);
        }
    }
}

HWTEST_F(StringTest, Search, TestSize.Level1)
{
    {
        BASE_NS::string checkString(SHORT_STRING);
        BASE_NS::string testString("A");
        EXPECT_EQ(checkString.find(testString), 0);
        EXPECT_EQ(checkString.find(testString, 0), 0);
        EXPECT_EQ(checkString.find(testString, 1), -1);
        EXPECT_EQ(checkString.find('A'), 0);
        EXPECT_EQ(checkString.find("A"), 0);
        EXPECT_EQ(checkString.find('A', 1), -1);
        EXPECT_EQ(checkString.find("B", 1), 1);

        BASE_NS::string testString2("BC");
        EXPECT_EQ(checkString.find(testString2), 1);
        EXPECT_EQ(checkString.find(testString2, 0), 1);
        EXPECT_EQ(checkString.find(testString2, 2), -1);

        // ERROR?
        EXPECT_EQ(checkString.find(SHORT_STRING), 0);
        EXPECT_EQ(checkString.find(SHORT_STRING, 0), 0);
        EXPECT_EQ(checkString.find(SHORT_STRING, 2), -1);

        EXPECT_EQ(checkString.find(checkString), 0);
        EXPECT_EQ(checkString.find(checkString, 0), 0);
        EXPECT_EQ(checkString.find(checkString, 2), -1);

        EXPECT_EQ(checkString.find("F"), 5);
        EXPECT_EQ(checkString.find("F", 0), 5);
        EXPECT_EQ(checkString.find("F", 2), 5);

        EXPECT_EQ(checkString.find("Z"), -1);
        EXPECT_EQ(checkString.find("Z", 0), -1);
        EXPECT_EQ(checkString.find("Z", 2), -1);
    }
    {
        BASE_NS::string checkString(SHORT_STRING);
        BASE_NS::string testString("A");
        EXPECT_EQ(checkString.rfind(testString), 0);
        EXPECT_EQ(checkString.rfind(testString, 0), 0);
        EXPECT_EQ(checkString.rfind(testString, 1), 0);
        EXPECT_EQ(checkString.rfind('A'), 0);
        EXPECT_EQ(checkString.rfind("A"), 0);
        EXPECT_EQ(checkString.rfind('A', 1), 0);
        EXPECT_EQ(checkString.rfind("B", 1), 1);

        BASE_NS::string testString2("BC");
        EXPECT_EQ(checkString.rfind(testString2), 1);
        EXPECT_EQ(checkString.rfind(testString2, 0), -1);
        EXPECT_EQ(checkString.rfind(testString2, 2), 1);

        EXPECT_EQ(checkString.rfind(SHORT_STRING), 0);
        EXPECT_EQ(checkString.rfind(SHORT_STRING, 0), 0);
        EXPECT_EQ(checkString.rfind(SHORT_STRING, 2), 0);

        EXPECT_EQ(checkString.rfind(checkString), 0);
        EXPECT_EQ(checkString.rfind(checkString, 0), 0);
        EXPECT_EQ(checkString.rfind(checkString, 2), 0);

        EXPECT_EQ(checkString.rfind("F"), 5);
        EXPECT_EQ(checkString.rfind("F", 0), -1);
        EXPECT_EQ(checkString.rfind("F", 2), -1);

        EXPECT_EQ(checkString.rfind("Z"), -1);
        EXPECT_EQ(checkString.rfind("Z", 0), -1);
        EXPECT_EQ(checkString.rfind("Z", 2), -1);
    }
    {
        BASE_NS::string checkString(SHORT_STRING);
        checkString += SHORT_STRING;
        BASE_NS::string testString("A");
        EXPECT_EQ(checkString.find_first_of(testString), 0);
        EXPECT_EQ(checkString.find_first_of(testString, 0), 0);
        EXPECT_EQ(checkString.find_first_of(testString, 1), 6);
        EXPECT_EQ(checkString.find_first_of('A'), 0);
        EXPECT_EQ(checkString.find_first_of("A"), 0);
        EXPECT_EQ(checkString.find_first_of('A', 1), 6);
        EXPECT_EQ(checkString.find_first_of("B", 1), 1);

        BASE_NS::string testString2("BC");
        EXPECT_EQ(checkString.find_first_of(testString2), 1);
        EXPECT_EQ(checkString.find_first_of(testString2, 0), 1);
        EXPECT_EQ(checkString.find_first_of(testString2, 2), 2); // C == C
        EXPECT_EQ(checkString.find_first_of(testString2, 3), 7);

        // ERROR?
        EXPECT_EQ(checkString.find_first_of(SHORT_STRING), 0);
        EXPECT_EQ(checkString.find_first_of(SHORT_STRING, 0), 0);
        EXPECT_EQ(checkString.find_first_of(SHORT_STRING, 3), 3);

        EXPECT_EQ(checkString.find_first_of(checkString), 0);
        EXPECT_EQ(checkString.find_first_of(checkString, 0), 0);
        EXPECT_EQ(checkString.find_first_of(checkString, 2), 2);

        EXPECT_EQ(checkString.find_first_of("F"), 5);
        EXPECT_EQ(checkString.find_first_of("F", 0), 5);
        EXPECT_EQ(checkString.find_first_of("F", 2), 5);

        EXPECT_EQ(checkString.find_first_of("Z"), -1);
        EXPECT_EQ(checkString.find_first_of("Z", 0), -1);
        EXPECT_EQ(checkString.find_first_of("Z", 2), -1);
    }
    {
        BASE_NS::string checkString(SHORT_STRING);
        checkString += SHORT_STRING;
        BASE_NS::string testString("A");
        EXPECT_EQ(checkString.find_last_of(testString), 6);
        EXPECT_EQ(checkString.find_last_of(testString, 0), 0);
        EXPECT_EQ(checkString.find_last_of(testString, 1), 0);
        EXPECT_EQ(checkString.find_last_of('A'), 6);
        EXPECT_EQ(checkString.find_last_of("A"), 6);
        EXPECT_EQ(checkString.find_last_of('A', 1), 0);
        EXPECT_EQ(checkString.find_last_of("B", 1), 1);

        BASE_NS::string testString2("BC");
        EXPECT_EQ(checkString.find_last_of(testString2), 8);
        EXPECT_EQ(checkString.find_last_of(testString2, 0), -1);
        EXPECT_EQ(checkString.find_last_of(testString2, 2), 2);

        // ERROR?
        EXPECT_EQ(checkString.find_last_of(SHORT_STRING), 11);
        EXPECT_EQ(checkString.find_last_of(SHORT_STRING, 0), 0);
        EXPECT_EQ(checkString.find_last_of(SHORT_STRING, 2), 2);

        EXPECT_EQ(checkString.find_last_of(checkString), 11);
        EXPECT_EQ(checkString.find_last_of(checkString, 0), 0);
        EXPECT_EQ(checkString.find_last_of(checkString, 2), 2);

        EXPECT_EQ(checkString.find_last_of("F"), 11);
        EXPECT_EQ(checkString.find_last_of("F", 0), -1);
        EXPECT_EQ(checkString.find_last_of("F", 2), -1);

        EXPECT_EQ(checkString.find_last_of("AF"), 11);
        EXPECT_EQ(checkString.find_last_of("AF", 0), 0);
        EXPECT_EQ(checkString.find_last_of("AF", 2), 0);

        EXPECT_EQ(checkString.find_last_of("Z"), -1);
        EXPECT_EQ(checkString.find_last_of("Z", 0), -1);
        EXPECT_EQ(checkString.find_last_of("Z", 2), -1);
    }
}

HWTEST_F(StringTest, Upper, TestSize.Level1)
{
    {
        BASE_NS::string checkString("");
        BASE_NS::string testString("");
        Compare(testString.toUpper(), checkString);
        testString.upper();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(SHORT_STRING);
        BASE_NS::string testString(SHORT_STRING);
        Compare(testString.toUpper(), checkString);
        testString.upper();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(SHORT_STRING);
        BASE_NS::string testString(SHORT_STRING_LOWERCASE);
        Compare(testString.toUpper(), checkString);
        testString.upper();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(LONG_STRING);
        BASE_NS::string testString(LONG_STRING);
        Compare(testString.toUpper(), checkString);
        testString.upper();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(LONG_STRING);
        BASE_NS::string testString(LONG_STRING_LOWERCASE);
        Compare(testString.toUpper(), checkString);
        testString.upper();
        Compare(testString, checkString);
    }
}

HWTEST_F(StringTest, Lower, TestSize.Level1)
{
    {
        BASE_NS::string checkString("");
        BASE_NS::string testString("");
        Compare(testString.toLower(), checkString);
        testString.lower();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(SHORT_STRING_LOWERCASE);
        BASE_NS::string testString(SHORT_STRING);
        Compare(testString.toLower(), checkString);
        testString.lower();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(SHORT_STRING_LOWERCASE);
        BASE_NS::string testString(SHORT_STRING_LOWERCASE);
        Compare(testString.toLower(), checkString);
        testString.lower();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(LONG_STRING_LOWERCASE);
        BASE_NS::string testString(LONG_STRING);
        Compare(testString.toLower(), checkString);
        testString.lower();
        Compare(testString, checkString);
    }
    {
        BASE_NS::string checkString(LONG_STRING_LOWERCASE);
        BASE_NS::string testString(LONG_STRING_LOWERCASE);
        Compare(testString.toLower(), checkString);
        testString.lower();
        Compare(testString, checkString);
    }
}

HWTEST_F(StringTest, OperatorPlus, TestSize.Level1)
{
    {
        BASE_NS::string testString("");
        BASE_NS::string checkString("A");
        testString = testString + 'A';
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string checkString("A");
        testString = 'A' + testString;
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string checkString(SHORT_STRING);
        testString = testString + SHORT_STRING;
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string checkString(SHORT_STRING);
        testString = SHORT_STRING + testString;
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string_view stringViewA("A");
        BASE_NS::string_view stringViewB("B");
        BASE_NS::string checkString("AB");
        testString = stringViewA + stringViewB;
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string_view stringViewA("A");
        BASE_NS::string checkString("AB");
        testString = stringViewA + 'B';
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string_view stringViewB("B");
        BASE_NS::string checkString("AB");
        testString = 'A' + stringViewB;
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string stringA("A");
        BASE_NS::string stringB("B");
        BASE_NS::string checkString("AABB");
        testString = (stringA + stringA) + (stringB + stringB);
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string stringA("A");
        BASE_NS::string checkString("AABB");
        testString = (stringA + stringA) + "BB";
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string stringA("A");
        BASE_NS::string checkString("AABB");
        testString = (stringA + stringA) + 'B' + 'B';
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string stringB("B");
        BASE_NS::string checkString("AABB");
        testString = "AA" + (stringB + stringB);
        Compare(testString, checkString);
    }
    {
        BASE_NS::string testString("");
        BASE_NS::string stringB("B");
        BASE_NS::string checkString("AABB");
        testString = 'A' + ('A' + (stringB + stringB));
        Compare(testString, checkString);
    }
}

HWTEST_F(StringTest, Substr, TestSize.Level1)
{
    static constexpr BASE_NS::string_view file = "file";
    static constexpr BASE_NS::string_view suffix = ".ext";

    // substr from beginning
    {
        BASE_NS::string path = file + suffix;

        BASE_NS::string path2;
        path2 = path.substr(0, path.length() - suffix.length());
        EXPECT_EQ(file, path2);
        // BASE_NS::string path3 = path.substr(0, path.length() - suffix.length())); // no go, missing operator..
        BASE_NS::string path3(path.substr(0, path.length() - suffix.length()));
        EXPECT_EQ(file, path3);
        path = path.substr(0, path.length() - suffix.length());
        EXPECT_EQ(file, path);
    }
    // substr from end
    {
        BASE_NS::string path = file + suffix;

        BASE_NS::string path2;
        path2 = path.substr(path.length() - suffix.length(), suffix.length());
        EXPECT_EQ(suffix, path2);
        BASE_NS::string path3(path.substr(path.length() - suffix.length(), suffix.length()));
        EXPECT_EQ(suffix, path3);
        path = path.substr(path.length() - suffix.length(), suffix.length());
        EXPECT_EQ(suffix, path);
    }
    // substr overlap
    {
        BASE_NS::string path = file + suffix;

        path = path.substr(1, path.length() - suffix.length() - 1);
        EXPECT_EQ(file.substr(1, BASE_NS::string_view::npos), path);
    }
}

HWTEST_F(StringTest, StartsWith, TestSize.Level1)
{
    BASE_NS::string checkString(SHORT_STRING);
    EXPECT_TRUE(checkString.starts_with(BASE_NS::string_view("A")));
    EXPECT_TRUE(checkString.starts_with("A"));
    EXPECT_TRUE(checkString.starts_with('A'));
    EXPECT_TRUE(checkString.starts_with(BASE_NS::string_view("ABC")));
    EXPECT_TRUE(checkString.starts_with("ABC"));
    EXPECT_FALSE(checkString.starts_with(BASE_NS::string_view("ABCDEFG")));
    EXPECT_FALSE(checkString.starts_with("ABCDEFG"));
    EXPECT_FALSE(checkString.starts_with(BASE_NS::string_view("AbC")));
    EXPECT_FALSE(checkString.starts_with("AbC"));
    EXPECT_FALSE(checkString.starts_with(BASE_NS::string_view("B")));
    EXPECT_FALSE(checkString.starts_with("B"));
    EXPECT_FALSE(checkString.starts_with('B'));
}

HWTEST_F(StringTest, EndssWith, TestSize.Level1)
{
    BASE_NS::string checkString(SHORT_STRING);
    EXPECT_TRUE(checkString.ends_with(BASE_NS::string_view("F")));
    EXPECT_TRUE(checkString.ends_with("F"));
    EXPECT_TRUE(checkString.ends_with('F'));
    EXPECT_TRUE(checkString.ends_with(BASE_NS::string_view("DEF")));
    EXPECT_TRUE(checkString.ends_with("DEF"));
    EXPECT_FALSE(checkString.ends_with(BASE_NS::string_view("ABCDEFG")));
    EXPECT_FALSE(checkString.ends_with("ABCDEFG"));
    EXPECT_FALSE(checkString.ends_with(BASE_NS::string_view("DeF")));
    EXPECT_FALSE(checkString.ends_with("DeF"));
    EXPECT_FALSE(checkString.ends_with(BASE_NS::string_view("E")));
    EXPECT_FALSE(checkString.ends_with("E"));
    EXPECT_FALSE(checkString.ends_with('E'));
}

HWTEST_F(StringTest, EndsWith_substring, TestSize.Level1)
{
    BASE_NS::string fullString = "hello world!";
    auto substring = fullString.substr(6, 5);
    ASSERT_EQ(substring, "world");

    ASSERT_TRUE(substring.ends_with('d'));
    ASSERT_FALSE(substring.ends_with('l'));
    ASSERT_FALSE(substring.ends_with('!'));

    ASSERT_TRUE(substring.ends_with("d"));
    ASSERT_FALSE(substring.ends_with("l"));
    ASSERT_FALSE(substring.ends_with("!"));
}

HWTEST_F(StringTest, EndsWith_substring_out_of_bounds, TestSize.Level1)
{
    BASE_NS::string fullString = "hello world here";
    auto substring = fullString.substr(6, 5);
    ASSERT_EQ(substring, "world");

    ASSERT_TRUE(substring.ends_with("world"));
    ASSERT_TRUE(substring.ends_with("rld"));
    ASSERT_TRUE(substring.ends_with("d"));
    ASSERT_TRUE(substring.ends_with(""));

    ASSERT_FALSE(substring.ends_with(" "));
    ASSERT_FALSE(substring.ends_with("d "));
    ASSERT_FALSE(substring.ends_with("d here"));

    ASSERT_FALSE(substring.ends_with(" world"));
    ASSERT_FALSE(substring.ends_with("hello world"));
}

HWTEST_F(StringTest, EndsWith_string, TestSize.Level1)
{
    BASE_NS::string testString = "world";

    ASSERT_TRUE(testString.ends_with('d'));
    ASSERT_FALSE(testString.ends_with('l'));
    ASSERT_FALSE(testString.ends_with('\0'));

    ASSERT_TRUE(testString.ends_with("d"));
    ASSERT_FALSE(testString.ends_with("l"));
}
HWTEST_F(StringTest, UserDefinedLiteral_QualifiedNamespace, TestSize.Level1)
{
    using namespace BASE_NS::literals::string_literals;
    auto myString = "hello"_s;
    static_assert(BASE_NS::is_same_v<BASE_NS::string, decltype(myString)>);
    ASSERT_STREQ("hello", myString.c_str());
}

HWTEST_F(StringTest, UserDefinedLiteral, TestSize.Level1)
{
    using namespace BASE_NS::literals;
    auto myString = "hello"_s;
    static_assert(BASE_NS::is_same_v<BASE_NS::string, decltype(myString)>);
    ASSERT_STREQ("hello", myString.c_str());
}

HWTEST_F(StringTest, UserDefinedLiteral_zeroBytes, TestSize.Level1)
{
    using namespace BASE_NS::literals;
    auto myString = "hel\0lo"_s;
    static_assert(BASE_NS::is_same_v<BASE_NS::string, decltype(myString)>);
    ASSERT_EQ(6, myString.size());
    ASSERT_EQ(BASE_NS::string("hel\0lo", 6), myString);
    ASSERT_STREQ("hel", myString.c_str());
}
