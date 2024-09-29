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

#include <meta/api/container/container_observer.h>
#include <meta/api/make_callback.h>
#include <meta/base/ref_uri.h>

#include "src/test_runner.h"
#include "src/test_utils.h"
#include "src/testing_objects.h"
#include "src/util.h"

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

class IntfContainerObserverTest : public testing::Test {
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
        container1_1_->Add(interface_pointer_cast<IObject>(container2_1_));

        observer_.Container(container_)
            .OnDescendantAdded([this](const ChildChangedInfo& info) { OnAdded(info); })
            .OnDescendantRemoved([this](const ChildChangedInfo& info) { OnRemoved(info); })
            .OnDescendantMoved([this](const ChildMovedInfo& info) { OnMoved(info); });
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

    IContainer::Ptr container_;
    IContainer::Ptr container1_1_;
    IContainer::Ptr container2_1_;
    META_NS::ContainerObserver observer_;
};

/**
 * @tc.name: IntfContainerObserverTest
 * @tc.desc: test AddDirectChild function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContainerObserverTest, AddDirectChild, TestSize.Level1)
{
    auto child = CreateTestType<IObject>("DirectChild");
    ASSERT_TRUE(container_->Add(child));

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container_);
}

/**
 * @tc.name: IntfContainerObserverTest
 * @tc.desc: test AddDescendantChild function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContainerObserverTest, AddDescendantChild, TestSize.Level1)
{
    auto child = CreateTestType<IObject>("Descendant");
    ASSERT_TRUE(container2_1_->Add(child));

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container2_1_);
}

/**
 * @tc.name: IntfContainerObserverTest
 * @tc.desc: test ChangedAfterChild function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContainerObserverTest, ChangeAfterChild, TestSize.Level1)
{
    auto container = CreateTestContainer<IContainer>("DescendantContainer");
    ASSERT_TRUE(container2_1_->Add(container));

    ASSERT_THAT(addedCalls_, SizeIs(1));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = addedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container));
    EXPECT_EQ(info.parent.lock(), container2_1_);

    auto child = CreateTestType<IObject>("DescendantChild");
    ASSERT_TRUE(container->Add(child));

    ASSERT_THAT(addedCalls_, SizeIs(2));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    info = addedCalls_[1];
    EXPECT_EQ(info.object, child);
    EXPECT_EQ(info.parent.lock(), container);

    EXPECT_TRUE(container1_1_->Remove(container2_1_));

    EXPECT_TRUE(container->Remove(child));
    ASSERT_THAT(addedCalls_, SizeIs(2));
    ASSERT_THAT(removedCalls_, SizeIs(1));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    info = removedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container2_1_));
    EXPECT_EQ(info.parent.lock(), container1_1_);
}

/**
 * @tc.name: IntfContainerObserverTest
 * @tc.desc: test RemoveDescendant function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(IntfContainerObserverTest, RemoveDescendant, TestSize.Level1)
{
    container_->Remove(container1_1_);
    ASSERT_THAT(addedCalls_, SizeIs(0));
    ASSERT_THAT(removedCalls_, SizeIs(1));
    ASSERT_THAT(movedCalls_, SizeIs(0));

    auto info = removedCalls_[0];
    EXPECT_EQ(info.object, interface_pointer_cast<IObject>(container1_1_));
    EXPECT_EQ(info.parent.lock(), container_);
    removedCalls_.clear();

    auto child = CreateTestType<IObject>("Descendant");
    container1_1_->Add(child);

    ASSERT_THAT(addedCalls_, SizeIs(0));
    ASSERT_THAT(removedCalls_, SizeIs(0));
    ASSERT_THAT(movedCalls_, SizeIs(0));
}

META_END_NAMESPACE()