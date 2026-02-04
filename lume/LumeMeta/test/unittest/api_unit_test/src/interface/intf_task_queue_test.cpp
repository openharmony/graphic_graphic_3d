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
#include <chrono>
#include <test_framework.h>
#include <thread>

#include <meta/api/future.h>
#include <meta/api/make_callback.h>
#include <meta/api/task.h>
#include <meta/api/task_queue.h>
#include <meta/ext/task_queue.h>
#include <meta/interface/intf_task_queue.h>
#include <meta/interface/intf_task_queue_registry.h>
#include <meta/interface/object_macros.h>

META_BEGIN_NAMESPACE()
namespace UTest {

using namespace TimeSpanLiterals;

namespace {

template<typename Func>
class Task : public IntroduceInterfaces<ITaskQueueTask> {
    META_INTERFACE(IntroduceInterfaces<ITaskQueueTask>, Task, "20779859-b4c9-426d-92f1-9143520e6917")

public:
    Task(Func func) : func_(BASE_NS::move(func)) {}

    bool Invoke() override
    {
        return func_();
    }

private:
    Func func_;
};

} // namespace

class API_TaskQueueTest : public ::testing::TestWithParam<ClassInfo> {
public:
    API_TaskQueueTest() : queue(GetObjectRegistry().Create<ITaskQueue>(GetParam())) {}

    void Process()
    {
        if (GetParam() == ClassId::PollingTaskQueue) {
            interface_cast<IPollingTaskQueue>(queue)->ProcessTasks();
        }
    }

    template<typename Func>
    ITaskQueue::Token AddTask(Func func)
    {
        return this->queue->AddTask(ITaskQueueTask::Ptr(new Task<Func>(func)));
    }

    template<typename Func>
    ITaskQueue::Token AddTask(Func func, TimeSpan span)
    {
        return this->queue->AddTask(ITaskQueueTask::Ptr(new Task<Func>(func)), span);
    }

    template<typename Func>
    bool TimedWait(size_t timeout, Func func)
    {
        bool ret = false;
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
        while (!(ret = func()) && std::chrono::steady_clock::now() < end) {
            Process();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return ret;
    }

    void Wait(size_t timeout)
    {
        auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeout);
        while (std::chrono::steady_clock::now() < end) {
            Process();
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }

    ITaskQueue::Ptr queue;
};

#define EXPECT_TRUE_TIMED(millis, arg) EXPECT_TRUE(this->TimedWait(millis, [&] { return (arg); }));

/**
 * @tc.name: SingleShot
 * @tc.desc: Tests for Single Shot. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_TaskQueueTest, SingleShot, testing::ext::TestSize.Level1)
{
    {
        std::atomic<int> count = 0;
        this->AddTask([&] {
            ++count;
            return false;
        });

        EXPECT_TRUE_TIMED(50, count == 1);
    }
    {
        std::atomic<int> count = 0;
        this->AddTask(
            [&] {
                ++count;
                return false;
            },
            150_ms);

        EXPECT_TRUE_TIMED(200, count == 1);
    }
}

/**
 * @tc.name: CurrentQueue
 * @tc.desc: Tests for Current Queue. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_TaskQueueTest, CurrentQueue, testing::ext::TestSize.Level1)
{
    {
        std::atomic<int> count = 0;
        this->AddTask([&] {
            EXPECT_EQ(GetTaskQueueRegistry().GetCurrentTaskQueue(), this->queue);
            ++count;
            return false;
        });

        EXPECT_TRUE_TIMED(50, count == 1);

        EXPECT_TRUE(!GetTaskQueueRegistry().GetCurrentTaskQueue());
    }
}

/**
 * @tc.name: Recurring
 * @tc.desc: Tests for Recurring. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_TaskQueueTest, Recurring, testing::ext::TestSize.Level1)
{
    std::atomic<int> count = 0;

    auto start = std::chrono::steady_clock::now();
    this->AddTask(
        [&] {
            ++count;
            return true;
        },
        75_ms);

    EXPECT_TRUE_TIMED(200, count == 2);

    auto end = std::chrono::steady_clock::now();
    EXPECT_TRUE((end - start) >= std::chrono::milliseconds(150));
}

/**
 * @tc.name: RecurringWithCancel
 * @tc.desc: Tests for Recurring With Cancel. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST_P(API_TaskQueueTest, RecurringWithCancel, testing::ext::TestSize.Level1)
{
    std::atomic<int> count = 0;
    ITaskQueue::Token token = this->AddTask(
        [&] {
            ++count;
            if (count == 2) {
                this->queue->CancelTask(token);
            }
            return true;
        },
        20_ms);

    Wait(200);
    this->queue->CancelTask(token); // no-op as the task is already cancelled.
    EXPECT_TRUE(count == 2);

    count = 0;
    token = this->AddTask(
        [&] {
            ++count;
            if (count == 3) {
                return false;
            }
            return true;
        },
        20_ms);
    Wait(200);
    this->queue->CancelTask(token); // no-op as the task is already cancelled.
    EXPECT_TRUE(count == 3);

    count = 0;
    token = this->AddTask(
        [&] {
            ++count;
            return true;
        },
        20_ms);
    Wait(200);
    this->queue->CancelTask(token); // actually cancels the task now.
    EXPECT_TRUE(count > 2);
    int curCount = count;
    Wait(100);
    EXPECT_EQ(count, curCount);
}

/**
 * @tc.name: AddWaitableTaskWithValue
 * @tc.desc: Tests for Add Waitable Task With Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, AddWaitableTaskWithValue, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f = queue->AddWaitableTask(CreateWaitableTask([&] { return 1; }));
    ASSERT_TRUE(f);
    EXPECT_EQ(f->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetState(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetResultOr<int>(0), 1);
}

/**
 * @tc.name: AddWaitableTaskWithoutValue
 * @tc.desc: Tests for Add Waitable Task Without Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, AddWaitableTaskWithoutValue, testing::ext::TestSize.Level1)
{
    std::atomic<int> count = 0;
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f = queue->AddWaitableTask(CreateWaitableTask([&] { ++count; }));
    ASSERT_TRUE(f);
    EXPECT_EQ(f->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetState(), IFuture::COMPLETED);
    auto v = f->GetResult();
    ASSERT_FALSE(v);
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: AddWaitableTaskTimedWait
 * @tc.desc: Tests for Add Waitable Task Timed Wait. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, AddWaitableTaskTimedWait, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto f = queue->AddWaitableTask(CreateWaitableTask([&] { return 1; }));
    ASSERT_TRUE(f);
    EXPECT_EQ(f->WaitFor(10_ms), IFuture::WAITING);
    EXPECT_EQ(f->GetState(), IFuture::WAITING);
    queue->ProcessTasks();
    EXPECT_EQ(f->WaitFor(10_ms), IFuture::COMPLETED);
    EXPECT_EQ(f->GetState(), IFuture::COMPLETED);
    auto v = f->GetResult();
    ASSERT_TRUE(v);
    auto tv = GetValue<int>(*v);
    EXPECT_EQ(tv, 1);
    EXPECT_EQ(f->GetResultOr<int>(0), 1);
    EXPECT_EQ(f->GetResultOr<uint8_t>(0), 0);
}

/**
 * @tc.name: AddWaitableTaskAbandoned
 * @tc.desc: Tests for Add Waitable Task Abandoned. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, AddWaitableTaskAbandoned, testing::ext::TestSize.Level1)
{
    IFuture::Ptr f;
    {
        auto queue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
        f = queue->AddWaitableTask(CreateWaitableTask([&] {}));
        ASSERT_TRUE(f);
        EXPECT_EQ(f->GetState(), IFuture::WAITING);
    }
    EXPECT_EQ(f->GetState(), IFuture::ABANDONED);
    // make sure the Wait doesn't block after the task has been destroyed
    EXPECT_EQ(f->WaitFor(10_ms), IFuture::ABANDONED);
    EXPECT_EQ(f->Wait(), IFuture::ABANDONED);
    EXPECT_FALSE(f->GetResult());
}

/**
 * @tc.name: ThenWithValue
 * @tc.desc: Tests for Then With Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ThenWithValue, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f = queue->AddWaitableTask(CreateWaitableTask([] { return 1; }))
                 ->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue);
    ASSERT_TRUE(f);
    EXPECT_EQ(f->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetState(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetResultOr<int>(0), 2);
}

/**
 * @tc.name: ThenWithValueInline
 * @tc.desc: Tests for Then With Value Inline. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ThenWithValueInline, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f = queue->AddWaitableTask(CreateWaitableTask([] { return 1; }))
                 ->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), nullptr);
    ASSERT_TRUE(f);
    EXPECT_EQ(f->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetState(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetResultOr<int>(0), 2);
}

/**
 * @tc.name: ThenWithoutValue
 * @tc.desc: Tests for Then Without Value. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ThenWithoutValue, testing::ext::TestSize.Level1)
{
    std::atomic<int> count = 0;
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f = queue->AddWaitableTask(CreateWaitableTask([&] { ++count; }))
                 ->Then(CreateContinuation([&](IAny::Ptr p) {
                     if (!p) {
                         ++count;
                     }
                 }),
                     queue);
    ASSERT_TRUE(f);
    EXPECT_EQ(f->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetState(), IFuture::COMPLETED);
    auto v = f->GetResult();
    ASSERT_FALSE(v);
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: ThenAbandoned
 * @tc.desc: Tests for Then Abandoned. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ThenAbandoned, testing::ext::TestSize.Level1)
{
    IFuture::Ptr f;
    {
        auto queue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
        f = queue->AddWaitableTask(CreateWaitableTask([] {}))->Then(CreateContinuation([](IAny::Ptr) {}), queue);
        ASSERT_TRUE(f);
        EXPECT_EQ(f->GetState(), IFuture::WAITING);
    }
    EXPECT_EQ(f->GetState(), IFuture::ABANDONED);
    // make sure the Wait doesn't block after the task has been destroyed
    EXPECT_EQ(f->WaitFor(10_ms), IFuture::ABANDONED);
    EXPECT_EQ(f->Wait(), IFuture::ABANDONED);
    EXPECT_FALSE(f->GetResult());
}

/**
 * @tc.name: MultiThen
 * @tc.desc: Tests for Multi Then. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, MultiThen, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f = queue->AddWaitableTask(CreateWaitableTask([] { return 1; }))
                 ->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue)
                 ->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue)
                 ->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue)
                 ->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue);
    ASSERT_TRUE(f);
    EXPECT_EQ(f->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetState(), IFuture::COMPLETED);
    EXPECT_EQ(f->GetResultOr<int>(0), 5);
}

/**
 * @tc.name: MultiThenOnSameFuture
 * @tc.desc: Tests for Multi Then On Same Future. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, MultiThenOnSameFuture, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto orig = queue->AddWaitableTask(CreateWaitableTask([] { return 1; }));
    auto f1 = orig->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue);
    auto f2 = orig->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 2; }), nullptr);
    auto f3 = orig->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 2; }), queue)
                  ->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue);
    auto f4 = orig->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 4; }), nullptr);
    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);
    ASSERT_TRUE(f3);
    ASSERT_TRUE(f4);
    EXPECT_EQ(f1->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f2->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f3->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f4->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f1->GetResultOr<int>(0), 2);
    EXPECT_EQ(f2->GetResultOr<int>(0), 3);
    EXPECT_EQ(f3->GetResultOr<int>(0), 4);
    EXPECT_EQ(f4->GetResultOr<int>(0), 5);
}

/**
 * @tc.name: ThenAndIntermediateResult
 * @tc.desc: Tests for Then And Intermediate Result. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ThenAndIntermediateResult, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f1 = queue->AddWaitableTask(CreateWaitableTask([] { return 1; }));
    auto f2 = f1->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue);

    ASSERT_TRUE(f1);
    ASSERT_TRUE(f2);
    EXPECT_EQ(f1->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f2->Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f1->GetState(), IFuture::COMPLETED);
    EXPECT_EQ(f2->GetState(), IFuture::COMPLETED);
    EXPECT_EQ(f1->GetResultOr<int>(0), 1);
    EXPECT_EQ(f2->GetResultOr<int>(0), 2);
}

/**
 * @tc.name: ThenOnSucceededFuture
 * @tc.desc: Tests for Then On Succeeded Future. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ThenOnSucceededFuture, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto f1 = queue->AddWaitableTask(CreateWaitableTask([] { return 1; }));
    f1->Wait();
    EXPECT_EQ(f1->GetState(), IFuture::COMPLETED);
    auto f2 = f1->Then(CreateContinuation([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }), queue);
    EXPECT_EQ(f2->GetResultOr<int>(0), 2);
}

/**
 * @tc.name: FutureCancel
 * @tc.desc: Tests for Future Cancel. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, FutureCancel, testing::ext::TestSize.Level1)
{
    {
        auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
        auto f = queue->AddWaitableTask(CreateWaitableTask([&] { return 1; }));
        f->Cancel();
        EXPECT_TRUE(f->GetState() == IFuture::COMPLETED || f->GetState() == IFuture::ABANDONED);
        if (f->GetState() == IFuture::COMPLETED) {
            EXPECT_EQ(f->GetResultOr<int>(0), 1);
        } else {
            EXPECT_EQ(f->GetResultOr<int>(0), 0);
        }
    }

    {
        std::atomic<int> value {};
        auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
        queue->AddWaitableTask(CreateWaitableTask([] { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }));
        auto f = queue->AddWaitableTask(CreateWaitableTask([&] {
            value = 1;
            return 1;
        }));
        f->Cancel();
        EXPECT_TRUE(f->GetState() == IFuture::ABANDONED);
        EXPECT_EQ(value, 0);
    }
    {
        std::atomic<int> value {};
        auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
        auto f = queue->AddWaitableTask(
            CreateWaitableTask([] { std::this_thread::sleep_for(std::chrono::milliseconds(20)); }));
        auto then = f->Then(CreateContinuation([&](IAny::Ptr) {
            value = 1;
            return 2;
        }),
            queue);
        f->Cancel();
        EXPECT_TRUE(f->GetState() != IFuture::WAITING);
        EXPECT_EQ(then->GetState(), IFuture::ABANDONED);
        EXPECT_EQ(value, 0);
        EXPECT_EQ(then->GetResultOr<int>(0), 0);
    }
}

/**
 * @tc.name: ThenOnAbandonedFuture
 * @tc.desc: Tests for Then On Abandoned Future. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ThenOnAbandonedFuture, testing::ext::TestSize.Level1)
{
    IFuture::Ptr f1;
    {
        auto queue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
        f1 = queue->AddWaitableTask(CreateWaitableTask([&] {}));
    }
    EXPECT_EQ(f1->GetState(), IFuture::ABANDONED);
    auto queue2 = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto f2 = f1->Then(CreateContinuation([](IAny::Ptr p) {}), queue2);
    EXPECT_EQ(f2->GetState(), IFuture::ABANDONED);
}

/**
 * @tc.name: CreateWaitableTask
 * @tc.desc: Tests for Create Waitable Task. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, CreateWaitableTask, testing::ext::TestSize.Level1)
{
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);

    IPromise::Ptr promise(GetObjectRegistry().Create<IPromise>(ClassId::Promise));
    ASSERT_TRUE(promise);
    BASE_NS::shared_ptr<PromisedQueueTask> task(
        new PromisedQueueTask(CreateWaitableTask([] { return 1; }), BASE_NS::move(promise)));
    auto f = task->GetFuture();
    ASSERT_TRUE(f);

    queue->AddTask(task);
    f->Wait();
    EXPECT_EQ(f->GetResultOr<int>(0), 1);
}

#ifdef DISABLED_TESTS_ON
/**
 * @tc.name: RecurringMultipleOrder
 * @tc.desc: Tests for Recurring Multiple Order. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, DISABLED_RecurringMultipleOrder, testing::ext::TestSize.Level1)
{
    // make sure that the order of recurring tasks does not change.
    auto queue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    std::atomic<int> count = 0;
    int task1 = 0;
    int task2 = 0;
    queue->AddTask(META_NS::MakeCallback<ITaskQueueTask>([&] {
        ++count;
        task1 = count;
        return true;
    }));
    queue->AddTask(META_NS::MakeCallback<ITaskQueueTask>([&] {
        ++count;
        task2 = count;
        return true;
    }));

    queue->ProcessTasks();
    EXPECT_TRUE(task1 == 1);
    EXPECT_TRUE(task2 == 2);
    queue->ProcessTasks();
    EXPECT_TRUE(task1 == 3);
    EXPECT_TRUE(task2 == 4);
    queue->ProcessTasks();
    EXPECT_TRUE(task1 == 5);
    EXPECT_TRUE(task2 == 6);
}
#endif // DISABLED_TESTS_ON

/**
 * @tc.name: ApiFuture
 * @tc.desc: Tests for Api Future. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, ApiFuture, testing::ext::TestSize.Level1)
{
    {
        Future<short> f(nullptr);
        EXPECT_FALSE(f);
        EXPECT_EQ(f.GetState(), IFuture::ABANDONED);
        EXPECT_EQ(f.Wait(), IFuture::ABANDONED);
        EXPECT_EQ(f.WaitFor(10_ms), IFuture::ABANDONED);
        EXPECT_EQ(f.GetResult(), 0);
        EXPECT_EQ(GetResultOr(f, 10), 10);
    }

    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    Future<int> f = queue->AddWaitableTask(CreateWaitableTask([&] { return 1; }));
    ASSERT_TRUE(f);
    EXPECT_EQ(f.Wait(), IFuture::COMPLETED);
    EXPECT_EQ(f.GetState(), IFuture::COMPLETED);
    EXPECT_EQ(f.GetResult(), 1);

    auto t1 = f.Then([](IAny::Ptr p) { return GetValue<int>(*p, 0) + 1; }, queue);
    EXPECT_EQ(t1.GetResult(), 2);

    auto t2 = t1.Then([](int v) { return v + 1; }, queue);
    EXPECT_EQ(t2.GetResult(), 3);

    auto t3 = t2.Then(
        [i = 0](int v) mutable {
            i = 1;
            return v + 1;
        },
        queue);
    EXPECT_EQ(t3.GetResult(), 4);

    auto t4 = t3.Then([](int v) { return v + 1; });
    EXPECT_EQ(t4.GetResult(), 5);

    auto t5 = t4.Then([i = 0](int v) mutable {
        i = 1;
        return v + 1;
    });
    EXPECT_EQ(t5.GetResult(), 6);

    Future<void> vf = queue->AddWaitableTask(CreateWaitableTask([] {}));
    ASSERT_TRUE(vf);
    EXPECT_EQ(vf.Wait(), IFuture::COMPLETED);
}

/**
 * @tc.name: AddFutureTaskOrRunDirectly
 * @tc.desc: Tests for Add Future Task Or Run Directly. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, AddFutureTaskOrRunDirectly, testing::ext::TestSize.Level1)
{
    std::atomic<int> value {};
    auto queue = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto f = AddFutureTaskOrRunDirectly(queue, [&] {
        ++value;
        auto fut = AddFutureTaskOrRunDirectly(queue, [&] { return 1; });
        EXPECT_EQ(fut.GetState(), IFuture::COMPLETED);
        EXPECT_EQ(fut.GetResult(), 1);
    });

    EXPECT_EQ(value, 0);
    queue->ProcessTasks();
    EXPECT_EQ(value, 1);
}

/**
 * @tc.name: CancelRunningTask
 * @tc.desc: Tests for Cancel Running Task. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, CancelRunningTask, testing::ext::TestSize.Level1)
{
    std::atomic<bool> start {};
    std::atomic<bool> done {};
    auto queue = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto h = queue->AddTask(MakeCallback<ITaskQueueTask>([&] {
        start = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        done = true;
        return false;
    }));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    queue->CancelTask(h);
    EXPECT_TRUE(start);
    EXPECT_TRUE(done);
}

/**
 * @tc.name: NestedTaskQueues
 * @tc.desc: Tests for Nested Task Queues. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, NestedTaskQueues, testing::ext::TestSize.Level1)
{
    auto top = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto nested = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);

    std::atomic<bool> done {};
    top->AddTask(MakeCallback<ITaskQueueTask>([&] {
        nested->ProcessTasks();

        auto f = AddFutureTaskOrRunDirectly(nested, [&] { done = true; });
        f.Wait();

        return false;
    }));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    EXPECT_TRUE(done);
}

/**
 * @tc.name: OutsideQueuesTask
 * @tc.desc: Tests for Outside Queues Task. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, OutsideQueuesTask, testing::ext::TestSize.Level1)
{
    auto q = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);

    std::atomic<bool> done {};

    q->ProcessTasks();

    auto f = AddFutureTaskOrRunDirectly(q, [&] { done = true; });
    f.Wait();

    EXPECT_TRUE(done);
}

/**
 * @tc.name: TaskDestructionTask
 * @tc.desc: Tests for Task Destruction Task. [AUTO-GENERATED]
 * @tc.type: FUNC
 */
UNIT_TEST(API_TaskQueueTest, TaskDestructionTask, testing::ext::TestSize.Level1)
{
    auto q = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);

    std::atomic<bool> done {};
    q->ProcessTasks();

    struct obj {
        obj(IPollingTaskQueue::Ptr queue, std::atomic<bool>& done) : queue(queue), done(done) {}
        ~obj()
        {
            auto f = AddFutureTaskOrRunDirectly(queue, [&] { done = true; });
            f.Wait();
        }
        IPollingTaskQueue::Ptr queue;
        std::atomic<bool>& done;
    };
    q->AddTask(MakeCallback<ITaskQueueTask>([o = obj { q, done }] { return false; }));
    q->ProcessTasks();

    EXPECT_TRUE(done);
}

INSTANTIATE_TEST_SUITE_P(
    API_TaskQueueTest, API_TaskQueueTest, ::testing::Values(ClassId::PollingTaskQueue, ClassId::ThreadedTaskQueue));

} // namespace UTest
META_END_NAMESPACE()
