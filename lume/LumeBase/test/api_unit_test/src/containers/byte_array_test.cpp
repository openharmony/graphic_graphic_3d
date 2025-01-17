/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <base/containers/byte_array.h>

using namespace BASE_NS;
using namespace testing::ext;

class ByteArrayTest : public testing::Test {
public:
    static void SetUpTestSuite() {}
    static void TearDownTestSuite() {}
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(ByteArrayTest, constuctor, TestSize.Level1)
{
    ByteArray(128U);
}

HWTEST_F(ByteArrayTest, getData, TestSize.Level1)
{
    {
        constexpr auto size = 128U;
        auto array = ByteArray(size);
        auto data = array.GetData();
        EXPECT_EQ(data.size(), size);
    }
    {
        constexpr const auto size = 128U;
        const auto array = ByteArray(size);
        const auto data = array.GetData();
        EXPECT_EQ(data.size(), size);
    }
}
