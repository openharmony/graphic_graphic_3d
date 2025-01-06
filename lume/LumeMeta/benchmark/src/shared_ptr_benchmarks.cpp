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

#include <meta/api/object.h>
#include <meta/base/namespace.h>

#include "utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

void SharedPtrConstruct(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::shared_ptr<IBenchmarkType> p(object.get());
        benchmark::DoNotOptimize(p);
    }
}

void SharedPtrCopy(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::shared_ptr<IBenchmarkType> p(object);
        benchmark::DoNotOptimize(p);
    }
}

void SharedPtrMove(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::shared_ptr<IBenchmarkType> p(BASE_NS::move(object));
        object = BASE_NS::move(p);
        benchmark::DoNotOptimize(p);
    }
}

void WeakPtrConstruct(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    for (auto _ : state) {
        BASE_NS::weak_ptr<IBenchmarkType> p(object);
        benchmark::DoNotOptimize(p);
    }
}

void WeakPtrLock(benchmark::State& state)
{
    auto object = CreateBenchmarkType();
    BASE_NS::weak_ptr<IBenchmarkType> weak(object);
    for (auto _ : state) {
        auto p = weak.lock();
        benchmark::DoNotOptimize(p);
    }
}

BENCHMARK(SharedPtrConstruct);
BENCHMARK(SharedPtrCopy);
BENCHMARK(SharedPtrMove);
BENCHMARK(WeakPtrConstruct);
BENCHMARK(WeakPtrLock);

} // namespace benchmarks
META_END_NAMESPACE()
