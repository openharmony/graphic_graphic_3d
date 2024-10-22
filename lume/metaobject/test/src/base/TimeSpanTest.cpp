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

#include <meta/base/time_span.h>

#include "src/test_runner.h"
#include "src/test_utils.h"

using namespace testing::ext;

META_BEGIN_NAMESPACE()

class TimeSpanTest : public testing::Test {
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

using namespace META_NS::TimeSpanLiterals;

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test InitMethods function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, InitMethods, TestSize.Level1)
{
    constexpr auto v1 = TimeSpan::Milliseconds(2000);
    constexpr auto v2 = TimeSpan::Microseconds(2000000);
    constexpr auto v3 = TimeSpan::Seconds(2);
    constexpr TimeSpan v4 = 2_s;
    constexpr auto v5 = 2000_ms;
    constexpr auto v6 = 2000000_us;

    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v2, v3);
    EXPECT_EQ(v4, v5);
    EXPECT_EQ(v5, v6);
    EXPECT_EQ(v6, v1);
}

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test Addition function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, Addition, TestSize.Level1)
{
    constexpr auto v1 = TimeSpan::Milliseconds(400);
    constexpr auto v2 = TimeSpan::Microseconds(2000);

    constexpr auto v3 = v1 + v2;

    EXPECT_EQ(TimeSpan::Milliseconds(402), v3);

    EXPECT_EQ(TimeSpan::Milliseconds(400), v1);
    EXPECT_EQ(TimeSpan::Microseconds(2000), v2);
}

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test Subtraction function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, Subtraction, TestSize.Level1)
{
    constexpr auto v1 = TimeSpan::Milliseconds(400);
    constexpr auto v2 = TimeSpan::Microseconds(2000);

    constexpr auto v3 = v1 - v2;

    EXPECT_EQ(TimeSpan::Milliseconds(398), v3);

    EXPECT_EQ(TimeSpan::Milliseconds(400), v1);
    EXPECT_EQ(TimeSpan::Microseconds(2000), v2);
}

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test Negation function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, Negation, TestSize.Level1)
{
    constexpr auto v1 = -TimeSpan::Milliseconds(200);
    constexpr auto v2 = -200_ms;

    EXPECT_EQ(v1, v2);
    EXPECT_EQ(v1, TimeSpan::Milliseconds(-200));
    EXPECT_EQ(v2, TimeSpan::Milliseconds(-200));
}

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test Infinities function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, Infinities, TestSize.Level1)
{
    EXPECT_EQ(TimeSpan::Infinite(), TimeSpan::Infinite() + TimeSpan::Infinite());
    EXPECT_EQ(-TimeSpan::Infinite(), -TimeSpan::Infinite() - TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Infinite(), 3 * TimeSpan::Infinite());
    EXPECT_EQ(-TimeSpan::Infinite(), -1 * TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Infinite(), -1 * -TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Zero(), 0 * TimeSpan::Infinite());
    EXPECT_EQ(TimeSpan::Zero(), 0 * -TimeSpan::Infinite());

    EXPECT_EQ(TimeSpan::Infinite(), TimeSpan::Infinite() + TimeSpan::Microseconds(10));
    EXPECT_EQ(TimeSpan::Infinite(), TimeSpan::Infinite() - TimeSpan::Microseconds(10));
    EXPECT_EQ(-TimeSpan::Infinite(), -TimeSpan::Infinite() + TimeSpan::Microseconds(10));
    EXPECT_EQ(-TimeSpan::Infinite(), -TimeSpan::Infinite() - TimeSpan::Microseconds(10));

    auto v1 = TimeSpan::Infinite();
    v1 += TimeSpan::Milliseconds(10);

    EXPECT_EQ(TimeSpan::Infinite(), v1);

    auto v2 = TimeSpan::Infinite();
    v2 -= TimeSpan::Milliseconds(10);

    EXPECT_EQ(TimeSpan::Infinite(), v2);
}

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test TimeIsRoundedToMicroseconds function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, TimeIsRoundedToMicroseconds, TestSize.Level1)
{
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Seconds(1e-6));
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Seconds(0.4e-6));
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Seconds(0.5e-6));

    auto timeSpan = TimeSpan::Microseconds(1);
    timeSpan.SetSeconds(0.4e-6);
    EXPECT_EQ(TimeSpan::Microseconds(0), timeSpan);

    timeSpan.SetSeconds(0.5e-6);
    EXPECT_EQ(TimeSpan::Microseconds(1), timeSpan);
}

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test MultiplicationResultIsRoundedToMicroseconds function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, MultiplicationResultIsRoundedToMicroseconds, TestSize.Level1)
{
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Microseconds(1) * 0.5f);
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Microseconds(1) * 0.1f);
}

/**
 * @tc.name: TimeSpanTest
 * @tc.desc: test MultiplicationResultIsRoundedToMicroseconds function
 * @tc.type: FUNC
 * @tc.require: I7DMS1
 */
HWTEST_F(TimeSpanTest, DivisionResultIsRoundedToMicroseconds, TestSize.Level1)
{
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Microseconds(1) / 2);
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Microseconds(1) / 10);
    EXPECT_EQ(TimeSpan::Microseconds(1), TimeSpan::Microseconds(1) / 2.f);
    EXPECT_EQ(TimeSpan::Microseconds(0), TimeSpan::Microseconds(1) / 10.f);
}

META_END_NAMESPACE()