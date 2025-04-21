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

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

#include <base/containers/unique_ptr.h>
#include <core/implementation_uids.h>

#include "TestRunner.h"
#include "io/file_manager.h"
#include "threading/dispatcher_task_queue.h"
#include "threading/parallel_task_queue.h"
#include "threading/sequential_impl.h"
#include "threading/sequential_task_queue.h"
#include "threading/task_queue.h"

using namespace CORE_NS;
using namespace testing;
using namespace testing::ext;

namespace {
class Storage {
public:
    void Reset()
    {
        data.clear();
    }
    void Store(int number)
    {
        std::lock_guard<std::mutex> lock(mutex);
        data.push_back(number);
    }

    void CheckValidity(size_t count)
    {
        ASSERT_EQ(count, data.size());

        for (size_t i = 1; i < data.size(); ++i) {
            ASSERT_TRUE(data[i] > data[i - 1]);
        }
    }

    std::vector<int> data;
    std::mutex mutex;
};

static Storage g_storage;

template<size_t VALUE>
void TestFunction()
{
    g_storage.Store(VALUE);
}

void Wait(int ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}
} // namespace

struct TestContext {
    std::shared_ptr<ISceneInit> sceneInit_ = nullptr;
    CORE_NS::IEcs::Ptr ecs_;
};
static TestContext g_context;

using IntfPtr = BASE_NS::shared_ptr<CORE_NS::IInterface>;
using IntfWeakPtr = BASE_NS::weak_ptr<CORE_NS::IInterface>;
static constexpr BASE_NS::Uid ENGINE_THREAD{"2070e705-d061-40e4-bfb7-90fad2c280af"};
static constexpr BASE_NS::Uid APP_THREAD{"b2e8cef3-453a-4651-b564-5190f8b5190d"};
static constexpr BASE_NS::Uid IO_QUEUE{"be88e9a0-9cd8-45ab-be48-937953dc258f"};
static constexpr BASE_NS::Uid JS_RELEASE_THREAD{"3784fa96-b25b-4e9c-bbf1-e897d36f73af"};

bool SceneDispose(TestContext &context)
{
    context.ecs_ = nullptr;
    context.sceneInit_ = nullptr;
    return true;
}

bool SceneCreate(TestContext &context)
{
    context.sceneInit_ = CreateTestScene();
    context.sceneInit_->LoadPluginsAndInit();
    if (!context.sceneInit_->GetEngineInstance().engine_) {
        WIDGET_LOGE("fail to get engine");
        return false;
    }
    context.ecs_ = context.sceneInit_->GetEngineInstance().engine_->CreateEcs();
    if (!context.ecs_) {
        WIDGET_LOGE("fail to get ecs");
        return false;
    }
    auto factory = GetInstance<ISystemGraphLoaderFactory>(UID_SYSTEM_GRAPH_LOADER);
    auto systemGraphLoader = factory->Create(context.sceneInit_->GetEngineInstance().engine_->GetFileManager());
    systemGraphLoader->Load("rofs3D://systemGraph.json", *(context.ecs_));
    auto& ecs = *(context.ecs_);
    ecs.Initialize();

    using namespace SCENE_NS;
#if SCENE_META_FUZZ
    auto fun = [&context]() {
        auto &obr = META_NS::GetObjectRegistry();

        context.params_ = interface_pointer_cast<META_NS::IMetadata>(obr.GetDefaultObjectContext());
        if (!context.params_) {
            CORE_LOG_E("default obj null");
        }
        context.scene_ =
            interface_pointer_cast<SCENE_NS::IScene>(obr.Create(SCENE_NS::ClassId::Scene, context.params_));
        
        auto onLoaded = META_NS::MakeCallback<META_NS::IOnChanged>([&context]() {
            bool complete = false;
            auto status = context.scene_->Status()->GetValue();
            if (status == SCENE_NS::IScene::SCENE_STATUS_READY) {
                // still in engine thread
                complete = true;
            } else if (status == SCENE_NS::IScene::SCENE_STATUS_LOADING_FAILED) {
                // make sure we don't have anything in result if error
                complete = true;
            }

            if (complete) {
                if (context.scene_) {
                    auto &obr = META_NS::GetObjectRegistry();
                    // make sure we have renderconfig
                    auto rc = context.scene_->RenderConfiguration()->GetValue();
                    if (!rc) {
                        // Create renderconfig
                        rc = obr.Create<SCENE_NS::IRenderConfiguration>(SCENE_NS::ClassId::RenderConfiguration);
                        context.scene_->RenderConfiguration()->SetValue(rc);
                    }

                    interface_cast<IEcsScene>(context.scene_)
                        ->RenderMode()
                        ->SetValue(IEcsScene::RenderMode::RENDER_ALWAYS);
                    auto duh = context.params_->GetArrayPropertyByName<IntfWeakPtr>("Scenes");
                    if (!duh) {
                        return ;
                    }
                    duh->AddValue(interface_pointer_cast<CORE_NS::IInterface>(context.scene_));
                }
            }
        });
        context.scene_->Asynchronous()->SetValue(false);
        context.scene_->Uri()->SetValue("scene://empty");
        return META_NS::IAny::Ptr{};
    };
    // Should it be possible to cancel? (ie. do we need to Store the token for something ..)
    META_NS::GetTaskQueueRegistry()
        .GetTaskQueue(ENGINE_THREAD)
        ->AddWaitableTask(META_NS::MakeCallback<META_NS::ITaskQueueWaitableTask>(BASE_NS::move(fun)))
        ->Wait();
#endif
    return true;
}

class TaskQueueTest : public testing::Test {
public:
    static void SetUpTestSuite()
    {
        SceneCreate(g_context);
    }
    static void TearDownTestSuite()
    {
        SceneDispose(g_context);
    }
    void SetUp() override {}
    void TearDown() override {}
};

HWTEST_F(TaskQueueTest, testExecutionOrder, TestSize.Level1)
{
    SequentialTaskQueue queue(nullptr);

    queue.Submit(1, FunctionTask::Create([]() { g_storage.Store(1); }));
    queue.Submit(4, FunctionTask::Create([]() { g_storage.Store(4); }));
    queue.SubmitAfter(1, 2, FunctionTask::Create(TestFunction<2>));
    queue.SubmitBefore(4, 3, FunctionTask::Create([]() { TestFunction<3>(); }));
    queue.SubmitAfter(9, 5, FunctionTask::Create(TestFunction<5>));
    queue.SubmitBefore(1, 0, FunctionTask::Create(std::bind(&Storage::Reset, &g_storage)));

    // Execute and Wait for completion.
    queue.Execute();

    g_storage.CheckValidity(5);
}

HWTEST_F(TaskQueueTest, testHierarchy, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4U);
    ASSERT_TRUE(threadPool);
    EXPECT_EQ(threadPool->GetNumberOfThreads(), 4U);

    SequentialTaskQueue mainQueue(threadPool);

    mainQueue.Submit(0, FunctionTask::Create(std::bind(&Storage::Reset, &g_storage)));

    mainQueue.Submit(1, FunctionTask::Create([]() { g_storage.Store(1); }));
    mainQueue.Submit(2, FunctionTask::Create([]() { g_storage.Store(2); }));
    mainQueue.Submit(10, FunctionTask::Create([threadPool]() {
        // Threaded sequential queue.
        SequentialTaskQueue queue(threadPool);
        queue.Submit(4, FunctionTask::Create([]() { g_storage.Store(4); }));
        queue.Submit(5, FunctionTask::Create([]() { g_storage.Store(5); }));
        queue.Submit(11, FunctionTask::Create([threadPool]() {
            // Threaded parallel queue.
            ParallelTaskQueue subQueue(threadPool);
            subQueue.Submit(8, FunctionTask::Create([]() {
                Wait(2000);
                g_storage.Store(8);
            }));
            subQueue.Submit(7, FunctionTask::Create([]() {
                Wait(1000);
                g_storage.Store(7);
            }));
            subQueue.Execute();
        }));

        queue.SubmitBefore(11, 6, FunctionTask::Create([]() { g_storage.Store(6); }));
        queue.SubmitAfter(11, 9, FunctionTask::Create([]() { g_storage.Store(9); }));
        queue.Execute();
    }));

    mainQueue.SubmitBefore(10, 3, FunctionTask::Create([]() { g_storage.Store(3); }));

    // Execute (async) and Wait for completion.
    mainQueue.ExecuteAsync();
    mainQueue.Wait();

    g_storage.CheckValidity(9);
}

HWTEST_F(TaskQueueTest, testParallelWithDependencies, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    ParallelTaskQueue mainQueue(threadPool);

    // Reset.
    mainQueue.Submit(0, FunctionTask::Create([]() {
        g_storage.Reset();
        Wait(1000);
    }));

    // Store number 1
    mainQueue.SubmitAfter(0, 1, FunctionTask::Create([]() { g_storage.Store(1); }));
    mainQueue.SubmitAfter(0, 9, FunctionTask::Create([]() { g_storage.Store(9); }));
    mainQueue.Remove(9);
    mainQueue.Remove(10);
    // Store number 2 after storing number 1.
    mainQueue.SubmitAfter(1, 2, FunctionTask::Create([]() { g_storage.Store(2); }));

    // Store number 3 after Reset and storing number 2.
    constexpr const uint64_t afterIds[] = { 2, 0 };
    mainQueue.SubmitAfter(afterIds, 3, FunctionTask::Create([]() { g_storage.Store(3); }));

    // Execute (async) and Wait for completion.
    mainQueue.ExecuteAsync();
    mainQueue.Wait();

    g_storage.CheckValidity(3);
}

HWTEST_F(TaskQueueTest, testBlocking, TestSize.Level1)
{
    // Reserve 1 core for long running operations and 3 for quick operations.
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    g_storage.Reset();

    // Create sequential queue with a few 'long running' tasks, this preserves one core.
    SequentialTaskQueue sequentialQueue(threadPool);
    for (int i = 0; i < 2; ++i) {
        // First and last operation in storage.
        sequentialQueue.Submit(i, FunctionTask::Create([i]() {
            g_storage.Store(i * 10);
            Wait(2000);
        }));
    }

    // Create parallel queue with 'quickly running' tasks, this preserves rest of the cores.
    ParallelTaskQueue parallelQueue(threadPool);
    for (int i = 0; i < 5; ++i) {
        // Preserve index 0 for long running op that starts before this.
        parallelQueue.Submit(i, FunctionTask::Create([i]() {
            Wait(50 * i);
            g_storage.Store(i + 1);
        }));
    }

    // Start long running operations on background.
    sequentialQueue.ExecuteAsync();

    // Wait 250ms to make sure 1st sequential task starts before the parallel ones.
    Wait(250);

    // Execute quick operations in parallel.
    parallelQueue.Execute();

    // Wait for long running operations to complete.
    sequentialQueue.Wait();

    g_storage.CheckValidity(7);
}

HWTEST_F(TaskQueueTest, testPrematureDestruction, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);
    {
        g_storage.Reset();

        // Creating with new so we can explicitly destroy this too early on purpose.
        DispatcherTaskQueue* queue = new DispatcherTaskQueue(threadPool);

        queue->Submit(0, FunctionTask::Create([]() { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }));

        queue->ExecuteAsync();
        delete queue;
    }
}

HWTEST_F(TaskQueueTest, testDispatcherSync, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    g_storage.Reset();

    DispatcherTaskQueue queue(threadPool);

    // Submit a few tasks to queue.
    for (int i = 0; i < 5; ++i) {
        queue.Submit(i, FunctionTask::Create([i]() { g_storage.Store(i); }));
    }

    queue.SubmitAfter(5, 5, FunctionTask::Create([]() { g_storage.Store(5); }));
    queue.SubmitAfter(1, 6, FunctionTask::Create([]() { g_storage.Store(6); }));

    BASE_NS::vector<uint64_t> afterIdentifiers(1);
    afterIdentifiers[0] = 6;
    BASE_NS::array_view<const uint64_t> nullAfters;
    BASE_NS::array_view<const uint64_t> singleafters =
        BASE_NS::array_view<const uint64_t>(afterIdentifiers.data(), afterIdentifiers.size());
    queue.SubmitAfter(nullAfters, 7, FunctionTask::Create([]() { g_storage.Store(7); }));
    queue.SubmitAfter(singleafters, 7, FunctionTask::Create([]() { g_storage.Store(7); }));

    queue.Remove(6);
    queue.Remove(7);

    for (int i = 0; i < 6; ++i) {
        // Process one task from queue.
        queue.Execute();

        auto collectedTasks = queue.CollectFinishedTasks();
        ASSERT_EQ(collectedTasks.size(), 1);
    }

    g_storage.CheckValidity(6);

    queue.Clear();
}

HWTEST_F(TaskQueueTest, testDispatcherAsync, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    g_storage.Reset();

    DispatcherTaskQueue queue(threadPool);

    // Submit a few tasks to queue.
    for (int i = 0; i < 5; ++i) {
        queue.Submit(i, FunctionTask::Create([i]() { g_storage.Store(i); }));
    }

    // Process all tasks asynchronously, one by one.
    size_t numberOfTasksCompleted = 0;
    while (numberOfTasksCompleted != 5) {
        // Progress queue.
        queue.ExecuteAsync();

        auto collectedTasks = queue.CollectFinishedTasks();
        numberOfTasksCompleted += collectedTasks.size();
    }

    g_storage.CheckValidity(5);

    // Test other taskqueue

    const uint64_t afters[3] = { 9, 10, 11 };
    BASE_NS::array_view<const uint64_t> afterIds(afters);
    BASE_NS::array_view<const uint64_t> nullAfters;

    auto dispatcher = factory->CreateDispatcherTaskQueue(threadPool);
    dispatcher->Submit(10, FunctionTask::Create([]() { g_storage.Store(10); }));
    dispatcher->SubmitAfter(10, 11, FunctionTask::Create([]() { g_storage.Store(11); }));
    dispatcher->SubmitAfter(afterIds, 12, FunctionTask::Create([]() { g_storage.Store(12); }));
    dispatcher->SubmitAfter(nullAfters, 15, FunctionTask::Create([]() { g_storage.Store(15); }));
    dispatcher->Execute();
    EXPECT_EQ(dispatcher->CollectFinishedTasks().size(), 1);
    dispatcher->Clear();

    auto sequential = factory->CreateSequentialTaskQueue(threadPool);
    sequential->Submit(10, FunctionTask::Create([]() { g_storage.Store(10); }));
    sequential->SubmitAfter(10, 11, FunctionTask::Create([]() { g_storage.Store(11); }));
    sequential->SubmitAfter(afterIds, 12, FunctionTask::Create([]() { g_storage.Store(12); }));
    sequential->Execute();
    sequential->Clear();

    auto parallel = factory->CreateParallelTaskQueue(threadPool);
    parallel->Submit(10, FunctionTask::Create([]() { g_storage.Store(10); }));
    parallel->SubmitAfter(10, 11, FunctionTask::Create([]() { g_storage.Store(11); }));
    parallel->SubmitAfter(afterIds, 12, FunctionTask::Create([]() { g_storage.Store(12); }));
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

HWTEST_F(TaskQueueTest, testSequentialMethods, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(1);

    g_storage.Reset();

    auto seq = factory->CreateSequentialTaskQueue(threadPool);
    EXPECT_TRUE(seq != nullptr);
    SequentialTaskQueue sq { threadPool };
    sq.Remove(1);
    sq.Submit(1, FunctionTask::Create([]() { g_storage.Store(1); }));
    sq.Remove(1);
}

HWTEST_F(TaskQueueTest, testDispatcherOverMultipleFrames, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    g_storage.Reset();

    DispatcherTaskQueue queue(threadPool);

    const int tasksToSpawn = 10;
    int numberOfTasksSpawned = 0;
    size_t numberOfTasksExecuted = 0;

    while (numberOfTasksExecuted != tasksToSpawn) {
        if (numberOfTasksSpawned < tasksToSpawn) {
            // Spawn new task.
            int timeout = (numberOfTasksSpawned & 0x1) ? 8 : 24;
            queue.Submit(numberOfTasksSpawned, FunctionTask::Create([numberOfTasksSpawned, timeout]() {
                Wait(timeout);
                g_storage.Store(numberOfTasksSpawned);
            }));
            numberOfTasksSpawned++;
        }

        // Progress queue.
        queue.ExecuteAsync();

        // Progress "frame".
        Wait(16);

        // Collect results.
        auto collectedTasks = queue.CollectFinishedTasks();
        numberOfTasksExecuted += collectedTasks.size();
    }

    g_storage.CheckValidity(10);
}

HWTEST_F(TaskQueueTest, multithreadIo, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4);

    g_storage.Reset();

    SequentialTaskQueue queue(threadPool);

    queue.Submit(1, FunctionTask::Create([]() {
        auto& files = g_context.sceneInit_->GetEngineInstance().engine_->GetFileManager();

        auto directory = files.OpenDirectory("file:///data/local/test_data/io/test_directory");
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

        g_storage.Store(1);
    }));

    // Execute and Wait for completion.
    queue.ExecuteAsync();
    queue.Wait();

    g_storage.CheckValidity(1);
}

HWTEST_F(TaskQueueTest, testThreadPool, TestSize.Level1)
{
    const auto factory = GetInstance<ITaskQueueFactory>(UID_TASK_QUEUE_FACTORY);
    auto threadPool = factory->CreateThreadPool(4U);
    {
        // task which resets the storage after a delay. the delay tries to give time to add more tasks.
        auto resetTask = FunctionTask::Create([]() {
            Wait(100);
            g_storage.Reset();
        });
        const auto* resetTaskPtr = resetTask.get();
        threadPool->PushNoWait(BASE_NS::move(resetTask));

        // two tasks which should Wait for the Reset task.
        auto task1 = FunctionTask::Create([]() {
            g_storage.Store(1);
            Wait(50);
        });
        auto task1Ptr = task1.get();
        const CORE_NS::IThreadPool::ITask* deps0[] = { resetTaskPtr };
        threadPool->PushNoWait(BASE_NS::move(task1), deps0);

        auto task2 = FunctionTask::Create([]() {
            Wait(50);
            g_storage.Store(2);
        });
        auto task2Ptr = task2.get();
        threadPool->PushNoWait(BASE_NS::move(task2), deps0);

        // one more task which should start after the above tasks.
        auto task3 = FunctionTask::Create([]() { g_storage.Store(3); });
        auto task3Ptr = task3.get();
        const CORE_NS::IThreadPool::ITask* deps12[] = { task1Ptr, task2Ptr };
        auto result = threadPool->Push(BASE_NS::move(task3), deps12);

        // assuming tasks were created fast enough the last task isn't ready until we Wait.
        EXPECT_FALSE(result->IsDone());
        result->Wait();
        EXPECT_TRUE(result->IsDone());

        ASSERT_EQ(g_storage.data.size(), 3);
        EXPECT_EQ(g_storage.data[2], 3);
    }
    // do the same without delays. dependencies should work even if the work was already completed.
    {
        // create all the tasks first to guarantee that the pointers are unique.
        auto resetTask = FunctionTask::Create([]() { g_storage.Reset(); });
        auto task1 = FunctionTask::Create([]() { g_storage.Store(1); });
        auto task2 = FunctionTask::Create([]() { g_storage.Store(2); });
        auto task3 = FunctionTask::Create([]() { g_storage.Store(3); });

        const auto* resetTaskPtr = resetTask.get();
        threadPool->PushNoWait(BASE_NS::move(resetTask));

        // two tasks which should Wait for the Reset task.
        auto task1Ptr = task1.get();
        const CORE_NS::IThreadPool::ITask* deps0[] = { resetTaskPtr };
        threadPool->PushNoWait(BASE_NS::move(task1), deps0);

        auto task2Ptr = task2.get();
        threadPool->PushNoWait(BASE_NS::move(task2), deps0);

        // one more task which should start after the above tasks.
        auto task3Ptr = task3.get();
        const CORE_NS::IThreadPool::ITask* deps12[] = { task1Ptr, task2Ptr };
        auto result = threadPool->Push(BASE_NS::move(task3), deps12);

        // assuming tasks were created fast enough the last task isn't ready until we Wait.
        EXPECT_FALSE(result->IsDone());
        result->Wait();
        EXPECT_TRUE(result->IsDone());

        ASSERT_EQ(g_storage.data.size(), 3);
        EXPECT_EQ(g_storage.data[2], 3);
    }
}
