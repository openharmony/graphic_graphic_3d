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
#include <meta/base/ref_uri.h>

#include "helpers/test_utils.h"
#include "helpers/testing_objects.h"
#include "helpers/util.h"

META_BEGIN_NAMESPACE()

namespace UTest {

class API_ContainerObserverTest : public ::testing::Test {
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
        container1_1_->Add(interface_pointer_cast<IObject>(container2_1_));

        observer_.Container(container_).OnDescendantChanged([this](const ChildChangedInfo& info) {
            if (info.type == ContainerChangeType::ADDED) {
                OnAdded(info);
            } else if (info.type == ContainerChangeType::REMOVED) {
                OnRemoved(info);
            } else if (info.type == ContainerChangeType::MOVED) {
                OnMoved(info);
            }
        });
    }
    void TearDown() override
    {
        container_.reset();
        observer_.Release();
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

    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
    META_NS::ContainerObserver observer_ { CreateInstance(ClassId::ContainerObserver) };
};

/**
 * @tc.name: AddDirectChild
 * @tc.desc: Tests for Add Direct Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerObserverTest, AddDirectChild, testing::ext::TestSize.Level1)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container_->Add(child));

    ASSERT_THAT(addedCalls_, testing::SizeIs(1));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container_);
}

/**
 * @tc.name: AddDescendantChild
 * @tc.desc: Tests for Add Descendant Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerObserverTest, AddDescendantChild, testing::ext::TestSize.Level1)
{
    auto child = CreateTestType<IObject>("Descendant");
    ASSERT_TRUE(container2_1_->Add(child));

    ASSERT_THAT(addedCalls_, testing::SizeIs(1));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container2_1_);
}

/**
 * @tc.name: ChangeAfterChild
 * @tc.desc: Tests for Change After Child. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerObserverTest, ChangeAfterChild, testing::ext::TestSize.Level1)
{
    auto container = CreateTestContainer<IContainer>("DescendantContainer");
    ASSERT_TRUE(container2_1_->Add(container));

    ASSERT_THAT(addedCalls_, testing::SizeIs(1));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container));
    EXPECT_EQ(info.parent.lock(), container2_1_);

    // Add a child to the newly-added descendant
    auto child = CreateTestType<IObject>("DescendantChild");
    ASSERT_TRUE(container->Add(child));
    ASSERT_THAT(addedCalls_, testing::SizeIs(2));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));

    info = addedCalls_[1];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container);

    // Remove an ancestor of our manually added container hierarchy -> notification
    EXPECT_TRUE(container1_1_->Remove(container2_1_));
    // Remove a descendant of the ancestor we removed -> no notification
    EXPECT_TRUE(container->Remove(child));
    ASSERT_THAT(addedCalls_, testing::SizeIs(2));
    ASSERT_THAT(removedCalls_, testing::SizeIs(1));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));

    info = removedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container2_1_));
    EXPECT_EQ(info.parent.lock(), container1_1_);
}

/**
 * @tc.name: RemoveDescendant
 * @tc.desc: Tests for Remove Descendant. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_ContainerObserverTest, RemoveDescendant, testing::ext::TestSize.Level1)
{
    container_->Remove(container1_1_);
    ASSERT_THAT(addedCalls_, testing::SizeIs(0));
    ASSERT_THAT(removedCalls_, testing::SizeIs(1));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));

    auto info = removedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container1_1_));
    EXPECT_EQ(info.parent.lock(), container_);
    removedCalls_.clear();

    // Add a new child to container2_1_, we should not get notification
    // as container2_1_ is not part of container_'s child hierarchy anymore
    auto child = CreateTestType<IObject>("Descendant");
    container1_1_->Add(child);

    ASSERT_THAT(addedCalls_, testing::SizeIs(0));
    ASSERT_THAT(removedCalls_, testing::SizeIs(0));
    ASSERT_THAT(movedCalls_, testing::SizeIs(0));
}

} // namespace UTest
META_END_NAMESPACE()
