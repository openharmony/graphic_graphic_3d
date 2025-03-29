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

#include <TestRunner.h>
#include <src/testing_objects.h>

#include <gmock/gmock-matchers.h>
#include <gtest/gtest.h>

#include <meta/api/compatible_value_util.h>
#include <meta/interface/detail/any.h>

using namespace testing;
using namespace testing::ext;

META_BEGIN_NAMESPACE()

using types = TypeList<bool, char, int16_t, int32_t>;

class CompatibleValueUtilTest : public testing::Test {
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
 * @tc.name: SetGet
 * @tc.desc: test Set Get
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(CompatibleValueUtilTest, SetGet, TestSize.Level1)
{
    Any<int16_t> any { 2 };

    int64_t v = 0;
    EXPECT_TRUE(GetCompatibleValue(any, v, types {}));
    EXPECT_EQ(v, 2);

    size_t s = 3;
    EXPECT_TRUE(SetCompatibleValue(s, any, types {}));
    EXPECT_EQ(any.InternalGetValue(), 3);
}

META_END_NAMESPACE()
