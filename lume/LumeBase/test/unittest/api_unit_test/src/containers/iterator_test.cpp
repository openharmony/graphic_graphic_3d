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
#include <base/containers/iterator.h>
#include <base/containers/string_view.h>

#include "test_framework.h"

using namespace BASE_NS;

struct Node {
    int value = 0;
    Node* next = nullptr;
};

/**
 * @tc.name: ConstIterator
 * @tc.desc: Tests for Const Iterator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersIterator, ConstIterator, testing::ext::TestSize.Level1)
{
    int int_data[] = { 4, 3, 2, 1, 0 };
    //{
    //	BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>();
    //	ASSERT_TRUE(int_av.size() == 0);
    //	ASSERT_TRUE(int_av.empty());
    //}
    BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(int_data);
    {
        BASE_NS::const_iterator<array_view<int>> it = int_av.begin();

        ASSERT_EQ(*it, int_data[0]);

        auto v_prefix = ++it;
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_prefix, int_data[1]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator++();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        auto prev = it;
        auto v_postfix = it.operator++(3);
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator++(100);
        ASSERT_EQ(*it, int_data[4]);
        ASSERT_EQ(*v_postfix, int_data[3]);
        ASSERT_TRUE(v_postfix == prev);

        v_prefix = --it;
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_prefix, int_data[3]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator--();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        prev = it;
        v_postfix = it.operator--(3);
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator--(100);
        ASSERT_EQ(*it, int_data[0]);
        ASSERT_EQ(*v_postfix, int_data[1]);
        ASSERT_TRUE(v_postfix == prev);
    }
    {
        BASE_NS::const_iterator<array_view<int>> it = int_av.begin();

        ASSERT_EQ(*it, int_data[0]);

        auto v = it + 3;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, int_data[3]);
        ASSERT_EQ(*it, int_data[3]);

        v = it - 2;
        ASSERT_EQ(v - it, -2);
        it -= 2;
        ASSERT_EQ(*v, int_data[1]);
        ASSERT_EQ(*it, int_data[1]);
    }

    {
        BASE_NS::const_iterator<array_view<int>> it_begin = int_av.begin();
        BASE_NS::const_iterator<array_view<int>> it_cbegin = int_av.cbegin();
        BASE_NS::const_iterator<array_view<int>> it_end = int_av.end();
        BASE_NS::const_iterator<array_view<int>> it_cend = int_av.cend();

        ASSERT_TRUE(it_begin == it_cbegin);
        ASSERT_TRUE(it_begin != it_end);
        ASSERT_TRUE(it_begin != it_cend);

        ASSERT_TRUE(it_cbegin != it_end);
        ASSERT_TRUE(it_cbegin != it_cend);

        ASSERT_TRUE(it_end == it_cend);

        ASSERT_TRUE(it_begin >= it_cbegin);
        ASSERT_TRUE(it_begin <= it_cbegin);
        ASSERT_TRUE(it_begin <= it_end);
        ASSERT_TRUE(it_end >= it_begin);
        ASSERT_TRUE(it_begin <= it_cend);
        ASSERT_TRUE(it_cend >= it_begin);

        ASSERT_TRUE(it_cbegin <= it_end);
        ASSERT_TRUE(it_end >= it_cbegin);
        ASSERT_TRUE(it_cbegin <= it_cend);
        ASSERT_TRUE(it_cend >= it_cbegin);

        ASSERT_TRUE(it_end <= it_cend);
        ASSERT_TRUE(it_end >= it_cend);

        ASSERT_TRUE(it_begin < it_end);
        ASSERT_TRUE(it_end > it_begin);
        ASSERT_TRUE(it_begin < it_cend);
        ASSERT_TRUE(it_end > it_begin);

        ASSERT_TRUE(it_cbegin < it_end);
        ASSERT_TRUE(it_end > it_cbegin);
        ASSERT_TRUE(it_cbegin < it_cend);
        ASSERT_TRUE(it_end > it_cbegin);
    }

    Node node_array[5];
    node_array[0].value = 4;
    node_array[1].value = 3;
    node_array[2].value = 2;
    node_array[3].value = 1;
    node_array[4].value = 0;
    BASE_NS::array_view<Node> node_av = BASE_NS::array_view<Node>(node_array);
    {
        BASE_NS::const_iterator<array_view<Node>> it = node_av.begin();
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }
}

/**
 * @tc.name: Iterator
 * @tc.desc: Tests for Iterator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersIterator, Iterator, testing::ext::TestSize.Level1)
{
    int int_data[] = { 4, 3, 2, 1, 0 };
    //{
    //	BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>();
    //	ASSERT_TRUE(int_av.size() == 0);
    //	ASSERT_TRUE(int_av.empty());
    //}
    BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(int_data);
    {
        BASE_NS::iterator<array_view<int>> it = int_av.begin();

        ASSERT_EQ(*it, int_data[0]);

        auto v_prefix = ++it;
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_prefix, int_data[1]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator++();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        auto prev = it;
        auto v_postfix = it.operator++(3);
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator++(100);
        ASSERT_EQ(*it, int_data[4]);
        ASSERT_EQ(*v_postfix, int_data[3]);
        ASSERT_TRUE(v_postfix == prev);

        v_prefix = --it;
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_prefix, int_data[3]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator--();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        prev = it;
        v_postfix = it.operator--(3);
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator--(100);
        ASSERT_EQ(*it, int_data[0]);
        ASSERT_EQ(*v_postfix, int_data[1]);
        ASSERT_TRUE(v_postfix == prev);
    }
    {
        BASE_NS::iterator<array_view<int>> it = int_av.begin();

        ASSERT_EQ(*it, int_data[0]);

        auto v = it + 3;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, int_data[3]);
        ASSERT_EQ(*it, int_data[3]);

        v = it - 2;
        ASSERT_EQ(v - it, -2);
        it -= 2;
        ASSERT_EQ(*v, int_data[1]);
        ASSERT_EQ(*it, int_data[1]);
    }

    {
        BASE_NS::iterator<array_view<int>> it_begin = int_av.begin();
        BASE_NS::iterator<array_view<int>> it_begin2 =
            int_av.end() - static_cast<BASE_NS::iterator<array_view<int>>::difference_type>(int_av.size());
        BASE_NS::iterator<array_view<int>> it_end = int_av.end();
        BASE_NS::iterator<array_view<int>> it_end2 =
            int_av.begin() + static_cast<BASE_NS::iterator<array_view<int>>::difference_type>(int_av.size());

        ASSERT_TRUE(it_begin == it_begin2);
        ASSERT_TRUE(it_begin != it_end);
        ASSERT_TRUE(it_begin != it_end2);

        ASSERT_TRUE(it_begin2 != it_end);
        ASSERT_TRUE(it_begin2 != it_end2);

        ASSERT_TRUE(it_end == it_end2);

        ASSERT_TRUE(it_begin >= it_begin2);
        ASSERT_TRUE(it_begin <= it_begin2);
        ASSERT_TRUE(it_begin <= it_end);
        ASSERT_TRUE(it_end >= it_begin);
        ASSERT_TRUE(it_begin <= it_end2);
        ASSERT_TRUE(it_end2 >= it_begin);

        ASSERT_TRUE(it_begin2 <= it_end);
        ASSERT_TRUE(it_end >= it_begin2);
        ASSERT_TRUE(it_begin2 <= it_end2);
        ASSERT_TRUE(it_end2 >= it_begin2);

        ASSERT_TRUE(it_end <= it_end2);
        ASSERT_TRUE(it_end >= it_end2);

        ASSERT_TRUE(it_begin < it_end);
        ASSERT_TRUE(it_end > it_begin);
        ASSERT_TRUE(it_begin < it_end2);
        ASSERT_TRUE(it_end > it_begin);

        ASSERT_TRUE(it_begin2 < it_end);
        ASSERT_TRUE(it_end > it_begin2);
        ASSERT_TRUE(it_begin2 < it_end2);
        ASSERT_TRUE(it_end > it_begin2);
    }

    Node node_array[5];
    node_array[0].value = 4;
    node_array[1].value = 3;
    node_array[2].value = 2;
    node_array[3].value = 1;
    node_array[4].value = 0;
    BASE_NS::array_view<Node> node_av = BASE_NS::array_view<Node>(node_array);
    {
        BASE_NS::iterator<array_view<Node>> it = node_av.begin();
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }

    {
        const BASE_NS::iterator<array_view<Node>> it = (node_av.end() - 1);
        ASSERT_EQ(it->value, 0);
        ASSERT_EQ(it->next, nullptr);
    }
}

/**
 * @tc.name: ReverseIterator
 * @tc.desc: Tests for Reverse Iterator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersIterator, ReverseIterator, testing::ext::TestSize.Level1)
{
    int int_data[] = { 4, 3, 2, 1, 0 };
    //{
    //	BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>();
    //	ASSERT_TRUE(int_av.size() == 0);
    //	ASSERT_TRUE(int_av.empty());
    //}
    BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(int_data);
    {
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it(int_av.begin());
        it--; // to set to [0]

        ASSERT_EQ(*it, int_data[0]);

        auto v_prefix = --it;
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_prefix, int_data[1]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator--();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        auto prev = it;
        auto v_postfix = it.operator--(3);
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator--(100);
        ASSERT_EQ(*it, int_data[4]);
        ASSERT_EQ(*v_postfix, int_data[3]);
        ASSERT_TRUE(v_postfix == prev);

        v_prefix = ++it;
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_prefix, int_data[3]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator++();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        prev = it;
        v_postfix = it.operator++(3);
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator++(100);
        ASSERT_EQ(*it, int_data[0]);
        ASSERT_EQ(*v_postfix, int_data[1]);
        ASSERT_TRUE(v_postfix == prev);
    }
    {
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it(int_av.begin());
        it--; // to set to [0]

        ASSERT_EQ(*it, int_data[0]);

        auto v = it - 3;
        ASSERT_EQ(v - it, -3);
        it -= 3;
        ASSERT_EQ(*v, int_data[3]);
        ASSERT_EQ(*it, int_data[3]);

        v = it + 2;
        ASSERT_EQ(v - it, 2);
        it += 2;
        ASSERT_EQ(*v, int_data[1]);
        ASSERT_EQ(*it, int_data[1]);
    }

    {
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it_begin(int_av.begin());
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it_begin2(
            int_av.end() - static_cast<array_view<int>::iterator::difference_type>(int_av.size()));
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it_end(int_av.end());
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it_end2(
            int_av.begin() + static_cast<array_view<int>::iterator::difference_type>(int_av.size()));

        ASSERT_TRUE(it_begin == it_begin2);
        ASSERT_TRUE(it_begin != it_end);
        ASSERT_TRUE(it_begin != it_end2);

        ASSERT_TRUE(it_begin2 != it_end);
        ASSERT_TRUE(it_begin2 != it_end2);

        ASSERT_TRUE(it_end == it_end2);

        ASSERT_TRUE(it_begin >= it_begin2);
        ASSERT_TRUE(it_begin <= it_begin2);
        ASSERT_TRUE(it_begin >= it_end);
        ASSERT_TRUE(it_end <= it_begin);
        ASSERT_TRUE(it_begin >= it_end2);
        ASSERT_TRUE(it_end2 <= it_begin);

        ASSERT_TRUE(it_begin2 >= it_end);
        ASSERT_TRUE(it_end <= it_begin2);
        ASSERT_TRUE(it_begin2 >= it_end2);
        ASSERT_TRUE(it_end2 <= it_begin2);

        ASSERT_TRUE(it_end <= it_end2);
        ASSERT_TRUE(it_end >= it_end2);

        ASSERT_TRUE(it_begin > it_end);
        ASSERT_TRUE(it_end < it_begin);
        ASSERT_TRUE(it_begin > it_end2);
        ASSERT_TRUE(it_end < it_begin);

        ASSERT_TRUE(it_begin2 > it_end);
        ASSERT_TRUE(it_end < it_begin2);
        ASSERT_TRUE(it_begin2 > it_end2);
        ASSERT_TRUE(it_end < it_begin2);
    }

    Node node_array[5];
    node_array[0].value = 4;
    node_array[1].value = 3;
    node_array[2].value = 2;
    node_array[3].value = 1;
    node_array[4].value = 0;
    BASE_NS::array_view<Node> node_av = BASE_NS::array_view<Node>(node_array);
    {
        BASE_NS::reverse_iterator<iterator<array_view<Node>>> it(node_av.begin());
        it--;
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }

    {
        const BASE_NS::reverse_iterator<iterator<array_view<Node>>> it(
            (node_av.end())); //- 1 removed cause iterator is created before original value
        ASSERT_EQ(it->value, 0);
        ASSERT_EQ(it->next, nullptr);
    }

    const char* char_data = "43210";
    BASE_NS::string_view sv(char_data);
    {
        auto it = sv.rend(); // rend = begin -> needs -- to point to [0]
        it--;
        ASSERT_EQ(*it, char_data[0]);

        auto v_prefix = --it;
        ASSERT_EQ(*it, char_data[1]);
        ASSERT_EQ(*v_prefix, char_data[1]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator--();
        ASSERT_EQ(*it, char_data[2]);
        ASSERT_EQ(*v_prefix, char_data[2]);
        ASSERT_TRUE(v_prefix == it);

        auto prev = it;
        auto v_postfix = it.operator--(3);
        ASSERT_EQ(*it, char_data[3]);
        ASSERT_EQ(*v_postfix, char_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator--(100);
        ASSERT_EQ(*it, char_data[4]);
        ASSERT_EQ(*v_postfix, char_data[3]);
        ASSERT_TRUE(v_postfix == prev);

        v_prefix = ++it;
        ASSERT_EQ(*it, char_data[3]);
        ASSERT_EQ(*v_prefix, char_data[3]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator++();
        ASSERT_EQ(*it, char_data[2]);
        ASSERT_EQ(*v_prefix, char_data[2]);
        ASSERT_TRUE(v_prefix == it);

        prev = it;
        v_postfix = it.operator++(3);
        ASSERT_EQ(*it, char_data[1]);
        ASSERT_EQ(*v_postfix, char_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator++(100);
        ASSERT_EQ(*it, char_data[0]);
        ASSERT_EQ(*v_postfix, char_data[1]);
        ASSERT_TRUE(v_postfix == prev);
    }
}

/**
 * @tc.name: MoveIterator
 * @tc.desc: Tests for Move Iterator. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ContainersIterator, MoveIterator, testing::ext::TestSize.Level1)
{
    // behaves as iterator. *it is rvalue, not lvalue.
    int int_data[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> int_av = BASE_NS::array_view<int>(int_data);
    typedef BASE_NS::array_view<int>::iterator Iter;
    {
        BASE_NS::move_iterator<Iter> it(int_av.begin());

        ASSERT_EQ(*it, int_data[0]);

        auto v_prefix = ++it;
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_prefix, int_data[1]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator++();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        auto prev = it;
        auto v_postfix = it.operator++(3);
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator++(100);
        ASSERT_EQ(*it, int_data[4]);
        ASSERT_EQ(*v_postfix, int_data[3]);
        ASSERT_TRUE(v_postfix == prev);

        v_prefix = --it;
        ASSERT_EQ(*it, int_data[3]);
        ASSERT_EQ(*v_prefix, int_data[3]);
        ASSERT_TRUE(v_prefix == it);

        v_prefix = it.operator--();
        ASSERT_EQ(*it, int_data[2]);
        ASSERT_EQ(*v_prefix, int_data[2]);
        ASSERT_TRUE(v_prefix == it);

        prev = it;
        v_postfix = it.operator--(3);
        ASSERT_EQ(*it, int_data[1]);
        ASSERT_EQ(*v_postfix, int_data[2]);
        ASSERT_TRUE(v_postfix == prev);

        prev = it;
        v_postfix = it.operator--(100);
        ASSERT_EQ(*it, int_data[0]);
        ASSERT_EQ(*v_postfix, int_data[1]);
        ASSERT_TRUE(v_postfix == prev);
    }
    {
        BASE_NS::move_iterator<Iter> it(int_av.begin());

        ASSERT_EQ(*it, int_data[0]);

        auto v = it + 3;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, int_data[3]);
        ASSERT_EQ(*it, int_data[3]);

        v = it - 2;
        ASSERT_EQ(v - it, -2);
        it -= 2;
        ASSERT_EQ(*v, int_data[1]);
        ASSERT_EQ(*it, int_data[1]);

        v = 3 + it;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, int_data[4]);
        ASSERT_EQ(*it, int_data[4]);
    }

    {
        BASE_NS::move_iterator<Iter> it_begin(int_av.begin());
        BASE_NS::move_iterator<Iter> it_begin2(
            int_av.end() - static_cast<array_view<int>::iterator::difference_type>(int_av.size()));
        BASE_NS::move_iterator<Iter> it_end(int_av.end());
        BASE_NS::move_iterator<Iter> it_end2(
            int_av.begin() + static_cast<array_view<int>::iterator::difference_type>(int_av.size()));

        ASSERT_TRUE(it_begin == it_begin2);
        ASSERT_TRUE(it_begin != it_end);
        ASSERT_TRUE(it_begin != it_end2);

        ASSERT_TRUE(it_begin2 != it_end);
        ASSERT_TRUE(it_begin2 != it_end2);

        ASSERT_TRUE(it_end == it_end2);

        ASSERT_TRUE(it_begin >= it_begin2);
        ASSERT_TRUE(it_begin <= it_begin2);
        ASSERT_TRUE(it_begin <= it_end);
        ASSERT_TRUE(it_end >= it_begin);
        ASSERT_TRUE(it_begin <= it_end2);
        ASSERT_TRUE(it_end2 >= it_begin);

        ASSERT_TRUE(it_begin2 <= it_end);
        ASSERT_TRUE(it_end >= it_begin2);
        ASSERT_TRUE(it_begin2 <= it_end2);
        ASSERT_TRUE(it_end2 >= it_begin2);

        ASSERT_TRUE(it_end <= it_end2);
        ASSERT_TRUE(it_end >= it_end2);

        ASSERT_TRUE(it_begin < it_end);
        ASSERT_TRUE(it_end > it_begin);
        ASSERT_TRUE(it_begin < it_end2);
        ASSERT_TRUE(it_end > it_begin);

        ASSERT_TRUE(it_begin2 < it_end);
        ASSERT_TRUE(it_end > it_begin2);
        ASSERT_TRUE(it_begin2 < it_end2);
        ASSERT_TRUE(it_end > it_begin2);
    }

    Node node_array[5];
    node_array[0].value = 4;
    node_array[1].value = 3;
    node_array[2].value = 2;
    node_array[3].value = 1;
    node_array[4].value = 0;
    typedef BASE_NS::array_view<Node>::iterator IterNode;
    BASE_NS::array_view<Node> node_av = BASE_NS::array_view<Node>(node_array);
    {
        BASE_NS::move_iterator<IterNode> it = make_move_iterator(node_av.begin());
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }

    {
        const BASE_NS::move_iterator<IterNode> it = make_move_iterator(node_av.end() - 1);
        ASSERT_EQ(it->value, 0);
        ASSERT_EQ(it->next, nullptr);
    }
    // int int_data_2d[][2] = { {0,1},{2,3} };
    // BASE_NS::array_view<int[2]> int_2d_av = BASE_NS::array_view<int[2]>(int_data_2d);
    //{
    //	BASE_NS::move_iterator<array_view<int[2]>::iterator> it = make_move_iterator(int_2d_av.begin());
    //	ASSERT_EQ(it[0], int_data_2d[0][0]);
    //	ASSERT_EQ(it[1], int_data_2d[0][1]);
    //	it++;
    //	ASSERT_EQ(it[0], int_data_2d[1][0]);
    //	ASSERT_EQ(it[1], int_data_2d[1][1]);
    // }
}