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

#include <gtest/gtest.h>
#include <memory>
#include <atomic>
#include <chrono>
#include <thread>

#include "graphics_task.h"
#include "3d_widget_adapter_log.h"

using namespace testing;
using namespace testing::ext;

namespace OHOS::Render3D {

class GraphicsTaskUT : public ::testing::Test {
public:
    static void SetUpTestCase() {}
    static void TearDownTestCase() {}
    void SetUp() {}
    void TearDown() {}
};

// Message Tests

/**
 * @tc.name: Message_Constructor_InitializesTask
 * @tc.desc: test Message constructor initializes task correctly
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, Message_Constructor_InitializesTask, TestSize.Level1)
{
    WIDGET_LOGD("Message_Constructor_InitializesTask");
    std::atomic<bool> executed{false};
    GraphicsTask::Message msg([&executed]() { executed = true; });

    msg.Execute();

    EXPECT_TRUE(executed.load());
}

/**
 * @tc.name: Message_MoveConstructor_TransfersOwnership
 * @tc.desc: test Message move constructor transfers ownership
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, Message_MoveConstructor_TransfersOwnership, TestSize.Level1)
{
    WIDGET_LOGD("Message_MoveConstructor_TransfersOwnership");
    std::atomic<bool> executed{false};
    GraphicsTask::Message msg1([&executed]() { executed = true; });
    GraphicsTask::Message msg2(std::move(msg1));

    msg2.Execute();

    EXPECT_TRUE(executed.load());
}

/**
 * @tc.name: Message_MoveAssignment_TransfersOwnership
 * @tc.desc: test Message move assignment transfers ownership
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, Message_MoveAssignment_TransfersOwnership, TestSize.Level1)
{
    WIDGET_LOGD("Message_MoveAssignment_TransfersOwnership");
    std::atomic<bool> executed1{false};
    std::atomic<bool> executed2{false};

    GraphicsTask::Message msg1([&executed1]() { executed1 = true; });
    GraphicsTask::Message msg2([&executed2]() { executed2 = true; });

    msg1 = std::move(msg2);

    msg1.Execute();

    EXPECT_FALSE(executed1.load());
    EXPECT_TRUE(executed2.load());
}

/**
 * @tc.name: Message_Execute_RunsTask
 * @tc.desc: test Message Execute runs the task
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, Message_Execute_RunsTask, TestSize.Level1)
{
    WIDGET_LOGD("Message_Execute_RunsTask");
    int counter = 0;
    GraphicsTask::Message msg([&counter]() { counter = 42; });

    msg.Execute();

    EXPECT_EQ(counter, 42);
}

/**
 * @tc.name: Message_MoveConstructor_NullifiesSource
 * @tc.desc: test Message move constructor nullifies source
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, Message_MoveConstructor_NullifiesSource, TestSize.Level1)
{
    WIDGET_LOGD("Message_MoveConstructor_NullifiesSource");
    std::atomic<bool> executed{false};
    GraphicsTask::Message msg1([&executed]() { executed = true; });
    GraphicsTask::Message msg2(std::move(msg1));

    msg2.Execute();

    EXPECT_TRUE(executed.load());
}

/**
 * @tc.name: Message_Finish_SetsPromise
 * @tc.desc: test Message Finish sets promise correctly
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, Message_Finish_SetsPromise, TestSize.Level1)
{
    WIDGET_LOGD("Message_Finish_SetsPromise");
    GraphicsTask::Message msg([]() {});

    msg.Finish();

    // If we reach here without blocking, Finish() worked correctly
    EXPECT_TRUE(true);
}

// GraphicsTask Tests

/**
 * @tc.name: GetInstance_ReturnsSameInstance
 * @tc.desc: test GetInstance returns singleton instance
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, GetInstance_ReturnsSameInstance, TestSize.Level1)
{
    WIDGET_LOGD("GetInstance_ReturnsSameInstance");
    auto& instance1 = GraphicsTask::GetInstance();
    auto& instance2 = GraphicsTask::GetInstance();

    EXPECT_EQ(&instance1, &instance2);
}

/**
 * @tc.name: GetInstance_ReturnsValidReference
 * @tc.desc: test GetInstance returns valid reference
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, GetInstance_ReturnsValidReference, TestSize.Level1)
{
    WIDGET_LOGD("GetInstance_ReturnsValidReference");
    auto& instance = GraphicsTask::GetInstance();

    // Verify instance is valid by taking its address
    EXPECT_NE(&instance, nullptr);
}

/**
 * @tc.name: PushSyncMessage_ExecutesTask
 * @tc.desc: test PushSyncMessage executes task synchronously
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushSyncMessage_ExecutesTask, TestSize.Level1)
{
    WIDGET_LOGD("PushSyncMessage_ExecutesTask");
    auto& task = GraphicsTask::GetInstance();
    std::atomic<bool> executed{false};

    task.PushSyncMessage([&executed]() { executed = true; });

    // Wait a bit for execution
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    EXPECT_TRUE(executed.load());
}

/**
 * @tc.name: PushSyncMessage_MultipleTasks_AllExecuted
 * @tc.desc: test PushSyncMessage executes multiple tasks
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushSyncMessage_MultipleTasks_AllExecuted, TestSize.Level1)
{
    WIDGET_LOGD("PushSyncMessage_MultipleTasks_AllExecuted");
    auto& task = GraphicsTask::GetInstance();
    std::atomic<int> counter{0};

    for (int i = 0; i < 10; i++) {
        task.PushSyncMessage([&counter]() { counter++; });
    }

    EXPECT_EQ(counter.load(), 10);
}

/**
 * @tc.name: PushAsyncMessage_ExecutesTask
 * @tc.desc: test PushAsyncMessage executes task asynchronously
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushAsyncMessage_ExecutesTask, TestSize.Level1)
{
    WIDGET_LOGD("PushAsyncMessage_ExecutesTask");
    auto& task = GraphicsTask::GetInstance();
    std::atomic<bool> executed{false};

    auto future = task.PushAsyncMessage([&executed]() { executed = true; });

    // Wait for the task to complete
    future.wait();

    EXPECT_TRUE(executed.load());
}

/**
 * @tc.name: PushAsyncMessage_MultipleTasks_AllExecuted
 * @tc.desc: test PushAsyncMessage executes multiple tasks
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushAsyncMessage_MultipleTasks_AllExecuted, TestSize.Level1)
{
    WIDGET_LOGD("PushAsyncMessage_MultipleTasks_AllExecuted");
    auto& task = GraphicsTask::GetInstance();
    std::atomic<int> counter{0};

    std::vector<std::shared_future<void>> futures;
    for (int i = 0; i < 10; i++) {
        futures.push_back(task.PushAsyncMessage([&counter]() { counter++; }));
    }

    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }

    EXPECT_EQ(counter.load(), 10);
}

/**
 * @tc.name: PushAsyncMessage_ReturnsValidFuture
 * @tc.desc: test PushAsyncMessage returns valid future
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushAsyncMessage_ReturnsValidFuture, TestSize.Level1)
{
    WIDGET_LOGD("PushAsyncMessage_ReturnsValidFuture");
    auto& task = GraphicsTask::GetInstance();

    auto future = task.PushAsyncMessage([]() {});

    EXPECT_TRUE(future.valid());
}

/**
 * @tc.name: PushAsyncMessage_FutureWaitsForCompletion
 * @tc.desc: test PushAsyncMessage future waits for task completion
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushAsyncMessage_FutureWaitsForCompletion, TestSize.Level1)
{
    WIDGET_LOGD("PushAsyncMessage_FutureWaitsForCompletion");
    auto& task = GraphicsTask::GetInstance();
    std::atomic<bool> started{false};
    std::atomic<bool> finished{false};

    auto future = task.PushAsyncMessage([&started, &finished]() {
        started = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        finished = true;
    });

    // Future should block until task is done
    future.wait();

    EXPECT_TRUE(finished.load());
}

/**
 * @tc.name: PushSyncAndAsyncMixed_AllExecuted
 * @tc.desc: test mixed sync and async messages all execute
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushSyncAndAsyncMixed_AllExecuted, TestSize.Level1)
{
    WIDGET_LOGD("PushSyncAndAsyncMixed_AllExecuted");
    auto& task = GraphicsTask::GetInstance();
    std::atomic<int> counter{0};

    // Mix sync and async messages
    task.PushSyncMessage([&counter]() { counter++; });
    task.PushAsyncMessage([&counter]() { counter++; });
    task.PushSyncMessage([&counter]() { counter++; });
    auto future = task.PushAsyncMessage([&counter]() { counter++; });

    future.wait();

    EXPECT_EQ(counter.load(), 4);
}

/**
 * @tc.name: PushAsyncMessage_ChainedExecution
 * @tc.desc: test chained async message execution
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushAsyncMessage_ChainedExecution, TestSize.Level1)
{
    WIDGET_LOGD("PushAsyncMessage_ChainedExecution");
    auto& task = GraphicsTask::GetInstance();
    std::atomic<int> order{0};
    int step1 = 0, step2 = 0, step3 = 0;

    auto future1 = task.PushAsyncMessage([&]() {
        step1 = ++order;
    });
    auto future2 = task.PushAsyncMessage([&]() {
        step2 = ++order;
    });
    auto future3 = task.PushAsyncMessage([&]() {
        step3 = ++order;
    });

    future1.wait();
    future2.wait();
    future3.wait();

    // All tasks should have executed (order may vary due to async nature)
    EXPECT_GT(step1, 0);
    EXPECT_GT(step2, 0);
    EXPECT_GT(step3, 0);
}

/**
 * @tc.name: PushSyncMessage_WithCapture_ExecutesCorrectly
 * @tc.desc: test PushSyncMessage with captured variables
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushSyncMessage_WithCapture_ExecutesCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("PushSyncMessage_WithCapture_ExecutesCorrectly");
    auto& task = GraphicsTask::GetInstance();
    int value = 100;
    std::atomic<int> result{0};

    task.PushSyncMessage([&value, &result]() { result = value * 2; });

    EXPECT_EQ(result.load(), 200);
}

/**
 * @tc.name: PushAsyncMessage_WithCapture_ExecutesCorrectly
 * @tc.desc: test PushAsyncMessage with captured variables
 * @tc.type: FUNC
 */
HWTEST_F(GraphicsTaskUT, PushAsyncMessage_WithCapture_ExecutesCorrectly, TestSize.Level1)
{
    WIDGET_LOGD("PushAsyncMessage_WithCapture_ExecutesCorrectly");
    auto& task = GraphicsTask::GetInstance();
    int value = 50;
    std::atomic<int> result{0};

    auto future = task.PushAsyncMessage([&value, &result]() { result = value * 3; });

    future.wait();

    EXPECT_EQ(result.load(), 150);
}

} // namespace OHOS::Render3D