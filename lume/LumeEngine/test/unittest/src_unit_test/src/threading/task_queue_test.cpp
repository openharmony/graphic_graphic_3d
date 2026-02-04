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

#include <chrono>
#include <thread>

#include <base/containers/unique_ptr.h>
#include <core/implementation_uids.h>

#include "test_framework.h"

#if defined(UNIT_TESTS_USE_HCPPTEST)
#include "test_runner_ohos_system.h"
#else
#include "test_runner.h"
#endif
#include "io/file_manager.h"
#include "threading/dispatcher_task_queue.h"
#include "threading/parallel_task_queue.h"
#include "threading/sequential_impl.h"
#include "threading/sequential_task_queue.h"
#include "threading/task_queue.h"

using namespace CORE_NS;

class Storage {
public:
    void reset()
    {
        data.clear();
    }
    void store(int number)
    {
        std::lock_guard<std::mutex> lock(mutex);
        data.push_back(number);
    }

    void checkValidity(size_t count)
    {
        ASSERT_EQ(count, data.size());

        for (size_t i = 1; i < data.size(); ++i) {
            ASSERT_TRUE(data[i] > data[i - 1]);
        }
    }

    std::vector<int> data;
    std::mutex mutex;
};

namespace {
static Storage gStorage;
}

template<size_t VALUE>
void function()
{
    gStorage.store(VALUE);
}

void wait(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

/**
 * @tc.name: testExecutionOrder
 * @tc.desc: Tests for Test Execution Order. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testExecutionOrder, testing::ext::TestSize.Level1)
{
    SequentialTaskQueue queue(nullptr);

    queue.Submit(1, FunctionTask::Create([]() { gStorage.store(1); }));
    queue.Submit(4, FunctionTask::Create([]() { gStorage.store(4); }));
    queue.SubmitAfter(1, 2, FunctionTask::Create(function<2>));
    queue.SubmitBefore(4, 3, FunctionTask::Create([]() { function<3>(); }));
    queue.SubmitAfter(9, 5, FunctionTask::Create(function<5>));
    queue.SubmitBefore(1, 0, FunctionTask::Create(std::bind(&Storage::reset, &gStorage)));

    // Execute and wait for completion.
    queue.Execute();

    gStorage.checkValidity(5);
}

#ifdef DISABLED_TESTS_ON
#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: testHierarchy
 * @tc.desc: Tests for Test Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, DISABLED_testHierarchy, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: testHierarchy
 * @tc.desc: Tests for Test Hierarchy. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testHierarchy, testing::ext::TestSize.Level1)
#endif
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4U);
    ASSERT_TRUE(threadPool);
    EXPECT_EQ(threadPool->GetNumberOfThreads(), 4U);

    SequentialTaskQueue mainQueue(threadPool);

    mainQueue.Submit(0, FunctionTask::Create(std::bind(&Storage::reset, &gStorage)));

    mainQueue.Submit(1, FunctionTask::Create([]() { gStorage.store(1); }));
    mainQueue.Submit(2, FunctionTask::Create([]() { gStorage.store(2); }));
    mainQueue.Submit(10, FunctionTask::Create([threadPool]() {
        // Threaded sequential queue.
        SequentialTaskQueue queue(threadPool);
        queue.Submit(4, FunctionTask::Create([]() { gStorage.store(4); }));
        queue.Submit(5, FunctionTask::Create([]() { gStorage.store(5); }));
        queue.Submit(11, FunctionTask::Create([threadPool]() {
            // Threaded parallel queue.
            ParallelTaskQueue subQueue(threadPool);
            subQueue.Submit(8, FunctionTask::Create([]() {
                wait(2000);
                gStorage.store(8);
            }));
            subQueue.Submit(7, FunctionTask::Create([]() {
                wait(1000);
                gStorage.store(7);
            }));
            subQueue.Execute();
        }));

        queue.SubmitBefore(11, 6, FunctionTask::Create([]() { gStorage.store(6); }));
        queue.SubmitAfter(11, 9, FunctionTask::Create([]() { gStorage.store(9); }));
        queue.Execute();
    }));

    mainQueue.SubmitBefore(10, 3, FunctionTask::Create([]() { gStorage.store(3); }));

    // Execute (async) and wait for completion.
    mainQueue.ExecuteAsync();
    mainQueue.Wait();

    gStorage.checkValidity(9);
}

#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: testParallelWithDependencies
 * @tc.desc: Tests for Test Parallel With Dependencies. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, DISABLED_testParallelWithDependencies, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: testParallelWithDependencies
 * @tc.desc: Tests for Test Parallel With Dependencies. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testParallelWithDependencies, testing::ext::TestSize.Level1)
#endif
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    ParallelTaskQueue mainQueue(threadPool);

    // Reset.
    mainQueue.Submit(0, FunctionTask::Create([]() {
        gStorage.reset();
        wait(1000);
    }));

    // Store number 1
    mainQueue.SubmitAfter(0, 1, FunctionTask::Create([]() { gStorage.store(1); }));
    mainQueue.SubmitAfter(0, 9, FunctionTask::Create([]() { gStorage.store(9); }));
    mainQueue.Remove(9);
    mainQueue.Remove(10);
    // Store number 2 after storing number 1.
    mainQueue.SubmitAfter(1, 2, FunctionTask::Create([]() { gStorage.store(2); }));

    // Store number 3 after reset and storing number 2.
    constexpr const uint64_t afterIds[] = { 2, 0 };
    mainQueue.SubmitAfter(afterIds, 3, FunctionTask::Create([]() { gStorage.store(3); }));

    // Execute (async) and wait for completion.
    mainQueue.ExecuteAsync();
    mainQueue.Wait();

    gStorage.checkValidity(3);
}

#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: testBlocking
 * @tc.desc: Tests for Test Blocking. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, DISABLED_testBlocking, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: testBlocking
 * @tc.desc: Tests for Test Blocking. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testBlocking, testing::ext::TestSize.Level1)
#endif
{
    // Reserve 1 core for long running operations and 3 for quick operations.
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    gStorage.reset();

    // Create sequential queue with a few 'long running' tasks, this preserves one core.
    SequentialTaskQueue sequentialQueue(threadPool);
    for (int i = 0; i < 2; ++i) {
        // First and last operation in storage.
        sequentialQueue.Submit(i, FunctionTask::Create([i]() {
            gStorage.store(i * 10);
            wait(2000);
        }));
    }

    // Create parallel queue with 'quickly running' tasks, this preserves rest of the cores.
    ParallelTaskQueue parallelQueue(threadPool);
    for (int i = 0; i < 5; ++i) {
        // Preserve index 0 for long running op that starts before this.
        parallelQueue.Submit(i, FunctionTask::Create([i]() {
            wait(50 * i);
            gStorage.store(i + 1);
        }));
    }

    // Start long running operations on background.
    sequentialQueue.ExecuteAsync();

    // Wait 250ms to make sure 1st sequential task starts before the parallel ones.
    wait(250);

    // Execute quick operations in parallel.
    parallelQueue.Execute();

    // Wait for long running operations to complete.
    sequentialQueue.Wait();

    gStorage.checkValidity(7);
}
#endif // DISABLED_TESTS_ON

/**
 * @tc.name: testPrematureDestruction
 * @tc.desc: Tests for Test Premature Destruction. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testPrematureDestruction, testing::ext::TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);
    {
        gStorage.reset();

        // Creating with new so we can explicitly destroy this too early on purpose.
        DispatcherTaskQueue* queue = new DispatcherTaskQueue(threadPool);

        queue->Submit(0, FunctionTask::Create([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }));

        queue->ExecuteAsync();
        delete queue;
    }
}

/**
 * @tc.name: testDispatcherSync
 * @tc.desc: Tests for Test Dispatcher Sync. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testDispatcherSync, testing::ext::TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    gStorage.reset();

    DispatcherTaskQueue queue(threadPool);

    // Submit a few tasks to queue.
    for (int i = 0; i < 5; ++i) {
        queue.Submit(i, FunctionTask::Create([i]() { gStorage.store(i); }));
    }

    queue.SubmitAfter(5, 5, FunctionTask::Create([]() { gStorage.store(5); }));
    queue.SubmitAfter(1, 6, FunctionTask::Create([]() { gStorage.store(6); }));

    BASE_NS::vector<uint64_t> afterIdentifiers(1);
    afterIdentifiers[0] = 6;
    BASE_NS::array_view<const uint64_t> nullAfters;
    BASE_NS::array_view<const uint64_t> singleafters =
        BASE_NS::array_view<const uint64_t>(afterIdentifiers.data(), afterIdentifiers.size());
    queue.SubmitAfter(nullAfters, 7, FunctionTask::Create([]() { gStorage.store(7); }));
    queue.SubmitAfter(singleafters, 7, FunctionTask::Create([]() { gStorage.store(7); }));

    queue.Remove(6);
    queue.Remove(7);

    for (int i = 0; i < 6; ++i) {
        // Process one task from queue.
        queue.Execute();

        auto collectedTasks = queue.CollectFinishedTasks();
        ASSERT_EQ(collectedTasks.size(), 1);
    }

    gStorage.checkValidity(6);

    queue.Clear();
}

/**
 * @tc.name: testDispatcherAsync
 * @tc.desc: Tests for Test Dispatcher Async. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testDispatcherAsync, testing::ext::TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    gStorage.reset();

    DispatcherTaskQueue queue(threadPool);

    // Submit a few tasks to queue.
    for (int i = 0; i < 5; ++i) {
        queue.Submit(i, FunctionTask::Create([i]() { gStorage.store(i); }));
    }

    // Process all tasks asynchronously, one by one.
    size_t numberOfTasksCompleted = 0;
    while (numberOfTasksCompleted != 5) {
        // Progress queue.
        queue.ExecuteAsync();

        auto collectedTasks = queue.CollectFinishedTasks();
        numberOfTasksCompleted += collectedTasks.size();
    }

    gStorage.checkValidity(5);

    // Test other taskqueue

    const uint64_t afters[3] = { 9, 10, 11 };
    BASE_NS::array_view<const uint64_t> afterIds(afters);
    BASE_NS::array_view<const uint64_t> nullAfters;

    auto dispatcher = factory->CreateDispatcherTaskQueue(threadPool);
    dispatcher->Submit(10, FunctionTask::Create([]() { gStorage.store(10); }));
    dispatcher->SubmitAfter(10, 11, FunctionTask::Create([]() { gStorage.store(11); }));
    dispatcher->SubmitAfter(afterIds, 12, FunctionTask::Create([]() { gStorage.store(12); }));
    dispatcher->SubmitAfter(nullAfters, 15, FunctionTask::Create([]() { gStorage.store(15); }));
    dispatcher->Execute();
    EXPECT_EQ(dispatcher->CollectFinishedTasks().size(), 1);
    dispatcher->Clear();

    auto sequential = factory->CreateSequentialTaskQueue(threadPool);
    sequential->Submit(10, FunctionTask::Create([]() { gStorage.store(10); }));
    sequential->SubmitAfter(10, 11, FunctionTask::Create([]() { gStorage.store(11); }));
    sequential->SubmitAfter(afterIds, 12, FunctionTask::Create([]() { gStorage.store(12); }));
    sequential->Execute();
    sequential->Clear();

    auto parallel = factory->CreateParallelTaskQueue(threadPool);
    parallel->Submit(10, FunctionTask::Create([]() { gStorage.store(10); }));
    parallel->SubmitAfter(10, 11, FunctionTask::Create([]() { gStorage.store(11); }));
    parallel->SubmitAfter(afterIds, 12, FunctionTask::Create([]() { gStorage.store(12); }));
    parallel->Execute();
    parallel->Clear();

    EXPECT_TRUE(threadPool->GetInterface(IThreadPool::UID) != nullptr);
    EXPECT_FALSE(threadPool->GetInterface(UID_TASK_QUEUE_FACTORY) != nullptr);
    const auto& constPool = *threadPool;
    EXPECT_TRUE(constPool.GetInterface(IThreadPool::UID) != nullptr);
    EXPECT_FALSE(constPool.GetInterface(UID_TASK_QUEUE_FACTORY) != nullptr);

    factory->Ref();
    factory->Unref();

    EXPECT_TRUE(threadPool->Push(nullptr) != nullptr);
}

/**
 * @tc.name: testSequentialMethods
 * @tc.desc: Tests for Test Sequential Methods. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testSequentialMethods, testing::ext::TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(1);

    gStorage.reset();

    auto seq = factory->CreateSequentialTaskQueue(threadPool);
    EXPECT_TRUE(seq != nullptr);
    SequentialTaskQueue sq { threadPool };
    sq.Remove(1);
    sq.Submit(1, FunctionTask::Create([]() { gStorage.store(1); }));
    sq.Remove(1);
}

/**
 * @tc.name: testDispatcherOverMultipleFrames
 * @tc.desc: Tests for Test Dispatcher Over Multiple Frames. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testDispatcherOverMultipleFrames, testing::ext::TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    gStorage.reset();

    DispatcherTaskQueue queue(threadPool);

    const int tasksToSpawn = 10;
    int numberOfTasksSpawned = 0;
    size_t numberOfTasksExecuted = 0;

    while (numberOfTasksExecuted != tasksToSpawn) {
        if (numberOfTasksSpawned < tasksToSpawn) {
            // Spawn new task.
            int timeout = (numberOfTasksSpawned & 0x1) ? 8 : 24;
            queue.Submit(numberOfTasksSpawned, FunctionTask::Create([numberOfTasksSpawned, timeout]() {
                wait(timeout);
                gStorage.store(numberOfTasksSpawned);
            }));
            numberOfTasksSpawned++;
        }

        // Progress queue.
        queue.ExecuteAsync();

        // Progress "frame".
        wait(16);

        // Collect results.
        auto collectedTasks = queue.CollectFinishedTasks();
        numberOfTasksExecuted += collectedTasks.size();
    }

    gStorage.checkValidity(10);
}

#ifdef DISABLED_TESTS_ON
#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: multithreadIo
 * @tc.desc: Tests for Multithread Io. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, DISABLED_multithreadIo, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: multithreadIo
 * @tc.desc: Tests for Multithread Io. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, multithreadIo, testing::ext::TestSize.Level1)
#endif
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    gStorage.reset();

    SequentialTaskQueue queue(threadPool);

    queue.Submit(1, FunctionTask::Create([]() {
        auto& files = CORE_NS::UTest::GetTestEnv()->fileManager;
        ASSERT_TRUE(files != nullptr);

        auto directory = files->OpenDirectory("test://io/test_directory");
        ASSERT_TRUE(directory != nullptr);

        // There should be 5 files and 1 directory.
        int fileCount = 0;
        int dirCount = 0;
        for (const auto& entry : directory->GetEntries()) {
            if (entry.type == IDirectory::Entry::FILE) {
                fileCount++;
            } else if (entry.type == IDirectory::Entry::DIRECTORY) {
                if (entry.name != "." && entry.name != "..") {
                    dirCount++;
                }
            }
        }
        ASSERT_EQ(fileCount, 5);
        ASSERT_EQ(dirCount, 1);

        gStorage.store(1);
    }));

    // Execute and wait for completion.
    queue.ExecuteAsync();
    queue.Wait();

    gStorage.checkValidity(1);
}

#if defined(__OHOS__) && !defined(__OHOS_PLATFORM__)
/**
 * @tc.name: testThreadPool
 * @tc.desc: Tests for Test Thread Pool. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, DISABLED_testThreadPool, testing::ext::TestSize.Level1)
#else
/**
 * @tc.name: testThreadPool
 * @tc.desc: Tests for Test Thread Pool. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(SRC_TaskQueueTest, testThreadPool, testing::ext::TestSize.Level1)
#endif
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4U);
    {
        // task which resets the storage after a delay. the delay tries to give time to add more tasks.
        auto resetTask = FunctionTask::Create([]() {
            wait(100);
            gStorage.reset();
        });
        const auto* resetTaskPtr = resetTask.get();
        threadPool->PushNoWait(BASE_NS::move(resetTask));

        // two tasks which should wait for the reset task.
        auto task1 = FunctionTask::Create([]() {
            gStorage.store(1);
            wait(50);
        });
        auto task1Ptr = task1.get();
        const CORE_NS::IThreadPool::ITask* deps0[] = { resetTaskPtr };
        threadPool->PushNoWait(BASE_NS::move(task1), deps0);

        auto task2 = FunctionTask::Create([]() {
            wait(50);
            gStorage.store(2);
        });
        auto task2Ptr = task2.get();
        threadPool->PushNoWait(BASE_NS::move(task2), deps0);

        // one more task which should start after the above tasks.
        auto task3 = FunctionTask::Create([]() { gStorage.store(3); });
        auto task3Ptr = task3.get();
        const CORE_NS::IThreadPool::ITask* deps12[] = { task1Ptr, task2Ptr };
        auto result = threadPool->Push(BASE_NS::move(task3), deps12);

        // assuming tasks were created fast enough the last task isn't ready until we wait.
        EXPECT_FALSE(result->IsDone());
        result->Wait();
        EXPECT_TRUE(result->IsDone());

        ASSERT_EQ(gStorage.data.size(), 3);
        EXPECT_EQ(gStorage.data[2], 3);
    }
    // do the same without delays. dependencies should work even if the work was already completed.
    {
        // create all the tasks first to guarantee that the pointers are unique.
        auto resetTask = FunctionTask::Create([]() { gStorage.reset(); });
        auto task1 = FunctionTask::Create([]() {
            gStorage.store(1);
            wait(5);
        });
        auto task2 = FunctionTask::Create([]() { gStorage.store(2); });
        auto task3 = FunctionTask::Create([]() { gStorage.store(3); });

        const auto* resetTaskPtr = resetTask.get();
        threadPool->PushNoWait(BASE_NS::move(resetTask));

        // two tasks which should wait for the reset task.
        auto task1Ptr = task1.get();
        const CORE_NS::IThreadPool::ITask* deps0[] = { resetTaskPtr };
        threadPool->PushNoWait(BASE_NS::move(task1), deps0);

        auto task2Ptr = task2.get();
        threadPool->PushNoWait(BASE_NS::move(task2), deps0);

        // one more task which should start after the above tasks.
        auto task3Ptr = task3.get();
        const CORE_NS::IThreadPool::ITask* deps12[] = { task1Ptr, task2Ptr };
        auto result = threadPool->Push(BASE_NS::move(task3), deps12);

        // assuming tasks were created fast enough the last task isn't ready until we wait.
        EXPECT_FALSE(result->IsDone());
        result->Wait();
        EXPECT_TRUE(result->IsDone());

        ASSERT_EQ(gStorage.data.size(), 3);
        EXPECT_EQ(gStorage.data[2], 3);
    }
}
#endif // DISABLED_TESTS_ON