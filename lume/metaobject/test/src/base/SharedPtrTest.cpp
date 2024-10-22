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

#include <gtest/gtest.h>

#include <meta/base/shared_ptr.h>

#include "src/test_runner.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class SharedPtrTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        ResetTest();
    }
    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test SharedPtrCanBeConvertedToSharedPtrWithConstType function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, SharedPtrCanBeConvertedToSharedPtrWithConstType, TestSize.Level1)
{
    constexpr auto initialValue = 120;

    BASE_NS::shared_ptr<const int> cptr = ptr;

    EXPECT_EQ(*cptr, initialValue);
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test SharedPtrCanBeReturnedToSharedPtrWithConstType function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, SharedPtrCanBeReturnedAsSharedPtrWithConstType, TestSize.Level1)
{
    constexpr auto initialValue = 120;

    auto functionReturningSharedConstPtr = [ptr]() -> BASE_NS::shared_ptr<const int> { return ptr; };

    auto cptr = functionReturningSharedConstPtr();

    EXPECT_EQ(*cptr, initialValue);
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test DefaultConstructedSharedPtrIsEmpty function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, DefaultConstructedSharedPtrIsEmpty, TestSize.Level1)
{
    BASE_NS::shared_ptr<int> ptr;

    const auto rawPtr = ptr.get();
    const bool boolOperatorResult = ptr.operator bool();

    EXPECT_EQ(rawPtr, nullptr);
    EXPECT_EQ(boolOperatorResult, false);
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test CopiedSharedPtrExtendObjectLifetime function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, CopiedSharedPtrExtendObjectLifetime, TestSize.Level1)
{
    constexpr auto initialValue = 120;
    BASE_NS::shared_ptr<const int> cptr;

    {
        cptr = ptr;
    }

    EXPECT_EQ(*cptr, initialValue);
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test CopyConstructedSharedPtrExtendObjectLifetime function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, CopyConstructedSharedPtrExtendObjectLifetime, TestSize.Level1)
{
    constexpr auto initialValue = 120;
    BASE_NS::shared_ptr<int> ptr = CreateShared<int>(initialValue);

    auto copy = BASE_NS::shared_ptr<int>(ptr);
    ptr.reset();

    EXPECT_EQ(*copy, initialValue);
}

struct CountType {
    int count { 1 };
};

void CountDeleter(void* p)
{
    --static_cast<CountType*>(p)->count;
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test MoveMemoryLeakAndAssert function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, MoveMemoryLeakAndAssert, TestSize.Level1)
{
    CountType test;
    {
        BASE_NS::shared_ptr<CountType> p(&test, CountDeleter);
        auto copy = p;
        p = BASE_NS::move(copy);
    }

    EXPECT_EQ(test.count, 0);
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test Expired function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, Expired, TestSize.Level1)
{
    BASE_NS::weak_ptr<int> w = ptr;
    EXPECT_FALSE(w.expired());
    ptr.reset();
    EXPECT_TRUE(w.expired());
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test Comparison function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, Comparison, TestSize.Level1)
{
    auto p2 = p1;
    auto p3 = BASE_NS::shared_ptr<int>(p1.get(), [](void*) {});

    EXPECT_EQ(p1, p2);
    EXPECT_EQ(p1, p3);
    EXPECT_EQ(p2, p3);
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test ExplicitDeleterBasicType function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, ExplicitDeleterBasicType, TestSize.Level1)
{
    int count = 0;
    {
        auto p = BASE_NS::shared_ptr<int>(&count, [&](int *) { ++count; });
    }
    EXPECT_EQ(count, 1);
}

namespace {
struct ITestInterface : CORE_NS::IInterface {
    const IInterface* GetInterface(const BASE_NS::Uid& uid) const override
    {
        return this;
    }

    IInterface* GetInterface(const BASE_NS::Uid& uid) override
    {
        return this;
    }

    void Ref() override
    {
        ++count;
    }

    void Unref() override
    {
        --count;
    }
    int count { 0 };
};
} // namespace

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test ExplicitDeleterInterface function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, ExplicitDeleterInterface, TestSize.Level1)
{
    {
        ITestInterface test;
        {
            auto p = BASE_NS::shared_ptr<ITestInterface>(&test);
            EXPECT_EQ(test.count, 1);
        }
        EXPECT_EQ(test.count, 0);
    }
    {
        ITestInterface test;
        {
            auto p = BASE_NS::shared_ptr<CORE_NS::IInterface>(&test);
            EXPECT_EQ(test.count, 1);
        }
        EXPECT_EQ(test.count, 0);
    }
    {
        int count = 0;
        ITestInterface test;
        {
            auto p = BASE_NS::shared_ptr<ITestInterface>(&test, [&count](ITestInterface*) { ++count; });
            EXPECT_EQ(test.count, 0);
        }
        EXPECT_EQ(count, 1);
        EXPECT_EQ(test.count, 0);
    }
}

/**
 * @tc.name: SharedPtrTest
 * @tc.desc: test UniquePtrConversion function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(SharedPtrTest, UniquePtrConversion, TestSize.Level1)
{
    {
        BASE_NS::shared_ptr<int> shared(BASE_NS::move(unique));
        ASSERT_TRUE(!unique);
        ASSERT_TRUE(shared);
        EXPECT_EQ(*shared, 2);
    }
    {
        ITestInterface object;
        object.Ref();
        EXPECT_EQ(object.count, 1);
        BASE_NS::unique_ptr<ITestInterface, decltype(deleter)> unique(&object, deleter);
        BASE_NS::shared_ptr<CORE_NS::IInterface> shared(BASE_NS::move(unique));
        ASSERT_TRUE(!unique);
        ASSERT_TRUE(shared);
        EXPECT_EQ(object.count, 1);
        shared.reset();
        EXPECT_EQ(object.count, 0);
    }
    {
        int count = 0;
        {
            int value = 2;
            BASE_NS::unique_ptr<int, decltype(deleter)> unique(&value, deleter);
            BASE_NS::shared_ptr<int> shared(BASE_NS::move(unique));
            ASSERT_TRUE(!unique);
            ASSERT_TRUE(shared);
            EXPECT_EQ(*shared, 2);
            EXPECT_EQ(count, 0);
        }
        EXPECT_EQ(count, 1);
    }
    {
        BASE_NS::shared_ptr<int> shared;
        shared = BASE_NS::move(unique);
        ASSERT_TRUE(!unique);
        ASSERT_TRUE(shared);
        EXPECT_EQ(*shared, 2);
    }
    {
        ITestInterface object;
        object.Ref();
        EXPECT_EQ(object.count, 1);
        BASE_NS::unique_ptr<ITestInterface, decltype(deleter)> unique(&object, deleter);
        BASE_NS::shared_ptr<CORE_NS::IInterface> shared;
        shared = BASE_NS::move(unique);
        ASSERT_TRUE(!unique);
        ASSERT_TRUE(shared);
        EXPECT_EQ(object.count, 1);
        shared.reset();
        EXPECT_EQ(object.count, 0);
    }
}

META_END_NAMESPACE()