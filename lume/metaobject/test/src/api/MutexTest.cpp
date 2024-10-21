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
#include <meta/api/threading/mutex.h>

#include "src/test_runner.h"
#include "src/testing_objects.h"

META_BEGIN_NAMESPACE()

using namespace testing::ext;
using CORE_NS::Mutex;
using CORE_NS::UniqueLock;

class MutexTest : public testing::Test {
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
 * @tc.name: MutexTest
 * @tc.desc: test UniqueLock function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(MutexTest, UniqueLock, TestSize.Level1)
{
    Mutex m;
    {
        UniqueLock l { m };
        EXPECT_TRUE(l);
        l.Unlock();
        EXPECT_FALSE(l);
    }

    {
        UniqueLock l { m };
        EXPECT_TRUE(l);
        l.Unlock();
        EXPECT_FALSE(l);
        l.Lock();
        EXPECT_TRUE(l);
    }

    {
        UniqueLock l { m };
        EXPECT_TRUE(l);
        UniqueLock l2 = BASE_NS::move(l);
        EXPECT_FALSE(l);
        EXPECT_TRUE(l2);
        l = std::move(l2);
        EXPECT_FALSE(l2);
        EXPECT_TRUE(l);
    }
}
META_END_NAMESPACE()