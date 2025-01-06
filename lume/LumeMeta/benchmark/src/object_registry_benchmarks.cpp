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

#include "property_utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

void ConstructObject(benchmark::State& state)
{
    auto& objr = GetObjectRegistry();
    for (auto _ : state) {
        auto obj = objr.Create(ClassId::BenchmarkType);
        state.PauseTiming();
        benchmark::DoNotOptimize(obj->GetName());
        state.ResumeTiming();
    }
}

void ConstructProperty(benchmark::State& state)
{
    for (auto _ : state) {
        auto property = META_NS::ConstructProperty<int>("Property");
        state.PauseTiming();
        benchmark::DoNotOptimize(property->GetValue());
        state.ResumeTiming();
    }
}

void ManyObjects(benchmark::State& state)
{
    constexpr size_t objectCount = 50000;
    auto& objr = GetObjectRegistry();
    objr.Purge(); // make sure there is nothing from other tests
    for (auto _ : state) {
        std::deque<IObject::Ptr> objects;
        for (size_t i = 0; i != objectCount; ++i) {
            objects.push_back(objr.Create(ClassId::BenchmarkType));
        }
    }
}

void ManyObjectsWithAddAndRemove(benchmark::State& state)
{
    constexpr size_t objectCount = 50000;
    auto& objr = GetObjectRegistry();
    objr.Purge();
    std::deque<IObject::Ptr> objects;
    for (size_t i = 0; i != objectCount; ++i) {
        // reverse, so when we remove from the back, it is the last one created
        objects.push_front(objr.Create(ClassId::BenchmarkType));
    }
    for (auto _ : state) {
        for (size_t i = 0; i != objectCount; ++i) {
            objects.pop_back();
            objects.push_back(objr.Create(ClassId::BenchmarkType));
        }
    }
}

BENCHMARK(ConstructObject);
BENCHMARK(ConstructProperty);
BENCHMARK(ManyObjects);
BENCHMARK(ManyObjectsWithAddAndRemove);

} // namespace benchmarks
META_END_NAMESPACE()
