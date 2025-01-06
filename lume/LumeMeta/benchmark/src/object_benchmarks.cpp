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
#include <meta/ext/object_fwd.h>

#include "property_utils.h"

META_BEGIN_NAMESPACE()
namespace benchmarks {
namespace {

#define TEST_INTERFACE(num)                                                                                   \
    class ITestInterface##num : public CORE_NS::IInterface {                                                  \
        META_INTERFACE(CORE_NS::IInterface, ITestInterface##num, MakeUid("ItestInterface" #num, "TestType")) \
    };

TEST_INTERFACE(1)
TEST_INTERFACE(2)
TEST_INTERFACE(3)
TEST_INTERFACE(4)
TEST_INTERFACE(5)
TEST_INTERFACE(6)
TEST_INTERFACE(7)
TEST_INTERFACE(8)
TEST_INTERFACE(9)
TEST_INTERFACE(10)

META_REGISTER_CLASS(ManyInterfacesTest, "6ac0d536-d0d9-4b4d-9db1-9833252f39de", ObjectCategoryBits::NO_CATEGORY)

struct ManyInterfacesTest
    : IntroduceInterfaces<ObjectFwd, ITestInterface1, ITestInterface2, ITestInterface3, ITestInterface4,
          ITestInterface5, ITestInterface6, ITestInterface7, ITestInterface8, ITestInterface9, ITestInterface10> {
    META_OBJECT(ManyInterfacesTest, ClassId::ManyInterfacesTest, IntroduceInterfaces)
};

} // namespace

void ObjectGetInterface(benchmark::State& state)
{
    auto obj = CreateBenchmarkType();
    for (auto _ : state) {
        benchmark::DoNotOptimize(obj->GetInterface(IAttachment::UID));
        benchmark::DoNotOptimize(obj->GetInterface(ILifecycle::UID));
    }
}

void ObjectGetInterfaceMany(benchmark::State& state)
{
    META_NS::RegisterObjectType<ManyInterfacesTest>();
    auto obj = GetObjectRegistry().Create(ClassId::ManyInterfacesTest);
    for (auto _ : state) {
        benchmark::DoNotOptimize(obj->GetInterface(ITestInterface1::UID));
        benchmark::DoNotOptimize(obj->GetInterface(ITestInterface10::UID));
    }
    META_NS::UnregisterObjectType<ManyInterfacesTest>();
}

class IUnknownInterface : public CORE_NS::IInterface {
    META_INTERFACE(CORE_NS::IInterface, IUnknownInterface, MakeUid("IUnknownInterface", "TestType"))
};

void ObjectGetInterfaceFail(benchmark::State& state)
{
    META_NS::RegisterObjectType<ManyInterfacesTest>();
    auto obj = GetObjectRegistry().Create(ClassId::ManyInterfacesTest);
    for (auto _ : state) {
        benchmark::DoNotOptimize(obj->GetInterface(IUnknownInterface::UID));
    }
    META_NS::UnregisterObjectType<ManyInterfacesTest>();
}

BENCHMARK(ObjectGetInterface);
BENCHMARK(ObjectGetInterfaceMany);
BENCHMARK(ObjectGetInterfaceFail);

} // namespace benchmarks
META_END_NAMESPACE()
