/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "TestRunner.h"
#include <src/testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/number.h>
#include <meta/interface/detail/any.h>

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

enum MyEnum : int16_t { ONE = 1, TWO, THREE };

class NumberTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
    void SetUp() {}
    void TearDown() {}
};

/**
 * @tc.name: Values
 * @tc.desc: test Values
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(NumberTest, Values, TestSize.Level1)
{
    Number n = 1;
    EXPECT_EQ(n.Get<uint32_t>(), 1);
    EXPECT_EQ(n.Get<int32_t>(), 1);
    EXPECT_EQ(n.Get<float>(), 1);
    EXPECT_EQ(n.Get<MyEnum>(), MyEnum::ONE);

    n = Any<uint64_t>(10);
    EXPECT_EQ(n.Get<uint32_t>(), 10);

    n = 1.2f;
    EXPECT_EQ(n.Get<uint32_t>(), 1);
    EXPECT_EQ(n.Get<int32_t>(), 1);
    EXPECT_NEAR(n.Get<float>(), 1.2, 0.0001);

    Number en = MyEnum::TWO;
    EXPECT_EQ(en.Get<uint32_t>(), 2);
    EXPECT_EQ(en.Get<MyEnum>(), MyEnum::TWO);
}

META_END_NAMESPACE()
