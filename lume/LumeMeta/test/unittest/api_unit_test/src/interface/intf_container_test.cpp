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

#include <test_framework.h>

#include <meta/api/iteration.h>
#include <meta/api/make_callback.h>
#include <meta/base/ref_uri.h>
#include <meta/interface/intf_attachment.h>
#include <meta/interface/intf_metadata.h>
#include <meta/interface/intf_object_flags.h>
#include <meta/interface/intf_required_interfaces.h>

#include "helpers/test_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()

bool operator==(const ChildChangedInfo& lhs, const ChildChangedInfo& rhs)
{
    return lhs.object == rhs.object && lhs.to == rhs.to && lhs.from == rhs.from &&
           lhs.parent.lock() == rhs.parent.lock();
}

namespace UTest {

class API_ContainerTestBase : public ::testing::Test {
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

        container_->OnContainerChanged()->AddHandler(MakeCallback<IOnChildChanged>([&](auto const& i) {
            if (i.type == ContainerChangeType::ADDED) {
                OnAdded(i);
            } else if (i.type == ContainerChangeType::REMOVED) {
                OnRemoved(i);
            } else if (i.type == ContainerChangeType::MOVED) {
                OnMoved(i);
            }
        }));
    }
    void TearDown() override
    {
        container_.reset();
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
    void OnMoved(const ChildChangedInfo& info)
    {
        movedCalls_.push_back(info);
    }

    BASE_NS::vector<ChildChangedInfo> addedCalls_;
    BASE_NS::vector<ChildChangedInfo> removedCalls_;
    BASE_NS::vector<ChildChangedInfo> movedCalls_;

    // These match the hierarchy defined in SetUp
    static constexpr size_t NumDirectChildContainers = 1;
    static constexpr size_t NumDirectChildTestTypes = 4;
    static constexpr size_t NumDirectChildren = NumDirectChildContainers + NumDirectChildTestTypes;
    static constexpr size_t NumChildContainers = 2;
    static constexpr size_t NumChildTestTypes = 7;
    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
};

class API_ContainerCommonTest : public API_ContainerTestBase, public ::testing::WithParamInterface<ClassInfo> {
public:
    IContainer::Ptr CreateContainer(BASE_NS::string name) override
    {
        return CreateTestContainer<IContainer>(name, GetParam());
    }
};

class API_ContainerTest : public API_ContainerTestBase {
public:
    IContainer::Ptr CreateContainer(BASE_NS::string name) override
    {
        return CreateTestContainer<IContainer>(name, META_NS::ClassId::TestContainer);
    }
};

class API_FlatContainerTest : public API_ContainerTestBase {
public:
    IContainer::Ptr CreateContainer(BASE_NS::string name) override
    {
        return CreateTestContainer<IContainer>(name, META_NS::ClassId::TestFlatContainer);
    }
};

/**
 * @tc.name: IsAncestorOf
 * @tc.desc: Tests for Is Ancestor Of. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, IsAncestorOf, testing::ext::TestSize.Level1)
{
    auto object = container1_1_->FindAnyFromHierarchy<IObject>("Object2_1");
    ASSERT_NE(object, nullptr);
    EXPECT_TRUE(container_->IsAncestorOf(object));
    EXPECT_FALSE(container_->IsAncestorOf(nullptr));

    EXPECT_FALSE(container1_1_->FindAnyFromHierarchy<IObject>(""));

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
 * @tc.name: AddChild
 * @tc.desc: Tests for Add Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, AddChild, testing::ext::TestSize.Level1)
{
    const auto object = interface_pointer_cast<IObject>(CreateTestType());
    EXPECT_TRUE(container_->Add(object));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    // Check events
    ASSERT_THAT(addedCalls_, testing::SizeIs(1));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));
    auto expected = ChildChangedInfo { ContainerChangeType::ADDED, object, container_, size_t(-1), NumDirectChildren };
    EXPECT_EQ(addedCalls_[0], expected);
}

/**
 * @tc.name: ContainsObject
 * @tc.desc: Tests for Contains Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, ContainsObject, testing::ext::TestSize.Level1)
{
    const auto object = CreateTestType();
    const auto notFound = CreateTestType();
    EXPECT_TRUE(container_->Add(object));
    EXPECT_FALSE(ContainsObject(nullptr, object));
    EXPECT_FALSE(ContainsObject(container_, IObject::Ptr {}));
    EXPECT_TRUE(ContainsObject(container_, object));
    EXPECT_FALSE(ContainsObject(container_, notFound));
}

/**
 * @tc.name: Insert
 * @tc.desc: Tests for Insert. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, Insert, testing::ext::TestSize.Level1)
{
    auto all = container_->GetAll();
    auto expectedSize = NumDirectChildren;
    const auto item1 = interface_pointer_cast<IObject>(CreateTestType());
    const auto item2 = interface_pointer_cast<IObject>(CreateTestType());
    const auto item3 = interface_pointer_cast<IObject>(CreateTestType());

    // Insert at end
    EXPECT_TRUE(container_->Insert(all.size(), item1));
    all = container_->GetAll();
    EXPECT_EQ(all.size(), ++expectedSize);
    EXPECT_EQ(all.back(), item1);

    // Insert at front
    EXPECT_TRUE(container_->Insert(0, item2));
    all = container_->GetAll();
    EXPECT_EQ(all.size(), ++expectedSize);
    EXPECT_EQ(all.front(), item2);

    // Insert past end, should add to end of container
    EXPECT_TRUE(container_->Insert(all.size() + 10, item3));
    all = container_->GetAll();
    EXPECT_EQ(all.size(), ++expectedSize);
    EXPECT_EQ(all.back(), item3);

    // Check events
    ASSERT_THAT(addedCalls_, testing::SizeIs(3));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));
    auto expected1 = ChildChangedInfo { ContainerChangeType::ADDED, item1, container_, size_t(-1), NumDirectChildren };
    auto expected2 = ChildChangedInfo { ContainerChangeType::ADDED, item2, container_, size_t(-1), 0 };
    auto expected3 = ChildChangedInfo { ContainerChangeType::ADDED, item3, container_, size_t(-1), expectedSize - 1 };
    EXPECT_THAT(addedCalls_, testing::ElementsAre(expected1, expected2, expected3));
}

/**
 * @tc.name: GetAt
 * @tc.desc: Tests for Get At. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, GetAt, testing::ext::TestSize.Level1)
{
    const auto all = container_->GetAll();
    ASSERT_EQ(all.size(), NumDirectChildren);

    EXPECT_EQ(all.front(), container_->GetAt(0));
    EXPECT_EQ(all.back(), container_->GetAt(NumDirectChildren - 1));
    EXPECT_EQ(all[NumDirectChildren / 2], container_->GetAt(NumDirectChildren / 2));
    EXPECT_EQ(container_->GetAt(NumDirectChildren + 10), nullptr);
    // Check events
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: RemoveChild
 * @tc.desc: Tests for Remove Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, RemoveChild, testing::ext::TestSize.Level1)
{
    auto children = container_->GetAll();
    auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    BASE_NS::vector<ChildChangedInfo> removed;

    auto baseIndex = expectedCount / 3;

    while (!children.empty()) {
        auto index = ++baseIndex % expectedCount; // To add variation to the index
        const auto child = children[index];
        removed.push_back({ ContainerChangeType::REMOVED, child, container_, index });
        EXPECT_TRUE(container_->Remove(child));

        expectedCount--;
        children = container_->GetAll();

        EXPECT_THAT(children, testing::Not(testing::Contains(child)));
        ASSERT_EQ(children.size(), expectedCount);
    }

    // Check events
    ASSERT_THAT(addedCalls_, testing::SizeIs(0));
    ASSERT_THAT(removedCalls_, testing::SizeIs(NumDirectChildren));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::ElementsAreArray(removed));

    // Templated Add/Remove
    auto child = CreateTestType();
    EXPECT_TRUE(container_->Add(child));
    EXPECT_TRUE(container_->Remove(child));
    EXPECT_EQ(container_->GetSize(), 0);
}

/**
 * @tc.name: RemoveIndex
 * @tc.desc: Tests for Remove Index. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, RemoveIndex, testing::ext::TestSize.Level1)
{
    auto children = container_->GetAll();
    auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    BASE_NS::vector<ChildChangedInfo> removed;

    auto baseIndex = expectedCount / 3;

    while (!children.empty()) {
        auto index = ++baseIndex % expectedCount; // To add variation to the index
        const auto child = children[index];
        removed.push_back({ ContainerChangeType::REMOVED, child, container_, index });
        EXPECT_TRUE(container_->Remove(index));

        expectedCount--;
        children = container_->GetAll();

        EXPECT_THAT(children, testing::Not(testing::Contains(child)));
        ASSERT_EQ(children.size(), expectedCount);
    }

    // Check events
    ASSERT_THAT(addedCalls_, testing::SizeIs(0));
    ASSERT_THAT(removedCalls_, testing::SizeIs(NumDirectChildren));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::ElementsAreArray(removed));
}

/**
 * @tc.name: RemoveChildInvalid
 * @tc.desc: Tests for Remove Child Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, RemoveChildInvalid, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    // Remove object which is not in the container
    const auto invalid = interface_pointer_cast<IObject>(CreateTestType());
    EXPECT_FALSE(container_->Remove(invalid));
    EXPECT_EQ(children.size(), expectedCount);
    EXPECT_THAT(children, testing::Not(testing::Contains(invalid)));
    // Remove null
    EXPECT_FALSE(container_->Remove(nullptr));
    EXPECT_EQ(children.size(), expectedCount);
    EXPECT_THAT(children, testing::Not(testing::Contains(invalid)));
    // Check events
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: RemoveAll
 * @tc.desc: Tests for Remove All. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, RemoveAll, testing::ext::TestSize.Level1)
{
    container_->RemoveAll();
    EXPECT_EQ(container_->GetSize(), 0);
    EXPECT_THAT(container_->GetAll(), testing::IsEmpty());
    // Check events
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(NumDirectChildren));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: Replace
 * @tc.desc: Tests for Replace. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, Replace, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto index = expectedCount / 2;
    const auto replace = children[index];
    const auto replaceWith = interface_pointer_cast<IObject>(CreateTestType("Replaced"));

    EXPECT_TRUE(container_->Replace(replace, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), testing::Contains(replaceWith));
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(replace)));
    // Check events
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    const auto removed = ChildChangedInfo { ContainerChangeType::REMOVED, replace, container_, index };
    const auto added = ChildChangedInfo { ContainerChangeType::ADDED, replaceWith, container_, size_t(-1), index };
    EXPECT_THAT(addedCalls_, testing::ElementsAre(added));
    EXPECT_THAT(removedCalls_, testing::ElementsAre(removed));
}

/**
 * @tc.name: ReplaceSame
 * @tc.desc: Tests for Replace Same. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, ReplaceSame, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    const auto notThere = interface_pointer_cast<IObject>(CreateTestType("NotThere"));
    const auto replace = children[expectedCount / 2];
    const auto replaceWith = children[expectedCount / 2];

    ASSERT_EQ(replace, replaceWith);

    // Replace an existing item with the same item
    EXPECT_TRUE(container_->Replace(replace, replaceWith));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), testing::Contains(replaceWith));
    EXPECT_THAT(container_->GetAll(), testing::Contains(replace));

    // Replace non-existing item with the same item
    EXPECT_FALSE(container_->Replace(notThere, notThere));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(notThere)));

    // Should get no events
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));

    // Replace non-existing item with the same item, but specify that the item should always be added
    EXPECT_TRUE(container_->Replace(notThere, notThere, true));
    EXPECT_EQ(container_->GetSize(), expectedCount + 1);
    EXPECT_THAT(container_->GetAll(), testing::Contains(notThere));

    // The item should have been added
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    const auto added = ChildChangedInfo { ContainerChangeType::ADDED, notThere, container_, size_t(-1), expectedCount };
    EXPECT_THAT(addedCalls_, testing::ElementsAre(added));
}

/**
 * @tc.name: ReplaceNull
 * @tc.desc: Tests for Replace Null. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, ReplaceNull, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    ASSERT_EQ(children.size(), NumDirectChildren);
    const auto replaceWith = interface_pointer_cast<IObject>(CreateTestType("Replaced"));
    const auto replaceWith2 = interface_pointer_cast<IObject>(CreateTestType("Replaced2"));

    // Replace null with null shouldn't do anything
    EXPECT_FALSE(container_->Replace({}, {}, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_FALSE(container_->Replace({}, {}, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    // Replace null with valid object shouldn't do anything if addAlways==false
    EXPECT_FALSE(container_->Replace({}, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    // Replace null with valid object should add when addAlways==true
    EXPECT_TRUE(container_->Replace({}, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);
    EXPECT_FALSE(container_->Replace({}, replaceWith2, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    const auto added =
        ChildChangedInfo { ContainerChangeType::ADDED, replaceWith, container_, size_t(-1), NumDirectChildren };
    EXPECT_THAT(addedCalls_, testing::ElementsAre(added));
    addedCalls_.clear();
    // Replace valid object with null should remove the object regardless of addAlways
    EXPECT_TRUE(container_->Replace(replaceWith, {}, false));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_TRUE(container_->Replace({}, replaceWith2, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);
    EXPECT_TRUE(container_->Replace(replaceWith2, {}, true));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    const auto removed1 = ChildChangedInfo { ContainerChangeType::REMOVED, replaceWith, container_, NumDirectChildren };
    const auto removed2 =
        ChildChangedInfo { ContainerChangeType::REMOVED, replaceWith2, container_, NumDirectChildren };
    const auto added2 =
        ChildChangedInfo { ContainerChangeType::ADDED, replaceWith2, container_, size_t(-1), NumDirectChildren };
    EXPECT_THAT(removedCalls_, testing::ElementsAre(removed1, removed2));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    EXPECT_THAT(addedCalls_, testing::ElementsAre(added2));
}

/**
 * @tc.name: ReplaceAdd
 * @tc.desc: Tests for Replace Add. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, ReplaceAdd, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    const auto replace = interface_pointer_cast<IObject>(CreateTestType("NotThere"));
    const auto replaceWith = interface_pointer_cast<IObject>(CreateTestType("Replaced"));

    // Replace item that is not found but do not add, should not change the container
    EXPECT_FALSE(container_->Replace(replace, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(replaceWith)));
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(replace)));
    // Check events
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));

    // Replace item that is not found but always add
    EXPECT_TRUE(container_->Replace(replace, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), expectedCount + 1);
    EXPECT_THAT(container_->GetAll(), testing::Contains(replaceWith));
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(replace)));
    // Check events
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    const auto added =
        ChildChangedInfo { ContainerChangeType::ADDED, replaceWith, container_, size_t(-1), expectedCount };
    EXPECT_THAT(addedCalls_, testing::ElementsAre(added));
}

/**
 * @tc.name: MoveEmpty
 * @tc.desc: Tests for Move Empty. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveEmpty, testing::ext::TestSize.Level1)
{
    auto moveItem = container_->GetAt(0);
    container_->RemoveAll();
    removedCalls_.clear();
    EXPECT_FALSE(container_->Move(1, 1));
    EXPECT_FALSE(container_->Move(0, 0));
    EXPECT_FALSE(container_->Move(0, 1));
    EXPECT_FALSE(container_->Move(moveItem, 1));
    EXPECT_FALSE(container_->Move(moveItem, 0));

    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: MoveBack
 * @tc.desc: Tests for Move Back. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveBack, testing::ext::TestSize.Level1)
{
    size_t from = NumDirectChildren - 1;
    size_t to = 0;
    auto moveItem = container_->GetAt(from);
    auto moved = ChildChangedInfo { ContainerChangeType::MOVED, moveItem, container_, from, to };
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    // Check events
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
}

/**
 * @tc.name: MoveForward
 * @tc.desc: Tests for Move Forward. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveForward, testing::ext::TestSize.Level1)
{
    // Move front to back
    size_t from = 0;
    size_t to = NumDirectChildren - 1;
    auto moveItem = container_->GetAt(from);
    auto moved = ChildChangedInfo { ContainerChangeType::MOVED, moveItem, container_, from, to };
    EXPECT_TRUE(container_->Move(from, to));

    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
}

/**
 * @tc.name: MoveNext
 * @tc.desc: Tests for Move Next. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveNext, testing::ext::TestSize.Level1)
{
    // Move to next
    size_t from = NumDirectChildren / 2;
    size_t to = from + 1;
    auto moveItem = container_->GetAt(from);
    auto moved = ChildChangedInfo { ContainerChangeType::MOVED, moveItem, container_, from, to };
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
}

/**
 * @tc.name: MoveSame
 * @tc.desc: Tests for Move Same. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveSame, testing::ext::TestSize.Level1)
{
    // Move to next
    size_t from = NumDirectChildren / 2;
    size_t to = from;
    auto moveItem = container_->GetAt(from);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0)); // No move as the indices are the same
}

/**
 * @tc.name: MoveFromBiggerThanSize
 * @tc.desc: Tests for Move From Bigger Than Size. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveFromBiggerThanSize, testing::ext::TestSize.Level1)
{
    // Move past last index to first, should move the last item
    size_t from = NumDirectChildren + 10;
    size_t to = 0;
    auto moveItem = container_->GetAt(NumDirectChildren - 1);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(to), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    auto moved = ChildChangedInfo { ContainerChangeType::MOVED, moveItem, container_, NumDirectChildren - 1, to };
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
}

/**
 * @tc.name: MoveToBiggerThanSize
 * @tc.desc: Tests for Move To Bigger Than Size. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveToBiggerThanSize, testing::ext::TestSize.Level1)
{
    // Move from middle to past last index, should move to end
    size_t from = NumDirectChildren / 2;
    size_t to = NumDirectChildren + 10;
    auto moveItem = container_->GetAt(from);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(NumDirectChildren - 1), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    auto moved = ChildChangedInfo { ContainerChangeType::MOVED, moveItem, container_, from, NumDirectChildren - 1 };
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
}

/**
 * @tc.name: MoveFromToBiggerThanSize
 * @tc.desc: Tests for Move From To Bigger Than Size. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveFromToBiggerThanSize, testing::ext::TestSize.Level1)
{
    // Move past last to past last, should not do anything
    size_t from = NumDirectChildren + 10;
    size_t to = NumDirectChildren + 4;
    auto moveItem = container_->GetAt(NumDirectChildren - 1);
    EXPECT_TRUE(container_->Move(from, to));
    EXPECT_EQ(container_->GetAt(NumDirectChildren - 1), moveItem);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: MoveObject
 * @tc.desc: Tests for Move Object. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, MoveObject, testing::ext::TestSize.Level1)
{
    size_t from = NumDirectChildren - 1;
    size_t to = 0;
    const auto child = container_->GetAt(from);
    auto moved = ChildChangedInfo { ContainerChangeType::MOVED, child, container_, from, to };
    EXPECT_TRUE(container_->Move(child, to));
    EXPECT_EQ(container_->GetAt(to), child);
    EXPECT_EQ(container_->GetSize(), NumDirectChildren);
    // Check events
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
}

/**
 * @tc.name: FindAllNameDirect
 * @tc.desc: Tests for Find All Name Direct. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, FindAllNameDirect, testing::ext::TestSize.Level1)
{
    // Direct child
    auto result1 = container_->FindAll({ "Object1_1", TraversalType::NO_HIERARCHY, {}, false });
    // Child of child
    auto result2 = container_->FindAll({ "Object2_1", TraversalType::NO_HIERARCHY, {}, false });

    // Should find direct child but not child of child
    EXPECT_THAT(result1, testing::SizeIs(1));
    EXPECT_THAT(result2, testing::SizeIs(0));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: SetRequiredInterfacesReplace
 * @tc.desc: Tests for Set Required Interfaces Replace. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, SetRequiredInterfacesReplace, testing::ext::TestSize.Level1)
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
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(replaceWithContainer)));
    EXPECT_THAT(container_->GetAll(), testing::Contains(replace));

    EXPECT_FALSE(container_->Replace(replace, replaceWithContainer, true));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(replaceWithContainer)));
    EXPECT_THAT(container_->GetAll(), testing::Contains(replace));

    EXPECT_TRUE(container_->Replace(replace, replaceWithItem, false));
    EXPECT_EQ(container_->GetSize(), expectedCount);
    EXPECT_THAT(container_->GetAll(), testing::Contains(replaceWithItem));
    EXPECT_THAT(container_->GetAll(), testing::Not(testing::Contains(replace)));
}

/**
 * @tc.name: FindAnyNameDirect
 * @tc.desc: Tests for Find Any Name Direct. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, FindAnyNameDirect, testing::ext::TestSize.Level1)
{
    // Direct child
    auto result1 = container_->FindAny({ "Object1_1", TraversalType::NO_HIERARCHY, {}, false });
    // Child of child
    auto result2 = container_->FindAny({ "Object2_1", TraversalType::NO_HIERARCHY, {}, false });

    // Should find direct child but not child of child
    EXPECT_NE(result1, nullptr);
    EXPECT_EQ(result2, nullptr);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: SetRequiredInterfaces
 * @tc.desc: Tests for Set Required Interfaces. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_ContainerCommonTest, SetRequiredInterfaces, testing::ext::TestSize.Level1)
{
    auto req = interface_cast<IRequiredInterfaces>(container_);
    ASSERT_TRUE(req);
    EXPECT_EQ(container_->GetSize(), NumDirectChildContainers + NumDirectChildTestTypes);
    // Should remove all items not implementing ITestType
    EXPECT_TRUE(req->SetRequiredInterfaces({ ITestType::UID }));
    EXPECT_EQ(container_->GetSize(), NumDirectChildTestTypes);
    // Should remove all items not implementing ITestContainer, leaving no items in the container
    EXPECT_TRUE(req->SetRequiredInterfaces({ ITestContainer::UID }));
    EXPECT_EQ(container_->GetSize(), 0);
    // TestType does not implement ITestContainer so this should fail and nothing should be added
    EXPECT_FALSE(container_->Add(interface_pointer_cast<IObject>(CreateTestType())));
    EXPECT_EQ(container_->GetSize(), 0);
    // TestContainer implements ITestContainer so adding should succeed
    const auto container = interface_pointer_cast<IObject>(CreateTestContainer());
    EXPECT_TRUE(container_->Add(container));
    EXPECT_EQ(container_->GetSize(), 1);
    const auto all = container_->GetAll();
    ASSERT_THAT(all, testing::SizeIs(1));
    EXPECT_EQ(all[0], container);
}

/**
 * @tc.name: FailAddLoop
 * @tc.desc: Tests for Fail Add Loop. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FailAddLoop, testing::ext::TestSize.Level1)
{
    // container_ is already a parent of container2_1_, so adding it as a child
    // of container2_1_ would lead to a loop in the hierarchy
    EXPECT_FALSE(container2_1_->Add(container_));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FailInsertLoop
 * @tc.desc: Tests for Fail Insert Loop. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FailInsertLoop, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(container2_1_->Insert(0, container_));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FailReplaceLoop
 * @tc.desc: Tests for Fail Replace Loop. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FailReplaceLoop, testing::ext::TestSize.Level1)
{
    EXPECT_FALSE(container1_1_->Replace(container1_1_->GetAll()[1], container_));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: ReplaceNullWithExisting
 * @tc.desc: Tests for Replace Null With Existing. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, ReplaceNullWithExisting, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto indexReplace = expectedCount / 2;
    const auto replaceWith = children[indexReplace];

    // Should do nothing as replaceWith is already in the container
    EXPECT_FALSE(container_->Replace({}, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), expectedCount);

    // Check events
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: ReplaceWithExistingAddAlways
 * @tc.desc: Tests for Replace With Existing Add Always. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, ReplaceWithExistingAddAlways, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto indexReplace = expectedCount / 2;
    auto indexReplaceWith = indexReplace + 1;
    // Both items are already in the container
    const auto replace = children[indexReplace];
    const auto replaceWith = children[indexReplaceWith];

    EXPECT_TRUE(container_->Replace(replace, replaceWith, false));
    EXPECT_EQ(container_->GetSize(), expectedCount - 1);

    // Check events
    const auto moved =
        ChildChangedInfo { ContainerChangeType::MOVED, replaceWith, container_, indexReplaceWith, indexReplace };
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    const auto removed = ChildChangedInfo { ContainerChangeType::REMOVED, replace, container_, indexReplace };
    EXPECT_THAT(removedCalls_, testing::ElementsAre(removed));
}

/**
 * @tc.name: ReplaceWithExistingDontAddAlways
 * @tc.desc: Tests for Replace With Existing Dont Add Always. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, ReplaceWithExistingDontAddAlways, testing::ext::TestSize.Level1)
{
    const auto children = container_->GetAll();
    const auto expectedCount = NumDirectChildren;
    ASSERT_EQ(children.size(), expectedCount);

    auto indexReplace = expectedCount / 2;
    auto indexReplaceWith = indexReplace + 1;
    // Both items are already in the container
    const auto replace = children[indexReplace];
    const auto replaceWith = children[indexReplaceWith];

    EXPECT_TRUE(container_->Replace(replace, replaceWith, true));
    EXPECT_EQ(container_->GetSize(), expectedCount - 1);

    // Check events
    const auto moved =
        ChildChangedInfo { ContainerChangeType::MOVED, replaceWith, container_, indexReplaceWith, indexReplace };
    EXPECT_THAT(movedCalls_, testing::ElementsAre(moved));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    const auto removed = ChildChangedInfo { ContainerChangeType::REMOVED, replace, container_, indexReplace };
    EXPECT_THAT(removedCalls_, testing::ElementsAre(removed));
}

/**
 * @tc.name: AddChildTwice
 * @tc.desc: Tests for Add Child Twice. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, AddChildTwice, testing::ext::TestSize.Level1)
{
    const auto child = interface_pointer_cast<IObject>(CreateTestType("Twice"));
    EXPECT_TRUE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    // Shouldn't do anything, adding second time fails
    EXPECT_FALSE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    const auto children = container_->FindAll({ "Twice", TraversalType::NO_HIERARCHY });
    ASSERT_THAT(children, testing::SizeIs(1));
    EXPECT_EQ(children[0], child);

    // Check events
    ASSERT_THAT(addedCalls_, testing::SizeIs(1));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));
    auto expected = ChildChangedInfo { ContainerChangeType::ADDED, child, container_, size_t(-1), NumDirectChildren };
    EXPECT_EQ(addedCalls_[0], expected);
}

/**
 * @tc.name: FindAllEmptyName
 * @tc.desc: Tests for Find All Empty Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAllEmptyName, testing::ext::TestSize.Level1)
{
    auto result1 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result2 = container_->FindAll({ "", TraversalType::NO_HIERARCHY, { ITestContainer::UID }, false });
    auto result3 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });
    auto result4 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID }, false });

    EXPECT_THAT(result1, testing::SizeIs(NumChildContainers + NumChildTestTypes)); // All children of container_
    EXPECT_THAT(result2, testing::SizeIs(1));                  // The one direct container child of container_
    EXPECT_THAT(result3, testing::SizeIs(NumChildContainers)); // All container children of container_
    EXPECT_THAT(result4, testing::SizeIs(NumChildTestTypes));  // Rest of the items
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAllNameRecursive
 * @tc.desc: Tests for Find All Name Recursive. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAllNameRecursive, testing::ext::TestSize.Level1)
{
    // Direct child
    auto result1 = container_->FindAll({ "Object1_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    // Child of child
    auto result2 = container_->FindAll({ "Object2_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });

    // Should find direct child and child of child
    EXPECT_THAT(result1, testing::SizeIs(1));
    EXPECT_THAT(result2, testing::SizeIs(1));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAllNameDuplicate
 * @tc.desc: Tests for Find All Name Duplicate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAllNameDuplicate, testing::ext::TestSize.Level1)
{
    auto result1 = container_->FindAll({ "ObjectDupe", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAll({ "ObjectDupe", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });

    // DIRECT should find only the direct child
    EXPECT_THAT(result1, testing::SizeIs(1));
    // RECURSIVE should find both direct child and child of child
    EXPECT_THAT(result2, testing::SizeIs(2));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAllUid
 * @tc.desc: Tests for Find All Uid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAllUid, testing::ext::TestSize.Level1)
{
    auto result1 = container_->FindAll({ "", TraversalType::NO_HIERARCHY, { ITestContainer::UID }, false });
    auto result2 = container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });
    auto result3 = container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result4 = container_->FindAll(
        { "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });

    // DIRECT should find only the direct children
    EXPECT_THAT(result1, testing::SizeIs(1));
    // RECURSIVE should find both direct children and children of children
    EXPECT_THAT(result2, testing::SizeIs(2));
    // Should both objects from hierarchy which have the same name (but different uids)
    EXPECT_THAT(result3, testing::SizeIs(3));
    // Should only find the one child which is also ITestContainer
    EXPECT_THAT(result4, testing::SizeIs(1));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAllUidStrict
 * @tc.desc: Tests for Find All Uid Strict. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAllUidStrict, testing::ext::TestSize.Level1)
{
    auto result1 = container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER,
        { ITestType::UID, ITestContainer::UID }, true });
    auto result2 =
        container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID }, true });
    auto result3 = container_->FindAll({ "SameNameDifferentUid", TraversalType::DEPTH_FIRST_PRE_ORDER,
        { ITestType::UID, ITestContainer::UID }, false });
    // No object implements both ITestType and ITestContainer
    EXPECT_THAT(result1, testing::SizeIs(0));
    // Only one of the objects has name "SameNameDifferentUid" and implements ITestType
    EXPECT_THAT(result2, testing::SizeIs(2));
    // Two objects have name "SameNameDifferentUid" and implement either ITestType or ITestContainer
    EXPECT_THAT(result3, testing::SizeIs(3));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAllInvalid
 * @tc.desc: Tests for Find All Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAllInvalid, testing::ext::TestSize.Level1)
{
    // Direct child
    auto result1 = container_->FindAll({ "InvalidObject", TraversalType::NO_HIERARCHY, {}, false });
    // Child of child
    auto result2 = container_->FindAll({ "InvalidObject", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result3 =
        container_->FindAll({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { META_NS::IAttachment::UID }, false });
    auto result4 = container_->FindAll(
        { "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestType::UID, META_NS::IAttachment::UID }, true });

    EXPECT_THAT(result1, testing::SizeIs(0));
    EXPECT_THAT(result2, testing::SizeIs(0));
    EXPECT_THAT(result3, testing::SizeIs(0));
    EXPECT_THAT(result4, testing::SizeIs(0));
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAnyEmptyName
 * @tc.desc: Tests for Find Any Empty Name. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAnyEmptyName, testing::ext::TestSize.Level1)
{
    auto result1 = container_->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    auto result2 = container_->FindAny({ "", TraversalType::NO_HIERARCHY, { ITestContainer::UID }, false });
    auto result3 = container_->FindAny({ "", TraversalType::DEPTH_FIRST_PRE_ORDER, { ITestContainer::UID }, false });

    EXPECT_NE(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_NE(result3, nullptr);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAnyNameRecursive
 * @tc.desc: Tests for Find Any Name Recursive. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAnyNameRecursive, testing::ext::TestSize.Level1)
{
    // Direct child
    auto result1 = container_->FindAny({ "Object1_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    // Child of child
    auto result2 = container_->FindAny({ "Object2_1", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });
    // Same as above but with helper template
    auto result3 = container_->FindAnyFromHierarchy<IObject>("Object2_1");

    // Should find direct child and child of child
    EXPECT_NE(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_EQ(result2, result3);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAnyNameDuplicate
 * @tc.desc: Tests for Find Any Name Duplicate. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAnyNameDuplicate, testing::ext::TestSize.Level1)
{
    auto result1 = container_->FindAny({ "ObjectDupe", TraversalType::NO_HIERARCHY, {}, false });
    auto result2 = container_->FindAny({ "ObjectDupe", TraversalType::DEPTH_FIRST_PRE_ORDER, {}, false });

    EXPECT_NE(result1, nullptr);
    EXPECT_NE(result2, nullptr);
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAnyUid
 * @tc.desc: Tests for Find Any Uid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAnyUid, testing::ext::TestSize.Level1)
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
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAnyUidStrict
 * @tc.desc: Tests for Find Any Uid Strict. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAnyUidStrict, testing::ext::TestSize.Level1)
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
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: FindAnyInvalid
 * @tc.desc: Tests for Find Any Invalid. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindAnyInvalid, testing::ext::TestSize.Level1)
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
    EXPECT_THAT(addedCalls_, testing::SizeIs(0));
    EXPECT_THAT(removedCalls_, testing::SizeIs(0));
    EXPECT_THAT(movedCalls_, testing::SizeIs(0));
}

/**
 * @tc.name: ObjectFlagsDefault
 * @tc.desc: Tests for Object Flags Default. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
 UNIT_TEST_F(API_ContainerTest, ObjectFlagsDefault, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    ASSERT_TRUE(ser.Export(container_));

    auto imported = ser.Import<IContainer>();
    ASSERT_TRUE(imported);

    EXPECT_EQ(imported->GetAll().size(), NumDirectChildren);
}
*/

/**
 * @tc.name: ObjectFlagsNoHierarchy
 * @tc.desc: Tests for Object Flags No Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST_F(API_ContainerTest, ObjectFlagsNoHierarchy, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    auto flags = interface_cast<IObjectFlags>(container_);
    ASSERT_TRUE(flags);

    auto objectFlags = flags->GetObjectFlags();
    objectFlags.Clear(ObjectFlagBits::SERIALIZE_HIERARCHY);
    flags->SetObjectFlags(objectFlags);

    ASSERT_TRUE(ser.Export(container_));

    auto imported = ser.Import<IContainer>();
    ASSERT_TRUE(imported);

    EXPECT_EQ(imported->GetAll().size(), 0);
}
*/

/**
 * @tc.name: IterationSupport
 * @tc.desc: Tests for Iteration Support. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, IterationSupport, testing::ext::TestSize.Level1)
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
 * @tc.name: IterationSupportNonIterable
 * @tc.desc: Tests for Iteration Support Non Iterable. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, IterationSupportNonIterable, testing::ext::TestSize.Level1)
{
    auto nonContainer = GetObjectRegistry().Create(ClassId::Object);
    int count = 0;
    auto fnv = [&](const IObject::Ptr&) { ++count; };
    auto fnr = [&](const IObject::Ptr&) {
        ++count;
        return true;
    };

    {
        auto f = fnv;
        ForEachShared(nonContainer, BASE_NS::move(f));
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        IterateShared(nonContainer, BASE_NS::move(f));
        EXPECT_EQ(count, 0);
    }

    {
        auto f = fnr;
        ForEachShared(nonContainer, BASE_NS::move(f), TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        IterateShared(nonContainer, BASE_NS::move(f), TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        ForEachUnique(nonContainer, BASE_NS::move(f), TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        IterateUnique(nonContainer, BASE_NS::move(f), TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        IterateShared(nonContainer, BASE_NS::move(f), TraversalType::DEPTH_FIRST_PRE_ORDER);
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        ConstIterate(nonContainer, MakeIterationConstCallable(BASE_NS::move(f)),
            IterateStrategy { TraversalType::DEPTH_FIRST_PRE_ORDER, LockType::NO_LOCK });
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        ConstIterate(nonContainer, MakeIterationConstCallable(BASE_NS::move(f)),
            IterateStrategy { TraversalType::FULL_HIERARCHY, LockType::UNIQUE_LOCK });
        EXPECT_EQ(count, 0);
    }
    {
        auto f = fnr;
        ConstIterate(nonContainer, MakeIterationConstCallable(BASE_NS::move(f)),
            IterateStrategy { TraversalType::BREADTH_FIRST_ORDER, LockType::UNIQUE_LOCK });
        EXPECT_EQ(count, 0);
    }
}

/**
 * @tc.name: FindOrder
 * @tc.desc: Tests for Find Order. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerTest, FindOrder, testing::ext::TestSize.Level1)
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

    // 1
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
    // 3
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
    // 4
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
 * @tc.name: SameItemMultipleTimes
 * @tc.desc: Tests for Same Item Multiple Times. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FlatContainerTest, SameItemMultipleTimes, testing::ext::TestSize.Level1)
{
    const auto child = interface_pointer_cast<IObject>(CreateTestType("Twice"));
    EXPECT_TRUE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 1);

    EXPECT_TRUE(container_->Add(child));
    EXPECT_EQ(container_->GetSize(), NumDirectChildren + 2);

    const auto children = container_->FindAll({ "Twice", TraversalType::NO_HIERARCHY });
    ASSERT_THAT(children, testing::SizeIs(2));
    EXPECT_EQ(children[0], child);
    EXPECT_EQ(children[1], child);

    // Check events
    ASSERT_THAT(addedCalls_, testing::SizeIs(2));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));
    auto expected1 = ChildChangedInfo { ContainerChangeType::ADDED, child, container_, size_t(-1), NumDirectChildren };
    auto expected2 =
        ChildChangedInfo { ContainerChangeType::ADDED, child, container_, size_t(-1), NumDirectChildren + 1 };
    EXPECT_EQ(addedCalls_[0], expected1);
    EXPECT_EQ(addedCalls_[1], expected2);
}

/**
 * @tc.name: IterationSupport
 * @tc.desc: Tests for Iteration Support. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_FlatContainerTest, IterationSupport, testing::ext::TestSize.Level1)
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

static std::string BuildTestName(const testing::TestParamInfo<API_ContainerCommonTest::ParamType>& info)
{
    return info.param == META_NS::ClassId::TestContainer ? "TestContainer" : "TestFlatContainer";
}

INSTANTIATE_TEST_SUITE_P(API_ContainerTests, API_ContainerCommonTest,
    testing::Values(META_NS::ClassId::TestContainer, META_NS::ClassId::TestFlatContainer), BuildTestName);

} // namespace UTest
META_END_NAMESPACE()
