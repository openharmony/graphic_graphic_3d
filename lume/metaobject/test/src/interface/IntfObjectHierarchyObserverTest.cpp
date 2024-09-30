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

#include <meta/api/container/object_hierarchy_observer.h>
#include <meta/api/content_object.h>
#include <meta/api/make_callback.h>
#include <meta/base/ref_uri.h>

#include "src/serialisation_utils.h"
#include "src/util.h"
#include "src/test_runner.h"
#include "src/test_utils.h"
#include "src/testing_objects.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

size_t GetMatching(const BASE_NS::vector<HierarchyChangedInfo>& changedCalls, HierarchyChangeType change,
    HierarchyChangeObjectType objectType)
{
    auto added = 0;
    for (const auto& info : changedCalls) {
        if (info.change == change && info .objectType == objectType) {
            added++;
        }
    }
    return added;
}

MATCHER_P(AddingChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDING, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(RemovingChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(AddedChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(RemovedChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(MovedChildrenEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::MOVED, HierarchyChangeObjectType::CHILD) == expected;
}
MATCHER_P(AddingContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDING, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(RemovingContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(AddedContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(RemovedContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(MovedContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::MOVED, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(AddingAttachmentrEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDING, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(RemovingAttachmentrEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(AddedAttachmentrEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(RemovedAttachmentrEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(MovedAttachmentrEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::MOVED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}

MATCHER_P(ObjectEq, expected, "")
{
    return interface_pointer_cast<IObject>(arg) == interface_pointer_cast<IObject>(expected);
}

MATCHER(ObjectIsNull, "")
{
    return interface_pointer_cast<IObject>(arg) == nullptr;
}

class IntfObjectHierarchyObserverTest : public ::testing::Test {
public:
    void SetUp() override
    {
        container_ = interface_pointer_cast<IContainer>(CreateTestContainer("Base"));
        ASSERT_NE(container_, nullptr);
        ASSERT_NE(observer_.GetIObject(), nullptr);
        container_->Add(CreateTestType<IObject>("Object1_1"));
        container_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_ = interface_pointer_cast<IContainer>(CreateTestContainer("Container1_1"));
        container_->Add(interface_pointer_cast<IObject>(container1_1_));
        container1_1_->Add(CreateTestType<IObject>("Object2_1"));
        container1_1_->Add(CreateTestType<IObject>("ObjectDupe"));
        container1_1_->Add(CreateTestType<IObject>("SameNameDifferentUid"));
        container_->Add(CreateTestType<IObject>("Object1_3"));
        container2_1_ = interface_pointer_cast<IContainer>(CreateTestContainer("SameNameDifferentUid"));
        container2_1_->Add(contentObject_);
        container1_1_->Add(interface_pointer_cast<IObject>(container2_1_));

        observer_.Target(interface_pointer_cast<IObject>(container_))
            .OnHierarchyChanged([this](const HierarchyChangedInfo& info) { OnHierarchyChanged(info); });
    }
    void TearDown() override
    {
        container_.reset();
        observer_.ResetIObject();
    }
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }
public:
    struct ExpectedCallCount {
        size_t addingChildren {};
        size_t addedChildren {};
        size_t removingChildren {};
        size_t removedChildren {};
        size_t movedChildren {};

        size_t addingContent {};
        size_t addedContent {};
        size_t removingContent {};
        size_t removedContent {};

        size_t addingAttachments {};
        size_t addedAttachments {};
        size_t removingAttachments {};
        size_t removedAttachments {};
        size_t movedAttachments {};

        void ExpectAddChildren(size_t count = 1)
        {
            addingChildren += count;
            addedChildren += count;
        }

        void ExpectAddContent(size_t count = 1)
        {
            addingContent += count;
            addedContent += count;
        }

        void ExpectAddAttachments(size_t count = 1)
        {
            addingAttachments += count;
            addedAttachments += count;
        }

        void ExpectRemoveChildren(size_t count = 1)
        {
            removingChildren += count;
            removedChildren += count;
        }

        void ExpectRemoveContent(size_t count = 1)
        {
            removingContent += count;
            removedContent += count;
        }

        void ExpectRemoveAttachments(size_t count = 1)
        {
            removingAttachments += count;
            removedAttachments += count;
        }

        size_t Sum() const
        {
            return addingChildren + addedChildren + removingChildren + removedChildren + movedChildren + addingContent +
                addedContent + removingContent + removedContent + addingAttachments + addedAttachments +
                removingAttachments + removedAttachments + movedAttachments;
        }
    };

    void VerifyCalls(const ExpectedCallCount& expected)
    {
        EXPECT_THAT(changedCalls_, AddingChildrenEq(expected.addingChildren));
        EXPECT_THAT(changedCalls_, AddedChildrenEq(expected.addedChildren));
        EXPECT_THAT(changedCalls_, RemovingChildrenEq(expected.removingChildren));
        EXPECT_THAT(changedCalls_, RemovedChildrenEq(expected.removedChildren));
        EXPECT_THAT(changedCalls_, MovedChildrenEq(expected.movedChildren));
        EXPECT_THAT(changedCalls_, AddingContentEq(expected.addingContent));
        EXPECT_THAT(changedCalls_, AddedContentEq(expected.addedContent));
        EXPECT_THAT(changedCalls_, RemovingContentEq(expected.removingContent));
        EXPECT_THAT(changedCalls_, RemovedContentEq(expected.removedContent));
        EXPECT_THAT(changedCalls_, AddingAttachmentsEq(expected.addingAttachments));
        EXPECT_THAT(changedCalls_, AddedAttachmentsEq(expected.addedAttachments));
        EXPECT_THAT(changedCalls_, RemovingAttachmentsEq(expected.removingAttachments));
        EXPECT_THAT(changedCalls_, RemovedAttachmentsEq(expected.removedAttachments));
        EXPECT_THAT(changedCalls_, MovedAttachmentsEq(expected.movedAttachments));
        ASSERT_EQ(changedCalls_.size(), expected.size());
    }
    void ResetCalls()
    {
        changedCalls_.clear();
    }
protected:
    void OnHierachyChanged(const HierarchyChangedInfo& info)
    {
        changedCalls_.push_back(info);
    }

    BASE_NS::vector<HierarchyChangedInfo> changedCalls_;

    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
    META_NS::ObjectHierarchyObserver observer_;
};

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test AddDirectChild function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, AddDirectChild, TestSize.Level1)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container_->Add(child));

    ExpectedCallCount expected;
    expected.ExpectAddChildren();
    VerifyCalls(expected);

    auto& info = changeCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_THAT(info.parent.lock(), ObjectEq(container_));
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test AddDescendantChild function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, AddDescendantChild, TestSize.Level1)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container2_1_->Add(child));

    ExpectedCallCount expected;
    expected.ExpectAddChildren();
    VerifyCalls(expected);

    auto& info = changeCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_THAT(info.parent.lock(), ObjectEq(container2_1_));
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test AddDescendantContent function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, AddDescendantContent, TestSize.Level1)
{
    auto child = CreateTestType<IObject>("Content");
    auto child2 = CreateTestType<IObject>("Content2");
    {
        contentObject_ = SetContent(child);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        VerifyCalls(expected);
        auto& info = changeCalls_[0];
        EXPECT_EQ(info.object, child);
        EXPECT_THAT(info.parent.lock(), ObjectEq(contentObject));
    }
    ResetCalls();
    {
        contentObject_ = SetContent(child2);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        expected.ExpectRemoveContent();
        VerifyCalls(expected);
        auto& remove = changedCalls_[0];
        auto& added = changedCalls_[2];
        EXPECT_EQ(remove.object, child);
        EXPECT_THAT(remove.parent.lock(), ObjectEq(contentObject));
        EXPECT_EQ(added.object, child);
        EXPECT_THAT(added.parent.lock(), ObjectEq(contentObject));
    }
    ResetCalls();
    {
        contentObject_.SetContent(nullptr);
        ExpectedCallCount expected;
        expected.ExpectRemoveContent();
        VerifyCalls(expected);
        auto& remove = changedCalls_[0];
        EXPECT_EQ(remove.object, child2);
        EXPECT_THAT(remove.parent.lock(), ObjectEq(contentObject));
    }
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test ContentChangeAfterRemove function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, ContentChangeAfterRemove, TestSize.Level1)
{
    auto child = CreateTestType<IObject>("Content");
    {
        contentObject_ = SetContent(child);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        VerifyCalls(expected);
        auto& info = changeCalls_[0];
        EXPECT_EQ(info.object, child);
        EXPECT_THAT(info.parent.lock(), ObjectEq(contentObject));
    }
    ResetCalls();
    {
        EXPECT_TRUE(container2_1->Remove(contentObject_));
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        auto& info = changeCalls_[0];
        EXPECT_EQ(info.object, child);
        EXPECT_THAT(info.parent.lock(), ObjectEq(container2_1));
    }
    ResetCalls();
    {
        contentObject_.SetContent(nullptr);
        VerifyCalls({});
    }
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test Properties function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, Properties, TestSize.Level1)
{
    auto obj = CreateTestType();
    auto meta = interface_cast<IMetadata>(obj);

    ObjectHierarchyObserver obs;
    HierarchyChangedInfo info;

    obs.Target(interface_pointer_cast<IObject>(meta->GetPropertyContainer()),
        HierarchyChangeModeValue(HierarchyChangeMode::NOTIFY_CONTAINER | HierarchyChangeMode::NOTIFY_OBJECT));
    obs.OnHierarchyChanged([&info](const HierarchyChangedInfo& i) { info = i; });

    obj->First()->SetValue(2);
    EXPECT_EQ(interface_pointer_cast<IProperty>(info.object), obj->First().GetProperty());
    EXPECT_EQ(info.change, HierarchyChangeType::CHANGED);
    EXPECT_EQ(info.objectType, HierarchyChangeObjectType::CHILD);
    EXPECT_EQ(info.parent.lock(), interface_pointer_cast<IObject>(meta->GetPropertyContainer()));

    obj->Second()->SetValue("hips");
    EXPECT_EQ(interface_pointer_cast<IProperty>(info.object), obj->Second().GetProperty());
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test GetAll function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, GetAll, TestSize.Level1)
{
    ObjectHierarchyObserver obs;
    EXPECT_THAT(obs.GetAllObserved(), IsEmpty());

    auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));

    auto container = interface_pointer_cast<IContainer>(containerObject);
    container->Add(childObject);

    EXPECT_TRUE(obs.Target(containerObject));
    EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject, childObject));
    EXPECT_THAT(obs.GetAllObserved<IContainer>(), UnorderedElementsAre(container));

    container->Remove(childObject);
    EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject));
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test Remove function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, Remove, TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs;
    auto callCount = 0;

    obs.OnHierarchyChanged([&removed, &callCount](const HierarchyChangedInfo& info) {
        if (info.change == HierarchyChangeType::REMOVED) {
            callCount++;
            removed.push_back(info.object);
        }
    });

    auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childContainerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    {
        auto container = interface_pointer_cast<IContainer>(containerObject);
        auto childContainer = interface_pointer_cast<IContainer>(childContainerObject);
        EXPECT_TRUE(container->Add(childObject));
        EXPECT_TRUE(childContainer->Add(childContainerObject));
        EXPECT_TRUE(obs.Target(childObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject, childContainerObject, childObject));

        container->RemoveAll();
    }
    EXPECT_EQ(callCount, 1);
    EXPECT_THAT(removed, ElementsAre(childContainerObject));
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test DestroyEvent function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, DestroyEvent, TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs;
    auto callCount = 0;
    auto removingCount = 0;

    auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childContainerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    auto childObject2 = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    auto childObject3 = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    auto childObject4 = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    {
        auto container = interface_pointer_cast<IContainer>(containerObject);
        auto childContainer = interface_pointer_cast<IContainer>(childContainerObject);
        EXPECT_TRUE(container->Add(childContainerObject));
        EXPECT_TRUE(container->Add(childObject2));
        EXPECT_TRUE(container->Add(childObject3));
        EXPECT_TRUE(container->Insert(1, childObject4));
        EXPECT_TRUE(childContainer->Add(childObject));
        EXPECT_TRUE(obs.Target(containerObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject, childContainerObject, childObject,
                                               childObject2, childObject3, childObject4));

        container->RemoveAll();

        EXPECT_THAT(obs.GetAllObserved(),
            UnorderedElementsAre(containerObject, childContainerObject, childObject, childObject4, childObject2));
    }

    obs.OnHierarchyChanged([&](const HierarchyChangedInfo& info) {
        if (info.change == HierarchyChangeType::REMOVED) {
            callCount++;
            removed.push_back(info.object);
        }
        if (info.change == HierarchyChangeType::REMOVING) {
            ++removingCount;
        }
    });

    containerObject.reset();
    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(removingCount, 3);
    EXPECT_THAT(removed, UnorderedElementsAre(childContainerObject, childObject4, childObject2));
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test DestroyEventContent function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, DestroyEventContent, TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs;
    auto callCount = 0;

    obs.OnHierarchyChanged([&removed, &callCount](const HierarchyChangedInfo& info) {
        if (info.change == HierarchyChangeType::REMOVED) {
            callCount++;
            removed.push_back(info.object);
        }
    });
    auto contentObject = META_NS::GetObjectRegistry().Create(ClassId::ContentObject);
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    {
        auto content = interface_pointer_cast<IContent>(contentObject);
        EXPECT_TRUE(content->SetContent(childObject));
        EXPECT_TRUE(obs.Target(contentObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(contentObject, childObject));
    }

    contentObject.reset();
    EXPECT_EQ(callCount, 1);
    EXPECT_THAT(removed, ElementsAre(childObject));
}

/**
 * @tc.name: IntfObjectHierarchyObserverTest
 * @tc.desc: test DoNotSerialise function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfObjectHierarchyObserverTest, DoNotSerialise, TestSize.Level1)
{
    TestSerialiser ser;
    {
        ObjectHierarchyObserver obs;
        auto containerObject = interface_pointer_cast<IObject>(CreateTestType("Container"));
        EXPECT_TRUE(obs.Target(containerObject));
        EXPECT_THAT(obs.GetAllObserved(), UnorderedElementsAre(containerObject));
        auto i = interface_cast<IAttach>(containerObject);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetAttachments().size(), 1);
        ser.Export(containerObject);
    }

    auto obj = ser.Import<IAttach>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetAttachments().size(), 0);
}
META_END_NAMESPACE()