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

#include <atomic>

#include <meta/api/timer.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "helpers/testing_macros.h"

META_BEGIN_NAMESPACE()

namespace UTest {

class API_TimerTests : public ::testing::Test {
    void SetUp() override
    {
        queue_ = GetObjectRegistry().Create<IThreadedTaskQueue>(ClassId::ThreadedTaskQueue);
        queueId_ = interface_cast<IObjectInstance>(queue_)->GetInstanceId();
        GetTaskQueueRegistry().RegisterTaskQueue(queue_, queueId_.ToUid());
    }

    void TearDown() override
    {
        GetTaskQueueRegistry().UnregisterTaskQueue(queueId_.ToUid());
    }

protected:
    ITaskQueue::Ptr queue_;
    InstanceId queueId_;
};

/**
 * @tc.name: Ctor
 * @tc.desc: Tests for Ctor. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TimerTests, Ctor, testing::ext::TestSize.Level1)
{
    {
        std::atomic<int> count = 0;
        Timer t(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid());
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
    {
        std::atomic<int> count = 0;
        Timer t(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queue_);
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
}

/**
 * @tc.name: Moving
 * @tc.desc: Tests for Moving. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TimerTests, Moving, testing::ext::TestSize.Level1)
{
    Timer t(
        TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING, queueId_.ToUid());
    EXPECT_TRUE(t.IsRunning());
    Timer t2 = BASE_NS::move(t);
    EXPECT_FALSE(t.IsRunning());
    EXPECT_TRUE(t2.IsRunning());
}

/**
 * @tc.name: Start
 * @tc.desc: Tests for Start. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TimerTests, Start, testing::ext::TestSize.Level1)
{
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid()));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queue_));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(350, count, 3);
    }
}

/**
 * @tc.name: Stop
 * @tc.desc: Tests for Stop. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TimerTests, Stop, testing::ext::TestSize.Level1)
{
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid()));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(150, count, 1);
        t.Stop();
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
        EXPECT_FALSE(t.IsRunning());
    }
    {
        std::atomic<int> count = 0;
        {
            Timer t;
            EXPECT_TRUE(t.Start(
                TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::RECURRING, queueId_.ToUid()));
            EXPECT_TRUE(t.IsRunning());
            EXPECT_EQ_TIMED(150, count, 1);
        }
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
    }
}

/**
 * @tc.name: SingleShot
 * @tc.desc: Tests for Single Shot. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TimerTests, SingleShot, testing::ext::TestSize.Level1)
{
    {
        std::atomic<int> count = 0;
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::SINGLE_SHOT, queueId_.ToUid()));
        EXPECT_TRUE(t.IsRunning());
        EXPECT_EQ_TIMED(150, count, 1);
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
        EXPECT_FALSE(t.IsRunning());
    }
    {
        std::atomic<int> count = 0;
        SingleShotTimer(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, queueId_.ToUid());
        EXPECT_EQ_TIMED(150, count, 1);
        META_WAIT_TIMED(150);
        EXPECT_EQ(count, 1);
    }
}

/**
 * @tc.name: Detach
 * @tc.desc: Tests for Detach. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TimerTests, Detach, testing::ext::TestSize.Level1)
{
    std::atomic<int> count = 0;
    {
        Timer t;
        EXPECT_TRUE(t.Start(
            TimeSpan::Milliseconds(100), [&count] { ++count; }, Timer::SINGLE_SHOT, queueId_.ToUid()));
        t.Detach();
        EXPECT_FALSE(t.IsRunning());
    }
    EXPECT_EQ_TIMED(150, count, 1);
    META_WAIT_TIMED(150);
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: BadQueue
 * @tc.desc: Tests for Bad Queue. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_F(API_TimerTests, BadQueue, testing::ext::TestSize.Level1)
{
    {
        Timer t(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING,
            BASE_NS::Uid { "b63ceb45-bdb3-4f23-bd95-bb640eba9144" });
        EXPECT_TRUE(!t.IsRunning());
    }
    {
        Timer t;
        EXPECT_FALSE(t.Start(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING,
            BASE_NS::Uid { "b63ceb45-bdb3-4f23-bd95-bb640eba9144" }));
        EXPECT_TRUE(!t.IsRunning());
    }
    {
        Timer t(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING, nullptr);
        EXPECT_TRUE(!t.IsRunning());
    }
    {
        Timer t;
        EXPECT_FALSE(t.Start(
            TimeSpan::Milliseconds(20), [] {}, Timer::RECURRING, nullptr));
        EXPECT_TRUE(!t.IsRunning());
    }
}

/**
 * @tc.name: Debug
 * @tc.desc: Tests for Debug. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
/*
UNIT_TEST_F(API_TimerTests, Debug, testing::ext::TestSize.Level1)
{
    Timer t;
    auto start = std::chrono::steady_clock::now();
    EXPECT_TRUE(t.Start(
        TimeSpan::Milliseconds(20),
        [start] {
            auto diff = std::chrono::steady_clock::now() - start;
            CORE_LOG_W("hit %lld", std::chrono::duration_cast<std::chrono::milliseconds>(diff).count());
        },
        Timer::RECURRING, queueId_));

    META_WAIT_TIMED(200);
}
*/
} // namespace UTest

META_END_NAMESPACE()
