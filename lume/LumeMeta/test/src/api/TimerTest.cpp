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
#include <thread>

#include "TestRunner.h"

#include <meta/api/timer.h>
#include <meta/interface/intf_object_registry.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>

#include "src/test_macros.h"
META_BEGIN_NAMESPACE()

using namespace testing;
using namespace testing::ext;

class TimerTest : public ::testing::Test {
public:
    static void SetUpTestSuite()
    {
        SetTest();
    }
    static void TearDownTestSuite()
    {
        TearDownTest();
    }
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
 * @tc.desc: test Ctor
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(TimerTest, Ctor, TestSize.Level1)
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
 * @tc.desc: test Moving
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(TimerTest, Moving, TestSize.Level1)
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
 * @tc.desc: test Start
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(TimerTest, Start, TestSize.Level1)
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
 * @tc.desc: test Stop
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(TimerTest, Stop, TestSize.Level1)
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
 * @tc.desc: test SingleShot
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(TimerTest, SingleShot, TestSize.Level1)
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
 * @tc.desc: test Detach
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(TimerTest, Detach, TestSize.Level1)
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
 * @tc.desc: test BadQueue
 * @tc.type:FUNC
 * @tc.require:
 */
HWTEST_F(TimerTest, BadQueue, TestSize.Level1)
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

META_END_NAMESPACE()
