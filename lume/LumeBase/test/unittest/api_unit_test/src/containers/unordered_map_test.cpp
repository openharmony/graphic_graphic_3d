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

#include <chrono>

#include <base/containers/unordered_map.h>
#include <base/util/algorithm.h>

#include "test_framework.h"

using namespace BASE_NS;

template<typename T>
struct Node {
    T x, y, z;
    // Constructor
    Node() noexcept = default;
    Node(T x, T y, T z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }
};

/**
 * @tc.name: PairVerification
 * @tc.desc: Tests for Pair Verification. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, PairVerification, testing::ext::TestSize.Level1)
{
    // Default constructor
    auto original = pair<int, int>();
    original.first = 12;
    original.second = 15;
    ASSERT_EQ(original.first, 12);
    ASSERT_EQ(original.second, 15);
    original = {};
    ASSERT_EQ(original.first, 0);
    ASSERT_EQ(original.second, 0);
    static constexpr auto other = pair(12, 15);
    original = other;
    ASSERT_EQ(original.first, 12);
    ASSERT_EQ(original.second, 15);
    static constexpr auto otherL = pair(13L, 14L);
    original = otherL;
    ASSERT_EQ(original.first, 13);
    ASSERT_EQ(original.second, 14);
    // Other(s) constructor(s)
    // pair<int, int> original2(20, 15);
    // ASSERT_EQ(original2.first, 20);
    // ASSERT_EQ(original2.second, 15);
    {
        pair<int, int> original2 = other;
        ASSERT_EQ(original2.first, 12);
        ASSERT_EQ(original2.second, 15);
    }
    {
        pair<int, int> original2 = otherL;
        ASSERT_EQ(original2.first, 13);
        ASSERT_EQ(original2.second, 14);
    }
    {
        pair<int, int> original2 = pair(13L, 14L);
        ASSERT_EQ(original2.first, 13);
        ASSERT_EQ(original2.second, 14);
    }
    {
        auto original2 = pair(13L, 14L);
        original = BASE_NS::move(original2);
        ASSERT_EQ(original2.first, 13);
        ASSERT_EQ(original2.second, 14);
    }
    pair<int, int> original3 = other;
    ASSERT_EQ(original3.first, 12);
    ASSERT_EQ(original3.second, 15);
    {
        pair<int, string> data[] { { 2, "baz" }, { 2, "bar" }, { 1, "foo" } };
        BASE_NS::InsertionSort(data, data + countof(data));
        constexpr pair<int, string_view> first { 1, "foo" };
        constexpr pair<int, string_view> second { 2, "bar" };
        EXPECT_EQ(data[0], first);  // both eq
        EXPECT_EQ(data[1], second); // both eq

        EXPECT_LE(data[0], data[0]); // both eq
        EXPECT_LE(data[0], data[1]); // first lt, second gt
        EXPECT_LE(data[1], data[2]); // first eq, second lt

        EXPECT_GE(data[0], data[0]); // both eq
        EXPECT_GE(data[1], data[0]); // first gt, second lt
        EXPECT_GE(data[2], data[1]); // first eq, second gt

        EXPECT_LT(data[0], data[1]); // first lt, second gt
        EXPECT_LT(data[1], data[2]); // first eq, second lt

        EXPECT_GT(data[1], data[0]); // first gt, second lt
        EXPECT_GT(data[2], data[1]); // first eq, second gt
    }
}

/**
 * @tc.name: Default
 * @tc.desc: Tests for Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, Default, testing::ext::TestSize.Level1)
{
    // Test some types
    unordered_map<int, int> int_int1;
    auto int_int2 = unordered_map<int, int>();
    auto int_float = unordered_map<int, float>();
    auto int_bool = unordered_map<int, bool>();
    auto string_int = unordered_map<string, int>();
    auto int_string = unordered_map<int, string>();
    auto stringView_int = unordered_map<string_view, int>();
    auto int_stringView = unordered_map<int, string_view>();
    auto int_vector = unordered_map<int, vector<int>>();
    auto int_map = unordered_map<int, unordered_map<int, vector<int>>>();
    auto int_node = unordered_map<int, Node<int>>();

    // Not supported - hash function issue
    // unordered_map<char, int> map;
    // unordered_map<float, int> map;
}

/**
 * @tc.name: Initialization
 * @tc.desc: Tests for Initialization. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, Initialization, testing::ext::TestSize.Level1)
{
    // Test some types
    unordered_map<int, int> int_int;
    int_int[42] = 32;
    ASSERT_EQ(int_int[42], 32);

    auto int_float = unordered_map<int, float>();
    int_float[42] = 32.000005f;
    ASSERT_EQ(int_float[42], 32.000005f);

    auto int_bool = unordered_map<int, bool>();
    int_bool[42] = true;
    ASSERT_EQ(int_bool[42], true);

    auto string_int = unordered_map<string, int>();
    string_int["First"] = 10;
    string_int.insert({ "Second", 456 });
    string_int.insert({ "Third", 0 });
    const string cKey = "Key";
    const int cValue = 987;
    const pair<string_view, int> cPair = { cKey, cValue };
    string_int.insert(cPair);
    ASSERT_EQ(string_int["First"], 10);
    ASSERT_EQ(string_int["Second"], 456);
    ASSERT_EQ(string_int["Third"], 0);
    ASSERT_EQ(string_int.insert(cPair).second, false);
    ASSERT_EQ(string_int[cKey], cValue);

    auto int_string = unordered_map<int, string>();
    int_string.insert({ 10, "First" });
    int_string.insert({ 456, "Second" });
    ASSERT_EQ(int_string[10], "First");
    ASSERT_EQ(int_string[456], "Second");

    auto int_char = unordered_map<int, char>();
    int_char.insert({ 10, 'a' });
    int_char.insert({ 456, 'b' });
    ASSERT_EQ(int_char[10], 'a');
    ASSERT_EQ(int_char[456], 'b');

    auto int_node = unordered_map<int, Node<float>>();
    int_node.insert({ 1, Node(5.f, 10.f, 20.f) });
    ASSERT_EQ(int_node[1].x, 5.f);
    ASSERT_EQ(int_node[1].y, 10.f);
    ASSERT_EQ(int_node[1].z, 20.f);
}

/**
 * @tc.name: InitializerList
 * @tc.desc: Tests for Initializer List. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, InitializerList, testing::ext::TestSize.Level1)
{
    unordered_map<int, int> int_int({ { 1, 10 }, { 2, 20 }, { 3, 30 } });
    ASSERT_EQ(int_int[1], 10);
    ASSERT_EQ(int_int[2], 20);
    ASSERT_EQ(int_int[3], 30);
    unordered_map<string, int> string_int({ { "First", 10 }, { "Second", 20 }, { "Third", 30 } });
    ASSERT_EQ(string_int["First"], 10);
    ASSERT_EQ(string_int["Second"], 20);
    ASSERT_EQ(string_int["Third"], 30);
    string_int = { { "A", 1 }, { "B", 2 }, { "C", 3 } };
    ASSERT_EQ(string_int[string_view("A")], 1);
    ASSERT_EQ(string_int["B"], 2);
    ASSERT_EQ(string_int["C"], 3);
}

/**
 * @tc.name: Copy
 * @tc.desc: Tests for Copy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, Copy, testing::ext::TestSize.Level1)
{
    const auto original = unordered_map<int, int>();
    ASSERT_NO_FATAL_FAILURE(const auto copy = original);
    unordered_map<int, int> t;
    t.operator=(original);
}

/**
 * @tc.name: CopyVerification
 * @tc.desc: Tests for Copy Verification. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, CopyVerification, testing::ext::TestSize.Level1)
{
    auto original = unordered_map<int, int>();
    original[42] = 32;
    auto copy = original;
    ASSERT_TRUE(copy[42] == original[42]);
}

/**
 * @tc.name: PointerActions
 * @tc.desc: Tests for Pointer Actions. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, PointerActions, testing::ext::TestSize.Level1)
{
    auto original = unordered_map<int, int>();
    original[42] = 32;
    unordered_map<int, int> t;
    t.operator=(original);
    unordered_map<int, int>* p = new unordered_map<int, int>[3];
    p[0] = original;
    p[1] = t;
    p[2] = {};
    unordered_map<int, int>* p2 = &p[1];
    ASSERT_EQ(p2->begin(), p[1].begin());
    ASSERT_EQ(p2->end(), p[1].end());
    ASSERT_EQ(p[0][42], p[1][42]);
    ASSERT_NE(p[0].begin(), p[1].begin()); // copy
    ASSERT_EQ(p[2].begin(), p[2].end());   // empty
    ASSERT_EQ(p[2].cbegin(), p[2].cend()); // empty
    delete[] p;
}

/**
 * @tc.name: Capacity
 * @tc.desc: Tests for Capacity. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, Capacity, testing::ext::TestSize.Level1)
{
    // Size
    auto int_int = unordered_map<int, int>();
    int_int.insert({ 10, 20 });
    int_int.insert({ 10, 30 });
    int_int.insert({ 10, 40 });
    ASSERT_TRUE(int_int.size() == 1);
    ASSERT_EQ(int_int[10], 20);

    auto string_int = unordered_map<string, int>();
    string_int.insert({ "One", 20 });
    string_int.insert({ "ONE", 30 });
    string_int.insert({ "one", 40 });
    ASSERT_TRUE(string_int.size() == 3);

    // Empty
    auto original = unordered_map<int, int>();
    ASSERT_TRUE(original.empty());
}

/**
 * @tc.name: Iterators
 * @tc.desc: Tests for Iterators. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, Iterators, testing::ext::TestSize.Level1)
{
    // Test iterator
    auto string_int = unordered_map<string, int>();
    string_int.insert({ "First", 10 });
    string_int.insert({ "Second", 500 });
    string_int.insert({ "Third", 0 });

    // Values to match the order of unordered map (assigned hash values)
    vector<pair<string, int>> keyValVec;
    keyValVec.push_back({ "First", 10 });
    keyValVec.push_back({ "Third", 0 });
    keyValVec.push_back({ "Second", 500 });

    // Iterator pointing to begining of map
    unordered_map<string, int>::iterator it = string_int.begin();
    // Iterate over the map using iterator
    size_t count = 0;
    while (it != string_int.end()) {
        // ASSERT_EQ(it->first, keyValVec[count].first);
        ASSERT_STREQ(it->first.c_str(), keyValVec[count].first.c_str());
        ASSERT_EQ((*it).second, keyValVec[count].second);
        ++it;
        // it++ not supported
        count++;
    }

    // Iterate using range-based for loop
    count = 0;
    for (pair<const string, int> element : string_int) {
        ASSERT_EQ(element.first, keyValVec[count].first);
        ASSERT_EQ(element.second, keyValVec[count].second);
        count++;
    }

    // Iterate for loop using begin() and end()
    for (auto iter = string_int.begin(); iter != string_int.end(); ++iter) {
        auto cur = iter->first;
        string_int[cur] = 7;
    }
    ASSERT_EQ(string_int["First"], 7);
    ASSERT_EQ(string_int["Second"], 7);
    ASSERT_EQ(string_int["Third"], 7);
}

/**
 * @tc.name: Modifiers
 * @tc.desc: Tests for Modifiers. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, Modifiers, testing::ext::TestSize.Level1)
{
    auto original = unordered_map<int, int>();
    // Insert
    original.insert({ 15, 0 });
    original.insert({ 20, 15 });
    original.insert({ 70, 77 });
    ASSERT_EQ(original[15], 0);
    ASSERT_EQ(original[20], 15);
    ASSERT_EQ(original[70], 77);
    // Assign value to a key that already exists using "insert"
    ASSERT_NO_FATAL_FAILURE(original.insert({ 70, 80 }));
    // Confirm no value change for already existing key
    ASSERT_EQ(original[70], 77);

    // Insert or Assign
    original.insert_or_assign(15, 500);
    original.insert_or_assign(25, 2500);
    ASSERT_EQ(original[15], 500);
    ASSERT_EQ(original[25], 2500);
    ASSERT_EQ(original[20], 15);
    ASSERT_EQ(original[70], 77);

    {
        auto strMap = unordered_map<int, BASE_NS::string>();
        {
            BASE_NS::string lvalue = "lvalue";
            const BASE_NS::string constLvalue = "const lvalue";

            auto insertedLvalue = strMap.insert_or_assign(0, lvalue);
            EXPECT_EQ(insertedLvalue.first->second, lvalue);
            EXPECT_TRUE(insertedLvalue.second);

            auto insertedConstLvalue = strMap.insert_or_assign(1, constLvalue);
            EXPECT_EQ(insertedConstLvalue.first->second, constLvalue);
            EXPECT_TRUE(insertedConstLvalue.second);

            auto insertedRvalue = strMap.insert_or_assign(2, BASE_NS::string("rvalue"));
            EXPECT_EQ(insertedRvalue.first->second, "rvalue");
            EXPECT_TRUE(insertedRvalue.second);
        }
        {
            BASE_NS::string lvalue = "lvalue";
            const BASE_NS::string constLvalue = "const lvalue";

            auto assignedLvalue = strMap.insert_or_assign(0, lvalue);
            EXPECT_EQ(assignedLvalue.first->second, lvalue);
            EXPECT_FALSE(assignedLvalue.second);

            auto assignedConstLvalue = strMap.insert_or_assign(1, constLvalue);
            EXPECT_EQ(assignedConstLvalue.first->second, constLvalue);
            EXPECT_FALSE(assignedConstLvalue.second);

            auto assignedRvalue = strMap.insert_or_assign(2, BASE_NS::string("rvalue"));
            EXPECT_EQ(assignedRvalue.first->second, "rvalue");
            EXPECT_FALSE(assignedRvalue.second);
        }
    }

    // Erase using key
    ASSERT_EQ(original.erase(16), 0u);
    ASSERT_EQ(original.erase(15), 1u);
    ASSERT_EQ(original.size(), 3);
    ASSERT_EQ(original.erase(15), 0u);

    // Erase using iterator
    original.erase(original.begin());
    ASSERT_EQ(original.size(), 2);

    // Add more elements
    original.insert({ 44, -80 });
    original.insert({ 55, 80 });
    ASSERT_EQ(original.size(), 4);

    // Extract
    auto currentNodeHandle = original.extract(20);
    ASSERT_FALSE(currentNodeHandle.empty());
    ASSERT_EQ(currentNodeHandle.key(), 20);
    ASSERT_EQ(currentNodeHandle.mapped(), 15);
    ASSERT_EQ(currentNodeHandle.pointer->data.first, 20);
    ASSERT_EQ(currentNodeHandle.pointer->data.second, 15);

    // Not existing
    ASSERT_EQ(original[90], false);

    // Clear
    original.clear();
    ASSERT_EQ(original.size(), 0);

    // Add node
    original.insert(move(currentNodeHandle));
    ASSERT_EQ(original[20], 15);
}

/**
 * @tc.name: Lookup
 * @tc.desc: Tests for Lookup. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, Lookup, testing::ext::TestSize.Level1)
{
    auto string_int = unordered_map<string, int>();
    string_int.insert({ "First", 10 });
    string_int.insert({ "Second", 456 });
    string_int.insert({ "Third", 0 });

    // Find
    unordered_map<string, int>::const_iterator got = string_int.find("Second");
    ASSERT_EQ(got->first, "Second");
    ASSERT_EQ(got->second, 456);
    got = string_int.find("None");
    ASSERT_FALSE(got != string_int.end());

    const unordered_map<string, int>::const_iterator cGot = string_int.find("Third");
    ASSERT_EQ(cGot->first, "Third");
    ASSERT_EQ(cGot->second, 0);

    // Count
    ASSERT_EQ(string_int.count("First"), 1);
    ASSERT_EQ(string_int.count("Third"), 1);
    ASSERT_EQ(string_int.count("None"), 0);

    // Contains
    ASSERT_TRUE(string_int.contains("First"));
    ASSERT_TRUE(string_int.contains("Third"));
    ASSERT_FALSE(string_int.contains("None"));
}

/**
 * @tc.name: ExtractInsertBug
 * @tc.desc: Tests for Extract Insert Bug. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, ExtractInsertBug, testing::ext::TestSize.Level1)
{
    auto original = unordered_map<int, int>();
    original.insert({ 20, 15 });

    // This is a bit brittle and will break if implementation changes. Find a key which will have the same bucket index
    // as 20.
    auto bucketIndex = [](int key) {
        constexpr auto DEFAULT_SHIFT_AMOUNT = 4U;
        constexpr uint64_t GOLDEN_RATIO_64 = 0x9E3779B97F4A7C15ull;
        constexpr uint64_t shift = 64u - DEFAULT_SHIFT_AMOUNT;
        uint64_t h = hash(key);
        h ^= h >> shift;
        return static_cast<uint32_t>(((GOLDEN_RATIO_64 * h) >> shift));
    };
    const auto index20 = bucketIndex(20);
    int key = 0;
    for (uint32_t index = bucketIndex(key); index != index20; index = bucketIndex(++key))
        ;

    original.insert({ key, 81 });

    // Extract
    auto currentNodeHandle = original.extract(20);
    ASSERT_FALSE(currentNodeHandle.empty());

    // Clear
    original.clear();
    ASSERT_EQ(original.size(), 0);

    // Add node
    ASSERT_TRUE(original.insert(move(currentNodeHandle)).inserted);
    ASSERT_EQ(original.size(), 1);

    // Bug check: If the same bucket had multiple entries and one was extracted it's prev/next pointers were not
    // cleared. Due to the dangling prev pointer the second extract would update an invalid address to null and the
    // bucket would still point to the node. Inserting the node would find a match with the key and the node wasn't
    // added and there was a size mismatch.
    currentNodeHandle = original.extract(20);
    ASSERT_EQ(original.size(), 0);
    ASSERT_FALSE(currentNodeHandle.empty());
    ASSERT_TRUE(original.insert(move(currentNodeHandle)).inserted);
    ASSERT_EQ(original.size(), 1);
}

/**
 * @tc.name: ExtractInsertBug2
 * @tc.desc: Tests for Extract Insert Bug2. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, ExtractInsertBug2, testing::ext::TestSize.Level1)
{
    auto map = unordered_map<uint64_t, uint64_t>();

    constexpr uint64_t id1 = 0x300000011;
    constexpr uint64_t id2 = 0x200000059;
    constexpr uint64_t id3 = 0x10000007f;

    map[id1] = 1;
    map[id2] = 2;
    map[id3] = 3;
    map.erase(id2);
    map.erase(id1);

    ASSERT_TRUE(map.find(id1) == map.end()); // Should not be in the map.
    ASSERT_TRUE(map.find(id2) == map.end()); // Should not be in the map.
    ASSERT_TRUE(map.find(id3) != map.end()); // Should still be in the map.
}

// Improves coverage but takes calculation time. OpenCppCoverage works terable with it.

/**
 * @tc.name: HashPolicy
 * @tc.desc: Tests for Hash Policy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, DISABLED_HashPolicy, testing::ext::TestSize.Level1)
{
    // Reserve
    auto originalReserve = unordered_map<int, int>();
    auto originalNoReserve = unordered_map<int, int>();
    const int count = 1000000;
    double durationReserve = 0;
    double durationNoReserve = 0;

    // Reserve - no rehash
    originalReserve.reserve(count);

    // Measure performance (with pre memory allocation)
    {
        auto start = std::chrono::high_resolution_clock::now();

        for (int i = 0; i < count; ++i) {
            originalReserve.insert({ i, i % 2 });
        }

        // Get ending timepoint
        auto stop = std::chrono::high_resolution_clock::now();
        durationReserve =
            std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1U, 1U>>>(stop - start).count();
    }

    // Measure performance (without pre memory allocation)
    {
        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < count; ++i) {
            originalNoReserve.insert({ i, i % 2 });
        }
        auto stop = std::chrono::high_resolution_clock::now();
        durationReserve =
            std::chrono::duration_cast<std::chrono::duration<double, std::ratio<1U, 1U>>>(stop - start).count();
    }

    printf("Reserve - Time measured: %.6f seconds.\n", durationReserve);
    printf("No reserve - Time measured : %.6f seconds.\n", durationNoReserve);

    ASSERT_TRUE(durationReserve <= durationNoReserve);
}

/**
 * @tc.name: StringView
 * @tc.desc: Tests for String View. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersUnorderedMap, StringView, testing::ext::TestSize.Level1)
{
    unordered_map<BASE_NS::string, int> string_int = unordered_map<BASE_NS::string, int>();
    /*	Error for androuid build

        string_int["First"] = 10;
        string_int.insert({ "Second", 456 });
        string_int.insert({ "Third", 0 });
        string_int.insert({ string_view("Forth"), 234 });*/
    const string_view fifth = "Fifth";
    string_int.insert({ fifth, 345 });
    const string_view sixth = "Sixth";
    const pair<string_view, int> sixth_pair = { sixth, 6 };
    string_int.insert(sixth_pair);

    /*	Error for androuid build

        ASSERT_EQ(string_int["First"], 10);
        ASSERT_EQ(string_int["Second"], 456);
        ASSERT_EQ(string_int["Third"], 0);
        ASSERT_EQ(string_int["Forth"], 234);*/
    ASSERT_EQ(string_int[fifth], 345);
    ASSERT_EQ(string_int[sixth], 6);

    ASSERT_EQ((string_int.insert({ fifth, 345 })).second, false);
    ASSERT_EQ((string_int.insert(sixth_pair)).second, false);

    {
        BASE_NS::unordered_map_iterator<unordered_map_base<BASE_NS::string, int>> findRes = string_int.find(fifth);
        ASSERT_EQ(findRes->first, fifth);
        ASSERT_EQ(findRes->second, 345);
    }
    {
        const auto string_int_C = string_int;
        const auto findRes = string_int_C.find(fifth);
        ASSERT_EQ(findRes->first, fifth);
        ASSERT_EQ(findRes->second, 345);
    }

    ASSERT_EQ(true, string_int.contains(fifth));
    ASSERT_EQ(1, string_int.count(fifth));
    EXPECT_EQ(1, string_int.erase(fifth));
    EXPECT_EQ(0, string_int.erase(fifth));
    ASSERT_EQ(false, string_int.contains(fifth));
    ASSERT_EQ(0, string_int.contains(fifth));

    {
        BASE_NS::unordered_map_iterator<unordered_map_base<BASE_NS::string, int>> findRes = string_int.find(fifth);
        ASSERT_FALSE(findRes != string_int.end());
    }
    {
        const auto string_int_C = string_int;
        const auto findRes = string_int_C.find(fifth);
        ASSERT_FALSE(findRes != string_int_C.end());
    }
    ASSERT_EQ(string_int[fifth], false);
    // Checking element on existance adds it to unordered_map with false value
    ASSERT_EQ(string_int[fifth], false);
    ASSERT_EQ(true, string_int.contains(fifth));
    ASSERT_EQ(1, string_int.count(fifth));
    string_int.insert({ fifth, 345 });
    // As a result adding it with proper key will not overrite it
    ASSERT_EQ(1, string_int.count(fifth));
    ASSERT_EQ(string_int[fifth], false);
    string_int[fifth] = 345;
    ASSERT_EQ(string_int[fifth], 345);

    static constexpr auto seventh = string_view { "Seventh" };
    auto inserted = string_int.insert_or_assign(seventh, 346);
    EXPECT_EQ(inserted.first->first, seventh);
    EXPECT_EQ(inserted.first->second, 346);
    EXPECT_TRUE(inserted.second);

    auto assigned = string_int.insert_or_assign(seventh, 347);
    EXPECT_EQ(assigned.first->first, seventh);
    EXPECT_EQ(assigned.first->second, 347);
    EXPECT_FALSE(assigned.second);

    {
        // as node is not inserted back into the container we expect it to be properly cleaned up.
        auto node = string_int.extract(fifth);
        EXPECT_EQ(node.key(), fifth);
        node = string_int.extract(seventh);
        EXPECT_EQ(node.key(), seventh);
    }
}
