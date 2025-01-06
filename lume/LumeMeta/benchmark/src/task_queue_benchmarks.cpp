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

#include <benchmark/benchmark.h>
#include <deque>

#include <meta/api/make_callback.h>
#include <meta/api/object.h>
#include <meta/base/namespace.h>
#include <meta/interface/intf_task_queue.h>

#include "property_utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

void PushToTaskQueue(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto task = MakeCallback<ITaskQueueTask>([] { return false; });
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        q->ProcessTasks();
    }
}

void PushToTaskQueueWithLongLastingTasks(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<IPollingTaskQueue>(ClassId::PollingTaskQueue);
    auto task = MakeCallback<ITaskQueueTask>([] { return false; });
    for (int i = 0; i != 20; ++i) {
        q->AddTask(task, TimeSpan::Seconds(10));
    }
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        q->ProcessTasks();
    }
}

void PushToThreadedTaskQueue(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    std::atomic<int> value {};
    auto task = MakeCallback<ITaskQueueTask>([&] {
        ++value;
        return false;
    });
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        state.PauseTiming();
        while (value < 200) {
        }
        value = 0;
        state.ResumeTiming();
    }
}

void PushToThreadedTaskQueueLongLastingTasks(benchmark::State& state)
{
    auto q = GetObjectRegistry().Create<ITaskQueue>(ClassId::ThreadedTaskQueue);
    auto dummytask = MakeCallback<ITaskQueueTask>([] { return false; });
    for (int i = 0; i != 20; ++i) {
        q->AddTask(dummytask, TimeSpan::Seconds(10));
    }
    std::atomic<int> value {};
    auto task = MakeCallback<ITaskQueueTask>([&] {
        ++value;
        return false;
    });
    for (auto _ : state) {
        for (int i = 0; i != 200; ++i) {
            q->AddTask(task);
        }
        state.PauseTiming();
        while (value < 200) {
        }
        value = 0;
        state.ResumeTiming();
    }
}

BENCHMARK(PushToTaskQueue);
BENCHMARK(PushToTaskQueueWithLongLastingTasks);
BENCHMARK(PushToThreadedTaskQueue);
BENCHMARK(PushToThreadedTaskQueueLongLastingTasks);

} // namespace benchmarks
META_END_NAMESPACE()
