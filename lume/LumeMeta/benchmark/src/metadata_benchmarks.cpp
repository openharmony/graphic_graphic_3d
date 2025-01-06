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

void MetadataGetPropertyByName(benchmark::State& state)
{
    auto& objr = GetObjectRegistry();
    auto m = objr.Create<IMetadata>(ClassId::Object);
    for (int i = 0; i != 100; ++i) {
        auto name = "Thisispropertyname_" + std::to_string(i);
        m->AddProperty(ConstructProperty<float>(name.c_str()));
    }
    for (auto _ : state) {
        auto p = m->GetProperty("Thisispropertyname_50");
        if (!p) {
            state.SkipWithError("Could not find property");
            break;
        }
    }
}

void ConstructEmptyMetadata(benchmark::State& state)
{
    auto& objr = GetObjectRegistry();
    for (auto _ : state) {
        state.PauseTiming();
        BASE_NS::vector<IMetadata::Ptr> datas;
        datas.reserve(100);
        state.ResumeTiming();
        datas.push_back(objr.Create<IMetadata>(ClassId::Object));
    }
}

BENCHMARK(MetadataGetPropertyByName);
BENCHMARK(ConstructEmptyMetadata);

} // namespace benchmarks
META_END_NAMESPACE()
