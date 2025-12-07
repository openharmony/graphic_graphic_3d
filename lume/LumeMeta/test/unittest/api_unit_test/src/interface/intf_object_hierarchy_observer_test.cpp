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

#include <meta/api/container/observer.h>
#include <meta/api/make_callback.h>
#include <meta/base/ref_uri.h>

#include "helpers/serialisation_utils.h"
#include "helpers/test_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()

namespace UTest {

size_t GetMatching(const BASE_NS::vector<HierarchyChangedInfo>& changedCalls, HierarchyChangeType change,
    HierarchyChangeObjectType objectType)
{
    auto added = 0;
    for (const auto& info : changedCalls) {
        if (info.change == change && info.objectType == objectType) {
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
MATCHER_P(AddedContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(RemovingContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(RemovedContentEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::CONTENT) == expected;
}
MATCHER_P(AddingAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDING, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(RemovingAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVING, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(AddedAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::ADDED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(RemovedAttachmentsEq, expected, "")
{
    return GetMatching(arg, HierarchyChangeType::REMOVED, HierarchyChangeObjectType::ATTACHMENT) == expected;
}
MATCHER_P(MovedAttachmentsEq, expected, "")
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

class API_ObjectHierarchyObserverTest : public ::testing::Test {
    void SetUp() override
    {
        container_ = interface_pointer_cast<IContainer>(CreateTestContainer("Base"));
        ASSERT_NE(container_, nullptr);
        ASSERT_NE(observer_.GetPtr(), nullptr);
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

        observer_.SetTarget(interface_pointer_cast<IObject>(container_));
        observer_.OnHierarchyChanged([this](const HierarchyChangedInfo& info) { OnHierarchyChanged(info); });
    }
    void TearDown() override
    {
        container_.reset();
        observer_.Release();
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
        void ExpectAddAttachments(size_t count = 1)
        {
            addingAttachments += count;
            addedAttachments += count;
        }
        void ExpectAddContent(size_t count = 1)
        {
            addingContent += count;
            addedContent += count;
        }
        void ExpectRemoveChildren(size_t count = 1)
        {
            removingChildren += count;
            removedChildren += count;
        }
        void ExpectRemoveAttachments(size_t count = 1)
        {
            removingAttachments += count;
            removedAttachments += count;
        }
        void ExpectRemoveContent(size_t count = 1)
        {
            removingContent += count;
            removedContent += count;
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
        ASSERT_EQ(changedCalls_.size(), expected.Sum());
    }
    void ResetCalls()
    {
        changedCalls_.clear();
    }

protected:
    void OnHierarchyChanged(const HierarchyChangedInfo& info)
    {
        changedCalls_.push_back(info);
    }

    BASE_NS::vector<HierarchyChangedInfo> changedCalls_;

    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
    META_NS::ContentObject contentObject_ { CreateInstance(META_NS::ClassId::ContentObject) };
    META_NS::ObjectHierarchyObserver observer_ { CreateInstance(META_NS::ClassId::ObjectHierarchyObserver) };
};

/**
 * @tc.name: AddDirectChild
 * @tc.desc: Tests for Add Direct Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectHierarchyObserverTest, AddDirectChild, testing::ext::TestSize.Level1)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container_->Add(child));

    ExpectedCallCount expected;
    expected.ExpectAddChildren();
    VerifyCalls(expected);
    ASSERT_TRUE(!changedCalls_.empty());
    auto& info = changedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_THAT(info.parent.lock(), ObjectEq(container_));
}

/**
 * @tc.name: AddDescendantChild
 * @tc.desc: Tests for Add Descendant Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectHierarchyObserverTest, AddDescendantChild, testing::ext::TestSize.Level1)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container2_1_->Add(child));

    ExpectedCallCount expected;
    expected.ExpectAddChildren();
    VerifyCalls(expected);
    ASSERT_TRUE(!changedCalls_.empty());
    auto& info = changedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_THAT(info.parent.lock(), ObjectEq(container2_1_));
}

/**
 * @tc.name: AddDescendantContent
 * @tc.desc: Tests for Add Descendant Content. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectHierarchyObserverTest, AddDescendantContent, testing::ext::TestSize.Level1)
{
    auto child = CreateTestType<IObject>("Content");
    auto child2 = CreateTestType<IObject>("Content2");
    {
        // Set content
        contentObject_.SetContent(child);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& info = changedCalls_[0];
        EXPECT_EQ(info.object, child);
        EXPECT_THAT(info.parent.lock(), ObjectEq(contentObject_));
    }
    ResetCalls();
    {
        // Change content
        contentObject_.SetContent(child2);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        expected.ExpectRemoveContent();
        VerifyCalls(expected);
        ASSERT_TRUE(changedCalls_.size() > 2);
        auto& removed = changedCalls_[0];
        auto& added = changedCalls_[2];
        EXPECT_EQ(removed.object, child);
        EXPECT_THAT(removed.parent.lock(), ObjectEq(contentObject_));
        EXPECT_EQ(added.object, child2);
        EXPECT_THAT(added.parent.lock(), ObjectEq(contentObject_));
    }
    ResetCalls();
    {
        // Remove content
        contentObject_.SetContent(nullptr);
        ExpectedCallCount expected;
        expected.ExpectRemoveContent();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& removed = changedCalls_[0];
        EXPECT_EQ(removed.object, child2);
        EXPECT_THAT(removed.parent.lock(), ObjectEq(contentObject_));
    }
}

/**
 * @tc.name: ContentChangeAfterRemove
 * @tc.desc: Tests for Content Change After Remove. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectHierarchyObserverTest, ContentChangeAfterRemove, testing::ext::TestSize.Level1)
{
    auto child = CreateTestType<IObject>("Content");
    {
        contentObject_.SetContent(child);
        ExpectedCallCount expected;
        expected.ExpectAddContent();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& info = changedCalls_[0];
        EXPECT_EQ(info.object, child);
        EXPECT_THAT(info.parent.lock(), ObjectEq(contentObject_));
    }
    ResetCalls();
    {
        EXPECT_TRUE(container2_1_->Remove(contentObject_));
        ExpectedCallCount expected;
        expected.ExpectRemoveChildren();
        VerifyCalls(expected);
        ASSERT_TRUE(!changedCalls_.empty());
        auto& info = changedCalls_[0];
        EXPECT_EQ(info.object, contentObject_);
        EXPECT_THAT(info.parent.lock(), ObjectEq(container2_1_));
    }
    ResetCalls();
    {
        // Not in hierarchy anymore so we should not get any events
        contentObject_.SetContent(nullptr);
        VerifyCalls({});
    }
}

/**
 * @tc.name: ContainerPreTransaction
 * @tc.desc: Test pre transaction events.
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectHierarchyObserverTest, ContainerPreTransaction, testing::ext::TestSize.Level1)
{
    auto tc = CreateTestContainer("pretrans", ClassId::TestContainerPreTransaction);
    auto container = interface_cast<IContainer>(tc);
    auto object = CreateTestType("object");
    auto pre = interface_cast<IContainerPreTransaction>(container);
    ASSERT_TRUE(pre);
    bool succeed = true;

    uint32_t addingCount {};
    uint32_t removingCount {};
    pre->ContainerAboutToChange()->AddHandler(
        MakeCallback<IOnChildChanging>([&](const ChildChangedInfo& info, bool& success) {
            switch (info.type) {
                case ContainerChangeType::ADDING:
                    success = succeed;
                    addingCount++;
                    break;
                case ContainerChangeType::REMOVING:
                    success = succeed;
                    removingCount++;
                    break;
                default:
                    break;
            }
        }));

    uint32_t changeCount {};
    observer_.SetTarget(interface_pointer_cast<IObject>(tc));
    observer_.OnHierarchyChanged([&](const HierarchyChangedInfo& info) { changeCount++; });

    auto checkGT = [](uint32_t& current, uint32_t& previous) {
        EXPECT_GT(current, previous);
        previous = current;
    };

    auto previousAdding = addingCount;
    auto previousRemoving = removingCount;
    auto previousChange = changeCount;
    succeed = false;
    container->Add(object);
    checkGT(addingCount, previousAdding);
    checkGT(changeCount, previousChange);
    EXPECT_EQ(container->GetSize(), 0);
    succeed = true;
    container->Add(object);
    checkGT(addingCount, previousAdding);
    checkGT(changeCount, previousChange);
    EXPECT_EQ(container->GetSize(), 1);
    succeed = false;
    container->Remove(object);
    checkGT(removingCount, previousRemoving);
    checkGT(changeCount, previousChange);
    EXPECT_EQ(container->GetSize(), 1);
    succeed = true;
    container->Remove(object);
    checkGT(removingCount, previousRemoving);
    checkGT(changeCount, previousChange);
    EXPECT_EQ(container->GetSize(), 0);
}

/**
 * @tc.name: Attachment
 * @tc.desc: Tests for Attachment. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectHierarchyObserverTest, Attachment, testing::ext::TestSize.Level1)
{
    auto& obr = GetObjectRegistry();
    auto attachment1 = obr.Create<IAttachment>(ClassId::TestAttachment);
    auto test1 = interface_cast<ITestAttachment>(attachment1);
    ASSERT_TRUE(test1);
    {
        AttachmentContainer(contentObject_).Attach(attachment1);
        ExpectedCallCount expected;
        expected.ExpectAddAttachments();
        VerifyCalls(expected);
        EXPECT_EQ(test1->GetAttachCount(), 1);
        EXPECT_EQ(test1->GetDetachCount(), 0);
        EXPECT_THAT(contentObject_, ObjectEq(GetValue(attachment1->AttachedTo())));
    }
    ResetCalls();
    {
        AttachmentContainer(contentObject_).Detach(attachment1);
        ExpectedCallCount expected;
        expected.ExpectRemoveAttachments();
        VerifyCalls(expected);
        EXPECT_EQ(test1->GetAttachCount(), 1);
        EXPECT_EQ(test1->GetDetachCount(), 1);
        EXPECT_THAT(GetValue(attachment1->AttachedTo()), ObjectIsNull());
    }
}

/**
 * @tc.name: ContainerDescendantHierarchy
 * @tc.desc: Tests for Container Descendant Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ObjectHierarchyObserverTest, ContainerDescendantHierarchy, testing::ext::TestSize.Level1)
{
    auto container = CreateTestContainer<IContainer>("DescendamtHierarchy");
    container->Add(CreateTestType<IObject>("DescendamtChild"));
    auto childContainer = CreateTestContainer<IContainer>("DescendamtHierarchyChildContainer");
    container->Add(interface_pointer_cast<IObject>(childContainer));
    auto childOfChild = CreateTestType<IObject>("ChildOfChild");
    childContainer->Add(childOfChild);
    auto attachment = CreateTestAttachment<IAttachment>("ChildOfChildAttachment");
    auto attach = interface_cast<IAttach>(childOfChild);
    ASSERT_TRUE(attach);
    ASSERT_TRUE(attach->Attach(attachment));

    {
        EXPECT_TRUE(container2_1_->Add(interface_pointer_cast<IObject>(container)));
        ExpectedCallCount expected;
        expected.ExpectAddChildren();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(attach->Detach(attachment));
        ExpectedCallCount expected;
        expected.ExpectRemoveAttachments();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(attach->Attach(attachment));
        ExpectedCallCount expected;
        expected.ExpectAddAttachments();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(container->Remove(interface_pointer_cast<IObject>(childContainer)));
        ExpectedCallCount expected;
        expected.ExpectRemoveChildren();
        VerifyCalls(expected);
        EXPECT_TRUE(childContainer->Add(CreateTestType("AddedLater")));
        VerifyCalls(expected);
        EXPECT_TRUE(container->Add(interface_pointer_cast<IObject>(childContainer)));
        expected.ExpectAddChildren();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(container2_1_->Remove(interface_pointer_cast<IObject>(container)));
        ExpectedCallCount expected;
        expected.ExpectRemoveChildren();
        VerifyCalls(expected);
    }
    ResetCalls();
    {
        EXPECT_TRUE(attach->Detach(attachment));
        VerifyCalls({});
    }
}

/**
 * @tc.name: Properties
 * @tc.desc: Tests for Properties. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectHierarchyObserver, Properties, testing::ext::TestSize.Level1)
{
    auto obj = CreateTestType();
    auto att = interface_cast<IAttach>(obj);
    ASSERT_TRUE(att);

    ObjectHierarchyObserver obs(CreateInstance(ClassId::ObjectHierarchyObserver));
    HierarchyChangedInfo info;

    obs.SetTarget(interface_pointer_cast<IObject>(att->GetAttachmentContainer(true)),
        HierarchyChangeModeValue(HierarchyChangeMode::NOTIFY_CONTAINER) | HierarchyChangeMode::NOTIFY_OBJECT);
    obs.OnHierarchyChanged([&info](const HierarchyChangedInfo& i) { info = i; });

    obj->First()->SetValue(2);
    EXPECT_EQ(interface_pointer_cast<IProperty>(info.object), obj->First().GetProperty());
    EXPECT_EQ(info.change, HierarchyChangeType::CHANGED);
    EXPECT_EQ(info.objectType, HierarchyChangeObjectType::CHILD);
    EXPECT_EQ(info.parent.lock(), interface_pointer_cast<IObject>(att->GetAttachmentContainer(true)));

    obj->Second()->SetValue("hips");
    EXPECT_EQ(interface_pointer_cast<IProperty>(info.object), obj->Second().GetProperty());
}

/**
 * @tc.name: GetAll
 * @tc.desc: Tests for Get All. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectHierarchyObserver, GetAll, testing::ext::TestSize.Level1)
{
    ObjectHierarchyObserver obs(CreateInstance(ClassId::ObjectHierarchyObserver));
    EXPECT_THAT(obs.GetAllObserved(), testing::IsEmpty());

    auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));

    auto container = interface_pointer_cast<IContainer>(containerObject);
    container->Add(childObject);

    obs.SetTarget(containerObject);
    EXPECT_THAT(obs.GetAllObserved(), testing::UnorderedElementsAre(containerObject, childObject));
    EXPECT_THAT(obs.GetAllObserved<IContainer>(), testing::UnorderedElementsAre(container));

    container->Remove(childObject);
    EXPECT_THAT(obs.GetAllObserved(), testing::UnorderedElementsAre(containerObject));
}

/**
 * @tc.name: Remove
 * @tc.desc: Tests for Remove. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectHierarchyObserver, Remove, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs(CreateInstance(ClassId::ObjectHierarchyObserver));
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
        EXPECT_TRUE(container->Add(childContainerObject));
        EXPECT_TRUE(childContainer->Add(childObject));
        obs.SetTarget(containerObject);
        EXPECT_THAT(
            obs.GetAllObserved(), testing::UnorderedElementsAre(containerObject, childContainerObject, childObject));

        container->RemoveAll();
    }

    EXPECT_EQ(callCount, 1);
    EXPECT_THAT(removed, testing::ElementsAre(childContainerObject));
}

/**
 * @tc.name: DestroyEvents
 * @tc.desc: Tests for Destroy Events. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectHierarchyObserver, DestroyEvents, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs(CreateInstance(ClassId::ObjectHierarchyObserver));
    auto callCount = 0;
    auto removingCallCount = 0;

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
        obs.SetTarget(containerObject);
        EXPECT_THAT(obs.GetAllObserved(), testing::UnorderedElementsAre(containerObject, childContainerObject,
                                              childObject, childObject4, childObject2, childObject3));
        container->Remove(childObject3);
        EXPECT_THAT(obs.GetAllObserved(), testing::UnorderedElementsAre(containerObject, childContainerObject,
                                              childObject, childObject4, childObject2));
    }

    obs.OnHierarchyChanged([&](const HierarchyChangedInfo& info) {
        if (info.objectType == HierarchyChangeObjectType::CHILD) {
            if (info.change == HierarchyChangeType::REMOVED) {
                callCount++;
                removed.push_back(info.object);
            }
            if (info.change == HierarchyChangeType::REMOVING) {
                ++removingCallCount;
            }
        }
    });

    containerObject.reset();
    EXPECT_EQ(callCount, 3);
    EXPECT_EQ(removingCallCount, 3);
    EXPECT_THAT(removed, testing::ElementsAre(childContainerObject, childObject4, childObject2));
}

/**
 * @tc.name: DestroyEventsContent
 * @tc.desc: Tests for Destroy Events Content. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectHierarchyObserver, DestroyEventsContent, testing::ext::TestSize.Level1)
{
    BASE_NS::vector<IObject::Ptr> removed;
    ObjectHierarchyObserver obs(CreateInstance(ClassId::ObjectHierarchyObserver));
    auto callCount = 0;

    obs.OnHierarchyChanged([&removed, &callCount](const HierarchyChangedInfo& info) {
        if (info.objectType == HierarchyChangeObjectType::CONTENT) {
            if (info.change == HierarchyChangeType::REMOVED) {
                callCount++;
                removed.push_back(info.object);
            }
        }
    });

    auto contentObject = META_NS::GetObjectRegistry().Create(ClassId::ContentObject);
    auto childObject = interface_pointer_cast<IObject>(CreateTestType("TestType"));
    {
        auto content = interface_pointer_cast<IContent>(contentObject);
        EXPECT_TRUE(content->SetContent(childObject));
        obs.SetTarget(contentObject);
        EXPECT_THAT(obs.GetAllObserved(), testing::UnorderedElementsAre(contentObject, childObject));
    }

    contentObject.reset();
    EXPECT_EQ(callCount, 1);
    EXPECT_THAT(removed, testing::ElementsAre(childObject));
}

/**
 * @tc.name: DoNotSerialise
 * @tc.desc: Tests for Do Not Serialise. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_ObjectHierarchyObserver, DoNotSerialise, testing::ext::TestSize.Level1)
{
    TestSerialiser ser;
    {
        ObjectHierarchyObserver obs(CreateInstance(META_NS::ClassId::ObjectHierarchyObserver));
        auto containerObject = interface_pointer_cast<IObject>(CreateTestContainer("Container"));
        obs.SetTarget(containerObject);
        auto all = obs.GetAllObserved();
        EXPECT_THAT(obs.GetAllObserved(), testing::UnorderedElementsAre(containerObject));
        auto i = interface_cast<IAttach>(containerObject);
        ASSERT_TRUE(i);
        EXPECT_EQ(i->GetAttachments<IObjectHierarchyObserver>().size(), 1);
        ser.Export(containerObject);
    }

    auto obj = ser.Import<IAttach>();
    ASSERT_TRUE(obj);
    EXPECT_EQ(obj->GetAttachments<IObjectHierarchyObserver>().size(), 0);
}

} // namespace UTest
META_END_NAMESPACE()
