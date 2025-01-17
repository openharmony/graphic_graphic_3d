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
#include <base/containers/iterator.h>
#include <base/containers/string_view.h>

using namespace BASE_NS;
using namespace testing::ext;

struct Node {
    int value = 0;
    Node* next = nullptr;
};

class IteratorTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(IteratorTest, ConstIterator001, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    {
        BASE_NS::const_iterator<array_view<int>> it = intAv.begin();

        ASSERT_EQ(*it, intData[0]);

        auto vPrefix = ++it;
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPrefix, intData[1]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator++();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        auto prev = it;
        auto vPostfix = it.operator++(3);
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator++(100);
        ASSERT_EQ(*it, intData[4]);
        ASSERT_EQ(*vPostfix, intData[3]);
        ASSERT_TRUE(vPostfix == prev);

        vPrefix = --it;
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPrefix, intData[3]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator--();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        prev = it;
        vPostfix = it.operator--(3);
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator--(100);
        ASSERT_EQ(*it, intData[0]);
        ASSERT_EQ(*vPostfix, intData[1]);
        ASSERT_TRUE(vPostfix == prev);
    }
}

HWTEST_F(IteratorTest, ConstIterator002, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);

    {
        BASE_NS::const_iterator<array_view<int>> it = intAv.begin();

        ASSERT_EQ(*it, intData[0]);

        auto v = it + 3;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, intData[3]);
        ASSERT_EQ(*it, intData[3]);

        v = it - 2;
        ASSERT_EQ(v - it, -2);
        it -= 2;
        ASSERT_EQ(*v, intData[1]);
        ASSERT_EQ(*it, intData[1]);
    }
}

HWTEST_F(IteratorTest, ConstIterator003, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);

    {
        BASE_NS::const_iterator<array_view<int>> itBegin = intAv.begin();
        BASE_NS::const_iterator<array_view<int>> itCbegin = intAv.cbegin();
        BASE_NS::const_iterator<array_view<int>> itEnd = intAv.end();
        BASE_NS::const_iterator<array_view<int>> itCend = intAv.cend();

        ASSERT_TRUE(itBegin == itCbegin);
        ASSERT_TRUE(itBegin != itEnd);
        ASSERT_TRUE(itBegin != itCend);

        ASSERT_TRUE(itCbegin != itEnd);
        ASSERT_TRUE(itCbegin != itCend);

        ASSERT_TRUE(itEnd == itCend);

        ASSERT_TRUE(itBegin >= itCbegin);
        ASSERT_TRUE(itBegin <= itCbegin);
        ASSERT_TRUE(itBegin <= itEnd);
        ASSERT_TRUE(itEnd >= itBegin);
        ASSERT_TRUE(itBegin <= itCend);
        ASSERT_TRUE(itCend >= itBegin);

        ASSERT_TRUE(itCbegin <= itEnd);
        ASSERT_TRUE(itEnd >= itCbegin);
        ASSERT_TRUE(itCbegin <= itCend);
        ASSERT_TRUE(itCend >= itCbegin);

        ASSERT_TRUE(itEnd <= itCend);
        ASSERT_TRUE(itEnd >= itCend);

        ASSERT_TRUE(itBegin < itEnd);
        ASSERT_TRUE(itEnd > itBegin);
        ASSERT_TRUE(itBegin < itCend);
        ASSERT_TRUE(itEnd > itBegin);

        ASSERT_TRUE(itCbegin < itEnd);
        ASSERT_TRUE(itEnd > itCbegin);
        ASSERT_TRUE(itCbegin < itCend);
        ASSERT_TRUE(itEnd > itCbegin);
    }
}

HWTEST_F(IteratorTest, ConstIterator004, TestSize.Level1)
{
    Node nodeArray[5];
    nodeArray[0].value = 4;
    nodeArray[1].value = 3;
    nodeArray[2].value = 2;
    nodeArray[3].value = 1;
    nodeArray[4].value = 0;
    BASE_NS::array_view<Node> nodeAv = BASE_NS::array_view<Node>(nodeArray);
    {
        BASE_NS::const_iterator<array_view<Node>> it = nodeAv.begin();
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }
}

HWTEST_F(IteratorTest, Iterator001, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    {
        BASE_NS::iterator<array_view<int>> it = intAv.begin();

        ASSERT_EQ(*it, intData[0]);

        auto vPrefix = ++it;
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPrefix, intData[1]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator++();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        auto prev = it;
        auto vPostfix = it.operator++(3);
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator++(100);
        ASSERT_EQ(*it, intData[4]);
        ASSERT_EQ(*vPostfix, intData[3]);
        ASSERT_TRUE(vPostfix == prev);

        vPrefix = --it;
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPrefix, intData[3]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator--();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        prev = it;
        vPostfix = it.operator--(3);
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator--(100);
        ASSERT_EQ(*it, intData[0]);
        ASSERT_EQ(*vPostfix, intData[1]);
        ASSERT_TRUE(vPostfix == prev);
    }
}

HWTEST_F(IteratorTest, Iterator002, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    {
        BASE_NS::iterator<array_view<int>> it = intAv.begin();

        ASSERT_EQ(*it, intData[0]);

        auto v = it + 3;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, intData[3]);
        ASSERT_EQ(*it, intData[3]);

        v = it - 2;
        ASSERT_EQ(v - it, -2);
        it -= 2;
        ASSERT_EQ(*v, intData[1]);
        ASSERT_EQ(*it, intData[1]);
    }
}

HWTEST_F(IteratorTest, Iterator003, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    {
        BASE_NS::iterator<array_view<int>> itBegin = intAv.begin();
        BASE_NS::iterator<array_view<int>> itBegin2 =
            intAv.end() - static_cast<BASE_NS::iterator<array_view<int>>::difference_type>(intAv.size());
        BASE_NS::iterator<array_view<int>> itEnd = intAv.end();
        BASE_NS::iterator<array_view<int>> itEnd2 =
            intAv.begin() + static_cast<BASE_NS::iterator<array_view<int>>::difference_type>(intAv.size());

        ASSERT_TRUE(itBegin == itBegin2);
        ASSERT_TRUE(itBegin != itEnd);
        ASSERT_TRUE(itBegin != itEnd2);

        ASSERT_TRUE(itBegin2 != itEnd);
        ASSERT_TRUE(itBegin2 != itEnd2);

        ASSERT_TRUE(itEnd == itEnd2);

        ASSERT_TRUE(itBegin >= itBegin2);
        ASSERT_TRUE(itBegin <= itBegin2);
        ASSERT_TRUE(itBegin <= itEnd);
        ASSERT_TRUE(itEnd >= itBegin);
        ASSERT_TRUE(itBegin <= itEnd2);
        ASSERT_TRUE(itEnd2 >= itBegin);

        ASSERT_TRUE(itBegin2 <= itEnd);
        ASSERT_TRUE(itEnd >= itBegin2);
        ASSERT_TRUE(itBegin2 <= itEnd2);
        ASSERT_TRUE(itEnd2 >= itBegin2);

        ASSERT_TRUE(itEnd <= itEnd2);
        ASSERT_TRUE(itEnd >= itEnd2);

        ASSERT_TRUE(itBegin < itEnd);
        ASSERT_TRUE(itEnd > itBegin);
        ASSERT_TRUE(itBegin < itEnd2);
        ASSERT_TRUE(itEnd > itBegin);

        ASSERT_TRUE(itBegin2 < itEnd);
        ASSERT_TRUE(itEnd > itBegin2);
        ASSERT_TRUE(itBegin2 < itEnd2);
        ASSERT_TRUE(itEnd > itBegin2);
    }
}

HWTEST_F(IteratorTest, Iterator004, TestSize.Level1)
{
    Node nodeArray[5];
    nodeArray[0].value = 4;
    nodeArray[1].value = 3;
    nodeArray[2].value = 2;
    nodeArray[3].value = 1;
    nodeArray[4].value = 0;
    BASE_NS::array_view<Node> nodeAv = BASE_NS::array_view<Node>(nodeArray);
    {
        BASE_NS::iterator<array_view<Node>> it = nodeAv.begin();
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }

    {
        const BASE_NS::iterator<array_view<Node>> it = (nodeAv.end() - 1);
        ASSERT_EQ(it->value, 0);
        ASSERT_EQ(it->next, nullptr);
    }
}

HWTEST_F(IteratorTest, ReverseIterator001, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    {
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it(intAv.begin());
        it--; // to set to [0]

        ASSERT_EQ(*it, intData[0]);

        auto vPrefix = --it;
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPrefix, intData[1]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator--();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        auto prev = it;
        auto vPostfix = it.operator--(3);
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator--(100);
        ASSERT_EQ(*it, intData[4]);
        ASSERT_EQ(*vPostfix, intData[3]);
        ASSERT_TRUE(vPostfix == prev);

        vPrefix = ++it;
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPrefix, intData[3]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator++();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        prev = it;
        vPostfix = it.operator++(3);
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator++(100);
        ASSERT_EQ(*it, intData[0]);
        ASSERT_EQ(*vPostfix, intData[1]);
        ASSERT_TRUE(vPostfix == prev);
    }
}

HWTEST_F(IteratorTest, ReverseIterator002, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    {
        BASE_NS::reverse_iterator<iterator<array_view<int>>> it(intAv.begin());
        it--; // to set to [0]

        ASSERT_EQ(*it, intData[0]);

        auto v = it - 3;
        ASSERT_EQ(v - it, -3);
        it -= 3;
        ASSERT_EQ(*v, intData[3]);
        ASSERT_EQ(*it, intData[3]);

        v = it + 2;
        ASSERT_EQ(v - it, 2);
        it += 2;
        ASSERT_EQ(*v, intData[1]);
        ASSERT_EQ(*it, intData[1]);
    }
}

HWTEST_F(IteratorTest, ReverseIterator003, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    {
        BASE_NS::reverse_iterator<iterator<array_view<int>>> itBegin(intAv.begin());
        BASE_NS::reverse_iterator<iterator<array_view<int>>> itBegin2(
            intAv.end() - static_cast<array_view<int>::iterator::difference_type>(intAv.size()));
        BASE_NS::reverse_iterator<iterator<array_view<int>>> itEnd(intAv.end());
        BASE_NS::reverse_iterator<iterator<array_view<int>>> itEnd2(
            intAv.begin() + static_cast<array_view<int>::iterator::difference_type>(intAv.size()));

        ASSERT_TRUE(itBegin == itBegin2);
        ASSERT_TRUE(itBegin != itEnd);
        ASSERT_TRUE(itBegin != itEnd2);

        ASSERT_TRUE(itBegin2 != itEnd);
        ASSERT_TRUE(itBegin2 != itEnd2);

        ASSERT_TRUE(itEnd == itEnd2);

        ASSERT_TRUE(itBegin >= itBegin2);
        ASSERT_TRUE(itBegin <= itBegin2);
        ASSERT_TRUE(itBegin >= itEnd);
        ASSERT_TRUE(itEnd <= itBegin);
        ASSERT_TRUE(itBegin >= itEnd2);
        ASSERT_TRUE(itEnd2 <= itBegin);

        ASSERT_TRUE(itBegin2 >= itEnd);
        ASSERT_TRUE(itEnd <= itBegin2);
        ASSERT_TRUE(itBegin2 >= itEnd2);
        ASSERT_TRUE(itEnd2 <= itBegin2);

        ASSERT_TRUE(itEnd <= itEnd2);
        ASSERT_TRUE(itEnd >= itEnd2);

        ASSERT_TRUE(itBegin > itEnd);
        ASSERT_TRUE(itEnd < itBegin);
        ASSERT_TRUE(itBegin > itEnd2);
        ASSERT_TRUE(itEnd < itBegin);

        ASSERT_TRUE(itBegin2 > itEnd);
        ASSERT_TRUE(itEnd < itBegin2);
        ASSERT_TRUE(itBegin2 > itEnd2);
        ASSERT_TRUE(itEnd < itBegin2);
    }
}

HWTEST_F(IteratorTest, ReverseIterator004, TestSize.Level1)
{
    Node nodeArray[5];
    nodeArray[0].value = 4;
    nodeArray[1].value = 3;
    nodeArray[2].value = 2;
    nodeArray[3].value = 1;
    nodeArray[4].value = 0;
    BASE_NS::array_view<Node> nodeAv = BASE_NS::array_view<Node>(nodeArray);
    {
        BASE_NS::reverse_iterator<iterator<array_view<Node>>> it(nodeAv.begin());
        it--;
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }

    {
        const BASE_NS::reverse_iterator<iterator<array_view<Node>>> it(
            (nodeAv.end())); //- 1 removed cause iterator is created before original value
        ASSERT_EQ(it->value, 0);
        ASSERT_EQ(it->next, nullptr);
    }
}

HWTEST_F(IteratorTest, ReverseIterator005, TestSize.Level1)
{
    const char* charData = "43210";
    BASE_NS::string_view sv(charData);
    {
        auto it = sv.rend(); // rend = begin -> needs -- to point to [0]
        it--;
        ASSERT_EQ(*it, charData[0]);

        auto vPrefix = --it;
        ASSERT_EQ(*it, charData[1]);
        ASSERT_EQ(*vPrefix, charData[1]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator--();
        ASSERT_EQ(*it, charData[2]);
        ASSERT_EQ(*vPrefix, charData[2]);
        ASSERT_TRUE(vPrefix == it);

        auto prev = it;
        auto vPostfix = it.operator--(3);
        ASSERT_EQ(*it, charData[3]);
        ASSERT_EQ(*vPostfix, charData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator--(100);
        ASSERT_EQ(*it, charData[4]);
        ASSERT_EQ(*vPostfix, charData[3]);
        ASSERT_TRUE(vPostfix == prev);

        vPrefix = ++it;
        ASSERT_EQ(*it, charData[3]);
        ASSERT_EQ(*vPrefix, charData[3]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator++();
        ASSERT_EQ(*it, charData[2]);
        ASSERT_EQ(*vPrefix, charData[2]);
        ASSERT_TRUE(vPrefix == it);

        prev = it;
        vPostfix = it.operator++(3);
        ASSERT_EQ(*it, charData[1]);
        ASSERT_EQ(*vPostfix, charData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator++(100);
        ASSERT_EQ(*it, charData[0]);
        ASSERT_EQ(*vPostfix, charData[1]);
        ASSERT_TRUE(vPostfix == prev);
    }
}

HWTEST_F(IteratorTest, MoveIterator001, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    typedef BASE_NS::array_view<int>::iterator Iter;
    {
        BASE_NS::move_iterator<Iter> it(intAv.begin());

        ASSERT_EQ(*it, intData[0]);

        auto vPrefix = ++it;
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPrefix, intData[1]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator++();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        auto prev = it;
        auto vPostfix = it.operator++(3);
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator++(100);
        ASSERT_EQ(*it, intData[4]);
        ASSERT_EQ(*vPostfix, intData[3]);
        ASSERT_TRUE(vPostfix == prev);

        vPrefix = --it;
        ASSERT_EQ(*it, intData[3]);
        ASSERT_EQ(*vPrefix, intData[3]);
        ASSERT_TRUE(vPrefix == it);

        vPrefix = it.operator--();
        ASSERT_EQ(*it, intData[2]);
        ASSERT_EQ(*vPrefix, intData[2]);
        ASSERT_TRUE(vPrefix == it);

        prev = it;
        vPostfix = it.operator--(3);
        ASSERT_EQ(*it, intData[1]);
        ASSERT_EQ(*vPostfix, intData[2]);
        ASSERT_TRUE(vPostfix == prev);

        prev = it;
        vPostfix = it.operator--(100);
        ASSERT_EQ(*it, intData[0]);
        ASSERT_EQ(*vPostfix, intData[1]);
        ASSERT_TRUE(vPostfix == prev);
    }
}

HWTEST_F(IteratorTest, MoveIterator002, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    typedef BASE_NS::array_view<int>::iterator Iter;
    {
        BASE_NS::move_iterator<Iter> it(intAv.begin());

        ASSERT_EQ(*it, intData[0]);

        auto v = it + 3;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, intData[3]);
        ASSERT_EQ(*it, intData[3]);

        v = it - 2;
        ASSERT_EQ(v - it, -2);
        it -= 2;
        ASSERT_EQ(*v, intData[1]);
        ASSERT_EQ(*it, intData[1]);

        v = 3 + it;
        ASSERT_EQ(v - it, 3);
        it += 3;
        ASSERT_EQ(*v, intData[4]);
        ASSERT_EQ(*it, intData[4]);
    }
}

HWTEST_F(IteratorTest, MoveIterator003, TestSize.Level1)
{
    int intData[] = { 4, 3, 2, 1, 0 };
    BASE_NS::array_view<int> intAv = BASE_NS::array_view<int>(intData);
    typedef BASE_NS::array_view<int>::iterator Iter;
    {
        BASE_NS::move_iterator<Iter> itBegin(intAv.begin());
        BASE_NS::move_iterator<Iter> itBegin2(
            intAv.end() - static_cast<array_view<int>::iterator::difference_type>(intAv.size()));
        BASE_NS::move_iterator<Iter> itEnd(intAv.end());
        BASE_NS::move_iterator<Iter> itEnd2(
            intAv.begin() + static_cast<array_view<int>::iterator::difference_type>(intAv.size()));

        ASSERT_TRUE(itBegin == itBegin2);
        ASSERT_TRUE(itBegin != itEnd);
        ASSERT_TRUE(itBegin != itEnd2);

        ASSERT_TRUE(itBegin2 != itEnd);
        ASSERT_TRUE(itBegin2 != itEnd2);

        ASSERT_TRUE(itEnd == itEnd2);

        ASSERT_TRUE(itBegin >= itBegin2);
        ASSERT_TRUE(itBegin <= itBegin2);
        ASSERT_TRUE(itBegin <= itEnd);
        ASSERT_TRUE(itEnd >= itBegin);
        ASSERT_TRUE(itBegin <= itEnd2);
        ASSERT_TRUE(itEnd2 >= itBegin);

        ASSERT_TRUE(itBegin2 <= itEnd);
        ASSERT_TRUE(itEnd >= itBegin2);
        ASSERT_TRUE(itBegin2 <= itEnd2);
        ASSERT_TRUE(itEnd2 >= itBegin2);

        ASSERT_TRUE(itEnd <= itEnd2);
        ASSERT_TRUE(itEnd >= itEnd2);

        ASSERT_TRUE(itBegin < itEnd);
        ASSERT_TRUE(itEnd > itBegin);
        ASSERT_TRUE(itBegin < itEnd2);
        ASSERT_TRUE(itEnd > itBegin);

        ASSERT_TRUE(itBegin2 < itEnd);
        ASSERT_TRUE(itEnd > itBegin2);
        ASSERT_TRUE(itBegin2 < itEnd2);
        ASSERT_TRUE(itEnd > itBegin2);
    }
}

HWTEST_F(IteratorTest, MoveIterator004, TestSize.Level1)
{
    Node nodeArray[5];
    nodeArray[0].value = 4;
    nodeArray[1].value = 3;
    nodeArray[2].value = 2;
    nodeArray[3].value = 1;
    nodeArray[4].value = 0;
    typedef BASE_NS::array_view<Node>::iterator IterNode;
    BASE_NS::array_view<Node> nodeAv = BASE_NS::array_view<Node>(nodeArray);
    {
        BASE_NS::move_iterator<IterNode> it = make_move_iterator(nodeAv.begin());
        ASSERT_EQ(it->value, 4);
        ASSERT_EQ(it->next, nullptr);
    }

    {
        const BASE_NS::move_iterator<IterNode> it = make_move_iterator(nodeAv.end() - 1);
        ASSERT_EQ(it->value, 0);
        ASSERT_EQ(it->next, nullptr);
    }
}