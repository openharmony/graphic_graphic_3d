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

#include <meta/api/threading/mutex.h>

#include "helpers/testing_objects.h"
#include "test_framework.h"

using namespace testing;

META_BEGIN_NAMESPACE()

namespace UTest {

using CORE_NS::Mutex;
using CORE_NS::UniqueLock;

/**
 * @tc.name: UniqueLock
 * @tc.desc: Tests for Unique Lock. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_MutexTest, UniqueLock, testing::ext::TestSize.Level1)
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

} // namespace UTest

META_END_NAMESPACE()
