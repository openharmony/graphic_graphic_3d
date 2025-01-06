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

#include <meta/api/object.h>
#include <meta/base/namespace.h>

#include "property_utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {

template<typename Type>
void GettingPropertyValue(benchmark::State& state, Type&& setValue)
{
    const auto value = setValue;
    for (auto _ : state) {
        state.PauseTiming();
        auto property = ConstructProperty<Type>("Property");
        property->SetValue(value);
        state.ResumeTiming();
        benchmark::DoNotOptimize(property->GetValue());
    }
}

template<typename Type>
void SettingPropertyValue(benchmark::State& state, Type&& setValue)
{
    const auto value = setValue;
    for (auto _ : state) {
        state.PauseTiming();
        auto property = ConstructProperty<Type>("Property");
        state.ResumeTiming();
        property->SetValue(value);
        state.PauseTiming();
        benchmark::DoNotOptimize(property->GetValue());
        state.ResumeTiming();
    }
}

template<typename Type>
void SettingPropertyValueWithDependency(benchmark::State& state, Type&& setValue)
{
    const auto value = setValue;
    for (auto _ : state) {
        state.PauseTiming();
        auto property = ConstructProperty<Type>("Property");
        auto property_dep = ConstructProperty<Type>("Property Dep");
        property_dep->SetBind(property);
        state.ResumeTiming();
        property->SetValue(value);
    }
}

template<typename Type>
void SettingPropertyValueWithDependencyChain(benchmark::State& state, Type&& setValue)
{
    const auto value = setValue;
    for (auto _ : state) {
        state.PauseTiming();
        auto property1 = ConstructProperty<Type>("Property 1");
        auto property2 = ConstructProperty<Type>("Property 2");
        auto property3 = ConstructProperty<Type>("Property 3");
        auto property4 = ConstructProperty<Type>("Property 4");

        property2->SetBind(property1);
        property3->SetBind(property2);
        property4->SetBind(property3);

        state.ResumeTiming();
        property1->SetValue(value);
    }
}

template<typename Type, typename Validator>
void SettingPropertyValueWithValidator(
    benchmark::State& state, Type&& setValue, BASE_NS::shared_ptr<Validator> validator)
{
    const auto value = setValue;
    for (auto _ : state) {
        state.PauseTiming();
        auto property = ConstructProperty<Type>("Property");
        property->SetValidator(validator);

        state.ResumeTiming();
        property->SetValue(value);
        benchmark::DoNotOptimize(property->GetValue());
    }
}

template<typename Type>
void BindingProperty(benchmark::State& state, Type&& p1Value)
{
    for (auto _ : state) {
        state.PauseTiming();

        auto property1 = ConstructProperty<Type>("Property 1", p1Value);
        auto property2 = ConstructProperty<Type>("Property 2");

        state.ResumeTiming();
        property2->SetBind(property1);
    }
}

template<typename Type>
void BindingPropertyChain(benchmark::State& state, Type&& p1Value)
{
    for (auto _ : state) {
        state.PauseTiming();

        auto property1 = ConstructProperty<Type>("Property 1", p1Value);
        auto property2 = ConstructProperty<Type>("Property 2");
        auto property3 = ConstructProperty<Type>("Property 3");
        auto property4 = ConstructProperty<Type>("Property 4");

        property3->SetBind(property4);
        property2->SetBind(property3);

        state.ResumeTiming();
        property1->SetBind(property2);
    }
}

template<typename Type>
void EvaluatingProperty(benchmark::State& state, Type&& setValue)
{
    for (auto _ : state) {
        state.PauseTiming();

        auto property1 = ConstructProperty<Type>("Property 1");
        auto property2 = ConstructProperty<Type>("Property 2");

        property2->SetBind(property1);
        property1->SetValue(setValue);

        state.ResumeTiming();
        benchmark::DoNotOptimize(property2->GetValue());
    }
}

template<typename Type>
void EvaluatingPropertyChain(benchmark::State& state, Type&& setValue)
{
    for (auto _ : state) {
        state.PauseTiming();

        auto property1 = ConstructProperty<Type>("Property 1");
        auto property2 = ConstructProperty<Type>("Property 2");
        auto property3 = ConstructProperty<Type>("Property 3");
        auto property4 = ConstructProperty<Type>("Property 4");

        property2->SetBind(property1);
        property3->SetBind(property2);
        property4->SetBind(property3);
        property1->SetValue(setValue);

        state.ResumeTiming();
        benchmark::DoNotOptimize(property4->GetValue());
    }
}

// Integer
BENCHMARK_CAPTURE(GettingPropertyValue, Integer, 10);
BENCHMARK_CAPTURE(SettingPropertyValue, Integer, 10);
BENCHMARK_CAPTURE(SettingPropertyValueWithDependency, Integer, 10);
BENCHMARK_CAPTURE(SettingPropertyValueWithDependencyChain, Integer, 10);
BENCHMARK_CAPTURE(BindingProperty, Integer, 10);
BENCHMARK_CAPTURE(BindingPropertyChain, Integer, 10);
BENCHMARK_CAPTURE(EvaluatingProperty, Integer, 10);
BENCHMARK_CAPTURE(EvaluatingPropertyChain, Integer, 10);

// IObject
BENCHMARK_CAPTURE(GettingPropertyValue, IObject, Object().GetIObject());
BENCHMARK_CAPTURE(SettingPropertyValue, IObject, Object().GetIObject());
BENCHMARK_CAPTURE(SettingPropertyValueWithDependency, IObject, Object().GetIObject());
BENCHMARK_CAPTURE(SettingPropertyValueWithDependencyChain, IObject, Object().GetIObject());
BENCHMARK_CAPTURE(BindingProperty, IObject, Object().GetIObject());
BENCHMARK_CAPTURE(BindingPropertyChain, IObject, Object().GetIObject());
BENCHMARK_CAPTURE(EvaluatingProperty, IObject, Object().GetIObject());
BENCHMARK_CAPTURE(EvaluatingPropertyChain, IObject, Object().GetIObject());

// Custom interface
BENCHMARK_CAPTURE(GettingPropertyValue, IBenchmarkType, CreateBenchmarkType());
BENCHMARK_CAPTURE(SettingPropertyValue, IBenchmarkType, CreateBenchmarkType());
BENCHMARK_CAPTURE(SettingPropertyValueWithDependency, IBenchmarkType, CreateBenchmarkType());
BENCHMARK_CAPTURE(SettingPropertyValueWithDependencyChain, IBenchmarkType, CreateBenchmarkType());
BENCHMARK_CAPTURE(BindingProperty, IBenchmarkType, CreateBenchmarkType());
BENCHMARK_CAPTURE(BindingPropertyChain, IBenchmarkType, CreateBenchmarkType());
BENCHMARK_CAPTURE(EvaluatingProperty, IBenchmarkType, CreateBenchmarkType());
BENCHMARK_CAPTURE(EvaluatingPropertyChain, IBenchmarkType, CreateBenchmarkType());

// Custom non-interface type
BENCHMARK_CAPTURE(GettingPropertyValue, BenchmarkEnum, BenchmarkEnumA);
BENCHMARK_CAPTURE(SettingPropertyValue, BenchmarkEnum, BenchmarkEnumA);
BENCHMARK_CAPTURE(SettingPropertyValueWithDependency, BenchmarkEnum, BenchmarkEnumA);
BENCHMARK_CAPTURE(SettingPropertyValueWithDependencyChain, BenchmarkEnum, BenchmarkEnumA);
BENCHMARK_CAPTURE(BindingProperty, BenchmarkEnum, BenchmarkEnumA);
BENCHMARK_CAPTURE(BindingPropertyChain, BenchmarkEnum, BenchmarkEnumA);
BENCHMARK_CAPTURE(EvaluatingProperty, BenchmarkEnum, BenchmarkEnumA);
BENCHMARK_CAPTURE(EvaluatingPropertyChain, BenchmarkEnum, BenchmarkEnumA);
} // namespace benchmarks
META_END_NAMESPACE()
