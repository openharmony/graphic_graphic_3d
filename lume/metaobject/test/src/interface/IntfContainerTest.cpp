/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <meta/api/iteration.h>
#include <meta/api/make_callback.h>
#include <meta/base/ref_uri.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_required_interfaces.h>

#include "src/test_utils.h"
#include "src/testing_objects.h"
#include "src/util.h"
#include "src/test_runner.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

bool operator==(const ChildChangedInfo& lhs, const ChildChangedInfo& rhs)
{
    return lhs.object == rhs.object && lhs.index == rhs.index && lhs.parent.lock() == rhs.parent.lock();
}

bool operator==(const ChildMovedInfo& lhs, const ChildMovedInfo& rhs)
{
    return lhs.object == rhs.object && lhs.from == rhs.from && lhs.to == rhs.to &&
           lhs.parent.lock() == rhs.parent.lock();
}

class ContainerTestBase : public testing::Test {
public:
    virtual IContainer::Ptr CreateContainer(BASE_NS::string name) = 0;

    void SetUp() override
    {
        container_ = CreateContainer("Base");
        ASSERT_NE(container_, nullptr);
        container_->Add(CreateTestType<IObject>("Object1_1"));
        container_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_ = CreateContainer("Container1_1");
        container_->Add(interface_pointer_cast<IObject>(container1_1_));
        container1_1_->Add(CreateTestType<IObject>("Object2_1"));
        container1_1_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("Object1_3"));
        container2_1_ = CreateContainer("SameNameDifferentUid");
        container1_1_->Add(interface_pointer_cast<IObject>(container2_1_));

        ASSERT_EQ(container_->GetSize(), NumDirectChildren);

        container_->OnAdded()->AddHandler(MakeCallback<IOnChildChanged>(this, &ContainerTestBase::OnAdded));
        container_->OnRemoved()->AddHandler(MakeCallback<IOnChildChanged>(this, &ContainerTestBase::OnRemoved));
        container_->OnMoved()->AddHandler(MakeCallback<IOnChildMoved>(this, &ContainerTestBase::OnMoved));
    }
    void TearDown() override
    {
        container_.reset();
    }

    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }

protected:
    void OnAdded(const ChildChangedInfo& info)
    {
        addedCalls_.push_back(info);
    }
    void OnRemoved(const ChildChangedInfo& info)
    {
        removedCalls_.push_back(info);
    }
    void OnMoved(const ChildMovedInfo& info)
    {
        movedCalls_.push_back(info);
    }

    BASE_NS::vector<ChildChangedInfo> addedCalls_;
    BASE_NS::vector<ChildChangedInfo> removedCalls_;
    BASE_NS::vector<ChildMovedInfo> movedCalls_;

    static constexpr size_t NumDirectChildContainers = 1;
    static constexpr size_t NumDirectChildTestTypes = 4;
    static constexpr size_t NumDirectChildren = NumDirectChildContainers + NumDirectChildTestTypes;
    static constexpr size_t NumChildContainers = 2;
    static constexpr size_t NumChildTestTypes = 7;
    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
};

class ContainerCommonTest : public ContainerTestBase, public ::testing::WithParamInterface<ClassInfo> {
public:
    IContainer::Ptr CreateContainer(BASE_NS::string name) override
    {
        return CreateTestContainer<IContainer>(name, GetParam());
    }
};

class ContainerTest : public ContainerTestBase {
public:
    IContainer::Ptr CreateContainer(BASE_NS::string name) override
    {
        return CreateTestContainer<IContainer>(name, META_NS::ClassId::TestContainer);
    }
};

class FlatContainerTest : public ContainerTestBase {
public:
    IContainer::Ptr CreateContainer(BASE_NS::string name) override
    {
        return CreateTestContainer<IContainer>(name, META_NS::ClassId::TestFlatContainer);
    }
};

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test IsAncestorOf function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, IsAncestorOf, TestSize.Level1)
{
    auto object = container1_1_->FindAnyFromHierarchy<IObject>("Object2_1");
    ASSERT_NE(object, nullptr);
    EXPECT_TRUE(container_->IsAncestorOf(object));
    EXPECT_FALSE(container_->IsAncestorOf(nullptr));

    auto object2 = CreateTestType<IObject>("NotThere");
    ASSERT_NE(object2, nullptr);
    EXPECT_FALSE(container_->IsAncestorOf(object2));

    auto container2 = CreateTestContainer<IContainer>("Container2");
    ASSERT_NE(container2, nullptr);
    EXPECT_FALSE(container2->IsAncestorOf(object2));
    EXPECT_TRUE(container2->Add(object2));
    EXPECT_TRUE(container2->IsAncestorOf(object2));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test AddChild function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, AddChild, TestSize.Level1)
{
    const auto object = interface_pointer_cast<IObject>(CreateTestType());
    EXPECT_TRUE(container_->Add(object));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));
    auto expected = ChildChangedInfo { object, NumDirectChildren, container_ };
    EXPECT_EQ(addedCalls_[0], expected);
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test Insert function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, Insert, TestSize.Level1)
{
    auto all = container_->GetAll();
    auto expectedSize = NumDirectChildren;
    const auto item1 = interface_pointer_cast<IObject>(CreateTestType());
    const auto item2 = interface_pointer_cast<IObject>(CreateTestType());
    const auto item3 = interface_pointer_cast<IObject>(CreateTestType());

    EXPECT_TRUE(container_->Insert(all.size(), item1));
    all = container_->GetAll();
    EXPECT_EQ(all.size(), ++expectedSize);
    EXPECT_EQ(all.back(), item1);

    EXPECT_TRUE(container_->Insert(0, item2));
    all = container_->GetAll();
    EXPECT_EQ(all.size(), ++expectedSize);
    EXPECT_EQ(all.front(), item2);

    EXPECT_TRUE(container_->Insert(all.size() + 10, item3));
    all = container_->GetAll();
    EXPECT_EQ(all.size(), ++expectedSize);
    EXPECT_EQ(all.back(), item3);

    ASSERT_THAT(addedCalls_, SizeIs(3));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));
    auto expected1 = ChildChangedInfo { item1, NumDirectChildren, container_ };
    auto expected2 = ChildChangedInfo { item2, 0, container_ };
    auto expected3 = ChildChangedInfo { item3, expectedSize - 1, container_ };
    EXPECT_THAT(addedCalls_, ElementsAre(expected1, expected2, expected3));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test GetAt function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, GetAt, TestSize.Level1)
{
    const auto all = container_->GetAll();
    ASSERT_EQ(all.size(), NumDirectChildren);

    EXPECT_EQ(all.front(), container_->GetAt(0));
    EXPECT_EQ(all.back(), container_->GetAt(NumDirectChildren - 1));
    EXPECT_EQ(all[NumDirectChildren / 2], container_->GetAt(NumDirectChildren / 2));
    EXPECT_EQ(container_->GetAt(NumDirectChildren + 10), nullptr);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test RemoveChild function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, RemoveChild, TestSize.Level1)
{
    auto children = container_->GetAll();
    auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    BASE_NS::vector<ChildChangedInfo> removed;

    auto baseIndex = expectedCount / 3;

    while (!children.empty()) {
        auto index = ++baseIndex % expectedCount;
        const auto child = children[index];
        removed.push_back({ child, index, container_ });
        EXPECT_TRUE(container_->Remove(child));

        expectedCount--;
        children = container_->GetAll();

        EXPECT_THAT(children, Not(Contains(child)));
        ASSERT_EQ(children.size(), expectedCount);
    }

    ASSERT_THAT(addedCalls_, SizeIs(0));
    ASSERT_THAT(removedCalls_, SizeIs(NumDirectChildren));
    ASSERT_THAT(movedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, ElementsAreArray(removed));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test RemoveIndex function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, RemoveIndex, TestSize.Level1)
{
    auto children = container_->GetAll();
    auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    BASE_NS::vector<ChildChangedInfo> removed;

    auto baseIndex = expectedCount / 3;

    while (!children.empty()) {
        auto index = ++baseIndex % expectedCount;
        const auto child = children[index];
        removed.push_back({ child, index, container_ });
        EXPECT_TRUE(container_->Remove(index));

        expectedCount--;
        children = container_->GetAll();

        EXPECT_THAT(children, Not(Contains(child)));
        ASSERT_EQ(children.size(), expectedCount);
    }

    ASSERT_THAT(addedCalls_, SizeIs(0));
    ASSERT_THAT(removedCalls_, SizeIs(NumDirectChildren));
    ASSERT_THAT(movedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, ElementsAreArray(removed));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test RemoveChildInvalid function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, RemoveChildInvalid, TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    const auto invalid = interface_pointer_cast<IObject>(CreateTestType());
    EXPECT_FALSE(container_->Remove(invalid));
    EXPECT_EQ(children.size(), expectedCount);
    EXPECT_THAT(children, Not(Contains(invalid)));
    EXPECT_FALSE(container_->Remove(nullptr));
    EXPECT_EQ(children.size(), expectedCount);
    EXPECT_THAT(children, Not(Contains(invalid)));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test RemoveAll function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, RemoveAll, TestSize.Level1)
{
    container_->RemoveAll();
    EXPECT_EQ(container_->GetSize(), 0);
    EXPECT_THAT(container_->GetAll(), IsEmpty());
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(NumDirectChildren));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test Replace function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, Replace, TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto index = expectedCount / 2;
    const auto replace = children[index];
    const auto replaceWith = interface_pointer_cast<IObject>(CreateTestType("Replaced"));

    EXPECT_TRUE(container_->Replace(replace, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), Contains(replaceWith));
    EXPECT_THAT(container_->GetAll(), Not(Contains(replace)));
    EXPECT_THAT(movedCalls_, SizeIs(0));
    const auto removed = ChildChangedInfo { replace, index, container_ };
    const auto added = ChildChangedInfo { replaceWith, index, container_ };
    EXPECT_THAT(addedCalls_, ElementsAre(added));
    EXPECT_THAT(removedCalls_, ElementsAre(removed));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test ReplaceSame function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, ReplaceSame, TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    const auto notThere = interface_pointer_cast<IObject>(CreateTestType("NotThere"));
    const auto replace = children[expectedCount / 2];
    const auto replaceWith = children[expectedCount / 2];

    ASSERT_EQ(replace, replaceWith);

    EXPECT_TRUE(container_->Replace(replace, replaceWith));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), Contains(replaceWith));
    EXPECT_THAT(container_->GetAll(), Contains(replace));

    EXPECT_FALSE(container_->Replace(notThere, notThere));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), Not(Contains(notThere)));

    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));

    EXPECT_TRUE(container_->Replace(notThere, notThere, true));
    EXPECT_EQ(container_->GetSize(), expectedCount + 1);
    EXPECT_THAT(container_->GetAll(), Contains(notThere));

    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
    const auto added = ChildChangedInfo { notThere, expectedCount, container_ };
    EXPECT_THAT(addedCalls_, ElementsAre(added));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test ReplaceNull function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, ReplaceNull, TestSize.Level1)
{
    const auto children = container_->GetAll();
    ASSERT_EQ(children.size(), NumDirectChildren);
    const auto replaceWith = interface_pointer_cast<IObject>(CreateTestType("Replaced"));
    const auto replaceWith2 = interface_pointer_cast<IObject>(CreateTestType("Replaced2"));

    EXPECT_FALSE(container_->Replace({}, {}, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_FALSE(container_->Replace({}, {}, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
    EXPECT_FALSE(container_->Replace({}, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
    EXPECT_TRUE(container_->Replace({}, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);
    EXPECT_FALSE(container_->Replace({}, replaceWith2, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
    const auto added = ChildChangedInfo { replaceWith, NumDirectChildren, container_ };
    EXPECT_THAT(addedCalls_, ElementsAre(added));
    addedCalls_.clear();
    EXPECT_TRUE(container_->Replace(replaceWith, {}, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_TRUE(container_->Replace({}, replaceWith2, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);
    EXPECT_TRUE(container_->Replace(replaceWith2, {}, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    const auto removed1 = ChildChangedInfo { replaceWith, NumDirectChildren, container_ };
    const auto removed2 = ChildChangedInfo { replaceWith2, NumDirectChildren, container_ };
    const auto added2 = ChildChangedInfo { replaceWith2, NumDirectChildren, container_ };
    EXPECT_THAT(removedCalls_, ElementsAre(removed1, removed2));
    EXPECT_THAT(movedCalls_, SizeIs(0));
    EXPECT_THAT(addedCalls_, ElementsAre(added2));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test ReplaceAdd function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, ReplaceAdd, TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    const auto replace = interface_pointer_cast<IObject>(CreateTestType("NotThere"));
    const auto replaceWith = interface_pointer_cast<IObject>(CreateTestType("Replaced"));

    EXPECT_FALSE(container_->Replace(replace, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), Not(Contains(replaceWith)));
    EXPECT_THAT(container_->GetAll(), Not(Contains(replace)));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));

    EXPECT_TRUE(container_->Replace(replace, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), expectedCount + 1);
    EXPECT_THAT(container_->GetAll(), Contains(replaceWith));
    EXPECT_THAT(container_->GetAll(), Not(Contains(replace)));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
    const auto added = ChildChangedInfo { replaceWith, expectedCount, container_ };
    EXPECT_THAT(addedCalls_, ElementsAre(added));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveEmpty function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveEmpty, TestSize.Level1)
{
    auto moveItem = container_->GetAt(0);
    container_->RemoveAll();
    removedCalls_.clear();
    EXPECT_FALSE(container_->Move(1, 1));
    EXPECT_FALSE(container_->Move(0, 0));
    EXPECT_FALSE(container_->Move(0, 1));
    EXPECT_FALSE(container_->Move(moveItem, 1));
    EXPECT_FALSE(container_->Move(moveItem, 0));

    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveBack function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveBack, TestSize.Level1)
{
    size_t from = NumDirectChildren - 1;
    size_t to = 0;
    auto moveItem = container_->GetAt(from);
    auto moved = ChildMovedInfo { moveItem, from, to, container_ };
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveForward function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveForward, TestSize.Level1)
{
    size_t from = 0;
    size_t to = NumDirectChildren - 1;
    auto moveItem = container_->GetAt(from);
    auto moved = ChildMovedInfo { moveItem, from, to, container_ };
    EXPECT_TRUE(container_->Move(from, to));

    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveNext function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveNext, TestSize.Level1)
{
    size_t from = NumDirectChildren / 2;
    size_t to = from + 1;
    auto moveItem = container_->GetAt(from);
    auto moved = ChildMovedInfo { moveItem, from, to, container_ };
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveSame function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveSame, TestSize.Level1)
{
    size_t from = NumDirectChildren / 2;
    size_t to = from;
    auto moveItem = container_->GetAt(from);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveFromBiggerThanSize function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveFromBiggerThanSize, TestSize.Level1)
{
    size_t from = NumDirectChildren + 10;
    size_t to = 0;
    auto moveItem = container_->GetAt(NumDirectChildren - 1);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    auto moved = ChildMovedInfo { moveItem, NumDirectChildren - 1, to, container_ };
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveToBiggerThanSize function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveToBiggerThanSize, TestSize.Level1)
{
    size_t from = NumDirectChildren / 2;
    size_t to = NumDirectChildren + 10;
    auto moveItem = container_->GetAt(from);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(NumDirectChildren - 1), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    auto moved = ChildMovedInfo { moveItem, from, NumDirectChildren - 1, container_ };
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveFromToBiggerThanSize function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveFromToBiggerThanSize, TestSize.Level1)
{
    size_t from = NumDirectChildren + 10;
    size_t to = NumDirectChildren + 4;
    auto moveItem = container_->GetAt(NumDirectChildren - 1);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(NumDirectChildren - 1), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test MoveObject function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, MoveObject, TestSize.Level1)
{
    size_t from = NumDirectChildren - 1;
    size_t to = 0;
    const auto child = container_->GetAt(from);
    auto moved = ChildMovedInfo { child, from, to, container_ };
    EXPECT_TRUE(container_->Move(child, to));
    EXPECT_EQ(container_->GetAt(to), child);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test FindAllNameDirect function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, FindAllNameDirect, TestSize.Level1)
{
    auto result1 = container_->FindAll({ "Object1_1", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAll({ "Object2_1", TraversalType::NO_HIERARCHY, {}, false });

    EXPECT_THAT(result1, SizeIs(1));
    EXPECT_THAT(result2, SizeIs(0));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test SetRequiredInterfacesReplace function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, SetRequiredInterfacesReplace, TestSize.Level1)
{
    auto req = interface_cast<IRequiredInterfaces>(container_);
    ASSERT_TRUE(req);
    EXPECT_TRUE(req->SetRequiredInterfaces({ ITestType::UID }));
    EXPECT_EQ(container_->GetSize(), NumDirectChildTestTypes);

    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildTestTypes;
    ASSERT_EQ(children.size(), expectedCount);
    const auto replace = children[expectedCount / 2];

    const auto replaceWithItem = interface_pointer_cast<IObject>(CreateTestType("Replaced"));
    const auto replaceWithContainer = interface_pointer_cast<IObject>(CreateTestContainer("Replaced"));

    EXPECT_FALSE(container_->Replace(replace, replaceWithContainer, false));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), Not(Contains(replaceWithContainer)));
    EXPECT_THAT(container_->GetAll(), Contains(replace));

    EXPECT_FALSE(container_->Replace(replace, replaceWithContainer, true));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), Not(Contains(replaceWithContainer)));
    EXPECT_THAT(container_->GetAll(), Contains(replace));

    EXPECT_TRUE(container_->Replace(replace, replaceWithItem, false));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), Contains(replaceWithItem));
    EXPECT_THAT(container_->GetAll(), Not(Contains(replace)));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test FindAnyNameDirect function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, FindAnyNameDirect, TestSize.Level1)
{
    auto result1 = container_->FindAny({ "Object1_1", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAny({ "Object2_1", TraversalType::NO_HIERARCHY, {}, false });

    EXPECT_NE(result1, nullptr);
    EXPECT_EQ(result2, nullptr);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerCommonTest
 * @tc.desc: test SetRequiredInterfaces function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_P(ContainerCommonTest, SetRequiredInterfaces, TestSize.Level1)
{
    auto req = interface_cast<IRequiredInterfaces>(container_);
    ASSERT_TRUE(req);
    EXPECT_EQ(container_->GetSize(), NumDirectChildContainers + NumDirectChildTestTypes);
    EXPECT_TRUE(req->SetRequiredInterfaces({ ITestType::UID }));
    EXPECT_EQ(container_->GetSize(), NumDirectChildTestTypes);
    EXPECT_TRUE(req->SetRequiredInterfaces({ ITestContainer::UID }));
    EXPECT_EQ(container_->GetSize(), 0);
    EXPECT_FALSE(container_->Add(interface_pointer_cast<IObject>(CreateTestType())));
    EXPECT_EQ(container_->GetSize(), 0);
    const auto container = interface_pointer_cast<IObject>(CreateTestContainer());
    EXPECT_TRUE(container_->Add(container));
    EXPECT_EQ(container_->GetSize(), 1);
    const auto all = container_->GetAll();
    ASSERT_THAT(all, SizeIs(1));
    EXPECT_EQ(all[0], container);
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FailAddLoop function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FailAddLoop, TestSize.Level1)
{
    EXPECT_FALSE(container2_1_->Add(container_));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FailInsertLoop function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FailInsertLoop, TestSize.Level1)
{
    EXPECT_FALSE(container2_1_->Insert(0, container_));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FailReplaceLoop function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FailReplaceLoop, TestSize.Level1)
{
    EXPECT_FALSE(container1_1_->Replace(container1_1_->GetAll()[1], container_));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test ReplaceNullWithExisting function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, ReplaceNullWithExisting, TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto indexReplace = expectedCount / 2;
    const auto replaceWith = children[indexReplace];

    EXPECT_FALSE(container_->Replace({}, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), expectedCount);

    EXPECT_THAT(movedCalls_, SizeIs(0));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test ReplaceWithExistingAddAlways function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, ReplaceWithExistingAddAlways, TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto indexReplace = expectedCount / 2;
    auto indexReplaceWith = indexReplace + 1;
    const auto replace = children[indexReplace];
    const auto replaceWith = children[indexReplaceWith];

    EXPECT_TRUE(container_->Replace(replace, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), expectedCount - 1);

    const auto moved = ChildMovedInfo { replaceWith, indexReplaceWith, indexReplace, container_ };
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    const auto removed = ChildChangedInfo { replace, indexReplace, container_ };
    EXPECT_THAT(removedCalls_, ElementsAre(removed));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test ReplaceWithExistingDontAddAlways function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, ReplaceWithExistingDontAddAlways, TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto indexReplace = expectedCount / 2;
    auto indexReplaceWith = indexReplace + 1;
    const auto replace = children[indexReplace];
    const auto replaceWith = children[indexReplaceWith];

    EXPECT_TRUE(container_->Replace(replace, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), expectedCount - 1);

    const auto moved = ChildMovedInfo { replaceWith, indexReplaceWith, indexReplace, container_ };
    EXPECT_THAT(movedCalls_, ElementsAre(moved));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    const auto removed = ChildChangedInfo { replace, indexReplace, container_ };
    EXPECT_THAT(removedCalls_, ElementsAre(removed));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test AddChildTwice function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, AddChildTwice, TestSize.Level1)
{
    const auto child = interface_pointer_cast<IObject>(CreateTestType("Twice"));
    EXPECT_TRUE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    EXPECT_TRUE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    const auto children = container_->FindAll({ "Twice", TraversalType::NO_HIERARCHY });
    ASSERT_THAT(children, SizeIs(1));
    EXPECT_EQ(children[0], child);

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));
    auto expected = ChildChangedInfo { child, NumDirectChildren, container_ };
    EXPECT_EQ(addedCalls_[0], expected);
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAllEmptyName function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAllEmptyName, TestSize.Level1)
{
    auto result1 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result2 = container_->FindAll({ "", TraversalType::NO_HIERARCHY, { ITestContainer::UID }, false });
    auto result3 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });
    auto result4 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID }, false });

    EXPECT_THAT(result1, SizeIs(NumChildContainers + NumChildTestTypes));
    EXPECT_THAT(result2, SizeIs(1));
    EXPECT_THAT(result3, SizeIs(NumChildContainers));
    EXPECT_THAT(result4, SizeIs(NumChildTestTypes));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAllNameRecursive function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAllNameRecursive, TestSize.Level1)
{
    auto result1 = container_->FindAll({ "Object1_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result2 = container_->FindAll({ "Object2_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });

    EXPECT_THAT(result1, SizeIs(1));
    EXPECT_THAT(result2, SizeIs(1));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAllNameDuplicate function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAllNameDuplicate, TestSize.Level1)
{
    auto result1 = container_->FindAll({ "ObjectDupe", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAll({ "ObjectDupe", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });

    EXPECT_THAT(result1, SizeIs(1));
    EXPECT_THAT(result2, SizeIs(2));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAllUid function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAllUid, TestSize.Level1)
{
    auto result1 = container_->FindAll({ "", TraversalType::NO_HIERARCHY, { ITestContainer::UID }, false });
    auto result2 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });
    auto result3 = container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result4 = container_->FindAll(
        { "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });

    EXPECT_THAT(result1, SizeIs(1));
    EXPECT_THAT(result2, SizeIs(2));
    EXPECT_THAT(result3, SizeIs(3));
    EXPECT_THAT(result4, SizeIs(1));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAllUidStrict function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAllUidStrict, TestSize.Level1)
{
    auto result1 = container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER,
        { ITestType::UID, ITestContainer::UID }, true });
    auto result2 =
        container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID }, true });
    auto result3 = container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER,
        { ITestType::UID, ITestContainer::UID }, false });
    EXPECT_THAT(result1, SizeIs(0));
    EXPECT_THAT(result2, SizeIs(2));
    EXPECT_THAT(result3, SizeIs(3));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAllInvalid function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAllInvalid, TestSize.Level1)
{
    auto result1 = container_->FindAll({ "InvalidObject", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAll({ "InvalidObject", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result3 =
        container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { META_NS::IAttachment::UID }, false });
    auto result4 = container_->FindAll(
        { "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID, META_NS::IAttachment::UID }, true });

    EXPECT_THAT(result1, SizeIs(0));
    EXPECT_THAT(result2, SizeIs(0));
    EXPECT_THAT(result3, SizeIs(0));
    EXPECT_THAT(result4, SizeIs(0));
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAnyEmptyName function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAnyEmptyName, TestSize.Level1)
{
    auto result1 = container_->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result2 = container_->FindAny({ "", TraversalType::NO_HIERARCHY, { ITestContainer::UID }, false });
    auto result3 = container_->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });

    EXPECT_NE(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_NE(result3, nullptr);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAnyNameRecursive function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAnyNameRecursive, TestSize.Level1)
{
    auto result1 = container_->FindAny({ "Object1_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result2 = container_->FindAny({ "Object2_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result3 = container_->FindAnyFromHierarchy<IObject>("Object2_1");

    EXPECT_NE(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_EQ(result2, result3);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAnyNameDuplicate function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAnyNameDuplicate, TestSize.Level1)
{
    auto result1 = container_->FindAny({ "ObjectDupe", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAny({ "ObjectDupe", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });

    EXPECT_NE(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAnyUid function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAnyUid, TestSize.Level1)
{
    auto result1 = container_->FindAny({ "", TraversalType::NO_HIERARCHY, { ITestContainer::UID }, false });
    auto result2 = container_->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });
    auto result3 = container_->FindAny({ "SameNameDifferentUid", TraversalType::NO_HIERARCHY, {}, false });
    auto result4 = container_->FindAny(
        { "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });

    EXPECT_NE(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_NE(result3, nullptr);
    EXPECT_NE(result4, nullptr);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAnyUidStrict function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAnyUidStrict, TestSize.Level1)
{
    auto result1 = container_->FindAny({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER,
        { ITestType::UID, ITestContainer::UID }, true });
    auto result2 =
        container_->FindAny({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID }, true });
    auto result3 = container_->FindAny({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER,
        { ITestType::UID, ITestContainer::UID }, false });
    EXPECT_EQ(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_NE(result3, nullptr);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindAnyInvalid function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindAnyInvalid, TestSize.Level1)
{
    auto result1 = container_->FindAny({ "InvalidObject", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAny({ "InvalidObject", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result3 =
        container_->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { META_NS::IAttachment::UID }, false });
    auto result4 = container_->FindAny(
        { "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID, META_NS::IAttachment::UID }, true });

    EXPECT_EQ(result1, nullptr);
    EXPECT_EQ(result2, nullptr);
    EXPECT_EQ(result3, nullptr);
    EXPECT_EQ(result4, nullptr);
    EXPECT_THAT(addedCalls_, SizeIs(0));
    EXPECT_THAT(removedCalls_, SizeIs(0));
    EXPECT_THAT(movedCalls_, SizeIs(0));
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test IterationSupport function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, IterationSupport, TestSize.Level1)
{
    {
        int count = 0;
        ForEachShared(container_, [&](const IObject::Ptr&) { ++count; });
        EXPECT_EQ(count, NumDirectChildren);
    }
    {
        int count = 0;
        IterateShared(container_, [&](const IObject::Ptr&) {
            ++count;
            return true;
        });
        EXPECT_EQ(count, NumDirectChildren);
    }

    {
        int count = 0;
        ForEachShared(
            container_, [&](const IObject::Ptr&) { ++count; }, TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, NumChildContainers + NumChildTestTypes);
    }
    {
        int count = 0;
        IterateShared(
            container_,
            [&](const IObject::Ptr&) {
                ++count;
                return true;
            },
            TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, NumChildContainers + NumChildTestTypes);
    }
    {
        int count = 0;
        ForEachUnique(
            container_, [&](const IObject::Ptr&) { ++count; }, TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, NumChildContainers + NumChildTestTypes);
    }
    {
        int count = 0;
        IterateUnique(
            container_,
            [&](const IObject::Ptr&) {
                ++count;
                return true;
            },
            TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, NumChildContainers + NumChildTestTypes);
    }
    {
        int count = 0;
        IterateShared(
            container_,
            [&](const IObject::Ptr& o) {
                ++count;
                return o->GetName() != "SameNameDifferentUid";
            },
            TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_LT(count, NumChildContainers + NumChildTestTypes);
    }
    {
        int count = 0;
        ConstIterate(container_, MakeIterationConstCallable([&](const IObject::Ptr&) {
            ++count;
            return true;
        }),
            IterateStrategy { TraversalType::DEPTH_FIRST_PRE_ORDER, LockType::NO_LOCK });
        EXPECT_EQ(count, NumChildContainers + NumChildTestTypes);
    }
    {
        int count = 0;
        ConstIterate(container_, MakeIterationConstCallable([&](const IObject::Ptr&) {
            ++count;
            return true;
        }),
            IterateStrategy { TraversalType::FULL_HIERARCHY, LockType::UNIQUE_LOCK });
        EXPECT_EQ(count, NumChildContainers + NumChildTestTypes);
    }
    {
        int count = 0;
        ConstIterate(container_, MakeIterationConstCallable([&](const IObject::Ptr&) {
            ++count;
            return true;
        }),
            IterateStrategy { TraversalType::BREADTH_FIRST_ORDER, LockType::UNIQUE_LOCK });
        EXPECT_EQ(count, NumChildContainers + NumChildTestTypes);
    }
}

/**
 * @tc.name: ContainerTest
 * @tc.desc: test FindOrder function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(ContainerTest, FindOrder, TestSize.Level1)
{
    auto c = CreateContainer("X");
    auto c1 = CreateContainer("1");
    auto c2 = CreateContainer("2");
    auto c1_1 = CreateTestType<IObject>("3");
    auto c1_2 = CreateTestType<IObject>("1");
    auto c2_1 = CreateTestType<IObject>("3");
    auto c2_2 = CreateContainer("2_2");
    auto c2_2_1 = CreateContainer("4");
    auto c2_2_1_1 = CreateTestType<IObject>("4");
    auto c2_3 = CreateTestType<IObject>("4");
    auto c3 = CreateTestType<IObject>("3");

    c->Add(c1);
    c->Add(c2);
    c->Add(c3);
    c1->Add(c1_1);
    c1->Add(c1_2);
    c2->Add(c2_1);
    c2->Add(c2_2);
    c2->Add(c2_3);
    c2_2->Add(c2_2_1);
    c2_2_1->Add(c2_2_1_1);

    {
        auto r = c->FindAny({ "1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, interface_pointer_cast<IObject>(c1));
    }
    {
        auto r = c->FindAny({ "1", TraversalType::DEPTH_FIRST_POST_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, c1_2);
    }
    {
        auto r = c->FindAny({ "1", TraversalType::BREADTH_FIRST_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, interface_pointer_cast<IObject>(c1));
    }
    {
        auto r = c->FindAny({ "1", TraversalType::NO_HIERARCHY, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, interface_pointer_cast<IObject>(c1));
    }
    {
        auto r = c->FindAny({ "3", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, c1_1);
    }
    {
        auto r = c->FindAny({ "3", TraversalType::DEPTH_FIRST_POST_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, c1_1);
    }
    {
        auto r = c->FindAny({ "3", TraversalType::BREADTH_FIRST_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, c3);
    }
    {
        auto r = c->FindAny({ "3", TraversalType::NO_HIERARCHY, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, interface_pointer_cast<IObject>(c3));
    }
    {
        auto r = c->FindAny({ "4", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, interface_pointer_cast<IObject>(c2_2_1));
    }
    {
        auto r = c->FindAny({ "4", TraversalType::DEPTH_FIRST_POST_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, c2_2_1_1);
    }
    {
        auto r = c->FindAny({ "4", TraversalType::BREADTH_FIRST_ORDER, {}, false });
        ASSERT_TRUE(r);
        EXPECT_EQ(r, c2_3);
    }
    {
        auto r = c->FindAny({ "4", TraversalType::NO_HIERARCHY, {}, false });
        ASSERT_FALSE(r);
    }
}

/**
 * @tc.name: FlatContainerTest
 * @tc.desc: test SameItemMultipleTimes function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(FlatContainerTest, SameItemMultipleTimes, TestSize.Level1)
{
    const auto child = interface_pointer_cast<IObject>(CreateTestType("Twice"));
    EXPECT_TRUE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    EXPECT_TRUE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 2);

    const auto children = container_->FindAll({ "Twice", TraversalType::NO_HIERARCHY });
    ASSERT_THAT(children, SizeIs(2));
    EXPECT_EQ(children[0], child);
    EXPECT_EQ(children[1], child);

    ASSERT_THAT(addedCalls_, SizeIs(2));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));
    auto expected1 = ChildChangedInfo { child, NumDirectChildren, container_ };
    auto expected2 = ChildChangedInfo { child, NumDirectChildren + 1, container_ };
    EXPECT_EQ(addedCalls_[0], expected1);
    EXPECT_EQ(addedCalls_[1], expected2);
}

/**
 * @tc.name: FlatContainerTest
 * @tc.desc: test IterationSupport function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(FlatContainerTest, IterationSupport, TestSize.Level1)
{
    {
        int count = 0;
        ForEachShared(container_, [&](const IObject::Ptr&) { ++count; });
        EXPECT_EQ(count, NumDirectChildren);
    }
    {
        int count = 0;
        IterateShared(container_, [&](const IObject::Ptr&) {
            ++count;
            return true;
        });
        EXPECT_EQ(count, NumDirectChildren);
    }
}

static std::string BuildTestName(const testing::TestParamInfo<ContainerCommonTest::ParamType>& info)
{
    return info.param == META_NS::ClassId::TestContainer ? "TestContainer" : "TestFlatContainer";
}

INSTANTIATE_TEST_SUITE_P(ContainerTests, ContainerCommonTest,
    testing::Values(META_NS::ClassId::TestContainer, META_NS::ClassId::TestFlatContainer), BuildTestName);

META_END_NAMESPACE()
