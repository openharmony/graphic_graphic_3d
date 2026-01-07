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
#include <base/containers/string.h>

#include "test_framework.h"

using namespace BASE_NS;

/**
 * @tc.name: empty
 * @tc.desc: Tests for Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersArrayView, empty, testing::ext::TestSize.Level1)
{
    BASE_NS::array_view<int> int_av({});
    ASSERT_TRUE(int_av.size() == 0);
    ASSERT_TRUE(int_av.size_bytes() == 0);
    ASSERT_TRUE(int_av.empty());

    BASE_NS::array_view<bool> bool_av({});
    ASSERT_TRUE(bool_av.size() == 0);
    ASSERT_TRUE(bool_av.size_bytes() == 0);
    ASSERT_TRUE(bool_av.empty());

    BASE_NS::array_view<uint32_t> uint_av({});
    ASSERT_TRUE(uint_av.size() == 0);
    ASSERT_TRUE(uint_av.size_bytes() == 0);
    ASSERT_TRUE(uint_av.empty());

    BASE_NS::array_view<char> char_av({});
    ASSERT_TRUE(char_av.size() == 0);
    ASSERT_TRUE(char_av.size_bytes() == 0);
    ASSERT_TRUE(char_av.empty());

    BASE_NS::array_view<int*> p_av({});
    ASSERT_TRUE(p_av.size() == 0);
    ASSERT_TRUE(p_av.size_bytes() == 0);
    ASSERT_TRUE(p_av.empty());

    BASE_NS::array_view<array_view<int>> av_av({});
    ASSERT_TRUE(av_av.size() == 0);
    ASSERT_TRUE(av_av.size_bytes() == 0);
    ASSERT_TRUE(av_av.empty());
}

/**
 * @tc.name: dataAccess
 * @tc.desc: Tests for Data Access. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersArrayView, dataAccess, testing::ext::TestSize.Level1)
{
    int int_data[] = { 3, 2, 1, 0 };
    BASE_NS::array_view<int> int_av(int_data);
    ASSERT_TRUE(int_av.size() == 4);
    ASSERT_TRUE(int_av.size_bytes() == 4 * sizeof(int));
    ASSERT_FALSE(int_av.empty());

    ASSERT_TRUE(int_av.data() == int_data);
    ASSERT_TRUE(*int_av.begin() == int_data[0]);
    ASSERT_TRUE(*int_av.cbegin() == int_data[0]);
    ASSERT_TRUE(int_av[1] == int_data[1]);
    ASSERT_TRUE(int_av.at(2) == int_data[2]);
    ASSERT_TRUE(*(int_av.end() - 1) == int_data[3]);
    ASSERT_TRUE(*(int_av.cend() - 1) == int_data[3]);

    const BASE_NS::array_view<int> cint_av(int_data);

    ASSERT_TRUE(cint_av.size() == 4);
    ASSERT_TRUE(cint_av.size_bytes() == 4 * sizeof(int));
    ASSERT_FALSE(cint_av.empty());

    ASSERT_TRUE(cint_av.data() == int_data);
    ASSERT_TRUE(*cint_av.begin() == int_data[0]);
    ASSERT_TRUE(*cint_av.cbegin() == int_data[0]);
    ASSERT_TRUE(cint_av[1] == int_data[1]);
    ASSERT_TRUE(cint_av.at(2) == int_data[2]);
    ASSERT_TRUE(*(cint_av.end() - 1) == int_data[3]);
    ASSERT_TRUE(*(cint_av.cend() - 1) == int_data[3]);

    bool bool_data[] = { true, false };
    BASE_NS::array_view<bool> bool_av(bool_data);
    ASSERT_TRUE(bool_av.size() == 2);
    ASSERT_TRUE(bool_av.size_bytes() == 2 * sizeof(bool));
    ASSERT_FALSE(bool_av.empty());

    ASSERT_TRUE(bool_av[0] == bool_data[0]);
    ASSERT_TRUE(bool_av[1] == bool_data[1]);
}

/**
 * @tc.name: constructors
 * @tc.desc: Tests for Constructors. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersArrayView, constructors, testing::ext::TestSize.Level1)
{
    int int_data[] = { 3, 2, 1, 0 };
    {
        BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>();
        ASSERT_TRUE(int_av.size() == 0);
        ASSERT_TRUE(int_av.empty());
    }
    {
        BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(int_data);
        ASSERT_TRUE(int_av.size() == 4);
        ASSERT_TRUE(int_av[0] == int_data[0]);
        ASSERT_TRUE(int_av[1] == int_data[1]);
        ASSERT_TRUE(int_av[2] == int_data[2]);
        ASSERT_TRUE(int_av[3] == int_data[3]);
    }
    {
        BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(&int_data[0], &int_data[3]);
        ASSERT_TRUE(int_av.size() == 3);
        ASSERT_TRUE(int_av[0] == int_data[0]);
        ASSERT_TRUE(int_av[1] == int_data[1]);
        ASSERT_TRUE(int_av[2] == int_data[2]);
    }
    {
        BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(&int_data[0], 4);
        ASSERT_TRUE(int_av.size() == 4);
        ASSERT_TRUE(int_av[0] == int_data[0]);
        ASSERT_TRUE(int_av[1] == int_data[1]);
        ASSERT_TRUE(int_av[2] == int_data[2]);
        ASSERT_TRUE(int_av[3] == int_data[3]);
    }
    {
        BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(int_data);
        BASE_NS::array_view<int> int_av2 = BASE_NS::array_view<int>(int_av);
        ASSERT_TRUE(int_av2.size() == 4);
        ASSERT_TRUE(int_av2[0] == int_data[0]);
        ASSERT_TRUE(int_av2[1] == int_data[1]);
        ASSERT_TRUE(int_av2[2] == int_data[2]);
        ASSERT_TRUE(int_av2[3] == int_data[3]);
    }
    {
        const BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(int_data);
        BASE_NS::array_view<const int> int_av2 = BASE_NS::array_view<const int>(int_av);
        ASSERT_TRUE(int_av2.size() == 4);
        ASSERT_TRUE(int_av2[0] == int_data[0]);
        ASSERT_TRUE(int_av2[1] == int_data[1]);
        ASSERT_TRUE(int_av2[2] == int_data[2]);
        ASSERT_TRUE(int_av2[3] == int_data[3]);
    }
    {
        BASE_NS::array_view<int> int_av = BASE_NS::arrayview<int, 4>(int_data);
        ASSERT_TRUE(int_av.size() == 4);
        ASSERT_TRUE(int_av[0] == int_data[0]);
        ASSERT_TRUE(int_av[1] == int_data[1]);
        ASSERT_TRUE(int_av[2] == int_data[2]);
        ASSERT_TRUE(int_av[3] == int_data[3]);
    }
    {
        uint8_t* uint8_data = new uint8_t[4]();
        uint8_data[0] = 3;
        uint8_data[1] = 2;
        uint8_data[2] = 1;
        uint8_data[3] = 0;
        BASE_NS::array_view<const uint8_t> uint_av = BASE_NS::arrayviewU8<uint8_t>(*uint8_data);
        ASSERT_TRUE(uint_av.size() == 1); // pointer size is 1 -> data is the same but size is always 1
        ASSERT_TRUE(uint_av[0] == uint8_data[0]);
        delete[] uint8_data;
    }
    {
        uint8_t uint8_data[4] = { 3, 2, 1, 0 };
        BASE_NS::array_view<const uint8_t> uint_av = BASE_NS::arrayviewU8<uint8_t>(*uint8_data);
        ASSERT_TRUE(uint_av.size() == 1); // pointer size is 1 -> data is the same but size is always 1
        ASSERT_TRUE(uint_av[0] == uint8_data[0]);
    }
    {
        uint8_t uint8_data[4] = { 3, 2, 1, 0 };
        BASE_NS::array_view<const uint8_t> uint_av = BASE_NS::arrayviewU8(uint8_data);
        ASSERT_TRUE(uint_av.size() == 4); // array as input
        ASSERT_TRUE(uint_av[0] == uint8_data[0]);
        ASSERT_TRUE(uint_av[1] == uint8_data[1]);
        ASSERT_TRUE(uint_av[2] == uint8_data[2]);
        ASSERT_TRUE(uint_av[3] == uint8_data[3]);
    }
    {
        auto fun = [](BASE_NS::array_view<const int> int_av) {
            ASSERT_TRUE(int_av.size() == 4);
            ASSERT_TRUE(int_av[0] == 3);
            ASSERT_TRUE(int_av[1] == 2);
            ASSERT_TRUE(int_av[2] == 1);
            ASSERT_TRUE(int_av[3] == 0);
        };
        fun({ 3, 2, 1, 0 });
    }
    {
        auto fun = [](BASE_NS::array_view<const BASE_NS::string> string_av) {
            ASSERT_TRUE(string_av.size() == 2);
            ASSERT_TRUE(string_av[0] == "foo");
            ASSERT_TRUE(string_av[1] == "A really, really, really long string.");
        };
        fun({ BASE_NS::string("foo"), BASE_NS::string("A really, really, really long string.") });
    }
}

/**
 * @tc.name: hash
 * @tc.desc: Tests for Hash. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersArrayView, hash, testing::ext::TestSize.Level1)
{
    static constexpr const int data[] = { 1, 2, 3, 4 };
    auto h1 = FNV1aHash(data);
    auto h2 = hash(array_view(data));
    EXPECT_EQ(h1, h2);
}